#ifndef WIDGET_CONTEXT_HPP
#define WIDGET_CONTEXT_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/data_entry.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/layout_database.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔌Accès/undo_backend.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔌Accès/value_accessor.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

namespace godot
{
class PropertyWidget;

// Contexte de construction transmis aux widgets qui en contiennent d'autres (collections, objets, Variant,
// méthodes). Implémenté par le ProceduralEditor : un widget construit par build_widget est déjà configuré,
// relié à la synchronisation (SyncController) et à l'annulation (UndoBackend) ; l'appelant n'a qu'à
// l'ajouter à l'arbre.
class WidgetContext : public RefCounted
{
    GDCLASS(WidgetContext, RefCounted)

  protected:
    static void _bind_methods() {}

  public:
    // p_depth : profondeur du nouveau widget ; p_ancestors : ObjectID des objets déjà dépliés au-dessus (anti-cycle).
    // Retourne nullptr si la profondeur maximale est dépassée.
    virtual PropertyWidget *build_widget(const Ref<ValueAccessor> &p_accessor, const Ref<DataEntry> &p_entry, int p_depth, const PackedInt64Array &p_ancestors)
    {
        (void)p_accessor;
        (void)p_entry;
        (void)p_depth;
        (void)p_ancestors;
        return nullptr;
    }
    virtual int get_max_depth() const { return 4; }
    // Annulation des actions qui ne sont pas des écritures de valeur (liens de signaux...).
    virtual Ref<UndoBackend> get_undo_backend() const { return Ref<UndoBackend>(); }
    // Les liens de signaux en mode ressource ont changé : à réappliquer (en jeu).
    virtual void links_changed() {}
    // Base des interfaces par type (sous-objets dépliés) ; nulle : registres automatiques.
    virtual Ref<LayoutDatabase> get_layout_database() const { return Ref<LayoutDatabase>(); }
};
} // namespace godot

#endif // WIDGET_CONTEXT_HPP
