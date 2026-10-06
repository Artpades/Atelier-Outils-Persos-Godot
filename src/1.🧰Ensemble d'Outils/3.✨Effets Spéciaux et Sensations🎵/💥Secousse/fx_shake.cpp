#include "fx_shake.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

FXShake::FXShake()
{
    noise.instantiate();
    noise->set_noise_type(FastNoiseLite::TYPE_SIMPLEX);
    noise->set_frequency(1.0f); // la fréquence de l'effet est appliquée au temps échantillonné
}

// 🎬 Cycle de vie

void FXShake::_play(Node *p_target)
{
    // Relancée en pleine secousse : la position actuelle est décalée, l'origine du premier jeu est conservée
    if (!_is_restarting())
    {
        if (Control *control = Object::cast_to<Control>(p_target))
        {
            initial_position_2d = control->get_position();
        }
        else if (Node2D *node_2d = Object::cast_to<Node2D>(p_target))
        {
            initial_position_2d = node_2d->get_position();
        }
        else if (Node3D *node_3d = Object::cast_to<Node3D>(p_target))
        {
            initial_position_3d = node_3d->get_position();
        }
    }

    noise->set_seed((int)UtilityFunctions::randi());
    timer = 0.0;
}

void FXShake::_process(double p_delta, Node *p_target)
{
    if (!p_target)
    {
        return;
    }

    timer += p_delta;
    if (duration > 0.0f && timer >= duration)
    {
        stop_fx(p_target);
        return;
    }

    const float intensity = decrease_over_time && duration > 0.0f ? 1.0f - (float)(timer / duration) : 1.0f;
    const float amplitude = max_amplitude * intensity;

    if (Node3D *node_3d = Object::cast_to<Node3D>(p_target))
    {
        node_3d->set_position(initial_position_3d + Vector3(_sample(0), _sample(1), _sample(2)) * amplitude);
        return;
    }

    const Vector2 position = initial_position_2d + Vector2(_sample(0), _sample(1)) * amplitude;
    if (Control *control = Object::cast_to<Control>(p_target))
    {
        control->set_position(position);
    }
    else if (Node2D *node_2d = Object::cast_to<Node2D>(p_target))
    {
        node_2d->set_position(position);
    }
}

void FXShake::_stop(Node *p_target)
{
    if (Control *control = Object::cast_to<Control>(p_target))
    {
        control->set_position(initial_position_2d);
    }
    else if (Node2D *node_2d = Object::cast_to<Node2D>(p_target))
    {
        node_2d->set_position(initial_position_2d);
    }
    else if (Node3D *node_3d = Object::cast_to<Node3D>(p_target))
    {
        node_3d->set_position(initial_position_3d);
    }
}

float FXShake::_sample(int p_axis) const
{
    // Une ligne du bruit 2D par axe : trois courbes indépendantes
    return noise->get_noise_2d((float)timer * frequency, p_axis * 100.0f);
}

// 🔗 Liaisons

void FXShake::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_duration", "value"), &FXShake::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &FXShake::get_duration);
    ClassDB::bind_method(D_METHOD("set_frequency", "value"), &FXShake::set_frequency);
    ClassDB::bind_method(D_METHOD("get_frequency"), &FXShake::get_frequency);
    ClassDB::bind_method(D_METHOD("set_max_amplitude", "value"), &FXShake::set_max_amplitude);
    ClassDB::bind_method(D_METHOD("get_max_amplitude"), &FXShake::get_max_amplitude);
    ClassDB::bind_method(D_METHOD("set_decrease_over_time", "value"), &FXShake::set_decrease_over_time);
    ClassDB::bind_method(D_METHOD("get_decrease_over_time"), &FXShake::get_decrease_over_time);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0,5,0.01,or_greater,suffix:s"), "set_duration", "get_duration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frequency", PROPERTY_HINT_RANGE, "0.1,100,0.1,or_greater,suffix:Hz"), "set_frequency", "get_frequency");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_amplitude", PROPERTY_HINT_RANGE, "0,100,0.1,or_greater"), "set_max_amplitude", "get_max_amplitude");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "decrease_over_time"), "set_decrease_over_time", "get_decrease_over_time");
}
