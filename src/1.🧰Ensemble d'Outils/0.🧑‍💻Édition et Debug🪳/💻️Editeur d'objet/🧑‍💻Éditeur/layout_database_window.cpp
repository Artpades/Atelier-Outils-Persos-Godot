#include "layout_database_window.hpp"
#include "editor_undo_backend.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/container_widgets.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

ObjectID LayoutDatabaseWindow::current_window_id;

namespace
{
Button *make_button(const String &p_text, const String &p_tooltip = String())
{
    Button *button = memnew(Button);
    button->set_text(p_text);
    button->set_tooltip_text(p_tooltip);
    return button;
}
} // namespace

void LayoutDatabaseWindow::_bind_methods()
{
    ClassDB::bind_static_method("LayoutDatabaseWindow", D_METHOD("open"), &LayoutDatabaseWindow::open);
    ClassDB::bind_static_method("LayoutDatabaseWindow", D_METHOD("close_current"), &LayoutDatabaseWindow::close_current);
    ClassDB::bind_method(D_METHOD("add_type", "type"), &LayoutDatabaseWindow::_add_type);
    ClassDB::bind_method(D_METHOD("get_database"), &LayoutDatabaseWindow::get_database);
}

LayoutDatabaseWindow *LayoutDatabaseWindow::open()
{
    EditorInterface *editor = EditorInterface::get_singleton();
    if (!editor || !editor->get_base_control())
    {
        return nullptr;
    }
    if (LayoutDatabaseWindow *existing = Object::cast_to<LayoutDatabaseWindow>(ObjectDB::get_instance(current_window_id)))
    {
        existing->grab_focus();
        return existing;
    }
    LayoutDatabaseWindow *window = memnew(LayoutDatabaseWindow);
    window->set_title(U"Interfaces par type");
    window->set_size(Vector2i(1300, 780));
    window->set_theme(editor->get_editor_theme());
    window->set_visible(false);
    window->connect("close_requested", callable_mp(window, &LayoutDatabaseWindow::_on_close_requested));
    window->database = LayoutDatabase::load_project_database();
    window->_build_ui();
    editor->get_base_control()->add_child(window);
    current_window_id = ObjectID(window->get_instance_id());
    window->_refresh_types();
    window->popup_centered();
    return window;
}

void LayoutDatabaseWindow::close_current()
{
    if (LayoutDatabaseWindow *window = Object::cast_to<LayoutDatabaseWindow>(ObjectDB::get_instance(current_window_id)))
    {
        window->queue_free();
    }
    current_window_id = ObjectID();
}

Variant LayoutDatabaseWindow::instantiate_sample(const String &p_type)
{
    return ObjectWidget::instantiate_class(p_type); // natif via ClassDB, class_name via la liste des classes globales
}

Ref<TypeLayout> LayoutDatabaseWindow::_layout(int p_index) const
{
    TypedArray<TypeLayout> layouts = database.is_valid() ? database->get_layouts() : TypedArray<TypeLayout>();
    return (p_index >= 0 && p_index < layouts.size()) ? Ref<TypeLayout>(layouts[p_index]) : Ref<TypeLayout>();
}

void LayoutDatabaseWindow::_build_ui()
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
    VBoxContainer *root = memnew(VBoxContainer);
    margin->add_child(root);

    // En-tête : fichier, création, enregistrement.
    HBoxContainer *header = memnew(HBoxContainer);
    root->add_child(header);
    path_label = memnew(Label);
    path_label->set_text(U"Base : " + LayoutDatabase::get_project_path() + U"  (réglage de projet " + String(LayoutDatabase::SETTING_PATH) + ")");
    path_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    path_label->set_clip_text(true);
    header->add_child(path_label);
    dirty_label = memnew(Label);
    dirty_label->set_modulate(Color(1, 0.8, 0.4));
    header->add_child(dirty_label);
    create_button = make_button(U"Créer la base", U"Créer le fichier de la base à l'emplacement du réglage de projet");
    create_button->connect("pressed", callable_mp(this, &LayoutDatabaseWindow::_on_create));
    header->add_child(create_button);
    Button *save = make_button("Enregistrer");
    save->connect("pressed", callable_mp(this, &LayoutDatabaseWindow::_on_save));
    header->add_child(save);

    HSplitContainer *split = memnew(HSplitContainer);
    split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    root->add_child(split);

    // Gauche : types.
    VBoxContainer *left = memnew(VBoxContainer);
    left->set_custom_minimum_size(Vector2(280, 0));
    split->add_child(left);
    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    left->add_child(scroll);
    type_list = memnew(VBoxContainer);
    type_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    scroll->add_child(type_list);
    HBoxContainer *add_row = memnew(HBoxContainer);
    left->add_child(add_row);
    new_type = memnew(LineEdit);
    new_type->set_placeholder(U"Classe ou class_name");
    new_type->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    new_type->connect("text_submitted", callable_mp(this, &LayoutDatabaseWindow::_on_type_picked));
    add_row->add_child(new_type);
    Button *add = make_button("+", U"Ajouter ce type");
    add->connect("pressed", callable_mp(this, &LayoutDatabaseWindow::_on_add_typed));
    add_row->add_child(add);
    Button *pick = make_button(U"…", U"Choisir le type dans la liste des classes de Godot");
    pick->connect("pressed", callable_mp(this, &LayoutDatabaseWindow::_on_pick_type));
    add_row->add_child(pick);

    // Droite : type sélectionné.
    details = memnew(VBoxContainer);
    details->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    split->add_child(details);
    // Réglages du TypeLayout édités avec le ProceduralEditor lui-même.
    layout_editor = memnew(ProceduralEditor);
    layout_editor->set_scroll_enabled(false);
    layout_editor->set_use_layouts(false);
    layout_editor->set_undo_backend(Ref<UndoBackend>(memnew(EditorUndoBackend)));
    details->add_child(layout_editor);
    Button *sync = make_button(U"Actualiser les membres depuis le type", U"Crée une instance d'exemple du type pour lire ses propriétés, méthodes et signaux");
    sync->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
    sync->connect("pressed", callable_mp(this, &LayoutDatabaseWindow::_on_sync_members));
    details->add_child(sync);
    TabContainer *tabs = memnew(TabContainer);
    tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    details->add_child(tabs);
    data_matrix = memnew(DataMatrixEditor);
    data_matrix->connect("registry_changed", callable_mp(this, &LayoutDatabaseWindow::_on_changed));
    tabs->add_child(data_matrix);
    tabs->set_tab_title(0, U"Données");
    group_matrix = memnew(GroupMatrixEditor);
    group_matrix->connect("registry_changed", callable_mp(this, &LayoutDatabaseWindow::_on_changed));
    tabs->add_child(group_matrix);
    tabs->set_tab_title(1, "Groupes");
    gizmo_list = memnew(GizmoListEditor);
    gizmo_list->connect("registry_changed", callable_mp(this, &LayoutDatabaseWindow::_on_changed));
    tabs->add_child(gizmo_list);
    tabs->set_tab_title(2, "Gizmos");
}

void LayoutDatabaseWindow::_set_dirty(bool p_dirty) { dirty_label->set_text(p_dirty ? String(U"● modifiée (non enregistrée)") : String()); }

void LayoutDatabaseWindow::_refresh_types()
{
    create_button->set_visible(database.is_null());
    TypedArray<Node> children = type_list->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        type_list->remove_child(child);
        child->queue_free();
    }
    if (database.is_null())
    {
        Label *missing = memnew(Label);
        missing->set_text(U"Aucune base : « Créer la base ».");
        type_list->add_child(missing);
        details->set_visible(false);
        return;
    }
    TypedArray<TypeLayout> layouts = database->get_layouts();
    for (int i = 0; i < layouts.size(); i++)
    {
        Ref<TypeLayout> layout = layouts[i];
        if (layout.is_null())
        {
            continue;
        }
        HBoxContainer *row = memnew(HBoxContainer);
        Button *name = make_button(layout->get_type_name() + (layout->get_priority() != 0 ? U"  (priorité " + String::num_int64(layout->get_priority()) + ")" : String()));
        name->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        name->set_flat(i != selected);
        name->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
        name->connect("pressed", callable_mp(this, &LayoutDatabaseWindow::_select).bind(i));
        row->add_child(name);
        Button *remove = make_button(U"✕", "Retirer ce type de la base");
        remove->connect("pressed", callable_mp(this, &LayoutDatabaseWindow::_on_remove_type).bind(i));
        row->add_child(remove);
        type_list->add_child(row);
    }
    Ref<TypeLayout> current = _layout(selected);
    details->set_visible(current.is_valid());
}

void LayoutDatabaseWindow::_select(int64_t p_index)
{
    selected = p_index;
    Ref<TypeLayout> layout = _layout(selected);
    if (layout.is_valid())
    {
        layout->ensure_registries();
        // Réglages du type : seules ses propriétés propres (pas les registres, édités dans les onglets).
        Ref<DataRegistry> settings_registry;
        settings_registry.instantiate();
        settings_registry->set_scope(DataEntry::SCOPE_CLASS_ONLY);
        settings_registry->sync(layout.ptr());
        TypedArray<DataEntry> entries = settings_registry->get_entries();
        for (int i = 0; i < entries.size(); i++)
        {
            Ref<DataEntry> entry = entries[i];
            String name = entry->get_data_name();
            entry->set_enabled(name == "type_name" || name == "include_subclasses" || name == "priority");
        }
        layout_editor->set_registry(settings_registry);
        layout_editor->set_target(layout.ptr());
        data_matrix->set_registry(layout->get_data_registry());
        group_matrix->set_registries(layout->get_group_registry(), layout->get_data_registry());
        gizmo_list->set_context_type(layout->get_type_name()); // membres lus sur une instance d'exemple
        gizmo_list->set_registry(layout->get_gizmo_registry());
    }
    callable_mp(this, &LayoutDatabaseWindow::_refresh_types).call_deferred();
}

void LayoutDatabaseWindow::_add_type(const String &p_type)
{
    String type = p_type.strip_edges();
    if (type.is_empty() || database.is_null())
    {
        return;
    }
    Ref<TypeLayout> layout = database->find_by_type(type);
    if (layout.is_null())
    {
        layout = database->ensure_type(type);
        _set_dirty(true);
    }
    TypedArray<TypeLayout> layouts = database->get_layouts();
    _select(layouts.find(layout));
    _on_sync_members(); // membres lus d'emblée sur une instance d'exemple
}

void LayoutDatabaseWindow::_on_create()
{
    String path = LayoutDatabase::get_project_path();
    Ref<LayoutDatabase> created;
    created.instantiate();
    if (ResourceSaver::get_singleton()->save(created, path) != OK)
    {
        log_it(U"[color=red]Impossible de créer la base « " + path + U" »[/color]", "LayoutDatabaseWindow");
        return;
    }
    created->take_over_path(path); // les chargements suivants renvoient cette instance (cache)
    database = created;
    _refresh_types();
}

void LayoutDatabaseWindow::_on_save()
{
    if (database.is_null())
    {
        return;
    }
    String path = LayoutDatabase::get_project_path();
    if (ResourceSaver::get_singleton()->save(database, path) == OK)
    {
        _set_dirty(false);
        log_it(U"[color=green]Interfaces par type enregistrées : " + path + "[/color]", "LayoutDatabaseWindow");
    }
}

void LayoutDatabaseWindow::_on_add_typed()
{
    _add_type(new_type->get_text());
    new_type->clear();
}

void LayoutDatabaseWindow::_on_pick_type()
{
    if (EditorInterface *editor = EditorInterface::get_singleton())
    {
        editor->popup_create_dialog(callable_mp(this, &LayoutDatabaseWindow::_on_type_picked), "Object", "", U"Type de l'interface");
    }
}

void LayoutDatabaseWindow::_on_type_picked(const String &p_type) { _add_type(p_type); }

void LayoutDatabaseWindow::_on_remove_type(int64_t p_index)
{
    Ref<TypeLayout> layout = _layout(p_index);
    if (layout.is_null())
    {
        return;
    }
    TypedArray<TypeLayout> layouts = database->get_layouts();
    layouts.erase(layout);
    database->set_layouts(layouts);
    selected = -1;
    _set_dirty(true);
    callable_mp(this, &LayoutDatabaseWindow::_refresh_types).call_deferred();
}

void LayoutDatabaseWindow::_on_sync_members()
{
    Ref<TypeLayout> layout = _layout(selected);
    if (layout.is_null())
    {
        return;
    }
    Variant sample = instantiate_sample(layout->get_type_name());
    Object *object = sample.get_validated_object();
    if (!object)
    {
        log_it(U"[color=orange]Type « " + layout->get_type_name() + U" » non instanciable : membres non lus (classe abstraite, éditeur, ou inconnue)[/color]",
               "LayoutDatabaseWindow");
        return;
    }
    layout->ensure_registries();
    layout->get_data_registry()->sync(object, layout->get_group_registry());
    // Instance d'exemple libérée : un RefCounted l'est par le Variant, un objet simple (Node...) explicitement.
    if (!Object::cast_to<RefCounted>(object))
    {
        memdelete(object);
    }
    data_matrix->refresh();
    group_matrix->refresh();
    _set_dirty(true);
}

Ref<LayoutDatabase> LayoutDatabaseWindow::get_database() const { return database; }

void LayoutDatabaseWindow::_on_changed() { _set_dirty(true); }

void LayoutDatabaseWindow::_on_close_requested()
{
    if (current_window_id == ObjectID(get_instance_id()))
    {
        current_window_id = ObjectID();
    }
    queue_free();
}
