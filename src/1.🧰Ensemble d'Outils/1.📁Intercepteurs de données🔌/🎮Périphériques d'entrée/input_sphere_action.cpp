#include "input_sphere_action.hpp"

#include "input_names.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos_2d.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void InputSphereAction::_process(double p_delta)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    Vector3 current = get_vector();
    float threshold_sq = dirty_threshold * dirty_threshold;

    if (current.distance_squared_to(last_emitted) >= threshold_sq)
    {
        last_emitted = current;
        emit_signal("sphere_emitted", current);
        log_debug("sphere_emitted : " + String(current));
    }

    if (p_delta > 0.0)
    {
        Vector3 velocity = (current - previous_value) / (float)p_delta;
        if (velocity.distance_squared_to(last_emitted_velocity) >= threshold_sq)
        {
            last_emitted_velocity = velocity;
            emit_signal("velocity_changed", velocity);
        }
    }
    previous_value = current;

    if (current.length_squared() > 0.0f)
    {
        // Convention Godot : -Z = avant. Yaw autour de Y, pitch = élévation.
        float yaw = Math::atan2(current.x, -current.z);
        float pitch = Math::atan2(current.y, Vector2(current.x, current.z).length());
        Vector3 orientation(pitch, yaw, 0.0f);

        if (orientation.distance_squared_to(last_emitted_orientation) >= threshold_sq)
        {
            last_emitted_orientation = orientation;
            emit_signal("orientation_changed", orientation);
        }
    }

    draw_debug(current);
}

Vector3 InputSphereAction::get_vector(int p_device) const
{
    if (!action_enabled)
    {
        return Vector3();
    }

    Input *input = Input::get_singleton();
    int device = resolve_device(p_device);

    if (is_gamepad_allowed() && joy_axis_x != JOY_AXIS_INVALID && joy_axis_y != JOY_AXIS_INVALID && joy_axis_z != JOY_AXIS_INVALID)
    {
        Vector3 joy(input->get_joy_axis(device, joy_axis_x), input->get_joy_axis(device, joy_axis_y), input->get_joy_axis(device, joy_axis_z));
        if (joy.length() > dead_zone)
        {
            return joy.limit_length(1.0f);
        }
    }

    auto key_down = [input](Key p_key) { return p_key != KEY_NONE && input->is_key_pressed(p_key) ? 1.0f : 0.0f; };
    auto button_down = [input, device](JoyButton p_button) { return p_button != JOY_BUTTON_INVALID && input->is_joy_button_pressed(device, p_button) ? 1.0f : 0.0f; };

    Vector3 buttons;
    if (is_keyboard_mouse_allowed())
    {
        buttons += Vector3(key_down(right_key) - key_down(left_key), key_down(up_key) - key_down(down_key), key_down(forward_key) - key_down(backward_key));
    }
    if (is_gamepad_allowed())
    {
        buttons += Vector3(button_down(dpad_right) - button_down(dpad_left), button_down(dpad_up) - button_down(dpad_down), button_down(dpad_forward) - button_down(dpad_backward));
    }
    return buttons.limit_length(1.0f);
}

PackedStringArray InputSphereAction::get_binding_properties() const
{
    return PackedStringArray{"left_key",  "right_key", "up_key",     "down_key",   "forward_key", "backward_key", "joy_axis_x",   "joy_axis_y",
                             "joy_axis_z", "dpad_left", "dpad_right", "dpad_up",    "dpad_down",   "dpad_forward", "dpad_backward"};
}

void InputSphereAction::draw_debug(const Vector3 &p_value)
{
    if (!can_debug_draw())
    {
        return;
    }
    draw_debug_label();
    // Plan XY vu de face (Y écran inversé) + jauge verticale pour Z, à droite du cercle.
    float radius = debug_draw_size * 0.5f;
    Gizmos2D::draw_2D_circle(debug_gizmo(1), debug_draw_position, radius, Color(1, 1, 1, 0.5f), false, 2.0f);
    Gizmos2D::draw_2D_arrow(debug_gizmo(2), debug_draw_position, debug_draw_position + Vector2(p_value.x, -p_value.y) * radius, 8.0f, 2.0f, Color(0.3f, 0.8f, 1.0f));

    Vector2 z_origin = debug_draw_position + Vector2(radius + 12.0f, 0.0f);
    Gizmos2D::draw_2D_rectangle(debug_gizmo(3), Rect2(z_origin - Vector2(4.0f, radius), Vector2(8.0f, radius * 2.0f)), Color(1, 1, 1, 0.5f), false, 1.0f);
    Gizmos2D::draw_2D_rectangle(debug_gizmo(4), Rect2(z_origin.x - 4.0f, z_origin.y, 8.0f, -p_value.z * radius).abs(), Color(1.0f, 0.7f, 0.2f), true);
}

void InputSphereAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_vector", "device"), &InputSphereAction::get_vector, DEFVAL(-1));

    ClassDB::bind_method(D_METHOD("set_left_key", "key"), &InputSphereAction::set_left_key);
    ClassDB::bind_method(D_METHOD("get_left_key"), &InputSphereAction::get_left_key);
    ClassDB::bind_method(D_METHOD("set_right_key", "key"), &InputSphereAction::set_right_key);
    ClassDB::bind_method(D_METHOD("get_right_key"), &InputSphereAction::get_right_key);
    ClassDB::bind_method(D_METHOD("set_up_key", "key"), &InputSphereAction::set_up_key);
    ClassDB::bind_method(D_METHOD("get_up_key"), &InputSphereAction::get_up_key);
    ClassDB::bind_method(D_METHOD("set_down_key", "key"), &InputSphereAction::set_down_key);
    ClassDB::bind_method(D_METHOD("get_down_key"), &InputSphereAction::get_down_key);
    ClassDB::bind_method(D_METHOD("set_forward_key", "key"), &InputSphereAction::set_forward_key);
    ClassDB::bind_method(D_METHOD("get_forward_key"), &InputSphereAction::get_forward_key);
    ClassDB::bind_method(D_METHOD("set_backward_key", "key"), &InputSphereAction::set_backward_key);
    ClassDB::bind_method(D_METHOD("get_backward_key"), &InputSphereAction::get_backward_key);

    ClassDB::bind_method(D_METHOD("set_joy_axis_x", "axis"), &InputSphereAction::set_joy_axis_x);
    ClassDB::bind_method(D_METHOD("get_joy_axis_x"), &InputSphereAction::get_joy_axis_x);
    ClassDB::bind_method(D_METHOD("set_joy_axis_y", "axis"), &InputSphereAction::set_joy_axis_y);
    ClassDB::bind_method(D_METHOD("get_joy_axis_y"), &InputSphereAction::get_joy_axis_y);
    ClassDB::bind_method(D_METHOD("set_joy_axis_z", "axis"), &InputSphereAction::set_joy_axis_z);
    ClassDB::bind_method(D_METHOD("get_joy_axis_z"), &InputSphereAction::get_joy_axis_z);

    ClassDB::bind_method(D_METHOD("set_dpad_left", "button"), &InputSphereAction::set_dpad_left);
    ClassDB::bind_method(D_METHOD("get_dpad_left"), &InputSphereAction::get_dpad_left);
    ClassDB::bind_method(D_METHOD("set_dpad_right", "button"), &InputSphereAction::set_dpad_right);
    ClassDB::bind_method(D_METHOD("get_dpad_right"), &InputSphereAction::get_dpad_right);
    ClassDB::bind_method(D_METHOD("set_dpad_up", "button"), &InputSphereAction::set_dpad_up);
    ClassDB::bind_method(D_METHOD("get_dpad_up"), &InputSphereAction::get_dpad_up);
    ClassDB::bind_method(D_METHOD("set_dpad_down", "button"), &InputSphereAction::set_dpad_down);
    ClassDB::bind_method(D_METHOD("get_dpad_down"), &InputSphereAction::get_dpad_down);
    ClassDB::bind_method(D_METHOD("set_dpad_forward", "button"), &InputSphereAction::set_dpad_forward);
    ClassDB::bind_method(D_METHOD("get_dpad_forward"), &InputSphereAction::get_dpad_forward);
    ClassDB::bind_method(D_METHOD("set_dpad_backward", "button"), &InputSphereAction::set_dpad_backward);
    ClassDB::bind_method(D_METHOD("get_dpad_backward"), &InputSphereAction::get_dpad_backward);

    String key_hint = InputNames::key_hint();
    ADD_GROUP(U"⌨️ Key Axis", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "left_key", PROPERTY_HINT_ENUM, key_hint), "set_left_key", "get_left_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "right_key", PROPERTY_HINT_ENUM, key_hint), "set_right_key", "get_right_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "up_key", PROPERTY_HINT_ENUM, key_hint), "set_up_key", "get_up_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "down_key", PROPERTY_HINT_ENUM, key_hint), "set_down_key", "get_down_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "forward_key", PROPERTY_HINT_ENUM, key_hint), "set_forward_key", "get_forward_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "backward_key", PROPERTY_HINT_ENUM, key_hint), "set_backward_key", "get_backward_key");

    String axis_hint = InputNames::joy_axis_hint();
    ADD_GROUP(U"🔘 Joy Axis", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_axis_x", PROPERTY_HINT_ENUM, axis_hint), "set_joy_axis_x", "get_joy_axis_x");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_axis_y", PROPERTY_HINT_ENUM, axis_hint), "set_joy_axis_y", "get_joy_axis_y");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_axis_z", PROPERTY_HINT_ENUM, axis_hint), "set_joy_axis_z", "get_joy_axis_z");

    String button_hint = InputNames::joy_button_hint();
    ADD_GROUP(U"✖️ DPAD Axis", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_left", PROPERTY_HINT_ENUM, button_hint), "set_dpad_left", "get_dpad_left");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_right", PROPERTY_HINT_ENUM, button_hint), "set_dpad_right", "get_dpad_right");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_up", PROPERTY_HINT_ENUM, button_hint), "set_dpad_up", "get_dpad_up");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_down", PROPERTY_HINT_ENUM, button_hint), "set_dpad_down", "get_dpad_down");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_forward", PROPERTY_HINT_ENUM, button_hint), "set_dpad_forward", "get_dpad_forward");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_backward", PROPERTY_HINT_ENUM, button_hint), "set_dpad_backward", "get_dpad_backward");

    ADD_SIGNAL(MethodInfo("sphere_emitted", PropertyInfo(Variant::VECTOR3, "value")));
    ADD_SIGNAL(MethodInfo("velocity_changed", PropertyInfo(Variant::VECTOR3, "velocity")));
    ADD_SIGNAL(MethodInfo("orientation_changed", PropertyInfo(Variant::VECTOR3, "orientation")));
}
