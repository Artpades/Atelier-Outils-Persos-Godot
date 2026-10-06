#include "state.hpp"
#include "state_machine.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void State::set_transitions(const TypedArray<StateTransition> &p_transitions)
{
    transitions = p_transitions;
    update_configuration_warnings();
}

void State::set_initial_state(const NodePath &p_path)
{
    initial_state = p_path;
    last_resolved_initial = ObjectID();
    update_configuration_warnings();
}

// 🧭 Hiérarchie

State *State::get_parent_state() const { return Object::cast_to<State>(get_parent()); }

StateMachine *State::get_state_machine() const
{
    const State *top = this;
    while (State *parent = top->get_parent_state())
    {
        top = parent;
    }
    return Object::cast_to<StateMachine>(const_cast<State *>(top));
}

TypedArray<State> State::get_substates() const
{
    TypedArray<State> result;
    for (int i = 0; i < get_child_count(); i++)
    {
        if (State *child = Object::cast_to<State>(get_child(i)))
        {
            result.push_back(child);
        }
    }
    return result;
}

bool State::is_compound() const
{
    for (int i = 0; i < get_child_count(); i++)
    {
        if (Object::cast_to<State>(get_child(i)))
        {
            return true;
        }
    }
    return false;
}

State *State::get_initial_substate() const
{
    if (remember_last_substate)
    {
        State *last = Object::cast_to<State>(ObjectDB::get_instance(last_substate_id));
        if (last && last->get_parent() == this)
        {
            return last;
        }
    }
    if (!initial_state.is_empty())
    {
        State *initial = Object::cast_to<State>(get_node_or_null(initial_state));
        if (initial && initial->get_parent() == this)
        {
            last_resolved_initial = ObjectID(initial->get_instance_id());
            return initial;
        }
    }
    for (int i = 0; i < get_child_count(); i++)
    {
        if (State *child = Object::cast_to<State>(get_child(i)))
        {
            return child;
        }
    }
    return nullptr;
}

State *State::get_active_substate() const { return Object::cast_to<State>(ObjectDB::get_instance(active_substate_id)); }

// 🔌 Accès

Node *State::get_agent() const
{
    StateMachine *machine = get_state_machine();
    return machine ? machine->get_agent_node() : nullptr;
}

Ref<Blackboard> State::get_blackboard() const
{
    StateMachine *machine = get_state_machine();
    return machine ? machine->get_blackboard() : Ref<Blackboard>();
}

bool State::send_event(const StringName &p_event, const Variant &p_data)
{
    StateMachine *machine = get_state_machine();
    return machine && machine->send_event(p_event, p_data);
}

bool State::transition_to(const Variant &p_target)
{
    StateMachine *machine = get_state_machine();
    if (!machine)
    {
        return false;
    }
    // Un chemin donné à un état est relatif à cet état.
    if (p_target.get_type() == Variant::NODE_PATH || p_target.get_type() == Variant::STRING || p_target.get_type() == Variant::STRING_NAME)
    {
        return machine->transition_to(get_node_or_null(NodePath(String(p_target))));
    }
    return machine->transition_to(p_target);
}

PackedStringArray State::_get_configuration_warnings() const
{
    PackedStringArray warnings;
    if (!get_parent_state() && !Object::cast_to<StateMachine>(const_cast<State *>(this)))
    {
        warnings.push_back(U"Un State doit être placé sous une StateMachine (ou sous un autre State).");
    }
    if (!initial_state.is_empty())
    {
        State *initial = Object::cast_to<State>(get_node_or_null(initial_state));
        if (!initial || initial->get_parent() != this)
        {
            warnings.push_back(U"initial_state doit désigner un State enfant direct.");
        }
    }
    for (int i = 0; i < transitions.size(); i++)
    {
        Ref<StateTransition> transition = transitions[i];
        if (transition.is_null())
        {
            warnings.push_back(U"Transition n°" + String::num_int64(i) + U" vide.");
        }
        else if (is_inside_tree() && !transition->resolve_target(this))
        {
            warnings.push_back(U"Transition n°" + String::num_int64(i) + U" : état cible introuvable (" + String(transition->get_target_state()) + ").");
        }
    }
    return warnings;
}

// ⚙️ Interne

void State::_internal_enter()
{
    active = true;
    time_in_state = 0.0;
    active_substate_id = ObjectID();
    if (State *parent = get_parent_state())
    {
        parent->active_substate_id = ObjectID(get_instance_id());
    }
    GDVIRTUAL_CALL(_on_enter);
    emit_signal("entered");
}

void State::_internal_exit()
{
    GDVIRTUAL_CALL(_on_exit);
    active = false;
    active_substate_id = ObjectID();
    if (State *parent = get_parent_state())
    {
        parent->last_substate_id = ObjectID(get_instance_id());
        if (parent->active_substate_id == ObjectID(get_instance_id()))
        {
            parent->active_substate_id = ObjectID();
        }
    }
    emit_signal("exited");
}

void State::_internal_update(double p_delta)
{
    time_in_state += p_delta;
    GDVIRTUAL_CALL(_on_update, p_delta);
    emit_signal("updated", p_delta);
}

void State::_internal_physics_update(double p_delta) { GDVIRTUAL_CALL(_on_physics_update, p_delta); }

bool State::_internal_event(const StringName &p_event, const Variant &p_data)
{
    bool consumed = false;
    return GDVIRTUAL_CALL(_on_event, p_event, p_data, consumed) && consumed;
}

// 🔗 Liaisons

void State::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_transitions", "transitions"), &State::set_transitions);
    ClassDB::bind_method(D_METHOD("get_transitions"), &State::get_transitions);
    ClassDB::bind_method(D_METHOD("set_initial_state", "path"), &State::set_initial_state);
    ClassDB::bind_method(D_METHOD("get_initial_state"), &State::get_initial_state);
    ClassDB::bind_method(D_METHOD("set_remember_last_substate", "remember"), &State::set_remember_last_substate);
    ClassDB::bind_method(D_METHOD("get_remember_last_substate"), &State::get_remember_last_substate);

    ClassDB::bind_method(D_METHOD("get_parent_state"), &State::get_parent_state);
    ClassDB::bind_method(D_METHOD("get_state_machine"), &State::get_state_machine);
    ClassDB::bind_method(D_METHOD("get_substates"), &State::get_substates);
    ClassDB::bind_method(D_METHOD("is_compound"), &State::is_compound);
    ClassDB::bind_method(D_METHOD("get_initial_substate"), &State::get_initial_substate);
    ClassDB::bind_method(D_METHOD("get_active_substate"), &State::get_active_substate);
    ClassDB::bind_method(D_METHOD("is_active"), &State::is_active);
    ClassDB::bind_method(D_METHOD("get_time_in_state"), &State::get_time_in_state);
    ClassDB::bind_method(D_METHOD("get_agent"), &State::get_agent);
    ClassDB::bind_method(D_METHOD("get_blackboard"), &State::get_blackboard);
    ClassDB::bind_method(D_METHOD("send_event", "event", "data"), &State::send_event, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("transition_to", "target"), &State::transition_to);

    GDVIRTUAL_BIND(_on_enter);
    GDVIRTUAL_BIND(_on_exit);
    GDVIRTUAL_BIND(_on_update, "delta");
    GDVIRTUAL_BIND(_on_physics_update, "delta");
    GDVIRTUAL_BIND(_on_event, "event", "data");

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "transitions", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":StateTransition"), "set_transitions", "get_transitions");
    ADD_GROUP(U"🧩 Sous-états", "");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "initial_state", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "State"), "set_initial_state", "get_initial_state");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "remember_last_substate"), "set_remember_last_substate", "get_remember_last_substate");

    ADD_SIGNAL(MethodInfo("entered"));
    ADD_SIGNAL(MethodInfo("exited"));
    ADD_SIGNAL(MethodInfo("updated", PropertyInfo(Variant::FLOAT, "delta")));
}
