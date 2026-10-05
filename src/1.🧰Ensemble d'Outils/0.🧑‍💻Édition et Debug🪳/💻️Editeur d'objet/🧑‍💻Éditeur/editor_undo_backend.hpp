#ifndef EDITOR_UNDO_BACKEND_HPP
#define EDITOR_UNDO_BACKEND_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔌Accès/undo_backend.hpp"

namespace godot
{
// Annulation dans l'éditeur : EditorUndoRedoManager (enregistré au niveau EDITOR).
// L'éditeur choisit l'historique selon l'objet (scène éditée ou historique global) et marque la scène
// comme modifiée.
// undo()/redo() : EditorUndoRedoManager n'expose pas d'annulation (vérifié dans extension_api.json) ;
// le raccourci est donc transmis à la fenêtre principale de l'éditeur, qui exécute son propre
// « Annuler » / « Refaire » (raccourcis par défaut Ctrl+Z / Ctrl+Maj+Z).
class EditorUndoBackend : public UndoBackend
{
    GDCLASS(EditorUndoBackend, UndoBackend)

  private:
    static bool _forward_shortcut(bool p_redo);

  protected:
    static void _bind_methods();
    void _record_property(Object *p_target, const StringName &p_property, const Variant &p_old, const Variant &p_new) override;
    void _record_calls(const String &p_name, const Array &p_do_calls, const Array &p_undo_calls) override;

  public:
    bool undo() override;
    bool redo() override;
};
} // namespace godot

#endif // EDITOR_UNDO_BACKEND_HPP
