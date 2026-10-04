#ifndef SAVE_CONFIG_HPP
#define SAVE_CONFIG_HPP

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot
{
/// Configurateur des chemins et extensions de fichiers de sauvegarde.
class SaveConfig : public Resource
{
    GDCLASS(SaveConfig, Resource)
  private:
    /// Dossier racine où les fichiers de sauvegarde sont écrits.
    String save_directory = "user://saves/";
    /// Extension des fichiers de sauvegarde non chiffrés.
    String extension = ".json";
    /// Extension des fichiers de sauvegarde chiffrés.
    String encrypted_extension = ".sav";

  protected:
    static void _bind_methods();

  public:
#pragma region Getter / Setter
    void set_save_directory(const String &p_directory) { save_directory = p_directory; }
    String get_save_directory() const { return save_directory; }

    void set_extension(const String &p_extension) { extension = p_extension; }
    String get_extension() const { return extension; }

    void set_encrypted_extension(const String &p_extension) { encrypted_extension = p_extension; }
    String get_encrypted_extension() const { return encrypted_extension; }
#pragma endregion
};
} // namespace godot

#endif // SAVE_CONFIG_HPP