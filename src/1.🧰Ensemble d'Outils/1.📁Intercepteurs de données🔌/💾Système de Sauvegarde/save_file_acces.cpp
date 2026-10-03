#include "save_file_access.hpp"
#include "save_serializer.hpp"

#include <godot_cpp/classes/aes_context.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

// 📝 Écriture

Error SaveFileAccess::write(const Ref<SaveData> &p_save_data, const Ref<SaveConfig> &p_config)
{
    ERR_FAIL_COND_V_MSG(p_save_data.is_null(), ERR_INVALID_PARAMETER, "SaveFileAccess : SaveData nul.");
    ERR_FAIL_COND_V_MSG(p_config.is_null(), ERR_INVALID_PARAMETER, "SaveFileAccess : SaveConfig nul.");

    ensure_directory_exists(p_config);
    String path = get_path(p_save_data->get_system_id(), p_save_data->get_is_encrypted(), p_config);
    PackedByteArray data = SaveSerializer::serialize(p_save_data).to_utf8_buffer();

    if (p_save_data->get_is_encrypted())
    {
        data = encrypt(data, p_save_data->get_encryption_key());
    }

    Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
    if (file.is_null())
    {
        return FileAccess::get_open_error();
    }
    file->store_buffer(data);
    return OK;
}

// 📖 Lecture

Ref<SaveData> SaveFileAccess::read(const String &p_system_id, const Ref<SaveConfig> &p_config, bool p_encrypted, const String &p_encryption_key)
{
    ERR_FAIL_COND_V_MSG(p_config.is_null(), Ref<SaveData>(), "SaveFileAccess : SaveConfig nul.");

    String path = get_path(p_system_id, p_encrypted, p_config);
    if (!FileAccess::file_exists(path))
    {
        return Ref<SaveData>();
    }

    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file.is_null())
    {
        return Ref<SaveData>();
    }

    PackedByteArray data = file->get_buffer(file->get_length());

    if (p_encrypted)
    {
        data = decrypt(data, p_encryption_key);
        if (data.is_empty())
        {
            return Ref<SaveData>();
        }
    }

    return SaveSerializer::deserialize(data.get_string_from_utf8());
}

// 🗑️ Suppression

void SaveFileAccess::delete_file(const String &p_system_id, const Ref<SaveConfig> &p_config, bool p_encrypted)
{
    ERR_FAIL_COND_MSG(p_config.is_null(), "SaveFileAccess : SaveConfig nul.");

    String path = get_path(p_system_id, p_encrypted, p_config);
    if (FileAccess::file_exists(path))
    {
        DirAccess::remove_absolute(path);
    }
}

// 🔑 Chiffrement

PackedByteArray SaveFileAccess::pad_key(const String &p_key)
{
    // Normalise la clé sur 32 octets (AES-256) : tronquée, ou complétée par des zéros.
    PackedByteArray key_bytes;
    key_bytes.resize(32);
    key_bytes.fill(0);

    PackedByteArray raw = p_key.to_utf8_buffer();
    for (int64_t i = 0; i < raw.size() && i < 32; i++)
    {
        key_bytes.set(i, raw[i]);
    }
    return key_bytes;
}

PackedByteArray SaveFileAccess::generate_iv(const String &p_key)
{
    // IV dérivé de la clé : déterministe, identique à la version C# pour relire les anciennes sauvegardes.
    PackedByteArray iv;
    iv.resize(16);

    PackedByteArray raw = p_key.to_utf8_buffer();
    for (int64_t i = 0; i < 16; i++)
    {
        iv.set(i, i < raw.size() ? raw[i] : static_cast<uint8_t>(i));
    }
    return iv;
}

PackedByteArray SaveFileAccess::encrypt(const PackedByteArray &p_data, const String &p_key)
{
    PackedByteArray iv = generate_iv(p_key);

    // Bourrage par des zéros jusqu'au multiple de 16 suivant (toujours au moins 1 octet, comme en C#).
    int64_t padding = 16 - (p_data.size() % 16);
    PackedByteArray padded = p_data.duplicate();
    padded.resize(p_data.size() + padding);
    for (int64_t i = p_data.size(); i < padded.size(); i++)
    {
        padded.set(i, 0);
    }

    Ref<AESContext> aes;
    aes.instantiate();
    aes->start(AESContext::MODE_CBC_ENCRYPT, pad_key(p_key), iv);
    PackedByteArray encrypted = aes->update(padded);
    aes->finish();

    // Préfixe l'IV au résultat pour pouvoir déchiffrer sans le stocker séparément.
    PackedByteArray result = iv;
    result.append_array(encrypted);
    return result;
}

PackedByteArray SaveFileAccess::decrypt(const PackedByteArray &p_data, const String &p_key)
{
    ERR_FAIL_COND_V_MSG(p_data.size() < 32 || (p_data.size() - 16) % 16 != 0, PackedByteArray(), "SaveFileAccess : fichier chiffré corrompu (taille invalide).");

    // Extrait l'IV des 16 premiers octets.
    PackedByteArray iv = p_data.slice(0, 16);
    PackedByteArray encrypted = p_data.slice(16);

    Ref<AESContext> aes;
    aes.instantiate();
    aes->start(AESContext::MODE_CBC_DECRYPT, pad_key(p_key), iv);
    PackedByteArray decrypted = aes->update(encrypted);
    aes->finish();

    // Retire le bourrage de zéros (le JSON ne contient jamais d'octet nul).
    int64_t end = decrypted.size();
    while (end > 0 && decrypted[end - 1] == 0)
    {
        end--;
    }
    decrypted.resize(end);
    return decrypted;
}

// 🛠️ Utilitaires

String SaveFileAccess::get_path(const String &p_system_id, bool p_encrypted, const Ref<SaveConfig> &p_config)
{
    return p_config->get_save_directory() + p_system_id + (p_encrypted ? p_config->get_encrypted_extension() : p_config->get_extension());
}

void SaveFileAccess::ensure_directory_exists(const Ref<SaveConfig> &p_config)
{
    String directory = p_config->get_save_directory();
    if (!DirAccess::dir_exists_absolute(directory))
    {
        DirAccess::make_dir_recursive_absolute(directory);
    }
}

void SaveFileAccess::_bind_methods()
{
    ClassDB::bind_static_method("SaveFileAccess", D_METHOD("write", "save_data", "config"), &SaveFileAccess::write);
    ClassDB::bind_static_method("SaveFileAccess", D_METHOD("read", "system_id", "config", "encrypted", "encryption_key"), &SaveFileAccess::read, DEFVAL(false), DEFVAL(String()));
    ClassDB::bind_static_method("SaveFileAccess", D_METHOD("delete", "system_id", "config", "encrypted"), &SaveFileAccess::delete_file, DEFVAL(false));
}