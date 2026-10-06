#ifndef BEHAVIOR_STATE_HPP
#define BEHAVIOR_STATE_HPP

#include "state_transition.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
class StateMachine;

/// 🤖 État d'une machine à états hiérarchique (HFSM). Les State enfants sont ses sous-états : un état
/// qui en a est « composé », et un seul de ses sous-états est actif à la fois.
///
/// Comportement par script (extends State), toutes facultatives :
///   _on_enter(), _on_exit(), _on_update(delta), _on_physics_update(delta),
///   _on_event(event, data) -> bool  (true = événement consommé, il ne remonte pas aux parents).
/// Les transitions (StateTransition) sont portées par l'état source.
class State : public Node
{
    GDCLASS(State, Node)

  public:
    void set_transitions(const TypedArray<StateTransition> &p_transitions);
    TypedArray<StateTransition> get_transitions() const { return transitions; }
    void set_initial_state(const NodePath &p_path);
    NodePath get_initial_state() const { return initial_state; }
    void set_remember_last_substate(bool p_remember) { remember_last_substate = p_remember; }
    bool get_remember_last_substate() const { return remember_last_substate; }

    // 🧭 Hiérarchie
    /// État parent (nullptr pour la racine).
    State *get_parent_state() const;
    /// StateMachine racine de cet état (lui-même s'il en est une sans parent).
    StateMachine *get_state_machine() const;
    TypedArray<State> get_substates() const;
    bool is_compound() const;
    /// Sous-état d'entrée : le dernier actif (remember_last_substate), sinon initial_state, sinon le premier.
    State *get_initial_substate() const;
    /// Dernier état désigné avec succès par initial_state (pour réparer le chemin après un renommage).
    State *get_last_resolved_initial_state() const { return Object::cast_to<State>(ObjectDB::get_instance(last_resolved_initial)); }
    State *get_active_substate() const;
    bool is_active() const { return active; }
    double get_time_in_state() const { return time_in_state; }

    // 🔌 Accès
    Node *get_agent() const;
    Ref<Blackboard> get_blackboard() const;
    /// Raccourcis vers la StateMachine racine.
    bool send_event(const StringName &p_event, const Variant &p_data = Variant());
    bool transition_to(const Variant &p_target);

    PackedStringArray _get_configuration_warnings() const override;

    // ⚙️ Interne : appelé par la StateMachine
    void _internal_enter();
    void _internal_exit();
    void _internal_update(double p_delta);
    void _internal_physics_update(double p_delta);
    bool _internal_event(const StringName &p_event, const Variant &p_data);
    void _internal_forget_history() { last_substate_id = ObjectID(); }

  protected:
    static void _bind_methods();

    GDVIRTUAL0(_on_enter)
    GDVIRTUAL0(_on_exit)
    GDVIRTUAL1(_on_update, double)
    GDVIRTUAL1(_on_physics_update, double)
    GDVIRTUAL2R(bool, _on_event, StringName, Variant)

    bool active = false;
    double time_in_state = 0.0;
    ObjectID active_substate_id;
    ObjectID last_substate_id; // historique (remember_last_substate)

  private:
    TypedArray<StateTransition> transitions;
    NodePath initial_state;
    bool remember_last_substate = false;
    mutable ObjectID last_resolved_initial;
};
} // namespace godot

#endif // BEHAVIOR_STATE_HPP
