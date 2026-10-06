#include "bt_node.hpp"
#include "behavior_tree.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

BTNode::Status BTNode::execute(double p_delta)
{
    BehaviorTree *tree = get_behavior_tree();
    last_tick = tree ? tree->get_tick_count() : last_tick + 1;
    if (!running)
    {
        running = true;
        running_time = 0.0;
        _native_enter();
        GDVIRTUAL_CALL(_on_enter);
    }
    running_time += p_delta;

    Status status = SUCCESS;
    int scripted = SUCCESS;
    if (GDVIRTUAL_CALL(_tick, p_delta, scripted))
    {
        status = (scripted >= SUCCESS && scripted <= RUNNING) ? Status(scripted) : FAILURE;
    }
    else
    {
        status = _native_tick(p_delta);
    }

    last_status = status;
    if (status != RUNNING && running)
    {
        running = false;
        _native_exit();
        GDVIRTUAL_CALL(_on_exit);
    }
    return status;
}

void BTNode::abort()
{
    if (!running)
    {
        return;
    }
    _abort_running_children();
    running = false;
    _native_exit();
    GDVIRTUAL_CALL(_on_exit);
}

void BTNode::_abort_running_children(int p_from_index)
{
    int index = 0;
    for (int i = 0; i < get_child_count(); i++)
    {
        BTNode *child = Object::cast_to<BTNode>(get_child(i));
        if (!child)
        {
            continue;
        }
        if (index++ >= p_from_index && child->is_running())
        {
            child->abort();
        }
    }
}

// 🔌 Accès

BehaviorTree *BTNode::get_behavior_tree() const
{
    for (Node *parent = get_parent(); parent; parent = parent->get_parent())
    {
        if (BehaviorTree *tree = Object::cast_to<BehaviorTree>(parent))
        {
            return tree;
        }
        if (!Object::cast_to<BTNode>(parent))
        {
            return nullptr;
        }
    }
    return nullptr;
}

Node *BTNode::get_agent() const
{
    BehaviorTree *tree = get_behavior_tree();
    return tree ? tree->get_agent_node() : nullptr;
}

Ref<Blackboard> BTNode::get_blackboard() const
{
    BehaviorTree *tree = get_behavior_tree();
    return tree ? tree->get_blackboard() : Ref<Blackboard>();
}

double BTNode::_tree_time() const
{
    BehaviorTree *tree = get_behavior_tree();
    return tree ? tree->get_elapsed_time() : 0.0;
}

bool BTNode::_has_script_tick() const { return GDVIRTUAL_IS_OVERRIDDEN(_tick); }

TypedArray<BTNode> BTNode::get_bt_children() const
{
    TypedArray<BTNode> result;
    for (int i = 0; i < get_child_count(); i++)
    {
        if (BTNode *child = Object::cast_to<BTNode>(get_child(i)))
        {
            result.push_back(child);
        }
    }
    return result;
}

int BTNode::get_bt_child_count() const
{
    int count = 0;
    for (int i = 0; i < get_child_count(); i++)
    {
        count += Object::cast_to<BTNode>(get_child(i)) ? 1 : 0;
    }
    return count;
}

BTNode *BTNode::get_bt_child(int p_index) const
{
    int index = 0;
    for (int i = 0; i < get_child_count(); i++)
    {
        if (BTNode *child = Object::cast_to<BTNode>(get_child(i)))
        {
            if (index++ == p_index)
            {
                return child;
            }
        }
    }
    return nullptr;
}

PackedStringArray BTNode::_get_configuration_warnings() const
{
    PackedStringArray warnings;
    Node *parent = get_parent();
    if (parent && !Object::cast_to<BTNode>(parent) && !Object::cast_to<BehaviorTree>(parent))
    {
        warnings.push_back(U"Un BTNode doit être placé sous un BehaviorTree ou sous un autre BTNode.");
    }
    return warnings;
}

// 🔗 Liaisons

void BTNode::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("execute", "delta"), &BTNode::execute);
    ClassDB::bind_method(D_METHOD("abort"), &BTNode::abort);
    ClassDB::bind_method(D_METHOD("get_behavior_tree"), &BTNode::get_behavior_tree);
    ClassDB::bind_method(D_METHOD("get_agent"), &BTNode::get_agent);
    ClassDB::bind_method(D_METHOD("get_blackboard"), &BTNode::get_blackboard);
    ClassDB::bind_method(D_METHOD("get_bt_children"), &BTNode::get_bt_children);
    ClassDB::bind_method(D_METHOD("get_bt_child_count"), &BTNode::get_bt_child_count);
    ClassDB::bind_method(D_METHOD("get_bt_child", "index"), &BTNode::get_bt_child);
    ClassDB::bind_method(D_METHOD("get_last_status"), &BTNode::get_last_status);
    ClassDB::bind_method(D_METHOD("is_running"), &BTNode::is_running);
    ClassDB::bind_method(D_METHOD("get_running_time"), &BTNode::get_running_time);
    ClassDB::bind_method(D_METHOD("get_last_tick"), &BTNode::get_last_tick);
    ClassDB::bind_method(D_METHOD("get_debug_kind"), &BTNode::get_debug_kind);
    ClassDB::bind_method(D_METHOD("get_debug_info"), &BTNode::get_debug_info);

    GDVIRTUAL_BIND(_on_enter);
    GDVIRTUAL_BIND(_on_exit);
    GDVIRTUAL_BIND(_tick, "delta");

    BIND_ENUM_CONSTANT(SUCCESS);
    BIND_ENUM_CONSTANT(FAILURE);
    BIND_ENUM_CONSTANT(RUNNING);
}
