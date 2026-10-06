#include "interactable.hpp"
#include "character_core.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

LocalVector<ObjectID> Interactable::instances;

bool Interactable::interact(Node *p_by)
{
    if (!enabled)
    {
        return false;
    }
    if (one_shot)
    {
        enabled = false;
    }
    emit_signal("interacted", p_by);
    return true;
}

Vector3 Interactable::get_world_position() const { return node_world_position(get_parent()); }

bool Interactable::is_in_range_of(Node *p_node) const
{
    return p_node && get_parent() != p_node && node_world_position(p_node).distance_to(get_world_position()) <= interaction_range;
}

Interactable *Interactable::find_nearest(Node *p_from)
{
    Interactable *nearest = nullptr;
    double best = 0.0;
    Vector3 origin = node_world_position(p_from);
    for (uint32_t i = 0; i < instances.size(); i++)
    {
        Interactable *candidate = Object::cast_to<Interactable>(ObjectDB::get_instance(instances[i]));
        if (!candidate || !candidate->enabled || !candidate->is_in_range_of(p_from))
        {
            continue;
        }
        double distance = origin.distance_to(candidate->get_world_position());
        if (!nearest || distance < best)
        {
            nearest = candidate;
            best = distance;
        }
    }
    return nearest;
}

void Interactable::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_ENTER_TREE)
    {
        instances.push_back(ObjectID(get_instance_id()));
    }
    else if (p_what == NOTIFICATION_EXIT_TREE)
    {
        int64_t index = instances.find(ObjectID(get_instance_id()));
        if (index >= 0)
        {
            instances.remove_at(index);
        }
    }
}

void Interactable::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_prompt", "prompt"), &Interactable::set_prompt);
    ClassDB::bind_method(D_METHOD("get_prompt"), &Interactable::get_prompt);
    ClassDB::bind_method(D_METHOD("set_interaction_range", "range"), &Interactable::set_interaction_range);
    ClassDB::bind_method(D_METHOD("get_interaction_range"), &Interactable::get_interaction_range);
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &Interactable::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &Interactable::is_enabled);
    ClassDB::bind_method(D_METHOD("set_one_shot", "one_shot"), &Interactable::set_one_shot);
    ClassDB::bind_method(D_METHOD("is_one_shot"), &Interactable::is_one_shot);
    ClassDB::bind_method(D_METHOD("interact", "by"), &Interactable::interact);
    ClassDB::bind_method(D_METHOD("get_world_position"), &Interactable::get_world_position);
    ClassDB::bind_method(D_METHOD("is_in_range_of", "node"), &Interactable::is_in_range_of);
    ClassDB::bind_static_method("Interactable", D_METHOD("find_nearest", "from"), &Interactable::find_nearest);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "prompt"), "set_prompt", "get_prompt");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "interaction_range", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater"), "set_interaction_range", "get_interaction_range");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "one_shot"), "set_one_shot", "is_one_shot");

    ADD_SIGNAL(MethodInfo("interacted", PropertyInfo(Variant::OBJECT, "by", PROPERTY_HINT_NODE_TYPE, "Node")));
}
