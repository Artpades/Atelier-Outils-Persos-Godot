#ifndef BEHAVIOR_TREE_HPP
#define BEHAVIOR_TREE_HPP

#include "bt_node.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot
{
/// 🌿 Exécuteur d'arbre de comportement : son premier enfant BTNode est la racine de l'arbre.
/// À chaque tick (toutes les images, ou tous les tick_interval secondes), la racine est exécutée ;
/// quand elle renvoie SUCCESS ou FAILURE, l'arbre repart du début au tick suivant (signal finished).
class BehaviorTree : public Node
{
    GDCLASS(BehaviorTree, Node)

  public:
    enum ProcessCallback
    {
        PROCESS_IDLE,
        PROCESS_PHYSICS,
        PROCESS_MANUAL, // tick(delta) appelé par le code
    };

    void set_active(bool p_active);
    bool is_active() const { return active; }
    void set_process_callback(ProcessCallback p_callback);
    ProcessCallback get_process_callback() const { return process_callback; }
    void set_tick_interval(double p_interval) { tick_interval = MAX(0.0, p_interval); }
    double get_tick_interval() const { return tick_interval; }
    void set_agent_path(const NodePath &p_path) { agent_path = p_path; }
    NodePath get_agent_path() const { return agent_path; }
    void set_blackboard(const Ref<Blackboard> &p_blackboard) { blackboard = p_blackboard; }
    Ref<Blackboard> get_blackboard() const { return blackboard; }

    /// Exécute la racine une fois et renvoie son statut (-1 : pas de racine).
    int tick(double p_delta);
    /// Interrompt la branche en cours : le prochain tick repart de la racine.
    void interrupt();

    BTNode *get_root_bt_node() const;
    Node *get_agent_node() const;
    int get_last_status() const { return last_status; }
    uint64_t get_tick_count() const { return tick_count; }
    double get_elapsed_time() const { return elapsed_time; }

    /// Description complète pour le graphe de comportement (structure + statuts du dernier tick).
    Dictionary get_debug_snapshot() const;

    PackedStringArray _get_configuration_warnings() const override;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    bool active = true;
    ProcessCallback process_callback = PROCESS_IDLE;
    double tick_interval = 0.0;
    NodePath agent_path = NodePath("..");
    Ref<Blackboard> blackboard;

    int last_status = -1;
    uint64_t tick_count = 0;
    double elapsed_time = 0.0;
    double interval_accumulator = 0.0;

    void _update_processing();
    void _advance(double p_delta);
};
} // namespace godot

VARIANT_ENUM_CAST(BehaviorTree::ProcessCallback);

#endif // BEHAVIOR_TREE_HPP
