#include "knockback.hpp"

#include "../🧩Commun/movement_component.hpp"

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace
{
/// Un composant de mouvement actif s'occupe-t-il déjà du corps (et donc de la vitesse de recul) ?
bool has_active_movement(Node *p_body)
{
    for (int i = 0; p_body && i < p_body->get_child_count(); i++)
    {
        MovementComponent *component = Object::cast_to<MovementComponent>(p_body->get_child(i));
        if (component && component->is_active())
        {
            return true;
        }
    }
    return false;
}

template <typename T>
void bind_knockback_common(Variant::Type p_vector_type)
{
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &T::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &T::is_enabled);
    ClassDB::bind_method(D_METHOD("set_base_force", "force"), &T::set_base_force);
    ClassDB::bind_method(D_METHOD("get_base_force"), &T::get_base_force);
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &T::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &T::get_duration);
    ClassDB::bind_method(D_METHOD("set_friction", "friction"), &T::set_friction);
    ClassDB::bind_method(D_METHOD("get_friction"), &T::get_friction);
    ClassDB::bind_method(D_METHOD("set_gravity_affect", "affect"), &T::set_gravity_affect);
    ClassDB::bind_method(D_METHOD("get_gravity_affect"), &T::get_gravity_affect);
    ClassDB::bind_method(D_METHOD("set_upward_bias", "bias"), &T::set_upward_bias);
    ClassDB::bind_method(D_METHOD("get_upward_bias"), &T::get_upward_bias);
    ClassDB::bind_method(D_METHOD("set_rigid_mode", "mode"), &T::set_rigid_mode);
    ClassDB::bind_method(D_METHOD("get_rigid_mode"), &T::get_rigid_mode);
    ClassDB::bind_method(D_METHOD("set_add_torque", "add"), &T::set_add_torque);
    ClassDB::bind_method(D_METHOD("get_add_torque"), &T::get_add_torque);
    ClassDB::bind_method(D_METHOD("set_torque_factor", "factor"), &T::set_torque_factor);
    ClassDB::bind_method(D_METHOD("get_torque_factor"), &T::get_torque_factor);
    ClassDB::bind_method(D_METHOD("apply_knockback", "direction", "multiplier"), &T::apply_knockback, DEFVAL(1.0));
    ClassDB::bind_method(D_METHOD("apply_knockback_from", "point", "multiplier"), &T::apply_knockback_from, DEFVAL(1.0));
    ClassDB::bind_method(D_METHOD("is_knocked"), &T::is_knocked);
    ClassDB::bind_method(D_METHOD("get_lock_remaining"), &T::get_lock_remaining);
    ClassDB::bind_static_method(T::get_class_static(), D_METHOD("active_on", "body"), &T::active_on);

    StringName cls = T::get_class_static();
    ClassDB::bind_integer_constant(cls, "RigidMode", "RIGID_IMPULSE", 0);
    ClassDB::bind_integer_constant(cls, "RigidMode", "RIGID_CONTINUOUS", 1);
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "base_force", PROPERTY_HINT_RANGE, "0,5000,0.01,or_greater"), "set_base_force", "get_base_force");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0,3,0.01,or_greater,suffix:s"), "set_duration", "get_duration");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "friction", PROPERTY_HINT_RANGE, "0,10000,0.01,or_greater"), "set_friction", "get_friction");
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "gravity_affect"), "set_gravity_affect", "get_gravity_affect");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "upward_bias", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_upward_bias", "get_upward_bias");
    ClassDB::add_property_group(cls, U"🧱 Corps rigide", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::INT, "rigid_mode", PROPERTY_HINT_ENUM, "Impulse,Continuous"), "set_rigid_mode", "get_rigid_mode");
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "add_torque"), "set_add_torque", "get_add_torque");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "torque_factor", PROPERTY_HINT_RANGE, "0,10,0.001,or_greater"), "set_torque_factor", "get_torque_factor");
    ClassDB::add_signal(cls, MethodInfo("knocked_back", PropertyInfo(p_vector_type, "velocity")));
    ClassDB::add_signal(cls, MethodInfo("recovered"));
}
} // namespace

// 🌬️ 2D

void Knockback2D::apply_knockback(const Vector2 &p_direction, double p_multiplier)
{
    if (!enabled || p_direction.length_squared() < 1e-8)
    {
        return;
    }
    // L'axe Y 2D descend : un peu de « haut » rend le recul lisible en plateforme.
    Vector2 direction = (p_direction.normalized() + Vector2(0, -upward_bias)).normalized();
    Vector2 velocity = direction * base_force * p_multiplier;
    Node *body = get_parent();
    if (CharacterBody2D *character = Object::cast_to<CharacterBody2D>(body))
    {
        character->set_velocity(velocity);
        lock_remaining = duration;
    }
    else if (RigidBody2D *rigid = Object::cast_to<RigidBody2D>(body))
    {
        if (rigid_mode == RIGID_IMPULSE)
        {
            rigid->apply_central_impulse(velocity * rigid->get_mass());
        }
        else
        {
            continuous_velocity = velocity;
            lock_remaining = duration;
        }
        if (add_torque)
        {
            rigid->apply_torque_impulse(velocity.cross(Vector2(0, -1)) * torque_factor * rigid->get_mass());
        }
    }
    emit_signal("knocked_back", velocity);
}

void Knockback2D::apply_knockback_from(const Vector2 &p_point, double p_multiplier)
{
    Node2D *body = Object::cast_to<Node2D>(get_parent());
    if (body)
    {
        Vector2 away = body->get_global_position() - p_point;
        apply_knockback(away.length_squared() > 1e-6 ? away : Vector2(1, 0), p_multiplier);
    }
}

Knockback2D *Knockback2D::active_on(Node *p_body)
{
    for (int i = 0; p_body && i < p_body->get_child_count(); i++)
    {
        Knockback2D *knockback = Object::cast_to<Knockback2D>(p_body->get_child(i));
        if (knockback && knockback->is_knocked())
        {
            return knockback;
        }
    }
    return nullptr;
}

void Knockback2D::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_READY)
    {
        set_physics_process(true);
        // Après les composants de mouvement : ils ont déjà traité le recul s'ils sont actifs.
        set_physics_process_priority(10);
        return;
    }
    if (p_what != NOTIFICATION_PHYSICS_PROCESS || lock_remaining <= 0.0)
    {
        return;
    }
    double delta = get_physics_process_delta_time();
    Node *body = get_parent();
    if (CharacterBody2D *character = Object::cast_to<CharacterBody2D>(body))
    {
        if (!has_active_movement(character))
        {
            Vector2 velocity = character->get_velocity().move_toward(Vector2(), friction * delta);
            if (gravity_affect)
            {
                velocity += character->get_gravity() * delta;
            }
            character->set_velocity(velocity);
            character->move_and_slide();
        }
    }
    else if (RigidBody2D *rigid = Object::cast_to<RigidBody2D>(body))
    {
        rigid->apply_central_force(continuous_velocity * rigid->get_mass() / MAX(duration, 0.01));
    }
    lock_remaining -= delta;
    if (lock_remaining <= 0.0)
    {
        lock_remaining = 0.0;
        emit_signal("recovered");
    }
}

void Knockback2D::_bind_methods() { bind_knockback_common<Knockback2D>(Variant::VECTOR2); }

// 🌬️ 3D

void Knockback3D::apply_knockback(const Vector3 &p_direction, double p_multiplier)
{
    if (!enabled || p_direction.length_squared() < 1e-8)
    {
        return;
    }
    Vector3 direction = (p_direction.normalized() + Vector3(0, upward_bias, 0)).normalized();
    Vector3 velocity = direction * base_force * p_multiplier;
    Node *body = get_parent();
    if (CharacterBody3D *character = Object::cast_to<CharacterBody3D>(body))
    {
        character->set_velocity(velocity);
        lock_remaining = duration;
    }
    else if (RigidBody3D *rigid = Object::cast_to<RigidBody3D>(body))
    {
        if (rigid_mode == RIGID_IMPULSE)
        {
            rigid->apply_central_impulse(velocity * rigid->get_mass());
        }
        else
        {
            continuous_velocity = velocity;
            lock_remaining = duration;
        }
        if (add_torque)
        {
            Vector3 axis = velocity.cross(Vector3(0, 1, 0));
            if (axis.length_squared() > 1e-8)
            {
                rigid->apply_torque_impulse(axis.normalized() * velocity.length() * torque_factor * rigid->get_mass());
            }
        }
    }
    emit_signal("knocked_back", velocity);
}

void Knockback3D::apply_knockback_from(const Vector3 &p_point, double p_multiplier)
{
    Node3D *body = Object::cast_to<Node3D>(get_parent());
    if (body)
    {
        Vector3 away = body->get_global_position() - p_point;
        away.y = 0.0f;
        apply_knockback(away.length_squared() > 1e-6 ? away : Vector3(0, 0, 1), p_multiplier);
    }
}

Knockback3D *Knockback3D::active_on(Node *p_body)
{
    for (int i = 0; p_body && i < p_body->get_child_count(); i++)
    {
        Knockback3D *knockback = Object::cast_to<Knockback3D>(p_body->get_child(i));
        if (knockback && knockback->is_knocked())
        {
            return knockback;
        }
    }
    return nullptr;
}

void Knockback3D::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_READY)
    {
        set_physics_process(true);
        set_physics_process_priority(10);
        return;
    }
    if (p_what != NOTIFICATION_PHYSICS_PROCESS || lock_remaining <= 0.0)
    {
        return;
    }
    double delta = get_physics_process_delta_time();
    Node *body = get_parent();
    if (CharacterBody3D *character = Object::cast_to<CharacterBody3D>(body))
    {
        if (!has_active_movement(character))
        {
            Vector3 velocity = character->get_velocity();
            Vector3 horizontal = Vector3(velocity.x, 0, velocity.z).move_toward(Vector3(), friction * delta);
            velocity = Vector3(horizontal.x, velocity.y, horizontal.z);
            if (gravity_affect)
            {
                velocity += character->get_gravity() * delta;
            }
            character->set_velocity(velocity);
            character->move_and_slide();
        }
    }
    else if (RigidBody3D *rigid = Object::cast_to<RigidBody3D>(body))
    {
        rigid->apply_central_force(continuous_velocity * rigid->get_mass() / MAX(duration, 0.01));
    }
    lock_remaining -= delta;
    if (lock_remaining <= 0.0)
    {
        lock_remaining = 0.0;
        emit_signal("recovered");
    }
}

void Knockback3D::_bind_methods() { bind_knockback_common<Knockback3D>(Variant::VECTOR3); }
