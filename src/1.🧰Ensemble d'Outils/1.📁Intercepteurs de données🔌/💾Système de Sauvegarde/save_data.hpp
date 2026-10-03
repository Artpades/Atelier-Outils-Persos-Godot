#ifndef SAVE_DATA_HPP
#define SAVE_DATA_HPP

#include "save_config.hpp"

#include "godot_cpp/variant/dictionary.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot
{
// Un base de donnée contenant les données de propriétés à sauvegarder.
class SaveData : public Resource
{
    GDCLASS(SaveData, Resource);

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

  protected:
    static void _bind_methods();

  public:
    SaveData();

    // Getter / Setter

    String get_system_id() const { return system_id; }
    void set_system_id(const String &p_id) { system_id = p_id; }

    Dictionary get_data() const { return data; }
    void set_data(const Dictionary &p_data) { data = p_data; }

    bool get_is_encrypted() const { return is_encrypted; }
    void set_is_encrypted(bool p_encrypted) { is_encrypted = p_encrypted; }

    String get_encryption_key() const { return encryption_key; }
    void set_encryption_key(const String &p_key) { encryption_key = p_key; }

    Ref<SaveConfig> get_file_config() const { return file_config; }
    void set_file_config(const Ref<SaveConfig> &p_config) { file_config = p_config; }
};
} // namespace godot

#endif