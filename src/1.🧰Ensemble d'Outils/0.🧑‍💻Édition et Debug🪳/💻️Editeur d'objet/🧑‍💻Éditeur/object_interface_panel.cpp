#include "object_interface_panel.hpp"
#include "editor_undo_backend.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

void ObjectInterfacePanel::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_target", "target"), &ObjectInterfacePanel::set_target);
    ClassDB::bind_method(D_METHOD("get_target"), &ObjectInterfacePanel::get_target);
    ClassDB::bind_method(D_METHOD("get_layout"), &ObjectInterfacePanel::get_layout);
    ClassDB::bind_method(D_METHOD("is_pending"), &ObjectInterfacePanel::is_pending);
    ClassDB::bind_method(D_METHOD("is_dirty"), &ObjectInterfacePanel::is_dirty);
    ClassDB::bind_method(D_METHOD("get_preview"), &ObjectInterfacePanel::get_preview);
    ClassDB::bind_method(D_METHOD("get_gizmo_list"), &ObjectInterfacePanel::get_gizmo_list);
    ClassDB::bind_method(D_METHOD("get_gizmo_host"), &ObjectInterfacePanel::get_gizmo_host);
    ClassDB::bind_method(D_METHOD("save"), &ObjectInterfacePanel::save);
}

ObjectInterfacePanel::ObjectInterfacePanel()
{
    // En-tête : type et interface, état, interface propre, enregistrement.
    HBoxContainer *header = memnew(HBoxContainer);
    add_child(header);
    type_label = memnew(Label);
    type_label->set_h_size_flags(SIZE_EXPAND_FILL);
    type_label->set_clip_text(true);
    header->add_child(type_label);
    dirty_label = memnew(Label);
    dirty_label->set_modulate(Color(1, 0.8, 0.4));
    header->add_child(dirty_label);
    own_button = memnew(Button);
    own_button->connect("pressed", callable_mp(this, &ObjectInterfacePanel::_on_make_own));
    header->add_child(own_button);
    Button *save_button = memnew(Button);
    save_button->set_text("Enregistrer");
    save_button->set_tooltip_text(U"Enregistrer l'interface dans la base du projet (réglage " + String(LayoutDatabase::SETTING_PATH) + ")");
    save_button->connect("pressed", callable_mp(this, &ObjectInterfacePanel::save));
    header->add_child(save_button);

    tabs = memnew(TabContainer);
    tabs->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(tabs);
    preview = memnew(ProceduralEditor);
    preview->set_undo_backend(Ref<UndoBackend>(memnew(EditorUndoBackend)));
    preview->set_draw_gizmos(false); // dessinés par gizmo_host (registre en cours d'édition)
    tabs->add_child(preview);
    tabs->set_tab_title(0, U"Aperçu");
    data_matrix = memnew(DataMatrixEditor);
    data_matrix->connect("registry_changed", callable_mp(this, &ObjectInterfacePanel::_on_registry_changed));
    tabs->add_child(data_matrix);
    tabs->set_tab_title(1, U"Données");
    group_matrix = memnew(GroupMatrixEditor);
    group_matrix->connect("registry_changed", callable_mp(this, &ObjectInterfacePanel::_on_registry_changed));
    tabs->add_child(group_matrix);
    tabs->set_tab_title(2, "Groupes");
    gizmo_list = memnew(GizmoListEditor);
    gizmo_list->connect("registry_changed", callable_mp(this, &ObjectInterfacePanel::_on_gizmos_changed));
    tabs->add_child(gizmo_list);
    tabs->set_tab_title(3, "Gizmos");

    gizmo_host = memnew(GizmoHost);
    add_child(gizmo_host, false, INTERNAL_MODE_BACK);
}

void ObjectInterfacePanel::set_target(Object *p_target)
{
    target_id = p_target ? ObjectID(p_target->get_instance_id()) : ObjectID();
    layout.unref();
    pending = false;
    own_type = String();
    _set_dirty(false);
    if (!p_target)
    {
        _bind_layout();
        return;
    }
    // Base du projet (en mémoire si le fichier n'existe pas encore : créé à l'enregistrement).
    database = LayoutDatabase::load_project_database();
    if (database.is_null())
    {
        database.instantiate();
    }
    PackedStringArray chain = LayoutDatabase::type_chain(p_target);
    own_type = chain.is_empty() ? p_target->get_class() : chain[0];
    layout = database->find_layout(p_target);
    if (layout.is_null())
    {
        layout.instantiate();
        layout->set_type_name(own_type);
        pending = true;
    }
    _bind_layout();
}

void ObjectInterfacePanel::_bind_layout()
{
    Object *target = get_target();
    if (layout.is_null() || !target)
    {
        gizmo_host->set_registry(Ref<GizmoRegistry>());
        gizmo_host->set_target_object(nullptr);
        preview->clear_registries();
        preview->set_target(nullptr);
        _update_header();
        return;
    }
    layout->ensure_registries();
    Ref<DataRegistry> data = layout->get_data_registry();
    Ref<GroupRegistry> groups = layout->get_group_registry();
    data->sync(target, groups); // membres ajoutés ou retirés depuis le dernier enregistrement

    preview->set_group_registry(groups);
    preview->set_registry(data);
    preview->set_target(target);
    data_matrix->set_registry(data);
    group_matrix->set_registries(groups, data);
    gizmo_list->set_context_object(target);
    gizmo_list->set_registry(layout->get_gizmo_registry());
    gizmo_host->set_registry(layout->get_gizmo_registry());
    gizmo_host->set_target_object(target);
    _update_header();
}

void ObjectInterfacePanel::_update_header()
{
    Object *target = get_target();
    if (!target || layout.is_null())
    {
        type_label->set_text(U"Aucun objet : sélectionner un nœud puis rouvrir la fenêtre.");
        own_button->set_visible(false);
        return;
    }
    Node *node = Object::cast_to<Node>(target);
    String name = node ? String(node->get_name()) + " (" + own_type + ")" : own_type;
    bool inherited = !pending && layout->get_type_name() != own_type;
    String state = pending    ? U"nouvelle interface « " + own_type + U" » (ajoutée à la base à l'enregistrement)"
                   : inherited ? U"interface héritée de « " + layout->get_type_name() + U" »"
                               : U"interface « " + layout->get_type_name() + U" »";
    type_label->set_text(name + U"  ·  " + state);
    own_button->set_visible(inherited);
    own_button->set_text(U"Créer une interface propre à « " + own_type + U" »");
    own_button->set_tooltip_text(U"Copie l'interface héritée dans une nouvelle interface pour « " + own_type + U" » seulement");
}

void ObjectInterfacePanel::_set_dirty(bool p_dirty)
{
    dirty = p_dirty;
    dirty_label->set_text(dirty ? String(U"● non enregistrée") : String());
}

void ObjectInterfacePanel::_on_registry_changed()
{
    _set_dirty(true);
    preview->rebuild();
}

void ObjectInterfacePanel::_on_gizmos_changed() { _set_dirty(true); }

void ObjectInterfacePanel::_on_make_own()
{
    if (layout.is_null() || own_type.is_empty())
    {
        return;
    }
    Ref<TypeLayout> own;
    own.instantiate();
    own->set_type_name(own_type);
    own->set_priority(layout->get_priority());
    // Copie profonde : l'interface parente reste inchangée.
    if (layout->get_data_registry().is_valid())
    {
        own->set_data_registry(layout->get_data_registry()->duplicate(true));
    }
    if (layout->get_group_registry().is_valid())
    {
        own->set_group_registry(layout->get_group_registry()->duplicate(true));
    }
    if (layout->get_gizmo_registry().is_valid())
    {
        own->set_gizmo_registry(layout->get_gizmo_registry()->duplicate(true));
    }
    layout = own;
    pending = true;
    _set_dirty(true);
    _bind_layout();
}

Error ObjectInterfacePanel::save()
{
    if (layout.is_null() || database.is_null())
    {
        return ERR_UNCONFIGURED;
    }
    if (pending)
    {
        TypedArray<TypeLayout> layouts = database->get_layouts();
        layouts.push_back(layout);
        database->set_layouts(layouts);
        pending = false;
    }
    String path = LayoutDatabase::get_project_path();
    Error error = ResourceSaver::get_singleton()->save(database, path);
    if (error != OK)
    {
        log_it(U"[color=red]Impossible d'enregistrer les interfaces dans « " + path + U" »[/color]", "ObjectInterfacePanel");
        return error;
    }
    if (database->get_path().is_empty())
    {
        database->take_over_path(path); // les chargements suivants renvoient cette instance (cache)
    }
    _set_dirty(false);
    _update_header();
    log_it(U"[color=green]Interface « " + layout->get_type_name() + U" » enregistrée : " + path + "[/color]", "ObjectInterfacePanel");
    return OK;
}

Object *ObjectInterfacePanel::get_target() const { return ObjectDB::get_instance(target_id); }
Ref<TypeLayout> ObjectInterfacePanel::get_layout() const { return layout; }
bool ObjectInterfacePanel::is_pending() const { return pending; }
bool ObjectInterfacePanel::is_dirty() const { return dirty; }
ProceduralEditor *ObjectInterfacePanel::get_preview() const { return preview; }
GizmoListEditor *ObjectInterfacePanel::get_gizmo_list() const { return gizmo_list; }
GizmoHost *ObjectInterfacePanel::get_gizmo_host() const { return gizmo_host; }
