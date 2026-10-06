#include "wall_movement.hpp"

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace
{
template <typename T>
void bind_wall_common()
{
    ClassDB::bind_method(D_METHOD("set_slide_speed", "value"), &T::set_slide_speed);
    ClassDB::bind_method(D_METHOD("get_slide_speed"), &T::get_slide_speed);
    ClassDB::bind_method(D_METHOD("set_require_input_toward_wall", "value"), &T::set_require_input_toward_wall);
    ClassDB::bind_method(D_METHOD("get_require_input_toward_wall"), &T::get_require_input_toward_wall);
    ClassDB::bind_method(D_METHOD("set_jump_away_velocity", "value"), &T::set_jump_away_velocity);
    ClassDB::bind_method(D_METHOD("get_jump_away_velocity"), &T::get_jump_away_velocity);
    ClassDB::bind_method(D_METHOD("set_jump_up_velocity", "value"), &T::set_jump_up_velocity);
    ClassDB::bind_method(D_METHOD("get_jump_up_velocity"), &T::get_jump_up_velocity);
    ClassDB::bind_method(D_METHOD("set_can_climb", "value"), &T::set_can_climb);
    ClassDB::bind_method(D_METHOD("get_can_climb"), &T::get_can_climb);
    ClassDB::bind_method(D_METHOD("set_climb_speed", "value"), &T::set_climb_speed);
    ClassDB::bind_method(D_METHOD("get_climb_speed"), &T::get_climb_speed);
    ClassDB::bind_method(D_METHOD("set_air_speed", "value"), &T::set_air_speed);
    ClassDB::bind_method(D_METHOD("get_air_speed"), &T::get_air_speed);
    ClassDB::bind_method(D_METHOD("set_air_acceleration", "value"), &T::set_air_acceleration);
    ClassDB::bind_method(D_METHOD("get_air_acceleration"), &T::get_air_acceleration);
    ClassDB::bind_method(D_METHOD("set_stick_velocity", "value"), &T::set_stick_velocity);
    ClassDB::bind_method(D_METHOD("get_stick_velocity"), &T::get_stick_velocity);
    ClassDB::bind_method(D_METHOD("is_touching_wall"), &T::is_touching_wall);
    ClassDB::bind_method(D_METHOD("is_sliding"), &T::is_sliding);
    ClassDB::bind_method(D_METHOD("is_climbing"), &T::is_climbing);

    StringName cls = T::get_class_static();
    ClassDB::add_property_group(cls, U"🧗 Glissade et saut", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "slide_speed", PROPERTY_HINT_RANGE, "0,2000,0.01,or_greater"), "set_slide_speed", "get_slide_speed");
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "require_input_toward_wall"), "set_require_input_toward_wall", "get_require_input_toward_wall");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "jump_away_velocity", PROPERTY_HINT_RANGE, "0,3000,0.01,or_greater"), "set_jump_away_velocity", "get_jump_away_velocity");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "jump_up_velocity", PROPERTY_HINT_RANGE, "0,3000,0.01,or_greater"), "set_jump_up_velocity", "get_jump_up_velocity");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "stick_velocity", PROPERTY_HINT_RANGE, "0,500,0.01,or_greater"), "set_stick_velocity", "get_stick_velocity");
    ClassDB::add_property_group(cls, U"🪜 Escalade", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "can_climb"), "set_can_climb", "get_can_climb");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "climb_speed", PROPERTY_HINT_RANGE, "0,2000,0.01,or_greater"), "set_climb_speed", "get_climb_speed");
    ClassDB::add_property_group(cls, U"🪂 Hors du mur", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "air_speed", PROPERTY_HINT_RANGE, "0,2000,0.01,or_greater"), "set_air_speed", "get_air_speed");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "air_acceleration", PROPERTY_HINT_RANGE, "0,10000,0.01,or_greater"), "set_air_acceleration", "get_air_acceleration");
    ClassDB::add_signal(cls, MethodInfo("wall_jumped"));
}
} // namespace

// 🧗 2D

bool WallMovement2D::is_touching_wall() const
{
    CharacterBody2D *character = Object::cast_to<CharacterBody2D>(get_body());
    return character && character->is_on_wall() && !character->is_on_floor();
}

void WallMovement2D::_physics_step(double p_delta)
{
    CharacterBody2D *character = Object::cast_to<CharacterBody2D>(get_body());
    if (!character)
    {
        return;
    }
    Vector2 gravity = character->get_gravity();
    Vector2 velocity = character->get_velocity();
    bool touching = is_touching_wall();
    if (touching)
    {
        last_normal = character->get_wall_normal();
    }
    bool pressing = touching && move_input.x * last_normal.x < -0.1f;
    sliding = false;
    climbing = touching && wall.can_climb;

    if (touching && has_jump_request(true))
    {
        velocity = Vector2(last_normal.x * wall.jump_away_velocity, -wall.jump_up_velocity);
        climbing = false;
        emit_signal("wall_jumped");
    }
    else if (climbing)
    {
        velocity.y = move_input.y * wall.climb_speed;
        velocity.x = -last_normal.x * wall.stick_velocity;
    }
    else if (touching && (pressing || !wall.require_input_toward_wall))
    {
        sliding = true;
        velocity.y = MIN(velocity.y + gravity.y * float(p_delta), float(wall.slide_speed));
        velocity.x = -last_normal.x * wall.stick_velocity;
    }
    else
    {
        velocity += gravity * p_delta;
        velocity.x = Math::move_toward(velocity.x, float(move_input.x * wall.air_speed), float(wall.air_acceleration * p_delta));
    }
    velocity += external_acceleration_2d(character) * p_delta;
    character->set_velocity(velocity);
    character->move_and_slide();
}

void WallMovement2D::_bind_methods() { bind_wall_common<WallMovement2D>(); }

// 🧗 3D

WallMovement3D::WallMovement3D()
{
    wall.slide_speed = 2.0;
    wall.jump_away_velocity = 5.0;
    wall.jump_up_velocity = 5.0;
    wall.climb_speed = 2.5;
    wall.air_speed = 5.0;
    wall.air_acceleration = 20.0;
    wall.stick_velocity = 0.5;
}

bool WallMovement3D::is_touching_wall() const
{
    CharacterBody3D *character = Object::cast_to<CharacterBody3D>(get_body());
    return character && character->is_on_wall() && !character->is_on_floor();
}

void WallMovement3D::_physics_step(double p_delta)
{
    CharacterBody3D *character = Object::cast_to<CharacterBody3D>(get_body());
    if (!character)
    {
        return;
    }
    Vector3 gravity = character->get_gravity();
    Vector3 velocity = character->get_velocity();
    bool touching = is_touching_wall();
    Vector3 normal = touching ? character->get_wall_normal() : Vector3();
    Vector3 flat_normal = Vector3(normal.x, 0, normal.z).normalized();
    Vector3 input(move_input.x, 0, move_input.y); // entrée dans le monde (x, z)
    bool pressing = touching && input.dot(flat_normal) < -0.1f;
    float vertical = Math::abs(vertical_input) > 0.1f ? vertical_input : -move_input.y;
    sliding = false;
    climbing = touching && wall.can_climb;

    if (touching && has_jump_request(true))
    {
        velocity = flat_normal * wall.jump_away_velocity + Vector3(0, wall.jump_up_velocity, 0);
        climbing = false;
        emit_signal("wall_jumped");
    }
    else if (climbing)
    {
        velocity = -flat_normal * wall.stick_velocity + Vector3(0, vertical * wall.climb_speed, 0);
    }
    else if (touching && (pressing || !wall.require_input_toward_wall))
    {
        sliding = true;
        velocity.y = MAX(velocity.y + gravity.y * float(p_delta), float(-wall.slide_speed));
        velocity.x = -flat_normal.x * wall.stick_velocity;
        velocity.z = -flat_normal.z * wall.stick_velocity;
    }
    else
    {
        velocity += gravity * p_delta;
        Vector2 horizontal = Vector2(velocity.x, velocity.z).move_toward(move_input * wall.air_speed, wall.air_acceleration * p_delta);
        velocity.x = horizontal.x;
        velocity.z = horizontal.y;
    }
    velocity += external_acceleration_3d(character) * p_delta;
    character->set_velocity(velocity);
    character->move_and_slide();
}

void WallMovement3D::_bind_methods() { bind_wall_common<WallMovement3D>(); }
