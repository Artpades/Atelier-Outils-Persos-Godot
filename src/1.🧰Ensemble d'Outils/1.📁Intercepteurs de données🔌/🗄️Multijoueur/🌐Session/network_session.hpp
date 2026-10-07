#ifndef NETWORK_SESSION_HPP
#define NETWORK_SESSION_HPP

#include "../🔐Chiffrement/secure_channel.hpp"

#include <godot_cpp/classes/crypto_key.hpp>
#include <godot_cpp/classes/multiplayer_peer.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_multiplayer.hpp>
#include <godot_cpp/classes/tls_options.hpp>
#include <godot_cpp/classes/x509_certificate.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <map>
#include <vector>

namespace godot
{
class LanDiscovery;

/// 🌐 Session multijoueur : héberger ou rejoindre une partie (ENet/UDP ou WebSocket), poignée de main
/// (même jeu, version, mot de passe, exigences de sécurité), liste des joueurs avec leur système
/// d'exploitation, et messages entre joueurs.
///
/// Utilise le MultiplayerAPI de sa branche (get_multiplayer(), un SceneMultiplayer) : les RPC, les
/// MultiplayerSpawner et MultiplayerSynchronizer du jeu fonctionnent normalement par-dessus.
///
/// Sécurité, deux niveaux cumulables :
/// - transport_encryption : tout le trafic (RPC compris) est chiffré entre chaque client et l'hôte
///   (DTLS pour ENet, TLS / wss:// pour WebSocket).
/// - end_to_end_encryption : les messages de send_message sont chiffrés de l'émetteur au destinataire
///   (AES-256 + HMAC-SHA256, clés échangées par RSA et signées). L'hôte qui relaie un message entre deux
///   clients ne peut pas le lire. Comparer les empreintes (get_local_fingerprint / get_peer_fingerprint)
///   permet de vérifier qu'aucun intermédiaire n'a substitué de clé.
class NetworkSession : public Node
{
    GDCLASS(NetworkSession, Node)

  public:
    enum Transport
    {
        TRANSPORT_ENET,      ///< UDP (ENet) : faible latence, DTLS pour le chiffrement.
        TRANSPORT_WEBSOCKET, ///< TCP (WebSocket) : compatible avec un export Web, TLS pour le chiffrement.
    };

    enum State
    {
        STATE_OFFLINE,
        STATE_CONNECTING,     ///< Connexion à l'hôte en cours (transport).
        STATE_AUTHENTICATING, ///< Connecté, poignée de main en cours.
        STATE_CONNECTED,      ///< Client accepté dans la partie.
        STATE_HOSTING,        ///< Hôte de la partie.
    };

    enum Reason
    {
        REASON_NONE,
        REASON_CLOSED,             ///< Fermeture demandée localement (close).
        REASON_CONNECTION_FAILED,  ///< Hôte injoignable ou poignée de main du transport refusée.
        REASON_HOST_LOST,          ///< Connexion à l'hôte perdue.
        REASON_TIMEOUT,            ///< Poignée de main trop longue.
        REASON_WRONG_PASSWORD,
        REASON_GAME_MISMATCH,      ///< Autre jeu (game_id différent).
        REASON_VERSION_MISMATCH,   ///< Autre version du jeu ou du protocole.
        REASON_SESSION_FULL,
        REASON_SECURITY_MISMATCH,  ///< Exigences de chiffrement incompatibles.
        REASON_INVALID_HANDSHAKE,
        REASON_KICKED,
    };

  private:
    /// Types des paquets du module envoyés par send_bytes (après l'octet PACKET_MAGIC).
    enum PacketType
    {
        PACKET_PLAYER_JOINED = 1,
        PACKET_PLAYER_UPDATE = 2,
        PACKET_MESSAGE = 3,
        PACKET_SECURE_MESSAGE = 4,
        PACKET_KEY_OFFER = 5,
        PACKET_KICK = 6,
        PACKET_ROSTER = 7,
    };

    /// Message en attente de la clé de bout en bout du destinataire.
    struct PendingMessage
    {
        PackedByteArray payload;
        int channel = 0;
    };

    /// Chiffrement de bout en bout avec un autre joueur : un canal par sens.
    struct PeerLink
    {
        Ref<CryptoKey> public_key;
        String fingerprint;
        /// Nos messages vers ce joueur (clé générée par nous).
        Ref<SecureChannel> outgoing;
        /// Ses messages vers nous (clé reçue de lui).
        Ref<SecureChannel> incoming;
        std::vector<PendingMessage> pending_out;
        /// Messages chiffrés reçus avant sa clé (canaux différents : l'ordre n'est pas garanti).
        std::vector<PackedByteArray> pending_in;
        /// Offre de clé reçue avant sa clé publique (paquets relayés par l'hôte).
        PackedByteArray pending_offer;
    };

    /// Client en cours de poignée de main (côté hôte).
    struct PendingClient
    {
        PackedByteArray nonce;
        Dictionary info;
    };

#pragma region Réglages
    String player_name;
    Dictionary player_data;
    String game_id;
    String game_version;
    bool allow_version_mismatch = false;
    Transport transport = TRANSPORT_ENET;
    int port = 47500;
    int max_players = 8;
    float connection_timeout = 8.0f;
    String password;

    bool transport_encryption = false;
    String certificate_common_name = "gameplaytoolkit.local";
    Ref<CryptoKey> transport_key;
    Ref<X509Certificate> transport_certificate;
    Ref<X509Certificate> trusted_certificate;
    bool end_to_end_encryption = false;
    Ref<CryptoKey> identity_key;
    uint64_t discovery_id = 0;
#pragma endregion

#pragma region État
    State state = STATE_OFFLINE;
    Ref<SceneMultiplayer> api;
    Ref<MultiplayerPeer> peer;
    /// Joueurs acceptés, par identifiant réseau (l'hôte est 1).
    std::map<int, Dictionary> players;
    std::map<int, PendingClient> pending_clients;
    std::map<int, PeerLink> links;
    /// Clients à déconnecter (refusés ou expulsés) : identifiant -> heure limite (ms).
    std::map<int, uint64_t> pending_kicks;
    /// Chiffrement de bout en bout de la partie en cours (décidé par l'hôte).
    bool session_e2e = false;
    bool session_transport_encrypted = false;
    bool session_password = false;
    int active_port = 0;
    uint64_t connect_start_msec = 0;
    Reason last_reason = REASON_NONE;
    String last_message;
    /// Hôte : sel du mot de passe et clé qui en est dérivée (preuve attendue = HMAC(clé, nonce)).
    PackedByteArray password_salt;
    PackedByteArray password_key;
#pragma endregion

    // Démarrage et arrêt
    Error _attach_api();
    void _detach_api();
    Error _start_join(const String &p_address, int p_port, Transport p_transport, bool p_encrypted);
    static void _release_peer(const Ref<SceneMultiplayer> &p_api, const Ref<MultiplayerPeer> &p_peer);
    Error _ensure_identity_key();
    Error _ensure_transport_certificate();
    Ref<TLSOptions> _client_tls_options() const;
    void _set_state(State p_state);
    void _end(Reason p_reason, const String &p_message);
    void _update_discovery();

    // Signaux de SceneMultiplayer
    void _on_peer_authenticating(int p_id);
    void _on_peer_authentication_failed(int p_id);
    void _on_peer_connected(int p_id);
    void _on_peer_disconnected(int p_id);
    void _on_peer_packet(int p_id, const PackedByteArray &p_packet);
    void _on_connected_to_server();
    void _on_connection_failed();
    void _on_server_disconnected();
    void _on_auth(int p_id, const PackedByteArray &p_data);

    // Poignée de main
    void _send_auth(int p_id, const Dictionary &p_message);
    void _host_handle_hello(int p_id, const Dictionary &p_hello);
    void _host_reject(int p_id, Reason p_reason, const String &p_message);
    void _client_handle_challenge(const Dictionary &p_challenge);
    void _client_handle_accept(const Dictionary &p_accept);
    void _client_handle_roster(const Dictionary &p_roster);
    Dictionary _wire_roster() const;
    Dictionary _local_player_info() const;
    PackedByteArray _password_proof(const String &p_password, const PackedByteArray &p_salt, const PackedByteArray &p_nonce) const;

    // Joueurs
    void _add_player(int p_id, const Dictionary &p_wire_info);
    static Dictionary _sanitize_info(const Dictionary &p_info);
    void _remove_player(int p_id);

    // Paquets et chiffrement de bout en bout
    Error _send_packet(PacketType p_type, const PackedByteArray &p_payload, int p_to, bool p_reliable = true, int p_channel = 0);
    Error _send_secure(int p_to, const PackedByteArray &p_payload, bool p_reliable, int p_channel);
    void _try_establish_link(int p_id);
    void _handle_key_offer(int p_from, const PackedByteArray &p_payload);
    void _handle_secure_message(int p_from, const PackedByteArray &p_payload);
    void _deliver_message(int p_from, const PackedByteArray &p_payload, bool p_encrypted);
    PackedByteArray _key_offer_digest(const PackedByteArray &p_encrypted_key, int p_from, int p_to) const;
    bool _is_reachable(int p_id) const;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  public:
    NetworkSession();

    void _process(double p_delta) override;

#pragma region Session
    /// Héberge une partie sur p_port (port si -1). L'hôte est le joueur 1.
    Error host(int p_port = -1);
    /// Rejoint la partie de p_address (IP, nom d'hôte, ou URL ws:// / wss:// en WebSocket).
    Error join(const String &p_address, int p_port = -1);
    /// Rejoint un appareil trouvé par LanDiscovery (adresse, port et transport de sa session).
    Error join_device(const Dictionary &p_device);
    /// Quitte ou ferme la partie.
    void close();
    /// Hôte seulement : expulse un joueur avec un message.
    Error kick(int p_peer_id, const String &p_message = "");

    State get_state() const { return state; }
    bool is_online() const { return state == STATE_CONNECTED || state == STATE_HOSTING; }
    bool is_host() const { return state == STATE_HOSTING; }
    int get_local_peer_id() const;
    Reason get_last_reason() const { return last_reason; }
    String get_last_message() const { return last_message; }
#pragma endregion

#pragma region Joueurs
    /// { identifiant: infos } de tous les joueurs, joueur local compris. Infos : peer_id, name, data, os,
    /// os_version, distribution, model, engine_version, game_version, fingerprint (vide sans E2E).
    Dictionary get_players() const;
    PackedInt32Array get_player_ids() const;
    Dictionary get_player_info(int p_peer_id) const;
    bool has_player(int p_peer_id) const { return players.count(p_peer_id) > 0; }
    int get_player_count() const { return (int)players.size(); }
    /// Change les données personnalisées du joueur local et les diffuse à tous (player_updated).
    Error update_player_data(const Dictionary &p_data);
#pragma endregion

#pragma region Messages
    /// Envoie p_data (tout Variant sans objet) à p_peer_id : 0 = tout le monde, négatif = tout le monde
    /// sauf -p_peer_id. Avec le chiffrement de bout en bout, chiffré pour chaque destinataire ; les messages
    /// fiables vers un joueur dont la clé n'est pas encore connue sont mis en attente.
    Error send_message(const Variant &p_data, int p_peer_id = 0, bool p_reliable = true, int p_channel = 0);
#pragma endregion

#pragma region Sécurité
    bool is_transport_encrypted() const { return session_transport_encrypted; }
    bool is_end_to_end_active() const { return session_e2e; }
    /// Vrai quand les messages avec p_peer_id sont chiffrés de bout en bout dans les deux sens.
    bool has_secure_link(int p_peer_id) const;
    /// Empreinte de notre clé d'identité (vide si pas de chiffrement de bout en bout).
    String get_local_fingerprint() const;
    /// Empreinte de la clé de p_peer_id telle que nous l'avons reçue. Si elle diffère de ce que ce joueur
    /// lit chez lui (get_local_fingerprint), un intermédiaire a substitué sa clé.
    String get_peer_fingerprint(int p_peer_id) const;
    /// Résumé : transport_encryption, transport_verified, end_to_end, password, secure_links (identifiants).
    Dictionary get_security_summary() const;
#pragma endregion

#pragma region Getter / Setter
    void set_player_name(const String &p_name) { player_name = p_name; }
    String get_player_name() const;
    void set_player_data(const Dictionary &p_data) { player_data = p_data; }
    Dictionary get_player_data() const { return player_data; }
    void set_game_id(const String &p_id) { game_id = p_id; }
    String get_game_id() const;
    void set_game_version(const String &p_version) { game_version = p_version; }
    String get_game_version() const;
    void set_allow_version_mismatch(bool p_allow) { allow_version_mismatch = p_allow; }
    bool is_version_mismatch_allowed() const { return allow_version_mismatch; }
    void set_transport(Transport p_transport) { transport = p_transport; }
    Transport get_transport() const { return transport; }
    void set_port(int p_port) { port = p_port; }
    int get_port() const { return port; }
    void set_max_players(int p_max) { max_players = MAX(1, p_max); }
    int get_max_players() const { return max_players; }
    void set_connection_timeout(float p_timeout) { connection_timeout = MAX(0.5f, p_timeout); }
    float get_connection_timeout() const { return connection_timeout; }
    void set_password(const String &p_password) { password = p_password; }
    String get_password() const { return password; }

    void set_transport_encryption(bool p_enabled) { transport_encryption = p_enabled; }
    bool is_transport_encryption_enabled() const { return transport_encryption; }
    void set_certificate_common_name(const String &p_name) { certificate_common_name = p_name; }
    String get_certificate_common_name() const { return certificate_common_name; }
    void set_transport_key(const Ref<CryptoKey> &p_key) { transport_key = p_key; }
    Ref<CryptoKey> get_transport_key() const { return transport_key; }
    void set_transport_certificate(const Ref<X509Certificate> &p_certificate) { transport_certificate = p_certificate; }
    Ref<X509Certificate> get_transport_certificate() const { return transport_certificate; }
    void set_trusted_certificate(const Ref<X509Certificate> &p_certificate) { trusted_certificate = p_certificate; }
    Ref<X509Certificate> get_trusted_certificate() const { return trusted_certificate; }
    void set_end_to_end_encryption(bool p_enabled) { end_to_end_encryption = p_enabled; }
    bool is_end_to_end_encryption_enabled() const { return end_to_end_encryption; }
    void set_identity_key(const Ref<CryptoKey> &p_key) { identity_key = p_key; }
    Ref<CryptoKey> get_identity_key() const { return identity_key; }
    void set_discovery(Node *p_discovery);
    Node *get_discovery() const;
#pragma endregion
};
} // namespace godot

VARIANT_ENUM_CAST(NetworkSession::Transport);
VARIANT_ENUM_CAST(NetworkSession::State);
VARIANT_ENUM_CAST(NetworkSession::Reason);

#endif // NETWORK_SESSION_HPP
