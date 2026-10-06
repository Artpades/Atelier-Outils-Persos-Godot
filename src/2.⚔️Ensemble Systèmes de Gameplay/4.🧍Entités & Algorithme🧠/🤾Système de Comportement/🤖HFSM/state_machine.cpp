#include "state_machine.hpp"

#include "../🧩Commun/behavior_debugger.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
// Garde-fou : transitions en chaîne (une transition qui en demande une autre dans _on_enter...).
constexpr int MAX_CHAINED_TRANSITIONS = 16;

String display_class(const Object *p_object)
{
    Ref<Script> script = p_object->get_script();
    if (script.is_valid() && !String(script->get_global_name()).is_empty())
    {
        return script->get_global_name();
    }
    return p_object->get_class();
}
} // namespace

void StateMachine::set_process_callback(ProcessCallback p_callback)
{
    process_callback = p_callback;
    _update_processing();
}

Ref<Blackboard> StateMachine::get_blackboard() const
{
    // Machine imbriquée : le tableau noir est celui de la machine racine.
    if (State *parent = get_parent_state())
    {
        return parent->get_blackboard();
    }
    return blackboard;
}

// ▶️ Exécution

void StateMachine::start()
{
    if (running)
    {
        return;
    }
    if (!is_root_machine())
    {
        ERR_PRINT("StateMachine imbriquée : elle est démarrée par sa machine parente.");
        return;
    }
    if (blackboard.is_null())
    {
        blackboard.instantiate();
    }
    running = true;
    in_transition = true;
    elapsed_time = 0.0;
    pending.clear();
    _internal_enter();
    _enter_initial_chain(this);
    in_transition = false;
    emit_signal("started");
    emit_signal("state_changed", Variant(), get_active_state());
    // Transitions demandées pendant les _on_enter du démarrage.
    if (!pending.is_empty())
    {
        PendingTransition next = pending[0];
        pending.remove_at(0);
        _fire(Object::cast_to<State>(ObjectDB::get_instance(next.source)), Object::cast_to<State>(ObjectDB::get_instance(next.target)), next.event);
    }
}

void StateMachine::stop()
{
    if (!running)
    {
        return;
    }
    in_transition = true;
    State *leaf = get_active_state();
    for (State *state = leaf; state; state = (state == this) ? nullptr : state->get_parent_state())
    {
        state->_internal_exit();
    }
    in_transition = false;
    pending.clear();
    running = false;
    emit_signal("stopped");
}

void StateMachine::restart()
{
    stop();
    // L'historique des sous-états est oublié : on repart des états initiaux.
    TypedArray<Node> all = find_children("*", "State", true, false);
    for (int i = 0; i < all.size(); i++)
    {
        Object::cast_to<State>(all[i])->_internal_forget_history();
    }
    _internal_forget_history();
    start();
}

bool StateMachine::send_event(const StringName &p_event, const Variant &p_data)
{
    if (!is_root_machine())
    {
        State *parent = get_parent_state();
        return parent && parent->send_event(p_event, p_data);
    }
    if (!running || p_event.is_empty())
    {
        return false;
    }
    emit_signal("event_received", p_event, p_data);
    // De la feuille vers la racine : consommation par le script, puis transitions portant l'événement.
    for (State *state = get_active_state(); state; state = (state == this) ? nullptr : state->get_parent_state())
    {
        if (state->_internal_event(p_event, p_data))
        {
            return true;
        }
        TypedArray<StateTransition> transitions = state->get_transitions();
        for (int i = 0; i < transitions.size(); i++)
        {
            Ref<StateTransition> transition = transitions[i];
            if (transition.is_null() || transition->get_event() != p_event)
            {
                continue;
            }
            State *target = transition->resolve_target(state);
            if (target && _owns(target) && transition->is_satisfied(state))
            {
                _fire(state, target, p_event);
                return true;
            }
        }
        if (!running)
        {
            break; // arrêtée par un script pendant l'événement
        }
    }
    return false;
}

bool StateMachine::transition_to(const Variant &p_target)
{
    if (!is_root_machine())
    {
        State *parent = get_parent_state();
        return parent && parent->transition_to(p_target);
    }
    State *target = nullptr;
    if (p_target.get_type() == Variant::OBJECT)
    {
        target = Object::cast_to<State>(p_target);
    }
    else if (p_target.get_type() == Variant::NODE_PATH || p_target.get_type() == Variant::STRING || p_target.get_type() == Variant::STRING_NAME)
    {
        target = Object::cast_to<State>(get_node_or_null(NodePath(String(p_target))));
    }
    if (!running || !target || !_owns(target) || target == this)
    {
        return false;
    }
    _fire(get_active_state(), target, StringName());
    return true;
}

void StateMachine::update(double p_delta)
{
    if (running)
    {
        _tick(p_delta);
    }
}

void StateMachine::physics_update(double p_delta)
{
    if (!running)
    {
        return;
    }
    TypedArray<State> path = get_active_path();
    for (int i = 0; i < path.size() && running; i++)
    {
        State *state = Object::cast_to<State>(path[i]);
        if (state && state->is_active())
        {
            state->_internal_physics_update(p_delta);
        }
    }
}

void StateMachine::_tick(double p_delta)
{
    elapsed_time += p_delta;
    TypedArray<State> path = get_active_path();
    for (int i = 0; i < path.size() && running; i++)
    {
        State *state = Object::cast_to<State>(path[i]);
        if (state && state->is_active())
        {
            state->_internal_update(p_delta);
        }
    }
    if (running)
    {
        _evaluate_automatic_transitions();
    }
    if (BehaviorDebugger *debugger = BehaviorDebugger::get_singleton())
    {
        debugger->notify(this);
    }
}

void StateMachine::_evaluate_automatic_transitions()
{
    for (State *state = get_active_state(); state; state = (state == this) ? nullptr : state->get_parent_state())
    {
        TypedArray<StateTransition> transitions = state->get_transitions();
        for (int i = 0; i < transitions.size(); i++)
        {
            Ref<StateTransition> transition = transitions[i];
            if (transition.is_null() || !transition->is_automatic())
            {
                continue;
            }
            State *target = transition->resolve_target(state);
            if (target && _owns(target) && transition->is_satisfied(state))
            {
                _fire(state, target, StringName());
                return;
            }
        }
    }
}

// 🔀 Transitions

void StateMachine::_fire(State *p_source, State *p_target, const StringName &p_event)
{
    if (!p_source || !p_target)
    {
        return;
    }
    if (in_transition)
    {
        pending.push_back({ObjectID(p_source->get_instance_id()), ObjectID(p_target->get_instance_id()), p_event});
        return;
    }
    _perform(p_source, p_target, p_event);
    for (int chained = 0; !pending.is_empty() && running; chained++)
    {
        PendingTransition next = pending[0];
        pending.remove_at(0);
        if (chained >= MAX_CHAINED_TRANSITIONS)
        {
            ERR_PRINT("StateMachine : trop de transitions en chaîne, les suivantes sont ignorées (boucle dans _on_enter ?).");
            pending.clear();
            break;
        }
        State *source = Object::cast_to<State>(ObjectDB::get_instance(next.source));
        State *target = Object::cast_to<State>(ObjectDB::get_instance(next.target));
        if (source && target && _owns(source) && _owns(target))
        {
            _perform(source->is_active() ? source : get_active_state(), target, next.event);
        }
    }
}

void StateMachine::_perform(State *p_source, State *p_target, const StringName &p_event)
{
    if (!running || p_target == this)
    {
        return;
    }
    // Domaine : le plus proche ancêtre STRICT commun à la source et à la cible (transition externe :
    // une transition vers soi-même ou vers un sous-état sort de la source puis y rentre).
    State *domain = nullptr;
    for (State *candidate = p_target->get_parent_state(); candidate && !domain; candidate = candidate->get_parent_state())
    {
        for (State *ancestor = p_source->get_parent_state(); ancestor; ancestor = ancestor->get_parent_state())
        {
            if (ancestor == candidate)
            {
                domain = candidate;
                break;
            }
        }
    }
    if (!domain)
    {
        domain = this;
    }

    in_transition = true;
    State *from_leaf = get_active_state();

    // Sortie : de la feuille active jusqu'au domaine (exclu).
    for (State *state = from_leaf; state && state != domain; state = state->get_parent_state())
    {
        if (state->is_active())
        {
            state->_internal_exit();
        }
    }
    // Entrée : du domaine (exclu) jusqu'à la cible, puis ses sous-états initiaux.
    LocalVector<State *> chain;
    for (State *state = p_target; state && state != domain; state = state->get_parent_state())
    {
        chain.push_back(state);
    }
    for (int64_t i = int64_t(chain.size()) - 1; i >= 0; i--)
    {
        chain[i]->_internal_enter();
    }
    _enter_initial_chain(p_target);
    in_transition = false;

    State *to_leaf = get_active_state();
    if (history_size > 0)
    {
        Dictionary entry;
        entry["from"] = _relative_path(from_leaf);
        entry["to"] = _relative_path(to_leaf);
        entry["source"] = _relative_path(p_source);
        entry["event"] = p_event;
        entry["time"] = elapsed_time;
        history.push_back(entry);
        while (history.size() > history_size)
        {
            history.pop_front();
        }
    }
    emit_signal("transitioned", p_source, p_target, p_event);
    if (from_leaf != to_leaf)
    {
        emit_signal("state_changed", from_leaf, to_leaf);
    }
}

void StateMachine::_enter_initial_chain(State *p_from)
{
    State *state = p_from;
    while (state && state->is_compound())
    {
        state = state->get_initial_substate();
        if (state)
        {
            state->_internal_enter();
        }
    }
}

bool StateMachine::_owns(const State *p_state) const { return p_state && (p_state == this || is_ancestor_of(const_cast<State *>(p_state))); }

String StateMachine::_relative_path(const Node *p_node) const { return p_node ? String(get_path_to(const_cast<Node *>(p_node))) : String(); }

// 🔍 Requêtes

State *StateMachine::get_active_state() const
{
    if (!running && is_root_machine())
    {
        return nullptr;
    }
    const State *state = this;
    while (State *child = state->get_active_substate())
    {
        state = child;
    }
    return const_cast<State *>(state);
}

TypedArray<State> StateMachine::get_active_path() const
{
    TypedArray<State> path;
    if (!running && is_root_machine())
    {
        return path;
    }
    const State *state = this;
    while (state)
    {
        path.push_back(const_cast<State *>(state));
        state = state->get_active_substate();
    }
    return path;
}

Node *StateMachine::get_agent_node() const
{
    if (State *parent = get_parent_state())
    {
        return parent->get_agent();
    }
    Node *agent = agent_path.is_empty() ? nullptr : get_node_or_null(agent_path);
    return agent ? agent : get_parent();
}

Dictionary StateMachine::get_debug_snapshot() const
{
    Array nodes;
    TypedArray<Node> states = find_children("*", "State", true, false);
    states.push_front(const_cast<StateMachine *>(this));
    for (int i = 0; i < states.size(); i++)
    {
        State *state = Object::cast_to<State>(states[i]);
        // Seuls les états dont toute la chaîne de parents est faite de State font partie de la machine.
        bool reachable = true;
        for (Node *n = state; n && n != this; n = n->get_parent())
        {
            if (!Object::cast_to<State>(n->get_parent()))
            {
                reachable = false;
                break;
            }
        }
        if (!reachable)
        {
            continue;
        }
        State *parent = state == this ? nullptr : state->get_parent_state();
        Dictionary node;
        node["id"] = _relative_path(state);
        node["parent"] = parent ? _relative_path(parent) : String();
        node["name"] = String(state->get_name());
        node["class"] = display_class(state);
        node["kind"] = state == this ? "machine" : (state->is_compound() ? "compound" : "state");
        node["active"] = state->is_active();
        node["initial"] = parent && parent->get_initial_substate() == state;
        node["status"] = -1;
        node["info"] = state->is_active() ? String::num(state->get_time_in_state(), 1) + " s" : String();
        Array links;
        TypedArray<StateTransition> transitions = state->get_transitions();
        for (int t = 0; t < transitions.size(); t++)
        {
            Ref<StateTransition> transition = transitions[t];
            State *target = transition.is_valid() ? transition->resolve_target(state) : nullptr;
            if (!target || !_owns(target))
            {
                continue;
            }
            Dictionary link;
            link["to"] = _relative_path(target);
            link["label"] = transition->get_label();
            link["event"] = transition->get_event();
            link["index"] = t;
            links.push_back(link);
        }
        node["links"] = links;
        nodes.push_back(node);
    }

    Dictionary snapshot;
    snapshot["type"] = "hfsm";
    snapshot["name"] = String(get_name());
    snapshot["running"] = running;
    snapshot["time"] = elapsed_time;
    snapshot["nodes"] = nodes;
    Array recent;
    for (int i = MAX(0, int(history.size()) - 8); i < history.size(); i++)
    {
        Dictionary entry = history[i];
        String event = entry["event"];
        recent.push_back(String(entry["from"]) + U" → " + String(entry["to"]) + (event.is_empty() ? String() : U"  ⚡" + event));
    }
    snapshot["history"] = recent;
    return snapshot;
}

PackedStringArray StateMachine::_get_configuration_warnings() const
{
    PackedStringArray warnings = State::_get_configuration_warnings();
    if (!is_compound())
    {
        warnings.push_back(U"Ajoutez au moins un State enfant.");
    }
    return warnings;
}

void StateMachine::_update_processing()
{
    bool runtime = !Engine::get_singleton()->is_editor_hint() && is_root_machine();
    set_process(runtime && process_callback != PROCESS_MANUAL);
    set_physics_process(runtime && process_callback != PROCESS_MANUAL);
}

void StateMachine::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_READY:
        _update_processing();
        if (autostart && is_root_machine() && !Engine::get_singleton()->is_editor_hint())
        {
            // Différé : l'agent (parent) n'est prêt qu'après ses enfants.
            call_deferred("start");
        }
        break;
    case NOTIFICATION_ENTER_TREE:
        if (BehaviorDebugger *debugger = BehaviorDebugger::get_singleton(); debugger && is_root_machine() && !Engine::get_singleton()->is_editor_hint())
        {
            debugger->register_runner(this);
        }
        break;
    case NOTIFICATION_EXIT_TREE:
        if (BehaviorDebugger *debugger = BehaviorDebugger::get_singleton())
        {
            debugger->unregister_runner(this);
        }
        break;
    case NOTIFICATION_PROCESS:
        if (running)
        {
            double delta = get_process_delta_time();
            if (process_callback == PROCESS_IDLE)
            {
                _tick(delta);
            }
            else
            {
                // Mise à jour des scripts à chaque image, transitions dans _physics_process.
                TypedArray<State> path = get_active_path();
                for (int i = 0; i < path.size() && running; i++)
                {
                    Object::cast_to<State>(path[i])->_internal_update(delta);
                }
            }
        }
        break;
    case NOTIFICATION_PHYSICS_PROCESS:
        if (running)
        {
            double delta = get_physics_process_delta_time();
            physics_update(delta);
            if (process_callback == PROCESS_PHYSICS && running)
            {
                elapsed_time += delta;
                _evaluate_automatic_transitions();
                if (BehaviorDebugger *debugger = BehaviorDebugger::get_singleton())
                {
                    debugger->notify(this);
                }
            }
        }
        break;
    default:
        break;
    }
}

// 🔗 Liaisons

void StateMachine::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_autostart", "autostart"), &StateMachine::set_autostart);
    ClassDB::bind_method(D_METHOD("get_autostart"), &StateMachine::get_autostart);
    ClassDB::bind_method(D_METHOD("set_process_callback", "callback"), &StateMachine::set_process_callback);
    ClassDB::bind_method(D_METHOD("get_process_callback"), &StateMachine::get_process_callback);
    ClassDB::bind_method(D_METHOD("set_agent_path", "path"), &StateMachine::set_agent_path);
    ClassDB::bind_method(D_METHOD("get_agent_path"), &StateMachine::get_agent_path);
    ClassDB::bind_method(D_METHOD("set_blackboard", "blackboard"), &StateMachine::set_blackboard);
    ClassDB::bind_method(D_METHOD("get_blackboard"), &StateMachine::get_blackboard);
    ClassDB::bind_method(D_METHOD("set_history_size", "size"), &StateMachine::set_history_size);
    ClassDB::bind_method(D_METHOD("get_history_size"), &StateMachine::get_history_size);

    ClassDB::bind_method(D_METHOD("start"), &StateMachine::start);
    ClassDB::bind_method(D_METHOD("stop"), &StateMachine::stop);
    ClassDB::bind_method(D_METHOD("restart"), &StateMachine::restart);
    ClassDB::bind_method(D_METHOD("is_running"), &StateMachine::is_running);
    ClassDB::bind_method(D_METHOD("is_root_machine"), &StateMachine::is_root_machine);
    ClassDB::bind_method(D_METHOD("send_event", "event", "data"), &StateMachine::send_event, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("transition_to", "target"), &StateMachine::transition_to);
    ClassDB::bind_method(D_METHOD("update", "delta"), &StateMachine::update);
    ClassDB::bind_method(D_METHOD("physics_update", "delta"), &StateMachine::physics_update);
    ClassDB::bind_method(D_METHOD("get_active_state"), &StateMachine::get_active_state);
    ClassDB::bind_method(D_METHOD("get_active_path"), &StateMachine::get_active_path);
    ClassDB::bind_method(D_METHOD("get_agent_node"), &StateMachine::get_agent_node);
    ClassDB::bind_method(D_METHOD("get_elapsed_time"), &StateMachine::get_elapsed_time);
    ClassDB::bind_method(D_METHOD("get_history"), &StateMachine::get_history);
    ClassDB::bind_method(D_METHOD("clear_history"), &StateMachine::clear_history);
    ClassDB::bind_method(D_METHOD("get_debug_snapshot"), &StateMachine::get_debug_snapshot);

    BIND_ENUM_CONSTANT(PROCESS_IDLE);
    BIND_ENUM_CONSTANT(PROCESS_PHYSICS);
    BIND_ENUM_CONSTANT(PROCESS_MANUAL);

    ADD_GROUP(U"▶️ Exécution", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autostart"), "set_autostart", "get_autostart");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "process_callback", PROPERTY_HINT_ENUM, "Idle,Physics,Manual"), "set_process_callback", "get_process_callback");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "agent_path"), "set_agent_path", "get_agent_path");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "blackboard", PROPERTY_HINT_RESOURCE_TYPE, "Blackboard"), "set_blackboard", "get_blackboard");
    ADD_GROUP(U"🐞 Débogage", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "history_size", PROPERTY_HINT_RANGE, "0,256,1"), "set_history_size", "get_history_size");

    ADD_SIGNAL(MethodInfo("started"));
    ADD_SIGNAL(MethodInfo("stopped"));
    ADD_SIGNAL(MethodInfo("state_changed", PropertyInfo(Variant::OBJECT, "from", PROPERTY_HINT_NODE_TYPE, "State"), PropertyInfo(Variant::OBJECT, "to", PROPERTY_HINT_NODE_TYPE, "State")));
    ADD_SIGNAL(MethodInfo("transitioned", PropertyInfo(Variant::OBJECT, "source", PROPERTY_HINT_NODE_TYPE, "State"), PropertyInfo(Variant::OBJECT, "target", PROPERTY_HINT_NODE_TYPE, "State"), PropertyInfo(Variant::STRING_NAME, "event")));
    ADD_SIGNAL(MethodInfo("event_received", PropertyInfo(Variant::STRING_NAME, "event"), PropertyInfo(Variant::NIL, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)));
}
