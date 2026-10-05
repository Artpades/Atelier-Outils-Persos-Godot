#include "object_editor_plugin.hpp"
#include "gizmo_config_window.hpp"
#include "editor_undo_backend.hpp"
#include "interface_config_window.hpp"
#include "layout_database_window.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧱Construction/object_editor_window.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

// --- ProceduralEditorInspectorPlugin ---

bool ProceduralEditorInspectorPlugin::_can_handle(Object *p_object) const { return Object::cast_to<ProceduralEditor>(p_object) || Object::cast_to<GizmoHost>(p_object); }

void ProceduralEditorInspectorPlugin::_parse_begin(Object *p_object)
{
    if (Object::cast_to<GizmoHost>(p_object))
    {
        Button *gizmos = memnew(Button);
        gizmos->set_text("Configurer les gizmos");
        gizmos->set_tooltip_text(U"Choisir les dessins automatiques et la source de leurs paramètres");
        gizmos->connect("pressed", callable_mp(this, &ProceduralEditorInspectorPlugin::_on_configure_gizmos_pressed).bind((uint64_t)p_object->get_instance_id()));
        add_custom_control(gizmos);
        return;
    }
    Button *button = memnew(Button);
    button->set_text("Configurer l'interface");
    button->set_tooltip_text(U"Choisir les données affichées et leurs widgets pour la cible de ce ProceduralEditor (target_path / target_resource)");
    button->connect("pressed", callable_mp(this, &ProceduralEditorInspectorPlugin::_on_configure_pressed).bind((uint64_t)p_object->get_instance_id()));
    add_custom_control(button);
}

void ProceduralEditorInspectorPlugin::_on_configure_pressed(uint64_t p_host_id)
{
    // Le nœud a pu être supprimé depuis l'affichage de l'inspecteur : résolution par ObjectID.
    InterfaceConfigWindow::open_for(Object::cast_to<ProceduralEditor>(ObjectDB::get_instance(p_host_id)));
}

void ProceduralEditorInspectorPlugin::_on_configure_gizmos_pressed(uint64_t p_host_id)
{
    GizmoConfigWindow::open_for(Object::cast_to<GizmoHost>(ObjectDB::get_instance(p_host_id)));
}

// --- ObjectEditorPlugin ---

namespace
{
const char32_t *const MENU_AUTO_OPEN = U"Éditeur d'objet : activer/désactiver l'ouverture automatique";
const char32_t *const MENU_LAYOUTS = U"Éditeur d'objet : interfaces par type…";
} // namespace

void ObjectEditorPlugin::_enter_tree()
{
    inspector_plugin.instantiate();
    add_inspector_plugin(inspector_plugin);
    export_plugin.instantiate();
    add_export_plugin(export_plugin);

    EditorInterface *editor = EditorInterface::get_singleton();
    Ref<EditorSettings> settings = editor->get_editor_settings();
    if (settings.is_valid())
    {
        if (!settings->has_setting(AUTO_OPEN_SETTING))
        {
            settings->set_setting(AUTO_OPEN_SETTING, true);
        }
        settings->set_initial_value(AUTO_OPEN_SETTING, true, false);
        Dictionary info;
        info["name"] = AUTO_OPEN_SETTING;
        info["type"] = Variant::BOOL;
        settings->add_property_info(info);
    }
    // Différé : la sélection et l'inspecteur sont mis à jour en plusieurs étapes dans la même image.
    editor->get_selection()->connect("selection_changed", callable_mp(this, &ObjectEditorPlugin::_on_selection_changed), CONNECT_DEFERRED);
    editor->get_inspector()->connect("edited_object_changed", callable_mp(this, &ObjectEditorPlugin::_on_inspected_changed), CONNECT_DEFERRED);
    add_tool_menu_item(MENU_AUTO_OPEN, callable_mp(this, &ObjectEditorPlugin::_on_toggle_auto_open));
    add_tool_menu_item(MENU_LAYOUTS, callable_mp(this, &ObjectEditorPlugin::_on_open_layouts));
}

void ObjectEditorPlugin::_exit_tree()
{
    if (EditorInterface *editor = EditorInterface::get_singleton())
    {
        Callable selection = callable_mp(this, &ObjectEditorPlugin::_on_selection_changed);
        if (editor->get_selection()->is_connected("selection_changed", selection))
        {
            editor->get_selection()->disconnect("selection_changed", selection);
        }
        Callable inspected = callable_mp(this, &ObjectEditorPlugin::_on_inspected_changed);
        if (editor->get_inspector() && editor->get_inspector()->is_connected("edited_object_changed", inspected))
        {
            editor->get_inspector()->disconnect("edited_object_changed", inspected);
        }
    }
    remove_tool_menu_item(MENU_AUTO_OPEN);
    remove_tool_menu_item(MENU_LAYOUTS);
    InterfaceConfigWindow::close_current();
    GizmoConfigWindow::close_current();
    LayoutDatabaseWindow::close_current();
    ObjectEditorWindow::close_current();
    if (inspector_plugin.is_valid())
    {
        remove_inspector_plugin(inspector_plugin);
        inspector_plugin.unref();
    }
    if (export_plugin.is_valid())
    {
        remove_export_plugin(export_plugin);
        export_plugin.unref();
    }
}

bool ObjectEditorPlugin::_is_auto_open() const
{
    Ref<EditorSettings> settings = EditorInterface::get_singleton()->get_editor_settings();
    return settings.is_null() || !settings->has_setting(AUTO_OPEN_SETTING) || bool(settings->get_setting(AUTO_OPEN_SETTING));
}

void ObjectEditorPlugin::_consider(Object *p_object)
{
    if (!p_object || ObjectEditorWindow::follow(p_object))
    {
        return; // fenêtre ouverte et non épinglée : elle suit la sélection
    }
    if (ObjectEditorWindow::get_current() || !_is_auto_open())
    {
        return; // épinglée, ou ouverture automatique désactivée
    }
    Ref<LayoutDatabase> database = LayoutDatabase::load_project_database();
    if (database.is_valid() && database->find_layout(p_object).is_valid())
    {
        ObjectEditorWindow::open_for(p_object, Ref<UndoBackend>(memnew(EditorUndoBackend)));
    }
}

void ObjectEditorPlugin::_on_selection_changed()
{
    TypedArray<Node> nodes = EditorInterface::get_singleton()->get_selection()->get_selected_nodes();
    if (nodes.size() == 1)
    {
        _consider(Object::cast_to<Node>(nodes[0]));
    }
}

void ObjectEditorPlugin::_on_inspected_changed()
{
    // Les nœuds passent par la sélection : ici seulement les Resources ouvertes dans l'inspecteur.
    EditorInspector *inspector = EditorInterface::get_singleton()->get_inspector();
    Object *edited = inspector ? inspector->get_edited_object() : nullptr;
    if (Object::cast_to<Resource>(edited))
    {
        _consider(edited);
    }
}

void ObjectEditorPlugin::_on_toggle_auto_open()
{
    Ref<EditorSettings> settings = EditorInterface::get_singleton()->get_editor_settings();
    if (settings.is_null())
    {
        return;
    }
    bool enabled = !_is_auto_open();
    settings->set_setting(AUTO_OPEN_SETTING, enabled);
    log_it(enabled ? String(U"Ouverture automatique de l'éditeur d'objet : activée") : String(U"Ouverture automatique de l'éditeur d'objet : désactivée"), "ObjectEditorPlugin");
}

void ObjectEditorPlugin::_on_open_layouts() { LayoutDatabaseWindow::open(); }
