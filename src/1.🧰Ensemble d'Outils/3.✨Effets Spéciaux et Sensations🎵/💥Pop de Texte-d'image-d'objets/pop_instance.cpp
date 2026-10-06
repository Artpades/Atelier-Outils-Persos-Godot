#include "pop_instance.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/sprite_base3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// 🚀 Mouvement

PopMotion PopMotion::launch(const PopStyle &p_style, bool p_3d)
{
    PopMotion motion;
    const float spread = Math::deg_to_rad(p_style.get_spread_degrees());
    const float tilt = Math::deg_to_rad(p_style.get_direction_degrees()) + (float)UtilityFunctions::randf_range(-spread / 2.0f, spread / 2.0f);
    const float speed = (float)UtilityFunctions::randf_range(Math::min(p_style.get_speed_min(), p_style.get_speed_max()), Math::max(p_style.get_speed_min(), p_style.get_speed_max()));
    if (p_3d)
    {
        // Inclinaison depuis la verticale, dans une direction horizontale au hasard
        const float azimuth = (float)UtilityFunctions::randf_range(0.0, Math_TAU);
        motion.velocity = Vector3(Math::sin(tilt) * Math::cos(azimuth), Math::cos(tilt), Math::sin(tilt) * Math::sin(azimuth)) * speed;
    }
    else
    {
        // 0° : vers le haut, angles positifs vers la droite
        motion.velocity = Vector3(Math::sin(tilt), Math::cos(tilt), 0.0f) * speed;
    }

    const float radius = p_style.get_spawn_radius();
    if (radius > 0.0f)
    {
        const float angle = (float)UtilityFunctions::randf_range(0.0, Math_TAU);
        const float distance = radius * Math::sqrt((float)UtilityFunctions::randf()); // répartition uniforme dans le disque
        motion.offset = p_3d ? Vector3(Math::cos(angle), 0.0f, Math::sin(angle)) * distance : Vector3(Math::cos(angle), Math::sin(angle), 0.0f) * distance;
    }

    motion.spin = Math::deg_to_rad((float)UtilityFunctions::randf_range(p_style.get_spin_min(), p_style.get_spin_max()));
    return motion;
}

void PopMotion::step(const PopStyle &p_style, double p_delta)
{
    const float delta = (float)p_delta;
    age += p_delta;
    velocity.y -= p_style.get_gravity() * delta;
    if (p_style.get_damping() > 0.0f)
    {
        velocity *= Math::exp(-p_style.get_damping() * delta);
    }
    offset += velocity * delta;
    angle += spin * delta;

    // Sol : rebond amorti, frottement horizontal et rotation freinée
    const float floor = -p_style.get_floor_offset();
    if (p_style.get_use_floor() && offset.y < floor)
    {
        offset.y = floor;
        if (velocity.y < 0.0f)
        {
            velocity.y = -velocity.y * p_style.get_bounce();
            if (velocity.y < 30.0f)
            {
                velocity.y = 0.0f; // posé
            }
            velocity.x *= p_style.get_floor_friction();
            velocity.z *= p_style.get_floor_friction();
            spin *= p_style.get_floor_friction();
        }
    }
}

// 🟦 2D

void PopInstance2D::setup(const Ref<PopStyle> &p_style, const Vector2 &p_origin, const PopMotion &p_motion)
{
    style = p_style;
    origin = p_origin;
    motion = p_motion;
    set_z_index(style->get_z_index());
    _update();
}

void PopInstance2D::_process(double p_delta)
{
    if (style.is_null())
    {
        return;
    }
    motion.step(**style, p_delta);
    if (motion.age >= style->get_lifetime())
    {
        queue_free();
        return;
    }
    _update();
}

void PopInstance2D::_update()
{
    const float scale = style->get_scale_at(motion.age);
    set_global_position(origin + Vector2(motion.offset.x, -motion.offset.y));
    set_rotation(motion.angle);
    set_scale(Vector2(scale, scale));
    Color modulate = get_modulate();
    modulate.a = style->get_alpha_at(motion.age);
    set_modulate(modulate);
}

void PopInstance2D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_age"), &PopInstance2D::get_age);
    ClassDB::bind_method(D_METHOD("get_velocity"), &PopInstance2D::get_velocity);
    ClassDB::bind_method(D_METHOD("get_style"), &PopInstance2D::get_style);
}

// 🧊 3D

void PopInstance3D::setup(const Ref<PopStyle> &p_style, const Vector3 &p_origin, const PopMotion &p_motion, float p_unit_scale)
{
    style = p_style;
    origin = p_origin;
    motion = p_motion;
    unit_scale = p_unit_scale;
    _update();
}

void PopInstance3D::_process(double p_delta)
{
    if (style.is_null())
    {
        return;
    }
    motion.step(**style, p_delta);
    if (motion.age >= style->get_lifetime())
    {
        queue_free();
        return;
    }
    _update();
}

void PopInstance3D::_update()
{
    if (!is_inside_tree())
    {
        return;
    }
    const float scale = Math::max(style->get_scale_at(motion.age), 0.001f); // base nulle : non inversible

    // Face à la caméra, rotation (spin) autour de l'axe de vue
    Basis basis;
    Camera3D *camera = get_viewport()->get_camera_3d();
    if (camera)
    {
        basis = camera->get_global_basis().orthonormalized();
    }
    basis = basis * Basis(Vector3(0, 0, 1), -motion.angle) * Basis::from_scale(Vector3(scale, scale, scale));
    set_global_transform(Transform3D(basis, origin + motion.offset * unit_scale));

    _apply_alpha(this, style->get_alpha_at(motion.age));
}

void PopInstance3D::_apply_alpha(Node *p_node, float p_alpha)
{
    if (Label3D *label = Object::cast_to<Label3D>(p_node))
    {
        Color color = label->get_modulate();
        color.a = p_alpha;
        label->set_modulate(color);
        Color outline = label->get_outline_modulate();
        outline.a = p_alpha;
        label->set_outline_modulate(outline);
    }
    else if (SpriteBase3D *sprite = Object::cast_to<SpriteBase3D>(p_node))
    {
        Color color = sprite->get_modulate();
        color.a = p_alpha;
        sprite->set_modulate(color);
    }
    else if (GeometryInstance3D *geometry = Object::cast_to<GeometryInstance3D>(p_node))
    {
        geometry->set_transparency(1.0f - p_alpha);
    }
    for (int64_t i = 0; i < p_node->get_child_count(); i++)
    {
        _apply_alpha(p_node->get_child(i), p_alpha);
    }
}

void PopInstance3D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_age"), &PopInstance3D::get_age);
    ClassDB::bind_method(D_METHOD("get_velocity"), &PopInstance3D::get_velocity);
    ClassDB::bind_method(D_METHOD("get_style"), &PopInstance3D::get_style);
}
