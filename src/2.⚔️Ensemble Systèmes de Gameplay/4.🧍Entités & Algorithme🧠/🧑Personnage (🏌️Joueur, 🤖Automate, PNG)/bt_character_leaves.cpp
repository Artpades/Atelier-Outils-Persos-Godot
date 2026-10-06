#include "bt_character_leaves.hpp"
#include "character_controllers.hpp"
#include "character_registry.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// 🎯 BTFindTarget

BTNode::Status BTFindTarget::_native_tick(double /*p_delta*/)
{
    Ref<Blackboard> blackboard = get_blackboard();
    Node *target = CharacterRegister::get_nearest(get_agent(), max_distance, hostile_only);
    if (blackboard.is_valid())
    {
        if (target)
        {
            blackboard->set_value(key, target);
        }
        else
        {
            blackboard->erase_value(key);
        }
    }
    return target ? SUCCESS : FAILURE;
}

String BTFindTarget::get_debug_info() const
{
    return key + (max_distance >= 0.0 ? String(U" ≤ ") + String::num(max_distance, 1) : String()) + (hostile_only ? String(U" · hostile") : String());
}

void BTFindTarget::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_key", "key"), &BTFindTarget::set_key);
    ClassDB::bind_method(D_METHOD("get_key"), &BTFindTarget::get_key);
    ClassDB::bind_method(D_METHOD("set_max_distance", "distance"), &BTFindTarget::set_max_distance);
    ClassDB::bind_method(D_METHOD("get_max_distance"), &BTFindTarget::get_max_distance);
    ClassDB::bind_method(D_METHOD("set_hostile_only", "hostile"), &BTFindTarget::set_hostile_only);
    ClassDB::bind_method(D_METHOD("get_hostile_only"), &BTFindTarget::get_hostile_only);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "key"), "set_key", "get_key");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "-1,5000,0.1,or_greater"), "set_max_distance", "get_max_distance");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hostile_only"), "set_hostile_only", "get_hostile_only");
}

// 🚶 BTMoveTo

AutomateController *BTMoveTo::_controller() const
{
    Node *agent = get_agent();
    for (int i = 0; agent && i < agent->get_child_count(); i++)
    {
        if (AutomateController *controller = Object::cast_to<AutomateController>(agent->get_child(i)))
        {
            return controller;
        }
    }
    return nullptr;
}

BTNode::Status BTMoveTo::_native_tick(double /*p_delta*/)
{
    AutomateController *controller = _controller();
    CharacterAPI *character = character_api_of(get_agent());
    Ref<Blackboard> blackboard = get_blackboard();
    if (!controller || !character || blackboard.is_null() || !blackboard->has_value(target_key))
    {
        return FAILURE;
    }
    Variant target = blackboard->get_value(target_key);
    if (target.get_type() == Variant::OBJECT && !Object::cast_to<Node>(target))
    {
        return FAILURE; // nœud libéré
    }
    Vector3 point = target.get_type() == Variant::OBJECT ? node_world_position(Object::cast_to<Node>(target))
                    : target.get_type() == Variant::VECTOR2 ? Vector3(Vector2(target).x, Vector2(target).y, 0.0f)
                                                            : Vector3(target);
    double stop = arrive_distance >= 0.0 ? arrive_distance : controller->get_effective_arrive_distance();
    if (character->planar_distance_to(point) <= stop)
    {
        controller->stop();
        return SUCCESS;
    }
    controller->move_to(point);
    character->set_running(run);
    return RUNNING;
}

void BTMoveTo::_native_exit()
{
    // Terminé ou interrompu : l'agent s'arrête (s'il exécutait encore notre déplacement).
    AutomateController *controller = _controller();
    if (controller && controller->get_mode() == AutomateController::MODE_MOVE_TO)
    {
        controller->stop();
    }
}

void BTMoveTo::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_target_key", "key"), &BTMoveTo::set_target_key);
    ClassDB::bind_method(D_METHOD("get_target_key"), &BTMoveTo::get_target_key);
    ClassDB::bind_method(D_METHOD("set_arrive_distance", "distance"), &BTMoveTo::set_arrive_distance);
    ClassDB::bind_method(D_METHOD("get_arrive_distance"), &BTMoveTo::get_arrive_distance);
    ClassDB::bind_method(D_METHOD("set_run", "run"), &BTMoveTo::set_run);
    ClassDB::bind_method(D_METHOD("get_run"), &BTMoveTo::get_run);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "target_key"), "set_target_key", "get_target_key");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "arrive_distance", PROPERTY_HINT_RANGE, "-1,1000,0.01,or_greater"), "set_arrive_distance", "get_arrive_distance");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "run"), "set_run", "get_run");
}

// 🌟 BTUseAbility

BTNode::Status BTUseAbility::_native_tick(double /*p_delta*/)
{
    AbilitySystem *system = AbilitySystem::find_on(get_agent());
    if (!system)
    {
        return FAILURE;
    }
    if (!started)
    {
        started = true;
        Node *target = nullptr;
        Ref<Blackboard> blackboard = get_blackboard();
        if (!target_key.is_empty() && blackboard.is_valid())
        {
            target = Object::cast_to<Node>(blackboard->get_value(target_key));
        }
        if (!system->activate(ability_id, target))
        {
            return FAILURE;
        }
    }
    return wait_until_end && system->is_ability_active(ability_id) ? RUNNING : SUCCESS;
}

void BTUseAbility::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_ability_id", "id"), &BTUseAbility::set_ability_id);
    ClassDB::bind_method(D_METHOD("get_ability_id"), &BTUseAbility::get_ability_id);
    ClassDB::bind_method(D_METHOD("set_target_key", "key"), &BTUseAbility::set_target_key);
    ClassDB::bind_method(D_METHOD("get_target_key"), &BTUseAbility::get_target_key);
    ClassDB::bind_method(D_METHOD("set_wait_until_end", "wait"), &BTUseAbility::set_wait_until_end);
    ClassDB::bind_method(D_METHOD("get_wait_until_end"), &BTUseAbility::get_wait_until_end);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "ability_id"), "set_ability_id", "get_ability_id");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "target_key"), "set_target_key", "get_target_key");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wait_until_end"), "set_wait_until_end", "get_wait_until_end");
}
