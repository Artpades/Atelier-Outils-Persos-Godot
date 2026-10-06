#include "input_context.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void InputContext::set_context_name(const StringName &p_name)
{
    context_name = p_name;
    emit_changed();
}

void InputContext::set_enabled(bool p_enabled)
{
    enabled = p_enabled;
    emit_changed();
}

void InputContext::set_priority(int p_priority)
{
    priority = p_priority;
    emit_changed();
}

void InputContext::set_exclusive(bool p_exclusive)
{
    exclusive = p_exclusive;
    emit_changed();
}

void InputContext::set_actions(const TypedArray<InputAction> &p_actions)
{
    actions = p_actions;
    emit_changed();
}

Ref<InputAction> InputContext::get_action(const StringName &p_name) const
{
    for (int i = 0; i < actions.size(); i++)
    {
        Ref<InputAction> action = actions[i];
        if (action.is_valid() && action->get_action_name() == p_name)
        {
            return action;
        }
    }
    return Ref<InputAction>();
}

PackedStringArray InputContext::get_action_names() const
{
    PackedStringArray names;
    for (int i = 0; i < actions.size(); i++)
    {
        Ref<InputAction> action = actions[i];
        if (action.is_valid())
        {
            names.push_back(action->get_action_name());
        }
    }
    return names;
}

void InputContext::add_action(const Ref<InputAction> &p_action)
{
    if (p_action.is_null() || actions.has(p_action))
    {
        return;
    }
    TypedArray<InputAction> copy = actions.duplicate(); // le tableau peut être partagé (getter, inspecteur)
    copy.push_back(p_action);
    set_actions(copy);
}

void InputContext::remove_action(const Ref<InputAction> &p_action)
{
    TypedArray<InputAction> copy = actions.duplicate();
    copy.erase(p_action);
    set_actions(copy);
}

Ref<InputContext> InputContext::make_runtime_copy() const
{
    Ref<InputContext> copy = duplicate(false);
    TypedArray<InputAction> copies;
    for (int i = 0; i < actions.size(); i++)
    {
        Ref<InputAction> action = actions[i];
        if (action.is_valid())
        {
            copies.push_back(action->make_runtime_copy());
        }
    }
    copy->set_actions(copies);
    return copy;
}

void InputContext::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_context_name", "name"), &InputContext::set_context_name);
    ClassDB::bind_method(D_METHOD("get_context_name"), &InputContext::get_context_name);
    ClassDB::bind_method(D_METHOD("set_description", "description"), &InputContext::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &InputContext::get_description);
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &InputContext::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &InputContext::is_enabled);
    ClassDB::bind_method(D_METHOD("set_priority", "priority"), &InputContext::set_priority);
    ClassDB::bind_method(D_METHOD("get_priority"), &InputContext::get_priority);
    ClassDB::bind_method(D_METHOD("set_exclusive", "exclusive"), &InputContext::set_exclusive);
    ClassDB::bind_method(D_METHOD("is_exclusive"), &InputContext::is_exclusive);
    ClassDB::bind_method(D_METHOD("set_actions", "actions"), &InputContext::set_actions);
    ClassDB::bind_method(D_METHOD("get_actions"), &InputContext::get_actions);
    ClassDB::bind_method(D_METHOD("get_action", "name"), &InputContext::get_action);
    ClassDB::bind_method(D_METHOD("has_action", "name"), &InputContext::has_action);
    ClassDB::bind_method(D_METHOD("get_action_names"), &InputContext::get_action_names);
    ClassDB::bind_method(D_METHOD("add_action", "action"), &InputContext::add_action);
    ClassDB::bind_method(D_METHOD("remove_action", "action"), &InputContext::remove_action);
    ClassDB::bind_method(D_METHOD("make_runtime_copy"), &InputContext::make_runtime_copy);

    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "context_name"), "set_context_name", "get_context_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description", "get_description");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "priority", PROPERTY_HINT_RANGE, "-100,100,1,or_greater,or_less"), "set_priority", "get_priority");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "exclusive"), "set_exclusive", "is_exclusive");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "actions", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":InputAction"), "set_actions", "get_actions");
}
