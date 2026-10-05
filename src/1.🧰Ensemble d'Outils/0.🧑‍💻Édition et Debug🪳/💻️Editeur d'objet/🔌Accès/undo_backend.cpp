#include "undo_backend.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void UndoBackend::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("edit_property", "accessor", "value", "committing"), &UndoBackend::edit_property);
    ClassDB::bind_method(D_METHOD("discard_edit", "accessor"), &UndoBackend::discard_edit);
    ClassDB::bind_method(D_METHOD("commit_calls", "name", "do_calls", "undo_calls"), &UndoBackend::commit_calls);
    ClassDB::bind_method(D_METHOD("can_undo_redo"), &UndoBackend::can_undo_redo);
    ClassDB::bind_method(D_METHOD("undo"), &UndoBackend::undo);
    ClassDB::bind_method(D_METHOD("redo"), &UndoBackend::redo);
}

String UndoBackend::_pending_key(const Ref<ValueAccessor> &p_accessor)
{
    // Une édition = une valeur pointée (racine + chemin) ; l'annulation, elle, porte sur la racine.
    return String::num_uint64((uint64_t)p_accessor->get_target()->get_instance_id()) + ":" + String(p_accessor->get_property()) + ":" + Variant(p_accessor->get_path()).stringify();
}

void UndoBackend::edit_property(const Ref<ValueAccessor> &p_accessor, const Variant &p_value, bool p_committing)
{
    if (p_accessor.is_null())
    {
        return;
    }
    Object *target = p_accessor->get_target();
    if (!target)
    {
        return;
    }
    if (!p_accessor->is_undoable())
    {
        p_accessor->write(p_value); // valeur locale (ex: argument de méthode) : hors historique
        return;
    }

    String key = _pending_key(p_accessor);
    if (!pending_old_values.has(key))
    {
        // Première écriture de cette édition : copie PROFONDE de la racine, sinon une collection modifiée
        // ailleurs en place rendrait l'annulation sans effet.
        pending_old_values[key] = p_accessor->read_root().duplicate(true);
    }

    p_accessor->write(p_value);
    if (!p_committing)
    {
        return;
    }

    Variant old_root = pending_old_values[key];
    pending_old_values.erase(key);

    // La valeur réellement prise (un setter a pu la transformer) devient la valeur "refaire".
    Variant new_root = p_accessor->read_root().duplicate(true);
    if (old_root.get_type() == new_root.get_type() && old_root.hash_compare(new_root))
    {
        return; // rien n'a changé : pas d'action vide dans l'historique
    }
    _record_property(target, p_accessor->get_property(), old_root, new_root);
}

void UndoBackend::discard_edit(const Ref<ValueAccessor> &p_accessor)
{
    if (p_accessor.is_valid() && p_accessor->get_target())
    {
        pending_old_values.erase(_pending_key(p_accessor));
    }
}

void UndoBackend::commit_calls(const String &p_name, const Array &p_do_calls, const Array &p_undo_calls)
{
    if (p_do_calls.is_empty())
    {
        return;
    }
    _record_calls(p_name, p_do_calls, p_undo_calls);
}
