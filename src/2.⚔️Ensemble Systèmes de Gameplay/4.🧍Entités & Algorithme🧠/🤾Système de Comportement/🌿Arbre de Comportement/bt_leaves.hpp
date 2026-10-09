#ifndef BT_LEAVES_HPP
#define BT_LEAVES_HPP

#include "bt_node.hpp"

#include "../🧩Commun/behavior_expression.hpp"

#include <godot_cpp/variant/array.hpp>

namespace godot
{
/// ▶️ Action : feuille dont le comportement est écrit par script (extends BTAction, redéfinir _tick).
/// Sans script, réussit immédiatement.
class BTAction : public BTNode
{
    GDCLASS(BTAction, BTNode)

  public:
    PackedStringArray _get_configuration_warnings() const override;

  protected:
    static void _bind_methods() {}
};

/// ❔ Condition : SUCCESS si expression est vraie, FAILURE sinon (ou si elle est invalide).
/// Par script : redéfinir _tick pour une condition codée (l'expression est alors ignorée).
class BTCondition : public BTNode
{
    GDCLASS(BTCondition, BTNode)

  public:
    void set_expression(const String &p_expression) { expression = p_expression; }
    String get_expression() const { return expression; }
    String get_error() const { return evaluator.get_error(); }

    String get_debug_kind() const override { return "condition"; }
    String get_debug_info() const override { return expression; }
    PackedStringArray _get_configuration_warnings() const override;

  protected:
    static void _bind_methods();
    Status _native_tick(double p_delta) override;

  private:
    String expression;
    BehaviorExpression evaluator;
};

/// ⏸️ Attente : RUNNING pendant duration secondes (± random_deviation), puis SUCCESS.
class BTWait : public BTNode
{
    GDCLASS(BTWait, BTNode)

  public:
    void set_duration(double p_duration) { duration = MAX(0.0, p_duration); }
    double get_duration() const { return duration; }
    void set_random_deviation(double p_deviation) { random_deviation = MAX(0.0, p_deviation); }
    double get_random_deviation() const { return random_deviation; }
    String get_debug_info() const override;

  protected:
    static void _bind_methods();
    void _native_enter() override;
    Status _native_tick(double p_delta) override;

  private:
    double duration = 1.0;
    double random_deviation = 0.0;
    /// Durée réellement attendue pour l'exécution en cours (duration ± random_deviation).
    double target = 1.0;
};

/// 📝 Écrit dans le tableau noir : key = valeur de l'expression value. SUCCESS, ou FAILURE si l'expression échoue.
class BTSetValue : public BTNode
{
    GDCLASS(BTSetValue, BTNode)

  public:
    void set_key(const String &p_key) { key = p_key; }
    String get_key() const { return key; }
    void set_value_expression(const String &p_expression) { value_expression = p_expression; }
    String get_value_expression() const { return value_expression; }
    String get_debug_info() const override { return key + String(" = ") + value_expression; }
    PackedStringArray _get_configuration_warnings() const override;

  protected:
    static void _bind_methods();
    Status _native_tick(double p_delta) override;

  private:
    String key;
    String value_expression;
    BehaviorExpression evaluator;
};

/// 📞 Appelle une méthode d'un nœud (chemin relatif à l'agent, vide = l'agent) avec des arguments fixes.
/// Le résultat peut être rangé dans le tableau noir (result_key) et, s'il est booléen, servir de statut
/// (use_result_as_status). FAILURE si le nœud ou la méthode est introuvable.
class BTCallMethod : public BTNode
{
    GDCLASS(BTCallMethod, BTNode)

  public:
    void set_node_path(const NodePath &p_path) { node_path = p_path; }
    NodePath get_node_path() const { return node_path; }
    void set_method(const StringName &p_method) { method = p_method; }
    StringName get_method() const { return method; }
    void set_arguments(const Array &p_arguments) { arguments = p_arguments; }
    Array get_arguments() const { return arguments; }
    void set_result_key(const String &p_key) { result_key = p_key; }
    String get_result_key() const { return result_key; }
    void set_use_result_as_status(bool p_use) { use_result_as_status = p_use; }
    bool get_use_result_as_status() const { return use_result_as_status; }
    String get_debug_info() const override { return String(method) + "()"; }

  protected:
    static void _bind_methods();
    Status _native_tick(double p_delta) override;

  private:
    NodePath node_path;
    StringName method;
    Array arguments;
    String result_key;
    bool use_result_as_status = false;
};

/// ⚡ Envoie un événement à une StateMachine (passerelle arbre → HFSM). state_machine_path est relatif à
/// l'agent ; vide : la première StateMachine enfant de l'agent. SUCCESS si l'événement a été consommé
/// (ou si require_consumed est faux et la machine tourne), FAILURE sinon.
class BTSendEvent : public BTNode
{
    GDCLASS(BTSendEvent, BTNode)

  public:
    void set_state_machine_path(const NodePath &p_path) { state_machine_path = p_path; }
    NodePath get_state_machine_path() const { return state_machine_path; }
    void set_event(const StringName &p_event) { event = p_event; }
    StringName get_event() const { return event; }
    void set_require_consumed(bool p_require) { require_consumed = p_require; }
    bool get_require_consumed() const { return require_consumed; }
    String get_debug_info() const override { return U"⚡" + String(event); }

  protected:
    static void _bind_methods();
    Status _native_tick(double p_delta) override;

  private:
    NodePath state_machine_path;
    StringName event;
    bool require_consumed = false;
};
} // namespace godot

#endif // BT_LEAVES_HPP
