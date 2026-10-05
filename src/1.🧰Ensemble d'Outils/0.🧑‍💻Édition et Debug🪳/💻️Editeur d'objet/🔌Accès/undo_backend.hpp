#ifndef UNDO_BACKEND_HPP
#define UNDO_BACKEND_HPP

#include "value_accessor.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot
{
// Toute écriture d'un widget passe par ici : une seule action d'annulation par édition.
//  - committing=false (glissement, frappe) : la valeur est appliquée directement, sans action ; la valeur
//    d'avant l'édition est mémorisée.
//  - committing=true (validation) : UNE action ancien → nouveau est enregistrée (sans ré-exécution,
//    la valeur étant déjà appliquée).
// Implémentations : RuntimeUndoBackend (UndoRedo, en jeu) / EditorUndoBackend (EditorUndoRedoManager, éditeur).
class UndoBackend : public RefCounted
{
    GDCLASS(UndoBackend, RefCounted)

  private:
    // Racine d'avant l'édition en cours (copie profonde), par "ObjectID:propriété:chemin".
    Dictionary pending_old_values;

    static String _pending_key(const Ref<ValueAccessor> &p_accessor);

  protected:
    static void _bind_methods();

    // Enregistre l'action ancien → nouveau. La nouvelle valeur est DÉJÀ appliquée à l'objet.
    virtual void _record_property(Object *p_target, const StringName &p_property, const Variant &p_old, const Variant &p_new) = 0;
    // Enregistre ET exécute une action faite d'appels de méthodes (chaque appel : [objet, méthode, arguments...]).
    virtual void _record_calls(const String &p_name, const Array &p_do_calls, const Array &p_undo_calls) = 0;

  public:
    void edit_property(const Ref<ValueAccessor> &p_accessor, const Variant &p_value, bool p_committing);
    // Oublie la valeur de départ d'une édition abandonnée (jamais validée).
    void discard_edit(const Ref<ValueAccessor> &p_accessor);
    // UNE action annulable faite d'appels de méthodes, exécutée immédiatement (ex: connect / disconnect d'un
    // lien natif, remplacement d'une liste de liens). Chaque appel : [objet, méthode, arguments...].
    void commit_calls(const String &p_name, const Array &p_do_calls, const Array &p_undo_calls);

    // Vrai si ce backend possède son propre historique (jeu). Dans l'éditeur, l'historique est celui de l'éditeur.
    virtual bool can_undo_redo() const { return false; }
    // Annule / rétablit la dernière action (en jeu : historique propre ; dans l'éditeur : celui de l'éditeur).
    virtual bool undo() { return false; }
    virtual bool redo() { return false; }
};
} // namespace godot

#endif // UNDO_BACKEND_HPP
