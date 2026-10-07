#include "secure_channel.hpp"

#include <godot_cpp/classes/hashing_context.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

SecureChannel::SecureChannel()
{
    crypto.instantiate();
}

PackedByteArray SecureChannel::generate_key()
{
    const PackedByteArray key = random_bytes(KEY_SIZE);
    set_key(key);
    return key;
}

Error SecureChannel::set_key(const PackedByteArray &p_key)
{
    ERR_FAIL_COND_V_MSG(p_key.size() != KEY_SIZE, ERR_INVALID_PARAMETER, "SecureChannel : la clé doit faire 64 octets (32 AES + 32 HMAC).");
    encryption_key = p_key.slice(0, 32);
    mac_key = p_key.slice(32, KEY_SIZE);
    reset_counters();
    return OK;
}

PackedByteArray SecureChannel::get_key() const
{
    PackedByteArray key = encryption_key.duplicate();
    key.append_array(mac_key);
    return key;
}

void SecureChannel::reset_counters()
{
    send_counter = 0;
    highest_received = 0;
    received_mask = 0;
    received_any = false;
    last_error = OK;
}

PackedByteArray SecureChannel::encrypt(const PackedByteArray &p_plaintext)
{
    ERR_FAIL_COND_V_MSG(!has_key(), PackedByteArray(), "SecureChannel : aucune clé installée.");

    // Remplissage PKCS#7 : AES-CBC chiffre des blocs complets de 16 octets.
    PackedByteArray padded = p_plaintext.duplicate();
    const int padding = 16 - (int)(padded.size() % 16);
    for (int i = 0; i < padding; i++)
    {
        padded.push_back(padding);
    }

    send_counter++;
    const PackedByteArray iv = random_bytes(16);

    PackedByteArray packet;
    packet.push_back(FORMAT_VERSION);
    for (int i = 0; i < 8; i++)
    {
        packet.push_back((send_counter >> (8 * i)) & 0xFF);
    }
    packet.append_array(iv);

    Ref<AESContext> aes;
    aes.instantiate();
    aes->start(AESContext::MODE_CBC_ENCRYPT, encryption_key, iv);
    packet.append_array(aes->update(padded));
    aes->finish();

    // Le HMAC couvre l'en-tête (version, compteur, IV) et le texte chiffré.
    packet.append_array(crypto->hmac_digest(HashingContext::HASH_SHA256, mac_key, packet));
    return packet;
}

PackedByteArray SecureChannel::decrypt(const PackedByteArray &p_packet)
{
    if (!has_key())
    {
        last_error = ERR_UNCONFIGURED;
        return PackedByteArray();
    }
    const int64_t cipher_size = p_packet.size() - HEADER_SIZE - TAG_SIZE;
    if (cipher_size < 16 || cipher_size % 16 != 0 || p_packet[0] != FORMAT_VERSION)
    {
        last_error = ERR_INVALID_DATA;
        return PackedByteArray();
    }

    const PackedByteArray authenticated = p_packet.slice(0, HEADER_SIZE + cipher_size);
    const PackedByteArray tag = p_packet.slice(HEADER_SIZE + cipher_size);
    if (!crypto->constant_time_compare(crypto->hmac_digest(HashingContext::HASH_SHA256, mac_key, authenticated), tag))
    {
        last_error = ERR_UNAUTHORIZED; // paquet modifié ou mauvaise clé
        return PackedByteArray();
    }

    uint64_t counter = 0;
    for (int i = 7; i >= 0; i--)
    {
        counter = (counter << 8) | p_packet[1 + i];
    }
    if (!_is_fresh(counter))
    {
        last_error = ERR_ALREADY_EXISTS; // rejeu d'un paquet déjà reçu (ou trop ancien)
        return PackedByteArray();
    }

    Ref<AESContext> aes;
    aes.instantiate();
    aes->start(AESContext::MODE_CBC_DECRYPT, encryption_key, p_packet.slice(9, HEADER_SIZE));
    PackedByteArray plaintext = aes->update(p_packet.slice(HEADER_SIZE, HEADER_SIZE + cipher_size));
    aes->finish();

    // Le HMAC est valide : un remplissage incorrect ne peut venir que d'un émetteur défaillant.
    const int padding = plaintext[plaintext.size() - 1];
    if (padding < 1 || padding > 16)
    {
        last_error = ERR_INVALID_DATA;
        return PackedByteArray();
    }
    plaintext.resize(plaintext.size() - padding);

    _mark_received(counter);
    last_error = OK;
    return plaintext;
}

bool SecureChannel::_is_fresh(uint64_t p_counter) const
{
    if (p_counter == 0)
    {
        return false; // l'émetteur commence à 1
    }
    if (!received_any || p_counter > highest_received)
    {
        return true;
    }
    const uint64_t age = highest_received - p_counter;
    if (age >= REPLAY_WINDOW)
    {
        return false;
    }
    return age == 0 ? false : (received_mask & (1ULL << (age - 1))) == 0;
}

void SecureChannel::_mark_received(uint64_t p_counter)
{
    if (!received_any)
    {
        received_any = true;
        highest_received = p_counter;
        received_mask = 0;
        return;
    }
    if (p_counter > highest_received)
    {
        // Décale la fenêtre : l'ancien maximum devient « reçu il y a shift messages ».
        const uint64_t shift = p_counter - highest_received;
        received_mask = shift >= REPLAY_WINDOW ? 0 : (received_mask << shift) | (1ULL << (shift - 1));
        highest_received = p_counter;
    }
    else
    {
        received_mask |= 1ULL << (highest_received - p_counter - 1);
    }
}

PackedByteArray SecureChannel::derive_key(const String &p_password, const PackedByteArray &p_salt, int p_iterations, int p_length)
{
    ERR_FAIL_COND_V(p_iterations < 1 || p_length < 1, PackedByteArray());
    Ref<Crypto> crypto;
    crypto.instantiate();
    PackedByteArray password = p_password.to_utf8_buffer();
    if (password.is_empty())
    {
        // HMAC complète la clé par des zéros : une clé vide équivaut à un octet nul (Godot refuse une clé vide).
        password.push_back(0);
    }

    // PBKDF2 (RFC 8018) : bloc i = U1 xor U2 xor ... ; U1 = HMAC(mot de passe, sel || i), Uj = HMAC(mot de passe, Uj-1).
    PackedByteArray key;
    for (uint32_t block = 1; key.size() < p_length; block++)
    {
        PackedByteArray salt_block = p_salt.duplicate();
        for (int i = 3; i >= 0; i--)
        {
            salt_block.push_back((block >> (8 * i)) & 0xFF); // gros-boutiste, comme la norme
        }
        PackedByteArray u = crypto->hmac_digest(HashingContext::HASH_SHA256, password, salt_block);
        PackedByteArray t = u.duplicate();
        for (int j = 1; j < p_iterations; j++)
        {
            u = crypto->hmac_digest(HashingContext::HASH_SHA256, password, u);
            for (int64_t k = 0; k < t.size(); k++)
            {
                t.set(k, t[k] ^ u[k]);
            }
        }
        key.append_array(t);
    }
    return key.slice(0, p_length);
}

String SecureChannel::fingerprint(const PackedByteArray &p_data)
{
    Ref<HashingContext> hashing;
    hashing.instantiate();
    hashing->start(HashingContext::HASH_SHA256);
    hashing->update(p_data);
    const String hex = hashing->finish().hex_encode().to_upper();

    // 16 premiers octets du SHA-256, en 8 groupes de 4 : assez court pour être lu à voix haute.
    PackedStringArray groups;
    for (int i = 0; i < 32; i += 4)
    {
        groups.push_back(hex.substr(i, 4));
    }
    return String(" ").join(groups);
}

PackedByteArray SecureChannel::random_bytes(int p_size)
{
    Ref<Crypto> crypto;
    crypto.instantiate();
    return crypto->generate_random_bytes(p_size);
}

void SecureChannel::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("generate_key"), &SecureChannel::generate_key);
    ClassDB::bind_method(D_METHOD("set_key", "key"), &SecureChannel::set_key);
    ClassDB::bind_method(D_METHOD("get_key"), &SecureChannel::get_key);
    ClassDB::bind_method(D_METHOD("has_key"), &SecureChannel::has_key);
    ClassDB::bind_method(D_METHOD("encrypt", "plaintext"), &SecureChannel::encrypt);
    ClassDB::bind_method(D_METHOD("decrypt", "packet"), &SecureChannel::decrypt);
    ClassDB::bind_method(D_METHOD("get_last_error"), &SecureChannel::get_last_error);
    ClassDB::bind_method(D_METHOD("reset_counters"), &SecureChannel::reset_counters);
    ClassDB::bind_method(D_METHOD("get_sent_count"), &SecureChannel::get_sent_count);

    ClassDB::bind_static_method("SecureChannel", D_METHOD("derive_key", "password", "salt", "iterations", "length"), &SecureChannel::derive_key, DEFVAL(10000), DEFVAL(KEY_SIZE));
    ClassDB::bind_static_method("SecureChannel", D_METHOD("fingerprint", "data"), &SecureChannel::fingerprint);
    ClassDB::bind_static_method("SecureChannel", D_METHOD("random_bytes", "size"), &SecureChannel::random_bytes);

    BIND_CONSTANT(KEY_SIZE);
}
