#include "fx_stretch_and_squash.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

// 🎬 Cycle de vie

void FXStretchAndSquash::_play(Node *p_target)
{
    is_first_frame = true;
    current_impact_squash_2d = 0.0f;
    current_impact_squash_3d = 0.0f;
    last_velocity_2d = Vector2();
    last_velocity_3d = Vector3();
    if (_is_restarting())
    {
        return; // la cible est déjà déformée : échelle et rotation d'origine conservées
    }

    if (Control *control = Object::cast_to<Control>(p_target))
    {
        control->set_pivot_offset(control->get_size() / 2.0f);
        last_pos_2d = _get_control_pivot_position(control);
        initial_scale_2d = control->get_scale();
        initial_rotation_2d = control->get_rotation();
    }
    else if (Node2D *node_2d = Object::cast_to<Node2D>(p_target))
    {
        last_pos_2d = node_2d->get_global_position();
        initial_scale_2d = node_2d->get_scale();
        initial_rotation_2d = node_2d->get_rotation();
    }
    else if (Node3D *node_3d = Object::cast_to<Node3D>(p_target))
    {
        last_pos_3d = node_3d->get_global_position();
        initial_scale_3d = node_3d->get_scale();
        initial_basis_3d = node_3d->get_global_transform().basis;
    }
}

void FXStretchAndSquash::_process(double p_delta, Node *p_target)
{
    const float delta = (float)p_delta;
    if (!p_target || delta <= 0.0f)
    {
        return;
    }

    if (Control *control = Object::cast_to<Control>(p_target))
    {
        _process_2d(control, _get_control_pivot_position(control), delta);
    }
    else if (Node2D *node_2d = Object::cast_to<Node2D>(p_target))
    {
        _process_2d(node_2d, node_2d->get_global_position(), delta);
    }
    else if (Node3D *node_3d = Object::cast_to<Node3D>(p_target))
    {
        _process_3d(node_3d, delta);
    }
}

void FXStretchAndSquash::_stop(Node *p_target)
{
    if (Control *control = Object::cast_to<Control>(p_target))
    {
        control->set_scale(initial_scale_2d);
        control->set_rotation(initial_rotation_2d);
    }
    else if (Node2D *node_2d = Object::cast_to<Node2D>(p_target))
    {
        node_2d->set_scale(initial_scale_2d);
        node_2d->set_rotation(initial_rotation_2d);
    }
    else if (Node3D *node_3d = Object::cast_to<Node3D>(p_target))
    {
        node_3d->set_scale(initial_scale_3d);
        Transform3D transform = node_3d->get_global_transform();
        transform.basis = initial_basis_3d;
        node_3d->set_global_transform(transform);
    }
}

// 🟦 2D (Control et Node2D)

Vector2 FXStretchAndSquash::_get_control_pivot_position(Control *p_control)
{
    return p_control->get_global_position() + p_control->get_pivot_offset() * p_control->get_scale();
}

void FXStretchAndSquash::_process_2d(Node *p_target, const Vector2 &p_current_pos, float p_delta)
{
    if (is_first_frame)
    {
        last_pos_2d = p_current_pos;
        is_first_frame = false;
        return;
    }

    const Vector2 velocity = (p_current_pos - last_pos_2d) / p_delta;
    last_pos_2d = p_current_pos;
    _apply_deformation_2d(p_target, velocity, p_delta);
}

void FXStretchAndSquash::_apply_deformation_2d(Node *p_target, const Vector2 &p_velocity, float p_delta)
{
    Control *control = Object::cast_to<Control>(p_target);
    Node2D *node_2d = Object::cast_to<Node2D>(p_target);

    // Détection d'impact (chute brutale de vitesse)
    if (enable_impact_squash)
    {
        const float speed_drop = last_velocity_2d.length() - p_velocity.length();
        if (speed_drop > impact_threshold)
        {
            current_impact_squash_2d = Math::clamp(speed_drop / max_speed, 0.0f, 1.0f) * max_impact_squash;
            last_impact_dir_2d = last_velocity_2d.normalized();
        }

        last_velocity_2d = p_velocity;
        current_impact_squash_2d = Math::lerp(current_impact_squash_2d, 0.0f, p_delta * impact_recovery_speed);
    }

    const float speed = p_velocity.length();
    const float stretch_intensity = Math::clamp(speed / max_speed, 0.0f, 1.0f) * max_stretch;

    const Vector2 current_scale = node_2d ? node_2d->get_scale() : (control ? control->get_scale() : Vector2(1, 1));
    Vector2 target_scale;

    if (orient_to_velocity && speed > 1.0f)
    {
        const float angle = p_velocity.angle();
        if (node_2d)
        {
            node_2d->set_rotation(angle);
        }
        else if (control)
        {
            control->set_rotation(angle);
        }

        const float factor = 1.0f + stretch_intensity;
        target_scale = Vector2(initial_scale_2d.x * factor, initial_scale_2d.y / factor);
    }
    else
    {
        float wx = 0.0f;
        float wy = 0.0f;
        if (speed > 1.0f)
        {
            wx = Math::abs(p_velocity.x) / speed;
            wy = Math::abs(p_velocity.y) / speed;
        }

        const float base = Math::max(1.0f + stretch_intensity, 0.01f);
        target_scale = Vector2(initial_scale_2d.x * Math::pow(base, wx - wy), initial_scale_2d.y * Math::pow(base, wy - wx));
    }

    // Déformation d'impact si active
    if (current_impact_squash_2d > 0.001f)
    {
        const float impact_wx = Math::abs(last_impact_dir_2d.x);
        const float impact_wy = Math::abs(last_impact_dir_2d.y);

        // S'il tombait verticalement (impact Y), on écrase Y et étire X
        const float squashed_y = 1.0f - current_impact_squash_2d * impact_wy;
        const float stretched_x = 1.0f + current_impact_squash_2d * impact_wy * 0.5f;
        const float squashed_x = 1.0f - current_impact_squash_2d * impact_wx;
        const float stretched_y = 1.0f + current_impact_squash_2d * impact_wx * 0.5f;

        target_scale.x *= squashed_x * stretched_x;
        target_scale.y *= squashed_y * stretched_y;
    }

    const Vector2 new_scale = current_scale.lerp(target_scale, p_delta * smooth_speed);
    if (node_2d)
    {
        node_2d->set_scale(new_scale);
    }
    else if (control)
    {
        control->set_scale(new_scale);
    }
}

// 🧊 3D

void FXStretchAndSquash::_process_3d(Node3D *p_node, float p_delta)
{
    const Vector3 current_pos = p_node->get_global_position();
    if (is_first_frame)
    {
        last_pos_3d = current_pos;
        is_first_frame = false;
        return;
    }

    const Vector3 velocity = (current_pos - last_pos_3d) / p_delta;
    last_pos_3d = current_pos;

    if (enable_impact_squash)
    {
        const float speed_drop = last_velocity_3d.length() - velocity.length();
        if (speed_drop > impact_threshold)
        {
            current_impact_squash_3d = Math::clamp(speed_drop / max_speed, 0.0f, 1.0f) * max_impact_squash;
            last_impact_dir_3d = last_velocity_3d.normalized();
        }

        last_velocity_3d = velocity;
        current_impact_squash_3d = Math::lerp(current_impact_squash_3d, 0.0f, p_delta * impact_recovery_speed);
    }

    const float speed = velocity.length();
    const float stretch_intensity = Math::clamp(speed / max_speed, 0.0f, 1.0f) * max_stretch;
    Vector3 target_scale;

    if (orient_to_velocity && speed > 0.1f)
    {
        // look_at échoue si la direction est parallèle au vecteur haut : autre vecteur haut dans ce cas
        const Vector3 direction = velocity / speed;
        const Vector3 up = Math::abs(direction.dot(Vector3(0, 1, 0))) > 0.999f ? Vector3(0, 0, 1) : Vector3(0, 1, 0);
        p_node->look_at(p_node->get_global_position() + velocity, up);

        const float factor = 1.0f + stretch_intensity;
        const float perp_factor = 1.0f / Math::sqrt(Math::max(factor, 0.01f));
        target_scale = Vector3(initial_scale_3d.x * perp_factor, initial_scale_3d.y * perp_factor, initial_scale_3d.z * factor);
    }
    else
    {
        float wx = 0.0f;
        float wy = 0.0f;
        float wz = 0.0f;
        if (speed > 0.1f)
        {
            wx = Math::abs(velocity.x) / speed;
            wy = Math::abs(velocity.y) / speed;
            wz = Math::abs(velocity.z) / speed;
        }

        const float base = Math::max(1.0f + stretch_intensity, 0.01f);
        target_scale = Vector3(
                initial_scale_3d.x * Math::pow(base, wx - (wy + wz) * 0.5f),
                initial_scale_3d.y * Math::pow(base, wy - (wx + wz) * 0.5f),
                initial_scale_3d.z * Math::pow(base, wz - (wx + wy) * 0.5f));
    }

    if (current_impact_squash_3d > 0.001f)
    {
        const float wy = Math::abs(last_impact_dir_3d.y);
        const float side_stretch = 1.0f + current_impact_squash_3d * wy * 0.5f;
        target_scale.y *= 1.0f - current_impact_squash_3d * wy;
        target_scale.x *= side_stretch;
        target_scale.z *= side_stretch;
    }

    p_node->set_scale(p_node->get_scale().lerp(target_scale, p_delta * smooth_speed));
}

// 🔗 Liaisons

void FXStretchAndSquash::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_max_speed", "value"), &FXStretchAndSquash::set_max_speed);
    ClassDB::bind_method(D_METHOD("get_max_speed"), &FXStretchAndSquash::get_max_speed);
    ClassDB::bind_method(D_METHOD("set_max_stretch", "value"), &FXStretchAndSquash::set_max_stretch);
    ClassDB::bind_method(D_METHOD("get_max_stretch"), &FXStretchAndSquash::get_max_stretch);
    ClassDB::bind_method(D_METHOD("set_smooth_speed", "value"), &FXStretchAndSquash::set_smooth_speed);
    ClassDB::bind_method(D_METHOD("get_smooth_speed"), &FXStretchAndSquash::get_smooth_speed);
    ClassDB::bind_method(D_METHOD("set_orient_to_velocity", "value"), &FXStretchAndSquash::set_orient_to_velocity);
    ClassDB::bind_method(D_METHOD("get_orient_to_velocity"), &FXStretchAndSquash::get_orient_to_velocity);
    ClassDB::bind_method(D_METHOD("set_enable_impact_squash", "value"), &FXStretchAndSquash::set_enable_impact_squash);
    ClassDB::bind_method(D_METHOD("get_enable_impact_squash"), &FXStretchAndSquash::get_enable_impact_squash);
    ClassDB::bind_method(D_METHOD("set_impact_threshold", "value"), &FXStretchAndSquash::set_impact_threshold);
    ClassDB::bind_method(D_METHOD("get_impact_threshold"), &FXStretchAndSquash::get_impact_threshold);
    ClassDB::bind_method(D_METHOD("set_max_impact_squash", "value"), &FXStretchAndSquash::set_max_impact_squash);
    ClassDB::bind_method(D_METHOD("get_max_impact_squash"), &FXStretchAndSquash::get_max_impact_squash);
    ClassDB::bind_method(D_METHOD("set_impact_recovery_speed", "value"), &FXStretchAndSquash::set_impact_recovery_speed);
    ClassDB::bind_method(D_METHOD("get_impact_recovery_speed"), &FXStretchAndSquash::get_impact_recovery_speed);

    ADD_GROUP("Stretch & Squash", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_speed", PROPERTY_HINT_RANGE, "1,2000,1,or_greater,suffix:px/s"), "set_max_speed", "get_max_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_stretch", PROPERTY_HINT_RANGE, "0,10,0.05,or_greater"), "set_max_stretch", "get_max_stretch");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "smooth_speed"), "set_smooth_speed", "get_smooth_speed");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "orient_to_velocity"), "set_orient_to_velocity", "get_orient_to_velocity");

    ADD_GROUP("Impact / Landing", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_impact_squash"), "set_enable_impact_squash", "get_enable_impact_squash");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "impact_threshold"), "set_impact_threshold", "get_impact_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_impact_squash", PROPERTY_HINT_RANGE, "0,5,0.05,or_greater"), "set_max_impact_squash", "get_max_impact_squash");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "impact_recovery_speed"), "set_impact_recovery_speed", "get_impact_recovery_speed");
}
