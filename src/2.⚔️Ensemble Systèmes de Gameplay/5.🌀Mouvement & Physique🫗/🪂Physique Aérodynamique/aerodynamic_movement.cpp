#include "aerodynamic_movement.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace
{
template <typename T>
void bind_aero_common()
{
    ClassDB::bind_method(D_METHOD("set_mode", "mode"), &T::set_mode);
    ClassDB::bind_method(D_METHOD("get_mode"), &T::get_mode);
    ClassDB::bind_method(D_METHOD("set_glide_fall_speed", "value"), &T::set_glide_fall_speed);
    ClassDB::bind_method(D_METHOD("get_glide_fall_speed"), &T::get_glide_fall_speed);
    ClassDB::bind_method(D_METHOD("set_glide_speed", "value"), &T::set_glide_speed);
    ClassDB::bind_method(D_METHOD("get_glide_speed"), &T::get_glide_speed);
    ClassDB::bind_method(D_METHOD("set_glide_acceleration", "value"), &T::set_glide_acceleration);
    ClassDB::bind_method(D_METHOD("get_glide_acceleration"), &T::get_glide_acceleration);
    ClassDB::bind_method(D_METHOD("set_glide_gravity_scale", "value"), &T::set_glide_gravity_scale);
    ClassDB::bind_method(D_METHOD("get_glide_gravity_scale"), &T::get_glide_gravity_scale);
    ClassDB::bind_method(D_METHOD("set_fly_speed", "value"), &T::set_fly_speed);
    ClassDB::bind_method(D_METHOD("get_fly_speed"), &T::get_fly_speed);
    ClassDB::bind_method(D_METHOD("set_fly_acceleration", "value"), &T::set_fly_acceleration);
    ClassDB::bind_method(D_METHOD("get_fly_acceleration"), &T::get_fly_acceleration);
    ClassDB::bind_method(D_METHOD("set_fly_drag", "value"), &T::set_fly_drag);
    ClassDB::bind_method(D_METHOD("get_fly_drag"), &T::get_fly_drag);
    ClassDB::bind_method(D_METHOD("set_wind_influence", "value"), &T::set_wind_influence);
    ClassDB::bind_method(D_METHOD("get_wind_influence"), &T::get_wind_influence);
    ClassDB::bind_method(D_METHOD("set_max_flaps", "value"), &T::set_max_flaps);
    ClassDB::bind_method(D_METHOD("get_max_flaps"), &T::get_max_flaps);
    ClassDB::bind_method(D_METHOD("set_flap_velocity", "value"), &T::set_flap_velocity);
    ClassDB::bind_method(D_METHOD("get_flap_velocity"), &T::get_flap_velocity);
    ClassDB::bind_method(D_METHOD("set_drag_coefficient", "value"), &T::set_drag_coefficient);
    ClassDB::bind_method(D_METHOD("get_drag_coefficient"), &T::get_drag_coefficient);
    ClassDB::bind_method(D_METHOD("set_lift_coefficient", "value"), &T::set_lift_coefficient);
    ClassDB::bind_method(D_METHOD("get_lift_coefficient"), &T::get_lift_coefficient);
    ClassDB::bind_method(D_METHOD("set_flutter_frequency", "value"), &T::set_flutter_frequency);
    ClassDB::bind_method(D_METHOD("get_flutter_frequency"), &T::get_flutter_frequency);
    ClassDB::bind_method(D_METHOD("get_flaps_left"), &T::get_flaps_left);

    StringName cls = T::get_class_static();
    ClassDB::bind_integer_constant(cls, "Mode", "MODE_GLIDE", 0);
    ClassDB::bind_integer_constant(cls, "Mode", "MODE_FLY", 1);
    ClassDB::add_property(cls, PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Glide,Fly"), "set_mode", "get_mode");
    ClassDB::add_property_group(cls, U"🪂 Plané", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "glide_fall_speed", PROPERTY_HINT_RANGE, "0,2000,0.01,or_greater"), "set_glide_fall_speed", "get_glide_fall_speed");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "glide_speed", PROPERTY_HINT_RANGE, "0,2000,0.01,or_greater"), "set_glide_speed", "get_glide_speed");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "glide_acceleration", PROPERTY_HINT_RANGE, "0,10000,0.01,or_greater"), "set_glide_acceleration", "get_glide_acceleration");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "glide_gravity_scale", PROPERTY_HINT_RANGE, "0,2,0.01,or_greater"), "set_glide_gravity_scale", "get_glide_gravity_scale");
    ClassDB::add_property(cls, PropertyInfo(Variant::INT, "max_flaps", PROPERTY_HINT_RANGE, "0,20,1,or_greater"), "set_max_flaps", "get_max_flaps");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "flap_velocity", PROPERTY_HINT_RANGE, "0,3000,0.01,or_greater"), "set_flap_velocity", "get_flap_velocity");
    ClassDB::add_property_group(cls, U"🕊️ Vol libre", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "fly_speed", PROPERTY_HINT_RANGE, "0,3000,0.01,or_greater"), "set_fly_speed", "get_fly_speed");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "fly_acceleration", PROPERTY_HINT_RANGE, "0,20000,0.01,or_greater"), "set_fly_acceleration", "get_fly_acceleration");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "fly_drag", PROPERTY_HINT_RANGE, "0,20,0.01,or_greater"), "set_fly_drag", "get_fly_drag");
    ClassDB::add_property_group(cls, U"🌬️ Air", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "wind_influence", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater"), "set_wind_influence", "get_wind_influence");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "drag_coefficient", PROPERTY_HINT_RANGE, "0,1,0.0001,or_greater"), "set_drag_coefficient", "get_drag_coefficient");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "lift_coefficient", PROPERTY_HINT_RANGE, "-1,1,0.0001,or_greater,or_less"), "set_lift_coefficient", "get_lift_coefficient");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "flutter_frequency", PROPERTY_HINT_RANGE, "0,20,0.01,or_greater"), "set_flutter_frequency", "get_flutter_frequency");
    ClassDB::add_signal(cls, MethodInfo("flapped"));
}

/// Signe de la portance : constant, ou oscillant (vol de feuille).
double lift_sign(const AeroSettings &p_aero) { return p_aero.flutter_frequency > 0.0 ? Math::sin(p_aero.time * p_aero.flutter_frequency * Math_TAU) : 1.0; }
} // namespace

// =====================================================================================================
// 🪂 2D
// =====================================================================================================

void AerodynamicMovement2D::_physics_step(double p_delta)
{
    CharacterBody2D *character = Object::cast_to<CharacterBody2D>(get_body());
    if (!character)
    {
        return; // corps rigide : traité passivement dans _notification
    }
    Vector2 velocity = character->get_velocity();
    if (aero.mode == MODE_GLIDE)
    {
        velocity += character->get_gravity() * aero.glide_gravity_scale * p_delta;
        velocity.x = Math::move_toward(velocity.x, float(move_input.x * aero.glide_speed), float(aero.glide_acceleration * p_delta));
        if (has_jump_request() && aero.flaps_left > 0)
        {
            has_jump_request(true);
            aero.flaps_left--;
            velocity.y = -aero.flap_velocity;
            emit_signal("flapped");
        }
        velocity.y = MIN(velocity.y, float(aero.glide_fall_speed));
    }
    else
    {
        velocity = velocity.move_toward(move_input * aero.fly_speed, aero.fly_acceleration * p_delta);
        velocity *= MAX(0.0, 1.0 - aero.fly_drag * p_delta * (move_input.length_squared() > 0.01f ? 0.0 : 1.0));
    }
    velocity += external_acceleration_2d(character) * aero.wind_influence * p_delta;
    character->set_velocity(velocity);
    character->move_and_slide();
}

void AerodynamicMovement2D::_notification(int p_what)
{
    if (p_what != NOTIFICATION_PHYSICS_PROCESS || Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    double delta = get_physics_process_delta_time();
    aero.time += delta;
    RigidBody2D *rigid = Object::cast_to<RigidBody2D>(get_body());
    if (!rigid)
    {
        return;
    }
    Vector2 velocity = rigid->get_linear_velocity();
    double speed = velocity.length();
    if (speed < 0.001)
    {
        return;
    }
    double mass = rigid->get_mass();
    Vector2 drag = -velocity * speed * aero.drag_coefficient;
    Vector2 lift = Vector2(-velocity.y, velocity.x) / speed * speed * speed * aero.lift_coefficient * lift_sign(aero);
    rigid->apply_central_force((drag + lift) * mass);
}

void AerodynamicMovement2D::_bind_methods() { bind_aero_common<AerodynamicMovement2D>(); }

// =====================================================================================================
// 🪂 3D
// =====================================================================================================

AerodynamicMovement3D::AerodynamicMovement3D()
{
    aero.glide_fall_speed = 1.5;
    aero.glide_speed = 7.0;
    aero.glide_acceleration = 10.0;
    aero.fly_speed = 7.0;
    aero.fly_acceleration = 25.0;
    aero.flap_velocity = 4.0;
    aero.drag_coefficient = 0.05;
}

void AerodynamicMovement3D::_physics_step(double p_delta)
{
    CharacterBody3D *character = Object::cast_to<CharacterBody3D>(get_body());
    if (!character)
    {
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
    Vector3 velocity = character->get_velocity();
    if (aero.mode == MODE_GLIDE)
    {
        velocity += character->get_gravity() * aero.glide_gravity_scale * p_delta;
        Vector2 horizontal = Vector2(velocity.x, velocity.z).move_toward(input * aero.glide_speed, aero.glide_acceleration * p_delta);
        velocity.x = horizontal.x;
        velocity.z = horizontal.y;
        if (has_jump_request() && aero.flaps_left > 0)
        {
            has_jump_request(true);
            aero.flaps_left--;
            velocity.y = aero.flap_velocity;
            emit_signal("flapped");
        }
        velocity.y = MAX(velocity.y, float(-aero.glide_fall_speed));
    }
    else
    {
        float vertical = Math::abs(vertical_input) > 0.1f ? vertical_input : (jump_held ? 1.0f : 0.0f);
        Vector3 target = Vector3(input.x, vertical, input.y) * aero.fly_speed;
        velocity = velocity.move_toward(target, aero.fly_acceleration * p_delta);
    }
    velocity += external_acceleration_3d(character) * aero.wind_influence * p_delta;
    character->set_velocity(velocity);
    character->move_and_slide();
}

void AerodynamicMovement3D::_notification(int p_what)
{
    if (p_what != NOTIFICATION_PHYSICS_PROCESS || Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    double delta = get_physics_process_delta_time();
    aero.time += delta;
    RigidBody3D *rigid = Object::cast_to<RigidBody3D>(get_body());
    if (!rigid)
    {
        return;
    }
    Vector3 velocity = rigid->get_linear_velocity();
    double speed = velocity.length();
    if (speed < 0.001)
    {
        return;
    }
    double mass = rigid->get_mass();
    Vector3 drag = -velocity * speed * aero.drag_coefficient;
    // Portance : perpendiculaire à la vitesse, dans le plan vertical qui la contient.
    Vector3 side = velocity.cross(Vector3(0, 1, 0));
    Vector3 lift_direction = side.length_squared() > 1e-6 ? side.cross(velocity).normalized() : Vector3();
    Vector3 lift = lift_direction * speed * speed * aero.lift_coefficient * lift_sign(aero);
    rigid->apply_central_force((drag + lift) * mass);
}

void AerodynamicMovement3D::_bind_methods()
{
    bind_aero_common<AerodynamicMovement3D>();
    ClassDB::bind_method(D_METHOD("set_camera_relative", "relative"), &AerodynamicMovement3D::set_camera_relative);
    ClassDB::bind_method(D_METHOD("get_camera_relative"), &AerodynamicMovement3D::get_camera_relative);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "camera_relative"), "set_camera_relative", "get_camera_relative");
}

// 🐞 Débogage

String AerodynamicMovement2D::_debug_text() const
{
    return String(aero.mode == 1 ? U"vol libre" : U"plané") + U" · battements " + String::num_int64(aero.flaps_left) + "/" + String::num_int64(aero.max_flaps);
}

void AerodynamicMovement2D::_draw_debug(DebugGizmos &p_debug)
{
    if (Node2D *body = Object::cast_to<Node2D>(get_body()))
    {
        Vector2 position = body->get_global_position();
        p_debug.arrow(position, position + external_acceleration_2d(body) * aero.wind_influence * 0.05f, debug_colors::FORCE, 8.0f, 3.0f);
    }
    MovementComponent::_draw_debug(p_debug);
}

String AerodynamicMovement3D::_debug_text() const
{
    return String(aero.mode == 1 ? U"vol libre" : U"plané") + U" · battements " + String::num_int64(aero.flaps_left) + "/" + String::num_int64(aero.max_flaps);
}

void AerodynamicMovement3D::_draw_debug(DebugGizmos &p_debug)
{
    if (Node3D *body = Object::cast_to<Node3D>(get_body()))
    {
        Vector3 position = body->get_global_position();
        p_debug.arrow3(position, position + external_acceleration_3d(body) * aero.wind_influence * 0.05f, debug_colors::FORCE, 0.2f, 3.0f);
    }
    MovementComponent::_draw_debug(p_debug);
}
