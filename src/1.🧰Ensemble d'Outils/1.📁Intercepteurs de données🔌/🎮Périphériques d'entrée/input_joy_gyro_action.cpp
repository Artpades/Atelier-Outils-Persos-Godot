#include "input_joy_gyro_action.hpp"

#include "input_names.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos_2d.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal/custom_logger.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void InputJoyGyroAction::_process(double p_delta)
{
    if (Engine::get_singleton()->is_editor_hint() || !action_enabled || !is_gamepad_allowed())
    {
        return;
    }

    Input *input = Input::get_singleton();
    if (auto_enable_sensors && !(bool)input->call("is_joy_motion_sensors_enabled", device_id))
    {
        if (!has_sensors())
        {
            // Signalé une seule fois par manette, même sans debug_log : sinon l'action reste muette sans raison visible.
            if (warned_device != device_id)
            {
                warned_device = device_id;
                CustomLogger::log(get_name(), InputNames::ICON_ACTION, "La manette " + itos(device_id) + " n'a pas de capteurs de mouvement.");
            }
            return;
        }
        set_sensors_enabled(true);
    }

    Vector3 gyro = get_gyroscope();
    if (!gyro_initialized || gyro.distance_squared_to(last_emitted_gyro) >= dirty_threshold * dirty_threshold)
    {
        gyro_initialized = true;
        last_emitted_gyro = gyro;
        emit_signal("gyroscope_emitted", gyro);
        log_debug("gyroscope_emitted : " + String(gyro));
    }

    if (track_accelerometer)
    {
        Vector3 accel = get_accelerometer();
        if (!accel_initialized || accel.distance_squared_to(last_emitted_accel) >= dirty_threshold * dirty_threshold)
        {
            accel_initialized = true;
            last_emitted_accel = accel;
            emit_signal("accelerometer_emitted", accel);
        }
    }

    draw_debug(gyro);
}

Vector3 InputJoyGyroAction::get_gyroscope() const
{
    if (!action_enabled || !is_gamepad_allowed())
    {
        return Vector3();
    }
    return (Vector3)Input::get_singleton()->call("get_joy_gyroscope", device_id) * gyro_sensitivity;
}

Vector3 InputJoyGyroAction::get_accelerometer() const
{
    if (!action_enabled || !is_gamepad_allowed())
    {
        return Vector3();
    }
    return Input::get_singleton()->call("get_joy_accelerometer", device_id);
}

void InputJoyGyroAction::set_sensors_enabled(bool p_enabled)
{
    Input::get_singleton()->call("set_joy_motion_sensors_enabled", device_id, p_enabled);
    log_debug(p_enabled ? "Capteurs activés" : String::utf8("Capteurs désactivés"));
}

bool InputJoyGyroAction::has_sensors() const { return Input::get_singleton()->call("has_joy_motion_sensors", device_id); }

void InputJoyGyroAction::draw_debug(const Vector3 &p_gyro)
{
    if (!can_debug_draw())
    {
        return;
    }
    draw_debug_label();
    // Une barre par axe de rotation (X rouge, Y vert, Z bleu), pleine échelle à 2π rad/s.
    const Color colors[3] = {Color(1, 0.3f, 0.3f), Color(0.3f, 1, 0.3f), Color(0.3f, 0.5f, 1)};
    for (int axis = 0; axis < 3; axis++)
    {
        float ratio = CLAMP(p_gyro[axis] / (float)Math_TAU, -1.0f, 1.0f);
        Vector2 origin = debug_draw_position + Vector2(-debug_draw_size, (axis - 1) * 10.0f);
        Rect2 bar(origin.x + debug_draw_size, origin.y - 3.0f, ratio * debug_draw_size, 6.0f);
        Gizmos2D::draw_2D_rectangle(debug_gizmo(1 + axis), bar.abs(), colors[axis], true);
    }
}

void InputJoyGyroAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_gyroscope"), &InputJoyGyroAction::get_gyroscope);
    ClassDB::bind_method(D_METHOD("get_accelerometer"), &InputJoyGyroAction::get_accelerometer);
    ClassDB::bind_method(D_METHOD("set_sensors_enabled", "enabled"), &InputJoyGyroAction::set_sensors_enabled);
    ClassDB::bind_method(D_METHOD("has_sensors"), &InputJoyGyroAction::has_sensors);
    ClassDB::bind_method(D_METHOD("set_auto_enable_sensors", "enabled"), &InputJoyGyroAction::set_auto_enable_sensors);
    ClassDB::bind_method(D_METHOD("get_auto_enable_sensors"), &InputJoyGyroAction::get_auto_enable_sensors);
    ClassDB::bind_method(D_METHOD("set_gyro_sensitivity", "sensitivity"), &InputJoyGyroAction::set_gyro_sensitivity);
    ClassDB::bind_method(D_METHOD("get_gyro_sensitivity"), &InputJoyGyroAction::get_gyro_sensitivity);
    ClassDB::bind_method(D_METHOD("set_track_accelerometer", "track"), &InputJoyGyroAction::set_track_accelerometer);
    ClassDB::bind_method(D_METHOD("get_track_accelerometer"), &InputJoyGyroAction::get_track_accelerometer);

    ADD_GROUP(U"⚙️ Paramètres Gyroscope", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_enable_sensors"), "set_auto_enable_sensors", "get_auto_enable_sensors");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gyro_sensitivity", PROPERTY_HINT_RANGE, "0.1,10.0,0.1"), "set_gyro_sensitivity", "get_gyro_sensitivity");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "track_accelerometer"), "set_track_accelerometer", "get_track_accelerometer");

    ADD_SIGNAL(MethodInfo("gyroscope_emitted", PropertyInfo(Variant::VECTOR3, "gyro")));
    ADD_SIGNAL(MethodInfo("accelerometer_emitted", PropertyInfo(Variant::VECTOR3, "accel")));
}
