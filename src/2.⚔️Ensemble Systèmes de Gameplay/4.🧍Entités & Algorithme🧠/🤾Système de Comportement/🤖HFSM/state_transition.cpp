#include "state_transition.hpp"
#include "state.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void StateTransition::set_target_state(const NodePath &p_path)
{
    target_state = p_path;
    last_resolved_target = ObjectID(); // chemin choisi explicitement : plus de réparation vers l'ancienne cible
    emit_changed();
}

void StateTransition::set_event(const StringName &p_event)
{
    event = p_event;
    emit_changed();
}

void StateTransition::set_condition(const String &p_condition)
{
    condition = p_condition;
    emit_changed();
}

void StateTransition::set_min_time_in_state(double p_time)
{
    min_time_in_state = MAX(0.0, p_time);
    emit_changed();
}

void StateTransition::set_enabled(bool p_enabled)
{
    enabled = p_enabled;
    emit_changed();
}

State *StateTransition::resolve_target(const State *p_source) const
{
    if (!p_source || target_state.is_empty())
    {
        return nullptr;
    }
    State *target = Object::cast_to<State>(p_source->get_node_or_null(target_state));
    if (target)
    {
        last_resolved_target = ObjectID(target->get_instance_id());
    }
    return target;
}

State *StateTransition::get_last_resolved_target() const { return Object::cast_to<State>(ObjectDB::get_instance(last_resolved_target)); }

bool StateTransition::is_satisfied(State *p_source)
{
    if (!enabled || !p_source)
    {
        return false;
    }
    if (p_source->get_time_in_state() < min_time_in_state)
    {
        return false;
    }
    if (!condition.strip_edges().is_empty())
    {
        bool ok = false;
        Variant result = expression.evaluate(condition, p_source->get_blackboard(), p_source->get_agent(), ok);
        if (!ok || !result.booleanize())
        {
            return false;
        }
    }
    bool scripted = true;
    if (GDVIRTUAL_CALL(_check, p_source, scripted) && !scripted)
    {
        return false;
    }
    return true;
}

String StateTransition::get_label() const
{
    PackedStringArray parts;
    if (!event.is_empty())
    {
        parts.push_back(U"⚡" + String(event));
    }
    if (!condition.strip_edges().is_empty())
    {
        parts.push_back("[" + condition.strip_edges() + "]");
    }
    if (min_time_in_state > 0.0)
    {
        parts.push_back(U"⏱" + String::num(min_time_in_state, 2) + " s");
    }
    if (GDVIRTUAL_IS_OVERRIDDEN(_check))
    {
        parts.push_back("_check()");
    }
    if (!enabled)
    {
        parts.push_back(U"(désactivée)");
    }
    return String(" ").join(parts);
}

// 🔗 Liaisons

void StateTransition::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_target_state", "path"), &StateTransition::set_target_state);
    ClassDB::bind_method(D_METHOD("get_target_state"), &StateTransition::get_target_state);
    ClassDB::bind_method(D_METHOD("set_event", "event"), &StateTransition::set_event);
    ClassDB::bind_method(D_METHOD("get_event"), &StateTransition::get_event);
    ClassDB::bind_method(D_METHOD("set_condition", "condition"), &StateTransition::set_condition);
    ClassDB::bind_method(D_METHOD("get_condition"), &StateTransition::get_condition);
    ClassDB::bind_method(D_METHOD("set_min_time_in_state", "time"), &StateTransition::set_min_time_in_state);
    ClassDB::bind_method(D_METHOD("get_min_time_in_state"), &StateTransition::get_min_time_in_state);
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &StateTransition::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &StateTransition::is_enabled);
    ClassDB::bind_method(D_METHOD("is_automatic"), &StateTransition::is_automatic);
    ClassDB::bind_method(D_METHOD("is_satisfied", "source"), &StateTransition::is_satisfied);
    ClassDB::bind_method(D_METHOD("resolve_target", "source"), &StateTransition::resolve_target);
    ClassDB::bind_method(D_METHOD("get_label"), &StateTransition::get_label);
    ClassDB::bind_method(D_METHOD("get_condition_error"), &StateTransition::get_condition_error);

    GDVIRTUAL_BIND(_check, "state");

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_state", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "State"), "set_target_state", "get_target_state");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "event"), "set_event", "get_event");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition", PROPERTY_HINT_EXPRESSION), "set_condition", "get_condition");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_time_in_state", PROPERTY_HINT_RANGE, "0,60,0.01,or_greater,suffix:s"), "set_min_time_in_state", "get_min_time_in_state");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
}
