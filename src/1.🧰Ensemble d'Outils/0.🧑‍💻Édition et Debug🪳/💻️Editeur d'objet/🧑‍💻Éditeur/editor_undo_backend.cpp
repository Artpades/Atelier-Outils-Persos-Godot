#include "editor_undo_backend.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧱Construction/procedural_editor.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/undo_redo.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void EditorUndoBackend::_bind_methods() {}

bool EditorUndoBackend::_forward_shortcut(bool p_redo)
{
    EditorInterface *editor = EditorInterface::get_singleton();
    Control *base = editor ? editor->get_base_control() : nullptr;
    Viewport *main_viewport = base ? base->get_viewport() : nullptr;
    if (!main_viewport)
    {
        return false;
    }
    for (bool pressed : {true, false})
    {
        Ref<InputEventKey> key;
        key.instantiate();
        key->set_keycode(KEY_Z);
        key->set_command_or_control_autoremap(true); // Ctrl, ou Cmd sur macOS
        key->set_shift_pressed(p_redo);
        key->set_pressed(pressed);
        // Marqué : un ProceduralEditor de la fenêtre principale ne doit pas le réintercepter (récursion).
        key->set_meta(ProceduralEditor::FORWARDED_SHORTCUT_META, true);
        // push_unhandled_input (et non push_input) : en mode fenêtre unique, push_input transmettrait le clavier
        // à la sous-fenêtre intégrée qui a le focus (notre fenêtre) au lieu des raccourcis de l'éditeur.
        main_viewport->push_unhandled_input(key);
    }
    return true;
}

bool EditorUndoBackend::undo() { return _forward_shortcut(false); }
bool EditorUndoBackend::redo() { return _forward_shortcut(true); }

void EditorUndoBackend::_record_property(Object *p_target, const StringName &p_property, const Variant &p_old, const Variant &p_new)
{
    EditorInterface *editor = EditorInterface::get_singleton();
    EditorUndoRedoManager *undo_redo = editor ? editor->get_editor_undo_redo() : nullptr;
    if (!undo_redo)
    {
        log_it("[color=red]EditorUndoRedoManager indisponible : modification non annulable[/color]", "EditorUndoBackend");
        return;
    }
    undo_redo->create_action(U"Modifier « " + String(p_property) + U" »", UndoRedo::MERGE_ENDS);
    undo_redo->add_do_property(p_target, p_property, p_new);
    undo_redo->add_undo_property(p_target, p_property, p_old);
    undo_redo->commit_action(false); // déjà appliquée
}

void EditorUndoBackend::_record_calls(const String &p_name, const Array &p_do_calls, const Array &p_undo_calls)
{
    EditorInterface *editor = EditorInterface::get_singleton();
    EditorUndoRedoManager *undo_redo = editor ? editor->get_editor_undo_redo() : nullptr;
    if (!undo_redo)
    {
        return;
    }
    // L'historique (scène ou global) est choisi d'après le premier objet des appels.
    undo_redo->create_action(p_name);
    for (int i = 0; i < p_do_calls.size(); i++)
    {
        undo_redo->callv("add_do_method", p_do_calls[i]); // add_do_method(objet, méthode, arguments...) : vararg
    }
    for (int i = 0; i < p_undo_calls.size(); i++)
    {
        undo_redo->callv("add_undo_method", p_undo_calls[i]);
    }
    undo_redo->commit_action(true);
}
