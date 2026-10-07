#include "movement_component.hpp"

#include "../🧲Zone de force/force_zone.hpp"

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
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

void MovementComponent::set_debug_mode(bool p_debug)
{
    debug_mode = p_debug;
    if (!debug_mode)
    {
        debug.release();
    }
}

void MovementComponent::_draw_debug(DebugGizmos &p_debug)
{
    Node *body = get_body();
    String text = String(get_name()) + " (" + get_class() + ")";
    String state = _debug_text();
    if (!state.is_empty())
    {
        text += String(U" · ") + state;
    }
    if (CharacterBody2D *character = Object::cast_to<CharacterBody2D>(body))
    {
        Vector2 position = character->get_global_position();
        p_debug.arrow(position, position + character->get_velocity() * 0.25f, debug_colors::VELOCITY);
        p_debug.arrow(position, position + move_input * 40.0f, debug_colors::INPUT, 6.0f);
        p_debug.text(position + Vector2(-60, -48), text, debug_colors::TEXT, 11);
    }
    else if (RigidBody2D *rigid = Object::cast_to<RigidBody2D>(body))
    {
        Vector2 position = rigid->get_global_position();
        p_debug.arrow(position, position + rigid->get_linear_velocity() * 0.25f, debug_colors::VELOCITY);
        p_debug.text(position + Vector2(-60, -40), text, debug_colors::TEXT, 11);
    }
    else if (CharacterBody3D *character3 = Object::cast_to<CharacterBody3D>(body))
    {
        Vector3 position = character3->get_global_position();
        p_debug.arrow3(position, position + character3->get_velocity() * 0.25f, debug_colors::VELOCITY);
        p_debug.arrow3(position, position + Vector3(move_input.x, 0, move_input.y), debug_colors::INPUT);
        p_debug.text3(position + Vector3(0, 1.4f, 0), text);
    }
    else if (RigidBody3D *rigid3 = Object::cast_to<RigidBody3D>(body))
    {
        Vector3 position = rigid3->get_global_position();
        p_debug.arrow3(position, position + rigid3->get_linear_velocity() * 0.25f, debug_colors::VELOCITY);
        p_debug.text3(position + Vector3(0, 1.0f, 0), text);
    }
}

void MovementComponent::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_EXIT_TREE)
    {
        debug.release();
    }
    else if (p_what == NOTIFICATION_READY)
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
        if (debug_mode)
        {
            debug.begin(this, is_body_3d());
            if (active)
            {
                _draw_debug(debug);
            }
            debug.end();
        }
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

    ClassDB::bind_method(D_METHOD("set_debug_mode", "debug"), &MovementComponent::set_debug_mode);
    ClassDB::bind_method(D_METHOD("get_debug_mode"), &MovementComponent::get_debug_mode);

    GDVIRTUAL_BIND(_movement_step, "delta");

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "jump_buffer_time", PROPERTY_HINT_RANGE, "0,1,0.01,suffix:s"), "set_jump_buffer_time", "get_jump_buffer_time");

    ADD_GROUP(U"🐞 Débogage", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_mode"), "set_debug_mode", "get_debug_mode");

    ADD_SIGNAL(MethodInfo("activated"));
    ADD_SIGNAL(MethodInfo("deactivated"));
}
