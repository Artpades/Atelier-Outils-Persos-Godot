#include "network_session.hpp"

#include "../📡Découverte/lan_discovery.hpp"
#include "../🧩Commun/multiplayer_common.hpp"

#include <godot_cpp/classes/crypto.hpp>
#include <godot_cpp/classes/e_net_connection.hpp>
#include <godot_cpp/classes/e_net_multiplayer_peer.hpp>
#include <godot_cpp/classes/e_net_packet_peer.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/hashing_context.hpp>
#include <godot_cpp/classes/offline_multiplayer_peer.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/web_socket_multiplayer_peer.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <iterator>

using namespace godot;
using namespace godot::multiplayer_common;

namespace
{
/// Premier octet des paquets du module envoyés par send_bytes : les autres paquets (ceux du jeu) sont ignorés.
constexpr uint8_t PACKET_MAGIC = 0xC7;
/// Itérations PBKDF2 du mot de passe de la partie.
constexpr int PASSWORD_ITERATIONS = 10000;
/// Délai laissé à un client refusé ou expulsé pour recevoir la raison avant la déconnexion.
constexpr uint64_t KICK_DELAY_MSEC = 500;
/// Messages gardés en attente par joueur (clé de bout en bout pas encore échangée).
constexpr size_t MAX_PENDING_MESSAGES = 256;
constexpr int MAX_TEXT_LENGTH = 64;

/// Signaux de SceneMultiplayer écoutés pendant une session.
const char *const API_SIGNALS[] = {
    "peer_authenticating", "peer_authentication_failed", "peer_connected", "peer_disconnected",
    "peer_packet", "connected_to_server", "connection_failed", "server_disconnected",
};

uint64_t now_msec()
{
    return Time::get_singleton()->get_ticks_msec();
}

String transport_name(NetworkSession::Transport p_transport)
{
    return p_transport == NetworkSession::TRANSPORT_WEBSOCKET ? "websocket" : "enet";
}
} // namespace

NetworkSession::NetworkSession()
{
}

void NetworkSession::_notification(int p_what)
{
    if (p_what == NOTIFICATION_EXIT_TREE)
    {
        close();
    }
}

#pragma region Démarrage et arrêt

Error NetworkSession::host(int p_port)
{
    ERR_FAIL_COND_V_MSG(!is_inside_tree(), ERR_UNCONFIGURED, U"NetworkSession : le nœud doit être dans l'arbre pour héberger.");
    close();

    const int use_port = p_port > 0 ? p_port : port;
    // Clé d'identité : signe le défi (les clients vérifient l'identité du serveur) et sert au bout en bout.
    if (end_to_end_encryption || access_policy.is_valid() || dedicated_server)
    {
        ERR_FAIL_COND_V(_ensure_identity_key() != OK, ERR_CANT_CREATE);
    }
    if (transport_encryption)
    {
        ERR_FAIL_COND_V(_ensure_transport_certificate() != OK, ERR_CANT_CREATE);
    }

    Error err = OK;
    if (transport == TRANSPORT_WEBSOCKET)
    {
        Ref<WebSocketMultiplayerPeer> websocket;
        websocket.instantiate();
        err = websocket->create_server(use_port, "*", transport_encryption ? TLSOptions::server(transport_key, transport_certificate) : Ref<TLSOptions>());
        peer = websocket;
    }
    else
    {
        Ref<ENetMultiplayerPeer> enet;
        enet.instantiate();
        // Une place de plus que de clients admis : le client en trop est prévenu (« partie complète »)
        // au lieu d'être refusé sans explication par ENet. L'hôte d'une partie occupe une place, pas un serveur dédié.
        err = enet->create_server(use_port, max_players + (dedicated_server ? 1 : 0));
        if (err == OK && transport_encryption)
        {
            err = enet->get_host()->dtls_server_setup(TLSOptions::server(transport_key, transport_certificate));
        }
        peer = enet;
    }
    if (err != OK)
    {
        peer.unref();
        last_reason = REASON_CONNECTION_FAILED;
        last_message = String(U"Impossible d'héberger sur le port {0} ({1}).").format(Array::make(use_port, UtilityFunctions::error_string(err)));
        UtilityFunctions::push_warning("NetworkSession : " + last_message);
        return err;
    }

    err = _attach_api();
    if (err != OK)
    {
        peer->close();
        peer.unref();
        return err;
    }
    api->set_multiplayer_peer(peer);
    if (access_policy.is_valid() && access_policy->is_approval_required())
    {
        // La validation manuelle peut prendre du temps : la poignée de main ne doit pas expirer avant.
        api->set_auth_timeout(MAX(connection_timeout, access_policy->get_approval_timeout() + 5.0f));
    }

    session_e2e = end_to_end_encryption;
    session_dedicated = dedicated_server;
    failures.clear();
    session_transport_encrypted = transport_encryption;
    session_password = !password.is_empty();
    password_salt = SecureChannel::random_bytes(16);
    password_key = session_password ? SecureChannel::derive_key(password, password_salt, PASSWORD_ITERATIONS, 32) : PackedByteArray();
    active_port = use_port;
    last_reason = REASON_NONE;
    last_message = "";

    _set_state(STATE_HOSTING);
    emit_signal("hosting_started", use_port);

    if (!dedicated_server)
    {
        Dictionary wire_info = _local_player_info();
        wire_info["peer_id"] = 1;
        wire_info["role"] = ServerAccessPolicy::ROLE_ADMIN;
        _add_player(1, wire_info);
    }
    _update_discovery();
    return OK;
}

Error NetworkSession::join(const String &p_address, int p_port)
{
    return _start_join(p_address, p_port > 0 ? p_port : port, transport, transport_encryption);
}

Error NetworkSession::join_device(const Dictionary &p_device)
{
    const Dictionary session = p_device.get("session", Dictionary());
    ERR_FAIL_COND_V_MSG(!(bool)session.get("hosting", false), ERR_INVALID_PARAMETER, U"NetworkSession : cet appareil n'héberge pas de partie.");

    const bool remote_encrypted = session.get("transport_encryption", false);
    if (transport_encryption && !remote_encrypted)
    {
        // Jamais de rétrogradation silencieuse : un client qui exige le chiffrement refuse un hôte qui ne l'a pas.
        last_reason = REASON_SECURITY_MISMATCH;
        last_message = U"L'hôte ne chiffre pas le transport, alors que ce client l'exige.";
        emit_signal("connection_failed", last_reason, last_message);
        return ERR_UNAUTHORIZED;
    }
    const Transport remote_transport = String(session.get("transport", "enet")) == "websocket" ? TRANSPORT_WEBSOCKET : TRANSPORT_ENET;
    return _start_join(p_device.get("address", ""), session.get("port", port), remote_transport, remote_encrypted);
}

Error NetworkSession::_start_join(const String &p_address, int p_port, Transport p_transport, bool p_encrypted)
{
    ERR_FAIL_COND_V_MSG(!is_inside_tree(), ERR_UNCONFIGURED, U"NetworkSession : le nœud doit être dans l'arbre pour rejoindre une partie.");
    ERR_FAIL_COND_V_MSG(p_address.is_empty(), ERR_INVALID_PARAMETER, U"NetworkSession : adresse vide.");
    close();

    if (end_to_end_encryption)
    {
        // Générée maintenant plutôt que pendant la poignée de main (environ 0,4 s).
        ERR_FAIL_COND_V(_ensure_identity_key() != OK, ERR_CANT_CREATE);
    }

    Error err = OK;
    if (p_transport == TRANSPORT_WEBSOCKET)
    {
        Ref<WebSocketMultiplayerPeer> websocket;
        websocket.instantiate();
        String url = p_address;
        if (!url.begins_with("ws://") && !url.begins_with("wss://"))
        {
            url = String(p_encrypted ? "wss://" : "ws://") + p_address + ":" + String::num_int64(p_port);
        }
        p_encrypted = url.begins_with("wss://");
        err = websocket->create_client(url, p_encrypted ? _client_tls_options() : Ref<TLSOptions>());
        peer = websocket;
    }
    else
    {
        Ref<ENetMultiplayerPeer> enet;
        enet.instantiate();
        err = enet->create_client(p_address, p_port);
        if (err == OK && p_encrypted)
        {
            err = enet->get_host()->dtls_client_setup(certificate_common_name, _client_tls_options());
        }
        peer = enet;
    }
    if (err != OK)
    {
        peer.unref();
        last_reason = REASON_CONNECTION_FAILED;
        last_message = String(U"Impossible de se connecter à {0} ({1}).").format(Array::make(p_address, UtilityFunctions::error_string(err)));
        emit_signal("connection_failed", last_reason, last_message);
        return err;
    }

    err = _attach_api();
    if (err != OK)
    {
        peer->close();
        peer.unref();
        return err;
    }
    api->set_multiplayer_peer(peer);

    session_e2e = false; // décidé par l'hôte dans le défi
    session_dedicated = false;
    server_fingerprint = "";
    server_verified = false;
    approval_deadline_msec = 0;
    session_transport_encrypted = p_encrypted;
    session_password = false;
    active_port = p_port;
    last_reason = REASON_NONE;
    last_message = "";
    connect_start_msec = now_msec();
    _set_state(STATE_CONNECTING);
    return OK;
}

void NetworkSession::close()
{
    if (state != STATE_OFFLINE)
    {
        _end(REASON_CLOSED, U"Session fermée.");
    }
}

Error NetworkSession::_attach_api()
{
    api = Ref<SceneMultiplayer>(Object::cast_to<SceneMultiplayer>(get_multiplayer().ptr()));
    ERR_FAIL_COND_V_MSG(api.is_null(), ERR_UNAVAILABLE, U"NetworkSession : le MultiplayerAPI de cette branche doit être un SceneMultiplayer.");

    api->set_auth_callback(callable_mp(this, &NetworkSession::_on_auth));
    api->set_auth_timeout(connection_timeout);
    api->set_server_relay_enabled(true); // messages et clés entre clients relayés par l'hôte

    const Callable handlers[] = {
        callable_mp(this, &NetworkSession::_on_peer_authenticating),
        callable_mp(this, &NetworkSession::_on_peer_authentication_failed),
        callable_mp(this, &NetworkSession::_on_peer_connected),
        callable_mp(this, &NetworkSession::_on_peer_disconnected),
        callable_mp(this, &NetworkSession::_on_peer_packet),
        callable_mp(this, &NetworkSession::_on_connected_to_server),
        callable_mp(this, &NetworkSession::_on_connection_failed),
        callable_mp(this, &NetworkSession::_on_server_disconnected),
    };
    for (int i = 0; i < (int)std::size(API_SIGNALS); i++)
    {
        api->connect(API_SIGNALS[i], handlers[i]);
    }
    return OK;
}

void NetworkSession::_detach_api()
{
    if (api.is_null())
    {
        return;
    }
    const Callable handlers[] = {
        callable_mp(this, &NetworkSession::_on_peer_authenticating),
        callable_mp(this, &NetworkSession::_on_peer_authentication_failed),
        callable_mp(this, &NetworkSession::_on_peer_connected),
        callable_mp(this, &NetworkSession::_on_peer_disconnected),
        callable_mp(this, &NetworkSession::_on_peer_packet),
        callable_mp(this, &NetworkSession::_on_connected_to_server),
        callable_mp(this, &NetworkSession::_on_connection_failed),
        callable_mp(this, &NetworkSession::_on_server_disconnected),
    };
    for (int i = 0; i < (int)std::size(API_SIGNALS); i++)
    {
        if (api->is_connected(API_SIGNALS[i], handlers[i]))
        {
            api->disconnect(API_SIGNALS[i], handlers[i]);
        }
    }
    api->set_auth_callback(Callable());

    // Fermeture différée : _end peut être appelé pendant que SceneMultiplayer parcourt ses pairs (poll).
    callable_mp_static(&NetworkSession::_release_peer).call_deferred(api, peer);
    api.unref();
    peer.unref();
}

void NetworkSession::_release_peer(const Ref<SceneMultiplayer> &p_api, const Ref<MultiplayerPeer> &p_peer)
{
    if (p_peer.is_valid())
    {
        p_peer->close();
    }
    // Ne touche pas au MultiplayerAPI si une nouvelle session y a déjà installé son propre pair.
    if (p_api.is_valid() && p_api->get_multiplayer_peer() == p_peer)
    {
        Ref<OfflineMultiplayerPeer> offline;
        offline.instantiate();
        p_api->set_multiplayer_peer(offline);
    }
}

void NetworkSession::_end(Reason p_reason, const String &p_message)
{
    const State previous = state;
    last_reason = p_reason;
    last_message = p_message;
    _detach_api();

    // player_left pour chaque joueur (local compris), symétrique de player_joined.
    const std::map<int, Dictionary> left = players;
    players.clear();
    links.clear();
    pending_clients.clear();
    pending_kicks.clear();
    session_e2e = false;
    session_transport_encrypted = false;
    session_password = false;
    session_dedicated = false;
    approval_deadline_msec = 0;
    active_port = 0;
    _set_state(STATE_OFFLINE);
    _update_discovery();

    for (const auto &entry : left)
    {
        emit_signal("player_left", entry.first, entry.second);
    }
    if (previous == STATE_CONNECTING || previous == STATE_AUTHENTICATING)
    {
        emit_signal("connection_failed", p_reason, p_message);
    }
    else if (previous == STATE_CONNECTED || previous == STATE_HOSTING)
    {
        emit_signal("session_ended", p_reason, p_message);
    }
}

void NetworkSession::_set_state(State p_state)
{
    if (state != p_state)
    {
        state = p_state;
        emit_signal("state_changed", state);
    }
}

Error NetworkSession::_ensure_identity_key()
{
    if (identity_key.is_valid() && !identity_key->is_public_only())
    {
        return OK;
    }
    Ref<Crypto> crypto;
    crypto.instantiate();
    identity_key = crypto->generate_rsa(2048);
    ERR_FAIL_COND_V_MSG(identity_key.is_null(), ERR_CANT_CREATE, U"NetworkSession : génération de la clé d'identité impossible.");
    return OK;
}

Error NetworkSession::_ensure_transport_certificate()
{
    if (transport_key.is_valid() && transport_certificate.is_valid())
    {
        return OK;
    }
    // Certificat auto-signé : chiffre le trafic ; un client ne l'authentifie que s'il l'a dans trusted_certificate.
    Ref<Crypto> crypto;
    crypto.instantiate();
    transport_key = crypto->generate_rsa(2048);
    ERR_FAIL_COND_V(transport_key.is_null(), ERR_CANT_CREATE);
    transport_certificate = crypto->generate_self_signed_certificate(transport_key, "CN=" + certificate_common_name + ",O=GameplayToolkit");
    ERR_FAIL_COND_V(transport_certificate.is_null(), ERR_CANT_CREATE);
    return OK;
}

Ref<TLSOptions> NetworkSession::_client_tls_options() const
{
    // Sans certificat de confiance, le trafic est chiffré mais l'hôte n'est pas authentifié.
    return trusted_certificate.is_valid() ? TLSOptions::client(trusted_certificate, certificate_common_name) : TLSOptions::client_unsafe();
}

void NetworkSession::_process(double p_delta)
{
    (void)p_delta;
    const uint64_t now = now_msec();

    if (state == STATE_CONNECTING || state == STATE_AUTHENTICATING)
    {
        const uint64_t deadline = approval_deadline_msec != 0 ? approval_deadline_msec : connect_start_msec + (uint64_t)(connection_timeout * 1000.0f);
        if (now > deadline)
        {
            _end(REASON_TIMEOUT, approval_deadline_msec != 0 ? String(U"Le serveur n'a pas validé la connexion à temps.") : String(U"L'hôte n'a pas répondu à temps."));
            return;
        }
    }

    for (auto it = pending_kicks.begin(); it != pending_kicks.end();)
    {
        if (now >= it->second && api.is_valid())
        {
            if (api->get_peers().has(it->first) || api->get_authenticating_peers().has(it->first))
            {
                api->disconnect_peer(it->first);
            }
            it = pending_kicks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void NetworkSession::_update_discovery()
{
    LanDiscovery *discovery = Object::cast_to<LanDiscovery>(get_discovery());
    if (discovery == nullptr)
    {
        return;
    }
    if (state != STATE_HOSTING)
    {
        discovery->clear_session_info();
        return;
    }
    // La session fait autorité : la découverte annonce le même jeu et la même version.
    discovery->set_game_id(get_game_id());
    discovery->set_game_version(get_game_version());

    Dictionary info;
    info["hosting"] = true;
    info["host_name"] = get_player_name();
    info["port"] = active_port;
    info["transport"] = transport_name(transport);
    info["players"] = (int)players.size();
    info["max_players"] = max_players;
    info["password_required"] = session_password;
    info["transport_encryption"] = session_transport_encrypted;
    info["end_to_end_encryption"] = session_e2e;
    info["dedicated"] = session_dedicated;
    discovery->set_session_info(info);
}

#pragma endregion

#pragma region Signaux de SceneMultiplayer

void NetworkSession::_on_peer_authenticating(int p_id)
{
    if (state == STATE_HOSTING)
    {
        // Défi : le client doit prouver qu'il connaît le mot de passe sans l'envoyer.
        PendingClient pending;
        pending.nonce = SecureChannel::random_bytes(32);
        pending_clients[p_id] = pending;

        Dictionary challenge;
        challenge["t"] = "challenge";
        challenge["protocol"] = PROTOCOL_VERSION;
        challenge["game_id"] = get_game_id();
        challenge["game_version"] = get_game_version();
        challenge["password"] = session_password;
        challenge["salt"] = password_salt;
        challenge["iterations"] = PASSWORD_ITERATIONS;
        challenge["nonce"] = pending.nonce;
        challenge["e2e"] = session_e2e;
        challenge["dedicated"] = session_dedicated;
        if (identity_key.is_valid())
        {
            // Le serveur prouve son identité : signature du défi avec sa clé privée.
            Ref<Crypto> crypto;
            crypto.instantiate();
            challenge["server_key"] = identity_key->save_to_string(true);
            challenge["server_sig"] = crypto->sign(HashingContext::HASH_SHA256, _signature_digest(pending.nonce, "server"), identity_key);
        }
        if (access_policy.is_valid())
        {
            challenge["identify"] = true; // l'appareil doit prouver son identité (bans, appareils reconnus)
            challenge["access_mode"] = access_policy->get_access_mode();
            challenge["account_seed"] = access_policy->get_account_seed();
        }
        _send_auth(p_id, challenge);
    }
    else if (state == STATE_CONNECTING && p_id == 1)
    {
        _set_state(STATE_AUTHENTICATING);
    }
}

void NetworkSession::_on_peer_authentication_failed(int p_id)
{
    // Un pair parti pendant l'authentification n'émet pas peer_disconnected : expulsion prévue annulée ici.
    pending_clients.erase(p_id);
    pending_kicks.erase(p_id);
    if (p_id == 1 && state == STATE_AUTHENTICATING && last_reason == REASON_NONE)
    {
        _end(REASON_TIMEOUT, U"Poignée de main avec l'hôte trop longue.");
    }
}

void NetworkSession::_on_peer_connected(int p_id)
{
    if (state == STATE_HOSTING)
    {
        const auto pending = pending_clients.find(p_id);
        if (pending == pending_clients.end())
        {
            return;
        }
        Dictionary wire_info = pending->second.info;
        pending_clients.erase(pending);

        // Liste envoyée maintenant (et non dans « accept ») : un client qui finissait sa poignée de main en
        // même temps n'aurait pas reçu les PACKET_PLAYER_JOINED (pas d'envoi à un pair non authentifié).
        Dictionary roster = _wire_roster();
        roster[p_id] = wire_info;
        _send_packet(PACKET_ROSTER, UtilityFunctions::var_to_bytes(roster), p_id);
        _add_player(p_id, wire_info);

        // Les autres clients apprennent l'arrivée.
        Dictionary joined;
        joined["id"] = p_id;
        joined["info"] = wire_info;
        _send_packet(PACKET_PLAYER_JOINED, UtilityFunctions::var_to_bytes(joined), -p_id);
        _update_discovery();
    }
    _try_establish_link(p_id);
}

void NetworkSession::_on_peer_disconnected(int p_id)
{
    pending_clients.erase(p_id);
    pending_kicks.erase(p_id);
    if (p_id == 1 && state != STATE_HOSTING)
    {
        return; // server_disconnected s'en charge
    }
    _remove_player(p_id);
    if (state == STATE_HOSTING)
    {
        _update_discovery();
    }
}

void NetworkSession::_on_connected_to_server()
{
    // Rien à faire : la session s'ouvre à la réception de la liste des joueurs (PACKET_ROSTER).
}

void NetworkSession::_client_handle_roster(const Dictionary &p_roster)
{
    _set_state(STATE_CONNECTED);
    emit_signal("session_joined", get_local_peer_id());

    const Array ids = p_roster.keys();
    for (int i = 0; i < ids.size(); i++)
    {
        const Variant info = p_roster[ids[i]];
        if (ids[i].get_type() == Variant::INT && info.get_type() == Variant::DICTIONARY)
        {
            _add_player(ids[i], info);
        }
    }
    if (session_dedicated)
    {
        _try_establish_link(1); // le serveur dédié n'est pas un joueur : lien établi à part
    }
}

Dictionary NetworkSession::_wire_roster() const
{
    // Infos de chaque joueur telles qu'envoyées sur le réseau : avec la clé publique de bout en bout.
    Dictionary roster;
    for (const auto &entry : players)
    {
        Dictionary wire = entry.second.duplicate(true);
        const auto link = links.find(entry.first);
        if (entry.first == 1 && session_e2e)
        {
            wire["public_key"] = identity_key->save_to_string(true);
        }
        else if (link != links.end() && link->second.public_key.is_valid())
        {
            wire["public_key"] = link->second.public_key->save_to_string(true);
        }
        roster[entry.first] = wire;
    }
    return roster;
}

void NetworkSession::_on_connection_failed()
{
    if (state == STATE_CONNECTING || state == STATE_AUTHENTICATING)
    {
        _end(REASON_CONNECTION_FAILED, U"Hôte injoignable, ou chiffrement du transport incompatible.");
    }
}

void NetworkSession::_on_server_disconnected()
{
    if (state == STATE_CONNECTING || state == STATE_AUTHENTICATING)
    {
        _end(REASON_CONNECTION_FAILED, U"L'hôte a fermé la connexion pendant la poignée de main.");
    }
    else if (state == STATE_CONNECTED)
    {
        _end(REASON_HOST_LOST, U"Connexion à l'hôte perdue.");
    }
}

#pragma endregion

#pragma region Poignée de main

void NetworkSession::_send_auth(int p_id, const Dictionary &p_message)
{
    if (api.is_valid())
    {
        api->send_auth(p_id, UtilityFunctions::var_to_bytes(p_message));
    }
}

void NetworkSession::_on_auth(int p_id, const PackedByteArray &p_data)
{
    const Variant decoded = UtilityFunctions::bytes_to_var(p_data); // sans objets : sûr
    if (decoded.get_type() != Variant::DICTIONARY)
    {
        if (state == STATE_HOSTING)
        {
            _host_reject(p_id, REASON_INVALID_HANDSHAKE, U"Poignée de main illisible.");
        }
        return;
    }
    const Dictionary message = decoded;
    const String type = message.get("t", "");

    if (state == STATE_HOSTING)
    {
        if (type == "hello")
        {
            _host_handle_hello(p_id, message);
        }
        return;
    }
    if (p_id != 1 || (state != STATE_AUTHENTICATING && state != STATE_CONNECTING))
    {
        return;
    }
    if (type == "challenge")
    {
        _client_handle_challenge(message);
    }
    else if (type == "accept")
    {
        _client_handle_accept(message);
    }
    else if (type == "wait")
    {
        // Le serveur fait valider cet appareil à la main : délai prolongé.
        const float timeout = CLAMP((float)message.get("timeout", 120.0f), 5.0f, 3600.0f);
        approval_deadline_msec = now_msec() + (uint64_t)(timeout * 1000.0f);
        api->set_auth_timeout(timeout + 5.0f);
        emit_signal("waiting_for_approval", timeout);
    }
    else if (type == "reject")
    {
        int reason = message.get("reason", REASON_INVALID_HANDSHAKE);
        if (reason <= REASON_CLOSED || reason > REASON_TOO_MANY_ATTEMPTS)
        {
            reason = REASON_INVALID_HANDSHAKE;
        }
        _end((Reason)reason, String(message.get("message", "")).substr(0, 256));
    }
}

void NetworkSession::_client_handle_challenge(const Dictionary &p_challenge)
{
    if ((int)p_challenge.get("protocol", -1) != PROTOCOL_VERSION)
    {
        _end(REASON_VERSION_MISMATCH, U"L'hôte utilise une autre version du module multijoueur.");
        return;
    }
    const bool host_e2e = p_challenge.get("e2e", false);
    if (end_to_end_encryption && !host_e2e)
    {
        _end(REASON_SECURITY_MISMATCH, U"L'hôte ne propose pas le chiffrement de bout en bout, alors que ce client l'exige.");
        return;
    }
    session_e2e = host_e2e;
    session_password = p_challenge.get("password", false);
    session_dedicated = p_challenge.get("dedicated", false);
    const PackedByteArray nonce = p_challenge.get("nonce", PackedByteArray());
    Ref<Crypto> crypto;
    crypto.instantiate();

    // Identité du serveur : signature du défi, et empreinte comparée à celle attendue (code de connexion).
    const String server_pem = p_challenge.get("server_key", "");
    Ref<CryptoKey> server_key;
    if (!server_pem.is_empty())
    {
        server_key.instantiate();
        if (server_key->load_from_string(server_pem, true) != OK ||
            !crypto->verify(HashingContext::HASH_SHA256, _signature_digest(nonce, "server"), p_challenge.get("server_sig", PackedByteArray()), server_key))
        {
            _end(REASON_SECURITY_MISMATCH, U"Signature du serveur invalide.");
            return;
        }
        server_fingerprint = SecureChannel::fingerprint(server_pem.to_utf8_buffer());
    }
    if (!expected_server_fingerprint.is_empty())
    {
        const String expected = expected_server_fingerprint.replace(" ", "").to_upper();
        if (server_fingerprint.replace(" ", "") != expected)
        {
            _end(REASON_SECURITY_MISMATCH, U"Ce serveur n'est pas celui attendu (empreinte différente) : connexion annulée, faux serveur possible.");
            return;
        }
        server_verified = true;
    }

    const bool identify = p_challenge.get("identify", false);
    if ((session_e2e || identify) && _ensure_identity_key() != OK)
    {
        _end(REASON_SECURITY_MISMATCH, U"Clé d'identité impossible à générer.");
        return;
    }
    if (session_e2e && server_key.is_valid())
    {
        // Serveur dédié (pas dans la liste des joueurs) : sa clé publique vient du défi.
        PeerLink &link = links[1];
        link.public_key = server_key;
        link.fingerprint = server_fingerprint;
    }

    Dictionary hello;
    hello["t"] = "hello";
    hello["protocol"] = PROTOCOL_VERSION;
    hello["game_id"] = get_game_id();
    hello["game_version"] = get_game_version();
    hello["info"] = _local_player_info();
    if (session_password && !password.is_empty()) // sans mot de passe : pas de preuve, l'hôte refusera
    {
        const int iterations = CLAMP((int)p_challenge.get("iterations", PASSWORD_ITERATIONS), 1, 1000000);
        const PackedByteArray key = SecureChannel::derive_key(password, p_challenge.get("salt", PackedByteArray()), iterations, 32);
        hello["proof"] = crypto->hmac_digest(HashingContext::HASH_SHA256, key, nonce);
    }
    if (identify)
    {
        // Preuve de possession de la clé d'identité : l'appareil ne peut pas se faire passer pour un autre.
        hello["device_key"] = identity_key->save_to_string(true);
        hello["device_sig"] = crypto->sign(HashingContext::HASH_SHA256, _signature_digest(nonce, "device"), identity_key);
    }
    if (!account_name.is_empty())
    {
        hello["username"] = account_name;
        const PackedByteArray account_key = ServerAccessPolicy::account_key(p_challenge.get("account_seed", PackedByteArray()), account_name, account_password);
        hello["account_proof"] = crypto->hmac_digest(HashingContext::HASH_SHA256, account_key, nonce);
    }
    if (!invite_code.is_empty())
    {
        hello["invite_proof"] = ServerAccessPolicy::invite_proof(invite_code, nonce);
    }
    _send_auth(1, hello);
}

void NetworkSession::_host_handle_hello(int p_id, const Dictionary &p_hello)
{
    const auto pending = pending_clients.find(p_id);
    if (pending == pending_clients.end() || pending_kicks.count(p_id))
    {
        return;
    }
    if ((int)p_hello.get("protocol", -1) != PROTOCOL_VERSION)
    {
        _host_reject(p_id, REASON_VERSION_MISMATCH, U"Version du module multijoueur différente de celle de l'hôte.");
        return;
    }
    if (String(p_hello.get("game_id", "")) != get_game_id())
    {
        _host_reject(p_id, REASON_GAME_MISMATCH, String(U"L'hôte joue à « {0} ».").format(Array::make(get_game_id())));
        return;
    }
    if (!allow_version_mismatch && String(p_hello.get("game_version", "")) != get_game_version())
    {
        _host_reject(p_id, REASON_VERSION_MISMATCH, String(U"L'hôte est en version « {0} », ce client en « {1} » : mettre à jour le jeu.")
                                                        .format(Array::make(get_game_version(), p_hello.get("game_version", ""))));
        return;
    }
    PendingClient &client = pending->second;
    client.address = get_peer_address(p_id);
    const uint64_t now = now_msec();
    const auto failure = failures.find(client.address);
    if (failure != failures.end() && failure->second.blocked_until_msec > now)
    {
        _host_reject(p_id, REASON_TOO_MANY_ATTEMPTS, String(U"Trop de tentatives échouées : réessayez dans {0} s.")
                                                         .format(Array::make((int64_t)((failure->second.blocked_until_msec - now) / 1000 + 1))));
        return;
    }
    if (access_policy.is_valid() && access_policy->is_address_banned(client.address))
    {
        _host_reject(p_id, REASON_BANNED, U"Adresse bannie de ce serveur.");
        return;
    }

    Ref<Crypto> crypto;
    crypto.instantiate();
    if (access_policy.is_valid())
    {
        // Identité de l'appareil : clé publique + signature du défi (une empreinte ne se déclare pas, elle se prouve).
        const String device_pem = p_hello.get("device_key", "");
        Ref<CryptoKey> device_key;
        device_key.instantiate();
        if (device_pem.is_empty() || device_key->load_from_string(device_pem, true) != OK ||
            !crypto->verify(HashingContext::HASH_SHA256, _signature_digest(client.nonce, "device"), p_hello.get("device_sig", PackedByteArray()), device_key))
        {
            _host_reject(p_id, REASON_SECURITY_MISMATCH, U"Identité de l'appareil absente ou invalide.");
            return;
        }
        client.fingerprint = SecureChannel::fingerprint(device_pem.to_utf8_buffer());
        if (access_policy->is_device_banned(client.fingerprint))
        {
            _host_reject(p_id, REASON_BANNED, U"Cet appareil est banni de ce serveur.");
            return;
        }
    }

    if (session_password)
    {
        const PackedByteArray expected = crypto->hmac_digest(HashingContext::HASH_SHA256, password_key, client.nonce);
        const Variant proof = p_hello.get("proof", Variant());
        if (proof.get_type() != Variant::PACKED_BYTE_ARRAY || !crypto->constant_time_compare(expected, proof))
        {
            _register_failure(client.address);
            _host_reject(p_id, REASON_WRONG_PASSWORD, U"Mot de passe incorrect.");
            return;
        }
    }

    // Règles d'accès : rôle donné par le compte, l'invitation ou l'appareil déjà reconnu.
    int role = access_policy.is_valid() ? (int)access_policy->get_default_role() : (int)ServerAccessPolicy::ROLE_PLAYER;
    String username;
    String invite;
    const bool known_device = access_policy.is_valid() && access_policy->is_device_known(client.fingerprint);
    if (known_device)
    {
        role = access_policy->get_device_role(client.fingerprint);
    }
    if (access_policy.is_valid() && access_policy->get_access_mode() == ServerAccessPolicy::ACCESS_ACCOUNTS)
    {
        username = String(p_hello.get("username", "")).substr(0, MAX_TEXT_LENGTH);
        if (!access_policy->verify_account(username, client.nonce, p_hello.get("account_proof", PackedByteArray())))
        {
            _register_failure(client.address);
            _host_reject(p_id, REASON_ACCESS_DENIED, U"Compte inconnu ou mot de passe incorrect.");
            return;
        }
        role = access_policy->get_account_role(username);
    }
    else if (access_policy.is_valid() && access_policy->get_access_mode() == ServerAccessPolicy::ACCESS_INVITATIONS && !known_device)
    {
        invite = access_policy->find_invite(client.nonce, p_hello.get("invite_proof", PackedByteArray()));
        if (invite.is_empty())
        {
            _register_failure(client.address);
            _host_reject(p_id, REASON_ACCESS_DENIED, U"Invitation absente, expirée ou déjà utilisée.");
            return;
        }
        role = ((Dictionary)access_policy->get_invites_data().get(invite, Dictionary())).get("role", role);
    }
    failures.erase(client.address);

    const Variant raw_info = p_hello.get("info", Dictionary());
    Dictionary info = _sanitize_info(raw_info.get_type() == Variant::DICTIONARY ? (Dictionary)raw_info : Dictionary());
    info["peer_id"] = p_id;
    info["game_version"] = String(p_hello.get("game_version", "")).substr(0, MAX_TEXT_LENGTH);
    info["role"] = role; // attribué par le serveur : un client ne peut pas le choisir
    if (!client.fingerprint.is_empty())
    {
        info["fingerprint"] = client.fingerprint;
    }
    if (!username.is_empty())
    {
        info["username"] = username;
    }
    if (session_e2e)
    {
        Ref<CryptoKey> key;
        key.instantiate();
        const String pem = raw_info.get_type() == Variant::DICTIONARY ? String(((Dictionary)raw_info).get("public_key", "")) : String();
        if (pem.is_empty() || key->load_from_string(pem, true) != OK)
        {
            _host_reject(p_id, REASON_SECURITY_MISMATCH, U"Cette partie exige le chiffrement de bout en bout : clé publique absente ou invalide.");
            return;
        }
        info["public_key"] = pem;
    }
    // Places prises : joueurs présents + clients déjà acceptés qui finissent leur poignée de main.
    size_t taken = players.size();
    for (const auto &entry : pending_clients)
    {
        taken += entry.second.info.is_empty() ? 0 : 1;
    }
    if (taken >= (size_t)max_players)
    {
        _host_reject(p_id, REASON_SESSION_FULL, String(U"Partie complète ({0} joueurs).").format(Array::make(max_players)));
        return;
    }
    client.info = info;
    client.role = role;
    client.invite = invite;

    if (access_policy.is_valid() && access_policy->is_approval_required() && !known_device)
    {
        // Appareil inconnu : l'administrateur du serveur décide (approve_peer / deny_peer).
        client.awaiting_approval = true;
        Dictionary wait;
        wait["t"] = "wait";
        wait["timeout"] = access_policy->get_approval_timeout();
        _send_auth(p_id, wait);
        emit_signal("access_requested", p_id, _pending_request(p_id));
        return;
    }
    _host_accept(p_id, false);
}

void NetworkSession::_host_accept(int p_id, bool p_remember)
{
    const auto pending = pending_clients.find(p_id);
    if (pending == pending_clients.end())
    {
        return;
    }
    PendingClient &client = pending->second;
    if (!client.invite.is_empty())
    {
        // Invitation consommée seulement maintenant : un appareil refusé ne gaspille pas d'usage.
        if (!access_policy->has_invite(client.invite))
        {
            _host_reject(p_id, REASON_ACCESS_DENIED, U"Invitation expirée ou déjà utilisée.");
            return;
        }
        access_policy->consume_invite(client.invite);
        p_remember = true; // un appareil invité revient ensuite sans nouveau code
    }
    if (p_remember && access_policy.is_valid() && !client.fingerprint.is_empty())
    {
        access_policy->remember_device(client.fingerprint, client.info.get("name", ""), (ServerAccessPolicy::Role)client.role);
    }
    client.awaiting_approval = false;

    Dictionary accept;
    accept["t"] = "accept";
    accept["peer_id"] = p_id;
    _send_auth(p_id, accept);
    api->complete_auth(p_id);
}

Dictionary NetworkSession::_pending_request(int p_id) const
{
    const auto pending = pending_clients.find(p_id);
    if (pending == pending_clients.end())
    {
        return Dictionary();
    }
    Dictionary request = pending->second.info.duplicate(true);
    request.erase("public_key");
    request["peer_id"] = p_id;
    request["address"] = pending->second.address;
    request["fingerprint"] = pending->second.fingerprint;
    request["role"] = pending->second.role;
    return request;
}

void NetworkSession::_register_failure(const String &p_address)
{
    const int max_attempts = access_policy.is_valid() ? access_policy->get_max_failed_attempts() : 5;
    const float duration = access_policy.is_valid() ? access_policy->get_block_duration() : 60.0f;
    if (max_attempts <= 0 || p_address.is_empty())
    {
        return;
    }
    Failures &entry = failures[p_address];
    entry.count++;
    if (entry.count >= max_attempts)
    {
        entry.count = 0;
        entry.blocked_until_msec = now_msec() + (uint64_t)(duration * 1000.0f);
    }
}

PackedByteArray NetworkSession::_signature_digest(const PackedByteArray &p_nonce, const String &p_label) const
{
    // Le libellé distingue la signature du serveur de celle d'un appareil (pas de réutilisation croisée).
    PackedByteArray data = p_label.to_utf8_buffer();
    data.append_array(p_nonce);
    Ref<HashingContext> hashing;
    hashing.instantiate();
    hashing->start(HashingContext::HASH_SHA256);
    hashing->update(data);
    return hashing->finish();
}

void NetworkSession::_host_reject(int p_id, Reason p_reason, const String &p_message)
{
    Dictionary reject;
    reject["t"] = "reject";
    reject["reason"] = p_reason;
    reject["message"] = p_message;
    _send_auth(p_id, reject);
    pending_clients.erase(p_id);
    pending_kicks[p_id] = now_msec() + KICK_DELAY_MSEC;
    emit_signal("peer_rejected", p_id, p_reason, p_message);
}

void NetworkSession::_client_handle_accept(const Dictionary &p_accept)
{
    if ((int)p_accept.get("peer_id", 0) != get_local_peer_id())
    {
        _end(REASON_INVALID_HANDSHAKE, U"Réponse de l'hôte illisible.");
        return;
    }
    api->complete_auth(1);
}

Dictionary NetworkSession::_local_player_info() const
{
    Dictionary info = local_system_info();
    info["name"] = get_player_name();
    info["data"] = player_data.duplicate(true);
    info["game_version"] = get_game_version();
    info["fingerprint"] = get_local_fingerprint();
    if (session_e2e || end_to_end_encryption)
    {
        info["public_key"] = identity_key.is_valid() ? identity_key->save_to_string(true) : String();
    }
    return info;
}

Dictionary NetworkSession::_sanitize_info(const Dictionary &p_info)
{
    // Infos venues du réseau : seulement les clés connues, textes tronqués.
    Dictionary info;
    for (const char *key : {"name", "os", "os_version", "distribution", "model", "engine_version"})
    {
        info[key] = String(p_info.get(key, "")).substr(0, MAX_TEXT_LENGTH);
    }
    const Variant data = p_info.get("data", Dictionary());
    info["data"] = data.get_type() == Variant::DICTIONARY ? data : Variant(Dictionary());
    return info;
}

#pragma endregion

#pragma region Joueurs

void NetworkSession::_add_player(int p_id, const Dictionary &p_wire_info)
{
    Dictionary info = p_wire_info.duplicate(true);
    const String pem = info.get("public_key", "");
    info.erase("public_key");
    info["peer_id"] = p_id;

    if (p_id == get_local_peer_id())
    {
        info["fingerprint"] = get_local_fingerprint();
    }
    else if (!pem.is_empty())
    {
        Ref<CryptoKey> key;
        key.instantiate();
        if (key->load_from_string(pem, true) == OK)
        {
            PeerLink &link = links[p_id];
            link.public_key = key;
            link.fingerprint = SecureChannel::fingerprint(key->save_to_string(true).to_utf8_buffer());
            info["fingerprint"] = link.fingerprint;
        }
    }
    const bool is_new = players.count(p_id) == 0;
    players[p_id] = info;
    if (is_new)
    {
        emit_signal("player_joined", p_id, info);
    }

    // Offre de clé arrivée avant la clé publique de ce joueur : traitée maintenant.
    const auto link = links.find(p_id);
    if (link != links.end() && !link->second.pending_offer.is_empty())
    {
        const PackedByteArray offer = link->second.pending_offer;
        link->second.pending_offer = PackedByteArray();
        _handle_key_offer(p_id, offer);
    }
    _try_establish_link(p_id);
}

void NetworkSession::_remove_player(int p_id)
{
    links.erase(p_id);
    const auto found = players.find(p_id);
    if (found == players.end())
    {
        return;
    }
    const Dictionary info = found->second;
    players.erase(found);
    emit_signal("player_left", p_id, info);
}

Dictionary NetworkSession::get_players() const
{
    Dictionary result;
    for (const auto &entry : players)
    {
        result[entry.first] = entry.second.duplicate(true);
    }
    return result;
}

PackedInt32Array NetworkSession::get_player_ids() const
{
    PackedInt32Array ids;
    for (const auto &entry : players)
    {
        ids.push_back(entry.first);
    }
    return ids;
}

Dictionary NetworkSession::get_player_info(int p_peer_id) const
{
    const auto found = players.find(p_peer_id);
    return found != players.end() ? found->second.duplicate(true) : Dictionary();
}

Error NetworkSession::update_player_data(const Dictionary &p_data)
{
    player_data = p_data.duplicate(true);
    if (!is_online())
    {
        return OK;
    }
    const int local_id = get_local_peer_id();
    Dictionary &info = players[local_id];
    info["data"] = player_data.duplicate(true);
    emit_signal("player_updated", local_id, info);

    if (is_host())
    {
        Dictionary update;
        update["id"] = local_id;
        update["info"] = info;
        return _send_packet(PACKET_PLAYER_UPDATE, UtilityFunctions::var_to_bytes(update), 0);
    }
    return _send_packet(PACKET_PLAYER_UPDATE, UtilityFunctions::var_to_bytes(player_data), 1);
}

int NetworkSession::get_local_peer_id() const
{
    if (state == STATE_OFFLINE || api.is_null())
    {
        return 0;
    }
    return api->get_unique_id();
}

Error NetworkSession::kick(int p_peer_id, const String &p_message)
{
    ERR_FAIL_COND_V_MSG(!is_host(), ERR_UNAUTHORIZED, U"NetworkSession : seul l'hôte peut expulser un joueur.");
    ERR_FAIL_COND_V_MSG(p_peer_id == 1 || !has_player(p_peer_id), ERR_INVALID_PARAMETER, U"NetworkSession : joueur inconnu.");
    Dictionary kick_message;
    kick_message["reason"] = REASON_KICKED;
    kick_message["message"] = p_message.is_empty() ? String(U"Expulsé par l'hôte.") : p_message;
    _send_packet(PACKET_KICK, UtilityFunctions::var_to_bytes(kick_message), p_peer_id);
    pending_kicks[p_peer_id] = now_msec() + KICK_DELAY_MSEC;
    return OK;
}

#pragma endregion

#pragma region Accès (serveur)

Array NetworkSession::get_pending_requests() const
{
    Array requests;
    for (const auto &entry : pending_clients)
    {
        if (entry.second.awaiting_approval)
        {
            requests.push_back(_pending_request(entry.first));
        }
    }
    return requests;
}

Error NetworkSession::approve_peer(int p_peer_id, bool p_remember, int p_role)
{
    ERR_FAIL_COND_V_MSG(!is_host(), ERR_UNAUTHORIZED, U"NetworkSession : seul le serveur valide les connexions.");
    const auto pending = pending_clients.find(p_peer_id);
    ERR_FAIL_COND_V_MSG(pending == pending_clients.end() || !pending->second.awaiting_approval, ERR_INVALID_PARAMETER, U"NetworkSession : aucune demande en attente pour ce pair.");
    if (p_role >= ServerAccessPolicy::ROLE_SPECTATOR && p_role <= ServerAccessPolicy::ROLE_ADMIN)
    {
        pending->second.role = p_role;
        pending->second.info["role"] = p_role;
    }
    _host_accept(p_peer_id, p_remember);
    return OK;
}

Error NetworkSession::deny_peer(int p_peer_id, const String &p_message)
{
    ERR_FAIL_COND_V_MSG(!is_host(), ERR_UNAUTHORIZED, U"NetworkSession : seul le serveur valide les connexions.");
    const auto pending = pending_clients.find(p_peer_id);
    ERR_FAIL_COND_V_MSG(pending == pending_clients.end() || !pending->second.awaiting_approval, ERR_INVALID_PARAMETER, U"NetworkSession : aucune demande en attente pour ce pair.");
    _host_reject(p_peer_id, REASON_ACCESS_DENIED, p_message.is_empty() ? String(U"Connexion refusée par le serveur.") : p_message);
    return OK;
}

Error NetworkSession::ban(int p_peer_id, const String &p_message, bool p_ban_address)
{
    ERR_FAIL_COND_V_MSG(!is_host(), ERR_UNAUTHORIZED, U"NetworkSession : seul le serveur peut bannir.");
    const auto pending = pending_clients.find(p_peer_id);
    ERR_FAIL_COND_V_MSG(p_peer_id == 1 || (!has_player(p_peer_id) && pending == pending_clients.end()), ERR_INVALID_PARAMETER, U"NetworkSession : pair inconnu.");
    if (access_policy.is_null())
    {
        access_policy.instantiate(); // règles ouvertes, juste pour retenir le bannissement
    }
    const String fingerprint = has_player(p_peer_id) ? String(players[p_peer_id].get("fingerprint", "")) : pending->second.fingerprint;
    access_policy->ban_device(fingerprint);
    if (p_ban_address)
    {
        access_policy->ban_address(get_peer_address(p_peer_id));
    }
    const String message = p_message.is_empty() ? String(U"Banni de ce serveur.") : p_message;
    if (!has_player(p_peer_id))
    {
        _host_reject(p_peer_id, REASON_BANNED, message);
        return OK;
    }
    Dictionary kick_message;
    kick_message["reason"] = REASON_BANNED;
    kick_message["message"] = message;
    _send_packet(PACKET_KICK, UtilityFunctions::var_to_bytes(kick_message), p_peer_id);
    pending_kicks[p_peer_id] = now_msec() + KICK_DELAY_MSEC;
    return OK;
}

String NetworkSession::get_peer_address(int p_peer_id) const
{
    if (peer.is_null() || !is_host())
    {
        return "";
    }
    if (ENetMultiplayerPeer *enet = Object::cast_to<ENetMultiplayerPeer>(peer.ptr()))
    {
        const Ref<ENetPacketPeer> packet_peer = enet->get_peer(p_peer_id);
        return packet_peer.is_valid() ? packet_peer->get_remote_address() : String();
    }
    if (WebSocketMultiplayerPeer *websocket = Object::cast_to<WebSocketMultiplayerPeer>(peer.ptr()))
    {
        const Ref<WebSocketPeer> socket = websocket->get_peer(p_peer_id);
        return socket.is_valid() ? socket->get_connected_host() : String();
    }
    return "";
}

bool NetworkSession::_is_known_peer(int p_id) const
{
    // Un serveur dédié n'est pas dans la liste des joueurs, mais reste un destinataire valide.
    return has_player(p_id) || (p_id == 1 && !is_host() && is_online());
}

#pragma endregion

#pragma region Code de connexion

String NetworkSession::create_connection_code(const String &p_address, const String &p_invite)
{
    ERR_FAIL_COND_V_MSG(p_address.is_empty(), "", U"NetworkSession : adresse vide.");
    // L'empreinte du code permet au client de vérifier qu'il parle bien à CE serveur.
    ERR_FAIL_COND_V(_ensure_identity_key() != OK, "");
    const String host = p_address.contains(":") && !p_address.begins_with("[") ? "[" + p_address + "]" : p_address;
    String code = "gtk://" + host + ":" + String::num_int64(active_port > 0 ? active_port : port);
    code += String("?t=") + transport_name(transport);
    code += String("&tls=") + (transport_encryption ? "1" : "0");
    code += String("&fp=") + get_local_fingerprint().replace(" ", "");
    if (!p_invite.is_empty())
    {
        code += String("&inv=") + ServerAccessPolicy::normalize_invite(p_invite);
    }
    return code;
}

Dictionary NetworkSession::parse_connection_code(const String &p_code)
{
    String code = p_code.strip_edges();
    if (!code.begins_with("gtk://"))
    {
        return Dictionary();
    }
    code = code.substr(6);
    const String host_port = code.get_slice("?", 0);
    const String query = code.contains("?") ? code.get_slice("?", 1) : String();

    String address;
    String port_text;
    if (host_port.begins_with("["))
    {
        // IPv6 : [adresse]:port
        address = host_port.substr(1, host_port.find("]") - 1);
        port_text = host_port.substr(host_port.find("]") + 2);
    }
    else
    {
        const int64_t colon = host_port.rfind(":");
        address = colon >= 0 ? host_port.substr(0, colon) : host_port;
        port_text = colon >= 0 ? host_port.substr(colon + 1) : String();
    }
    const int code_port = port_text.is_valid_int() ? port_text.to_int() : 0;
    if (address.is_empty() || code_port < 1 || code_port > 65535)
    {
        return Dictionary();
    }

    Dictionary result;
    result["address"] = address;
    result["port"] = code_port;
    result["transport"] = TRANSPORT_ENET;
    result["transport_encryption"] = false;
    result["fingerprint"] = "";
    result["invite"] = "";
    const PackedStringArray parameters = query.split("&", false);
    for (const String &parameter : parameters)
    {
        const String key = parameter.get_slice("=", 0);
        const String value = parameter.get_slice("=", 1).uri_decode();
        if (key == "t")
        {
            result["transport"] = value == "websocket" ? TRANSPORT_WEBSOCKET : TRANSPORT_ENET;
        }
        else if (key == "tls")
        {
            result["transport_encryption"] = value == "1";
        }
        else if (key == "fp")
        {
            // Remis en groupes de 4, comme get_local_fingerprint.
            PackedStringArray groups;
            const String hex = value.to_upper();
            for (int64_t i = 0; i < hex.length(); i += 4)
            {
                groups.push_back(hex.substr(i, 4));
            }
            result["fingerprint"] = String(" ").join(groups);
        }
        else if (key == "inv")
        {
            result["invite"] = ServerAccessPolicy::normalize_invite(value);
        }
    }
    return result;
}

Error NetworkSession::join_with_code(const String &p_code)
{
    const Dictionary parsed = parse_connection_code(p_code);
    if (parsed.is_empty())
    {
        last_reason = REASON_CONNECTION_FAILED;
        last_message = U"Code de connexion invalide.";
        emit_signal("connection_failed", last_reason, last_message);
        return ERR_INVALID_PARAMETER;
    }
    const bool remote_encrypted = parsed["transport_encryption"];
    if (transport_encryption && !remote_encrypted)
    {
        last_reason = REASON_SECURITY_MISMATCH;
        last_message = U"Le serveur ne chiffre pas le transport, alors que ce client l'exige.";
        emit_signal("connection_failed", last_reason, last_message);
        return ERR_UNAUTHORIZED;
    }
    expected_server_fingerprint = parsed["fingerprint"];
    if (!String(parsed["invite"]).is_empty())
    {
        invite_code = parsed["invite"];
    }
    return _start_join(parsed["address"], parsed["port"], (Transport)(int)parsed["transport"], remote_encrypted);
}

#pragma endregion

#pragma region Paquets et messages

Error NetworkSession::_send_packet(PacketType p_type, const PackedByteArray &p_payload, int p_to, bool p_reliable, int p_channel)
{
    ERR_FAIL_COND_V(api.is_null(), ERR_UNCONFIGURED);
    PackedByteArray packet;
    packet.push_back(PACKET_MAGIC);
    packet.push_back(p_type);
    packet.append_array(p_payload);
    return api->send_bytes(packet, p_to, p_reliable ? MultiplayerPeer::TRANSFER_MODE_RELIABLE : MultiplayerPeer::TRANSFER_MODE_UNRELIABLE, p_channel);
}

Error NetworkSession::send_message(const Variant &p_data, int p_peer_id, bool p_reliable, int p_channel)
{
    ERR_FAIL_COND_V_MSG(!is_online(), ERR_UNCONFIGURED, U"NetworkSession : aucune session ouverte.");
    const int local_id = get_local_peer_id();
    ERR_FAIL_COND_V_MSG(p_peer_id == local_id, ERR_INVALID_PARAMETER, U"NetworkSession : impossible de s'envoyer un message à soi-même.");
    ERR_FAIL_COND_V_MSG(p_peer_id > 0 && !_is_known_peer(p_peer_id), ERR_INVALID_PARAMETER, U"NetworkSession : destinataire inconnu.");
    const PackedByteArray payload = UtilityFunctions::var_to_bytes(p_data);

    if (!session_e2e)
    {
        return _send_packet(PACKET_MESSAGE, payload, p_peer_id, p_reliable, p_channel);
    }

    // Bout en bout : un chiffrement par destinataire (chacun a sa propre clé).
    if (p_peer_id > 0)
    {
        return _send_secure(p_peer_id, payload, p_reliable, p_channel);
    }
    Error result = OK;
    for (const auto &entry : players)
    {
        const int target = entry.first;
        const bool wanted = p_peer_id > 0 ? target == p_peer_id : target != local_id && target != -p_peer_id;
        if (wanted)
        {
            const Error err = _send_secure(target, payload, p_reliable, p_channel);
            result = err != OK ? err : result;
        }
    }
    return result;
}

Error NetworkSession::_send_secure(int p_to, const PackedByteArray &p_payload, bool p_reliable, int p_channel)
{
    PeerLink &link = links[p_to];
    if (link.outgoing.is_valid())
    {
        return _send_packet(PACKET_SECURE_MESSAGE, link.outgoing->encrypt(p_payload), p_to, p_reliable, p_channel);
    }
    // Clé pas encore échangée : en attente si fiable (jamais envoyé en clair), abandonné sinon.
    if (p_reliable && link.pending_out.size() < MAX_PENDING_MESSAGES)
    {
        link.pending_out.push_back(PendingMessage{p_payload, p_channel});
    }
    _try_establish_link(p_to);
    return OK;
}

void NetworkSession::_on_peer_packet(int p_id, const PackedByteArray &p_packet)
{
    if (p_packet.size() < 2 || p_packet[0] != PACKET_MAGIC)
    {
        return; // paquet du jeu (send_bytes), pas du module
    }
    const PacketType type = (PacketType)p_packet[1];
    const PackedByteArray payload = p_packet.slice(2);

    switch (type)
    {
    case PACKET_ROSTER:
    {
        const Variant decoded = UtilityFunctions::bytes_to_var(payload);
        if (p_id == 1 && (state == STATE_AUTHENTICATING || state == STATE_CONNECTING) && decoded.get_type() == Variant::DICTIONARY)
        {
            _client_handle_roster(decoded);
        }
        break;
    }
    case PACKET_PLAYER_JOINED:
    {
        const Variant decoded = UtilityFunctions::bytes_to_var(payload);
        if (p_id != 1 || is_host() || decoded.get_type() != Variant::DICTIONARY)
        {
            return;
        }
        const Dictionary joined = decoded;
        const Variant info = joined.get("info", Variant());
        if (joined.get("id", Variant()).get_type() == Variant::INT && info.get_type() == Variant::DICTIONARY)
        {
            _add_player(joined["id"], info);
        }
        break;
    }
    case PACKET_PLAYER_UPDATE:
    {
        const Variant decoded = UtilityFunctions::bytes_to_var(payload);
        if (decoded.get_type() != Variant::DICTIONARY)
        {
            return;
        }
        if (is_host())
        {
            // Un client change ses données : l'hôte les valide et les rediffuse aux autres.
            if (!has_player(p_id))
            {
                return;
            }
            Dictionary &info = players[p_id];
            info["data"] = decoded;
            emit_signal("player_updated", p_id, info);
            Dictionary update;
            update["id"] = p_id;
            update["info"] = info;
            _send_packet(PACKET_PLAYER_UPDATE, UtilityFunctions::var_to_bytes(update), -p_id);
        }
        else if (p_id == 1)
        {
            const Dictionary update = decoded;
            const int id = update.get("id", 0);
            const Variant info = update.get("info", Variant());
            if (has_player(id) && info.get_type() == Variant::DICTIONARY)
            {
                Dictionary &stored = players[id];
                stored["data"] = ((Dictionary)info).get("data", Dictionary());
                emit_signal("player_updated", id, stored);
            }
        }
        break;
    }
    case PACKET_MESSAGE:
        if (session_e2e)
        {
            // Message en clair dans une partie chiffrée : peut-être injecté par un intermédiaire.
            emit_signal("security_warning", p_id, String(U"Message non chiffré rejeté (la partie exige le chiffrement de bout en bout)."));
            return;
        }
        _deliver_message(p_id, payload, false);
        break;
    case PACKET_SECURE_MESSAGE:
        _handle_secure_message(p_id, payload);
        break;
    case PACKET_KEY_OFFER:
        _handle_key_offer(p_id, payload);
        break;
    case PACKET_KICK:
        if (p_id == 1 && state == STATE_CONNECTED)
        {
            const Variant decoded = UtilityFunctions::bytes_to_var(payload);
            const Dictionary kick_message = decoded.get_type() == Variant::DICTIONARY ? (Dictionary)decoded : Dictionary();
            const int reason = kick_message.get("reason", REASON_KICKED);
            _end(reason == REASON_BANNED ? REASON_BANNED : REASON_KICKED, String(kick_message.get("message", "")).substr(0, 256));
        }
        break;
    default:
        break;
    }
}

void NetworkSession::_deliver_message(int p_from, const PackedByteArray &p_payload, bool p_encrypted)
{
    emit_signal("message_received", p_from, UtilityFunctions::bytes_to_var(p_payload), p_encrypted);
}

#pragma endregion

#pragma region Chiffrement de bout en bout

bool NetworkSession::_is_reachable(int p_id) const
{
    if (api.is_null() || !is_online())
    {
        return false;
    }
    if (p_id == 1 && !is_host())
    {
        return true;
    }
    const PackedInt32Array peers = api->get_peers();
    return peers.has(p_id);
}

PackedByteArray NetworkSession::_key_offer_digest(const PackedByteArray &p_encrypted_key, int p_from, int p_to) const
{
    // La signature lie la clé à l'émetteur et au destinataire : impossible de la rejouer vers un autre joueur.
    PackedByteArray data = p_encrypted_key.duplicate();
    append_u32(data, (uint32_t)p_from);
    append_u32(data, (uint32_t)p_to);
    Ref<HashingContext> hashing;
    hashing.instantiate();
    hashing->start(HashingContext::HASH_SHA256);
    hashing->update(data);
    return hashing->finish();
}

void NetworkSession::_try_establish_link(int p_id)
{
    const int local_id = get_local_peer_id();
    if (!session_e2e || p_id == local_id || !_is_known_peer(p_id) || !_is_reachable(p_id) || identity_key.is_null())
    {
        return;
    }
    const auto found = links.find(p_id);
    if (found == links.end() || found->second.public_key.is_null() || found->second.outgoing.is_valid())
    {
        return;
    }
    PeerLink &link = found->second;

    // Notre clé d'envoi vers ce joueur : chiffrée avec SA clé publique, signée avec NOTRE clé privée.
    Ref<SecureChannel> channel;
    channel.instantiate();
    const PackedByteArray key = channel->generate_key();
    Ref<Crypto> crypto;
    crypto.instantiate();
    const PackedByteArray encrypted_key = crypto->encrypt(link.public_key, key);
    if (encrypted_key.is_empty())
    {
        emit_signal("security_warning", p_id, String(U"Chiffrement de la clé de session impossible."));
        return;
    }
    Dictionary offer;
    offer["key"] = encrypted_key;
    offer["sig"] = crypto->sign(HashingContext::HASH_SHA256, _key_offer_digest(encrypted_key, local_id, p_id), identity_key);
    if (_send_packet(PACKET_KEY_OFFER, UtilityFunctions::var_to_bytes(offer), p_id) != OK)
    {
        return;
    }
    link.outgoing = channel;

    const std::vector<PendingMessage> pending = link.pending_out;
    link.pending_out.clear();
    for (const PendingMessage &message : pending)
    {
        _send_packet(PACKET_SECURE_MESSAGE, channel->encrypt(message.payload), p_id, true, message.channel);
    }
    if (link.incoming.is_valid())
    {
        emit_signal("secure_link_established", p_id);
    }
}

void NetworkSession::_handle_key_offer(int p_from, const PackedByteArray &p_payload)
{
    if (!session_e2e || identity_key.is_null())
    {
        return;
    }
    PeerLink &link = links[p_from];
    if (link.public_key.is_null())
    {
        link.pending_offer = p_payload; // clé publique pas encore reçue (PACKET_PLAYER_JOINED en route)
        return;
    }
    const Variant decoded = UtilityFunctions::bytes_to_var(p_payload);
    if (decoded.get_type() != Variant::DICTIONARY)
    {
        return;
    }
    const Dictionary offer = decoded;
    const PackedByteArray encrypted_key = offer.get("key", PackedByteArray());
    const PackedByteArray signature = offer.get("sig", PackedByteArray());

    Ref<Crypto> crypto;
    crypto.instantiate();
    if (!crypto->verify(HashingContext::HASH_SHA256, _key_offer_digest(encrypted_key, p_from, get_local_peer_id()), signature, link.public_key))
    {
        emit_signal("security_warning", p_from, String(U"Clé de session rejetée : signature invalide (intermédiaire possible)."));
        return;
    }
    const PackedByteArray key = crypto->decrypt(identity_key, encrypted_key);
    if (key.size() != SecureChannel::KEY_SIZE)
    {
        emit_signal("security_warning", p_from, String(U"Clé de session illisible."));
        return;
    }
    Ref<SecureChannel> channel;
    channel.instantiate();
    channel->set_key(key);
    link.incoming = channel;

    const std::vector<PackedByteArray> pending = link.pending_in;
    link.pending_in.clear();
    for (const PackedByteArray &packet : pending)
    {
        _handle_secure_message(p_from, packet);
    }
    if (link.outgoing.is_valid())
    {
        emit_signal("secure_link_established", p_from);
    }
    else
    {
        _try_establish_link(p_from);
    }
}

void NetworkSession::_handle_secure_message(int p_from, const PackedByteArray &p_payload)
{
    if (!session_e2e)
    {
        return;
    }
    PeerLink &link = links[p_from];
    if (link.incoming.is_null())
    {
        if (link.pending_in.size() < MAX_PENDING_MESSAGES)
        {
            link.pending_in.push_back(p_payload);
        }
        return;
    }
    const PackedByteArray plaintext = link.incoming->decrypt(p_payload);
    const Error err = link.incoming->get_last_error();
    if (err != OK)
    {
        emit_signal("security_warning", p_from, String(U"Message chiffré rejeté ({0}).").format(Array::make(UtilityFunctions::error_string(err))));
        return;
    }
    _deliver_message(p_from, plaintext, true);
}

bool NetworkSession::has_secure_link(int p_peer_id) const
{
    const auto found = links.find(p_peer_id);
    return session_e2e && found != links.end() && found->second.outgoing.is_valid() && found->second.incoming.is_valid();
}

String NetworkSession::get_local_fingerprint() const
{
    if (identity_key.is_null())
    {
        return "";
    }
    return SecureChannel::fingerprint(identity_key->save_to_string(true).to_utf8_buffer());
}

String NetworkSession::get_server_fingerprint() const
{
    return is_host() ? get_local_fingerprint() : server_fingerprint;
}

String NetworkSession::get_peer_fingerprint(int p_peer_id) const
{
    if (p_peer_id == get_local_peer_id() && p_peer_id != 0)
    {
        return get_local_fingerprint();
    }
    const auto found = links.find(p_peer_id);
    return found != links.end() ? found->second.fingerprint : String();
}

Dictionary NetworkSession::get_security_summary() const
{
    Dictionary summary;
    summary["transport_encryption"] = session_transport_encrypted;
    // Hôte authentifié seulement si le client a son certificat (sinon chiffré, mais hôte non vérifié).
    summary["transport_verified"] = session_transport_encrypted && (is_host() || trusted_certificate.is_valid());
    summary["end_to_end"] = session_e2e;
    summary["password_protected"] = session_password;
    // Client : identité du serveur prouvée (signature) ET conforme à l'empreinte attendue.
    summary["server_verified"] = is_host() || server_verified;
    summary["dedicated"] = session_dedicated;
    summary["access_mode"] = is_host() && access_policy.is_valid() ? (int)access_policy->get_access_mode() : -1;
    PackedInt32Array secure_links;
    for (const auto &entry : links)
    {
        if (has_secure_link(entry.first))
        {
            secure_links.push_back(entry.first);
        }
    }
    summary["secure_links"] = secure_links;
    return summary;
}

#pragma endregion

#pragma region Getter / Setter

String NetworkSession::get_player_name() const
{
    return player_name.is_empty() ? default_device_name() : player_name;
}

String NetworkSession::get_game_id() const
{
    return game_id.is_empty() ? default_game_id() : game_id;
}

String NetworkSession::get_game_version() const
{
    return game_version.is_empty() ? default_game_version() : game_version;
}

void NetworkSession::set_discovery(Node *p_discovery)
{
    discovery_id = p_discovery != nullptr ? p_discovery->get_instance_id() : 0;
    _update_discovery();
}

Node *NetworkSession::get_discovery() const
{
    return discovery_id != 0 ? Object::cast_to<Node>(ObjectDB::get_instance(discovery_id)) : nullptr;
}

#pragma endregion

void NetworkSession::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("host", "port"), &NetworkSession::host, DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("join", "address", "port"), &NetworkSession::join, DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("join_device", "device"), &NetworkSession::join_device);
    ClassDB::bind_method(D_METHOD("close"), &NetworkSession::close);
    ClassDB::bind_method(D_METHOD("kick", "peer_id", "message"), &NetworkSession::kick, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("get_state"), &NetworkSession::get_state);
    ClassDB::bind_method(D_METHOD("is_online"), &NetworkSession::is_online);
    ClassDB::bind_method(D_METHOD("is_host"), &NetworkSession::is_host);
    ClassDB::bind_method(D_METHOD("get_local_peer_id"), &NetworkSession::get_local_peer_id);
    ClassDB::bind_method(D_METHOD("get_last_reason"), &NetworkSession::get_last_reason);
    ClassDB::bind_method(D_METHOD("get_last_message"), &NetworkSession::get_last_message);

    ClassDB::bind_method(D_METHOD("get_players"), &NetworkSession::get_players);
    ClassDB::bind_method(D_METHOD("get_player_ids"), &NetworkSession::get_player_ids);
    ClassDB::bind_method(D_METHOD("get_player_info", "peer_id"), &NetworkSession::get_player_info);
    ClassDB::bind_method(D_METHOD("has_player", "peer_id"), &NetworkSession::has_player);
    ClassDB::bind_method(D_METHOD("get_player_count"), &NetworkSession::get_player_count);
    ClassDB::bind_method(D_METHOD("update_player_data", "data"), &NetworkSession::update_player_data);

    ClassDB::bind_method(D_METHOD("send_message", "data", "peer_id", "reliable", "channel"), &NetworkSession::send_message, DEFVAL(0), DEFVAL(true), DEFVAL(0));

    ClassDB::bind_method(D_METHOD("is_transport_encrypted"), &NetworkSession::is_transport_encrypted);
    ClassDB::bind_method(D_METHOD("is_end_to_end_active"), &NetworkSession::is_end_to_end_active);
    ClassDB::bind_method(D_METHOD("has_secure_link", "peer_id"), &NetworkSession::has_secure_link);
    ClassDB::bind_method(D_METHOD("get_local_fingerprint"), &NetworkSession::get_local_fingerprint);
    ClassDB::bind_method(D_METHOD("get_peer_fingerprint", "peer_id"), &NetworkSession::get_peer_fingerprint);
    ClassDB::bind_method(D_METHOD("get_security_summary"), &NetworkSession::get_security_summary);

    ClassDB::bind_method(D_METHOD("set_player_name", "name"), &NetworkSession::set_player_name);
    ClassDB::bind_method(D_METHOD("get_player_name"), &NetworkSession::get_player_name);
    ClassDB::bind_method(D_METHOD("set_player_data", "data"), &NetworkSession::set_player_data);
    ClassDB::bind_method(D_METHOD("get_player_data"), &NetworkSession::get_player_data);
    ClassDB::bind_method(D_METHOD("set_game_id", "id"), &NetworkSession::set_game_id);
    ClassDB::bind_method(D_METHOD("get_game_id"), &NetworkSession::get_game_id);
    ClassDB::bind_method(D_METHOD("set_game_version", "version"), &NetworkSession::set_game_version);
    ClassDB::bind_method(D_METHOD("get_game_version"), &NetworkSession::get_game_version);
    ClassDB::bind_method(D_METHOD("set_allow_version_mismatch", "allow"), &NetworkSession::set_allow_version_mismatch);
    ClassDB::bind_method(D_METHOD("is_version_mismatch_allowed"), &NetworkSession::is_version_mismatch_allowed);
    ClassDB::bind_method(D_METHOD("set_transport", "transport"), &NetworkSession::set_transport);
    ClassDB::bind_method(D_METHOD("get_transport"), &NetworkSession::get_transport);
    ClassDB::bind_method(D_METHOD("set_port", "port"), &NetworkSession::set_port);
    ClassDB::bind_method(D_METHOD("get_port"), &NetworkSession::get_port);
    ClassDB::bind_method(D_METHOD("set_max_players", "max"), &NetworkSession::set_max_players);
    ClassDB::bind_method(D_METHOD("get_max_players"), &NetworkSession::get_max_players);
    ClassDB::bind_method(D_METHOD("set_connection_timeout", "timeout"), &NetworkSession::set_connection_timeout);
    ClassDB::bind_method(D_METHOD("get_connection_timeout"), &NetworkSession::get_connection_timeout);
    ClassDB::bind_method(D_METHOD("set_password", "password"), &NetworkSession::set_password);
    ClassDB::bind_method(D_METHOD("get_password"), &NetworkSession::get_password);
    ClassDB::bind_method(D_METHOD("set_transport_encryption", "enabled"), &NetworkSession::set_transport_encryption);
    ClassDB::bind_method(D_METHOD("is_transport_encryption_enabled"), &NetworkSession::is_transport_encryption_enabled);
    ClassDB::bind_method(D_METHOD("set_certificate_common_name", "name"), &NetworkSession::set_certificate_common_name);
    ClassDB::bind_method(D_METHOD("get_certificate_common_name"), &NetworkSession::get_certificate_common_name);
    ClassDB::bind_method(D_METHOD("set_transport_key", "key"), &NetworkSession::set_transport_key);
    ClassDB::bind_method(D_METHOD("get_transport_key"), &NetworkSession::get_transport_key);
    ClassDB::bind_method(D_METHOD("set_transport_certificate", "certificate"), &NetworkSession::set_transport_certificate);
    ClassDB::bind_method(D_METHOD("get_transport_certificate"), &NetworkSession::get_transport_certificate);
    ClassDB::bind_method(D_METHOD("set_trusted_certificate", "certificate"), &NetworkSession::set_trusted_certificate);
    ClassDB::bind_method(D_METHOD("get_trusted_certificate"), &NetworkSession::get_trusted_certificate);
    ClassDB::bind_method(D_METHOD("set_end_to_end_encryption", "enabled"), &NetworkSession::set_end_to_end_encryption);
    ClassDB::bind_method(D_METHOD("is_end_to_end_encryption_enabled"), &NetworkSession::is_end_to_end_encryption_enabled);
    ClassDB::bind_method(D_METHOD("set_identity_key", "key"), &NetworkSession::set_identity_key);
    ClassDB::bind_method(D_METHOD("get_identity_key"), &NetworkSession::get_identity_key);
    ClassDB::bind_method(D_METHOD("get_pending_requests"), &NetworkSession::get_pending_requests);
    ClassDB::bind_method(D_METHOD("approve_peer", "peer_id", "remember", "role"), &NetworkSession::approve_peer, DEFVAL(true), DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("deny_peer", "peer_id", "message"), &NetworkSession::deny_peer, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("ban", "peer_id", "message", "ban_address"), &NetworkSession::ban, DEFVAL(""), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("get_peer_address", "peer_id"), &NetworkSession::get_peer_address);
    ClassDB::bind_method(D_METHOD("is_dedicated_session"), &NetworkSession::is_dedicated_session);
    ClassDB::bind_method(D_METHOD("get_server_fingerprint"), &NetworkSession::get_server_fingerprint);
    ClassDB::bind_method(D_METHOD("create_connection_code", "address", "invite"), &NetworkSession::create_connection_code, DEFVAL(""));
    ClassDB::bind_static_method("NetworkSession", D_METHOD("parse_connection_code", "code"), &NetworkSession::parse_connection_code);
    ClassDB::bind_method(D_METHOD("join_with_code", "code"), &NetworkSession::join_with_code);

    ClassDB::bind_method(D_METHOD("set_dedicated_server", "dedicated"), &NetworkSession::set_dedicated_server);
    ClassDB::bind_method(D_METHOD("is_dedicated_server"), &NetworkSession::is_dedicated_server);
    ClassDB::bind_method(D_METHOD("set_access_policy", "policy"), &NetworkSession::set_access_policy);
    ClassDB::bind_method(D_METHOD("get_access_policy"), &NetworkSession::get_access_policy);
    ClassDB::bind_method(D_METHOD("set_account_name", "name"), &NetworkSession::set_account_name);
    ClassDB::bind_method(D_METHOD("get_account_name"), &NetworkSession::get_account_name);
    ClassDB::bind_method(D_METHOD("set_account_password", "password"), &NetworkSession::set_account_password);
    ClassDB::bind_method(D_METHOD("get_account_password"), &NetworkSession::get_account_password);
    ClassDB::bind_method(D_METHOD("set_invite_code", "code"), &NetworkSession::set_invite_code);
    ClassDB::bind_method(D_METHOD("get_invite_code"), &NetworkSession::get_invite_code);
    ClassDB::bind_method(D_METHOD("set_expected_server_fingerprint", "fingerprint"), &NetworkSession::set_expected_server_fingerprint);
    ClassDB::bind_method(D_METHOD("get_expected_server_fingerprint"), &NetworkSession::get_expected_server_fingerprint);
    ClassDB::bind_method(D_METHOD("set_discovery", "discovery"), &NetworkSession::set_discovery);
    ClassDB::bind_method(D_METHOD("get_discovery"), &NetworkSession::get_discovery);

    ADD_GROUP(U"Joueur", "");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "player_name", PROPERTY_HINT_PLACEHOLDER_TEXT, U"Utilisateur du système"), "set_player_name", "get_player_name");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "player_data"), "set_player_data", "get_player_data");

    ADD_GROUP(U"Jeu", "");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "game_id", PROPERTY_HINT_PLACEHOLDER_TEXT, "application/config/name"), "set_game_id", "get_game_id");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "game_version", PROPERTY_HINT_PLACEHOLDER_TEXT, "application/config/version"), "set_game_version", "get_game_version");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_version_mismatch"), "set_allow_version_mismatch", "is_version_mismatch_allowed");

    ADD_GROUP(U"Connexion", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "transport", PROPERTY_HINT_ENUM, "ENet (UDP),WebSocket (TCP)"), "set_transport", "get_transport");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "port", PROPERTY_HINT_RANGE, "1024,65535"), "set_port", "get_port");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_players", PROPERTY_HINT_RANGE, "1,64"), "set_max_players", "get_max_players");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "connection_timeout", PROPERTY_HINT_RANGE, "0.5,60,0.5,suffix:s"), "set_connection_timeout", "get_connection_timeout");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "password", PROPERTY_HINT_PASSWORD), "set_password", "get_password");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "discovery", PROPERTY_HINT_NODE_TYPE, "LanDiscovery"), "set_discovery", "get_discovery");

    ADD_GROUP(U"Serveur", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dedicated_server"), "set_dedicated_server", "is_dedicated_server");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "access_policy", PROPERTY_HINT_RESOURCE_TYPE, "ServerAccessPolicy"), "set_access_policy", "get_access_policy");

    ADD_GROUP(U"Identifiants (client)", "");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "account_name"), "set_account_name", "get_account_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "account_password", PROPERTY_HINT_PASSWORD), "set_account_password", "get_account_password");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "invite_code"), "set_invite_code", "get_invite_code");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "expected_server_fingerprint"), "set_expected_server_fingerprint", "get_expected_server_fingerprint");

    ADD_GROUP(U"Sécurité", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "transport_encryption"), "set_transport_encryption", "is_transport_encryption_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "end_to_end_encryption"), "set_end_to_end_encryption", "is_end_to_end_encryption_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "certificate_common_name"), "set_certificate_common_name", "get_certificate_common_name");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "transport_key", PROPERTY_HINT_RESOURCE_TYPE, "CryptoKey"), "set_transport_key", "get_transport_key");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "transport_certificate", PROPERTY_HINT_RESOURCE_TYPE, "X509Certificate"), "set_transport_certificate", "get_transport_certificate");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "trusted_certificate", PROPERTY_HINT_RESOURCE_TYPE, "X509Certificate"), "set_trusted_certificate", "get_trusted_certificate");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "identity_key", PROPERTY_HINT_RESOURCE_TYPE, "CryptoKey"), "set_identity_key", "get_identity_key");

    ADD_SIGNAL(MethodInfo("state_changed", PropertyInfo(Variant::INT, "state", PROPERTY_HINT_ENUM, "Offline,Connecting,Authenticating,Connected,Hosting")));
    ADD_SIGNAL(MethodInfo("hosting_started", PropertyInfo(Variant::INT, "port")));
    ADD_SIGNAL(MethodInfo("session_joined", PropertyInfo(Variant::INT, "local_peer_id")));
    ADD_SIGNAL(MethodInfo("connection_failed", PropertyInfo(Variant::INT, "reason"), PropertyInfo(Variant::STRING, "message")));
    ADD_SIGNAL(MethodInfo("session_ended", PropertyInfo(Variant::INT, "reason"), PropertyInfo(Variant::STRING, "message")));
    ADD_SIGNAL(MethodInfo("peer_rejected", PropertyInfo(Variant::INT, "peer_id"), PropertyInfo(Variant::INT, "reason"), PropertyInfo(Variant::STRING, "message")));
    ADD_SIGNAL(MethodInfo("player_joined", PropertyInfo(Variant::INT, "peer_id"), PropertyInfo(Variant::DICTIONARY, "info")));
    ADD_SIGNAL(MethodInfo("player_left", PropertyInfo(Variant::INT, "peer_id"), PropertyInfo(Variant::DICTIONARY, "info")));
    ADD_SIGNAL(MethodInfo("player_updated", PropertyInfo(Variant::INT, "peer_id"), PropertyInfo(Variant::DICTIONARY, "info")));
    ADD_SIGNAL(MethodInfo("message_received", PropertyInfo(Variant::INT, "from_peer_id"), PropertyInfo(Variant::NIL, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT), PropertyInfo(Variant::BOOL, "encrypted")));
    ADD_SIGNAL(MethodInfo("secure_link_established", PropertyInfo(Variant::INT, "peer_id")));
    ADD_SIGNAL(MethodInfo("security_warning", PropertyInfo(Variant::INT, "peer_id"), PropertyInfo(Variant::STRING, "message")));
    ADD_SIGNAL(MethodInfo("access_requested", PropertyInfo(Variant::INT, "peer_id"), PropertyInfo(Variant::DICTIONARY, "request")));
    ADD_SIGNAL(MethodInfo("waiting_for_approval", PropertyInfo(Variant::FLOAT, "timeout")));

    BIND_ENUM_CONSTANT(TRANSPORT_ENET);
    BIND_ENUM_CONSTANT(TRANSPORT_WEBSOCKET);

    BIND_ENUM_CONSTANT(STATE_OFFLINE);
    BIND_ENUM_CONSTANT(STATE_CONNECTING);
    BIND_ENUM_CONSTANT(STATE_AUTHENTICATING);
    BIND_ENUM_CONSTANT(STATE_CONNECTED);
    BIND_ENUM_CONSTANT(STATE_HOSTING);

    BIND_ENUM_CONSTANT(REASON_NONE);
    BIND_ENUM_CONSTANT(REASON_CLOSED);
    BIND_ENUM_CONSTANT(REASON_CONNECTION_FAILED);
    BIND_ENUM_CONSTANT(REASON_HOST_LOST);
    BIND_ENUM_CONSTANT(REASON_TIMEOUT);
    BIND_ENUM_CONSTANT(REASON_WRONG_PASSWORD);
    BIND_ENUM_CONSTANT(REASON_GAME_MISMATCH);
    BIND_ENUM_CONSTANT(REASON_VERSION_MISMATCH);
    BIND_ENUM_CONSTANT(REASON_SESSION_FULL);
    BIND_ENUM_CONSTANT(REASON_SECURITY_MISMATCH);
    BIND_ENUM_CONSTANT(REASON_INVALID_HANDSHAKE);
    BIND_ENUM_CONSTANT(REASON_KICKED);
    BIND_ENUM_CONSTANT(REASON_ACCESS_DENIED);
    BIND_ENUM_CONSTANT(REASON_BANNED);
    BIND_ENUM_CONSTANT(REASON_TOO_MANY_ATTEMPTS);
}
