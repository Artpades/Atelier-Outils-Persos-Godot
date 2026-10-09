#ifndef OBJECT_EDITOR_PLUGIN_HPP
#define OBJECT_EDITOR_PLUGIN_HPP

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>

#include "layout_database_export_plugin.hpp"

namespace godot
{
/// Ajoute en tête de l'inspecteur le bouton « Configurer l'interface » (ProceduralEditor) ou
/// « Configurer les gizmos » (GizmoHost).
class ProceduralEditorInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(ProceduralEditorInspectorPlugin, EditorInspectorPlugin)

  private:
    void _on_configure_pressed(uint64_t p_host_id);
    void _on_configure_gizmos_pressed(uint64_t p_host_id);

  protected:
    static void _bind_methods() {}

  public:
    bool _can_handle(Object *p_object) const override;
    void _parse_begin(Object *p_object) override;
};

/// Plugin d'éditeur de l'éditeur d'objet (enregistré au niveau EDITOR via EditorPlugins::add_by_type) :
/// - installe l'inspector plugin ci-dessus ;
/// - ouverture automatique de l'ObjectEditorWindow à la sélection (nœuds : EditorSelection ; Resources :
///   objet de l'inspecteur) pour un objet qui a une interface de type, ou suivi de la sélection si la fenêtre
///   est ouverte et non épinglée. Réglage de l'éditeur AUTO_OPEN_SETTING (actif par défaut) ;
/// - export : la base des interfaces toujours incluse (LayoutDatabaseExportPlugin) ;
/// - menu Outils : basculer l'ouverture automatique, ouvrir la fenêtre « Interfaces par type ».
class ObjectEditorPlugin : public EditorPlugin
{
    GDCLASS(ObjectEditorPlugin, EditorPlugin)

  private:
    Ref<ProceduralEditorInspectorPlugin> inspector_plugin;
    Ref<LayoutDatabaseExportPlugin> export_plugin;

    bool _is_auto_open() const;
    void _consider(Object *p_object);
    void _on_selection_changed();
    void _on_inspected_changed();
    void _on_toggle_auto_open();
    void _on_open_layouts();

  protected:
    static void _bind_methods() {}

  public:
    static constexpr const char *AUTO_OPEN_SETTING = "gameplay_toolkit/object_editor/auto_open";

    void _enter_tree() override;
    void _exit_tree() override;
};
} // namespace godot

#endif // OBJECT_EDITOR_PLUGIN_HPP
