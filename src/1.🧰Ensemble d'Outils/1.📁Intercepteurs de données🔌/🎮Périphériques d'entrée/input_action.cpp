#include "input_action.hpp"

#include "input_names.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos_2d.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal/custom_logger.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void InputAction::_exit_tree() { free_debug_gizmos(); }

void InputAction::set_debug_draw(bool p_enabled)
{
    debug_draw = p_enabled;
    if (!debug_draw)
    {
        free_debug_gizmos();
    }
}

void InputAction::log_debug(const String &p_message) const
{
    if (debug_log)
    {
        CustomLogger::log(String(get_name()) + " (" + String(action_name) + ")", InputNames::ICON_ACTION, p_message);
    }
}

bool InputAction::can_debug_draw()
{
    if (!debug_draw || !is_inside_tree() || Engine::get_singleton()->is_editor_hint())
    {
        free_debug_gizmos();
        return false;
    }
    return true;
}

RID InputAction::debug_gizmo(int p_index)
{
    while (debug_gizmos.size() <= p_index)
    {
        debug_gizmos.push_back(Gizmos2D::create_2D_gizmos(get_viewport()));
    }
    return debug_gizmos[p_index];
}

void InputAction::draw_debug_label()
{
    Gizmos2D::draw_2D_text(debug_gizmo(0), debug_draw_position + Vector2(-debug_draw_size, -debug_draw_size - 8.0f), String(action_name), Color(1, 1, 1, 0.9f), 12);
}

void InputAction::free_debug_gizmos()
{
    for (const RID &rid : debug_gizmos)
    {
        if (rid.is_valid())
        {
            Gizmos2D::free_gizmos(rid);
        }
    }
    debug_gizmos.clear();
}

void InputAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_detector_type", "type"), &InputAction::set_detector_type);
    ClassDB::bind_method(D_METHOD("get_detector_type"), &InputAction::get_detector_type);
    ClassDB::bind_method(D_METHOD("set_action_name", "name"), &InputAction::set_action_name);
    ClassDB::bind_method(D_METHOD("get_action_name"), &InputAction::get_action_name);
    ClassDB::bind_method(D_METHOD("set_action_enabled", "enabled"), &InputAction::set_action_enabled);
    ClassDB::bind_method(D_METHOD("is_action_enabled"), &InputAction::is_action_enabled);
    ClassDB::bind_method(D_METHOD("set_dead_zone", "dead_zone"), &InputAction::set_dead_zone);
    ClassDB::bind_method(D_METHOD("get_dead_zone"), &InputAction::get_dead_zone);
    ClassDB::bind_method(D_METHOD("set_dirty_threshold", "threshold"), &InputAction::set_dirty_threshold);
    ClassDB::bind_method(D_METHOD("get_dirty_threshold"), &InputAction::get_dirty_threshold);
    ClassDB::bind_method(D_METHOD("set_device_id", "id"), &InputAction::set_device_id);
    ClassDB::bind_method(D_METHOD("get_device_id"), &InputAction::get_device_id);
    ClassDB::bind_method(D_METHOD("set_debug_log", "enabled"), &InputAction::set_debug_log);
    ClassDB::bind_method(D_METHOD("get_debug_log"), &InputAction::get_debug_log);
    ClassDB::bind_method(D_METHOD("set_debug_draw", "enabled"), &InputAction::set_debug_draw);
    ClassDB::bind_method(D_METHOD("get_debug_draw"), &InputAction::get_debug_draw);
    ClassDB::bind_method(D_METHOD("set_debug_draw_position", "position"), &InputAction::set_debug_draw_position);
    ClassDB::bind_method(D_METHOD("get_debug_draw_position"), &InputAction::get_debug_draw_position);
    ClassDB::bind_method(D_METHOD("set_debug_draw_size", "size"), &InputAction::set_debug_draw_size);
    ClassDB::bind_method(D_METHOD("get_debug_draw_size"), &InputAction::get_debug_draw_size);
    ClassDB::bind_method(D_METHOD("is_keyboard_mouse_allowed"), &InputAction::is_keyboard_mouse_allowed);
    ClassDB::bind_method(D_METHOD("is_gamepad_allowed"), &InputAction::is_gamepad_allowed);
    ClassDB::bind_method(D_METHOD("get_binding_properties"), &InputAction::get_binding_properties);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "detector_type", PROPERTY_HINT_ENUM, "All Devices:1,Keyboard Mouse:2,Gamepad:3"), "set_detector_type", "get_detector_type");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_name"), "set_action_name", "get_action_name");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "action_enabled"), "set_action_enabled", "is_action_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dead_zone", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_dead_zone", "get_dead_zone");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dirty_threshold", PROPERTY_HINT_RANGE, "0,1,0.001,or_greater"), "set_dirty_threshold", "get_dirty_threshold");
    // Fourni par PlayerInput au lancement : visible dans l'inspecteur mais pas enregistré avec la scène.
    ADD_PROPERTY(PropertyInfo(Variant::INT, "device_id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_device_id", "get_device_id");

    ADD_GROUP(U"🪳 Debug", "debug_");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_log"), "set_debug_log", "get_debug_log");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_draw"), "set_debug_draw", "get_debug_draw");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "debug_draw_position"), "set_debug_draw_position", "get_debug_draw_position");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "debug_draw_size", PROPERTY_HINT_RANGE, "8,256,1,suffix:px"), "set_debug_draw_size", "get_debug_draw_size");

    BIND_ENUM_CONSTANT(DETECTOR_ALL_DEVICES);
    BIND_ENUM_CONSTANT(DETECTOR_KEYBOARD_MOUSE);
    BIND_ENUM_CONSTANT(DETECTOR_GAMEPAD);
}
