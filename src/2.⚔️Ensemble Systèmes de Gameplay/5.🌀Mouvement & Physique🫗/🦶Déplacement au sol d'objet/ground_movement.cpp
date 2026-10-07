#include "ground_movement.hpp"

#include "../🌬️Recule/knockback.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

bool GroundSettings::try_jump(bool p_on_floor)
{
    if (max_jumps <= 0)
    {
        return false;
    }
    if (p_on_floor || (coyote > 0.0 && jumps_used == 0))
    {
        jumps_used = 1;
    }
    else if (jumps_used == 0 && max_jumps > 1)
    {
        jumps_used = 2; // tombé d'un rebord sans sauter : le premier saut en l'air compte comme le deuxième
    }
    else if (jumps_used > 0 && jumps_used < max_jumps)
    {
        jumps_used++;
    }
    else
    {
        return false;
    }
    coyote = 0.0;
    jump_cut_done = false;
    return true;
}

namespace
{
template <typename T>
void bind_ground_common()
{
    ClassDB::bind_method(D_METHOD("set_max_speed", "value"), &T::set_max_speed);
    ClassDB::bind_method(D_METHOD("get_max_speed"), &T::get_max_speed);
    ClassDB::bind_method(D_METHOD("set_run_multiplier", "value"), &T::set_run_multiplier);
    ClassDB::bind_method(D_METHOD("get_run_multiplier"), &T::get_run_multiplier);
    ClassDB::bind_method(D_METHOD("set_acceleration", "value"), &T::set_acceleration);
    ClassDB::bind_method(D_METHOD("get_acceleration"), &T::get_acceleration);
    ClassDB::bind_method(D_METHOD("set_deceleration", "value"), &T::set_deceleration);
    ClassDB::bind_method(D_METHOD("get_deceleration"), &T::get_deceleration);
    ClassDB::bind_method(D_METHOD("set_air_control", "value"), &T::set_air_control);
    ClassDB::bind_method(D_METHOD("get_air_control"), &T::get_air_control);
    ClassDB::bind_method(D_METHOD("set_jump_velocity", "value"), &T::set_jump_velocity);
    ClassDB::bind_method(D_METHOD("get_jump_velocity"), &T::get_jump_velocity);
    ClassDB::bind_method(D_METHOD("set_gravity_scale", "value"), &T::set_gravity_scale);
    ClassDB::bind_method(D_METHOD("get_gravity_scale"), &T::get_gravity_scale);
    ClassDB::bind_method(D_METHOD("set_max_fall_speed", "value"), &T::set_max_fall_speed);
    ClassDB::bind_method(D_METHOD("get_max_fall_speed"), &T::get_max_fall_speed);
    ClassDB::bind_method(D_METHOD("set_coyote_time", "value"), &T::set_coyote_time);
    ClassDB::bind_method(D_METHOD("get_coyote_time"), &T::get_coyote_time);
    ClassDB::bind_method(D_METHOD("set_jump_cut", "value"), &T::set_jump_cut);
    ClassDB::bind_method(D_METHOD("get_jump_cut"), &T::get_jump_cut);
    ClassDB::bind_method(D_METHOD("set_max_jumps", "value"), &T::set_max_jumps);
    ClassDB::bind_method(D_METHOD("get_max_jumps"), &T::get_max_jumps);
    ClassDB::bind_method(D_METHOD("get_facing"), &T::get_facing);
    ClassDB::bind_method(D_METHOD("is_on_floor"), &T::is_on_floor);
    ClassDB::bind_method(D_METHOD("set_visual_path", "path"), &T::set_visual_path);
    ClassDB::bind_method(D_METHOD("get_visual_path"), &T::get_visual_path);

    StringName cls = T::get_class_static();
    ClassDB::add_property_group(cls, U"🏃 Déplacement", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "max_speed", PROPERTY_HINT_RANGE, "0,2000,0.01,or_greater"), "set_max_speed", "get_max_speed");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "run_multiplier", PROPERTY_HINT_RANGE, "0,5,0.01,or_greater"), "set_run_multiplier", "get_run_multiplier");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "acceleration", PROPERTY_HINT_RANGE, "0,20000,0.01,or_greater"), "set_acceleration", "get_acceleration");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "deceleration", PROPERTY_HINT_RANGE, "0,20000,0.01,or_greater"), "set_deceleration", "get_deceleration");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "air_control", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_air_control", "get_air_control");
    ClassDB::add_property(cls, PropertyInfo(Variant::NODE_PATH, "visual_path"), "set_visual_path", "get_visual_path");
    ClassDB::add_property_group(cls, U"🦘 Saut et gravité", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "jump_velocity", PROPERTY_HINT_RANGE, "0,3000,0.01,or_greater"), "set_jump_velocity", "get_jump_velocity");
    ClassDB::add_property(cls, PropertyInfo(Variant::INT, "max_jumps", PROPERTY_HINT_RANGE, "0,5,1,or_greater"), "set_max_jumps", "get_max_jumps");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "coyote_time", PROPERTY_HINT_RANGE, "0,1,0.01,suffix:s"), "set_coyote_time", "get_coyote_time");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "jump_cut", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_jump_cut", "get_jump_cut");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "gravity_scale", PROPERTY_HINT_RANGE, "-5,5,0.01,or_greater,or_less"), "set_gravity_scale", "get_gravity_scale");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "max_fall_speed", PROPERTY_HINT_RANGE, "0,10000,0.01,or_greater"), "set_max_fall_speed", "get_max_fall_speed");

    ClassDB::add_signal(cls, MethodInfo("jumped"));
    ClassDB::add_signal(cls, MethodInfo("landed"));
}
} // namespace

// =====================================================================================================
// 🦶 2D
// =====================================================================================================

bool GroundMovement2D::is_on_floor() const
{
    CharacterBody2D *character = Object::cast_to<CharacterBody2D>(get_body());
    return character && character->is_on_floor();
}

void GroundMovement2D::_update_visual()
{
    Node2D *visual = visual_path.is_empty() ? nullptr : Object::cast_to<Node2D>(get_node_or_null(visual_path));
    if (!visual || Math::is_zero_approx(facing.x))
    {
        return;
    }
    bool left = facing.x < 0.0f;
    if (visual->get("flip_h").get_type() == Variant::BOOL)
    {
        visual->set("flip_h", left);
    }
    else
    {
        Vector2 scale = visual->get_scale();
        visual->set_scale(Vector2(Math::abs(scale.x) * (left ? -1.0f : 1.0f), scale.y));
    }
}

void GroundMovement2D::_physics_step(double p_delta)
{
    Node *body = get_body();
    const bool platformer = mode == MODE_PLATFORMER;
    Knockback2D *knockback = Knockback2D::active_on(body);
    const Vector2 input = knockback ? Vector2() : move_input;
    const double speed = ground.max_speed * (running ? ground.run_multiplier : 1.0);
    const Vector2 external = external_acceleration_2d(body);

    if (CharacterBody2D *character = Object::cast_to<CharacterBody2D>(body))
    {
        const bool on_floor = platformer && character->is_on_floor();
        if (on_floor)
        {
            ground.coyote = ground.coyote_time;
            ground.jumps_used = 0;
        }
        else
        {
            ground.coyote -= p_delta;
        }
        Vector2 velocity = character->get_velocity();
        if (!platformer)
        {
            double rate = knockback ? knockback->get_friction() : (input.length_squared() > 0.0001f ? ground.acceleration : ground.deceleration);
            velocity = velocity.move_toward(input * speed, rate * p_delta);
        }
        else
        {
            if (!on_floor)
            {
                velocity += character->get_gravity() * ground.gravity_scale * p_delta;
                velocity.y = MIN(velocity.y, float(ground.max_fall_speed));
            }
            double rate = knockback ? knockback->get_friction() : (Math::abs(input.x) > 0.01f ? ground.acceleration : ground.deceleration) * (on_floor ? 1.0 : ground.air_control);
            velocity.x = Math::move_toward(velocity.x, float(input.x * speed), float(rate * p_delta));
            if (!knockback && has_jump_request() && ground.try_jump(on_floor))
            {
                has_jump_request(true);
                velocity.y = -ground.jump_velocity;
                emit_signal("jumped");
            }
            // Hauteur variable : saut relâché pendant la montée → vitesse réduite une fois.
            if (!ground.jump_cut_done && !jump_held && velocity.y < 0.0f)
            {
                velocity.y *= ground.jump_cut;
                ground.jump_cut_done = true;
            }
            if (velocity.y >= 0.0f)
            {
                ground.jump_cut_done = true;
            }
        }
        velocity += external * p_delta;
        character->set_velocity(velocity);
        character->move_and_slide();
        if (platformer)
        {
            bool now_on_floor = character->is_on_floor();
            if (now_on_floor && !ground.was_on_floor)
            {
                emit_signal("landed");
            }
            ground.was_on_floor = now_on_floor;
        }
    }
    else if (RigidBody2D *rigid = Object::cast_to<RigidBody2D>(body))
    {
        // Corps rigide : on pilote sa vitesse (horizontale en plateforme), la gravité reste celle du moteur.
        Vector2 velocity = rigid->get_linear_velocity();
        double rate = input.length_squared() > 0.0001f ? ground.acceleration : ground.deceleration;
        if (platformer)
        {
            velocity.x = Math::move_toward(velocity.x, float(input.x * speed), float(rate * p_delta));
            if (has_jump_request() && rigid->get_contact_count() > 0)
            {
                has_jump_request(true);
                velocity.y = -ground.jump_velocity;
                emit_signal("jumped");
            }
        }
        else
        {
            velocity = velocity.move_toward(input * speed, rate * p_delta);
        }
        rigid->set_linear_velocity(velocity);
    }

    if (input.length_squared() > 0.01f)
    {
        facing = platformer ? Vector2(input.x < 0.0f ? -1.0f : 1.0f, 0) : input.normalized();
        if (platformer && Math::is_zero_approx(input.x))
        {
            facing = Vector2(1, 0);
        }
        _update_visual();
    }
}

void GroundMovement2D::_bind_methods()
{
    bind_ground_common<GroundMovement2D>();
    ClassDB::bind_method(D_METHOD("set_mode", "mode"), &GroundMovement2D::set_mode);
    ClassDB::bind_method(D_METHOD("get_mode"), &GroundMovement2D::get_mode);
    BIND_ENUM_CONSTANT(MODE_PLATFORMER);
    BIND_ENUM_CONSTANT(MODE_TOP_DOWN);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Platformer,Top Down"), "set_mode", "get_mode");
}

// =====================================================================================================
// 🦶 3D
// =====================================================================================================

GroundMovement3D::GroundMovement3D()
{
    // Valeurs en mètres
    ground.max_speed = 5.0;
    ground.acceleration = 40.0;
    ground.deceleration = 50.0;
    ground.jump_velocity = 4.5;
    ground.max_fall_speed = 50.0;
}

bool GroundMovement3D::is_on_floor() const
{
    CharacterBody3D *character = Object::cast_to<CharacterBody3D>(get_body());
    return character && character->is_on_floor();
}

Vector2 GroundMovement3D::get_world_input() const
{
    if (!camera_relative)
    {
        return move_input; // x → X, y → Z du monde
    }
    Viewport *viewport = get_viewport();
    Camera3D *camera = viewport ? viewport->get_camera_3d() : nullptr;
    if (!camera)
    {
        return move_input;
    }
    Basis basis = camera->get_global_transform().basis;
    Vector3 forward = -basis.get_column(2);
    Vector3 right = basis.get_column(0);
    forward.y = 0.0f;
    right.y = 0.0f;
    Vector3 direction = right.normalized() * move_input.x + forward.normalized() * -move_input.y;
    return Vector2(direction.x, direction.z).limit_length(1.0f);
}

void GroundMovement3D::_physics_step(double p_delta)
{
    Node *body = get_body();
    Knockback3D *knockback = Knockback3D::active_on(body);
    const Vector2 input = knockback ? Vector2() : get_world_input();
    const double speed = ground.max_speed * (running ? ground.run_multiplier : 1.0);
    const Vector3 external = external_acceleration_3d(body);

    if (CharacterBody3D *character = Object::cast_to<CharacterBody3D>(body))
    {
        const bool on_floor = character->is_on_floor();
        if (on_floor)
        {
            ground.coyote = ground.coyote_time;
            ground.jumps_used = 0;
        }
        else
        {
            ground.coyote -= p_delta;
        }
        Vector3 velocity = character->get_velocity();
        Vector2 horizontal(velocity.x, velocity.z);
        double rate = knockback ? knockback->get_friction() : (input.length_squared() > 0.0001f ? ground.acceleration : ground.deceleration) * (on_floor ? 1.0 : ground.air_control);
        horizontal = horizontal.move_toward(input * speed, rate * p_delta);
        velocity.x = horizontal.x;
        velocity.z = horizontal.y;
        if (!on_floor)
        {
            velocity += character->get_gravity() * ground.gravity_scale * p_delta;
            velocity.y = MAX(velocity.y, float(-ground.max_fall_speed));
        }
        if (!knockback && has_jump_request() && ground.try_jump(on_floor))
        {
            has_jump_request(true);
            velocity.y = ground.jump_velocity;
            emit_signal("jumped");
        }
        if (!ground.jump_cut_done && !jump_held && velocity.y > 0.0f)
        {
            velocity.y *= ground.jump_cut;
            ground.jump_cut_done = true;
        }
        if (velocity.y <= 0.0f)
        {
            ground.jump_cut_done = true;
        }
        velocity += external * p_delta;
        character->set_velocity(velocity);
        character->move_and_slide();
        bool now_on_floor = character->is_on_floor();
        if (now_on_floor && !ground.was_on_floor)
        {
            emit_signal("landed");
        }
        ground.was_on_floor = now_on_floor;
    }
    else if (RigidBody3D *rigid = Object::cast_to<RigidBody3D>(body))
    {
        Vector3 velocity = rigid->get_linear_velocity();
        Vector2 horizontal(velocity.x, velocity.z);
        double rate = input.length_squared() > 0.0001f ? ground.acceleration : ground.deceleration;
        horizontal = horizontal.move_toward(input * speed, rate * p_delta);
        velocity.x = horizontal.x;
        velocity.z = horizontal.y;
        if (has_jump_request() && rigid->get_contact_count() > 0)
        {
            has_jump_request(true);
            velocity.y = ground.jump_velocity;
            emit_signal("jumped");
        }
        rigid->set_linear_velocity(velocity);
    }

    if (input.length_squared() > 0.01f)
    {
        facing = input.normalized();
    }
    if (rotate_toward_movement)
    {
        Node3D *visual = visual_path.is_empty() ? Object::cast_to<Node3D>(body) : Object::cast_to<Node3D>(get_node_or_null(visual_path));
        if (visual && !Object::cast_to<RigidBody3D>(visual))
        {
            // L'avant d'un Node3D est -Z : l'angle qui oriente -Z vers (x, z) est atan2(-x, -z).
            double target = Math::atan2(-facing.x, -facing.y);
            Vector3 rotation = visual->get_rotation();
            rotation.y = Math::lerp_angle(double(rotation.y), target, MIN(1.0, turn_speed * p_delta));
            visual->set_rotation(rotation);
        }
    }
}

void GroundMovement3D::_bind_methods()
{
    bind_ground_common<GroundMovement3D>();
    ClassDB::bind_method(D_METHOD("set_camera_relative", "relative"), &GroundMovement3D::set_camera_relative);
    ClassDB::bind_method(D_METHOD("get_camera_relative"), &GroundMovement3D::get_camera_relative);
    ClassDB::bind_method(D_METHOD("set_rotate_toward_movement", "rotate"), &GroundMovement3D::set_rotate_toward_movement);
    ClassDB::bind_method(D_METHOD("get_rotate_toward_movement"), &GroundMovement3D::get_rotate_toward_movement);
    ClassDB::bind_method(D_METHOD("set_turn_speed", "speed"), &GroundMovement3D::set_turn_speed);
    ClassDB::bind_method(D_METHOD("get_turn_speed"), &GroundMovement3D::get_turn_speed);
    ClassDB::bind_method(D_METHOD("get_world_input"), &GroundMovement3D::get_world_input);
    ADD_GROUP(U"🧭 Orientation", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "camera_relative"), "set_camera_relative", "get_camera_relative");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rotate_toward_movement"), "set_rotate_toward_movement", "get_rotate_toward_movement");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "turn_speed", PROPERTY_HINT_RANGE, "0,50,0.01,or_greater"), "set_turn_speed", "get_turn_speed");
}

// 🐞 Débogage

String GroundMovement2D::_debug_text() const
{
    return String(is_on_floor() ? U"au sol" : U"en l'air") + U" · sauts " + String::num_int64(ground.jumps_used) + "/" + String::num_int64(ground.max_jumps) + (running ? String(U" · course") : String());
}

void GroundMovement2D::_draw_debug(DebugGizmos &p_debug)
{
    if (CharacterBody2D *character = Object::cast_to<CharacterBody2D>(get_body()))
    {
        Vector2 position = character->get_global_position();
        p_debug.arrow(position, position + facing * 24.0f, debug_colors::HEADING, 6.0f);
        if (character->is_on_floor())
        {
            p_debug.arrow(position, position + character->get_floor_normal() * 30.0f, debug_colors::NORMAL, 6.0f);
        }
    }
    MovementComponent::_draw_debug(p_debug);
}

String GroundMovement3D::_debug_text() const
{
    return String(is_on_floor() ? U"au sol" : U"en l'air") + U" · sauts " + String::num_int64(ground.jumps_used) + "/" + String::num_int64(ground.max_jumps) + (running ? String(U" · course") : String());
}

void GroundMovement3D::_draw_debug(DebugGizmos &p_debug)
{
    if (CharacterBody3D *character = Object::cast_to<CharacterBody3D>(get_body()))
    {
        Vector3 position = character->get_global_position();
        Vector2 world = get_world_input();
        p_debug.arrow3(position + Vector3(0, 0.05f, 0), position + Vector3(world.x, 0.05f, world.y), debug_colors::INPUT);
        p_debug.arrow3(position, position + Vector3(facing.x, 0, facing.y) * 0.8f, debug_colors::HEADING);
        if (character->is_on_floor())
        {
            p_debug.arrow3(position, position + character->get_floor_normal(), debug_colors::NORMAL);
        }
        p_debug.arrow3(position, position + character->get_velocity() * 0.25f, debug_colors::VELOCITY);
        p_debug.text3(position + Vector3(0, 1.4f, 0), String(get_name()) + U" · " + _debug_text());
    }
}
