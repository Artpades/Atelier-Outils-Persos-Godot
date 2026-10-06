#include "input_special_actions.hpp"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// --- InputMouseAction ---

void InputMouseAction::_handle_event(const Ref<InputEvent> &p_event)
{
    if (!is_keyboard_mouse_allowed() || simulated)
    {
        return;
    }
    Ref<InputEventMouseMotion> mouse_motion = p_event;
    if (mouse_motion.is_valid())
    {
        accumulated_motion += mouse_motion->get_relative();
        current_pressure = mouse_motion->get_pressure();
        return;
    }
    Ref<InputEventMouseButton> button = p_event;
    if (button.is_valid() && button->is_pressed())
    {
        float factor = button->get_factor() > 0.0f ? button->get_factor() : 1.0f;
        switch (button->get_button_index())
        {
        case MOUSE_BUTTON_WHEEL_UP:
            accumulated_scroll.y -= factor;
            break;
        case MOUSE_BUTTON_WHEEL_DOWN:
            accumulated_scroll.y += factor;
            break;
        case MOUSE_BUTTON_WHEEL_LEFT:
            accumulated_scroll.x -= factor;
            break;
        case MOUSE_BUTTON_WHEEL_RIGHT:
            accumulated_scroll.x += factor;
            break;
        default:
            break;
        }
    }
}

void InputMouseAction::_update(double /*p_delta*/)
{
    if (!is_keyboard_mouse_allowed())
    {
        _release();
        return;
    }
    // 📍 Position
    Node *owner = get_owner_node();
    Viewport *viewport = owner && owner->is_inside_tree() ? owner->get_viewport() : nullptr;
    if (viewport)
    {
        Vector2 current = viewport->get_mouse_position();
        if (normalize_position)
        {
            Vector2 size = viewport->get_visible_rect().size;
            if (size.x > 0.0f && size.y > 0.0f)
            {
                current = Vector2(current.x / size.x, current.y / size.y);
            }
        }
        position = current;
        double threshold = normalize_position ? position_threshold / 1000.0 : position_threshold;
        if (!position_initialized || position.distance_to(last_emitted_position) > threshold)
        {
            position_initialized = true;
            last_emitted_position = position;
            emit_event("position_changed", Array::make(position));
        }
    }
    // ↗️ Mouvement relatif
    Vector2 frame_motion = accumulated_motion * motion_sensitivity;
    if (invert_y)
    {
        frame_motion.y = -frame_motion.y;
    }
    accumulated_motion = Vector2();
    _emit_motion(frame_motion);
    // ✒️ Pression
    if (!Math::is_equal_approx(current_pressure, pressure))
    {
        pressure = current_pressure;
        emit_event("pressure_changed", Array::make(pressure));
    }
    // 🛞 Molette
    if (accumulated_scroll != Vector2())
    {
        Vector2 scroll = accumulated_scroll;
        accumulated_scroll = Vector2();
        emit_event("scrolled", Array::make(scroll));
    }
}

void InputMouseAction::_emit_motion(const Vector2 &p_motion)
{
    // Un mouvement est émis à chaque image où la souris bouge, puis une fois (0, 0) à l'arrêt.
    if (p_motion != Vector2() || motion != Vector2())
    {
        motion = p_motion;
        emit_event("motion", Array::make(motion));
    }
}

void InputMouseAction::_apply_simulation(const Variant &p_value, double /*p_delta*/) { _emit_motion(Vector2(p_value) * motion_sensitivity); }

void InputMouseAction::_release()
{
    accumulated_motion = Vector2();
    accumulated_scroll = Vector2();
    _emit_motion(Vector2());
}

void InputMouseAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_motion_sensitivity", "sensitivity"), &InputMouseAction::set_motion_sensitivity);
    ClassDB::bind_method(D_METHOD("get_motion_sensitivity"), &InputMouseAction::get_motion_sensitivity);
    ClassDB::bind_method(D_METHOD("set_position_threshold", "threshold"), &InputMouseAction::set_position_threshold);
    ClassDB::bind_method(D_METHOD("get_position_threshold"), &InputMouseAction::get_position_threshold);
    ClassDB::bind_method(D_METHOD("set_normalize_position", "normalize"), &InputMouseAction::set_normalize_position);
    ClassDB::bind_method(D_METHOD("get_normalize_position"), &InputMouseAction::get_normalize_position);
    ClassDB::bind_method(D_METHOD("set_invert_y", "invert"), &InputMouseAction::set_invert_y);
    ClassDB::bind_method(D_METHOD("get_invert_y"), &InputMouseAction::get_invert_y);
    ClassDB::bind_method(D_METHOD("get_mouse_position"), &InputMouseAction::get_mouse_position);
    ClassDB::bind_method(D_METHOD("get_motion"), &InputMouseAction::get_motion);
    ClassDB::bind_method(D_METHOD("get_pressure"), &InputMouseAction::get_pressure);

    ADD_SIGNAL(MethodInfo("position_changed", PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("motion", PropertyInfo(Variant::VECTOR2, "relative")));
    ADD_SIGNAL(MethodInfo("pressure_changed", PropertyInfo(Variant::FLOAT, "pressure")));
    ADD_SIGNAL(MethodInfo("scrolled", PropertyInfo(Variant::VECTOR2, "delta")));

    ADD_GROUP(U"🖱️ Souris", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "motion_sensitivity", PROPERTY_HINT_RANGE, "0.01,10,0.01,or_greater"), "set_motion_sensitivity", "get_motion_sensitivity");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "position_threshold", PROPERTY_HINT_RANGE, "0,10,0.1,suffix:px"), "set_position_threshold", "get_position_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "normalize_position"), "set_normalize_position", "get_normalize_position");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "invert_y"), "set_invert_y", "get_invert_y");
}

// --- InputGyroAction ---
// Les capteurs de manette (get_joy_gyroscope...) datent de Godot 4.7 : appelés par nom, pour rester
// compatibles avec des en-têtes godot-cpp plus anciens.

bool InputGyroAction::has_sensors() const
{
    Input *input = Input::get_singleton();
    return input->has_method("has_joy_motion_sensors") && bool(input->call("has_joy_motion_sensors", get_device()));
}

void InputGyroAction::set_sensors_enabled(bool p_enabled)
{
    Input *input = Input::get_singleton();
    if (input->has_method("set_joy_motion_sensors_enabled"))
    {
        input->call("set_joy_motion_sensors_enabled", get_device(), p_enabled);
    }
}

void InputGyroAction::_update(double /*p_delta*/)
{
    if (!is_gamepad_allowed())
    {
        _release();
        return;
    }
    Input *input = Input::get_singleton();
    if (!input->has_method("get_joy_gyroscope"))
    {
        return;
    }
    if (auto_enable_sensors && input->has_method("is_joy_motion_sensors_enabled") && !bool(input->call("is_joy_motion_sensors_enabled", get_device())))
    {
        set_sensors_enabled(true);
    }
    Vector3 gyro = Vector3(input->call("get_joy_gyroscope", get_device())) * sensitivity;
    Vector3 accel = track_accelerometer ? Vector3(input->call("get_joy_accelerometer", get_device())) : Vector3();
    _process(gyro, accel);
}

void InputGyroAction::_apply_simulation(const Variant &p_value, double /*p_delta*/) { _process(Vector3(p_value) * sensitivity, accelerometer); }

void InputGyroAction::_release() { _process(Vector3(), Vector3()); }

void InputGyroAction::_process(const Vector3 &p_gyroscope, const Vector3 &p_accelerometer)
{
    double threshold = get_dirty_threshold();
    gyroscope = p_gyroscope;
    accelerometer = p_accelerometer;
    if (gyroscope != last_emitted_gyroscope && (gyroscope.distance_to(last_emitted_gyroscope) >= threshold || gyroscope == Vector3()))
    {
        last_emitted_gyroscope = gyroscope;
        emit_event("gyroscope_changed", Array::make(gyroscope));
    }
    if (accelerometer != last_emitted_accelerometer && (accelerometer.distance_to(last_emitted_accelerometer) >= threshold || accelerometer == Vector3()))
    {
        last_emitted_accelerometer = accelerometer;
        emit_event("accelerometer_changed", Array::make(accelerometer));
    }
}

void InputGyroAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_auto_enable_sensors", "enable"), &InputGyroAction::set_auto_enable_sensors);
    ClassDB::bind_method(D_METHOD("get_auto_enable_sensors"), &InputGyroAction::get_auto_enable_sensors);
    ClassDB::bind_method(D_METHOD("set_sensitivity", "sensitivity"), &InputGyroAction::set_sensitivity);
    ClassDB::bind_method(D_METHOD("get_sensitivity"), &InputGyroAction::get_sensitivity);
    ClassDB::bind_method(D_METHOD("set_track_accelerometer", "track"), &InputGyroAction::set_track_accelerometer);
    ClassDB::bind_method(D_METHOD("get_track_accelerometer"), &InputGyroAction::get_track_accelerometer);
    ClassDB::bind_method(D_METHOD("get_gyroscope"), &InputGyroAction::get_gyroscope);
    ClassDB::bind_method(D_METHOD("get_accelerometer"), &InputGyroAction::get_accelerometer);
    ClassDB::bind_method(D_METHOD("has_sensors"), &InputGyroAction::has_sensors);
    ClassDB::bind_method(D_METHOD("set_sensors_enabled", "enabled"), &InputGyroAction::set_sensors_enabled);

    ADD_SIGNAL(MethodInfo("gyroscope_changed", PropertyInfo(Variant::VECTOR3, "rotation_rate")));
    ADD_SIGNAL(MethodInfo("accelerometer_changed", PropertyInfo(Variant::VECTOR3, "acceleration")));

    ADD_GROUP(U"🧭 Capteurs", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_enable_sensors"), "set_auto_enable_sensors", "get_auto_enable_sensors");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sensitivity", PROPERTY_HINT_RANGE, "0.1,10,0.1,or_greater"), "set_sensitivity", "get_sensitivity");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "track_accelerometer"), "set_track_accelerometer", "get_track_accelerometer");
}
