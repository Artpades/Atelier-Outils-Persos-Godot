#ifndef BEHAVIOR_LIST_EDITOR_HPP
#define BEHAVIOR_LIST_EDITOR_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/data_registry.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔌Accès/undo_backend.hpp"

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// Onglet « Comportements » (éditeur) : liste des WidgetBehavior d'un DataRegistry. Par comportement :
/// activé, type, inversion, ordre, suppression ; condition (propriété ou fonction booléenne : liste des membres
/// compatibles de l'objet de contexte, ou saisie libre) ; cibles (données et groupes) ; réglages propres
/// (style, infobulle BBCode, icône ; propriétés exportées d'un comportement de script).
/// « + Comportement » propose les comportements fournis et les class_name qui héritent de WidgetBehavior.
/// Toute modification est une action d'annulation de l'éditeur ; émet "registry_changed".
class BehaviorListEditor : public VBoxContainer
{
    GDCLASS(BehaviorListEditor, VBoxContainer)

  private:
    Ref<DataRegistry> registry;
    Ref<GroupRegistry> groups;
    Ref<UndoBackend> backend;
    ObjectID context_id;
    String context_type;
    VBoxContainer *list = nullptr; // enfant possédé
    bool refresh_queued = false;

    Ref<WidgetBehavior> _behavior(int64_t p_index) const;
    void _queue_refresh();
    void _commit(const String &p_name, const Array &p_do, const Array &p_undo);
    void _commit_set(Object *p_object, const StringName &p_property, const Variant &p_value);
    void _set_behaviors(const TypedArray<WidgetBehavior> &p_behaviors, const String &p_name);
    Object *_resolve_context(Variant &r_sample) const;

    void _on_add_id(int64_t p_id, uint64_t p_menu_id);
    void _on_move(int64_t p_from, int64_t p_to);
    void _on_remove(int64_t p_index);
    void _on_toggle(bool p_on, int64_t p_index, const String &p_property);
    void _on_source_selected(int64_t p_item, int64_t p_index, uint64_t p_option_id);
    void _on_member_selected(int64_t p_item, int64_t p_index, uint64_t p_option_id);
    void _on_target_added(int64_t p_item, int64_t p_index, uint64_t p_option_id);
    void _on_target_removed(const String &p_name, int64_t p_index);
    void _on_tooltip_committed(int64_t p_index, uint64_t p_edit_id);
    void _on_child_changed();

  protected:
    static void _bind_methods();

  public:
    BehaviorListEditor();
    /// Comportements fournis : nom de classe -> nom affiché.
    static Dictionary builtin_types();
    void set_registries(const Ref<DataRegistry> &p_registry, const Ref<GroupRegistry> &p_groups);
    void set_context_object(Object *p_object);
    void set_context_type(const String &p_type);
    void refresh();
};
} // namespace godot

#endif // BEHAVIOR_LIST_EDITOR_HPP
