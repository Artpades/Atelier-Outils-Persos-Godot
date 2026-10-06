#include "behavior_tree.hpp"

#include "../🧩Commun/behavior_debugger.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void BehaviorTree::set_active(bool p_active)
{
    if (active == p_active)
    {
        return;
    }
    active = p_active;
    if (!active)
    {
        interrupt();
    }
    _update_processing();
}

void BehaviorTree::set_process_callback(ProcessCallback p_callback)
{
    process_callback = p_callback;
    _update_processing();
}

int BehaviorTree::tick(double p_delta)
{
    BTNode *root = get_root_bt_node();
    if (!root)
    {
        return -1;
    }
    if (blackboard.is_null())
    {
        blackboard.instantiate();
    }
    tick_count++;
    elapsed_time += p_delta;
    BTNode::Status status = root->execute(p_delta);
    last_status = status;
    emit_signal("ticked", status);
    if (status != BTNode::RUNNING)
    {
        emit_signal("finished", status);
    }
    if (BehaviorDebugger *debugger = BehaviorDebugger::get_singleton())
    {
        debugger->notify(this);
    }
    return status;
}

void BehaviorTree::interrupt()
{
    if (BTNode *root = get_root_bt_node())
    {
        root->abort();
    }
}

BTNode *BehaviorTree::get_root_bt_node() const
{
    for (int i = 0; i < get_child_count(); i++)
    {
        if (BTNode *child = Object::cast_to<BTNode>(get_child(i)))
        {
            return child;
        }
    }
    return nullptr;
}

Node *BehaviorTree::get_agent_node() const
{
    Node *agent = agent_path.is_empty() ? nullptr : get_node_or_null(agent_path);
    return agent ? agent : get_parent();
}

namespace
{
String display_class(const Object *p_object)
{
    Ref<Script> script = p_object->get_script();
    if (script.is_valid() && !String(script->get_global_name()).is_empty())
    {
        return script->get_global_name();
    }
    return p_object->get_class();
}

void collect(const BehaviorTree *p_tree, const BTNode *p_node, const String &p_parent, Array &r_nodes)
{
    String id = p_tree->get_path_to(const_cast<BTNode *>(p_node));
    Dictionary node;
    node["id"] = id;
    node["parent"] = p_parent;
    node["name"] = String(p_node->get_name());
    node["class"] = display_class(p_node);
    node["kind"] = p_node->get_debug_kind();
    node["status"] = p_node->get_last_status();
    node["active"] = p_node->is_running();
    // « frais » : exécuté pendant le dernier tick (les autres statuts sont anciens).
    node["fresh"] = p_tree->get_tick_count() > 0 && p_node->get_last_tick() == p_tree->get_tick_count();
    node["initial"] = false;
    node["info"] = p_node->get_debug_info();
    node["links"] = Array();
    r_nodes.push_back(node);
    for (int i = 0; i < p_node->get_child_count(); i++)
    {
        if (const BTNode *child = Object::cast_to<BTNode>(p_node->get_child(i)))
        {
            collect(p_tree, child, id, r_nodes);
        }
    }
}
} // namespace

Dictionary BehaviorTree::get_debug_snapshot() const
{
    Array nodes;
    Dictionary root;
    root["id"] = ".";
    root["parent"] = String();
    root["name"] = String(get_name());
    root["class"] = display_class(this);
    root["kind"] = "tree";
    root["status"] = last_status;
    root["active"] = active && last_status == BTNode::RUNNING;
    root["fresh"] = tick_count > 0;
    root["initial"] = false;
    root["info"] = tick_count > 0 ? U"tick n°" + String::num_uint64(tick_count) : String();
    root["links"] = Array();
    nodes.push_back(root);
    for (int i = 0; i < get_child_count(); i++)
    {
        if (const BTNode *child = Object::cast_to<BTNode>(get_child(i)))
        {
            collect(this, child, ".", nodes);
        }
    }

    Dictionary snapshot;
    snapshot["type"] = "bt";
    snapshot["name"] = String(get_name());
    snapshot["running"] = active;
    snapshot["time"] = elapsed_time;
    snapshot["tick"] = tick_count;
    snapshot["nodes"] = nodes;
    snapshot["history"] = Array();
    return snapshot;
}

PackedStringArray BehaviorTree::_get_configuration_warnings() const
{
    PackedStringArray warnings;
    int count = 0;
    for (int i = 0; i < get_child_count(); i++)
    {
        count += Object::cast_to<BTNode>(get_child(i)) ? 1 : 0;
    }
    if (count == 0)
    {
        warnings.push_back(U"Ajoutez un BTNode enfant (Séquence, Sélecteur...) : c'est la racine de l'arbre.");
    }
    else if (count > 1)
    {
        warnings.push_back(U"Seul le premier BTNode enfant est exécuté : regroupez les autres sous une Séquence ou un Sélecteur.");
    }
    return warnings;
}

void BehaviorTree::_update_processing()
{
    bool runtime = active && !Engine::get_singleton()->is_editor_hint() && is_inside_tree();
    set_process(runtime && process_callback == PROCESS_IDLE);
    set_physics_process(runtime && process_callback == PROCESS_PHYSICS);
}

void BehaviorTree::_advance(double p_delta)
{
    if (tick_interval <= 0.0)
    {
        tick(p_delta);
        return;
    }
    interval_accumulator += p_delta;
    if (interval_accumulator >= tick_interval)
    {
        double delta = interval_accumulator;
        interval_accumulator = 0.0;
        tick(delta);
    }
}

void BehaviorTree::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_READY:
        if (blackboard.is_null() && !Engine::get_singleton()->is_editor_hint())
        {
            blackboard.instantiate();
        }
        _update_processing();
        break;
    case NOTIFICATION_ENTER_TREE:
        if (BehaviorDebugger *debugger = BehaviorDebugger::get_singleton(); debugger && !Engine::get_singleton()->is_editor_hint())
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
        _advance(get_process_delta_time());
        break;
    case NOTIFICATION_PHYSICS_PROCESS:
        _advance(get_physics_process_delta_time());
        break;
    default:
        break;
    }
}

// 🔗 Liaisons

void BehaviorTree::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_active", "active"), &BehaviorTree::set_active);
    ClassDB::bind_method(D_METHOD("is_active"), &BehaviorTree::is_active);
    ClassDB::bind_method(D_METHOD("set_process_callback", "callback"), &BehaviorTree::set_process_callback);
    ClassDB::bind_method(D_METHOD("get_process_callback"), &BehaviorTree::get_process_callback);
    ClassDB::bind_method(D_METHOD("set_tick_interval", "interval"), &BehaviorTree::set_tick_interval);
    ClassDB::bind_method(D_METHOD("get_tick_interval"), &BehaviorTree::get_tick_interval);
    ClassDB::bind_method(D_METHOD("set_agent_path", "path"), &BehaviorTree::set_agent_path);
    ClassDB::bind_method(D_METHOD("get_agent_path"), &BehaviorTree::get_agent_path);
    ClassDB::bind_method(D_METHOD("set_blackboard", "blackboard"), &BehaviorTree::set_blackboard);
    ClassDB::bind_method(D_METHOD("get_blackboard"), &BehaviorTree::get_blackboard);

    ClassDB::bind_method(D_METHOD("tick", "delta"), &BehaviorTree::tick);
    ClassDB::bind_method(D_METHOD("interrupt"), &BehaviorTree::interrupt);
    ClassDB::bind_method(D_METHOD("get_root_bt_node"), &BehaviorTree::get_root_bt_node);
    ClassDB::bind_method(D_METHOD("get_agent_node"), &BehaviorTree::get_agent_node);
    ClassDB::bind_method(D_METHOD("get_last_status"), &BehaviorTree::get_last_status);
    ClassDB::bind_method(D_METHOD("get_tick_count"), &BehaviorTree::get_tick_count);
    ClassDB::bind_method(D_METHOD("get_elapsed_time"), &BehaviorTree::get_elapsed_time);
    ClassDB::bind_method(D_METHOD("get_debug_snapshot"), &BehaviorTree::get_debug_snapshot);

    BIND_ENUM_CONSTANT(PROCESS_IDLE);
    BIND_ENUM_CONSTANT(PROCESS_PHYSICS);
    BIND_ENUM_CONSTANT(PROCESS_MANUAL);

    ADD_GROUP(U"▶️ Exécution", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "process_callback", PROPERTY_HINT_ENUM, "Idle,Physics,Manual"), "set_process_callback", "get_process_callback");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tick_interval", PROPERTY_HINT_RANGE, "0,2,0.01,or_greater,suffix:s"), "set_tick_interval", "get_tick_interval");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "agent_path"), "set_agent_path", "get_agent_path");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "blackboard", PROPERTY_HINT_RESOURCE_TYPE, "Blackboard"), "set_blackboard", "get_blackboard");

    ADD_SIGNAL(MethodInfo("ticked", PropertyInfo(Variant::INT, "status")));
    ADD_SIGNAL(MethodInfo("finished", PropertyInfo(Variant::INT, "status")));
}
