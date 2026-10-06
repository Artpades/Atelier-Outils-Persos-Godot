#include "water.hpp"

#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

LocalVector<ObjectID> WaterArea2D::waters;
LocalVector<ObjectID> WaterArea3D::waters;

namespace
{
template <typename T>
void register_instance(LocalVector<ObjectID> &r_list, T *p_node, bool p_enter)
{
    ObjectID id(p_node->get_instance_id());
    int64_t index = r_list.find(id);
    if (p_enter && index < 0)
    {
        r_list.push_back(id);
    }
    else if (!p_enter && index >= 0)
    {
        r_list.remove_at(index);
    }
}

template <typename T>
void bind_water_common(Variant::Type p_vector_type)
{
    ClassDB::bind_method(D_METHOD("set_buoyancy", "value"), &T::set_buoyancy);
    ClassDB::bind_method(D_METHOD("get_buoyancy"), &T::get_buoyancy);
    ClassDB::bind_method(D_METHOD("set_float_depth", "value"), &T::set_float_depth);
    ClassDB::bind_method(D_METHOD("get_float_depth"), &T::get_float_depth);
    ClassDB::bind_method(D_METHOD("set_linear_drag", "value"), &T::set_linear_drag);
    ClassDB::bind_method(D_METHOD("get_linear_drag"), &T::get_linear_drag);
    ClassDB::bind_method(D_METHOD("set_angular_drag", "value"), &T::set_angular_drag);
    ClassDB::bind_method(D_METHOD("get_angular_drag"), &T::get_angular_drag);
    ClassDB::bind_method(D_METHOD("set_current", "current"), &T::set_current);
    ClassDB::bind_method(D_METHOD("get_current"), &T::get_current);
    ClassDB::bind_method(D_METHOD("set_surface_offset", "offset"), &T::set_surface_offset);
    ClassDB::bind_method(D_METHOD("get_surface_offset"), &T::get_surface_offset);
    ClassDB::bind_method(D_METHOD("get_surface_y"), &T::get_surface_y);
    ClassDB::bind_method(D_METHOD("get_depth", "point"), &T::get_depth);
    ClassDB::bind_static_method(T::get_class_static(), D_METHOD("find_containing", "body"), &T::find_containing);

    StringName cls = T::get_class_static();
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "buoyancy", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater"), "set_buoyancy", "get_buoyancy");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "float_depth", PROPERTY_HINT_RANGE, "0.001,500,0.001,or_greater"), "set_float_depth", "get_float_depth");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "linear_drag", PROPERTY_HINT_RANGE, "0,20,0.01,or_greater"), "set_linear_drag", "get_linear_drag");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "angular_drag", PROPERTY_HINT_RANGE, "0,20,0.01,or_greater"), "set_angular_drag", "get_angular_drag");
    ClassDB::add_property(cls, PropertyInfo(p_vector_type, "current"), "set_current", "get_current");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "surface_offset"), "set_surface_offset", "get_surface_offset");
}

template <typename T>
void bind_swim_common()
{
    ClassDB::bind_method(D_METHOD("set_swim_speed", "value"), &T::set_swim_speed);
    ClassDB::bind_method(D_METHOD("get_swim_speed"), &T::get_swim_speed);
    ClassDB::bind_method(D_METHOD("set_acceleration", "value"), &T::set_acceleration);
    ClassDB::bind_method(D_METHOD("get_acceleration"), &T::get_acceleration);
    ClassDB::bind_method(D_METHOD("set_drag", "value"), &T::set_drag);
    ClassDB::bind_method(D_METHOD("get_drag"), &T::get_drag);
    ClassDB::bind_method(D_METHOD("set_buoyancy", "value"), &T::set_buoyancy);
    ClassDB::bind_method(D_METHOD("get_buoyancy"), &T::get_buoyancy);
    ClassDB::bind_method(D_METHOD("set_float_depth", "value"), &T::set_float_depth);
    ClassDB::bind_method(D_METHOD("get_float_depth"), &T::get_float_depth);
    ClassDB::bind_method(D_METHOD("set_surface_tolerance", "value"), &T::set_surface_tolerance);
    ClassDB::bind_method(D_METHOD("get_surface_tolerance"), &T::get_surface_tolerance);
    ClassDB::bind_method(D_METHOD("set_jump_out_velocity", "value"), &T::set_jump_out_velocity);
    ClassDB::bind_method(D_METHOD("get_jump_out_velocity"), &T::get_jump_out_velocity);
    ClassDB::bind_method(D_METHOD("set_stroke_velocity", "value"), &T::set_stroke_velocity);
    ClassDB::bind_method(D_METHOD("get_stroke_velocity"), &T::get_stroke_velocity);
    ClassDB::bind_method(D_METHOD("get_water"), &T::get_water);

    StringName cls = T::get_class_static();
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "swim_speed", PROPERTY_HINT_RANGE, "0,2000,0.01,or_greater"), "set_swim_speed", "get_swim_speed");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "acceleration", PROPERTY_HINT_RANGE, "0,10000,0.01,or_greater"), "set_acceleration", "get_acceleration");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "drag", PROPERTY_HINT_RANGE, "0,20,0.01,or_greater"), "set_drag", "get_drag");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "buoyancy", PROPERTY_HINT_RANGE, "0,5000,0.01,or_greater"), "set_buoyancy", "get_buoyancy");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "float_depth"), "set_float_depth", "get_float_depth");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "surface_tolerance", PROPERTY_HINT_RANGE, "0,500,0.01,or_greater"), "set_surface_tolerance", "get_surface_tolerance");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "jump_out_velocity", PROPERTY_HINT_RANGE, "0,3000,0.01,or_greater"), "set_jump_out_velocity", "get_jump_out_velocity");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "stroke_velocity", PROPERTY_HINT_RANGE, "0,3000,0.01,or_greater"), "set_stroke_velocity", "get_stroke_velocity");
    ClassDB::add_signal(cls, MethodInfo("jumped_out"));
    ClassDB::add_signal(cls, MethodInfo("stroked"));
}
} // namespace

// =====================================================================================================
// 🌊 WaterArea2D
// =====================================================================================================

double WaterArea2D::get_surface_y() const
{
    for (int i = 0; i < get_child_count(); i++)
    {
        CollisionShape2D *shape = Object::cast_to<CollisionShape2D>(get_child(i));
        Ref<RectangleShape2D> rectangle = shape ? Ref<RectangleShape2D>(shape->get_shape()) : Ref<RectangleShape2D>();
        if (rectangle.is_valid())
        {
            Transform2D transform = shape->is_inside_tree() ? shape->get_global_transform() : get_transform() * shape->get_transform();
            return transform.xform(Vector2(0, -rectangle->get_size().y * 0.5f)).y;
        }
    }
    return (is_inside_tree() ? get_global_position().y : get_position().y) + surface_offset;
}

WaterArea2D *WaterArea2D::find_containing(Node *p_body)
{
    for (uint32_t i = 0; p_body && i < waters.size(); i++)
    {
        WaterArea2D *water = Object::cast_to<WaterArea2D>(ObjectDB::get_instance(waters[i]));
        if (water && water->overlaps_body(p_body))
        {
            return water;
        }
    }
    return nullptr;
}

void WaterArea2D::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_EXIT_TREE)
    {
        register_instance(waters, this, p_what == NOTIFICATION_ENTER_TREE);
        set_physics_process(true);
        return;
    }
    if (p_what != NOTIFICATION_PHYSICS_PROCESS)
    {
        return;
    }
    // Objets flottants : poussée proportionnelle à l'immersion, freinage, courant.
    TypedArray<Node2D> bodies = get_overlapping_bodies();
    for (int i = 0; i < bodies.size(); i++)
    {
        RigidBody2D *rigid = Object::cast_to<RigidBody2D>(bodies[i]);
        if (!rigid)
        {
            continue;
        }
        double submerged = CLAMP(get_depth(rigid->get_global_position()) / float_depth + 0.5, 0.0, 1.0);
        double mass = rigid->get_mass();
        Vector2 gravity = rigid->get_gravity();
        rigid->apply_central_force(-gravity * buoyancy * submerged * mass);
        rigid->apply_central_force(-rigid->get_linear_velocity() * linear_drag * submerged * mass);
        rigid->apply_central_force(current * submerged * mass);
        rigid->apply_torque(-rigid->get_angular_velocity() * angular_drag * submerged * rigid->get_inertia());
    }
}

void WaterArea2D::_bind_methods() { bind_water_common<WaterArea2D>(Variant::VECTOR2); }

// =====================================================================================================
// 🌊 WaterArea3D
// =====================================================================================================

WaterArea3D::WaterArea3D() {}

double WaterArea3D::get_surface_y() const
{
    for (int i = 0; i < get_child_count(); i++)
    {
        CollisionShape3D *shape = Object::cast_to<CollisionShape3D>(get_child(i));
        Ref<BoxShape3D> box = shape ? Ref<BoxShape3D>(shape->get_shape()) : Ref<BoxShape3D>();
        if (box.is_valid())
        {
            Transform3D transform = shape->is_inside_tree() ? shape->get_global_transform() : get_transform() * shape->get_transform();
            return transform.xform(Vector3(0, box->get_size().y * 0.5f, 0)).y;
        }
    }
    return (is_inside_tree() ? get_global_position().y : get_position().y) + surface_offset;
}

WaterArea3D *WaterArea3D::find_containing(Node *p_body)
{
    for (uint32_t i = 0; p_body && i < waters.size(); i++)
    {
        WaterArea3D *water = Object::cast_to<WaterArea3D>(ObjectDB::get_instance(waters[i]));
        if (water && water->overlaps_body(p_body))
        {
            return water;
        }
    }
    return nullptr;
}

void WaterArea3D::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_EXIT_TREE)
    {
        register_instance(waters, this, p_what == NOTIFICATION_ENTER_TREE);
        set_physics_process(true);
        return;
    }
    if (p_what != NOTIFICATION_PHYSICS_PROCESS)
    {
        return;
    }
    TypedArray<Node3D> bodies = get_overlapping_bodies();
    for (int i = 0; i < bodies.size(); i++)
    {
        RigidBody3D *rigid = Object::cast_to<RigidBody3D>(bodies[i]);
        if (!rigid)
        {
            continue;
        }
        double submerged = CLAMP(get_depth(rigid->get_global_position()) / float_depth + 0.5, 0.0, 1.0);
        double mass = rigid->get_mass();
        rigid->apply_central_force(-rigid->get_gravity() * buoyancy * submerged * mass);
        rigid->apply_central_force(-rigid->get_linear_velocity() * linear_drag * submerged * mass);
        rigid->apply_central_force(current * submerged * mass);
        rigid->apply_torque(-rigid->get_angular_velocity() * angular_drag * submerged * mass);
    }
}

void WaterArea3D::_bind_methods() { bind_water_common<WaterArea3D>(Variant::VECTOR3); }

// =====================================================================================================
// 🏊 SwimMovement2D
// =====================================================================================================

void SwimMovement2D::_physics_step(double p_delta)
{
    CharacterBody2D *character = Object::cast_to<CharacterBody2D>(get_body());
    if (!character)
    {
        return;
    }
    WaterArea2D *water = get_water();
    Vector2 velocity = character->get_velocity();
    if (!water)
    {
        velocity += character->get_gravity() * p_delta; // sorti de l'eau sans changer de contexte
        character->set_velocity(velocity);
        character->move_and_slide();
        return;
    }
    double depth = water->get_depth(character->get_global_position());
    Vector2 target = move_input * swim.swim_speed;
    velocity.x = Math::move_toward(velocity.x, target.x, float(swim.acceleration * p_delta));
    if (Math::abs(move_input.y) > 0.1f)
    {
        velocity.y = Math::move_toward(velocity.y, target.y, float(swim.acceleration * p_delta));
    }
    else
    {
        // Sans commande verticale : retour vers la profondeur d'équilibre (flottaison).
        double offset = CLAMP((depth - swim.float_depth) / MAX(swim.float_depth, 1.0), -1.0, 1.0);
        velocity.y -= offset * swim.buoyancy * p_delta;
    }
    // Freinage de l'eau sur les axes non commandés (la nage commandée atteint swim_speed).
    double resistance = MAX(0.0, 1.0 - swim.drag * p_delta);
    if (Math::abs(move_input.x) < 0.1f)
    {
        velocity.x *= resistance;
    }
    if (Math::abs(move_input.y) < 0.1f)
    {
        velocity.y *= resistance;
    }
    velocity += water->get_current() * p_delta + external_acceleration_2d(character) * p_delta;
    if (has_jump_request(true))
    {
        if (depth <= swim.surface_tolerance)
        {
            velocity.y = -swim.jump_out_velocity;
            emit_signal("jumped_out");
        }
        else
        {
            velocity.y = MIN(velocity.y, float(-swim.stroke_velocity));
            emit_signal("stroked");
        }
    }
    character->set_velocity(velocity);
    character->move_and_slide();
}

void SwimMovement2D::_bind_methods() { bind_swim_common<SwimMovement2D>(); }

// =====================================================================================================
// 🏊 SwimMovement3D
// =====================================================================================================

SwimMovement3D::SwimMovement3D()
{
    swim.swim_speed = 4.0;
    swim.acceleration = 20.0;
    swim.buoyancy = 6.0;
    swim.float_depth = 0.6;
    swim.surface_tolerance = 0.8;
    swim.jump_out_velocity = 5.0;
    swim.stroke_velocity = 3.0;
}

void SwimMovement3D::_physics_step(double p_delta)
{
    CharacterBody3D *character = Object::cast_to<CharacterBody3D>(get_body());
    if (!character)
    {
        return;
    }
    WaterArea3D *water = get_water();
    Vector3 velocity = character->get_velocity();
    if (!water)
    {
        velocity += character->get_gravity() * p_delta;
        character->set_velocity(velocity);
        character->move_and_slide();
        return;
    }
    Vector2 input = move_input;
    if (camera_relative)
    {
        Camera3D *camera = get_viewport() ? get_viewport()->get_camera_3d() : nullptr;
        if (camera)
        {
            Basis basis = camera->get_global_transform().basis;
            Vector3 forward = -basis.get_column(2);
            Vector3 right = basis.get_column(0);
            forward.y = 0.0f;
            right.y = 0.0f;
            Vector3 direction = right.normalized() * input.x + forward.normalized() * -input.y;
            input = Vector2(direction.x, direction.z).limit_length(1.0f);
        }
    }
    double depth = water->get_depth(character->get_global_position());
    float vertical = Math::abs(vertical_input) > 0.1f ? vertical_input : (jump_held ? 1.0f : 0.0f);
    Vector2 horizontal(velocity.x, velocity.z);
    horizontal = horizontal.move_toward(input * swim.swim_speed, swim.acceleration * p_delta);
    velocity.x = horizontal.x;
    velocity.z = horizontal.y;
    if (vertical != 0.0f && !(vertical > 0.0f && depth <= swim.float_depth))
    {
        velocity.y = Math::move_toward(velocity.y, float(vertical * swim.swim_speed), float(swim.acceleration * p_delta));
    }
    else
    {
        double offset = CLAMP((depth - swim.float_depth) / MAX(swim.float_depth, 0.01), -1.0, 1.0);
        velocity.y += offset * swim.buoyancy * p_delta;
    }
    double resistance = MAX(0.0, 1.0 - swim.drag * p_delta);
    if (input.length_squared() < 0.01f)
    {
        velocity.x *= resistance;
        velocity.z *= resistance;
    }
    if (vertical == 0.0f)
    {
        velocity.y *= resistance;
    }
    velocity += water->get_current() * p_delta + external_acceleration_3d(character) * p_delta;
    if (has_jump_request(true) && depth <= swim.surface_tolerance)
    {
        velocity.y = swim.jump_out_velocity;
        emit_signal("jumped_out");
    }
    character->set_velocity(velocity);
    character->move_and_slide();
}

void SwimMovement3D::_bind_methods()
{
    bind_swim_common<SwimMovement3D>();
    ClassDB::bind_method(D_METHOD("set_camera_relative", "relative"), &SwimMovement3D::set_camera_relative);
    ClassDB::bind_method(D_METHOD("get_camera_relative"), &SwimMovement3D::get_camera_relative);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "camera_relative"), "set_camera_relative", "get_camera_relative");
}
