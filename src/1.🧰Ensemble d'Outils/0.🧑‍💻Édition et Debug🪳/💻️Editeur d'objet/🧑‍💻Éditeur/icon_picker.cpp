#include "icon_picker.hpp"
#include "editor_undo_backend.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_resource_picker.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

// =====================================================================================================
// EditorIconPicker
// =====================================================================================================

void EditorIconPicker::_bind_methods()
{
    ClassDB::bind_static_method("EditorIconPicker", D_METHOD("copy_editor_icon", "name"), &EditorIconPicker::copy_editor_icon);
}

EditorIconPicker *EditorIconPicker::open(const Callable &p_on_picked)
{
    EditorInterface *editor = EditorInterface::get_singleton();
    if (!editor || !editor->get_base_control())
    {
        return nullptr;
    }
    EditorIconPicker *dialog = memnew(EditorIconPicker);
    dialog->on_picked = p_on_picked;
    dialog->set_title(U"Icône de l'éditeur");
    dialog->set_ok_button_text("Choisir");
    VBoxContainer *box = memnew(VBoxContainer);
    dialog->add_child(box);
    dialog->filter = memnew(LineEdit);
    dialog->filter->set_placeholder("Filtrer (ex : Node, Script, Color...)");
    dialog->filter->connect("text_changed", callable_mp(dialog, &EditorIconPicker::_on_filter_changed));
    box->add_child(dialog->filter);
    dialog->list = memnew(ItemList);
    dialog->list->set_custom_minimum_size(Vector2(560, 380));
    dialog->list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    dialog->list->set_icon_mode(ItemList::ICON_MODE_TOP);
    dialog->list->set_max_columns(0);
    dialog->list->set_same_column_width(true);
    dialog->list->set_fixed_icon_size(Vector2i(24, 24));
    dialog->list->connect("item_activated", callable_mp(dialog, &EditorIconPicker::_on_item_activated));
    box->add_child(dialog->list);
    dialog->connect("confirmed", callable_mp(dialog, &EditorIconPicker::_on_confirmed));
    dialog->connect("canceled", callable_mp((Node *)dialog, &Node::queue_free));
    dialog->_fill(String());
    editor->get_base_control()->add_child(dialog);
    dialog->popup_centered();
    return dialog;
}

void EditorIconPicker::_fill(const String &p_filter)
{
    list->clear();
    Ref<Theme> theme = EditorInterface::get_singleton()->get_editor_theme();
    PackedStringArray names = theme->get_icon_list("EditorIcons");
    names.sort();
    for (const String &name : names)
    {
        if (!p_filter.is_empty() && name.findn(p_filter) < 0)
        {
            continue;
        }
        int index = list->add_item(name, theme->get_icon(name, "EditorIcons"));
        list->set_item_tooltip(index, name);
    }
}

void EditorIconPicker::_on_filter_changed(const String &p_text) { _fill(p_text); }

void EditorIconPicker::_on_item_activated(int64_t p_index)
{
    (void)p_index;
    _on_confirmed();
    hide();
}

void EditorIconPicker::_on_confirmed()
{
    PackedInt32Array selected = list->get_selected_items();
    if (!selected.is_empty())
    {
        Ref<Texture2D> icon = copy_editor_icon(list->get_item_text(selected[0]));
        if (icon.is_valid() && on_picked.is_valid())
        {
            on_picked.call(icon);
        }
    }
    queue_free();
}

Ref<Texture2D> EditorIconPicker::copy_editor_icon(const String &p_name)
{
    String path = String(ICON_DIR) + p_name + ".res";
    if (ResourceLoader::get_singleton()->exists(path))
    {
        return ResourceLoader::get_singleton()->load(path);
    }
    EditorInterface *editor = EditorInterface::get_singleton();
    Ref<Theme> theme = editor ? editor->get_editor_theme() : Ref<Theme>();
    if (theme.is_null() || !theme->has_icon(p_name, "EditorIcons"))
    {
        return Ref<Texture2D>();
    }
    Ref<Image> image = theme->get_icon(p_name, "EditorIcons")->get_image();
    if (image.is_null())
    {
        return Ref<Texture2D>();
    }
    DirAccess::make_dir_recursive_absolute(ProjectSettings::get_singleton()->globalize_path(ICON_DIR));
    Ref<ImageTexture> copy = ImageTexture::create_from_image(image);
    if (ResourceSaver::get_singleton()->save(copy, path) != OK)
    {
        log_it(U"[color=red]Impossible de copier l'icône « " + p_name + U" » dans " + path + "[/color]", "EditorIconPicker");
        return Ref<Texture2D>();
    }
    if (EditorFileSystem *filesystem = editor->get_resource_filesystem())
    {
        filesystem->update_file(path);
    }
    return ResourceLoader::get_singleton()->load(path);
}

// =====================================================================================================
// IconPickerRow
// =====================================================================================================

void IconPickerRow::_bind_methods() { ADD_SIGNAL(MethodInfo("icon_changed")); }

void IconPickerRow::setup(Object *p_owner, const StringName &p_property, const String &p_label)
{
    owner_id = p_owner ? ObjectID(p_owner->get_instance_id()) : ObjectID();
    property = p_property;
    backend = Ref<UndoBackend>(memnew(EditorUndoBackend));
    if (!p_label.is_empty())
    {
        Label *label = memnew(Label);
        label->set_text(p_label);
        label->set_custom_minimum_size(Vector2(120, 0));
        add_child(label);
    }
    EditorResourcePicker *picker = memnew(EditorResourcePicker);
    picker->set_base_type("Texture2D");
    picker->set_edited_resource(p_owner ? Ref<Resource>(p_owner->get(p_property)) : Ref<Resource>());
    picker->set_h_size_flags(SIZE_EXPAND_FILL);
    picker->connect("resource_changed", callable_mp(this, &IconPickerRow::_on_resource_changed));
    add_child(picker);
    Button *editor_icon = memnew(Button);
    editor_icon->set_text(U"Icône de l'éditeur…");
    editor_icon->set_tooltip_text(U"Choisir une icône de l'éditeur Godot (copiée dans " + String(EditorIconPicker::ICON_DIR) + U" pour fonctionner en jeu)");
    editor_icon->connect("pressed", callable_mp(this, &IconPickerRow::_on_editor_icon_pressed));
    add_child(editor_icon);
}

void IconPickerRow::_commit(const Ref<Resource> &p_icon)
{
    Object *owner = ObjectDB::get_instance(owner_id);
    if (!owner)
    {
        return;
    }
    backend->commit_calls(U"Icône : " + String(property), Array::make(Array::make(owner, "set", property, p_icon)),
                          Array::make(Array::make(owner, "set", property, owner->get(property))));
    emit_signal("icon_changed");
}

void IconPickerRow::_on_resource_changed(const Ref<Resource> &p_resource) { _commit(p_resource); }
void IconPickerRow::_on_editor_icon_pressed() { EditorIconPicker::open(callable_mp(this, &IconPickerRow::_on_editor_icon_picked)); }
void IconPickerRow::_on_editor_icon_picked(const Ref<Texture2D> &p_icon)
{
    _commit(p_icon);
    // Sélecteur à jour (la ressource vient d'ailleurs que de lui).
    TypedArray<Node> children = get_children();
    for (int i = 0; i < children.size(); i++)
    {
        if (EditorResourcePicker *picker = Object::cast_to<EditorResourcePicker>(children[i]))
        {
            picker->set_edited_resource(p_icon);
        }
    }
}
