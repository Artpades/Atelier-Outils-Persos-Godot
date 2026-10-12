#ifndef SERVER_ACCESS_POLICY_HPP
#define SERVER_ACCESS_POLICY_HPP

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot
{
/// 🚪 Règles d'accès d'un serveur : qui peut se connecter et avec quel rôle.
///
/// Trois modes : ouvert, comptes (identifiant + mot de passe) ou invitations (codes à usages et durée
/// limités). Options cumulables : validation manuelle des nouveaux appareils, appareils et adresses IP
/// bannis, blocage temporaire après trop d'échecs. Chaque appareil est reconnu par l'empreinte de sa clé
/// d'identité (impossible à usurper sans la clé privée), pas par une information qu'il déclare.
///
/// Ressource sauvegardable (.tres) : les mots de passe n'y sont jamais stockés en clair (clé dérivée
/// PBKDF2, utilisée comme preuve HMAC sur un défi aléatoire : le mot de passe ne circule pas).
class ServerAccessPolicy : public Resource
{
    GDCLASS(ServerAccessPolicy, Resource)

  public:
    enum AccessMode
    {
        ACCESS_OPEN,        ///< Tout appareil (sous réserve du mot de passe du serveur, des bans et de la validation).
        ACCESS_ACCOUNTS,    ///< Identifiant + mot de passe d'un compte créé sur le serveur.
        ACCESS_INVITATIONS, ///< Code d'invitation ; l'appareil invité est ensuite reconnu sans nouveau code.
    };

    enum Role
    {
        ROLE_SPECTATOR,
        ROLE_PLAYER,
        ROLE_MODERATOR,
        ROLE_ADMIN,
    };

    /// Itérations PBKDF2 des mots de passe de compte (côté client, à chaque connexion).
    static constexpr int ACCOUNT_ITERATIONS = 10000;

  private:
    AccessMode access_mode = ACCESS_OPEN;
    bool require_approval = false;
    Role default_role = ROLE_PLAYER;
    int max_failed_attempts = 5;
    float block_duration = 60.0f;
    float approval_timeout = 120.0f;

    /// Graine aléatoire propre à ce serveur, mêlée au sel des comptes (un même mot de passe donne une
    /// clé différente d'un serveur à l'autre).
    PackedByteArray account_seed;
    /// identifiant (minuscules) -> { name, key (clé dérivée), role }
    Dictionary accounts;
    /// code normalisé -> { uses_left (-1 = illimité), expires_unix (0 = jamais), role, created_unix }
    Dictionary invites;
    /// empreinte d'appareil -> { name, role, since_unix }
    Dictionary known_devices;
    PackedStringArray banned_devices;
    PackedStringArray banned_addresses;

  protected:
    static void _bind_methods();

  public:
    ServerAccessPolicy();

#pragma region Comptes
    /// Crée ou remplace un compte. Renvoie ERR_INVALID_PARAMETER si l'identifiant ou le mot de passe est vide.
    Error add_account(const String &p_username, const String &p_password, Role p_role = ROLE_PLAYER);
    void remove_account(const String &p_username);
    bool has_account(const String &p_username) const;
    Error set_account_password(const String &p_username, const String &p_password);
    void set_account_role(const String &p_username, Role p_role);
    Role get_account_role(const String &p_username) const;
    PackedStringArray get_account_names() const;
    /// Vrai si p_proof = HMAC(clé du compte, p_nonce).
    bool verify_account(const String &p_username, const PackedByteArray &p_nonce, const PackedByteArray &p_proof) const;
    /// Clé qu'un client dérive de son mot de passe (même calcul des deux côtés).
    static PackedByteArray account_key(const PackedByteArray &p_seed, const String &p_username, const String &p_password);
    PackedByteArray get_account_seed() const { return account_seed; }
#pragma endregion

#pragma region Invitations
    /// Crée un code d'invitation (ex. « K7QF-9XM2-PL4D ») utilisable p_uses fois (-1 : illimité) pendant
    /// p_duration_hours heures (0 : sans limite), qui donne le rôle p_role.
    String create_invite(int p_uses = 1, float p_duration_hours = 24.0f, Role p_role = ROLE_PLAYER);
    void revoke_invite(const String &p_code);
    bool has_invite(const String &p_code) const;
    /// Invitations encore valides : Array de { code, uses_left, expires_unix, role }. Les expirées sont retirées.
    Array get_invites();
    /// Code de l'invitation valide dont p_proof = HMAC(SHA-256(code), p_nonce), ou "" (sans la consommer).
    String find_invite(const PackedByteArray &p_nonce, const PackedByteArray &p_proof) const;
    /// Décompte un usage ; l'invitation disparaît à son dernier usage. Renvoie son rôle.
    Role consume_invite(const String &p_code);
    /// Code en majuscules, sans tirets ni espaces.
    static String normalize_invite(const String &p_code);
    /// Preuve de possession d'un code d'invitation (calcul identique côté client).
    static PackedByteArray invite_proof(const String &p_code, const PackedByteArray &p_nonce);
#pragma endregion

#pragma region Appareils
    void remember_device(const String &p_fingerprint, const String &p_name, Role p_role);
    void forget_device(const String &p_fingerprint);
    bool is_device_known(const String &p_fingerprint) const;
    Role get_device_role(const String &p_fingerprint) const;
    /// Array de { fingerprint, name, role, since_unix }.
    Array get_known_devices() const;

    void ban_device(const String &p_fingerprint);
    void unban_device(const String &p_fingerprint);
    bool is_device_banned(const String &p_fingerprint) const;
    void ban_address(const String &p_address);
    void unban_address(const String &p_address);
    bool is_address_banned(const String &p_address) const;
#pragma endregion

#pragma region Getter / Setter
    void set_access_mode(AccessMode p_mode);
    AccessMode get_access_mode() const { return access_mode; }
    void set_require_approval(bool p_require);
    bool is_approval_required() const { return require_approval; }
    void set_default_role(Role p_role);
    Role get_default_role() const { return default_role; }
    void set_max_failed_attempts(int p_max);
    int get_max_failed_attempts() const { return max_failed_attempts; }
    void set_block_duration(float p_seconds);
    float get_block_duration() const { return block_duration; }
    void set_approval_timeout(float p_seconds);
    float get_approval_timeout() const { return approval_timeout; }

    // Stockage (.tres) : modifiés par les méthodes ci-dessus, pas à la main.
    void set_account_seed(const PackedByteArray &p_seed) { account_seed = p_seed; }
    void set_accounts(const Dictionary &p_accounts) { accounts = p_accounts; }
    Dictionary get_accounts() const { return accounts; }
    void set_invites_data(const Dictionary &p_invites) { invites = p_invites; }
    Dictionary get_invites_data() const { return invites; }
    void set_known_devices_data(const Dictionary &p_devices) { known_devices = p_devices; }
    Dictionary get_known_devices_data() const { return known_devices; }
    void set_banned_devices(const PackedStringArray &p_devices) { banned_devices = p_devices; }
    PackedStringArray get_banned_devices() const { return banned_devices; }
    void set_banned_addresses(const PackedStringArray &p_addresses) { banned_addresses = p_addresses; }
    PackedStringArray get_banned_addresses() const { return banned_addresses; }
#pragma endregion
};
} // namespace godot

VARIANT_ENUM_CAST(ServerAccessPolicy::AccessMode);
VARIANT_ENUM_CAST(ServerAccessPolicy::Role);

#endif // SERVER_ACCESS_POLICY_HPP
