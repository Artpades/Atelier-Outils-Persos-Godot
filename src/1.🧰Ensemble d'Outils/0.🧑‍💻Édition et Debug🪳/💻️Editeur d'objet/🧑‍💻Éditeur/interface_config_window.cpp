#include "interface_config_window.hpp"
#include "editor_undo_backend.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

ObjectID InterfaceConfigWindow::current_window_id;

void InterfaceConfigWindow::_bind_methods() { ClassDB::bind_method(D_METHOD("resync"), &InterfaceConfigWindow::resync); }

void InterfaceConfigWindow::open_for(ProceduralEditor *p_host)
{
    EditorInterface *editor = EditorInterface::get_singleton();
    if (!p_host || !editor || !editor->get_base_control())
    {
        return;
    }
    close_current();

    InterfaceConfigWindow *window = memnew(InterfaceConfigWindow);
    window->host_id = ObjectID(p_host->get_instance_id());
    window->set_title(U"Configurer l'interface — " + String(p_host->get_name()));
    window->set_size(Vector2i(1300, 760));
    window->set_theme(editor->get_editor_theme());
    window->set_visible(false);
    window->connect("close_requested", callable_mp(window, &InterfaceConfigWindow::_on_close_requested));
    window->_build_ui();

    editor->get_base_control()->add_child(window);
    current_window_id = ObjectID(window->get_instance_id());
    window->resync();
    window->popup_centered();
}

void InterfaceConfigWindow::close_current()
{
    if (InterfaceConfigWindow *window = Object::cast_to<InterfaceConfigWindow>(ObjectDB::get_instance(current_window_id)))
    {
        window->queue_free();
    }
    current_window_id = ObjectID();
}

ProceduralEditor *InterfaceConfigWindow::_get_host() const { return Object::cast_to<ProceduralEditor>(ObjectDB::get_instance(host_id)); }

void InterfaceConfigWindow::_build_ui()
{
    Panel *background = memnew(Panel);
    background->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    add_child(background);

    MarginContainer *margin = memnew(MarginContainer);
    margin->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    const char *sides[] = {"margin_left", "margin_top", "margin_right", "margin_bottom"};
    for (const char *side : sides)
    {
        margin->add_theme_constant_override(side, 8);
    }
    add_child(margin);

    HSplitContainer *split = memnew(HSplitContainer);
    margin->add_child(split);

    // --- Gauche : cible, barre de scan, tableau ---
    VBoxContainer *left = memnew(VBoxContainer);
    left->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    left->set_stretch_ratio(2.0);
    split->add_child(left);

    target_label = memnew(Label);
    left->add_child(target_label);

    HBoxContainer *bar = memnew(HBoxContainer);
    left->add_child(bar);
    scan_properties = memnew(CheckBox);
    scan_properties->set_text(U"Propriétés");
    scan_methods = memnew(CheckBox);
    scan_methods->set_text(U"Méthodes");
    scan_signals = memnew(CheckBox);
    scan_signals->set_text("Signaux");
    private_methods = memnew(CheckBox);
    private_methods->set_text(U"Méthodes privées");
    private_methods->set_tooltip_text(U"Inclure les méthodes et signaux commençant par « _ »");
    CheckBox *scans[] = {scan_properties, scan_methods, scan_signals, private_methods};
    for (CheckBox *scan : scans)
    {
        scan->connect("toggled", callable_mp(this, &InterfaceConfigWindow::_on_scan_toggled));
        bar->add_child(scan);
    }
    scope = memnew(OptionButton);
    scope->set_tooltip_text(U"Périmètre des membres scannés");
    scope->add_item("Auto (script, sinon classe)", DataEntry::SCOPE_AUTO);
    scope->add_item("Script seulement", DataEntry::SCOPE_SCRIPT_ONLY);
    scope->add_item("Classe seulement", DataEntry::SCOPE_CLASS_ONLY);
    scope->add_item("Tout (moteur compris)", DataEntry::SCOPE_ALL);
    scope->connect("item_selected", callable_mp(this, &InterfaceConfigWindow::_on_scope_selected));
    bar->add_child(scope);
    Button *refresh = memnew(Button);
    refresh->set_text("Actualiser");
    refresh->set_tooltip_text(U"Relit la cible déclarée (target_path / target_resource) et ses membres");
    refresh->connect("pressed", callable_mp(this, &InterfaceConfigWindow::resync));
    bar->add_child(refresh);

    TabContainer *tabs = memnew(TabContainer);
    tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    left->add_child(tabs);
    matrix = memnew(DataMatrixEditor);
    matrix->connect("registry_changed", callable_mp(this, &InterfaceConfigWindow::_on_registry_changed));
    tabs->add_child(matrix);
    tabs->set_tab_title(0, U"Données");
    group_matrix = memnew(GroupMatrixEditor);
    group_matrix->connect("registry_changed", callable_mp(this, &InterfaceConfigWindow::_on_registry_changed));
    tabs->add_child(group_matrix);
    tabs->set_tab_title(1, "Groupes");
    style_editor = memnew(InterfaceStyleEditor);
    style_editor->connect("style_changed", callable_mp(this, &InterfaceConfigWindow::_on_style_changed));
    tabs->add_child(style_editor);
    tabs->set_tab_title(2, "Style");

    // --- Droite : aperçu en direct ---
    VBoxContainer *right = memnew(VBoxContainer);
    right->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    right->set_custom_minimum_size(Vector2(360, 0));
    split->add_child(right);
    Label *preview_title = memnew(Label);
    preview_title->set_text(U"Aperçu (branché sur la vraie cible)");
    preview_title->set_modulate(Color(1, 1, 1, 0.7));
    right->add_child(preview_title);
    preview = memnew(ProceduralEditor);
    preview->set_undo_backend(Ref<UndoBackend>(memnew(EditorUndoBackend)));
    right->add_child(preview);
}

void InterfaceConfigWindow::resync()
{
    ProceduralEditor *host = _get_host();
    if (!host)
    {
        target_label->set_text(U"Le ProceduralEditor configuré n'existe plus.");
        return;
    }

    // Registre du ProceduralEditor : créé s'il n'en a pas (enregistré avec la scène).
    registry = host->get_registry();
    if (registry.is_null())
    {
        registry.instantiate();
        host->set_registry(registry);
        EditorInterface::get_singleton()->mark_scene_as_unsaved();
    }

    scan_properties->set_pressed_no_signal(registry->get_scan_properties());
    scan_methods->set_pressed_no_signal(registry->get_scan_methods());
    scan_signals->set_pressed_no_signal(registry->get_scan_signals());
    private_methods->set_pressed_no_signal(registry->get_include_private_methods());

    // Registre des groupes du ProceduralEditor : créé s'il n'en a pas.
    groups = host->get_group_registry();
    if (groups.is_null())
    {
        groups.instantiate();
        host->set_group_registry(groups);
        EditorInterface::get_singleton()->mark_scene_as_unsaved();
    }
    scope->select(scope->get_item_index(registry->get_scope()));

    Object *target = host->resolve_declared_target();
    if (target)
    {
        Node *node = Object::cast_to<Node>(target);
        target_label->set_text("Cible : " + (node ? String(node->get_name()) + " (" + node->get_class() + ")" : target->get_class()));
        registry->sync(target, groups);
    }
    else
    {
        target_label->set_text(U"⚠ Aucune cible : renseigner target_path ou target_resource du ProceduralEditor, puis Actualiser.");
    }

    matrix->set_context_object(target);
    group_matrix->set_context_object(target);
    style_editor->set_context_object(target);
    style_editor->set_owner(host, "style"); // style global : propriété « style » du ProceduralEditor
    matrix->set_registry(registry);
    group_matrix->set_registries(groups, registry);
    preview->set_style(host->get_style());
    preview->set_group_registry(groups);
    preview->set_registry(registry);
    preview->set_target(target);
}

void InterfaceConfigWindow::_on_scan_toggled(bool p_on)
{
    (void)p_on;
    if (registry.is_null())
    {
        return;
    }
    registry->set_scan_properties(scan_properties->is_pressed());
    registry->set_scan_methods(scan_methods->is_pressed());
    registry->set_scan_signals(scan_signals->is_pressed());
    registry->set_include_private_methods(private_methods->is_pressed());
    _on_registry_changed();
    resync();
}

void InterfaceConfigWindow::_on_scope_selected(int64_t p_index)
{
    if (registry.is_null())
    {
        return;
    }
    registry->set_scope((DataEntry::InspectionScope)scope->get_item_id(p_index));
    _on_registry_changed();
    resync();
}

void InterfaceConfigWindow::_on_style_changed()
{
    if (ProceduralEditor *host = _get_host())
    {
        preview->set_style(host->get_style());
    }
    _on_registry_changed();
}

void InterfaceConfigWindow::_on_registry_changed()
{
    // Le registre est une sous-ressource de la scène éditée : la scène doit être enregistrée.
    EditorInterface::get_singleton()->mark_scene_as_unsaved();
    preview->rebuild();
}

void InterfaceConfigWindow::_on_close_requested()
{
    if (current_window_id == ObjectID(get_instance_id()))
    {
        current_window_id = ObjectID();
    }
    queue_free();
}
