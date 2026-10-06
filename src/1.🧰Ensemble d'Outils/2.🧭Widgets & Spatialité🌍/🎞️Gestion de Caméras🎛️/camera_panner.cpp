#include "camera_panner.hpp"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_magnify_gesture.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_pan_gesture.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void CameraPanner::_ready()
{
    target_position = get_position();
    target_zoom = get_zoom().x;
}

void CameraPanner::_process(double p_delta)
{
    // --- GESTION DE LA MANETTE ---
    _handle_joypad_input((float)p_delta);

    // --- DÉPLACEMENT FLUIDE --- (poids borné à 1 : pas de dépassement si la frame est longue)
    const float weight = MIN(lerp_speed * (float)p_delta, 1.0f);
    set_position(get_position().lerp(target_position, weight));
    const float smooth_zoom = Math::lerp(get_zoom().x, target_zoom, weight);
    set_zoom(Vector2(smooth_zoom, smooth_zoom));
}

void CameraPanner::_input(const Ref<InputEvent> &p_event)
{
    // 1. Panning à la souris (bouton maintenu)
    Ref<InputEventMouseMotion> motion = p_event;
    if (motion.is_valid() && Input::get_singleton()->is_mouse_button_pressed(pan_mouse_button))
    {
        target_position -= motion->get_relative() / target_zoom;
        return;
    }

    // 2. Tactile / Trackpad : glissement à deux doigts
    Ref<InputEventPanGesture> pan_gesture = p_event;
    if (pan_gesture.is_valid())
    {
        target_position += pan_gesture->get_delta() * touch_pan_sensitivity * 15.0f / target_zoom;
        return;
    }

    // 3. Tactile / Trackpad : pincement (facteur multiplicateur, ex. 1.02 pour un zoom avant)
    Ref<InputEventMagnifyGesture> magnify_gesture = p_event;
    if (magnify_gesture.is_valid())
    {
        _apply_zoom(magnify_gesture->get_factor(), magnify_gesture->get_position());
        return;
    }

    // 4. Zoom à la molette
    Ref<InputEventMouseButton> mouse_button = p_event;
    if (mouse_button.is_valid() && mouse_button->is_pressed())
    {
        switch (mouse_button->get_button_index())
        {
        case MOUSE_BUTTON_WHEEL_UP:
            _apply_zoom(zoom_factor, mouse_button->get_position());
            break;
        case MOUSE_BUTTON_WHEEL_DOWN:
            _apply_zoom(1.0f / zoom_factor, mouse_button->get_position());
            break;
        default:
            break;
        }
    }
}

void CameraPanner::_handle_joypad_input(float p_delta)
{
    Input *input = Input::get_singleton();
    // get_vector / get_axis appliquent déjà la zone morte des actions.
    const Vector2 pan_direction = input->get_vector(action_pan_left, action_pan_right, action_pan_up, action_pan_down);
    const float zoom_direction = input->get_axis(action_zoom_out, action_zoom_in);

    if (pan_direction.length() > 0.1f)
    {
        target_position += pan_direction * joypad_pan_speed * p_delta / target_zoom;
    }

    if (Math::abs(zoom_direction) > 0.1f)
    {
        // Pour la manette, on zoome depuis le centre de l'écran
        const Vector2 screen_center = get_viewport_rect().size / 2.0f;
        _apply_zoom(1.0f + zoom_direction * joypad_zoom_speed * p_delta, screen_center);
    }
}

void CameraPanner::_apply_zoom(float p_factor, const Vector2 &p_screen_position)
{
    const float new_zoom = CLAMP(target_zoom * p_factor, min_zoom, max_zoom);

    const Vector2 screen_center = get_viewport_rect().size / 2.0f;
    const Vector2 offset_screen = p_screen_position - screen_center;
    const Vector2 world_before = target_position + offset_screen / target_zoom;
    const Vector2 world_after = target_position + offset_screen / new_zoom;
    target_position += world_before - world_after;

    target_zoom = new_zoom;
}

void CameraPanner::teleport(const Vector2 &p_position, float p_zoom)
{
    target_position = p_position;
    target_zoom = CLAMP(p_zoom, min_zoom, max_zoom);
    set_position(target_position);
    set_zoom(Vector2(target_zoom, target_zoom));
}

void CameraPanner::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_pan_mouse_button", "button"), &CameraPanner::set_pan_mouse_button);
    ClassDB::bind_method(D_METHOD("get_pan_mouse_button"), &CameraPanner::get_pan_mouse_button);
    ClassDB::bind_method(D_METHOD("set_zoom_factor", "factor"), &CameraPanner::set_zoom_factor);
    ClassDB::bind_method(D_METHOD("get_zoom_factor"), &CameraPanner::get_zoom_factor);
    ClassDB::bind_method(D_METHOD("set_min_zoom", "zoom"), &CameraPanner::set_min_zoom);
    ClassDB::bind_method(D_METHOD("get_min_zoom"), &CameraPanner::get_min_zoom);
    ClassDB::bind_method(D_METHOD("set_max_zoom", "zoom"), &CameraPanner::set_max_zoom);
    ClassDB::bind_method(D_METHOD("get_max_zoom"), &CameraPanner::get_max_zoom);
    ClassDB::bind_method(D_METHOD("set_lerp_speed", "speed"), &CameraPanner::set_lerp_speed);
    ClassDB::bind_method(D_METHOD("get_lerp_speed"), &CameraPanner::get_lerp_speed);
    ClassDB::bind_method(D_METHOD("set_joypad_pan_speed", "speed"), &CameraPanner::set_joypad_pan_speed);
    ClassDB::bind_method(D_METHOD("get_joypad_pan_speed"), &CameraPanner::get_joypad_pan_speed);
    ClassDB::bind_method(D_METHOD("set_joypad_zoom_speed", "speed"), &CameraPanner::set_joypad_zoom_speed);
    ClassDB::bind_method(D_METHOD("get_joypad_zoom_speed"), &CameraPanner::get_joypad_zoom_speed);
    ClassDB::bind_method(D_METHOD("set_touch_pan_sensitivity", "sensitivity"), &CameraPanner::set_touch_pan_sensitivity);
    ClassDB::bind_method(D_METHOD("get_touch_pan_sensitivity"), &CameraPanner::get_touch_pan_sensitivity);
    ClassDB::bind_method(D_METHOD("set_action_pan_left", "action"), &CameraPanner::set_action_pan_left);
    ClassDB::bind_method(D_METHOD("get_action_pan_left"), &CameraPanner::get_action_pan_left);
    ClassDB::bind_method(D_METHOD("set_action_pan_right", "action"), &CameraPanner::set_action_pan_right);
    ClassDB::bind_method(D_METHOD("get_action_pan_right"), &CameraPanner::get_action_pan_right);
    ClassDB::bind_method(D_METHOD("set_action_pan_up", "action"), &CameraPanner::set_action_pan_up);
    ClassDB::bind_method(D_METHOD("get_action_pan_up"), &CameraPanner::get_action_pan_up);
    ClassDB::bind_method(D_METHOD("set_action_pan_down", "action"), &CameraPanner::set_action_pan_down);
    ClassDB::bind_method(D_METHOD("get_action_pan_down"), &CameraPanner::get_action_pan_down);
    ClassDB::bind_method(D_METHOD("set_action_zoom_in", "action"), &CameraPanner::set_action_zoom_in);
    ClassDB::bind_method(D_METHOD("get_action_zoom_in"), &CameraPanner::get_action_zoom_in);
    ClassDB::bind_method(D_METHOD("set_action_zoom_out", "action"), &CameraPanner::set_action_zoom_out);
    ClassDB::bind_method(D_METHOD("get_action_zoom_out"), &CameraPanner::get_action_zoom_out);
    ClassDB::bind_method(D_METHOD("teleport", "position", "zoom"), &CameraPanner::teleport);

    ADD_GROUP(U"Souris", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "pan_mouse_button", PROPERTY_HINT_ENUM, "Left:1,Right:2,Middle:3,Extra 1:8,Extra 2:9"), "set_pan_mouse_button", "get_pan_mouse_button");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "zoom_factor", PROPERTY_HINT_RANGE, "1.01,2,0.01,or_greater"), "set_zoom_factor", "get_zoom_factor");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_zoom", PROPERTY_HINT_RANGE, "0.01,1,0.01,or_greater"), "set_min_zoom", "get_min_zoom");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_zoom", PROPERTY_HINT_RANGE, "1,50,0.1,or_greater"), "set_max_zoom", "get_max_zoom");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lerp_speed", PROPERTY_HINT_RANGE, "1,100,0.5,or_greater"), "set_lerp_speed", "get_lerp_speed");

    ADD_GROUP(U"Manette et tactile", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "joypad_pan_speed", PROPERTY_HINT_RANGE, "0,2000,1,or_greater"), "set_joypad_pan_speed", "get_joypad_pan_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "joypad_zoom_speed", PROPERTY_HINT_RANGE, "0,10,0.1,or_greater"), "set_joypad_zoom_speed", "get_joypad_zoom_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "touch_pan_sensitivity", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater"), "set_touch_pan_sensitivity", "get_touch_pan_sensitivity");

    ADD_GROUP(U"Actions d'entrée", "action_");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_pan_left", PROPERTY_HINT_INPUT_NAME), "set_action_pan_left", "get_action_pan_left");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_pan_right", PROPERTY_HINT_INPUT_NAME), "set_action_pan_right", "get_action_pan_right");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_pan_up", PROPERTY_HINT_INPUT_NAME), "set_action_pan_up", "get_action_pan_up");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_pan_down", PROPERTY_HINT_INPUT_NAME), "set_action_pan_down", "get_action_pan_down");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_zoom_in", PROPERTY_HINT_INPUT_NAME), "set_action_zoom_in", "get_action_zoom_in");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_zoom_out", PROPERTY_HINT_INPUT_NAME), "set_action_zoom_out", "get_action_zoom_out");
}
