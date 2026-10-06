#ifndef BEHAVIOR_STATE_MACHINE_HPP
#define BEHAVIOR_STATE_MACHINE_HPP

#include "state.hpp"

#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot
{
/// 🤖 Machine à états hiérarchique : racine d'un arbre de State.
///
/// Elle est elle-même un état composé, toujours actif pendant son exécution : son chemin actif va
/// d'elle jusqu'à l'état feuille (get_active_path). À chaque mise à jour :
///   1. _on_update(delta) de chaque état actif, de la racine vers la feuille ;
///   2. vérification des transitions automatiques (sans événement), de la feuille vers la racine :
///      la première remplie est franchie (une seule par mise à jour).
/// send_event(event) : chaque état actif, de la feuille vers la racine, peut le consommer (_on_event),
/// sinon ses transitions portant cet événement sont essayées.
/// Une transition demandée pendant une autre (dans _on_enter par exemple) est mise en file.
/// Placée sous un autre State, une StateMachine n'est qu'un état composé de la machine parente.
class StateMachine : public State
{
    GDCLASS(StateMachine, State)

  public:
    enum ProcessCallback
    {
        PROCESS_IDLE,    // transitions vérifiées dans _process
        PROCESS_PHYSICS, // transitions vérifiées dans _physics_process
        PROCESS_MANUAL,  // update(delta) / physics_update(delta) appelés par le code
    };

    void set_autostart(bool p_autostart) { autostart = p_autostart; }
    bool get_autostart() const { return autostart; }
    void set_process_callback(ProcessCallback p_callback);
    ProcessCallback get_process_callback() const { return process_callback; }
    void set_agent_path(const NodePath &p_path) { agent_path = p_path; }
    NodePath get_agent_path() const { return agent_path; }
    void set_blackboard(const Ref<Blackboard> &p_blackboard) { blackboard = p_blackboard; }
    Ref<Blackboard> get_blackboard() const;
    void set_history_size(int p_size) { history_size = MAX(0, p_size); }
    int get_history_size() const { return history_size; }

    void start();
    void stop();
    void restart();
    bool is_running() const { return running; }
    bool is_root_machine() const { return get_parent_state() == nullptr; }

    /// true si l'événement a été consommé par un état ou a déclenché une transition.
    bool send_event(const StringName &p_event, const Variant &p_data = Variant());
    /// Cible : un State, ou un chemin relatif à la machine. Franchie depuis l'état feuille actif.
    bool transition_to(const Variant &p_target);

    void update(double p_delta);
    void physics_update(double p_delta);

    /// État feuille actif (la machine elle-même si elle n'a aucun sous-état).
    State *get_active_state() const;
    TypedArray<State> get_active_path() const;
    Node *get_agent_node() const;
    double get_elapsed_time() const { return elapsed_time; }
    /// Transitions récentes : [{ "from": chemin, "to": chemin, "event": StringName, "time": secondes }]
    Array get_history() const { return history; }
    void clear_history() { history.clear(); }

    /// Description complète pour le graphe de comportement (structure + état actif).
    Dictionary get_debug_snapshot() const;

    PackedStringArray _get_configuration_warnings() const override;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    struct PendingTransition
    {
        ObjectID source;
        ObjectID target;
        StringName event;
    };

    bool autostart = true;
    ProcessCallback process_callback = PROCESS_IDLE;
    NodePath agent_path = NodePath("..");
    Ref<Blackboard> blackboard;
    int history_size = 32;

    bool running = false;
    bool in_transition = false;
    double elapsed_time = 0.0;
    Array history;
    LocalVector<PendingTransition> pending;

    void _update_processing();
    void _tick(double p_delta);
    void _evaluate_automatic_transitions();
    void _fire(State *p_source, State *p_target, const StringName &p_event);
    void _perform(State *p_source, State *p_target, const StringName &p_event);
    void _enter_initial_chain(State *p_from);
    bool _owns(const State *p_state) const;
    String _relative_path(const Node *p_node) const;
};
} // namespace godot

VARIANT_ENUM_CAST(StateMachine::ProcessCallback);

#endif // BEHAVIOR_STATE_MACHINE_HPP
