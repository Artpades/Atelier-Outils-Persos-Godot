#include "port_forwarder.hpp"

#include <godot_cpp/classes/ip.hpp>
#include <godot_cpp/classes/upnp_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
String upnp_error(int p_result)
{
    switch (p_result)
    {
    case UPNP::UPNP_RESULT_NO_GATEWAY:
    case UPNP::UPNP_RESULT_NO_DEVICES:
        return U"Aucune box compatible UPnP trouvée (UPnP désactivé dans la box ?).";
    case UPNP::UPNP_RESULT_NOT_AUTHORIZED:
    case UPNP::UPNP_RESULT_ACTION_FAILED:
        return U"La box refuse d'ouvrir le port (UPnP restreint).";
    case UPNP::UPNP_RESULT_CONFLICT_WITH_OTHER_MAPPING:
    case UPNP::UPNP_RESULT_CONFLICT_WITH_OTHER_MECHANISM:
    case UPNP::UPNP_RESULT_SAME_PORT_VALUES_REQUIRED:
        return U"Ce port est déjà redirigé vers un autre appareil.";
    default:
        return String(U"Erreur UPnP {0}.").format(Array::make(p_result));
    }
}
} // namespace

void PortForwarder::_notification(int p_what)
{
    if (p_what == NOTIFICATION_EXIT_TREE)
    {
        if (thread.is_valid() && thread->is_started())
        {
            thread->wait_to_finish(); // au plus discover_timeout
            _finish_thread();
        }
        close_port();
    }
}

#pragma region UPnP

Error PortForwarder::open_port(int p_port)
{
    ERR_FAIL_COND_V_MSG(status == STATUS_WORKING, ERR_BUSY, U"PortForwarder : ouverture déjà en cours.");
    if (status == STATUS_OPEN)
    {
        close_port();
    }
    status = STATUS_WORKING;
    thread_result = Dictionary();
    thread.instantiate();
    return thread->start(callable_mp(this, &PortForwarder::_open_in_thread).bind(p_port > 0 ? p_port : port));
}

void PortForwarder::_open_in_thread(int p_port)
{
    // Dans le thread : aucun accès à l'arbre ni signal, seulement thread_result (lu après wait_to_finish).
    Dictionary result;
    result["port"] = p_port;
    Ref<UPNP> device_finder;
    device_finder.instantiate();
    const int discovered = device_finder->discover(discover_timeout, 2, "InternetGatewayDevice");
    const Ref<UPNPDevice> gateway = device_finder->get_gateway();
    if (discovered != UPNP::UPNP_RESULT_SUCCESS || gateway.is_null() || !gateway->is_valid_gateway())
    {
        result["ok"] = false;
        result["message"] = upnp_error(discovered == UPNP::UPNP_RESULT_SUCCESS ? (int)UPNP::UPNP_RESULT_NO_GATEWAY : discovered);
        thread_result = result;
        return;
    }

    PackedStringArray protocols;
    if (forward_udp)
    {
        protocols.push_back("UDP");
    }
    if (forward_tcp)
    {
        protocols.push_back("TCP");
    }
    for (const String &protocol : protocols)
    {
        int mapped = device_finder->add_port_mapping(p_port, p_port, description, protocol, lease_duration);
        if (mapped != UPNP::UPNP_RESULT_SUCCESS && lease_duration == 0)
        {
            // Certaines box refusent un bail illimité : nouvel essai avec un bail d'une heure.
            mapped = device_finder->add_port_mapping(p_port, p_port, description, protocol, 3600);
        }
        if (mapped != UPNP::UPNP_RESULT_SUCCESS)
        {
            result["ok"] = false;
            result["message"] = String(U"{0} ({1})").format(Array::make(upnp_error(mapped), protocol));
            thread_result = result;
            return;
        }
    }
    result["ok"] = true;
    result["external"] = device_finder->query_external_address();
    result["upnp"] = device_finder;
    thread_result = result;
}

void PortForwarder::_process(double p_delta)
{
    (void)p_delta;
    if (thread.is_valid() && thread->is_started() && !thread->is_alive())
    {
        thread->wait_to_finish();
        _finish_thread();
    }
}

void PortForwarder::_finish_thread()
{
    thread.unref();
    const Dictionary result = thread_result;
    thread_result = Dictionary();
    if (!(bool)result.get("ok", false))
    {
        status = STATUS_FAILED;
        last_message = result.get("message", U"Échec de l'ouverture du port.");
        emit_signal("port_failed", last_message);
        return;
    }
    status = STATUS_OPEN;
    upnp = result["upnp"];
    mapped_port = result["port"];
    external_address = result.get("external", "");
    last_message = String(U"Port {0} ouvert sur la box.").format(Array::make(mapped_port));
    emit_signal("port_opened", external_address, mapped_port);
    _check_reachability(external_address);
}

void PortForwarder::close_port()
{
    if (upnp.is_valid() && mapped_port > 0)
    {
        if (forward_udp)
        {
            upnp->delete_port_mapping(mapped_port, "UDP");
        }
        if (forward_tcp)
        {
            upnp->delete_port_mapping(mapped_port, "TCP");
        }
    }
    upnp.unref();
    mapped_port = 0;
    if (status == STATUS_OPEN)
    {
        status = STATUS_IDLE;
        emit_signal("port_closed");
    }
}

void PortForwarder::_check_reachability(const String &p_address)
{
    switch (classify_address(p_address))
    {
    case ADDRESS_CGNAT:
        emit_signal("nat_warning", String(U"La box n'a pas d'adresse publique ({0} est partagée par l'opérateur : CGNAT). "
                                          U"Les appareils distants ne pourront pas joindre ce serveur : demander une IP publique à l'opérateur, "
                                          U"héberger le serveur sur une machine louée (VPS), ou utiliser un VPN maillé (Tailscale, ZeroTier...).")
                                       .format(Array::make(p_address)));
        break;
    case ADDRESS_PRIVATE:
        emit_signal("nat_warning", String(U"La box est elle-même derrière un autre routeur (adresse {0} : double NAT). "
                                          U"Ouvrir aussi le port sur ce routeur, ou passer la box en mode « bridge ».")
                                       .format(Array::make(p_address)));
        break;
    default:
        break;
    }
}

#pragma endregion

#pragma region Adresse publique

Error PortForwarder::find_public_address()
{
    ERR_FAIL_COND_V_MSG(public_ip_service_url.is_empty(), ERR_UNCONFIGURED, U"PortForwarder : public_ip_service_url est vide.");
    if (http == nullptr)
    {
        http = memnew(HTTPRequest);
        http->set_timeout(10.0);
        add_child(http, false, INTERNAL_MODE_FRONT);
        http->connect("request_completed", callable_mp(this, &PortForwarder::_on_http_completed));
    }
    http->cancel_request();
    return http->request(public_ip_service_url);
}

void PortForwarder::_on_http_completed(int p_result, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_body)
{
    (void)p_headers;
    const String address = p_body.get_string_from_utf8().strip_edges();
    if (p_result != HTTPRequest::RESULT_SUCCESS || p_code != 200 || !address.is_valid_ip_address())
    {
        emit_signal("public_address_failed", String(U"Adresse publique introuvable (résultat {0}, code HTTP {1}).").format(Array::make(p_result, p_code)));
        return;
    }
    public_address = address;
    emit_signal("public_address_found", address);
    if (status == STATUS_OPEN && !external_address.is_empty() && external_address != address)
    {
        emit_signal("nat_warning", String(U"La box annonce l'adresse {0}, mais Internet voit {1} : double NAT ou CGNAT, le port ouvert sur la box ne suffit pas.")
                                       .format(Array::make(external_address, address)));
    }
}

#pragma endregion

#pragma region Adresses

PackedStringArray PortForwarder::get_local_addresses()
{
    PackedStringArray result;
    const PackedStringArray addresses = IP::get_singleton()->get_local_addresses();
    for (const String &address : addresses)
    {
        if (address.contains(".") && classify_address(address) == ADDRESS_PRIVATE && !address.begins_with("169.254."))
        {
            result.push_back(address);
        }
    }
    return result;
}

PortForwarder::AddressKind PortForwarder::classify_address(const String &p_address)
{
    String address = p_address.strip_edges().to_lower();
    if (address.begins_with("::ffff:") && address.contains("."))
    {
        address = address.substr(7); // IPv4 encapsulée dans une IPv6
    }
    if (!address.is_valid_ip_address())
    {
        return ADDRESS_INVALID;
    }
    if (address.contains("."))
    {
        const PackedStringArray parts = address.split(".");
        const int a = parts[0].to_int();
        const int b = parts[1].to_int();
        if (a == 127)
        {
            return ADDRESS_LOOPBACK;
        }
        if (a == 10 || (a == 172 && b >= 16 && b <= 31) || (a == 192 && b == 168) || (a == 169 && b == 254))
        {
            return ADDRESS_PRIVATE;
        }
        if (a == 100 && b >= 64 && b <= 127)
        {
            return ADDRESS_CGNAT;
        }
        return ADDRESS_PUBLIC;
    }
    if (address == "::1")
    {
        return ADDRESS_LOOPBACK;
    }
    if (address.begins_with("fc") || address.begins_with("fd") || address.begins_with("fe8") || address.begins_with("fe9") ||
        address.begins_with("fea") || address.begins_with("feb"))
    {
        return ADDRESS_PRIVATE;
    }
    return ADDRESS_PUBLIC;
}

#pragma endregion

void PortForwarder::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("open_port", "port"), &PortForwarder::open_port, DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("close_port"), &PortForwarder::close_port);
    ClassDB::bind_method(D_METHOD("find_public_address"), &PortForwarder::find_public_address);
    ClassDB::bind_method(D_METHOD("get_status"), &PortForwarder::get_status);
    ClassDB::bind_method(D_METHOD("is_port_open"), &PortForwarder::is_port_open);
    ClassDB::bind_method(D_METHOD("get_external_address"), &PortForwarder::get_external_address);
    ClassDB::bind_method(D_METHOD("get_public_address"), &PortForwarder::get_public_address);
    ClassDB::bind_method(D_METHOD("get_last_message"), &PortForwarder::get_last_message);
    ClassDB::bind_static_method("PortForwarder", D_METHOD("get_local_addresses"), &PortForwarder::get_local_addresses);
    ClassDB::bind_static_method("PortForwarder", D_METHOD("classify_address", "address"), &PortForwarder::classify_address);

    ClassDB::bind_method(D_METHOD("set_port", "port"), &PortForwarder::set_port);
    ClassDB::bind_method(D_METHOD("get_port"), &PortForwarder::get_port);
    ClassDB::bind_method(D_METHOD("set_forward_udp", "enabled"), &PortForwarder::set_forward_udp);
    ClassDB::bind_method(D_METHOD("is_forwarding_udp"), &PortForwarder::is_forwarding_udp);
    ClassDB::bind_method(D_METHOD("set_forward_tcp", "enabled"), &PortForwarder::set_forward_tcp);
    ClassDB::bind_method(D_METHOD("is_forwarding_tcp"), &PortForwarder::is_forwarding_tcp);
    ClassDB::bind_method(D_METHOD("set_description", "description"), &PortForwarder::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &PortForwarder::get_description);
    ClassDB::bind_method(D_METHOD("set_lease_duration", "seconds"), &PortForwarder::set_lease_duration);
    ClassDB::bind_method(D_METHOD("get_lease_duration"), &PortForwarder::get_lease_duration);
    ClassDB::bind_method(D_METHOD("set_discover_timeout", "msec"), &PortForwarder::set_discover_timeout);
    ClassDB::bind_method(D_METHOD("get_discover_timeout"), &PortForwarder::get_discover_timeout);
    ClassDB::bind_method(D_METHOD("set_public_ip_service_url", "url"), &PortForwarder::set_public_ip_service_url);
    ClassDB::bind_method(D_METHOD("get_public_ip_service_url"), &PortForwarder::get_public_ip_service_url);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "port", PROPERTY_HINT_RANGE, "1024,65535"), "set_port", "get_port");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "forward_udp"), "set_forward_udp", "is_forwarding_udp");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "forward_tcp"), "set_forward_tcp", "is_forwarding_tcp");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description"), "set_description", "get_description");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "lease_duration", PROPERTY_HINT_RANGE, "0,86400,1,suffix:s"), "set_lease_duration", "get_lease_duration");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "discover_timeout", PROPERTY_HINT_RANGE, "100,10000,1,suffix:ms"), "set_discover_timeout", "get_discover_timeout");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "public_ip_service_url"), "set_public_ip_service_url", "get_public_ip_service_url");

    ADD_SIGNAL(MethodInfo("port_opened", PropertyInfo(Variant::STRING, "external_address"), PropertyInfo(Variant::INT, "port")));
    ADD_SIGNAL(MethodInfo("port_failed", PropertyInfo(Variant::STRING, "message")));
    ADD_SIGNAL(MethodInfo("port_closed"));
    ADD_SIGNAL(MethodInfo("public_address_found", PropertyInfo(Variant::STRING, "address")));
    ADD_SIGNAL(MethodInfo("public_address_failed", PropertyInfo(Variant::STRING, "message")));
    ADD_SIGNAL(MethodInfo("nat_warning", PropertyInfo(Variant::STRING, "message")));

    BIND_ENUM_CONSTANT(ADDRESS_PUBLIC);
    BIND_ENUM_CONSTANT(ADDRESS_PRIVATE);
    BIND_ENUM_CONSTANT(ADDRESS_CGNAT);
    BIND_ENUM_CONSTANT(ADDRESS_LOOPBACK);
    BIND_ENUM_CONSTANT(ADDRESS_INVALID);
    BIND_ENUM_CONSTANT(STATUS_IDLE);
    BIND_ENUM_CONSTANT(STATUS_WORKING);
    BIND_ENUM_CONSTANT(STATUS_OPEN);
    BIND_ENUM_CONSTANT(STATUS_FAILED);
}
