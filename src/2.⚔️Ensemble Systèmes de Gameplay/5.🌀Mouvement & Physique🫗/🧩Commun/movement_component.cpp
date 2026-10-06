#include "movement_component.hpp"

#include "../🧲Zone de force/force_zone.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void MovementComponent::set_active(bool p_active)
{
    if (active == p_active)
    {
        return;
    }
    active = p_active;
    if (active)
    {
        _on_activated();
        emit_signal("activated");
    }
    else
    {
        _on_deactivated();
        emit_signal("deactivated");
    }
}

void MovementComponent::jump()
{
    jump_held = true;
    jump_buffer = jump_buffer_time > 0.0 ? jump_buffer_time : 0.0001;
}

bool MovementComponent::has_jump_request(bool p_consume)
{
    bool requested = jump_buffer > 0.0;
    if (requested && p_consume)
    {
        jump_buffer = 0.0;
    }
    return requested;
}

Vector2 MovementComponent::external_acceleration_2d(Node *p_body) { return ForceZone2D::get_acceleration_on(p_body); }

Vector3 MovementComponent::external_acceleration_3d(Node *p_body) { return ForceZone3D::get_acceleration_on(p_body); }

void MovementComponent::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_READY)
    {
        set_physics_process(true);
    }
    else if (p_what == NOTIFICATION_PHYSICS_PROCESS)
    {
        double delta = get_physics_process_delta_time();
        if (active)
        {
            _physics_step(delta);
            GDVIRTUAL_CALL(_movement_step, delta);
        }
        jump_buffer = MAX(0.0, jump_buffer - delta);
    }
}

void MovementComponent::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_active", "active"), &MovementComponent::set_active);
    ClassDB::bind_method(D_METHOD("is_active"), &MovementComponent::is_active);
    ClassDB::bind_method(D_METHOD("move", "input"), &MovementComponent::move);
    ClassDB::bind_method(D_METHOD("get_move_input"), &MovementComponent::get_move_input);
    ClassDB::bind_method(D_METHOD("jump"), &MovementComponent::jump);
    ClassDB::bind_method(D_METHOD("release_jump"), &MovementComponent::release_jump);
    ClassDB::bind_method(D_METHOD("is_jump_held"), &MovementComponent::is_jump_held);
    ClassDB::bind_method(D_METHOD("set_running", "running"), &MovementComponent::set_running);
    ClassDB::bind_method(D_METHOD("is_running"), &MovementComponent::is_running);
    ClassDB::bind_method(D_METHOD("set_vertical_input", "value"), &MovementComponent::set_vertical_input);
    ClassDB::bind_method(D_METHOD("get_vertical_input"), &MovementComponent::get_vertical_input);
    ClassDB::bind_method(D_METHOD("set_jump_buffer_time", "time"), &MovementComponent::set_jump_buffer_time);
    ClassDB::bind_method(D_METHOD("get_jump_buffer_time"), &MovementComponent::get_jump_buffer_time);
    ClassDB::bind_method(D_METHOD("has_jump_request", "consume"), &MovementComponent::has_jump_request, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("get_body"), &MovementComponent::get_body);
    ClassDB::bind_static_method("MovementComponent", D_METHOD("external_acceleration_2d", "body"), &MovementComponent::external_acceleration_2d);
    ClassDB::bind_static_method("MovementComponent", D_METHOD("external_acceleration_3d", "body"), &MovementComponent::external_acceleration_3d);

    GDVIRTUAL_BIND(_movement_step, "delta");

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "jump_buffer_time", PROPERTY_HINT_RANGE, "0,1,0.01,suffix:s"), "set_jump_buffer_time", "get_jump_buffer_time");

    ADD_SIGNAL(MethodInfo("activated"));
    ADD_SIGNAL(MethodInfo("deactivated"));
}
