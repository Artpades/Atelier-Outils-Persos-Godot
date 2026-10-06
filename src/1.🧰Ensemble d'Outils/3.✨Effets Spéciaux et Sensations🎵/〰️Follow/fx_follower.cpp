#include "fx_follower.hpp"

#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/rect2.hpp>

using namespace godot;

void FXFollower::_process(double p_delta, Node *p_target)
{
    if (!p_target || targets.is_empty())
    {
        return;
    }

    if (Node2D *node_2d = Object::cast_to<Node2D>(p_target))
    {
        _update_2d(node_2d, (float)p_delta);
    }
    else if (Node3D *node_3d = Object::cast_to<Node3D>(p_target))
    {
        _update_3d(node_3d, (float)p_delta);
    }
}

float FXFollower::_weight(float p_delta) const { return Math::min(follow_speed * p_delta, 1.0f); }

// 🟦 2D

void FXFollower::_update_2d(Node2D *p_node, float p_delta)
{
    Vector2 target_center;
    int valid_count = 0;
    Rect2 bounds; // englobe les nœuds suivis (pour le zoom)
    for (int64_t i = 0; i < targets.size(); i++)
    {
        if (Node2D *target = Object::cast_to<Node2D>(_get_node_from_player(targets[i])))
        {
            const Vector2 position = target->get_global_position();
            target_center += position;
            bounds = valid_count == 0 ? Rect2(position, Vector2()) : bounds.expand(position);
            valid_count++;
        }
    }
    if (valid_count == 0)
    {
        return;
    }
    target_center /= (float)valid_count;

    p_node->set_global_position(use_interpolation ? p_node->get_global_position().lerp(target_center, _weight(p_delta)) : target_center);

    Camera2D *camera = Object::cast_to<Camera2D>(p_node);
    if (!camera || !auto_zoom_2d || valid_count < 2 || !camera->is_inside_tree())
    {
        return;
    }

    const Vector2 screen_size = camera->get_viewport()->get_visible_rect().size;
    const Vector2 required = bounds.size + Vector2(zoom_margin, zoom_margin) * 2.0f;
    const float zoom_factor = Math::min(screen_size.x / Math::max(required.x, 1.0f), screen_size.y / Math::max(required.y, 1.0f));
    const Vector2 target_zoom = Vector2(zoom_factor, zoom_factor).clamp(min_zoom, max_zoom);

    camera->set_zoom(use_interpolation ? camera->get_zoom().lerp(target_zoom, _weight(p_delta)) : target_zoom);
}

// 🧊 3D

void FXFollower::_update_3d(Node3D *p_node, float p_delta)
{
    Vector3 target_center;
    int valid_count = 0;
    for (int64_t i = 0; i < targets.size(); i++)
    {
        if (Node3D *target = Object::cast_to<Node3D>(_get_node_from_player(targets[i])))
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

    p_node->set_global_position(use_interpolation ? p_node->get_global_position().lerp(target_center, _weight(p_delta)) : target_center);
}

// 🔗 Liaisons

void FXFollower::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_targets", "value"), &FXFollower::set_targets);
    ClassDB::bind_method(D_METHOD("get_targets"), &FXFollower::get_targets);
    ClassDB::bind_method(D_METHOD("set_use_interpolation", "value"), &FXFollower::set_use_interpolation);
    ClassDB::bind_method(D_METHOD("get_use_interpolation"), &FXFollower::get_use_interpolation);
    ClassDB::bind_method(D_METHOD("set_follow_speed", "value"), &FXFollower::set_follow_speed);
    ClassDB::bind_method(D_METHOD("get_follow_speed"), &FXFollower::get_follow_speed);
    ClassDB::bind_method(D_METHOD("set_auto_zoom_2d", "value"), &FXFollower::set_auto_zoom_2d);
    ClassDB::bind_method(D_METHOD("get_auto_zoom_2d"), &FXFollower::get_auto_zoom_2d);
    ClassDB::bind_method(D_METHOD("set_min_zoom", "value"), &FXFollower::set_min_zoom);
    ClassDB::bind_method(D_METHOD("get_min_zoom"), &FXFollower::get_min_zoom);
    ClassDB::bind_method(D_METHOD("set_max_zoom", "value"), &FXFollower::set_max_zoom);
    ClassDB::bind_method(D_METHOD("get_max_zoom"), &FXFollower::get_max_zoom);
    ClassDB::bind_method(D_METHOD("set_zoom_margin", "value"), &FXFollower::set_zoom_margin);
    ClassDB::bind_method(D_METHOD("get_zoom_margin"), &FXFollower::get_zoom_margin);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "targets", PROPERTY_HINT_ARRAY_TYPE, "NodePath"), "set_targets", "get_targets");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_interpolation"), "set_use_interpolation", "get_use_interpolation");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "follow_speed", PROPERTY_HINT_RANGE, "0,20,0.1"), "set_follow_speed", "get_follow_speed");

    ADD_GROUP("Camera 2D Zoom", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_zoom_2d"), "set_auto_zoom_2d", "get_auto_zoom_2d");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "min_zoom", PROPERTY_HINT_LINK), "set_min_zoom", "get_min_zoom");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "max_zoom", PROPERTY_HINT_LINK), "set_max_zoom", "get_max_zoom");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "zoom_margin", PROPERTY_HINT_RANGE, "0,1000,1,or_greater,suffix:px"), "set_zoom_margin", "get_zoom_margin");
}
