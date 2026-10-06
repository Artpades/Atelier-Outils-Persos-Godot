#include "input_button_action.hpp"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

bool InputButtonAction::_read() const
{
    if (key_down(key) || mouse_down(mouse_button) || joy_button_down(joy_button))
    {
        return true;
    }
    float axis = joy_axis_value(joy_axis);
    if ((joy_axis_direction != AXIS_NEGATIVE && axis > get_dead_zone()) || (joy_axis_direction != AXIS_POSITIVE && axis < -get_dead_zone()))
    {
        return true;
    }
    if (input_map_action != StringName() && InputMap::get_singleton()->has_action(input_map_action))
    {
        return Input::get_singleton()->is_action_pressed(input_map_action);
    }
    return false;
}

void InputButtonAction::_update(double p_delta) { _process(_read(), p_delta); }

void InputButtonAction::_apply_simulation(const Variant &p_value, double p_delta) { _process(p_value.booleanize(), p_delta); }

void InputButtonAction::_release()
{
    just_pressed = false;
    just_released = false;
    if (pressed)
    {
        _process(false, 0.0);
    }
}

void InputButtonAction::_process(bool p_down, double p_delta)
{
    just_pressed = p_down && !pressed;
    just_released = !p_down && pressed;
    if (just_pressed)
    {
        pressed = true;
        held_time = 0.0;
        held_emitted = false;
        emit_event("pressed");
        emit_event("toggled", Array::make(true));
        if (multi_press_timer > multi_press_delay)
        {
            press_count = 0;
        }
        press_count++;
        multi_press_timer = 0.0;
        if (press_count >= multi_press_count)
        {
            emit_event("multi_pressed", Array::make(press_count));
            press_count = 0;
        }
    }
    else if (just_released)
    {
        pressed = false;
        emit_event("released");
        emit_event("toggled", Array::make(false));
    }
    if (pressed)
    {
        held_time += p_delta;
        if (!held_emitted && held_time >= held_delay)
        {
            held_emitted = true;
            emit_event("held");
        }
    }
    if (press_count > 0)
    {
        multi_press_timer += p_delta;
        if (multi_press_timer > multi_press_delay)
        {
            press_count = 0;
            multi_press_timer = 0.0;
        }
    }
}

// 🔁 Réassignation

PackedStringArray InputButtonAction::get_device_properties() const
{
    PackedStringArray properties;
    properties.push_back("key");
    properties.push_back("mouse_button");
    properties.push_back("joy_button");
    properties.push_back("joy_axis");
    properties.push_back("joy_axis_direction");
    return properties;
}

TypedArray<Dictionary> InputButtonAction::get_rebind_slots() const
{
    TypedArray<Dictionary> slots;
    Dictionary keyboard;
    keyboard["slot"] = "key";
    keyboard["label"] = String(U"Touche / souris");
    keyboard["group"] = "kbm";
    slots.push_back(keyboard);
    Dictionary gamepad;
    gamepad["slot"] = "joy_button";
    gamepad["label"] = String(U"Bouton / gâchette");
    gamepad["group"] = "gamepad";
    slots.push_back(gamepad);
    return slots;
}

bool InputButtonAction::apply_rebind(const StringName &p_slot, const Ref<InputEvent> &p_event, double p_joy_threshold)
{
    if (p_slot == StringName("key"))
    {
        if (int code = event_key(p_event, get_physical_keys()))
        {
            key = code;
            mouse_button = 0;
            emit_changed();
            return true;
        }
        if (int button = event_mouse_button(p_event))
        {
            mouse_button = button;
            key = 0;
            emit_changed();
            return true;
        }
        return false;
    }
    if (p_slot == StringName("joy_button"))
    {
        int button = event_joy_button(p_event);
        if (button >= 0)
        {
            joy_button = button;
            joy_axis = -1;
            emit_changed();
            return true;
        }
        int sign = 1;
        int axis = event_joy_axis(p_event, p_joy_threshold, &sign);
        if (axis >= 0)
        {
            joy_axis = axis;
            joy_axis_direction = sign > 0 ? AXIS_POSITIVE : AXIS_NEGATIVE;
            joy_button = -1;
            emit_changed();
            return true;
        }
    }
    return false;
}

String InputButtonAction::get_slot_text(const StringName &p_slot) const
{
    if (p_slot == StringName("key"))
    {
        return key ? key_text(key) : mouse_button_text(mouse_button);
    }
    if (p_slot == StringName("joy_button"))
    {
        if (joy_button >= 0 || joy_axis < 0)
        {
            return joy_button_text(joy_button);
        }
        String direction = joy_axis_direction == AXIS_POSITIVE ? String(" +") : (joy_axis_direction == AXIS_NEGATIVE ? String(U" −") : String(U" ±"));
        return joy_axis_text(joy_axis) + direction;
    }
    return InputAction::get_slot_text(p_slot);
}

void InputButtonAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_key", "key"), &InputButtonAction::set_key);
    ClassDB::bind_method(D_METHOD("get_key"), &InputButtonAction::get_key);
    ClassDB::bind_method(D_METHOD("set_mouse_button", "button"), &InputButtonAction::set_mouse_button);
    ClassDB::bind_method(D_METHOD("get_mouse_button"), &InputButtonAction::get_mouse_button);
    ClassDB::bind_method(D_METHOD("set_joy_button", "button"), &InputButtonAction::set_joy_button);
    ClassDB::bind_method(D_METHOD("get_joy_button"), &InputButtonAction::get_joy_button);
    ClassDB::bind_method(D_METHOD("set_joy_axis", "axis"), &InputButtonAction::set_joy_axis);
    ClassDB::bind_method(D_METHOD("get_joy_axis"), &InputButtonAction::get_joy_axis);
    ClassDB::bind_method(D_METHOD("set_joy_axis_direction", "direction"), &InputButtonAction::set_joy_axis_direction);
    ClassDB::bind_method(D_METHOD("get_joy_axis_direction"), &InputButtonAction::get_joy_axis_direction);
    ClassDB::bind_method(D_METHOD("set_input_map_action", "action"), &InputButtonAction::set_input_map_action);
    ClassDB::bind_method(D_METHOD("get_input_map_action"), &InputButtonAction::get_input_map_action);
    ClassDB::bind_method(D_METHOD("set_held_delay", "delay"), &InputButtonAction::set_held_delay);
    ClassDB::bind_method(D_METHOD("get_held_delay"), &InputButtonAction::get_held_delay);
    ClassDB::bind_method(D_METHOD("set_multi_press_delay", "delay"), &InputButtonAction::set_multi_press_delay);
    ClassDB::bind_method(D_METHOD("get_multi_press_delay"), &InputButtonAction::get_multi_press_delay);
    ClassDB::bind_method(D_METHOD("set_multi_press_count", "count"), &InputButtonAction::set_multi_press_count);
    ClassDB::bind_method(D_METHOD("get_multi_press_count"), &InputButtonAction::get_multi_press_count);
    ClassDB::bind_method(D_METHOD("is_pressed"), &InputButtonAction::is_pressed);
    ClassDB::bind_method(D_METHOD("is_just_pressed"), &InputButtonAction::is_just_pressed);
    ClassDB::bind_method(D_METHOD("is_just_released"), &InputButtonAction::is_just_released);
    ClassDB::bind_method(D_METHOD("get_held_time"), &InputButtonAction::get_held_time);

    BIND_ENUM_CONSTANT(AXIS_BOTH);
    BIND_ENUM_CONSTANT(AXIS_POSITIVE);
    BIND_ENUM_CONSTANT(AXIS_NEGATIVE);

    ADD_SIGNAL(MethodInfo("pressed"));
    ADD_SIGNAL(MethodInfo("released"));
    ADD_SIGNAL(MethodInfo("toggled", PropertyInfo(Variant::BOOL, "pressed")));
    ADD_SIGNAL(MethodInfo("held"));
    ADD_SIGNAL(MethodInfo("multi_pressed", PropertyInfo(Variant::INT, "count")));

    ADD_GROUP(U"⌨️ Clavier et souris", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "key", PROPERTY_HINT_ENUM, key_hint()), "set_key", "get_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mouse_button", PROPERTY_HINT_ENUM, mouse_button_hint()), "set_mouse_button", "get_mouse_button");
    ADD_GROUP(U"🎮 Manette", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_button", PROPERTY_HINT_ENUM, joy_button_hint()), "set_joy_button", "get_joy_button");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_axis", PROPERTY_HINT_ENUM, joy_axis_hint()), "set_joy_axis", "get_joy_axis");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_axis_direction", PROPERTY_HINT_ENUM, U"Les deux,Positif,Négatif"), "set_joy_axis_direction", "get_joy_axis_direction");
    ADD_GROUP(U"🗺️ InputMap", "");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "input_map_action"), "set_input_map_action", "get_input_map_action");
    ADD_GROUP(U"⏱️ Appuis", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "held_delay", PROPERTY_HINT_RANGE, "0,10,0.005,or_greater,suffix:s"), "set_held_delay", "get_held_delay");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "multi_press_delay", PROPERTY_HINT_RANGE, "0,10,0.005,or_greater,suffix:s"), "set_multi_press_delay", "get_multi_press_delay");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "multi_press_count", PROPERTY_HINT_RANGE, "2,10,1"), "set_multi_press_count", "get_multi_press_count");
}
