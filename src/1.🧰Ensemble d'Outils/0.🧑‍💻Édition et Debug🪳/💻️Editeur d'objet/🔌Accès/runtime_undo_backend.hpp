#ifndef RUNTIME_UNDO_BACKEND_HPP
#define RUNTIME_UNDO_BACKEND_HPP

#include "undo_backend.hpp"

#include <godot_cpp/classes/undo_redo.hpp>

namespace godot
{
// Annulation en jeu : UndoRedo possédé par ce backend (créé à la construction, libéré dans NOTIFICATION_PREDELETE).
class RuntimeUndoBackend : public UndoBackend
{
    GDCLASS(RuntimeUndoBackend, UndoBackend)

  private:
    UndoRedo *undo_redo = nullptr; // possédé

  protected:
    static void _bind_methods();
    void _notification(int p_what);
    void _record_property(Object *p_target, const StringName &p_property, const Variant &p_old, const Variant &p_new) override;
    void _record_calls(const String &p_name, const Array &p_do_calls, const Array &p_undo_calls) override;

  public:
    RuntimeUndoBackend();

    bool can_undo_redo() const override { return true; }
    bool undo() override;
    bool redo() override;
    bool has_undo() const;
    bool has_redo() const;
    void clear_history();
};
} // namespace godot

#endif // RUNTIME_UNDO_BACKEND_HPP
