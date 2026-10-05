#include "runtime_undo_backend.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>

using namespace godot;

void RuntimeUndoBackend::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("has_undo"), &RuntimeUndoBackend::has_undo);
    ClassDB::bind_method(D_METHOD("has_redo"), &RuntimeUndoBackend::has_redo);
    ClassDB::bind_method(D_METHOD("clear_history"), &RuntimeUndoBackend::clear_history);
}

RuntimeUndoBackend::RuntimeUndoBackend() { undo_redo = memnew(UndoRedo); }

void RuntimeUndoBackend::_notification(int p_what)
{
    if (p_what == NOTIFICATION_PREDELETE && undo_redo)
    {
        memdelete(undo_redo);
        undo_redo = nullptr;
    }
}

void RuntimeUndoBackend::_record_property(Object *p_target, const StringName &p_property, const Variant &p_old, const Variant &p_new)
{
    // MERGE_ENDS : des modifications successives de la même propriété (ex: flèches d'un SpinBox)
    // fusionnent en une seule action, qui garde la valeur de départ de la première.
    undo_redo->create_action(U"Modifier « " + String(p_property) + U" »", UndoRedo::MERGE_ENDS);
    undo_redo->add_do_property(p_target, p_property, p_new);
    undo_redo->add_undo_property(p_target, p_property, p_old);
    undo_redo->commit_action(false); // déjà appliquée
}

bool RuntimeUndoBackend::undo() { return undo_redo->undo(); }
bool RuntimeUndoBackend::redo() { return undo_redo->redo(); }
bool RuntimeUndoBackend::has_undo() const { return undo_redo->has_undo(); }
bool RuntimeUndoBackend::has_redo() const { return undo_redo->has_redo(); }
void RuntimeUndoBackend::clear_history() { undo_redo->clear_history(); }

namespace
{
// [objet, méthode, arguments...] -> Callable(objet, méthode).bindv(arguments)
Callable to_callable(const Array &p_call)
{
    Object *object = p_call[0];
    Array arguments = p_call.slice(2);
    return Callable(object, p_call[1]).bindv(arguments);
}
} // namespace

void RuntimeUndoBackend::_record_calls(const String &p_name, const Array &p_do_calls, const Array &p_undo_calls)
{
    undo_redo->create_action(p_name);
    for (int i = 0; i < p_do_calls.size(); i++)
    {
        undo_redo->add_do_method(to_callable(p_do_calls[i]));
    }
    for (int i = 0; i < p_undo_calls.size(); i++)
    {
        undo_redo->add_undo_method(to_callable(p_undo_calls[i]));
    }
    undo_redo->commit_action(true);
}
