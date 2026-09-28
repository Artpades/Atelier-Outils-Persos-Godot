#include "input_circle_action.hpp"

#include "input_names.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos_api.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void InputCircleAction::set_key_axis_preset(KeyAxisPreset p_preset)
{
    key_axis_preset = p_preset;
    switch (p_preset)
    {
    case KEY_PRESET_WASD:
        up_key = KEY_W, down_key = KEY_S, left_key = KEY_A, right_key = KEY_D;
        break;
    case KEY_PRESET_ZQSD:
        up_key = KEY_Z, down_key = KEY_S, left_key = KEY_Q, right_key = KEY_D;
        break;
    case KEY_PRESET_ARROW_KEYS:
        up_key = KEY_UP, down_key = KEY_DOWN, left_key = KEY_LEFT, right_key = KEY_RIGHT;
        break;
    case KEY_PRESET_CUSTOM:
        break;
    }
    notify_property_list_changed();
}

void InputCircleAction::set_joy_axis_preset(JoyAxisPreset p_preset)
{
    joy_axis_preset = p_preset;
    switch (p_preset)
    {
    case JOY_PRESET_LEFT_STICK:
        joy_axis_x = JOY_AXIS_LEFT_X, joy_axis_y = JOY_AXIS_LEFT_Y;
        break;
    case JOY_PRESET_RIGHT_STICK:
        joy_axis_x = JOY_AXIS_RIGHT_X, joy_axis_y = JOY_AXIS_RIGHT_Y;
        break;
    case JOY_PRESET_CUSTOM:
        break;
    }
    notify_property_list_changed();
}

void InputCircleAction::set_dpad_preset(DpadPreset p_preset)
{
    dpad_preset = p_preset;
    switch (p_preset)
    {
    case DPAD_PRESET_TRADITIONAL:
        dpad_up = JOY_BUTTON_DPAD_UP, dpad_down = JOY_BUTTON_DPAD_DOWN, dpad_left = JOY_BUTTON_DPAD_LEFT, dpad_right = JOY_BUTTON_DPAD_RIGHT;
        break;
    case DPAD_PRESET_ABXY:
        dpad_up = JOY_BUTTON_Y, dpad_down = JOY_BUTTON_A, dpad_left = JOY_BUTTON_X, dpad_right = JOY_BUTTON_B;
        break;
    case DPAD_PRESET_CUSTOM:
        break;
    }
    notify_property_list_changed();
}

void InputCircleAction::_process(double p_delta)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    // Pas de retour anticipé si l'action est désactivée : get_vector() renvoie alors zéro, ce qui émet
    // un dernier circle_emitted(Vector2.ZERO) et relâche proprement ce qui écoute.
    Vector2 current = get_vector();
    float threshold_sq = dirty_threshold * dirty_threshold;

    if (current.distance_squared_to(last_emitted) >= threshold_sq)
    {
        last_emitted = current;
        emit_signal("circle_emitted", current);
        log_debug("circle_emitted : " + String(current));
    }

    if (p_delta > 0.0)
    {
        Vector2 velocity = (current - previous_value) / (float)p_delta;
        if (velocity.distance_squared_to(last_emitted_velocity) >= threshold_sq)
        {
            last_emitted_velocity = velocity;
            emit_signal("velocity_changed", velocity);
        }
    }
    previous_value = current;

    if (current.length_squared() > 0.0f)
    {
        float orientation = Math::atan2(current.y, current.x);
        if (Math::abs((float)UtilityFunctions::angle_difference(last_emitted_orientation, orientation)) >= dirty_threshold)
        {
            last_emitted_orientation = orientation;
            emit_signal("orientation_changed", orientation);
        }
    }

    draw_debug(current);
}

Vector2 InputCircleAction::get_vector(int p_device) const
{
    if (!action_enabled)
    {
        return Vector2();
    }

    Input *input = Input::get_singleton();
    int device = resolve_device(p_device);

    if (is_gamepad_allowed() && joy_axis_x != JOY_AXIS_INVALID && joy_axis_y != JOY_AXIS_INVALID)
    {
        Vector2 joy(input->get_joy_axis(device, joy_axis_x), input->get_joy_axis(device, joy_axis_y));
        if (joy.length() > dead_zone)
        {
            return joy.limit_length(1.0f);
        }
    }

    Vector2 buttons;
    if (is_keyboard_mouse_allowed())
    {
        buttons.x += (right_key != KEY_NONE && input->is_key_pressed(right_key)) ? 1.0f : 0.0f;
        buttons.x -= (left_key != KEY_NONE && input->is_key_pressed(left_key)) ? 1.0f : 0.0f;
        buttons.y += (down_key != KEY_NONE && input->is_key_pressed(down_key)) ? 1.0f : 0.0f;
        buttons.y -= (up_key != KEY_NONE && input->is_key_pressed(up_key)) ? 1.0f : 0.0f;
    }
    if (is_gamepad_allowed())
    {
        buttons.x += (dpad_right != JOY_BUTTON_INVALID && input->is_joy_button_pressed(device, dpad_right)) ? 1.0f : 0.0f;
        buttons.x -= (dpad_left != JOY_BUTTON_INVALID && input->is_joy_button_pressed(device, dpad_left)) ? 1.0f : 0.0f;
        buttons.y += (dpad_down != JOY_BUTTON_INVALID && input->is_joy_button_pressed(device, dpad_down)) ? 1.0f : 0.0f;
        buttons.y -= (dpad_up != JOY_BUTTON_INVALID && input->is_joy_button_pressed(device, dpad_up)) ? 1.0f : 0.0f;
    }
    return buttons.limit_length(1.0f);
}

PackedStringArray InputCircleAction::get_binding_properties() const
{
    return PackedStringArray{"key_axis_preset", "joy_axis_preset", "dpad_preset", "up_key", "down_key", "left_key", "right_key",
                             "joy_axis_x", "joy_axis_y", "dpad_up", "dpad_down", "dpad_left", "dpad_right"};
}

void InputCircleAction::draw_debug(const Vector2 &p_value)
{
    if (!can_debug_draw())
    {
        return;
    }
    draw_debug_label();
    gizmos_2d_api::draw_2D_circle(debug_gizmo(1), debug_draw_position, debug_draw_size * 0.5f, Color(1, 1, 1, 0.5f), false, 2.0f);
    // Cercle de zone morte : le stick est ignoré tant qu'il reste à l'intérieur.
    gizmos_2d_api::draw_2D_circle(debug_gizmo(2), debug_draw_position, debug_draw_size * 0.5f * dead_zone, Color(1, 0.4f, 0.3f, 0.4f), false, 1.0f);
    gizmos_2d_api::draw_2D_arrow(debug_gizmo(3), debug_draw_position, debug_draw_position + p_value * debug_draw_size * 0.5f, 8.0f, 2.0f, Color(0.3f, 0.8f, 1.0f));
}

void InputCircleAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_vector", "device"), &InputCircleAction::get_vector, DEFVAL(-1));

    ClassDB::bind_method(D_METHOD("set_key_axis_preset", "preset"), &InputCircleAction::set_key_axis_preset);
    ClassDB::bind_method(D_METHOD("get_key_axis_preset"), &InputCircleAction::get_key_axis_preset);
    ClassDB::bind_method(D_METHOD("set_joy_axis_preset", "preset"), &InputCircleAction::set_joy_axis_preset);
    ClassDB::bind_method(D_METHOD("get_joy_axis_preset"), &InputCircleAction::get_joy_axis_preset);
    ClassDB::bind_method(D_METHOD("set_dpad_preset", "preset"), &InputCircleAction::set_dpad_preset);
    ClassDB::bind_method(D_METHOD("get_dpad_preset"), &InputCircleAction::get_dpad_preset);

    ClassDB::bind_method(D_METHOD("set_up_key", "key"), &InputCircleAction::set_up_key);
    ClassDB::bind_method(D_METHOD("get_up_key"), &InputCircleAction::get_up_key);
    ClassDB::bind_method(D_METHOD("set_down_key", "key"), &InputCircleAction::set_down_key);
    ClassDB::bind_method(D_METHOD("get_down_key"), &InputCircleAction::get_down_key);
    ClassDB::bind_method(D_METHOD("set_left_key", "key"), &InputCircleAction::set_left_key);
    ClassDB::bind_method(D_METHOD("get_left_key"), &InputCircleAction::get_left_key);
    ClassDB::bind_method(D_METHOD("set_right_key", "key"), &InputCircleAction::set_right_key);
    ClassDB::bind_method(D_METHOD("get_right_key"), &InputCircleAction::get_right_key);

    ClassDB::bind_method(D_METHOD("set_joy_axis_x", "axis"), &InputCircleAction::set_joy_axis_x);
    ClassDB::bind_method(D_METHOD("get_joy_axis_x"), &InputCircleAction::get_joy_axis_x);
    ClassDB::bind_method(D_METHOD("set_joy_axis_y", "axis"), &InputCircleAction::set_joy_axis_y);
    ClassDB::bind_method(D_METHOD("get_joy_axis_y"), &InputCircleAction::get_joy_axis_y);

    ClassDB::bind_method(D_METHOD("set_dpad_up", "button"), &InputCircleAction::set_dpad_up);
    ClassDB::bind_method(D_METHOD("get_dpad_up"), &InputCircleAction::get_dpad_up);
    ClassDB::bind_method(D_METHOD("set_dpad_down", "button"), &InputCircleAction::set_dpad_down);
    ClassDB::bind_method(D_METHOD("get_dpad_down"), &InputCircleAction::get_dpad_down);
    ClassDB::bind_method(D_METHOD("set_dpad_left", "button"), &InputCircleAction::set_dpad_left);
    ClassDB::bind_method(D_METHOD("get_dpad_left"), &InputCircleAction::get_dpad_left);
    ClassDB::bind_method(D_METHOD("set_dpad_right", "button"), &InputCircleAction::set_dpad_right);
    ClassDB::bind_method(D_METHOD("get_dpad_right"), &InputCircleAction::get_dpad_right);

    // Chaque préréglage est déclaré avant les valeurs qu'il écrit : au chargement de la scène, les propriétés
    // sont assignées dans cet ordre, les valeurs enregistrées ont donc le dernier mot.
    String key_hint = InputNames::key_hint();
    ADD_GROUP(U"⌨️ Key Axis", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "key_axis_preset", PROPERTY_HINT_ENUM, "WASD,ZQSD,Arrow Keys,Custom"), "set_key_axis_preset", "get_key_axis_preset");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "up_key", PROPERTY_HINT_ENUM, key_hint), "set_up_key", "get_up_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "down_key", PROPERTY_HINT_ENUM, key_hint), "set_down_key", "get_down_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "left_key", PROPERTY_HINT_ENUM, key_hint), "set_left_key", "get_left_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "right_key", PROPERTY_HINT_ENUM, key_hint), "set_right_key", "get_right_key");

    String axis_hint = InputNames::joy_axis_hint();
    ADD_GROUP(U"🔘 Joy Axis", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_axis_preset", PROPERTY_HINT_ENUM, "Left Stick,Right Stick,Custom"), "set_joy_axis_preset", "get_joy_axis_preset");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_axis_x", PROPERTY_HINT_ENUM, axis_hint), "set_joy_axis_x", "get_joy_axis_x");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_axis_y", PROPERTY_HINT_ENUM, axis_hint), "set_joy_axis_y", "get_joy_axis_y");

    String button_hint = InputNames::joy_button_hint();
    ADD_GROUP(U"✖️ DPAD Axis", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_preset", PROPERTY_HINT_ENUM, "Traditional DPAD,ABXY,Custom"), "set_dpad_preset", "get_dpad_preset");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_up", PROPERTY_HINT_ENUM, button_hint), "set_dpad_up", "get_dpad_up");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_down", PROPERTY_HINT_ENUM, button_hint), "set_dpad_down", "get_dpad_down");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_left", PROPERTY_HINT_ENUM, button_hint), "set_dpad_left", "get_dpad_left");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_right", PROPERTY_HINT_ENUM, button_hint), "set_dpad_right", "get_dpad_right");

    ADD_SIGNAL(MethodInfo("circle_emitted", PropertyInfo(Variant::VECTOR2, "value")));
    ADD_SIGNAL(MethodInfo("velocity_changed", PropertyInfo(Variant::VECTOR2, "velocity")));
    ADD_SIGNAL(MethodInfo("orientation_changed", PropertyInfo(Variant::FLOAT, "orientation")));

    BIND_ENUM_CONSTANT(KEY_PRESET_WASD);
    BIND_ENUM_CONSTANT(KEY_PRESET_ZQSD);
    BIND_ENUM_CONSTANT(KEY_PRESET_ARROW_KEYS);
    BIND_ENUM_CONSTANT(KEY_PRESET_CUSTOM);
    BIND_ENUM_CONSTANT(JOY_PRESET_LEFT_STICK);
    BIND_ENUM_CONSTANT(JOY_PRESET_RIGHT_STICK);
    BIND_ENUM_CONSTANT(JOY_PRESET_CUSTOM);
    BIND_ENUM_CONSTANT(DPAD_PRESET_TRADITIONAL);
    BIND_ENUM_CONSTANT(DPAD_PRESET_ABXY);
    BIND_ENUM_CONSTANT(DPAD_PRESET_CUSTOM);
}
