#include "layout_database_export_plugin.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/layout_database.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

using namespace godot;

String LayoutDatabaseExportPlugin::_get_name() const { return "LayoutDatabaseExportPlugin"; }

void LayoutDatabaseExportPlugin::_collect(const String &p_path)
{
    // Fichier importé (texture, son...) : son export passe par le système d'import, non dupliqué ici.
    if (added.has(p_path) || !FileAccess::file_exists(p_path) || FileAccess::file_exists(p_path + String(".import")))
    {
        return;
    }
    added.push_back(p_path);
    add_file(p_path, FileAccess::get_file_as_bytes(p_path), false);
    // Dépendances : « uid://…::Type::res://chemin » ou « res://chemin::Type » selon la version.
    PackedStringArray dependencies = ResourceLoader::get_singleton()->get_dependencies(p_path);
    for (const String &dependency : dependencies)
    {
        PackedStringArray parts = dependency.split("::");
        for (const String &part : parts)
        {
            if (part.begins_with("res://"))
            {
                _collect(part);
            }
        }
    }
}

void LayoutDatabaseExportPlugin::_export_begin(const PackedStringArray &, bool, const String &, uint32_t)
{
    added.clear();
    String path = LayoutDatabase::get_project_path();
    if (path.is_empty() || !FileAccess::file_exists(path))
    {
        return; // pas de base : rien à ajouter
    }
    _collect(path);
    log_it(U"Interfaces par type ajoutées à l'export : " + String(", ").join(added), "LayoutDatabaseExportPlugin");
}

void LayoutDatabaseExportPlugin::_export_file(const String &p_path, const String &, const PackedStringArray &)
{
    if (added.has(p_path))
    {
        skip(); // déjà ajouté tel quel par _export_begin
    }
}

void LayoutDatabaseExportPlugin::_export_end() { added.clear(); }
