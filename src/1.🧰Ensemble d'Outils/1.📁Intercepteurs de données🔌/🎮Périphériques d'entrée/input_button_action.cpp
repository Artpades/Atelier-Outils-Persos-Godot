#include "input_button_action.hpp"

#include "input_names.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos_api.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void InputButtonAction::_process(double p_delta)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (!action_enabled)
    {
        // Désactivée pendant un appui : on libère, sinon l'écouteur ne recevrait jamais released.
        if (was_pressed)
        {
            release();
        }
        return;
    }

    bool pressed = is_button_pressed();

    if (pressed && !was_pressed)
    {
        was_pressed = true;
        held_timer = 0.0f;
        held_emitted = false;
        emit_signal("pressed");
        log_debug("pressed");

        if (multi_press_timer > multi_press_delay)
        {
            press_count = 0;
        }
        press_count++;
        multi_press_timer = 0.0f;

        if (press_count >= multi_press_count)
        {
            emit_signal("multi_pressed");
            log_debug("multi_pressed (" + itos(press_count) + ")");
            press_count = 0;
        }
    }
    else if (!pressed && was_pressed)
    {
        release();
    }

    if (pressed && !held_emitted)
    {
        held_timer += (float)p_delta;
        if (held_timer >= held_delay)
        {
            held_emitted = true;
            emit_signal("held");
            log_debug("held");
        }
    }

    if (press_count > 0)
    {
        multi_press_timer += (float)p_delta;
        if (multi_press_timer > multi_press_delay)
        {
            press_count = 0;
            multi_press_timer = 0.0f;
        }
    }

    draw_debug();
}

void InputButtonAction::release()
{
    was_pressed = false;
    emit_signal("released");
    log_debug("released");
}

bool InputButtonAction::is_button_pressed(int p_device) const
{
    Input *input = Input::get_singleton();
    int device = resolve_device(p_device);

    if (is_keyboard_mouse_allowed())
    {
        if (key != KEY_NONE && input->is_key_pressed(key))
        {
            return true;
        }
        if (mouse_button != MOUSE_BUTTON_NONE && input->is_mouse_button_pressed(mouse_button))
        {
            return true;
        }
    }

    if (is_gamepad_allowed())
    {
        if (gamepad_button != JOY_BUTTON_INVALID && input->is_joy_button_pressed(device, gamepad_button))
        {
            return true;
        }
        if (gamepad_axis != JOY_AXIS_INVALID && Math::abs(input->get_joy_axis(device, gamepad_axis)) > dead_zone)
        {
            return true;
        }
    }
    return false;
}

PackedStringArray InputButtonAction::get_binding_properties() const { return PackedStringArray{"key", "mouse_button", "gamepad_button", "gamepad_axis"}; }

void InputButtonAction::draw_debug()
{
    if (!can_debug_draw())
    {
        return;
    }
    draw_debug_label();
    Rect2 rect(debug_draw_position - Vector2(debug_draw_size, debug_draw_size) * 0.5f, Vector2(debug_draw_size, debug_draw_size));
    gizmos_2d_api::draw_2D_rectangle(debug_gizmo(1), rect, was_pressed ? Color(0.3f, 1.0f, 0.4f) : Color(1, 1, 1, 0.5f), was_pressed, 2.0f);
}

void InputButtonAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("is_button_pressed", "device"), &InputButtonAction::is_button_pressed, DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("set_key", "key"), &InputButtonAction::set_key);
    ClassDB::bind_method(D_METHOD("get_key"), &InputButtonAction::get_key);
    ClassDB::bind_method(D_METHOD("set_mouse_button", "button"), &InputButtonAction::set_mouse_button);
    ClassDB::bind_method(D_METHOD("get_mouse_button"), &InputButtonAction::get_mouse_button);
    ClassDB::bind_method(D_METHOD("set_gamepad_button", "button"), &InputButtonAction::set_gamepad_button);
    ClassDB::bind_method(D_METHOD("get_gamepad_button"), &InputButtonAction::get_gamepad_button);
    ClassDB::bind_method(D_METHOD("set_gamepad_axis", "axis"), &InputButtonAction::set_gamepad_axis);
    ClassDB::bind_method(D_METHOD("get_gamepad_axis"), &InputButtonAction::get_gamepad_axis);
    ClassDB::bind_method(D_METHOD("set_held_delay", "delay"), &InputButtonAction::set_held_delay);
    ClassDB::bind_method(D_METHOD("get_held_delay"), &InputButtonAction::get_held_delay);
    ClassDB::bind_method(D_METHOD("set_multi_press_delay", "delay"), &InputButtonAction::set_multi_press_delay);
    ClassDB::bind_method(D_METHOD("get_multi_press_delay"), &InputButtonAction::get_multi_press_delay);
    ClassDB::bind_method(D_METHOD("set_multi_press_count", "count"), &InputButtonAction::set_multi_press_count);
    ClassDB::bind_method(D_METHOD("get_multi_press_count"), &InputButtonAction::get_multi_press_count);

    ADD_GROUP(U"🔗 Affectation", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "key", PROPERTY_HINT_ENUM, InputNames::key_hint()), "set_key", "get_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mouse_button", PROPERTY_HINT_ENUM, InputNames::mouse_button_hint()), "set_mouse_button", "get_mouse_button");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "gamepad_button", PROPERTY_HINT_ENUM, InputNames::joy_button_hint()), "set_gamepad_button", "get_gamepad_button");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "gamepad_axis", PROPERTY_HINT_ENUM, InputNames::joy_axis_hint()), "set_gamepad_axis", "get_gamepad_axis");

    ADD_GROUP(U"⏱️ Temporisation", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "held_delay", PROPERTY_HINT_RANGE, "0,10,0.005,or_greater,suffix:s"), "set_held_delay", "get_held_delay");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "multi_press_delay", PROPERTY_HINT_RANGE, "0,10,0.005,or_greater,suffix:s"), "set_multi_press_delay", "get_multi_press_delay");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "multi_press_count", PROPERTY_HINT_RANGE, "2,10,1"), "set_multi_press_count", "get_multi_press_count");

    ADD_SIGNAL(MethodInfo("pressed"));
    ADD_SIGNAL(MethodInfo("held"));
    ADD_SIGNAL(MethodInfo("multi_pressed"));
    ADD_SIGNAL(MethodInfo("released"));
}
