#ifndef LAN_DISCOVERY_HPP
#define LAN_DISCOVERY_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packet_peer_udp.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <map>

namespace godot
{
/// 📡 Découverte des appareils à proximité (réseau local) qui exécutent un jeu fait avec ce module.
///
/// Chaque appareil qui annonce (advertise) répond aux recherches avec son système d'exploitation, son
/// modèle, son jeu et sa version, et la partie qu'il héberge éventuellement (joueurs, mot de passe,
/// chiffrement). Un appareil qui cherche (start_search) envoie une requête en broadcast UDP sur une
/// plage de ports et tient à jour la liste des appareils qui répondent.
///
/// Un appareil SANS le logiciel ne répond jamais : il n'apparaît pas dans la liste. Pour rejoindre une
/// partie, un appareil doit exécuter le même jeu (même game_id) et, sauf exception, la même version :
/// la clé « compatibility » de chaque appareil le signale.
class LanDiscovery : public Node
{
    GDCLASS(LanDiscovery, Node)

  public:
    enum Compatibility
    {
        COMPATIBLE,        ///< Même jeu, même version, même protocole.
        VERSION_MISMATCH,  ///< Même jeu, autre version : l'un des deux appareils doit se mettre à jour.
        OTHER_GAME,        ///< Autre jeu fait avec ce module.
        PROTOCOL_MISMATCH, ///< Version du module réseau différente : mise à jour nécessaire.
    };

  private:
    enum PacketType
    {
        PACKET_QUERY = 1,
        PACKET_ANNOUNCE = 2,
        PACKET_BYE = 3,
    };

    /// Appareil qui nous a interrogés récemment : prévenu tout de suite d'un changement ou d'un départ.
    struct Searcher
    {
        String address;
        int port = 0;
        uint64_t last_query_msec = 0;
    };

#pragma region Réglages
    int port = 47474;
    int port_range = 4;
    bool advertise = true;
    bool autostart = true;
    float query_interval = 1.0f;
    float device_timeout = 4.0f;
    String device_name;
    String game_id;
    String game_version;
    PackedStringArray extra_query_addresses;
    bool include_other_games = true;
#pragma endregion

#pragma region État
    Ref<PacketPeerUDP> socket;
    int bound_port = 0;
    bool running = false;
    bool searching = false;
    double query_cooldown = 0.0;
    /// Identifiant aléatoire de cette instance (distingue deux jeux lancés sur la même machine).
    String device_id;
    Dictionary session_info;
    /// Appareils trouvés, par device_id.
    std::map<String, Dictionary> devices;
    std::map<String, Searcher> searchers;
#pragma endregion

    Error _open_socket();
    void _close_socket();
    void _send(const String &p_address, int p_port, PacketType p_type, const Dictionary &p_payload);
    void _send_query();
    void _announce_to_searchers(PacketType p_type);
    void _receive_packets();
    void _handle_query(const Dictionary &p_payload, const String &p_address, int p_port);
    void _handle_announce(const Dictionary &p_payload, const String &p_address);
    void _handle_bye(const Dictionary &p_payload);
    void _expire_devices();
    Dictionary _build_announce() const;
    Compatibility _compatibility_of(const Dictionary &p_announce) const;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  public:
    LanDiscovery();

    void _process(double p_delta) override;

    /// Ouvre le port de découverte : l'appareil répond aux recherches si advertise est vrai.
    Error start();
    /// Ferme le port, prévient les appareils qui nous cherchaient et vide la liste.
    void stop();
    bool is_running() const { return running; }

    /// Commence à chercher les appareils (ouvre le port si besoin). Une requête part aussitôt, puis
    /// toutes les query_interval secondes.
    Error start_search();
    void stop_search();
    bool is_searching() const { return searching; }
    /// Envoie une requête immédiatement (sans attendre query_interval).
    void refresh();

    /// Liste des appareils trouvés (Dictionary par appareil, voir la documentation de la classe).
    Array get_devices() const;
    /// Appareils hébergeant une partie que cet appareil peut rejoindre (can_join vrai).
    Array get_joinable_devices() const;
    Dictionary get_device(const String &p_device_id) const;
    int get_device_count() const { return (int)devices.size(); }

    /// Ce que cet appareil annonce : identité, système, jeu, et session hébergée.
    Dictionary get_local_device_info() const;
    String get_device_id() const { return device_id; }
    /// Port réellement ouvert (0 si arrêté) : le premier libre de [port ; port + port_range[.
    int get_bound_port() const { return bound_port; }

    /// Informations de la partie hébergée, ajoutées aux annonces (renseignées par NetworkSession :
    /// hosting, port, transport, players, max_players, password_required, transport_encryption,
    /// end_to_end_encryption). Les appareils qui nous cherchent sont prévenus aussitôt.
    void set_session_info(const Dictionary &p_info);
    Dictionary get_session_info() const { return session_info; }
    void clear_session_info();

#pragma region Getter / Setter
    void set_port(int p_port) { port = p_port; }
    int get_port() const { return port; }
    void set_port_range(int p_range) { port_range = MAX(1, p_range); }
    int get_port_range() const { return port_range; }
    void set_advertise(bool p_advertise);
    bool is_advertising() const { return advertise; }
    void set_autostart(bool p_autostart) { autostart = p_autostart; }
    bool is_autostart() const { return autostart; }
    void set_query_interval(float p_interval) { query_interval = MAX(0.05f, p_interval); }
    float get_query_interval() const { return query_interval; }
    void set_device_timeout(float p_timeout) { device_timeout = MAX(0.1f, p_timeout); }
    float get_device_timeout() const { return device_timeout; }
    void set_device_name(const String &p_name);
    String get_device_name() const;
    void set_game_id(const String &p_id);
    String get_game_id() const;
    void set_game_version(const String &p_version);
    String get_game_version() const;
    void set_extra_query_addresses(const PackedStringArray &p_addresses) { extra_query_addresses = p_addresses; }
    PackedStringArray get_extra_query_addresses() const { return extra_query_addresses; }
    void set_include_other_games(bool p_include) { include_other_games = p_include; }
    bool is_including_other_games() const { return include_other_games; }
#pragma endregion
};
} // namespace godot

VARIANT_ENUM_CAST(LanDiscovery::Compatibility);

#endif // LAN_DISCOVERY_HPP
