#include "gizmo_config_window.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

ObjectID GizmoConfigWindow::current_window_id;

void GizmoConfigWindow::open_for(GizmoHost *p_host)
{
    EditorInterface *editor = EditorInterface::get_singleton();
    if (!p_host || !editor || !editor->get_base_control())
    {
        return;
    }
    close_current();

    // Registre du GizmoHost : créé s'il n'en a pas (sous-ressource de la scène éditée).
    Ref<GizmoRegistry> registry = p_host->get_registry();
    if (registry.is_null())
    {
        registry.instantiate();
        p_host->set_registry(registry);
        editor->mark_scene_as_unsaved();
    }

    GizmoConfigWindow *window = memnew(GizmoConfigWindow);
    window->host_id = ObjectID(p_host->get_instance_id());
    window->set_title(U"Configurer les gizmos — " + String(p_host->get_name()));
    window->set_size(Vector2i(1000, 650));
    window->set_theme(editor->get_editor_theme());
    window->set_visible(false);
    window->connect("close_requested", callable_mp(window, &GizmoConfigWindow::_on_close_requested));
    window->_build_ui();

    Object *target = p_host->resolve_target();
    Node *target_node = Object::cast_to<Node>(target);
    window->target_label->set_text(target ? U"Cible des propriétés : " + (target_node ? String(target_node->get_name()) + " (" + target_node->get_class() + ")" : target->get_class())
                                          : String(U"⚠ Cible introuvable (target_path)"));
    window->list->set_registry(registry);
    window->list->set_context_object(target); // membres proposés pour les sources Propriété / Fonction

    editor->get_base_control()->add_child(window);
    current_window_id = ObjectID(window->get_instance_id());
    window->popup_centered();
}

void GizmoConfigWindow::close_current()
{
    if (GizmoConfigWindow *window = Object::cast_to<GizmoConfigWindow>(ObjectDB::get_instance(current_window_id)))
    {
        window->queue_free();
    }
    current_window_id = ObjectID();
}

void GizmoConfigWindow::_build_ui()
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
    target_label = memnew(Label);
    box->add_child(target_label);
    Label *hint = memnew(Label);
    hint->set_text(U"Chaque gizmo dessine une forme (1 RID) ; le premier paramètre (RID) est fourni par le GizmoHost. "
                   U"Source « Propriété » : chemin relatif à la cible (vide : la cible), propriété ex. position ou global_position.");
    hint->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    hint->set_modulate(Color(1, 1, 1, 0.6));
    box->add_child(hint);
    list = memnew(GizmoListEditor);
    box->add_child(list);
}

void GizmoConfigWindow::_on_close_requested()
{
    if (current_window_id == ObjectID(get_instance_id()))
    {
        current_window_id = ObjectID();
    }
    queue_free();
}
