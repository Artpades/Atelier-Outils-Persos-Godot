#include "bt_leaves.hpp"

#include "../🤖HFSM/state_machine.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// ▶️ Action

PackedStringArray BTAction::_get_configuration_warnings() const
{
    PackedStringArray warnings = BTNode::_get_configuration_warnings();
    if (!_has_script_tick())
    {
        warnings.push_back(U"Ajoutez un script qui redéfinit _tick(delta) -> int (sinon l'action réussit immédiatement).");
    }
    return warnings;
}

// ❔ Condition

BTNode::Status BTCondition::_native_tick(double /*p_delta*/)
{
    bool ok = false;
    Variant result = evaluator.evaluate(expression, get_blackboard(), get_agent(), ok);
    return ok && result.booleanize() ? SUCCESS : FAILURE;
}

PackedStringArray BTCondition::_get_configuration_warnings() const
{
    PackedStringArray warnings = BTNode::_get_configuration_warnings();
    if (expression.strip_edges().is_empty() && !_has_script_tick())
    {
        warnings.push_back(U"Renseignez expression (ex. « vie < 20 ») ou redéfinissez _tick par script.");
    }
    return warnings;
}

void BTCondition::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_expression", "expression"), &BTCondition::set_expression);
    ClassDB::bind_method(D_METHOD("get_expression"), &BTCondition::get_expression);
    ClassDB::bind_method(D_METHOD("get_error"), &BTCondition::get_error);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "expression", PROPERTY_HINT_EXPRESSION), "set_expression", "get_expression");
}

// ⏸️ Attente

void BTWait::_native_enter()
{
    target = duration;
    if (random_deviation > 0.0)
    {
        target = MAX(0.0, duration + UtilityFunctions::randf_range(-random_deviation, random_deviation));
    }
}

BTNode::Status BTWait::_native_tick(double /*p_delta*/) { return get_running_time() >= target ? SUCCESS : RUNNING; }

String BTWait::get_debug_info() const
{
    if (is_running())
    {
        return String::num(MIN(get_running_time(), target), 1) + " / " + String::num(target, 1) + " s";
    }
    return String::num(duration, 1) + " s" + (random_deviation > 0.0 ? U" ±" + String::num(random_deviation, 1) : String());
}

void BTWait::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &BTWait::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &BTWait::get_duration);
    ClassDB::bind_method(D_METHOD("set_random_deviation", "deviation"), &BTWait::set_random_deviation);
    ClassDB::bind_method(D_METHOD("get_random_deviation"), &BTWait::get_random_deviation);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0,60,0.01,or_greater,suffix:s"), "set_duration", "get_duration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "random_deviation", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater,suffix:s"), "set_random_deviation", "get_random_deviation");
}

// 📝 Écriture dans le tableau noir

BTNode::Status BTSetValue::_native_tick(double /*p_delta*/)
{
    Ref<Blackboard> blackboard = get_blackboard();
    if (blackboard.is_null() || key.is_empty())
    {
        return FAILURE;
    }
    bool ok = false;
    Variant value = evaluator.evaluate(value_expression, blackboard, get_agent(), ok);
    if (!ok)
    {
        return FAILURE;
    }
    blackboard->set_value(key, value);
    return SUCCESS;
}

PackedStringArray BTSetValue::_get_configuration_warnings() const
{
    PackedStringArray warnings = BTNode::_get_configuration_warnings();
    if (key.is_empty())
    {
        warnings.push_back(U"Renseignez key (la clé du tableau noir à écrire).");
    }
    return warnings;
}

void BTSetValue::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_key", "key"), &BTSetValue::set_key);
    ClassDB::bind_method(D_METHOD("get_key"), &BTSetValue::get_key);
    ClassDB::bind_method(D_METHOD("set_value_expression", "expression"), &BTSetValue::set_value_expression);
    ClassDB::bind_method(D_METHOD("get_value_expression"), &BTSetValue::get_value_expression);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "key"), "set_key", "get_key");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "value_expression", PROPERTY_HINT_EXPRESSION), "set_value_expression", "get_value_expression");
}

// 📞 Appel de méthode

BTNode::Status BTCallMethod::_native_tick(double /*p_delta*/)
{
    Node *agent = get_agent();
    Node *target = agent && !node_path.is_empty() ? agent->get_node_or_null(node_path) : agent;
    if (!target || method.is_empty() || !target->has_method(method))
    {
        return FAILURE;
    }
    Variant result = target->callv(method, arguments);
    if (!result_key.is_empty())
    {
        Ref<Blackboard> blackboard = get_blackboard();
        if (blackboard.is_valid())
        {
            blackboard->set_value(result_key, result);
        }
    }
    if (use_result_as_status && result.get_type() == Variant::BOOL)
    {
        return bool(result) ? SUCCESS : FAILURE;
    }
    return SUCCESS;
}

void BTCallMethod::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_node_path", "path"), &BTCallMethod::set_node_path);
    ClassDB::bind_method(D_METHOD("get_node_path"), &BTCallMethod::get_node_path);
    ClassDB::bind_method(D_METHOD("set_method", "method"), &BTCallMethod::set_method);
    ClassDB::bind_method(D_METHOD("get_method"), &BTCallMethod::get_method);
    ClassDB::bind_method(D_METHOD("set_arguments", "arguments"), &BTCallMethod::set_arguments);
    ClassDB::bind_method(D_METHOD("get_arguments"), &BTCallMethod::get_arguments);
    ClassDB::bind_method(D_METHOD("set_result_key", "key"), &BTCallMethod::set_result_key);
    ClassDB::bind_method(D_METHOD("get_result_key"), &BTCallMethod::get_result_key);
    ClassDB::bind_method(D_METHOD("set_use_result_as_status", "use"), &BTCallMethod::set_use_result_as_status);
    ClassDB::bind_method(D_METHOD("get_use_result_as_status"), &BTCallMethod::get_use_result_as_status);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "node_path"), "set_node_path", "get_node_path");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "method"), "set_method", "get_method");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "arguments"), "set_arguments", "get_arguments");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "result_key"), "set_result_key", "get_result_key");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_result_as_status"), "set_use_result_as_status", "get_use_result_as_status");
}

// ⚡ Événement vers une StateMachine

BTNode::Status BTSendEvent::_native_tick(double /*p_delta*/)
{
    Node *agent = get_agent();
    if (!agent || event.is_empty())
    {
        return FAILURE;
    }
    StateMachine *machine = nullptr;
    if (!state_machine_path.is_empty())
    {
        machine = Object::cast_to<StateMachine>(agent->get_node_or_null(state_machine_path));
    }
    else
    {
        for (int i = 0; i < agent->get_child_count() && !machine; i++)
        {
            machine = Object::cast_to<StateMachine>(agent->get_child(i));
        }
    }
    if (!machine || !machine->is_running())
    {
        return FAILURE;
    }
    bool consumed = machine->send_event(event);
    return consumed || !require_consumed ? SUCCESS : FAILURE;
}

void BTSendEvent::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_state_machine_path", "path"), &BTSendEvent::set_state_machine_path);
    ClassDB::bind_method(D_METHOD("get_state_machine_path"), &BTSendEvent::get_state_machine_path);
    ClassDB::bind_method(D_METHOD("set_event", "event"), &BTSendEvent::set_event);
    ClassDB::bind_method(D_METHOD("get_event"), &BTSendEvent::get_event);
    ClassDB::bind_method(D_METHOD("set_require_consumed", "require"), &BTSendEvent::set_require_consumed);
    ClassDB::bind_method(D_METHOD("get_require_consumed"), &BTSendEvent::get_require_consumed);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "state_machine_path"), "set_state_machine_path", "get_state_machine_path");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "event"), "set_event", "get_event");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "require_consumed"), "set_require_consumed", "get_require_consumed");
}
