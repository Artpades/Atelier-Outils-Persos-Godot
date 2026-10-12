#include "server_access_policy.hpp"

#include "../🔐Chiffrement/secure_channel.hpp"

#include <godot_cpp/classes/crypto.hpp>
#include <godot_cpp/classes/hashing_context.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace
{
/// Alphabet des codes d'invitation : sans 0/O ni 1/I, pour être dicté sans ambiguïté. 32 caractères :
/// un octet aléatoire modulo 32 reste uniforme.
const char *const INVITE_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
constexpr int INVITE_LENGTH = 12;

int64_t now_unix()
{
    return (int64_t)Time::get_singleton()->get_unix_time_from_system();
}

PackedByteArray sha256(const PackedByteArray &p_data)
{
    Ref<HashingContext> hashing;
    hashing.instantiate();
    hashing->start(HashingContext::HASH_SHA256);
    hashing->update(p_data);
    return hashing->finish();
}

String account_id(const String &p_username)
{
    return p_username.strip_edges().to_lower();
}
} // namespace

ServerAccessPolicy::ServerAccessPolicy()
{
    account_seed = SecureChannel::random_bytes(16);
}

#pragma region Comptes

PackedByteArray ServerAccessPolicy::account_key(const PackedByteArray &p_seed, const String &p_username, const String &p_password)
{
    PackedByteArray salt = p_seed.duplicate();
    salt.append_array(account_id(p_username).to_utf8_buffer());
    return SecureChannel::derive_key(p_password, sha256(salt), ACCOUNT_ITERATIONS, 32);
}

Error ServerAccessPolicy::add_account(const String &p_username, const String &p_password, Role p_role)
{
    const String id = account_id(p_username);
    ERR_FAIL_COND_V_MSG(id.is_empty() || p_password.is_empty(), ERR_INVALID_PARAMETER, U"ServerAccessPolicy : identifiant et mot de passe obligatoires.");
    Dictionary account;
    account["name"] = p_username.strip_edges();
    account["key"] = account_key(account_seed, id, p_password);
    account["role"] = p_role;
    accounts[id] = account;
    emit_changed();
    return OK;
}

void ServerAccessPolicy::remove_account(const String &p_username)
{
    if (accounts.erase(account_id(p_username)))
    {
        emit_changed();
    }
}

bool ServerAccessPolicy::has_account(const String &p_username) const
{
    return accounts.has(account_id(p_username));
}

Error ServerAccessPolicy::set_account_password(const String &p_username, const String &p_password)
{
    ERR_FAIL_COND_V(!has_account(p_username), ERR_DOES_NOT_EXIST);
    return add_account(p_username, p_password, get_account_role(p_username));
}

void ServerAccessPolicy::set_account_role(const String &p_username, Role p_role)
{
    const String id = account_id(p_username);
    if (accounts.has(id))
    {
        Dictionary account = accounts[id];
        account["role"] = p_role;
        emit_changed();
    }
}

ServerAccessPolicy::Role ServerAccessPolicy::get_account_role(const String &p_username) const
{
    const Dictionary account = accounts.get(account_id(p_username), Dictionary());
    return (Role)(int)account.get("role", default_role);
}

PackedStringArray ServerAccessPolicy::get_account_names() const
{
    PackedStringArray names;
    const Array ids = accounts.keys();
    for (int i = 0; i < ids.size(); i++)
    {
        names.push_back(((Dictionary)accounts[ids[i]]).get("name", ids[i]));
    }
    return names;
}

bool ServerAccessPolicy::verify_account(const String &p_username, const PackedByteArray &p_nonce, const PackedByteArray &p_proof) const
{
    const Dictionary account = accounts.get(account_id(p_username), Dictionary());
    const PackedByteArray key = account.get("key", PackedByteArray());
    if (key.is_empty() || p_proof.is_empty())
    {
        return false;
    }
    Ref<Crypto> crypto;
    crypto.instantiate();
    return crypto->constant_time_compare(crypto->hmac_digest(HashingContext::HASH_SHA256, key, p_nonce), p_proof);
}

#pragma endregion

#pragma region Invitations

String ServerAccessPolicy::normalize_invite(const String &p_code)
{
    return p_code.to_upper().replace("-", "").replace(" ", "").strip_edges();
}

PackedByteArray ServerAccessPolicy::invite_proof(const String &p_code, const PackedByteArray &p_nonce)
{
    Ref<Crypto> crypto;
    crypto.instantiate();
    return crypto->hmac_digest(HashingContext::HASH_SHA256, sha256(normalize_invite(p_code).to_utf8_buffer()), p_nonce);
}

String ServerAccessPolicy::create_invite(int p_uses, float p_duration_hours, Role p_role)
{
    const PackedByteArray random = SecureChannel::random_bytes(INVITE_LENGTH);
    String code;
    for (int i = 0; i < INVITE_LENGTH; i++)
    {
        code += String::chr(INVITE_ALPHABET[random[i] % 32]);
    }
    Dictionary invite;
    invite["uses_left"] = p_uses <= 0 ? -1 : p_uses;
    invite["expires_unix"] = p_duration_hours > 0.0f ? now_unix() + (int64_t)(p_duration_hours * 3600.0f) : (int64_t)0;
    invite["role"] = p_role;
    invite["created_unix"] = now_unix();
    invites[code] = invite;
    emit_changed();
    return code.substr(0, 4) + String("-") + code.substr(4, 4) + String("-") + code.substr(8, 4);
}

void ServerAccessPolicy::revoke_invite(const String &p_code)
{
    if (invites.erase(normalize_invite(p_code)))
    {
        emit_changed();
    }
}

bool ServerAccessPolicy::has_invite(const String &p_code) const
{
    const Dictionary invite = invites.get(normalize_invite(p_code), Dictionary());
    const int64_t expires = invite.get("expires_unix", 0);
    return !invite.is_empty() && (expires == 0 || expires > now_unix());
}

Array ServerAccessPolicy::get_invites()
{
    Array list;
    const Array codes = invites.keys();
    bool removed = false;
    for (int i = 0; i < codes.size(); i++)
    {
        if (!has_invite(codes[i]))
        {
            invites.erase(codes[i]);
            removed = true;
            continue;
        }
        Dictionary entry = ((Dictionary)invites[codes[i]]).duplicate();
        const String code = codes[i];
        entry["code"] = code.substr(0, 4) + String("-") + code.substr(4, 4) + String("-") + code.substr(8, 4);
        list.push_back(entry);
    }
    if (removed)
    {
        emit_changed();
    }
    return list;
}

String ServerAccessPolicy::find_invite(const PackedByteArray &p_nonce, const PackedByteArray &p_proof) const
{
    if (p_proof.is_empty())
    {
        return "";
    }
    Ref<Crypto> crypto;
    crypto.instantiate();
    const Array codes = invites.keys();
    for (int i = 0; i < codes.size(); i++)
    {
        if (has_invite(codes[i]) && crypto->constant_time_compare(invite_proof(codes[i], p_nonce), p_proof))
        {
            return codes[i];
        }
    }
    return "";
}

ServerAccessPolicy::Role ServerAccessPolicy::consume_invite(const String &p_code)
{
    const String code = normalize_invite(p_code);
    Dictionary invite = invites.get(code, Dictionary());
    const Role role = (Role)(int)invite.get("role", default_role);
    const int uses_left = invite.get("uses_left", -1);
    if (uses_left == 1)
    {
        invites.erase(code);
    }
    else if (uses_left > 1)
    {
        invite["uses_left"] = uses_left - 1;
    }
    emit_changed();
    return role;
}

#pragma endregion

#pragma region Appareils

void ServerAccessPolicy::remember_device(const String &p_fingerprint, const String &p_name, Role p_role)
{
    ERR_FAIL_COND(p_fingerprint.is_empty());
    Dictionary device;
    device["name"] = p_name;
    device["role"] = p_role;
    device["since_unix"] = now_unix();
    known_devices[p_fingerprint] = device;
    emit_changed();
}

void ServerAccessPolicy::forget_device(const String &p_fingerprint)
{
    if (known_devices.erase(p_fingerprint))
    {
        emit_changed();
    }
}

bool ServerAccessPolicy::is_device_known(const String &p_fingerprint) const
{
    return !p_fingerprint.is_empty() && known_devices.has(p_fingerprint);
}

ServerAccessPolicy::Role ServerAccessPolicy::get_device_role(const String &p_fingerprint) const
{
    const Dictionary device = known_devices.get(p_fingerprint, Dictionary());
    return (Role)(int)device.get("role", default_role);
}

Array ServerAccessPolicy::get_known_devices() const
{
    Array list;
    const Array keys = known_devices.keys();
    for (int i = 0; i < keys.size(); i++)
    {
        Dictionary entry = ((Dictionary)known_devices[keys[i]]).duplicate();
        entry["fingerprint"] = keys[i];
        list.push_back(entry);
    }
    return list;
}

void ServerAccessPolicy::ban_device(const String &p_fingerprint)
{
    if (!p_fingerprint.is_empty() && !banned_devices.has(p_fingerprint))
    {
        banned_devices.push_back(p_fingerprint);
        known_devices.erase(p_fingerprint);
        emit_changed();
    }
}

void ServerAccessPolicy::unban_device(const String &p_fingerprint)
{
    const int64_t index = banned_devices.find(p_fingerprint);
    if (index >= 0)
    {
        banned_devices.remove_at(index);
        emit_changed();
    }
}

bool ServerAccessPolicy::is_device_banned(const String &p_fingerprint) const
{
    return !p_fingerprint.is_empty() && banned_devices.has(p_fingerprint);
}

void ServerAccessPolicy::ban_address(const String &p_address)
{
    if (!p_address.is_empty() && !banned_addresses.has(p_address))
    {
        banned_addresses.push_back(p_address);
        emit_changed();
    }
}

void ServerAccessPolicy::unban_address(const String &p_address)
{
    const int64_t index = banned_addresses.find(p_address);
    if (index >= 0)
    {
        banned_addresses.remove_at(index);
        emit_changed();
    }
}

bool ServerAccessPolicy::is_address_banned(const String &p_address) const
{
    return !p_address.is_empty() && banned_addresses.has(p_address);
}

#pragma endregion

#pragma region Getter / Setter

void ServerAccessPolicy::set_access_mode(AccessMode p_mode)
{
    access_mode = p_mode;
    emit_changed();
}

void ServerAccessPolicy::set_require_approval(bool p_require)
{
    require_approval = p_require;
    emit_changed();
}

void ServerAccessPolicy::set_default_role(Role p_role)
{
    default_role = p_role;
    emit_changed();
}

void ServerAccessPolicy::set_max_failed_attempts(int p_max)
{
    max_failed_attempts = MAX(0, p_max);
    emit_changed();
}

void ServerAccessPolicy::set_block_duration(float p_seconds)
{
    block_duration = MAX(0.0f, p_seconds);
    emit_changed();
}

void ServerAccessPolicy::set_approval_timeout(float p_seconds)
{
    approval_timeout = MAX(5.0f, p_seconds);
    emit_changed();
}

#pragma endregion

void ServerAccessPolicy::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("add_account", "username", "password", "role"), &ServerAccessPolicy::add_account, DEFVAL(ROLE_PLAYER));
    ClassDB::bind_method(D_METHOD("remove_account", "username"), &ServerAccessPolicy::remove_account);
    ClassDB::bind_method(D_METHOD("has_account", "username"), &ServerAccessPolicy::has_account);
    ClassDB::bind_method(D_METHOD("set_account_password", "username", "password"), &ServerAccessPolicy::set_account_password);
    ClassDB::bind_method(D_METHOD("set_account_role", "username", "role"), &ServerAccessPolicy::set_account_role);
    ClassDB::bind_method(D_METHOD("get_account_role", "username"), &ServerAccessPolicy::get_account_role);
    ClassDB::bind_method(D_METHOD("get_account_names"), &ServerAccessPolicy::get_account_names);

    ClassDB::bind_method(D_METHOD("create_invite", "uses", "duration_hours", "role"), &ServerAccessPolicy::create_invite, DEFVAL(1), DEFVAL(24.0f), DEFVAL(ROLE_PLAYER));
    ClassDB::bind_method(D_METHOD("revoke_invite", "code"), &ServerAccessPolicy::revoke_invite);
    ClassDB::bind_method(D_METHOD("has_invite", "code"), &ServerAccessPolicy::has_invite);
    ClassDB::bind_method(D_METHOD("get_invites"), &ServerAccessPolicy::get_invites);
    ClassDB::bind_static_method("ServerAccessPolicy", D_METHOD("normalize_invite", "code"), &ServerAccessPolicy::normalize_invite);

    ClassDB::bind_method(D_METHOD("remember_device", "fingerprint", "name", "role"), &ServerAccessPolicy::remember_device);
    ClassDB::bind_method(D_METHOD("forget_device", "fingerprint"), &ServerAccessPolicy::forget_device);
    ClassDB::bind_method(D_METHOD("is_device_known", "fingerprint"), &ServerAccessPolicy::is_device_known);
    ClassDB::bind_method(D_METHOD("get_device_role", "fingerprint"), &ServerAccessPolicy::get_device_role);
    ClassDB::bind_method(D_METHOD("get_known_devices"), &ServerAccessPolicy::get_known_devices);
    ClassDB::bind_method(D_METHOD("ban_device", "fingerprint"), &ServerAccessPolicy::ban_device);
    ClassDB::bind_method(D_METHOD("unban_device", "fingerprint"), &ServerAccessPolicy::unban_device);
    ClassDB::bind_method(D_METHOD("is_device_banned", "fingerprint"), &ServerAccessPolicy::is_device_banned);
    ClassDB::bind_method(D_METHOD("ban_address", "address"), &ServerAccessPolicy::ban_address);
    ClassDB::bind_method(D_METHOD("unban_address", "address"), &ServerAccessPolicy::unban_address);
    ClassDB::bind_method(D_METHOD("is_address_banned", "address"), &ServerAccessPolicy::is_address_banned);

    ClassDB::bind_method(D_METHOD("set_access_mode", "mode"), &ServerAccessPolicy::set_access_mode);
    ClassDB::bind_method(D_METHOD("get_access_mode"), &ServerAccessPolicy::get_access_mode);
    ClassDB::bind_method(D_METHOD("set_require_approval", "require"), &ServerAccessPolicy::set_require_approval);
    ClassDB::bind_method(D_METHOD("is_approval_required"), &ServerAccessPolicy::is_approval_required);
    ClassDB::bind_method(D_METHOD("set_default_role", "role"), &ServerAccessPolicy::set_default_role);
    ClassDB::bind_method(D_METHOD("get_default_role"), &ServerAccessPolicy::get_default_role);
    ClassDB::bind_method(D_METHOD("set_max_failed_attempts", "max"), &ServerAccessPolicy::set_max_failed_attempts);
    ClassDB::bind_method(D_METHOD("get_max_failed_attempts"), &ServerAccessPolicy::get_max_failed_attempts);
    ClassDB::bind_method(D_METHOD("set_block_duration", "seconds"), &ServerAccessPolicy::set_block_duration);
    ClassDB::bind_method(D_METHOD("get_block_duration"), &ServerAccessPolicy::get_block_duration);
    ClassDB::bind_method(D_METHOD("set_approval_timeout", "seconds"), &ServerAccessPolicy::set_approval_timeout);
    ClassDB::bind_method(D_METHOD("get_approval_timeout"), &ServerAccessPolicy::get_approval_timeout);

    ClassDB::bind_method(D_METHOD("set_account_seed", "seed"), &ServerAccessPolicy::set_account_seed);
    ClassDB::bind_method(D_METHOD("get_account_seed"), &ServerAccessPolicy::get_account_seed);
    ClassDB::bind_method(D_METHOD("set_accounts", "accounts"), &ServerAccessPolicy::set_accounts);
    ClassDB::bind_method(D_METHOD("get_accounts"), &ServerAccessPolicy::get_accounts);
    ClassDB::bind_method(D_METHOD("set_invites_data", "invites"), &ServerAccessPolicy::set_invites_data);
    ClassDB::bind_method(D_METHOD("get_invites_data"), &ServerAccessPolicy::get_invites_data);
    ClassDB::bind_method(D_METHOD("set_known_devices_data", "devices"), &ServerAccessPolicy::set_known_devices_data);
    ClassDB::bind_method(D_METHOD("get_known_devices_data"), &ServerAccessPolicy::get_known_devices_data);
    ClassDB::bind_method(D_METHOD("set_banned_devices", "devices"), &ServerAccessPolicy::set_banned_devices);
    ClassDB::bind_method(D_METHOD("get_banned_devices"), &ServerAccessPolicy::get_banned_devices);
    ClassDB::bind_method(D_METHOD("set_banned_addresses", "addresses"), &ServerAccessPolicy::set_banned_addresses);
    ClassDB::bind_method(D_METHOD("get_banned_addresses"), &ServerAccessPolicy::get_banned_addresses);

    ADD_GROUP(U"Accès", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "access_mode", PROPERTY_HINT_ENUM, "Ouvert,Comptes,Invitations"), "set_access_mode", "get_access_mode");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "require_approval"), "set_require_approval", "is_approval_required");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "default_role", PROPERTY_HINT_ENUM, "Spectateur,Joueur,Moderateur,Administrateur"), "set_default_role", "get_default_role");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "approval_timeout", PROPERTY_HINT_RANGE, "5,600,1,suffix:s"), "set_approval_timeout", "get_approval_timeout");

    ADD_GROUP(U"Protection", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_failed_attempts", PROPERTY_HINT_RANGE, "0,50"), "set_max_failed_attempts", "get_max_failed_attempts");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "block_duration", PROPERTY_HINT_RANGE, "0,3600,1,suffix:s"), "set_block_duration", "get_block_duration");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "banned_devices"), "set_banned_devices", "get_banned_devices");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "banned_addresses"), "set_banned_addresses", "get_banned_addresses");

    // Données gérées par les méthodes : sauvegardées, mais pas affichées dans l'inspecteur.
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "account_seed", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_account_seed", "get_account_seed");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "accounts", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_accounts", "get_accounts");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "invites_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_invites_data", "get_invites_data");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "known_devices_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_known_devices_data", "get_known_devices_data");

    BIND_ENUM_CONSTANT(ACCESS_OPEN);
    BIND_ENUM_CONSTANT(ACCESS_ACCOUNTS);
    BIND_ENUM_CONSTANT(ACCESS_INVITATIONS);
    BIND_ENUM_CONSTANT(ROLE_SPECTATOR);
    BIND_ENUM_CONSTANT(ROLE_PLAYER);
    BIND_ENUM_CONSTANT(ROLE_MODERATOR);
    BIND_ENUM_CONSTANT(ROLE_ADMIN);
}
