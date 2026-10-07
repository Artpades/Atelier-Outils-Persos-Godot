#include "jiggle.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace
{
template <typename T>
void bind_jiggle_common(Variant::Type p_vector_type)
{
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &T::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &T::is_enabled);
    ClassDB::bind_method(D_METHOD("set_stiffness", "value"), &T::set_stiffness);
    ClassDB::bind_method(D_METHOD("get_stiffness"), &T::get_stiffness);
    ClassDB::bind_method(D_METHOD("set_damping", "value"), &T::set_damping);
    ClassDB::bind_method(D_METHOD("get_damping"), &T::get_damping);
    ClassDB::bind_method(D_METHOD("set_max_offset", "value"), &T::set_max_offset);
    ClassDB::bind_method(D_METHOD("get_max_offset"), &T::get_max_offset);
    ClassDB::bind_method(D_METHOD("set_lean_amount", "value"), &T::set_lean_amount);
    ClassDB::bind_method(D_METHOD("get_lean_amount"), &T::get_lean_amount);
    ClassDB::bind_method(D_METHOD("set_squash_amount", "value"), &T::set_squash_amount);
    ClassDB::bind_method(D_METHOD("get_squash_amount"), &T::get_squash_amount);
    ClassDB::bind_method(D_METHOD("set_max_stretch", "value"), &T::set_max_stretch);
    ClassDB::bind_method(D_METHOD("get_max_stretch"), &T::get_max_stretch);
    ClassDB::bind_method(D_METHOD("set_gravity", "gravity"), &T::set_gravity);
    ClassDB::bind_method(D_METHOD("get_gravity"), &T::get_gravity);
    ClassDB::bind_method(D_METHOD("impulse", "velocity"), &T::impulse);
    ClassDB::bind_method(D_METHOD("reset"), &T::reset);
    ClassDB::bind_method(D_METHOD("get_offset"), &T::get_offset);

    StringName cls = T::get_class_static();
    ClassDB::add_property_group(cls, U"🫨 Ressort", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "stiffness", PROPERTY_HINT_RANGE, "0,2000,0.01,or_greater"), "set_stiffness", "get_stiffness");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "damping", PROPERTY_HINT_RANGE, "0,60,0.01,or_greater"), "set_damping", "get_damping");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "max_offset", PROPERTY_HINT_RANGE, "0,500,0.01,or_greater"), "set_max_offset", "get_max_offset");
    ClassDB::add_property(cls, PropertyInfo(p_vector_type, "gravity"), "set_gravity", "get_gravity");
    ClassDB::add_property_group(cls, U"🎭 Déformation", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "lean_amount", PROPERTY_HINT_RANGE, "-1,1,0.0001,or_greater,or_less"), "set_lean_amount", "get_lean_amount");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "squash_amount", PROPERTY_HINT_RANGE, "0,0.1,0.00001,or_greater"), "set_squash_amount", "get_squash_amount");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "max_stretch", PROPERTY_HINT_RANGE, "0,0.95,0.01"), "set_max_stretch", "get_max_stretch");
}
} // namespace

// 🫨 2D

void Jiggle2D::reset()
{
    initialized = false;
    simulated_velocity = Vector2();
    offset = Vector2();
    if (initialized || is_inside_tree())
    {
        set_position(rest_position);
        set_rotation(rest_rotation);
        set_scale(rest_scale);
    }
}

void Jiggle2D::_step(double p_delta)
{
    Node2D *parent = Object::cast_to<Node2D>(get_parent());
    Transform2D parent_transform = parent ? parent->get_global_transform() : Transform2D();
    Vector2 target = parent_transform.xform(rest_position);
    if (!initialized)
    {
        simulated_position = target;
        simulated_velocity = Vector2();
        initialized = true;
    }
    if (!jiggle.enabled)
    {
        set_position(rest_position);
        return;
    }
    Vector2 acceleration = (target - simulated_position) * jiggle.stiffness + gravity;
    simulated_velocity += acceleration * p_delta;
    simulated_velocity *= Math::exp(-jiggle.damping * p_delta);
    simulated_position += simulated_velocity * p_delta;
    offset = (simulated_position - target).limit_length(jiggle.max_offset);
    simulated_position = target + offset;

    set_global_position(target + offset);
    Vector2 local_offset = parent_transform.basis_xform_inv(offset);
    set_rotation(rest_rotation + local_offset.x * jiggle.lean_amount);
    double stretch = MIN(simulated_velocity.length() * jiggle.squash_amount, jiggle.max_stretch);
    set_scale(rest_scale * Vector2(1.0 - stretch * 0.5, 1.0 + stretch));
}

void Jiggle2D::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_READY)
    {
        rest_position = get_position();
        rest_rotation = get_rotation();
        rest_scale = get_scale();
        set_process(true);
    }
    else if (p_what == NOTIFICATION_PROCESS)
    {
        _step(get_process_delta_time());
        if (debug_mode)
        {
            _draw_debug();
        }
    }
    else if (p_what == NOTIFICATION_EXIT_TREE)
    {
        debug.release();
    }
}

void Jiggle2D::_draw_debug()
{
    debug.begin(this, false);
    Vector2 current = get_global_position();
    Vector2 rest = current - offset;
    debug.circle(rest, float(jiggle.max_offset), Color(1.0f, 0.6f, 0.9f, 0.5f), false, 1.0f);
    debug.line(rest, current, debug_colors::HEADING, 2.0f);
    debug.circle(rest, 2.5f, debug_colors::HEADING, true);
    debug.end();
}

void Jiggle2D::_bind_methods()
{
    bind_jiggle_common<Jiggle2D>(Variant::VECTOR2);
    bind_debug_mode<Jiggle2D>();
}

// 🫨 3D

Jiggle3D::Jiggle3D()
{
    jiggle.max_offset = 0.4;
    jiggle.lean_amount = 1.2;
    jiggle.squash_amount = 0.05;
}

void Jiggle3D::reset()
{
    initialized = false;
    simulated_velocity = Vector3();
    offset = Vector3();
    if (is_inside_tree())
    {
        set_transform(rest_transform);
    }
}

void Jiggle3D::_step(double p_delta)
{
    Node3D *parent = Object::cast_to<Node3D>(get_parent());
    Transform3D parent_transform = parent ? parent->get_global_transform() : Transform3D();
    Vector3 target = parent_transform.xform(rest_transform.origin);
    if (!initialized)
    {
        simulated_position = target;
        simulated_velocity = Vector3();
        initialized = true;
    }
    if (!jiggle.enabled)
    {
        set_transform(rest_transform);
        return;
    }
    Vector3 acceleration = (target - simulated_position) * jiggle.stiffness + gravity;
    simulated_velocity += acceleration * p_delta;
    simulated_velocity *= Math::exp(-jiggle.damping * p_delta);
    simulated_position += simulated_velocity * p_delta;
    offset = (simulated_position - target).limit_length(jiggle.max_offset);
    simulated_position = target + offset;

    // Inclinaison : rotation autour de l'axe perpendiculaire au décalage (dans le repère du parent).
    Vector3 local_offset = parent_transform.basis.xform_inv(offset);
    Basis basis = rest_transform.basis;
    Vector3 axis = Vector3(0, 1, 0).cross(local_offset);
    if (axis.length_squared() > 1e-10)
    {
        basis = Basis(axis.normalized(), local_offset.length() * jiggle.lean_amount) * basis;
    }
    double stretch = MIN(simulated_velocity.length() * jiggle.squash_amount, jiggle.max_stretch);
    basis = basis * Basis::from_scale(Vector3(1.0 - stretch * 0.5, 1.0 + stretch, 1.0 - stretch * 0.5));
    set_transform(Transform3D(basis, rest_transform.origin));
    set_global_position(target + offset);
}

void Jiggle3D::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_READY)
    {
        rest_transform = get_transform();
        set_process(true);
    }
    else if (p_what == NOTIFICATION_PROCESS)
    {
        _step(get_process_delta_time());
        if (debug_mode)
        {
            _draw_debug();
        }
    }
    else if (p_what == NOTIFICATION_EXIT_TREE)
    {
        debug.release();
    }
}

void Jiggle3D::_draw_debug()
{
    debug.begin(this, true);
    Vector3 current = get_global_position();
    Vector3 rest = current - offset;
    debug.sphere3(rest, float(jiggle.max_offset), Color(1.0f, 0.6f, 0.9f, 0.5f));
    debug.line3(rest, current, debug_colors::HEADING, 2.0f);
    debug.end();
}

void Jiggle3D::_bind_methods()
{
    bind_jiggle_common<Jiggle3D>(Variant::VECTOR3);
    bind_debug_mode<Jiggle3D>();
}
