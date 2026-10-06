#ifndef STATE_TRANSITION_HPP
#define STATE_TRANSITION_HPP

#include "../🧩Commun/behavior_expression.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot
{
class State;

/// ➡️ Transition d'un State vers un autre état de la même StateMachine.
///
/// - Avec un événement (event) : franchie quand StateMachine.send_event(event) est appelé alors que
///   l'état source est actif. Sans événement : vérifiée automatiquement à chaque mise à jour.
/// - condition : expression évaluée avec le tableau noir (ex. « distance < 3 and not blesse »), vide = vraie.
/// - min_time_in_state : durée minimale passée dans l'état source avant de pouvoir partir.
/// - Script : redéfinir _check(state) -> bool pour une condition codée (combinée avec les autres par « et »).
/// target_state est relatif à l'état source (ex. « ../Poursuite »).
class StateTransition : public Resource
{
    GDCLASS(StateTransition, Resource)

  public:
    void set_target_state(const NodePath &p_path);
    NodePath get_target_state() const { return target_state; }
    void set_event(const StringName &p_event);
    StringName get_event() const { return event; }
    void set_condition(const String &p_condition);
    String get_condition() const { return condition; }
    void set_min_time_in_state(double p_time);
    double get_min_time_in_state() const { return min_time_in_state; }
    void set_enabled(bool p_enabled);
    bool is_enabled() const { return enabled; }

    bool is_automatic() const { return event.is_empty(); }
    /// État cible résolu depuis l'état source (nullptr si introuvable ou si ce n'est pas un State).
    State *resolve_target(const State *p_source) const;
    /// Toutes les conditions (activée, durée minimale, expression, script) sont-elles remplies ?
    bool is_satisfied(State *p_source);
    /// Texte court pour le graphe : « événement [condition] ⏱ durée ».
    String get_label() const;
    /// Dernière erreur de l'expression (vide si aucune).
    String get_condition_error() const { return expression.get_error(); }
    /// Dernière cible résolue avec succès (même si target_state ne la désigne plus, après un renommage
    /// ou un déplacement) : permet à l'éditeur de réparer le chemin.
    State *get_last_resolved_target() const;

  protected:
    static void _bind_methods();

    GDVIRTUAL1R(bool, _check, Node *)

  private:
    NodePath target_state;
    StringName event;
    String condition;
    double min_time_in_state = 0.0;
    bool enabled = true;

    BehaviorExpression expression; // cache d'analyse de la condition
    mutable ObjectID last_resolved_target;
};
} // namespace godot

#endif // STATE_TRANSITION_HPP
