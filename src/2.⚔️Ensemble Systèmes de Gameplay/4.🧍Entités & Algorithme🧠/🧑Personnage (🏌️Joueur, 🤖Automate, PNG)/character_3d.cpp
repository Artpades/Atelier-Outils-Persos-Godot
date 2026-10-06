#include "character_3d.hpp"
#include "character_registry.hpp"
#include "health_component.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

Character3D::Character3D()
{
    core.facing = Vector2(0, -1);
    set_physics_process_priority(-30);
}

Vector2 Character3D::get_facing() const { return core.facing; }

double Character3D::take_damage(double p_amount, Object *p_source)
{
    HealthComponent *health = core.find_health(this);
    return health ? health->take_damage(p_amount, p_source) : 0.0;
}

double Character3D::heal(double p_amount)
{
    HealthComponent *health = core.find_health(this);
    return health ? health->heal(p_amount) : 0.0;
}

double Character3D::get_health() const
{
    HealthComponent *health = core.find_health(this);
    return health ? health->get_health() : 0.0;
}

double Character3D::get_max_health() const
{
    HealthComponent *health = core.find_health(this);
    return health ? health->get_max_health() : 0.0;
}

Vector3 Character3D::get_world_position() const { return node_world_position(this); }

Vector2 Character3D::direction_input_to(const Vector3 &p_point) const
{
    Vector3 position = get_world_position();
    Vector2 difference(p_point.x - position.x, p_point.z - position.z);
    return difference.length_squared() > 0.0001f ? difference.normalized() : Vector2();
}

double Character3D::planar_distance_to(const Vector3 &p_point) const
{
    Vector3 position = get_world_position();
    return Vector2(p_point.x - position.x, p_point.z - position.z).length();
}

void Character3D::face_toward(const Vector3 &p_point)
{
    Vector2 direction = direction_input_to(p_point);
    if (direction == Vector2())
    {
        return;
    }
    core.facing = direction;
    // L'avant d'un Node3D est -Z : l'angle qui oriente -Z vers (x, z) est atan2(-x, -z).
    Vector3 rotation = get_rotation();
    rotation.y = Math::atan2(-direction.x, -direction.y);
    set_rotation(rotation);
}

void Character3D::_notification(int p_what)
{
    if (p_what == NOTIFICATION_CHILD_ORDER_CHANGED)
    {
        notify_property_list_changed();
        return;
    }
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    switch (p_what)
    {
    case NOTIFICATION_ENTER_TREE:
        CharacterRegister::register_character(this);
        break;
    case NOTIFICATION_EXIT_TREE:
        CharacterRegister::unregister_character(this);
        break;
    case NOTIFICATION_READY:
        set_physics_process(true);
        break;
    case NOTIFICATION_PHYSICS_PROCESS:
    {
        core.forward_movement(this);
        Node *movement = core.find_active_movement(this);
        if (movement && core.move_input.length_squared() > 0.01f && movement->has_method("get_facing"))
        {
            core.facing = movement->call("get_facing");
        }
        else if (!movement && core.move_input.length_squared() > 0.01f)
        {
            core.facing = core.move_input.normalized();
        }
        break;
    }
    default:
        break;
    }
}

void Character3D::_bind_methods() { bind_character_common<Character3D>(); }
