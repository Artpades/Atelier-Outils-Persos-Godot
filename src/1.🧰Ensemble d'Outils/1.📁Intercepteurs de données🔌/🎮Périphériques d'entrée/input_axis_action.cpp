#include "input_axis_action.hpp"

#include "input_names.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos_2d.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void InputAxisAction::_process(double p_delta)
{
    if (Engine::get_singleton()->is_editor_hint() || !action_enabled)
    {
        return;
    }

    float value = get_axis_value();
    if (Math::abs(value - last_emitted_value) >= dirty_threshold)
    {
        last_emitted_value = value;
        emit_signal("value_changed", value);
        log_debug("value_changed : " + String::num(value, 3));
    }

    if (p_delta > 0.0)
    {
        float velocity = (value - previous_value) / (float)p_delta;
        if (Math::abs(velocity - last_emitted_velocity) >= dirty_threshold)
        {
            last_emitted_velocity = velocity;
            emit_signal("velocity_changed", velocity);
        }
    }
    previous_value = value;

    draw_debug(value);
}

float InputAxisAction::get_axis_value(int p_device) const
{
    if (!action_enabled)
    {
        return 0.0f;
    }

    Input *input = Input::get_singleton();
    int device = resolve_device(p_device);
    float sign = invert_axis ? -1.0f : 1.0f;

    if (is_gamepad_allowed() && gamepad_axis != JOY_AXIS_INVALID)
    {
        float joy = input->get_joy_axis(device, gamepad_axis);
        if (Math::abs(joy) > dead_zone)
        {
            return Math::clamp(joy, -1.0f, 1.0f) * sign;
        }
    }

    float value = 0.0f;
    if (is_keyboard_mouse_allowed())
    {
        if (positive_key != KEY_NONE && input->is_key_pressed(positive_key))
        {
            value += 1.0f;
        }
        if (negative_key != KEY_NONE && input->is_key_pressed(negative_key))
        {
            value -= 1.0f;
        }
    }
    if (is_gamepad_allowed())
    {
        if (positive_gamepad_button != JOY_BUTTON_INVALID && input->is_joy_button_pressed(device, positive_gamepad_button))
        {
            value += 1.0f;
        }
        if (negative_gamepad_button != JOY_BUTTON_INVALID && input->is_joy_button_pressed(device, negative_gamepad_button))
        {
            value -= 1.0f;
        }
    }
    return Math::clamp(value, -1.0f, 1.0f) * sign;
}

PackedStringArray InputAxisAction::get_binding_properties() const
{
    return PackedStringArray{"positive_key", "negative_key", "gamepad_axis", "positive_gamepad_button", "negative_gamepad_button"};
}

void InputAxisAction::draw_debug(float p_value)
{
    if (!can_debug_draw())
    {
        return;
    }
    draw_debug_label();
    // Jauge horizontale : cadre de [-1, 1], barre pleine du centre jusqu'à la valeur.
    Vector2 half(debug_draw_size, debug_draw_size * 0.15f);
    Gizmos2D::draw_2D_rectangle(debug_gizmo(1), Rect2(debug_draw_position - half, half * 2.0f), Color(1, 1, 1, 0.5f), false, 2.0f);
    Rect2 bar(debug_draw_position.x, debug_draw_position.y - half.y, p_value * debug_draw_size, half.y * 2.0f);
    Gizmos2D::draw_2D_rectangle(debug_gizmo(2), bar.abs(), Color(0.3f, 0.8f, 1.0f), true);
}

void InputAxisAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_axis_value", "device"), &InputAxisAction::get_axis_value, DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("set_positive_key", "key"), &InputAxisAction::set_positive_key);
    ClassDB::bind_method(D_METHOD("get_positive_key"), &InputAxisAction::get_positive_key);
    ClassDB::bind_method(D_METHOD("set_negative_key", "key"), &InputAxisAction::set_negative_key);
    ClassDB::bind_method(D_METHOD("get_negative_key"), &InputAxisAction::get_negative_key);
    ClassDB::bind_method(D_METHOD("set_gamepad_axis", "axis"), &InputAxisAction::set_gamepad_axis);
    ClassDB::bind_method(D_METHOD("get_gamepad_axis"), &InputAxisAction::get_gamepad_axis);
    ClassDB::bind_method(D_METHOD("set_positive_gamepad_button", "button"), &InputAxisAction::set_positive_gamepad_button);
    ClassDB::bind_method(D_METHOD("get_positive_gamepad_button"), &InputAxisAction::get_positive_gamepad_button);
    ClassDB::bind_method(D_METHOD("set_negative_gamepad_button", "button"), &InputAxisAction::set_negative_gamepad_button);
    ClassDB::bind_method(D_METHOD("get_negative_gamepad_button"), &InputAxisAction::get_negative_gamepad_button);
    ClassDB::bind_method(D_METHOD("set_invert_axis", "invert"), &InputAxisAction::set_invert_axis);
    ClassDB::bind_method(D_METHOD("get_invert_axis"), &InputAxisAction::get_invert_axis);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "invert_axis"), "set_invert_axis", "get_invert_axis");

    ADD_GROUP(U"⌨️ Key Axis", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "positive_key", PROPERTY_HINT_ENUM, InputNames::key_hint()), "set_positive_key", "get_positive_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "negative_key", PROPERTY_HINT_ENUM, InputNames::key_hint()), "set_negative_key", "get_negative_key");

    ADD_GROUP(U"🔘 Joy Axis", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "gamepad_axis", PROPERTY_HINT_ENUM, InputNames::joy_axis_hint()), "set_gamepad_axis", "get_gamepad_axis");

    ADD_GROUP(U"🔲 Joy Button", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "positive_gamepad_button", PROPERTY_HINT_ENUM, InputNames::joy_button_hint()), "set_positive_gamepad_button", "get_positive_gamepad_button");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "negative_gamepad_button", PROPERTY_HINT_ENUM, InputNames::joy_button_hint()), "set_negative_gamepad_button", "get_negative_gamepad_button");

    ADD_SIGNAL(MethodInfo("value_changed", PropertyInfo(Variant::FLOAT, "value")));
    ADD_SIGNAL(MethodInfo("velocity_changed", PropertyInfo(Variant::FLOAT, "velocity")));
}
