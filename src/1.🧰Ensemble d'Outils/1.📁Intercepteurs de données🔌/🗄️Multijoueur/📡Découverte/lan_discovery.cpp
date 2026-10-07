#include "lan_discovery.hpp"

#include "../🔐Chiffrement/secure_channel.hpp"
#include "../🧩Commun/multiplayer_common.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
/// En-tête de chaque paquet de découverte : "GTKD" puis la version du format.
const uint8_t MAGIC[4] = {'G', 'T', 'K', 'D'};
constexpr uint8_t FORMAT_VERSION = 1;
constexpr int HEADER_SIZE = 6; // magic (4) + version (1) + type (1)
/// Un appareil qui ne nous a plus interrogés depuis ce délai n'est plus prévenu des changements.
constexpr uint64_t SEARCHER_TIMEOUT_MSEC = 10000;
/// Clés d'un appareil qui changent à chaque annonce : ignorées pour décider d'émettre device_updated.
const char *const VOLATILE_KEYS[] = {"last_seen_msec", "latency_ms"};

uint64_t now_msec()
{
    return Time::get_singleton()->get_ticks_msec();
}
} // namespace

LanDiscovery::LanDiscovery()
{
    device_id = SecureChannel::random_bytes(8).hex_encode();
    extra_query_addresses.push_back("127.0.0.1");
}

#pragma region Cycle de vie

void LanDiscovery::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_READY:
        if (autostart && !Engine::get_singleton()->is_editor_hint())
        {
            start();
        }
        break;
    case NOTIFICATION_EXIT_TREE:
        stop();
        break;
    default:
        break;
    }
}

Error LanDiscovery::start()
{
    if (running)
    {
        return OK;
    }
    const Error err = _open_socket();
    if (err != OK)
    {
        return err;
    }
    running = true;
    set_process(true);
    return OK;
}

void LanDiscovery::stop()
{
    if (!running)
    {
        return;
    }
    _announce_to_searchers(PACKET_BYE);
    _close_socket();
    running = false;
    searching = false;
    set_process(false);

    // Copie : un récepteur de device_lost peut relire la liste.
    const std::map<String, Dictionary> lost = devices;
    devices.clear();
    searchers.clear();
    for (const auto &entry : lost)
    {
        emit_signal("device_lost", entry.second);
    }
}

Error LanDiscovery::start_search()
{
    const Error err = start();
    if (err != OK)
    {
        return err;
    }
    searching = true;
    refresh();
    return OK;
}

void LanDiscovery::stop_search()
{
    searching = false;
}

void LanDiscovery::refresh()
{
    if (running)
    {
        _send_query();
        query_cooldown = query_interval;
    }
}

Error LanDiscovery::_open_socket()
{
    socket.instantiate();
    socket->set_broadcast_enabled(true);
    bound_port = 0;

    if (advertise)
    {
        // Plusieurs instances sur la même machine : chacune prend le premier port libre de la plage.
        for (int candidate = port; candidate < port + port_range; candidate++)
        {
            if (socket->bind(candidate) == OK)
            {
                bound_port = candidate;
                return OK;
            }
        }
        UtilityFunctions::push_warning(String(U"LanDiscovery : aucun port libre entre {0} et {1}, l'appareil ne sera pas visible (recherche toujours possible).")
                                           .format(Array::make(port, port + port_range - 1)));
        socket.instantiate();
        socket->set_broadcast_enabled(true);
    }

    // Recherche seule : port quelconque, les réponses reviennent à l'adresse d'envoi.
    const Error err = socket->bind(0);
    ERR_FAIL_COND_V_MSG(err != OK, err, U"LanDiscovery : impossible d'ouvrir un port UDP.");
    bound_port = socket->get_local_port();
    return OK;
}

void LanDiscovery::_close_socket()
{
    if (socket.is_valid())
    {
        socket->close();
        socket.unref();
    }
    bound_port = 0;
}

void LanDiscovery::_process(double p_delta)
{
    if (!running)
    {
        return;
    }
    _receive_packets();

    if (searching)
    {
        query_cooldown -= p_delta;
        if (query_cooldown <= 0.0)
        {
            refresh();
        }
    }
    _expire_devices();
}

#pragma endregion

#pragma region Envoi

void LanDiscovery::_send(const String &p_address, int p_port, PacketType p_type, const Dictionary &p_payload)
{
    if (socket.is_null())
    {
        return;
    }
    PackedByteArray packet;
    for (uint8_t byte : MAGIC)
    {
        packet.push_back(byte);
    }
    packet.push_back(FORMAT_VERSION);
    packet.push_back(p_type);
    packet.append_array(UtilityFunctions::var_to_bytes(p_payload));

    socket->set_dest_address(p_address, p_port);
    socket->put_packet(packet);
}

void LanDiscovery::_send_query()
{
    Dictionary query;
    query["device_id"] = device_id;
    query["game_id"] = get_game_id();
    query["sent_msec"] = (int64_t)now_msec();

    PackedStringArray addresses;
    addresses.push_back("255.255.255.255");
    addresses.append_array(extra_query_addresses);
    for (const String &address : addresses)
    {
        for (int target = port; target < port + port_range; target++)
        {
            _send(address, target, PACKET_QUERY, query);
        }
    }
}

void LanDiscovery::_announce_to_searchers(PacketType p_type)
{
    if (!running || !advertise)
    {
        return;
    }
    Dictionary payload = p_type == PACKET_BYE ? Dictionary() : _build_announce();
    payload["device_id"] = device_id;
    for (const auto &entry : searchers)
    {
        _send(entry.second.address, entry.second.port, p_type, payload);
    }
}

Dictionary LanDiscovery::_build_announce() const
{
    Dictionary announce = get_local_device_info();
    announce.erase("address"); // l'adresse est celle vue par le destinataire
    return announce;
}

#pragma endregion

#pragma region Réception

void LanDiscovery::_receive_packets()
{
    while (socket.is_valid() && socket->get_available_packet_count() > 0)
    {
        const PackedByteArray packet = socket->get_packet();
        const String address = socket->get_packet_ip();
        const int source_port = socket->get_packet_port();

        // Paquet d'un autre logiciel sur le même port, ou d'une autre version du format : ignoré.
        if (packet.size() <= HEADER_SIZE || packet[4] != FORMAT_VERSION)
        {
            continue;
        }
        bool magic_ok = true;
        for (int i = 0; i < 4; i++)
        {
            magic_ok = magic_ok && packet[i] == MAGIC[i];
        }
        if (!magic_ok)
        {
            continue;
        }
        const Variant decoded = UtilityFunctions::bytes_to_var(packet.slice(HEADER_SIZE)); // sans objets : sûr
        if (decoded.get_type() != Variant::DICTIONARY)
        {
            continue;
        }
        const Dictionary payload = decoded;
        if (String(payload.get("device_id", "")) == device_id)
        {
            continue; // notre propre requête, revenue par le broadcast
        }

        switch (packet[5])
        {
        case PACKET_QUERY:
            _handle_query(payload, address, source_port);
            break;
        case PACKET_ANNOUNCE:
            _handle_announce(payload, address);
            break;
        case PACKET_BYE:
            _handle_bye(payload);
            break;
        default:
            break;
        }
    }
}

void LanDiscovery::_handle_query(const Dictionary &p_payload, const String &p_address, int p_port)
{
    if (!advertise || bound_port < port || bound_port >= port + port_range)
    {
        return; // invisible : pas d'annonce, ou port hors de la plage de découverte
    }
    const String key = p_address + String(":") + String::num_int64(p_port);
    searchers[key] = Searcher{p_address, p_port, now_msec()};

    Dictionary announce = _build_announce();
    announce["echo_msec"] = p_payload.get("sent_msec", 0);
    _send(p_address, p_port, PACKET_ANNOUNCE, announce);
}

void LanDiscovery::_handle_announce(const Dictionary &p_payload, const String &p_address)
{
    const String id = p_payload.get("device_id", "");
    if (id.is_empty())
    {
        return;
    }
    const Compatibility compatibility = _compatibility_of(p_payload);
    if (compatibility == OTHER_GAME && !include_other_games)
    {
        return;
    }

    Dictionary device = p_payload.duplicate(true);
    device.erase("echo_msec");
    const auto existing = devices.find(id);
    // Un appareil de la même machine répond au broadcast ET à 127.0.0.1 : on garde la première adresse.
    device["address"] = existing != devices.end() ? existing->second.get("address", p_address) : Variant(p_address);
    device["compatibility"] = compatibility;

    const Dictionary session = device.get("session", Dictionary());
    const bool hosting = session.get("hosting", false);
    const int players = session.get("players", 0);
    const int max_players = session.get("max_players", 0);
    device["can_join"] = hosting && compatibility == COMPATIBLE && (max_players <= 0 || players < max_players);

    device["last_seen_msec"] = (int64_t)now_msec();
    const int64_t echo = p_payload.get("echo_msec", 0);
    if (echo > 0)
    {
        device["latency_ms"] = (int64_t)now_msec() - echo;
    }
    else if (existing != devices.end())
    {
        device["latency_ms"] = existing->second.get("latency_ms", -1);
    }
    else
    {
        device["latency_ms"] = -1;
    }

    if (existing == devices.end())
    {
        devices[id] = device;
        emit_signal("device_found", device);
        return;
    }

    // device_updated seulement si quelque chose d'utile a changé (pas pour l'heure de la dernière réponse).
    Dictionary before = existing->second.duplicate();
    Dictionary after = device.duplicate();
    for (const char *volatile_key : VOLATILE_KEYS)
    {
        before.erase(volatile_key);
        after.erase(volatile_key);
    }
    existing->second = device;
    if (!before.recursive_equal(after, 4))
    {
        emit_signal("device_updated", device);
    }
}

void LanDiscovery::_handle_bye(const Dictionary &p_payload)
{
    const auto found = devices.find(String(p_payload.get("device_id", "")));
    if (found == devices.end())
    {
        return;
    }
    const Dictionary device = found->second;
    devices.erase(found);
    emit_signal("device_lost", device);
}

void LanDiscovery::_expire_devices()
{
    const uint64_t now = now_msec();
    const uint64_t timeout = (uint64_t)(device_timeout * 1000.0f);

    for (auto it = devices.begin(); it != devices.end();)
    {
        const uint64_t last_seen = (int64_t)it->second.get("last_seen_msec", 0);
        if (now - last_seen > timeout)
        {
            const Dictionary device = it->second;
            it = devices.erase(it);
            emit_signal("device_lost", device);
        }
        else
        {
            ++it;
        }
    }
    for (auto it = searchers.begin(); it != searchers.end();)
    {
        it = now - it->second.last_query_msec > SEARCHER_TIMEOUT_MSEC ? searchers.erase(it) : std::next(it);
    }
}

LanDiscovery::Compatibility LanDiscovery::_compatibility_of(const Dictionary &p_announce) const
{
    if ((int)p_announce.get("protocol", -1) != multiplayer_common::PROTOCOL_VERSION)
    {
        return PROTOCOL_MISMATCH;
    }
    if (String(p_announce.get("game_id", "")) != get_game_id())
    {
        return OTHER_GAME;
    }
    if (String(p_announce.get("game_version", "")) != get_game_version())
    {
        return VERSION_MISMATCH;
    }
    return COMPATIBLE;
}

#pragma endregion

#pragma region Lecture

Array LanDiscovery::get_devices() const
{
    Array list;
    for (const auto &entry : devices)
    {
        list.push_back(entry.second.duplicate(true));
    }
    return list;
}

Array LanDiscovery::get_joinable_devices() const
{
    Array list;
    for (const auto &entry : devices)
    {
        if (entry.second.get("can_join", false))
        {
            list.push_back(entry.second.duplicate(true));
        }
    }
    return list;
}

Dictionary LanDiscovery::get_device(const String &p_device_id) const
{
    const auto found = devices.find(p_device_id);
    return found != devices.end() ? found->second.duplicate(true) : Dictionary();
}

Dictionary LanDiscovery::get_local_device_info() const
{
    Dictionary info = multiplayer_common::local_system_info();
    info["device_id"] = device_id;
    info["name"] = get_device_name();
    info["game_id"] = get_game_id();
    info["game_version"] = get_game_version();
    info["protocol"] = multiplayer_common::PROTOCOL_VERSION;
    Dictionary session = session_info.duplicate(true);
    if (!session.has("hosting"))
    {
        session["hosting"] = false;
    }
    info["session"] = session;
    info["address"] = "127.0.0.1";
    return info;
}

void LanDiscovery::set_session_info(const Dictionary &p_info)
{
    if (session_info.recursive_equal(p_info, 4))
    {
        return;
    }
    session_info = p_info.duplicate(true);
    _announce_to_searchers(PACKET_ANNOUNCE);
}

void LanDiscovery::clear_session_info()
{
    set_session_info(Dictionary());
}

#pragma endregion

#pragma region Getter / Setter

void LanDiscovery::set_advertise(bool p_advertise)
{
    if (advertise == p_advertise)
    {
        return;
    }
    if (running)
    {
        // Change de port : celui de la plage de découverte pour être visible, un port quelconque sinon.
        if (!p_advertise)
        {
            _announce_to_searchers(PACKET_BYE);
        }
        advertise = p_advertise;
        _close_socket();
        _open_socket();
        return;
    }
    advertise = p_advertise;
}

void LanDiscovery::set_device_name(const String &p_name)
{
    device_name = p_name;
    _announce_to_searchers(PACKET_ANNOUNCE);
}

String LanDiscovery::get_device_name() const
{
    return device_name.is_empty() ? multiplayer_common::default_device_name() : device_name;
}

void LanDiscovery::set_game_id(const String &p_id)
{
    game_id = p_id;
}

String LanDiscovery::get_game_id() const
{
    return game_id.is_empty() ? multiplayer_common::default_game_id() : game_id;
}

void LanDiscovery::set_game_version(const String &p_version)
{
    game_version = p_version;
}

String LanDiscovery::get_game_version() const
{
    return game_version.is_empty() ? multiplayer_common::default_game_version() : game_version;
}

#pragma endregion

void LanDiscovery::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("start"), &LanDiscovery::start);
    ClassDB::bind_method(D_METHOD("stop"), &LanDiscovery::stop);
    ClassDB::bind_method(D_METHOD("is_running"), &LanDiscovery::is_running);
    ClassDB::bind_method(D_METHOD("start_search"), &LanDiscovery::start_search);
    ClassDB::bind_method(D_METHOD("stop_search"), &LanDiscovery::stop_search);
    ClassDB::bind_method(D_METHOD("is_searching"), &LanDiscovery::is_searching);
    ClassDB::bind_method(D_METHOD("refresh"), &LanDiscovery::refresh);

    ClassDB::bind_method(D_METHOD("get_devices"), &LanDiscovery::get_devices);
    ClassDB::bind_method(D_METHOD("get_joinable_devices"), &LanDiscovery::get_joinable_devices);
    ClassDB::bind_method(D_METHOD("get_device", "device_id"), &LanDiscovery::get_device);
    ClassDB::bind_method(D_METHOD("get_device_count"), &LanDiscovery::get_device_count);
    ClassDB::bind_method(D_METHOD("get_local_device_info"), &LanDiscovery::get_local_device_info);
    ClassDB::bind_method(D_METHOD("get_device_id"), &LanDiscovery::get_device_id);
    ClassDB::bind_method(D_METHOD("get_bound_port"), &LanDiscovery::get_bound_port);

    ClassDB::bind_method(D_METHOD("set_session_info", "info"), &LanDiscovery::set_session_info);
    ClassDB::bind_method(D_METHOD("get_session_info"), &LanDiscovery::get_session_info);
    ClassDB::bind_method(D_METHOD("clear_session_info"), &LanDiscovery::clear_session_info);

    ClassDB::bind_method(D_METHOD("set_port", "port"), &LanDiscovery::set_port);
    ClassDB::bind_method(D_METHOD("get_port"), &LanDiscovery::get_port);
    ClassDB::bind_method(D_METHOD("set_port_range", "range"), &LanDiscovery::set_port_range);
    ClassDB::bind_method(D_METHOD("get_port_range"), &LanDiscovery::get_port_range);
    ClassDB::bind_method(D_METHOD("set_advertise", "advertise"), &LanDiscovery::set_advertise);
    ClassDB::bind_method(D_METHOD("is_advertising"), &LanDiscovery::is_advertising);
    ClassDB::bind_method(D_METHOD("set_autostart", "autostart"), &LanDiscovery::set_autostart);
    ClassDB::bind_method(D_METHOD("is_autostart"), &LanDiscovery::is_autostart);
    ClassDB::bind_method(D_METHOD("set_query_interval", "interval"), &LanDiscovery::set_query_interval);
    ClassDB::bind_method(D_METHOD("get_query_interval"), &LanDiscovery::get_query_interval);
    ClassDB::bind_method(D_METHOD("set_device_timeout", "timeout"), &LanDiscovery::set_device_timeout);
    ClassDB::bind_method(D_METHOD("get_device_timeout"), &LanDiscovery::get_device_timeout);
    ClassDB::bind_method(D_METHOD("set_device_name", "name"), &LanDiscovery::set_device_name);
    ClassDB::bind_method(D_METHOD("get_device_name"), &LanDiscovery::get_device_name);
    ClassDB::bind_method(D_METHOD("set_game_id", "id"), &LanDiscovery::set_game_id);
    ClassDB::bind_method(D_METHOD("get_game_id"), &LanDiscovery::get_game_id);
    ClassDB::bind_method(D_METHOD("set_game_version", "version"), &LanDiscovery::set_game_version);
    ClassDB::bind_method(D_METHOD("get_game_version"), &LanDiscovery::get_game_version);
    ClassDB::bind_method(D_METHOD("set_extra_query_addresses", "addresses"), &LanDiscovery::set_extra_query_addresses);
    ClassDB::bind_method(D_METHOD("get_extra_query_addresses"), &LanDiscovery::get_extra_query_addresses);
    ClassDB::bind_method(D_METHOD("set_include_other_games", "include"), &LanDiscovery::set_include_other_games);
    ClassDB::bind_method(D_METHOD("is_including_other_games"), &LanDiscovery::is_including_other_games);

    ADD_GROUP(U"Identité", "");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "device_name", PROPERTY_HINT_PLACEHOLDER_TEXT, U"Utilisateur du système"), "set_device_name", "get_device_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "game_id", PROPERTY_HINT_PLACEHOLDER_TEXT, "application/config/name"), "set_game_id", "get_game_id");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "game_version", PROPERTY_HINT_PLACEHOLDER_TEXT, "application/config/version"), "set_game_version", "get_game_version");

    ADD_GROUP(U"Réseau", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "port", PROPERTY_HINT_RANGE, "1024,65535"), "set_port", "get_port");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "port_range", PROPERTY_HINT_RANGE, "1,32"), "set_port_range", "get_port_range");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "extra_query_addresses"), "set_extra_query_addresses", "get_extra_query_addresses");

    ADD_GROUP(U"Comportement", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autostart"), "set_autostart", "is_autostart");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "advertise"), "set_advertise", "is_advertising");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "include_other_games"), "set_include_other_games", "is_including_other_games");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "query_interval", PROPERTY_HINT_RANGE, "0.05,10,0.05,suffix:s"), "set_query_interval", "get_query_interval");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "device_timeout", PROPERTY_HINT_RANGE, "0.1,60,0.1,suffix:s"), "set_device_timeout", "get_device_timeout");

    ADD_SIGNAL(MethodInfo("device_found", PropertyInfo(Variant::DICTIONARY, "device")));
    ADD_SIGNAL(MethodInfo("device_updated", PropertyInfo(Variant::DICTIONARY, "device")));
    ADD_SIGNAL(MethodInfo("device_lost", PropertyInfo(Variant::DICTIONARY, "device")));

    BIND_ENUM_CONSTANT(COMPATIBLE);
    BIND_ENUM_CONSTANT(VERSION_MISMATCH);
    BIND_ENUM_CONSTANT(OTHER_GAME);
    BIND_ENUM_CONSTANT(PROTOCOL_MISMATCH);
}
