#ifndef SAVE_DATA_HPP
#define SAVE_DATA_HPP

#include "save_config.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot
{
/// 💾 Données de sauvegarde d'un système de jeu : un identifiant de système et un dictionnaire de données
/// génériques (clés String, valeurs Variant), sérialisé en JSON à l'écriture et reconstruit à la lecture.
class SaveData : public Resource
{
    GDCLASS(SaveData, Resource)

  public:
    SaveData();

    void set_system_id(const String &p_id) { system_id = p_id; }
    String get_system_id() const { return system_id; }

    void set_data(const Dictionary &p_data) { data = p_data; }
    Dictionary get_data() const { return data; }

    void set_is_encrypted(bool p_encrypted) { is_encrypted = p_encrypted; }
    bool get_is_encrypted() const { return is_encrypted; }

    void set_encryption_key(const String &p_key) { encryption_key = p_key; }
    String get_encryption_key() const { return encryption_key; }

    void set_file_config(const Ref<SaveConfig> &p_config) { file_config = p_config; }
    Ref<SaveConfig> get_file_config() const { return file_config; }

  protected:
    static void _bind_methods();

  private:
    /// Identifiant du système de jeu : sert aussi de nom au fichier de sauvegarde.
    String system_id;
    /// Données à sauvegarder (statistiques, état du monde, objets possédés, etc.).
    Dictionary data;
    /// Indique si les données de ce système doivent être chiffrées lors de l'écriture.
    bool is_encrypted = true;
    /// Clé de chiffrement AES-256 pour ce système. Doit faire exactement 32 caractères.
    String encryption_key = "REMPLACER_PAR_UNE_CLE_SECRETE_32";
    /// Chemins et extensions des fichiers de sauvegarde de ce système.
    Ref<SaveConfig> file_config;
};
} // namespace godot

#endif // SAVE_DATA_HPP
