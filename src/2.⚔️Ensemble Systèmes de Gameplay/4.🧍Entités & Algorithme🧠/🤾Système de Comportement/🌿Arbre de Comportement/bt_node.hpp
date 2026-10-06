#ifndef BT_NODE_HPP
#define BT_NODE_HPP

#include "../🧩Commun/blackboard.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
class BehaviorTree;

/// 🌿 Nœud d'arbre de comportement (classe de base, non instanciable seule).
///
/// execute(delta) : _on_enter() au premier passage, puis _tick(delta) qui renvoie SUCCESS, FAILURE ou
/// RUNNING ; _on_exit() quand il ne renvoie plus RUNNING ou qu'il est interrompu (abort).
/// Par script (extends BTAction, BTCondition...) : redéfinir _tick(delta) -> int, et si besoin
/// _on_enter() / _on_exit(). Seuls les enfants BTNode participent à l'arbre.
class BTNode : public Node
{
    GDCLASS(BTNode, Node)

  public:
    enum Status
    {
        SUCCESS,
        FAILURE,
        RUNNING,
    };

    /// Exécute ce nœud pour un tick de l'arbre.
    Status execute(double p_delta);
    /// Interrompt ce nœud s'il est en cours (et ses enfants en cours) : _on_exit() est appelé.
    void abort();

    BehaviorTree *get_behavior_tree() const;
    Node *get_agent() const;
    Ref<Blackboard> get_blackboard() const;

    TypedArray<BTNode> get_bt_children() const;
    int get_bt_child_count() const;
    BTNode *get_bt_child(int p_index) const;

    /// Dernier statut renvoyé (-1 : jamais exécuté).
    int get_last_status() const { return last_status; }
    bool is_running() const { return running; }
    /// Temps passé depuis _on_enter (remis à zéro à chaque nouvelle exécution).
    double get_running_time() const { return running_time; }
    /// Numéro du tick de l'arbre lors de la dernière exécution (pour savoir si ce nœud vient de s'exécuter).
    uint64_t get_last_tick() const { return last_tick; }

    /// Catégorie affichée dans le graphe : composite, decorator, action, condition.
    virtual String get_debug_kind() const { return "action"; }
    /// Détail affiché dans le graphe (compteur, durée restante...).
    virtual String get_debug_info() const { return String(); }

    PackedStringArray _get_configuration_warnings() const override;

  protected:
    static void _bind_methods();

    GDVIRTUAL0(_on_enter)
    GDVIRTUAL0(_on_exit)
    GDVIRTUAL1R(int, _tick, double)

    /// Comportement natif des classes C++ (utilisé si aucun script ne redéfinit _tick).
    virtual Status _native_tick(double /*p_delta*/) { return SUCCESS; }
    virtual void _native_enter() {}
    virtual void _native_exit() {}
    /// Interrompt les enfants en cours (composites et décorateurs).
    void _abort_running_children(int p_from_index = 0);
    /// Temps de l'arbre (secondes cumulées par ses ticks).
    double _tree_time() const;
    bool _has_script_tick() const;

  private:
    bool running = false;
    int last_status = -1;
    double running_time = 0.0;
    uint64_t last_tick = 0;
};
} // namespace godot

VARIANT_ENUM_CAST(BTNode::Status);

#endif // BT_NODE_HPP
