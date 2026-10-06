#include "fx_targeter.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/templates/local_vector.hpp>

using namespace godot;

void FXTargeter::_process(double p_delta, Node *p_target)
{
    if (!p_target || nodes_to_target.is_empty())
    {
        return;
    }

    if (Node2D *node_2d = Object::cast_to<Node2D>(p_target))
    {
        _process_2d(node_2d, (float)p_delta);
    }
    else if (Node3D *node_3d = Object::cast_to<Node3D>(p_target))
    {
        _process_3d(node_3d, (float)p_delta);
    }
}

// 🟦 2D

void FXTargeter::_process_2d(Node2D *p_node, float p_delta)
{
    Vector2 target_center;
    int valid_count = 0;
    for (int64_t i = 0; i < nodes_to_target.size(); i++)
    {
        if (Node2D *target = Object::cast_to<Node2D>(_get_node_from_player(nodes_to_target[i])))
        {
            target_center += target->get_global_position();
            valid_count++;
        }
    }
    if (valid_count == 0)
    {
        return;
    }
    target_center /= (float)valid_count;

    // Même convention avec ou sans interpolation : l'axe +X pointe vers la cible (comme look_at)
    const float target_angle = (target_center - p_node->get_global_position()).angle();
    const float angle = add_lerp ? Math::lerp_angle(p_node->get_global_rotation(), target_angle, Math::min(lerp_speed * p_delta, 1.0f)) : target_angle;
    p_node->set_global_rotation(angle);
}

// 🧊 3D

void FXTargeter::_process_3d(Node3D *p_node, float p_delta)
{
    Vector3 target_center;
    LocalVector<Vector3> valid_positions;
    for (int64_t i = 0; i < nodes_to_target.size(); i++)
    {
        if (Node3D *target = Object::cast_to<Node3D>(_get_node_from_player(nodes_to_target[i])))
        {
            const Vector3 position = target->get_global_position();
            target_center += position;
            valid_positions.push_back(position);
        }
    }
    if (valid_positions.is_empty())
    {
        return;
    }
    target_center /= (float)valid_positions.size();

    float max_spread = 0.1f;
    for (uint32_t i = 0; i < valid_positions.size(); i++)
    {
        for (uint32_t j = i + 1; j < valid_positions.size(); j++)
        {
            max_spread = Math::max(max_spread, valid_positions[i].distance_to(valid_positions[j]));
        }
    }

    // Orientation (sans look_at : il échoue si la cible est confondue avec le nœud ou alignée avec le haut)
    const Vector3 direction = target_center - p_node->get_global_position();
    if (direction.length_squared() > 1e-6f)
    {
        const Vector3 forward = direction.normalized();
        const Vector3 up = Math::abs(forward.dot(Vector3(0, 1, 0))) > 0.999f ? Vector3(0, 0, 1) : Vector3(0, 1, 0);
        const Basis current = p_node->get_global_basis();
        const Quaternion target_rotation = Basis::looking_at(forward, up).get_rotation_quaternion();
        const Quaternion rotation = add_lerp ? current.get_rotation_quaternion().slerp(target_rotation, Math::min(lerp_speed * p_delta, 1.0f)) : target_rotation;
        p_node->set_global_basis(Basis(rotation) * Basis::from_scale(current.get_scale()));
    }

    Camera3D *camera = Object::cast_to<Camera3D>(p_node);
    if (camera && enable_camera_3d_depth_control)
    {
        const float fov = Math::deg_to_rad(camera->get_fov());
        const float ideal_distance = Math::clamp(max_spread * depth_margin / (2.0f * Math::tan(fov / 2.0f)), min_distance, max_distance);

        const Vector3 camera_forward = -camera->get_global_basis().get_column(2).normalized();
        const Vector3 target_position = target_center - camera_forward * ideal_distance;
        camera->set_global_position(add_lerp ? camera->get_global_position().lerp(target_position, Math::min(lerp_speed * p_delta, 1.0f)) : target_position);
    }
}

// 🔗 Liaisons

void FXTargeter::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_nodes_to_target", "value"), &FXTargeter::set_nodes_to_target);
    ClassDB::bind_method(D_METHOD("get_nodes_to_target"), &FXTargeter::get_nodes_to_target);
    ClassDB::bind_method(D_METHOD("set_add_lerp", "value"), &FXTargeter::set_add_lerp);
    ClassDB::bind_method(D_METHOD("get_add_lerp"), &FXTargeter::get_add_lerp);
    ClassDB::bind_method(D_METHOD("set_lerp_speed", "value"), &FXTargeter::set_lerp_speed);
    ClassDB::bind_method(D_METHOD("get_lerp_speed"), &FXTargeter::get_lerp_speed);
    ClassDB::bind_method(D_METHOD("set_enable_camera_3d_depth_control", "value"), &FXTargeter::set_enable_camera_3d_depth_control);
    ClassDB::bind_method(D_METHOD("get_enable_camera_3d_depth_control"), &FXTargeter::get_enable_camera_3d_depth_control);
    ClassDB::bind_method(D_METHOD("set_min_distance", "value"), &FXTargeter::set_min_distance);
    ClassDB::bind_method(D_METHOD("get_min_distance"), &FXTargeter::get_min_distance);
    ClassDB::bind_method(D_METHOD("set_max_distance", "value"), &FXTargeter::set_max_distance);
    ClassDB::bind_method(D_METHOD("get_max_distance"), &FXTargeter::get_max_distance);
    ClassDB::bind_method(D_METHOD("set_depth_margin", "value"), &FXTargeter::set_depth_margin);
    ClassDB::bind_method(D_METHOD("get_depth_margin"), &FXTargeter::get_depth_margin);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "nodes_to_target", PROPERTY_HINT_ARRAY_TYPE, "NodePath"), "set_nodes_to_target", "get_nodes_to_target");

    ADD_GROUP("Interpolation", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "add_lerp"), "set_add_lerp", "get_add_lerp");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lerp_speed", PROPERTY_HINT_RANGE, "0,20,0.1,or_greater"), "set_lerp_speed", "get_lerp_speed");

    ADD_GROUP("Camera3D Distance Control", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_camera_3d_depth_control"), "set_enable_camera_3d_depth_control", "get_enable_camera_3d_depth_control");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_distance", PROPERTY_HINT_RANGE, "0,100,0.1,or_greater,suffix:m"), "set_min_distance", "get_min_distance");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0,500,0.1,or_greater,suffix:m"), "set_max_distance", "get_max_distance");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "depth_margin", PROPERTY_HINT_RANGE, "0.1,5,0.05,or_greater"), "set_depth_margin", "get_depth_margin");
}
