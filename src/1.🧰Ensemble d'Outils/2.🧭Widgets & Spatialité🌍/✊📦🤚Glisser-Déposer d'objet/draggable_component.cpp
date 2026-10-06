#include "draggable_component.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void DraggableComponent::save_initial_position(Node *p_node)
{
    Node *target = p_node ? p_node : get_target_node();
    if (Node2D *node_2d = Object::cast_to<Node2D>(target))
    {
        initial_position_2d = node_2d->get_global_position();
    }
    else if (Control *control = Object::cast_to<Control>(target))
    {
        initial_position_2d = control->get_global_position();
    }
    else if (Node3D *node_3d = Object::cast_to<Node3D>(target))
    {
        initial_position_3d = node_3d->get_global_position();
    }
}

// 🔗 Liaisons

void DraggableComponent::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_target_node_path", "path"), &DraggableComponent::set_target_node_path);
    ClassDB::bind_method(D_METHOD("get_target_node_path"), &DraggableComponent::get_target_node_path);
    ClassDB::bind_method(D_METHOD("set_target_node", "node"), &DraggableComponent::set_target_node);
    ClassDB::bind_method(D_METHOD("get_target_node"), &DraggableComponent::get_target_node);
    ClassDB::bind_method(D_METHOD("set_local_bounds_2d", "bounds"), &DraggableComponent::set_local_bounds_2d);
    ClassDB::bind_method(D_METHOD("get_local_bounds_2d"), &DraggableComponent::get_local_bounds_2d);
    ClassDB::bind_method(D_METHOD("set_bounds_3d", "bounds"), &DraggableComponent::set_bounds_3d);
    ClassDB::bind_method(D_METHOD("get_bounds_3d"), &DraggableComponent::get_bounds_3d);
    ClassDB::bind_method(D_METHOD("set_snap_to_cursor", "enabled"), &DraggableComponent::set_snap_to_cursor);
    ClassDB::bind_method(D_METHOD("get_snap_to_cursor"), &DraggableComponent::get_snap_to_cursor);
    ClassDB::bind_method(D_METHOD("set_drag_plane_3d", "plane"), &DraggableComponent::set_drag_plane_3d);
    ClassDB::bind_method(D_METHOD("get_drag_plane_3d"), &DraggableComponent::get_drag_plane_3d);
    ClassDB::bind_method(D_METHOD("set_release_behavior", "behavior"), &DraggableComponent::set_release_behavior);
    ClassDB::bind_method(D_METHOD("get_release_behavior"), &DraggableComponent::get_release_behavior);
    ClassDB::bind_method(D_METHOD("set_snap_behavior", "mode"), &DraggableComponent::set_snap_behavior);
    ClassDB::bind_method(D_METHOD("get_snap_behavior"), &DraggableComponent::get_snap_behavior);
    ClassDB::bind_method(D_METHOD("set_snap_distance_threshold", "distance"), &DraggableComponent::set_snap_distance_threshold);
    ClassDB::bind_method(D_METHOD("get_snap_distance_threshold"), &DraggableComponent::get_snap_distance_threshold);
    ClassDB::bind_method(D_METHOD("set_snap_targets", "targets"), &DraggableComponent::set_snap_targets);
    ClassDB::bind_method(D_METHOD("get_snap_targets"), &DraggableComponent::get_snap_targets);
    ClassDB::bind_method(D_METHOD("set_initial_position_2d", "position"), &DraggableComponent::set_initial_position_2d);
    ClassDB::bind_method(D_METHOD("get_initial_position_2d"), &DraggableComponent::get_initial_position_2d);
    ClassDB::bind_method(D_METHOD("set_initial_position_3d", "position"), &DraggableComponent::set_initial_position_3d);
    ClassDB::bind_method(D_METHOD("get_initial_position_3d"), &DraggableComponent::get_initial_position_3d);
    ClassDB::bind_method(D_METHOD("save_initial_position", "node"), &DraggableComponent::save_initial_position, DEFVAL(Variant()));

    BIND_ENUM_CONSTANT(DRAG_PLANE_CAMERA);
    BIND_ENUM_CONSTANT(DRAG_PLANE_OBJECT_X);
    BIND_ENUM_CONSTANT(DRAG_PLANE_OBJECT_Y);
    BIND_ENUM_CONSTANT(DRAG_PLANE_OBJECT_Z);
    BIND_ENUM_CONSTANT(DRAG_PLANE_WORLD_X);
    BIND_ENUM_CONSTANT(DRAG_PLANE_WORLD_Y);
    BIND_ENUM_CONSTANT(DRAG_PLANE_WORLD_Z);

    BIND_ENUM_CONSTANT(RELEASE_RETURN_TO_INITIAL);
    BIND_ENUM_CONSTANT(RELEASE_SNAP_TO_VALID_POINT);
    BIND_ENUM_CONSTANT(RELEASE_LEAVE_IN_PLACE);
    BIND_ENUM_CONSTANT(RELEASE_DO_NOTHING);

    BIND_ENUM_CONSTANT(SNAP_NONE);
    BIND_ENUM_CONSTANT(SNAP_WHEN_CLOSE);
    BIND_ENUM_CONSTANT(SNAP_ALWAYS);

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_node_path"), "set_target_node_path", "get_target_node_path");
    // Accès par code uniquement (non sauvegardé, absent de l'inspecteur) : résolu depuis target_node_path.
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target_node", PROPERTY_HINT_NODE_TYPE, "Node", PROPERTY_USAGE_NONE), "set_target_node", "get_target_node");

    ADD_GROUP(U"🎯 Détection 2D", "");
    ADD_PROPERTY(PropertyInfo(Variant::RECT2, "local_bounds_2d", PROPERTY_HINT_NONE, "suffix:px"), "set_local_bounds_2d", "get_local_bounds_2d");
    ADD_GROUP(U"🎯 Détection 3D", "");
    ADD_PROPERTY(PropertyInfo(Variant::AABB, "bounds_3d", PROPERTY_HINT_NONE, "suffix:m"), "set_bounds_3d", "get_bounds_3d");

    ADD_GROUP(U"✊ Glissement", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "snap_to_cursor"), "set_snap_to_cursor", "get_snap_to_cursor");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "drag_plane_3d", PROPERTY_HINT_ENUM, "Camera,Object X,Object Y,Object Z,World X,World Y,World Z"), "set_drag_plane_3d", "get_drag_plane_3d");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "release_behavior", PROPERTY_HINT_ENUM, "Return To Initial,Snap To Valid Point,Leave In Place,Do Nothing"), "set_release_behavior", "get_release_behavior");

    ADD_GROUP(U"🧲 Aimantage", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "snap_behavior", PROPERTY_HINT_ENUM, "None,Snap When Close,Always Snap"), "set_snap_behavior", "get_snap_behavior");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "snap_distance_threshold", PROPERTY_HINT_RANGE, "0,500,0.1,or_greater"), "set_snap_distance_threshold", "get_snap_distance_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "snap_targets", PROPERTY_HINT_ARRAY_TYPE, "NodePath"), "set_snap_targets", "get_snap_targets");

    // État d'exécution (non sauvegardé) : mémorisé par save_initial_position.
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "initial_position_2d", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_initial_position_2d", "get_initial_position_2d");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "initial_position_3d", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_initial_position_3d", "get_initial_position_3d");
}
