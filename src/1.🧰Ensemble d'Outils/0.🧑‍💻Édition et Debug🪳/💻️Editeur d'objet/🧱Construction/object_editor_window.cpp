#include "object_editor_window.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

ObjectID ObjectEditorWindow::current_window_id;

void ObjectEditorWindow::_bind_methods()
{
    ClassDB::bind_static_method("ObjectEditorWindow", D_METHOD("open_for", "target", "backend"), &ObjectEditorWindow::open_for, DEFVAL(Variant()));
    ClassDB::bind_static_method("ObjectEditorWindow", D_METHOD("follow", "target"), &ObjectEditorWindow::follow);
    ClassDB::bind_static_method("ObjectEditorWindow", D_METHOD("get_current"), &ObjectEditorWindow::get_current);
    ClassDB::bind_static_method("ObjectEditorWindow", D_METHOD("close_current"), &ObjectEditorWindow::close_current);
    ClassDB::bind_method(D_METHOD("is_pinned"), &ObjectEditorWindow::is_pinned);
    ClassDB::bind_method(D_METHOD("set_pinned", "pinned"), &ObjectEditorWindow::set_pinned);
    ClassDB::bind_method(D_METHOD("get_editor"), &ObjectEditorWindow::get_editor);
}

ObjectEditorWindow *ObjectEditorWindow::get_current() { return Object::cast_to<ObjectEditorWindow>(ObjectDB::get_instance(current_window_id)); }

ObjectEditorWindow *ObjectEditorWindow::open_for(Object *p_target, const Ref<UndoBackend> &p_backend)
{
    if (!p_target)
    {
        return nullptr;
    }
    ObjectEditorWindow *window = get_current();
    if (!window)
    {
        // Parent : base de l'éditeur (code éditeur uniquement derrière is_editor_hint + singleton), sinon racine du jeu.
        Node *parent = nullptr;
        EditorInterface *editor_interface = Engine::get_singleton()->is_editor_hint() ? EditorInterface::get_singleton() : nullptr;
        if (editor_interface)
        {
            parent = editor_interface->get_base_control();
        }
        else if (SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop()))
        {
            parent = tree->get_root();
        }
        if (!parent)
        {
            return nullptr;
        }
        window = memnew(ObjectEditorWindow);
        window->set_size(Vector2i(560, 720));
        window->set_visible(false);
        if (editor_interface)
        {
            window->set_theme(editor_interface->get_editor_theme());
        }
        window->connect("close_requested", callable_mp(window, &ObjectEditorWindow::_on_close_requested));
        window->_build_ui(p_backend);
        parent->add_child(window);
        current_window_id = ObjectID(window->get_instance_id());
        window->popup_centered();
    }
    window->_show(p_target);
    return window;
}

bool ObjectEditorWindow::follow(Object *p_target)
{
    ObjectEditorWindow *window = get_current();
    if (!window || !p_target || window->is_pinned())
    {
        return false;
    }
    window->_show(p_target);
    return true;
}

void ObjectEditorWindow::close_current()
{
    if (ObjectEditorWindow *window = get_current())
    {
        window->queue_free();
    }
    current_window_id = ObjectID();
}

void ObjectEditorWindow::_build_ui(const Ref<UndoBackend> &p_backend)
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

    VBoxContainer *box = memnew(VBoxContainer);
    margin->add_child(box);
    HBoxContainer *header = memnew(HBoxContainer);
    box->add_child(header);
    target_label = memnew(Label);
    target_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    target_label->set_clip_text(true);
    header->add_child(target_label);
    pin = memnew(CheckBox);
    pin->set_text(U"Épingler");
    pin->set_tooltip_text(U"Garder cet objet quand la sélection change");
    header->add_child(pin);

    editor = memnew(ProceduralEditor);
    if (p_backend.is_valid())
    {
        editor->set_undo_backend(p_backend);
    }
    box->add_child(editor);
}

void ObjectEditorWindow::_show(Object *p_target)
{
    Node *node = Object::cast_to<Node>(p_target);
    String name = node ? String(node->get_name()) : (Object::cast_to<Resource>(p_target) ? Object::cast_to<Resource>(p_target)->get_path() : String());
    set_title(U"Éditeur d'objet — " + (name.is_empty() ? p_target->get_class() : name));
    // Registres résolus pour CHAQUE cible (interface de son type) : ceux de la cible précédente sont oubliés.
    editor->clear_registries();
    editor->set_target(p_target);
    Ref<TypeLayout> layout = editor->get_active_layout();
    target_label->set_text(p_target->get_class() + (layout.is_valid() ? U"  ·  interface « " + layout->get_type_name() + U" »" : String(U"  ·  interface générale")));
}

bool ObjectEditorWindow::is_pinned() const { return pin && pin->is_pressed(); }
void ObjectEditorWindow::set_pinned(bool p_pinned)
{
    if (pin)
    {
        pin->set_pressed(p_pinned);
    }
}
ProceduralEditor *ObjectEditorWindow::get_editor() const { return editor; }

void ObjectEditorWindow::_on_close_requested()
{
    if (current_window_id == ObjectID(get_instance_id()))
    {
        current_window_id = ObjectID();
    }
    queue_free();
}
