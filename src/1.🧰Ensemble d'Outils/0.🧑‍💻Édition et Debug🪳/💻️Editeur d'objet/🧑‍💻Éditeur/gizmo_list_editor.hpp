#ifndef GIZMO_LIST_EDITOR_HPP
#define GIZMO_LIST_EDITOR_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🎯Gizmos/gizmo_registry.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔌Accès/undo_backend.hpp"

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
// Éditeur d'un GizmoRegistry (éditeur, port de GizmoListEditor C#) : une ligne par entrée (activée,
// fonction du catalogue, visibilité, redessin, ordre, suppression) puis, par paramètre (sauf un RID, fourni par
// l'hôte), sa source : constante (éditée par un ProceduralEditor avec le widget du type du paramètre),
// propriété ou fonction d'un objet (liste des membres compatibles de l'objet de contexte, ou saisie libre).
// Toute modification est une action d'annulation de l'éditeur.
class GizmoListEditor : public VBoxContainer
{
    GDCLASS(GizmoListEditor, VBoxContainer)

  private:
    Ref<GizmoRegistry> registry;
    Ref<UndoBackend> backend;
    VBoxContainer *list = nullptr; // enfant possédé
    bool refresh_queued = false;
    Array watched; // ressources dont on écoute "changed" (registre + entrées)
    // Objet dont on liste les membres compatibles (sources Propriété / Fonction) : objet réel s'il est donné,
    // sinon instance d'exemple de context_type, créée le temps d'une reconstruction.
    ObjectID context_id;
    String context_type;

    Ref<GizmoEntry> _entry(int64_t p_index) const;
    void _watch_all();
    void _unwatch_all();
    void _queue_refresh();
    void _commit(const String &p_name, const Array &p_do, const Array &p_undo);
    void _commit_set(Object *p_object, const StringName &p_property, const Variant &p_value);

    void _on_add();
    void _on_move(int64_t p_from, int64_t p_to);
    void _on_remove(int64_t p_index);
    void _on_enabled_toggled(bool p_on, int64_t p_index);
    void _on_function_selected(int64_t p_item, int64_t p_index, uint64_t p_option_id);
    void _on_visibility_selected(int64_t p_item, int64_t p_index);
    void _on_redraw_selected(int64_t p_item, int64_t p_index);
    void _on_source_selected(int64_t p_item, int64_t p_index, int64_t p_param, uint64_t p_option_id);
    void _on_member_selected(int64_t p_item, int64_t p_index, int64_t p_param, uint64_t p_option_id);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  public:
    GizmoListEditor();
    void set_registry(const Ref<GizmoRegistry> &p_registry);
    Ref<GizmoRegistry> get_registry() const;
    void set_context_object(Object *p_object);
    Object *get_context_object() const;
    void set_context_type(const String &p_type);
    String get_context_type() const;
    void refresh();
};
} // namespace godot

#endif // GIZMO_LIST_EDITOR_HPP
