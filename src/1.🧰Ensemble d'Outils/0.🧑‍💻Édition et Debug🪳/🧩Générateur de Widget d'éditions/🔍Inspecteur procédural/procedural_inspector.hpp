#ifndef PROCEDURAL_INSPECTOR_HPP
#define PROCEDURAL_INSPECTOR_HPP

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot
{
/// 🔍 Panneau d'inspection procédural : génère une interface d'édition à partir des propriétés, méthodes et
/// signaux d'un objet en mémoire.
/// Hiérarchie créée par code (nœuds internes, non enregistrés) : PanelContainer → ScrollContainer → VBoxContainer.
///
/// Cible : `set_target()` (par code), sinon `target_resource`, sinon `target_path` (résolu depuis ce nœud).
/// Fonctionne aussi dans l'éditeur : les modifications marquent la scène comme non enregistrée, et les
/// connexions de signaux ajoutées sont persistantes (enregistrées avec la scène).
class ProceduralInspector : public Control
{
    GDCLASS(ProceduralInspector, Control)

  public:
    enum Scope
    {
        SCOPE_AUTO,
        SCOPE_SCRIPT_ONLY,
        SCOPE_ALL,
    };

    void _ready() override;

    void set_target(Object *p_target);
    Object *get_target() const;

    void set_target_path(const NodePath &p_path);
    NodePath get_target_path() const { return target_path; }

    void set_target_resource(const Ref<Resource> &p_resource);
    Ref<Resource> get_target_resource() const { return target_resource; }

    void set_scope(Scope p_scope);
    Scope get_scope() const { return scope; }

    void set_show_properties(bool p_show);
    bool get_show_properties() const { return show_properties; }

    void set_show_methods(bool p_show);
    bool get_show_methods() const { return show_methods; }

    void set_show_signals(bool p_show);
    bool get_show_signals() const { return show_signals; }

    /// Cible effective : set_target(), sinon target_resource, sinon target_path.
    Object *resolve_target() const;

    /// Régénère tous les widgets (à appeler si la structure de l'objet change).
    void rebuild();

    /// Relit immédiatement toutes les valeurs, sans attendre le prochain sondage.
    void refresh();

    /// 🔗 Liaison widget ⇄ valeur (InspectorBinding) pilotée par des Callable : point d'entrée utilisable
    /// depuis GDScript et depuis les autres modules GDExtension (qui ne peuvent pas appeler la version C++).
    /// `getter()` lit la valeur, `pull(valeur)` met le widget à jour, `setter(valeur)` (optionnel) écrit
    /// la valeur quand on appelle `commit(valeur)` sur la liaison retournée.
    /// La liaison s'arrête quand `getter` devient invalide (objet source libéré).
    static Node *bind_value(Control *p_owner, const Callable &p_getter, const Callable &p_pull, const Callable &p_setter = Callable());
    /// Force la relecture de toutes les liaisons sous `p_root`.
    static void refresh_bindings(Node *p_root);

  protected:
    static void _bind_methods();

  private:
    ObjectID target_id;
    NodePath target_path;
    Ref<Resource> target_resource;
    Scope scope = SCOPE_AUTO;
    bool show_properties = true;
    bool show_methods = true;
    bool show_signals = true;
    VBoxContainer *content = nullptr;
};

} // namespace godot

VARIANT_ENUM_CAST(ProceduralInspector::Scope);

#endif // PROCEDURAL_INSPECTOR_HPP
