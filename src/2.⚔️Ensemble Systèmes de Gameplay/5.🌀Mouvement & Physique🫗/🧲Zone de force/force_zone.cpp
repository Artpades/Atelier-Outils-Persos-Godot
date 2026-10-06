#include "force_zone.hpp"

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

LocalVector<ObjectID> ForceZone2D::zones;
LocalVector<ObjectID> ForceZone3D::zones;

// ⚙️ Réglages communs

double ForceZoneSettings::attenuation(double p_distance) const
{
    switch (falloff)
    {
    case FALLOFF_LINEAR:
        return MAX(0.0, 1.0 - p_distance / falloff_radius);
    case FALLOFF_INVERSE_SQUARE:
    {
        double ratio = p_distance / falloff_radius;
        return 1.0 / (1.0 + ratio * ratio);
    }
    default:
        return 1.0;
    }
}

double ForceZoneSettings::turbulence_factor(double p_time, double p_seed) const
{
    if (turbulence <= 0.0)
    {
        return 1.0;
    }
    double wave = 0.6 * Math::sin(p_time * turbulence_speed * 1.3 + p_seed) + 0.4 * Math::sin(p_time * turbulence_speed * 2.7 + p_seed * 1.7);
    return MAX(0.0, 1.0 + turbulence * wave);
}

namespace
{

template <typename T>
void bind_force_zone_common(Variant::Type p_vector_type)
{
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &T::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &T::is_enabled);
    ClassDB::bind_method(D_METHOD("set_mode", "mode"), &T::set_mode);
    ClassDB::bind_method(D_METHOD("get_mode"), &T::get_mode);
    ClassDB::bind_method(D_METHOD("set_strength", "strength"), &T::set_strength);
    ClassDB::bind_method(D_METHOD("get_strength"), &T::get_strength);
    ClassDB::bind_method(D_METHOD("set_direction", "direction"), &T::set_direction);
    ClassDB::bind_method(D_METHOD("get_direction"), &T::get_direction);
    ClassDB::bind_method(D_METHOD("set_local_direction", "local"), &T::set_local_direction);
    ClassDB::bind_method(D_METHOD("get_local_direction"), &T::get_local_direction);
    ClassDB::bind_method(D_METHOD("set_falloff", "falloff"), &T::set_falloff);
    ClassDB::bind_method(D_METHOD("get_falloff"), &T::get_falloff);
    ClassDB::bind_method(D_METHOD("set_falloff_radius", "radius"), &T::set_falloff_radius);
    ClassDB::bind_method(D_METHOD("get_falloff_radius"), &T::get_falloff_radius);
    ClassDB::bind_method(D_METHOD("set_turbulence", "value"), &T::set_turbulence);
    ClassDB::bind_method(D_METHOD("get_turbulence"), &T::get_turbulence);
    ClassDB::bind_method(D_METHOD("set_turbulence_speed", "speed"), &T::set_turbulence_speed);
    ClassDB::bind_method(D_METHOD("get_turbulence_speed"), &T::get_turbulence_speed);
    ClassDB::bind_method(D_METHOD("set_affect_rigid_bodies", "affect"), &T::set_affect_rigid_bodies);
    ClassDB::bind_method(D_METHOD("get_affect_rigid_bodies"), &T::get_affect_rigid_bodies);
    ClassDB::bind_method(D_METHOD("set_affect_characters", "affect"), &T::set_affect_characters);
    ClassDB::bind_method(D_METHOD("get_affect_characters"), &T::get_affect_characters);
    ClassDB::bind_method(D_METHOD("compute_acceleration", "point", "velocity"), &T::compute_acceleration);
    ClassDB::bind_static_method(T::get_class_static(), D_METHOD("get_acceleration_on", "body"), &T::get_acceleration_on);

    StringName cls = T::get_class_static();
    ClassDB::bind_integer_constant(cls, "Mode", "MODE_DIRECTIONAL", ForceZoneSettings::MODE_DIRECTIONAL);
    ClassDB::bind_integer_constant(cls, "Mode", "MODE_ATTRACT", ForceZoneSettings::MODE_ATTRACT);
    ClassDB::bind_integer_constant(cls, "Mode", "MODE_VORTEX", ForceZoneSettings::MODE_VORTEX);
    ClassDB::bind_integer_constant(cls, "Mode", "MODE_DRAG", ForceZoneSettings::MODE_DRAG);
    ClassDB::bind_integer_constant(cls, "Falloff", "FALLOFF_NONE", ForceZoneSettings::FALLOFF_NONE);
    ClassDB::bind_integer_constant(cls, "Falloff", "FALLOFF_LINEAR", ForceZoneSettings::FALLOFF_LINEAR);
    ClassDB::bind_integer_constant(cls, "Falloff", "FALLOFF_INVERSE_SQUARE", ForceZoneSettings::FALLOFF_INVERSE_SQUARE);

    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
    ClassDB::add_property_group(cls, U"🧲 Force", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Directional,Attract,Vortex,Drag"), "set_mode", "get_mode");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "strength", PROPERTY_HINT_RANGE, "-10000,10000,0.01,or_greater,or_less"), "set_strength", "get_strength");
    ClassDB::add_property(cls, PropertyInfo(p_vector_type, "direction"), "set_direction", "get_direction");
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "local_direction"), "set_local_direction", "get_local_direction");
    ClassDB::add_property_group(cls, U"📉 Atténuation et turbulence", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::INT, "falloff", PROPERTY_HINT_ENUM, "None,Linear,Inverse Square"), "set_falloff", "get_falloff");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "falloff_radius", PROPERTY_HINT_RANGE, "0.001,5000,0.01,or_greater"), "set_falloff_radius", "get_falloff_radius");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "turbulence", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_turbulence", "get_turbulence");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "turbulence_speed", PROPERTY_HINT_RANGE, "0,20,0.01,or_greater"), "set_turbulence_speed", "get_turbulence_speed");
    ClassDB::add_property_group(cls, U"🎯 Cibles", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "affect_rigid_bodies"), "set_affect_rigid_bodies", "get_affect_rigid_bodies");
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "affect_characters"), "set_affect_characters", "get_affect_characters");
}
} // namespace

// 🧲 2D

ForceZone2D::ForceZone2D() {}

Vector2 ForceZone2D::compute_acceleration(const Vector2 &p_point, const Vector2 &p_velocity) const
{
    if (!settings.enabled)
    {
        return Vector2();
    }
    Vector2 center = is_inside_tree() ? get_global_position() : get_position();
    Vector2 offset = p_point - center;
    double distance = offset.length();
    double intensity = settings.strength * settings.attenuation(distance) * settings.turbulence_factor(elapsed, double(get_instance_id() % 997));
    switch (settings.mode)
    {
    case ForceZoneSettings::MODE_DIRECTIONAL:
    {
        Vector2 heading = settings.local_direction && is_inside_tree() ? direction.rotated(get_global_rotation()) : direction;
        return heading.normalized() * intensity;
    }
    case ForceZoneSettings::MODE_ATTRACT:
        return distance > 0.001 ? -offset / distance * intensity : Vector2();
    case ForceZoneSettings::MODE_VORTEX:
        return distance > 0.001 ? Vector2(-offset.y, offset.x) / distance * intensity : Vector2();
    case ForceZoneSettings::MODE_DRAG:
        return -p_velocity * intensity; // strength : coefficient de freinage (par seconde)
    }
    return Vector2();
}

Vector2 ForceZone2D::get_acceleration_on(Node *p_body)
{
    Node2D *body = Object::cast_to<Node2D>(p_body);
    if (!body || !body->is_inside_tree())
    {
        return Vector2();
    }
    CharacterBody2D *character = Object::cast_to<CharacterBody2D>(body);
    Vector2 velocity = character ? character->get_velocity() : Vector2();
    Vector2 total;
    for (uint32_t i = 0; i < zones.size(); i++)
    {
        ForceZone2D *zone = Object::cast_to<ForceZone2D>(ObjectDB::get_instance(zones[i]));
        if (zone && zone->settings.enabled && zone->settings.affect_characters && zone->overlaps_body(body))
        {
            total += zone->compute_acceleration(body->get_global_position(), velocity);
        }
    }
    return total;
}

void ForceZone2D::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    switch (p_what)
    {
    case NOTIFICATION_ENTER_TREE:
        zones.push_back(ObjectID(get_instance_id()));
        set_physics_process(true);
        break;
    case NOTIFICATION_EXIT_TREE:
    {
        int64_t index = zones.find(ObjectID(get_instance_id()));
        if (index >= 0)
        {
            zones.remove_at(index);
        }
        break;
    }
    case NOTIFICATION_PHYSICS_PROCESS:
    {
        elapsed += get_physics_process_delta_time();
        if (!settings.enabled || !settings.affect_rigid_bodies)
        {
            break;
        }
        TypedArray<Node2D> bodies = get_overlapping_bodies();
        for (int i = 0; i < bodies.size(); i++)
        {
            if (RigidBody2D *rigid = Object::cast_to<RigidBody2D>(bodies[i]))
            {
                rigid->apply_central_force(compute_acceleration(rigid->get_global_position(), rigid->get_linear_velocity()) * rigid->get_mass());
            }
        }
        break;
    }
    default:
        break;
    }
}

void ForceZone2D::_bind_methods() { bind_force_zone_common<ForceZone2D>(Variant::VECTOR2); }

// 🧲 3D

ForceZone3D::ForceZone3D()
{
    settings.strength = 15.0;
    settings.falloff_radius = 5.0;
}

Vector3 ForceZone3D::compute_acceleration(const Vector3 &p_point, const Vector3 &p_velocity) const
{
    if (!settings.enabled)
    {
        return Vector3();
    }
    bool inside = is_inside_tree();
    Vector3 center = inside ? get_global_position() : get_position();
    Vector3 offset = p_point - center;
    double distance = offset.length();
    double intensity = settings.strength * settings.attenuation(distance) * settings.turbulence_factor(elapsed, double(get_instance_id() % 997));
    Basis basis = inside ? get_global_transform().basis.orthonormalized() : Basis();
    switch (settings.mode)
    {
    case ForceZoneSettings::MODE_DIRECTIONAL:
    {
        Vector3 heading = settings.local_direction ? basis.xform(direction) : direction;
        return heading.normalized() * intensity;
    }
    case ForceZoneSettings::MODE_ATTRACT:
        return distance > 0.001 ? -offset / distance * intensity : Vector3();
    case ForceZoneSettings::MODE_VORTEX:
    {
        Vector3 axis = basis.get_column(1).normalized();
        Vector3 planar = offset - axis * axis.dot(offset);
        Vector3 tangent = axis.cross(planar);
        return tangent.length() > 0.001 ? tangent.normalized() * intensity : Vector3();
    }
    case ForceZoneSettings::MODE_DRAG:
        return -p_velocity * intensity;
    }
    return Vector3();
}

Vector3 ForceZone3D::get_acceleration_on(Node *p_body)
{
    Node3D *body = Object::cast_to<Node3D>(p_body);
    if (!body || !body->is_inside_tree())
    {
        return Vector3();
    }
    CharacterBody3D *character = Object::cast_to<CharacterBody3D>(body);
    Vector3 velocity = character ? character->get_velocity() : Vector3();
    Vector3 total;
    for (uint32_t i = 0; i < zones.size(); i++)
    {
        ForceZone3D *zone = Object::cast_to<ForceZone3D>(ObjectDB::get_instance(zones[i]));
        if (zone && zone->settings.enabled && zone->settings.affect_characters && zone->overlaps_body(body))
        {
            total += zone->compute_acceleration(body->get_global_position(), velocity);
        }
    }
    return total;
}

void ForceZone3D::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    switch (p_what)
    {
    case NOTIFICATION_ENTER_TREE:
        zones.push_back(ObjectID(get_instance_id()));
        set_physics_process(true);
        break;
    case NOTIFICATION_EXIT_TREE:
    {
        int64_t index = zones.find(ObjectID(get_instance_id()));
        if (index >= 0)
        {
            zones.remove_at(index);
        }
        break;
    }
    case NOTIFICATION_PHYSICS_PROCESS:
    {
        elapsed += get_physics_process_delta_time();
        if (!settings.enabled || !settings.affect_rigid_bodies)
        {
            break;
        }
        TypedArray<Node3D> bodies = get_overlapping_bodies();
        for (int i = 0; i < bodies.size(); i++)
        {
            if (RigidBody3D *rigid = Object::cast_to<RigidBody3D>(bodies[i]))
            {
                rigid->apply_central_force(compute_acceleration(rigid->get_global_position(), rigid->get_linear_velocity()) * rigid->get_mass());
            }
        }
        break;
    }
    default:
        break;
    }
}

void ForceZone3D::_bind_methods() { bind_force_zone_common<ForceZone3D>(Variant::VECTOR3); }
