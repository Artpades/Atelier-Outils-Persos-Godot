#include "player_input.hpp"

#include "input_names.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal/custom_logger.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void PlayerInput::_ready()
{
    // Différé : les actions descendantes ne sont pas toutes prêtes quand le parent reçoit _ready.
    callable_mp(this, &PlayerInput::propagate_to_actions).call_deferred();
}

void PlayerInput::_input(const Ref<InputEvent> &p_event)
{
    if (!switch_detector_on_device_input || Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    if (Object::cast_to<InputEventKey>(*p_event) || Object::cast_to<InputEventMouse>(*p_event))
    {
        set_detector_type(InputAction::DETECTOR_KEYBOARD_MOUSE);
    }
    else if (Object::cast_to<InputEventJoypadButton>(*p_event))
    {
        set_detector_type(InputAction::DETECTOR_GAMEPAD);
    }
    else if (InputEventJoypadMotion *motion = Object::cast_to<InputEventJoypadMotion>(*p_event))
    {
        if (Math::abs(motion->get_axis_value()) > joy_input_threshold)
        {
            set_detector_type(InputAction::DETECTOR_GAMEPAD);
        }
    }
}

void PlayerInput::set_device_number(int p_device)
{
    if (device_number == p_device)
    {
        return;
    }
    device_number = p_device;
    propagate_to_actions();
}

void PlayerInput::set_detector_type(InputAction::DeviceDetectorType p_type)
{
    if (detector_type == p_type)
    {
        return;
    }
    detector_type = p_type;
    propagate_to_actions();

    if (debug_log)
    {
        const char *names[] = {"", "tous les périphériques", "clavier & souris", "manette"};
        CustomLogger::log(get_name(), InputNames::ICON_PLAYER, String::utf8("Détection : ") + String::utf8(names[CLAMP((int)p_type, 0, 3)]));
    }
}

void PlayerInput::propagate_to_actions()
{
    TypedArray<InputAction> actions = get_all_actions();
    for (int i = 0; i < actions.size(); i++)
    {
        InputAction *action = Object::cast_to<InputAction>(actions[i]);
        action->set_device_id(device_number);
        action->set_detector_type(detector_type);
    }
}

TypedArray<InputAction> PlayerInput::get_all_actions() const
{
    TypedArray<InputAction> actions;
    collect_actions(this, actions);
    return actions;
}

void PlayerInput::collect_actions(const Node *p_node, TypedArray<InputAction> &r_actions)
{
    for (int i = 0; i < p_node->get_child_count(); i++)
    {
        Node *child = p_node->get_child(i);
        if (InputAction *action = Object::cast_to<InputAction>(child))
        {
            r_actions.push_back(action);
        }
        collect_actions(child, r_actions);
    }
}

String PlayerInput::get_controller_name() const { return Input::get_singleton()->get_joy_name(device_number); }

String PlayerInput::get_joy_guid() const { return Input::get_singleton()->get_joy_guid(device_number); }

Dictionary PlayerInput::get_joy_info() const { return Input::get_singleton()->get_joy_info(device_number); }

void PlayerInput::start_vibration(float p_weak_magnitude, float p_strong_magnitude, float p_duration)
{
    Input::get_singleton()->start_joy_vibration(device_number, p_weak_magnitude, p_strong_magnitude, p_duration);
}

void PlayerInput::stop_vibration() { Input::get_singleton()->stop_joy_vibration(device_number); }

void PlayerInput::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_all_actions"), &PlayerInput::get_all_actions);
    ClassDB::bind_method(D_METHOD("get_controller_name"), &PlayerInput::get_controller_name);
    ClassDB::bind_method(D_METHOD("get_joy_guid"), &PlayerInput::get_joy_guid);
    ClassDB::bind_method(D_METHOD("get_joy_info"), &PlayerInput::get_joy_info);
    ClassDB::bind_method(D_METHOD("start_vibration", "weak_magnitude", "strong_magnitude", "duration"), &PlayerInput::start_vibration, DEFVAL(1.0f), DEFVAL(1.0f), DEFVAL(0.0f));
    ClassDB::bind_method(D_METHOD("stop_vibration"), &PlayerInput::stop_vibration);
    ClassDB::bind_method(D_METHOD("set_device_number", "device"), &PlayerInput::set_device_number);
    ClassDB::bind_method(D_METHOD("get_device_number"), &PlayerInput::get_device_number);
    ClassDB::bind_method(D_METHOD("set_detector_type", "type"), &PlayerInput::set_detector_type);
    ClassDB::bind_method(D_METHOD("get_detector_type"), &PlayerInput::get_detector_type);
    ClassDB::bind_method(D_METHOD("set_switch_detector_on_device_input", "enabled"), &PlayerInput::set_switch_detector_on_device_input);
    ClassDB::bind_method(D_METHOD("get_switch_detector_on_device_input"), &PlayerInput::get_switch_detector_on_device_input);
    ClassDB::bind_method(D_METHOD("set_joy_input_threshold", "threshold"), &PlayerInput::set_joy_input_threshold);
    ClassDB::bind_method(D_METHOD("get_joy_input_threshold"), &PlayerInput::get_joy_input_threshold);
    ClassDB::bind_method(D_METHOD("set_debug_log", "enabled"), &PlayerInput::set_debug_log);
    ClassDB::bind_method(D_METHOD("get_debug_log"), &PlayerInput::get_debug_log);

    // Lecture seule, non enregistrée : affiche dans l'inspecteur le nom de la manette branchée.
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "controller_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_controller_name");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "device_number", PROPERTY_HINT_RANGE, "0,16,1,or_greater"), "set_device_number", "get_device_number");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "detector_type", PROPERTY_HINT_ENUM, "All Devices:1,Keyboard Mouse:2,Gamepad:3"), "set_detector_type", "get_detector_type");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "switch_detector_on_device_input"), "set_switch_detector_on_device_input", "get_switch_detector_on_device_input");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "joy_input_threshold", PROPERTY_HINT_RANGE, "0.0,1.0,0.05"), "set_joy_input_threshold", "get_joy_input_threshold");

    ADD_GROUP(U"🪳 Debug", "debug_");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_log"), "set_debug_log", "get_debug_log");
}
