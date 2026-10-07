#ifndef SECURE_CHANNEL_HPP
#define SECURE_CHANNEL_HPP

#include <godot_cpp/classes/aes_context.hpp>
#include <godot_cpp/classes/crypto.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <cstdint>

namespace godot
{
/// 🔐 Canal chiffré symétrique : chiffrement authentifié AES-256-CBC + HMAC-SHA256 (chiffrer puis
/// authentifier), avec compteur de messages contre le rejeu (fenêtre glissante de 64 messages, ce qui
/// tolère les paquets non fiables arrivés dans le désordre).
///
/// Format d'un paquet : [version 1 o][compteur 8 o][IV 16 o][texte chiffré][HMAC 32 o].
/// Un canal sert UN sens de communication : l'émetteur appelle encrypt, le destinataire (même clé, autre
/// instance) appelle decrypt. Pour un échange bidirectionnel, utiliser une clé par sens.
class SecureChannel : public RefCounted
{
    GDCLASS(SecureChannel, RefCounted)

  public:
    static constexpr int KEY_SIZE = 64; // 32 octets AES-256 + 32 octets HMAC-SHA256
    static constexpr int FORMAT_VERSION = 1;
    static constexpr int HEADER_SIZE = 1 + 8 + 16;
    static constexpr int TAG_SIZE = 32;
    static constexpr int REPLAY_WINDOW = 64;

  private:
    PackedByteArray encryption_key;
    PackedByteArray mac_key;
    Ref<Crypto> crypto;

    uint64_t send_counter = 0;
    /// Plus grand compteur reçu et masque des 64 compteurs précédents déjà reçus.
    uint64_t highest_received = 0;
    uint64_t received_mask = 0;
    bool received_any = false;

    Error last_error = OK;

    /// Vrai si le compteur p_counter n'a jamais été reçu et n'est pas trop ancien.
    bool _is_fresh(uint64_t p_counter) const;
    void _mark_received(uint64_t p_counter);

  protected:
    static void _bind_methods();

  public:
    SecureChannel();

    /// Génère une clé aléatoire (64 octets), l'installe et la renvoie.
    PackedByteArray generate_key();
    /// Installe une clé de 64 octets (32 pour AES, 32 pour HMAC) et remet les compteurs à zéro.
    Error set_key(const PackedByteArray &p_key);
    PackedByteArray get_key() const;
    bool has_key() const { return !encryption_key.is_empty(); }

    /// Chiffre et authentifie p_plaintext. Renvoie un paquet vide si aucune clé n'est installée.
    PackedByteArray encrypt(const PackedByteArray &p_plaintext);
    /// Vérifie et déchiffre un paquet produit par encrypt. En cas d'échec (paquet modifié, mauvaise clé,
    /// rejeu...), renvoie un tableau vide et get_last_error() donne la cause.
    PackedByteArray decrypt(const PackedByteArray &p_packet);
    Error get_last_error() const { return last_error; }

    /// Oublie les compteurs d'envoi et de réception (la clé est conservée).
    void reset_counters();
    int64_t get_sent_count() const { return (int64_t)send_counter; }

    /// Dérive une clé de p_length octets d'un mot de passe (PBKDF2-HMAC-SHA256).
    static PackedByteArray derive_key(const String &p_password, const PackedByteArray &p_salt, int p_iterations = 10000, int p_length = KEY_SIZE);
    /// Empreinte lisible (SHA-256 en groupes de 4 caractères hexadécimaux) : à comparer de vive voix
    /// entre deux joueurs pour vérifier qu'aucun intermédiaire n'a remplacé une clé.
    static String fingerprint(const PackedByteArray &p_data);
    /// p_size octets aléatoires sûrs pour la cryptographie.
    static PackedByteArray random_bytes(int p_size);
};
} // namespace godot

#endif // SECURE_CHANNEL_HPP
