#include "bt_decorators.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// ◇ Décorateur

PackedStringArray BTDecorator::_get_configuration_warnings() const
{
    PackedStringArray warnings = BTNode::_get_configuration_warnings();
    int count = get_bt_child_count();
    if (count == 0)
    {
        warnings.push_back(U"Un décorateur a besoin d'un BTNode enfant.");
    }
    else if (count > 1)
    {
        warnings.push_back(U"Un décorateur n'utilise que son premier BTNode enfant.");
    }
    return warnings;
}

BTNode::Status BTDecorator::_native_tick(double p_delta)
{
    BTNode *child = get_child_node();
    return child ? child->execute(p_delta) : FAILURE;
}

void BTDecorator::_bind_methods() { ClassDB::bind_method(D_METHOD("get_child_node"), &BTDecorator::get_child_node); }

// 🔁 Inverseur

BTNode::Status BTInverter::_native_tick(double p_delta)
{
    BTNode *child = get_child_node();
    if (!child)
    {
        return FAILURE;
    }
    Status status = child->execute(p_delta);
    return status == RUNNING ? RUNNING : (status == SUCCESS ? FAILURE : SUCCESS);
}

// ✅ Résultat forcé

BTNode::Status BTForceResult::_native_tick(double p_delta)
{
    BTNode *child = get_child_node();
    if (child && child->execute(p_delta) == RUNNING)
    {
        return RUNNING;
    }
    return result;
}

void BTForceResult::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_result", "result"), &BTForceResult::set_result);
    ClassDB::bind_method(D_METHOD("get_result"), &BTForceResult::get_result);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "result", PROPERTY_HINT_ENUM, "Success,Failure"), "set_result", "get_result");
}

// 🔂 Répéteur

BTNode::Status BTRepeater::_native_tick(double p_delta)
{
    BTNode *child = get_child_node();
    if (!child)
    {
        return FAILURE;
    }
    Status status = child->execute(p_delta);
    if (status == RUNNING)
    {
        return RUNNING;
    }
    if ((mode == REPEAT_UNTIL_SUCCESS && status == SUCCESS) || (mode == REPEAT_UNTIL_FAILURE && status == FAILURE))
    {
        return SUCCESS;
    }
    iteration++;
    if (count > 0 && iteration >= count)
    {
        return mode == REPEAT_ALWAYS ? SUCCESS : FAILURE;
    }
    return RUNNING; // relancé au prochain tick
}

String BTRepeater::get_debug_info() const
{
    const char32_t *modes[] = {U"", U" jusqu'à réussite", U" jusqu'à échec"};
    return String::num_int64(iteration) + "/" + (count > 0 ? String::num_int64(count) : String(U"∞")) + String(modes[mode]);
}

void BTRepeater::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_count", "count"), &BTRepeater::set_count);
    ClassDB::bind_method(D_METHOD("get_count"), &BTRepeater::get_count);
    ClassDB::bind_method(D_METHOD("set_mode", "mode"), &BTRepeater::set_mode);
    ClassDB::bind_method(D_METHOD("get_mode"), &BTRepeater::get_mode);
    ClassDB::bind_method(D_METHOD("get_iteration"), &BTRepeater::get_iteration);

    BIND_ENUM_CONSTANT(REPEAT_ALWAYS);
    BIND_ENUM_CONSTANT(REPEAT_UNTIL_SUCCESS);
    BIND_ENUM_CONSTANT(REPEAT_UNTIL_FAILURE);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "count", PROPERTY_HINT_RANGE, "0,100,1,or_greater"), "set_count", "get_count");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Always,Until Success,Until Failure"), "set_mode", "get_mode");
}

// ⏳ Temps de recharge

double BTCooldown::get_remaining() const { return ready_time < 0.0 ? 0.0 : MAX(0.0, ready_time - _tree_time()); }

BTNode::Status BTCooldown::_native_tick(double p_delta)
{
    if (get_remaining() > 0.0)
    {
        return FAILURE;
    }
    BTNode *child = get_child_node();
    if (!child)
    {
        return FAILURE;
    }
    Status status = child->execute(p_delta);
    if (status == SUCCESS || (status == FAILURE && include_failures))
    {
        ready_time = _tree_time() + duration;
    }
    return status;
}

String BTCooldown::get_debug_info() const
{
    double remaining = get_remaining();
    return remaining > 0.0 ? U"recharge " + String::num(remaining, 1) + " s" : String(U"prêt");
}

void BTCooldown::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &BTCooldown::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &BTCooldown::get_duration);
    ClassDB::bind_method(D_METHOD("set_include_failures", "include"), &BTCooldown::set_include_failures);
    ClassDB::bind_method(D_METHOD("get_include_failures"), &BTCooldown::get_include_failures);
    ClassDB::bind_method(D_METHOD("get_remaining"), &BTCooldown::get_remaining);
    ClassDB::bind_method(D_METHOD("reset_cooldown"), &BTCooldown::reset_cooldown);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0,60,0.01,or_greater,suffix:s"), "set_duration", "get_duration");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "include_failures"), "set_include_failures", "get_include_failures");
}

// ⏱️ Limite de temps

BTNode::Status BTTimeLimit::_native_tick(double p_delta)
{
    BTNode *child = get_child_node();
    if (!child)
    {
        return FAILURE;
    }
    Status status = child->execute(p_delta);
    if (status == RUNNING && get_running_time() >= duration)
    {
        child->abort();
        return FAILURE;
    }
    return status;
}

String BTTimeLimit::get_debug_info() const
{
    return is_running() ? String::num(MAX(0.0, duration - get_running_time()), 1) + " s" : String::num(duration, 1) + " s max";
}

void BTTimeLimit::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &BTTimeLimit::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &BTTimeLimit::get_duration);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0,60,0.01,or_greater,suffix:s"), "set_duration", "get_duration");
}
