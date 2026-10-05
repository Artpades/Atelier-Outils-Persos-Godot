#ifndef LAYOUT_DATABASE_EXPORT_PLUGIN_HPP
#define LAYOUT_DATABASE_EXPORT_PLUGIN_HPP

#include <godot_cpp/classes/editor_export_plugin.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot
{
// Plugin d'export (éditeur) : la LayoutDatabase n'est référencée que par un réglage de projet, donc absente
// d'un export « ressources sélectionnées ». Ce plugin l'ajoute toujours, avec ses dépendances non importées
// (scripts, ressources externes), telles quelles. Pour éviter un doublon (export « toutes les ressources »),
// l'export normal de ces mêmes fichiers est ignoré (skip).
class LayoutDatabaseExportPlugin : public EditorExportPlugin
{
    GDCLASS(LayoutDatabaseExportPlugin, EditorExportPlugin)

  private:
    PackedStringArray added; // fichiers ajoutés par ce plugin pendant l'export en cours

    void _collect(const String &p_path);

  protected:
    static void _bind_methods() {}

  public:
    String _get_name() const override;
    void _export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) override;
    void _export_file(const String &p_path, const String &p_type, const PackedStringArray &p_features) override;
    void _export_end() override;
};
} // namespace godot

#endif // LAYOUT_DATABASE_EXPORT_PLUGIN_HPP
