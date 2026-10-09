#ifndef SAVE_FILE_ACCESS_HPP
#define SAVE_FILE_ACCESS_HPP

#include "save_config.hpp"
#include "save_data.hpp"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

namespace godot
{
/// 📁 Lit, écrit et supprime les fichiers de sauvegarde sur disque, avec chiffrement AES-256 CBC optionnel.
/// Les données passent par SaveSerializer (JSON). Le fichier se nomme `<save_directory><system_id><extension>`,
/// l'extension dépendant du chiffrement.
class SaveFileAccess : public Object
{
    GDCLASS(SaveFileAccess, Object)

  private:
    // 🔑 Chiffrement
    // IV dérivé de la clé et écrit en tête du fichier (compatibilité avec l'ancienne version C#) : il expose
    // les 16 premiers octets de la clé. Pas de HMAC : une altération n'est pas détectée.
    static PackedByteArray pad_key(const String &p_key);
    static PackedByteArray generate_iv(const String &p_key);
    static PackedByteArray encrypt(const PackedByteArray &p_data, const String &p_key);
    static PackedByteArray decrypt(const PackedByteArray &p_data, const String &p_key);

    // 🛠️ Utilitaires
    static String get_path(const String &p_system_id, bool p_encrypted, const Ref<SaveConfig> &p_config);
    static void ensure_directory_exists(const Ref<SaveConfig> &p_config);

  protected:
    static void _bind_methods();

  public:
    /// Écrit le contenu du SaveData dans son fichier (dossier créé si besoin), chiffré si is_encrypted.
    /// Renvoie OK, ERR_INVALID_PARAMETER (argument nul) ou l'erreur d'ouverture du fichier.
    static Error write(const Ref<SaveData> &p_save_data, const Ref<SaveConfig> &p_config);
    /// Retourne un SaveData nul si le fichier n'existe pas ou ne peut pas être lu.
    static Ref<SaveData> read(const String &p_system_id, const Ref<SaveConfig> &p_config, bool p_encrypted = false, const String &p_encryption_key = String());
    /// Supprime le fichier de sauvegarde de p_system_id s'il existe (extension selon p_encrypted).
    static void delete_file(const String &p_system_id, const Ref<SaveConfig> &p_config, bool p_encrypted = false);
};
} // namespace godot

#endif // SAVE_FILE_ACCESS_HPP