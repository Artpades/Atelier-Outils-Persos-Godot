#ifndef PORT_FORWARDER_HPP
#define PORT_FORWARDER_HPP

#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/upnp.hpp>

namespace godot
{
/// 🌍 Rend un serveur joignable depuis Internet : ouvre son port sur la box (UPnP), trouve l'adresse
/// publique, et détecte les cas où c'est impossible (double NAT, CGNAT de l'opérateur).
///
/// La recherche de la box (UPnP) bloque quelques secondes : elle tourne dans un thread, le résultat
/// arrive par signal. Le port ouvert est refermé à la sortie de l'arbre.
class PortForwarder : public Node
{
    GDCLASS(PortForwarder, Node)

  public:
    enum AddressKind
    {
        ADDRESS_PUBLIC,   ///< Joignable depuis Internet.
        ADDRESS_PRIVATE,  ///< Réseau local (10.x, 172.16-31.x, 192.168.x, fc00::/7, fe80::/10).
        ADDRESS_CGNAT,    ///< Partagée par l'opérateur (100.64.0.0/10) : redirection de port impossible.
        ADDRESS_LOOPBACK, ///< Cette machine (127.x, ::1).
        ADDRESS_INVALID,
    };

    enum Status
    {
        STATUS_IDLE,
        STATUS_WORKING,  ///< Recherche de la box / ouverture en cours.
        STATUS_OPEN,     ///< Port ouvert sur la box.
        STATUS_FAILED,
    };

  private:
    int port = 47500;
    bool forward_udp = true;
    bool forward_tcp = true;
    String description = "GameplayToolkit";
    int lease_duration = 0;
    int discover_timeout = 2000;
    String public_ip_service_url = "https://api.ipify.org";

    Status status = STATUS_IDLE;
    Ref<Thread> thread;
    Ref<UPNP> upnp;
    /// Résultat du thread : lu dans _process une fois le thread terminé.
    Dictionary thread_result;
    int mapped_port = 0;
    /// Adresse publique vue par la box (UPnP) et vue depuis Internet (service HTTP).
    String external_address;
    String public_address;
    String last_message;
    HTTPRequest *http = nullptr;

    void _open_in_thread(int p_port);
    void _finish_thread();
    void _on_http_completed(int p_result, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_body);
    void _check_reachability(const String &p_address);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  public:
    void _process(double p_delta) override;

    /// Ouvre p_port (port si -1) sur la box, en UDP et/ou TCP. Résultat : port_opened ou port_failed.
    Error open_port(int p_port = -1);
    /// Referme le port ouvert (appelé automatiquement à la sortie de l'arbre).
    void close_port();
    /// Demande l'adresse publique à public_ip_service_url (service HTTP externe qui voit cette adresse).
    /// Résultat : public_address_found ou public_address_failed.
    Error find_public_address();

    Status get_status() const { return status; }
    bool is_port_open() const { return status == STATUS_OPEN; }
    String get_external_address() const { return external_address; }
    String get_public_address() const { return public_address; }
    String get_last_message() const { return last_message; }

    /// Adresses IPv4 de cette machine sur le réseau local (pour un joueur du même réseau).
    static PackedStringArray get_local_addresses();
    /// Nature d'une adresse IP : publique, privée, CGNAT, boucle locale.
    static AddressKind classify_address(const String &p_address);

#pragma region Getter / Setter
    void set_port(int p_port) { port = p_port; }
    int get_port() const { return port; }
    void set_forward_udp(bool p_enabled) { forward_udp = p_enabled; }
    bool is_forwarding_udp() const { return forward_udp; }
    void set_forward_tcp(bool p_enabled) { forward_tcp = p_enabled; }
    bool is_forwarding_tcp() const { return forward_tcp; }
    void set_description(const String &p_description) { description = p_description; }
    String get_description() const { return description; }
    void set_lease_duration(int p_seconds) { lease_duration = MAX(0, p_seconds); }
    int get_lease_duration() const { return lease_duration; }
    void set_discover_timeout(int p_msec) { discover_timeout = MAX(100, p_msec); }
    int get_discover_timeout() const { return discover_timeout; }
    void set_public_ip_service_url(const String &p_url) { public_ip_service_url = p_url; }
    String get_public_ip_service_url() const { return public_ip_service_url; }
#pragma endregion
};
} // namespace godot

VARIANT_ENUM_CAST(PortForwarder::AddressKind);
VARIANT_ENUM_CAST(PortForwarder::Status);

#endif // PORT_FORWARDER_HPP
