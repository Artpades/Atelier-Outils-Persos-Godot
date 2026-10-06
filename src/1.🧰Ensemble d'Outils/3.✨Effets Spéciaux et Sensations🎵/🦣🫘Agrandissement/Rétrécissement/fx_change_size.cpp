#include "fx_change_size.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void FXChangeSize::_play(Node *p_target) { _tween_scale_to(p_target, final_scale); }

void FXChangeSize::_stop(Node *p_target) { _tween_scale_to(p_target, original_value); }

void FXChangeSize::_tween_scale_to(Node *p_target, float p_scale)
{
    if (!p_target || !p_target->is_inside_tree())
    {
        return;
    }

    Variant final_value;
    if (Control *control = Object::cast_to<Control>(p_target))
    {
        control->set_pivot_offset(control->get_size() / 2.0f);
        final_value = Vector2(p_scale, p_scale);
    }
    else if (Object::cast_to<Node2D>(p_target))
    {
        final_value = Vector2(p_scale, p_scale);
    }
    else if (Object::cast_to<Node3D>(p_target))
    {
        final_value = Vector3(p_scale, p_scale, p_scale);
    }
    else
    {
        return;
    }

    // L'animation précédente (agrandissement interrompu par l'arrêt...) ne doit plus agir sur l'échelle
    if (tween.is_valid())
    {
        tween->kill();
    }
    // Tween lié à la cible : arrêté automatiquement si elle est libérée
    tween = p_target->create_tween();
    tween->tween_property(p_target, "scale", final_value, duration)->set_trans(transition)->set_ease(ease_type);
}

// 🔗 Liaisons

void FXChangeSize::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_final_scale", "value"), &FXChangeSize::set_final_scale);
    ClassDB::bind_method(D_METHOD("get_final_scale"), &FXChangeSize::get_final_scale);
    ClassDB::bind_method(D_METHOD("set_original_value", "value"), &FXChangeSize::set_original_value);
    ClassDB::bind_method(D_METHOD("get_original_value"), &FXChangeSize::get_original_value);
    ClassDB::bind_method(D_METHOD("set_duration", "value"), &FXChangeSize::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &FXChangeSize::get_duration);
    ClassDB::bind_method(D_METHOD("set_transition", "value"), &FXChangeSize::set_transition);
    ClassDB::bind_method(D_METHOD("get_transition"), &FXChangeSize::get_transition);
    ClassDB::bind_method(D_METHOD("set_ease_type", "value"), &FXChangeSize::set_ease_type);
    ClassDB::bind_method(D_METHOD("get_ease_type"), &FXChangeSize::get_ease_type);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "final_scale", PROPERTY_HINT_RANGE, "0,5,0.01,or_greater"), "set_final_scale", "get_final_scale");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "original_value", PROPERTY_HINT_RANGE, "0,5,0.01,or_greater"), "set_original_value", "get_original_value");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0,5,0.01,or_greater,suffix:s"), "set_duration", "get_duration");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "transition", PROPERTY_HINT_ENUM, "Linear,Sine,Quint,Quart,Quad,Expo,Elastic,Cubic,Circ,Bounce,Back,Spring"), "set_transition", "get_transition");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ease_type", PROPERTY_HINT_ENUM, "In,Out,In Out,Out In"), "set_ease_type", "get_ease_type");
}
