#include "mobile_input.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_magnify_gesture.hpp>
#include <godot_cpp/classes/input_event_pan_gesture.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

#include <iterator>

using namespace godot;

namespace
{
/// Noms des capteurs, dans l'ordre de MobileInput::Sensor (préfixes des propriétés et des signaux).
const char *const SENSOR_NAMES[] = {"gyroscope", "accelerometer", "gravity", "magnetometer"};

/// Tolérance par composante pour considérer qu'une valeur de capteur a changé.
constexpr float SENSOR_CHANGE_TOLERANCE = 0.001f;
/// Variation de distance entre les deux doigts (en pixels) en dessous de laquelle le pincement n'est pas signalé.
constexpr float PINCH_MIN_STEP = 0.5f;

bool vector3_differs(const Vector3 &a, const Vector3 &b)
{
    return Math::abs(a.x - b.x) >= SENSOR_CHANGE_TOLERANCE || Math::abs(a.y - b.y) >= SENSOR_CHANGE_TOLERANCE ||
           Math::abs(a.z - b.z) >= SENSOR_CHANGE_TOLERANCE;
}
} // namespace

MobileInput::MobileInput()
{
    // Les valeurs brutes n'ont pas la même échelle : ~9.8 m/s² pour l'accéléromètre et la gravité,
    // 25 à 65 µT pour le magnétomètre, quelques rad/s pour le gyroscope.
    sensors[SENSOR_GYROSCOPE].sensitivity = 1.0f;
    sensors[SENSOR_ACCELEROMETER].sensitivity = 0.1f;
    sensors[SENSOR_GRAVITY].sensitivity = 0.1f;
    sensors[SENSOR_MAGNETOMETER].sensitivity = 0.02f;
}

void MobileInput::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("handle_input_event", "event"), &MobileInput::handle_input_event);

    ClassDB::bind_method(D_METHOD("set_tap_max_duration", "value"), &MobileInput::set_tap_max_duration);
    ClassDB::bind_method(D_METHOD("get_tap_max_duration"), &MobileInput::get_tap_max_duration);
    ClassDB::bind_method(D_METHOD("set_tap_max_distance", "value"), &MobileInput::set_tap_max_distance);
    ClassDB::bind_method(D_METHOD("get_tap_max_distance"), &MobileInput::get_tap_max_distance);
    ClassDB::bind_method(D_METHOD("set_double_tap_delay", "value"), &MobileInput::set_double_tap_delay);
    ClassDB::bind_method(D_METHOD("get_double_tap_delay"), &MobileInput::get_double_tap_delay);
    ClassDB::bind_method(D_METHOD("set_long_press_duration", "value"), &MobileInput::set_long_press_duration);
    ClassDB::bind_method(D_METHOD("get_long_press_duration"), &MobileInput::get_long_press_duration);
    ClassDB::bind_method(D_METHOD("set_swipe_min_distance", "value"), &MobileInput::set_swipe_min_distance);
    ClassDB::bind_method(D_METHOD("get_swipe_min_distance"), &MobileInput::get_swipe_min_distance);
    ClassDB::bind_method(D_METHOD("set_swipe_min_velocity", "value"), &MobileInput::set_swipe_min_velocity);
    ClassDB::bind_method(D_METHOD("get_swipe_min_velocity"), &MobileInput::get_swipe_min_velocity);
    ClassDB::bind_method(D_METHOD("set_max_fingers", "value"), &MobileInput::set_max_fingers);
    ClassDB::bind_method(D_METHOD("get_max_fingers"), &MobileInput::get_max_fingers);
    ClassDB::bind_method(D_METHOD("set_sensor_dead_zone", "value"), &MobileInput::set_sensor_dead_zone);
    ClassDB::bind_method(D_METHOD("get_sensor_dead_zone"), &MobileInput::get_sensor_dead_zone);
    ClassDB::bind_method(D_METHOD("set_sensor_enabled", "sensor", "enabled"), &MobileInput::set_sensor_enabled);
    ClassDB::bind_method(D_METHOD("is_sensor_enabled", "sensor"), &MobileInput::is_sensor_enabled);
    ClassDB::bind_method(D_METHOD("set_sensor_sensitivity", "sensor", "sensitivity"), &MobileInput::set_sensor_sensitivity);
    ClassDB::bind_method(D_METHOD("get_sensor_sensitivity", "sensor"), &MobileInput::get_sensor_sensitivity);

    ClassDB::bind_method(D_METHOD("get_finger_count"), &MobileInput::get_finger_count);
    ClassDB::bind_method(D_METHOD("has_finger", "index"), &MobileInput::has_finger);
    ClassDB::bind_method(D_METHOD("get_finger_position", "index"), &MobileInput::get_finger_position);
    ClassDB::bind_method(D_METHOD("get_primary_finger_position"), &MobileInput::get_primary_finger_position);
    ClassDB::bind_method(D_METHOD("get_fingers"), &MobileInput::get_fingers);
    ClassDB::bind_method(D_METHOD("is_pinching"), &MobileInput::is_pinching);
    ClassDB::bind_method(D_METHOD("get_sensor_value", "sensor"), &MobileInput::get_sensor_value);
    ClassDB::bind_method(D_METHOD("get_gyroscope"), &MobileInput::get_gyroscope);
    ClassDB::bind_method(D_METHOD("get_accelerometer"), &MobileInput::get_accelerometer);
    ClassDB::bind_method(D_METHOD("get_gravity"), &MobileInput::get_gravity);
    ClassDB::bind_method(D_METHOD("get_magnetometer"), &MobileInput::get_magnetometer);
    ClassDB::bind_method(D_METHOD("reset_touches"), &MobileInput::reset_touches);

    BIND_ENUM_CONSTANT(SWIPE_NONE);
    BIND_ENUM_CONSTANT(SWIPE_LEFT);
    BIND_ENUM_CONSTANT(SWIPE_RIGHT);
    BIND_ENUM_CONSTANT(SWIPE_UP);
    BIND_ENUM_CONSTANT(SWIPE_DOWN);

    BIND_ENUM_CONSTANT(SENSOR_GYROSCOPE);
    BIND_ENUM_CONSTANT(SENSOR_ACCELEROMETER);
    BIND_ENUM_CONSTANT(SENSOR_GRAVITY);
    BIND_ENUM_CONSTANT(SENSOR_MAGNETOMETER);
    BIND_ENUM_CONSTANT(SENSOR_MAX);

    ADD_GROUP(U"👆 Tap", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tap_max_duration", PROPERTY_HINT_RANGE, "0.05,1.0,0.05,suffix:s"), "set_tap_max_duration", "get_tap_max_duration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tap_max_distance", PROPERTY_HINT_RANGE, "0,100,1,suffix:px"), "set_tap_max_distance", "get_tap_max_distance");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "double_tap_delay", PROPERTY_HINT_RANGE, "0.1,1.0,0.05,suffix:s"), "set_double_tap_delay", "get_double_tap_delay");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "long_press_duration", PROPERTY_HINT_RANGE, "0.2,3.0,0.05,suffix:s"), "set_long_press_duration", "get_long_press_duration");

    ADD_GROUP(U"👉 Swipe", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "swipe_min_distance", PROPERTY_HINT_RANGE, "10,500,10,suffix:px"), "set_swipe_min_distance", "get_swipe_min_distance");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "swipe_min_velocity", PROPERTY_HINT_RANGE, "100,4000,50,suffix:px/s"), "set_swipe_min_velocity", "get_swipe_min_velocity");

    ADD_GROUP(U"🖐️ Multi-touch", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_fingers", PROPERTY_HINT_RANGE, "1,10,1"), "set_max_fingers", "get_max_fingers");

    ADD_GROUP(U"🧭 Capteurs", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sensor_dead_zone", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_sensor_dead_zone", "get_sensor_dead_zone");
    ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "use_gyroscope"), "set_sensor_enabled", "is_sensor_enabled", SENSOR_GYROSCOPE);
    ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "gyroscope_sensitivity", PROPERTY_HINT_RANGE, "0.1,5.0,0.1"), "set_sensor_sensitivity", "get_sensor_sensitivity", SENSOR_GYROSCOPE);
    ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "use_accelerometer"), "set_sensor_enabled", "is_sensor_enabled", SENSOR_ACCELEROMETER);
    ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "accelerometer_sensitivity", PROPERTY_HINT_RANGE, "0.01,1.0,0.01"), "set_sensor_sensitivity", "get_sensor_sensitivity", SENSOR_ACCELEROMETER);
    ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "use_gravity"), "set_sensor_enabled", "is_sensor_enabled", SENSOR_GRAVITY);
    ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "gravity_sensitivity", PROPERTY_HINT_RANGE, "0.01,1.0,0.01"), "set_sensor_sensitivity", "get_sensor_sensitivity", SENSOR_GRAVITY);
    ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "use_magnetometer"), "set_sensor_enabled", "is_sensor_enabled", SENSOR_MAGNETOMETER);
    ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "magnetometer_sensitivity", PROPERTY_HINT_RANGE, "0.001,0.1,0.001"), "set_sensor_sensitivity", "get_sensor_sensitivity", SENSOR_MAGNETOMETER);

    // Toucher de base
    ADD_SIGNAL(MethodInfo("touch_pressed", PropertyInfo(Variant::INT, "finger_index"), PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("touch_released", PropertyInfo(Variant::INT, "finger_index"), PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("drag", PropertyInfo(Variant::INT, "finger_index"), PropertyInfo(Variant::VECTOR2, "position"),
                          PropertyInfo(Variant::VECTOR2, "relative"), PropertyInfo(Variant::FLOAT, "pressure")));
    ADD_SIGNAL(MethodInfo("multi_touch_changed", PropertyInfo(Variant::INT, "finger_count")));

    // Gestes
    ADD_SIGNAL(MethodInfo("tap", PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("double_tap", PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("long_press", PropertyInfo(Variant::INT, "finger_index"), PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("swipe", PropertyInfo(Variant::INT, "direction", PROPERTY_HINT_ENUM, "None,Left,Right,Up,Down"),
                          PropertyInfo(Variant::VECTOR2, "start_position"), PropertyInfo(Variant::VECTOR2, "velocity")));
    ADD_SIGNAL(MethodInfo("pinch", PropertyInfo(Variant::FLOAT, "scale_factor"), PropertyInfo(Variant::VECTOR2, "center")));
    ADD_SIGNAL(MethodInfo("pinch_started", PropertyInfo(Variant::VECTOR2, "center")));
    ADD_SIGNAL(MethodInfo("pinch_ended"));
    ADD_SIGNAL(MethodInfo("pan", PropertyInfo(Variant::VECTOR2, "delta")));
    ADD_SIGNAL(MethodInfo("magnify", PropertyInfo(Variant::FLOAT, "factor")));

    // Capteurs : <capteur>_updated à chaque image, <capteur>_changed seulement si la valeur change.
    for (const char *name : SENSOR_NAMES)
    {
        ADD_SIGNAL(MethodInfo(String(name) + "_updated", PropertyInfo(Variant::VECTOR3, "value")));
        ADD_SIGNAL(MethodInfo(String(name) + "_changed", PropertyInfo(Variant::VECTOR3, "value")));
    }
}

void MobileInput::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_READY:
    {
        const bool running = !Engine::get_singleton()->is_editor_hint();
        set_process(running);
        set_process_input(running);
        break;
    }
    // Les relâchements peuvent se perdre quand l'application passe en arrière-plan.
    case NOTIFICATION_APPLICATION_PAUSED:
    case NOTIFICATION_WM_WINDOW_FOCUS_OUT:
        reset_touches();
        break;
    default:
        break;
    }
}

double MobileInput::_now()
{
    return Time::get_singleton()->get_ticks_usec() / 1000000.0;
}

#pragma region Évènements

void MobileInput::_input(const Ref<InputEvent> &p_event)
{
    handle_input_event(p_event);
}

void MobileInput::handle_input_event(const Ref<InputEvent> &p_event)
{
    if (p_event.is_null())
    {
        return;
    }

    if (Ref<InputEventScreenTouch> touch = p_event; touch.is_valid())
    {
        _handle_screen_touch(touch);
    }
    else if (Ref<InputEventScreenDrag> drag_event = p_event; drag_event.is_valid())
    {
        _handle_screen_drag(drag_event);
    }
    else if (Ref<InputEventPanGesture> pan_event = p_event; pan_event.is_valid())
    {
        emit_signal("pan", pan_event->get_delta());
    }
    else if (Ref<InputEventMagnifyGesture> magnify_event = p_event; magnify_event.is_valid())
    {
        emit_signal("magnify", magnify_event->get_factor());
    }
}

void MobileInput::_handle_screen_touch(const Ref<InputEventScreenTouch> &p_event)
{
    const int index = p_event->get_index();
    const Vector2 position = p_event->get_position();
    const double now = _now();

    if (p_event->is_pressed())
    {
        // Doigt déjà suivi (relâchement perdu) : on le reprend comme un nouvel appui.
        if (!has_finger(index) && get_finger_count() >= max_fingers)
        {
            return;
        }
        Finger finger;
        finger.position = position;
        finger.start_position = position;
        finger.press_time = now;
        fingers[index] = finger;

        emit_signal("touch_pressed", index, position);
        _on_finger_count_changed();
        return;
    }

    auto it = fingers.find(index);
    if (it == fingers.end())
    {
        return;
    }
    Finger finger = it->second;
    finger.position = position;
    fingers.erase(it);

    emit_signal("touch_released", index, position);
    // Un toucher annulé par le système (geste système, appel entrant...) ne déclenche aucun geste, ni un
    // doigt qui a participé à un geste à plusieurs doigts (les doigts d'un pincement ne font pas de swipe).
    if (!p_event->is_canceled() && !finger.in_multi_touch)
    {
        _detect_tap(finger, position, now);
        _detect_swipe(finger);
    }
    _on_finger_count_changed();
}

void MobileInput::_handle_screen_drag(const Ref<InputEventScreenDrag> &p_event)
{
    auto it = fingers.find(p_event->get_index());
    if (it == fingers.end())
    {
        return;
    }
    it->second.position = p_event->get_position();
    it->second.velocity = p_event->get_velocity();

    emit_signal("drag", p_event->get_index(), p_event->get_position(), p_event->get_relative(), p_event->get_pressure());
    _update_pinch();
}

void MobileInput::_on_finger_count_changed()
{
    emit_signal("multi_touch_changed", get_finger_count());
    if (get_finger_count() > 1)
    {
        for (auto &[index, finger] : fingers)
        {
            finger.in_multi_touch = true;
        }
    }

    // Le pincement n'existe qu'avec exactement deux doigts : il repart de zéro à chaque changement.
    const bool was_pinching = pinching;
    pinching = false;
    if (was_pinching)
    {
        emit_signal("pinch_ended");
    }
    if (get_finger_count() != 2)
    {
        return;
    }

    const Finger &a = fingers.begin()->second;
    const Finger &b = std::next(fingers.begin())->second;
    initial_pinch_distance = a.position.distance_to(b.position);
    last_pinch_distance = initial_pinch_distance;
    pinching = initial_pinch_distance > 0.0f;
    if (pinching)
    {
        emit_signal("pinch_started", (a.position + b.position) * 0.5f);
    }
}

#pragma endregion

#pragma region Gestes

void MobileInput::_detect_tap(const Finger &p_finger, const Vector2 &p_position, double p_now)
{
    if (p_finger.long_pressed || p_now - p_finger.press_time > tap_max_duration ||
        p_finger.start_position.distance_to(p_position) > tap_max_distance)
    {
        return;
    }

    if (has_pending_tap && p_now - pending_tap_time <= double_tap_delay &&
        pending_tap_position.distance_to(p_position) <= tap_max_distance * 2.0f)
    {
        has_pending_tap = false;
        emit_signal("double_tap", p_position);
        return;
    }

    // Tap mis en attente : il ne sera signalé qu'une fois le délai de double tap écoulé sans second tap.
    // Un ancien tap encore en attente (trop loin pour un double tap) est signalé tout de suite.
    if (has_pending_tap)
    {
        emit_signal("tap", pending_tap_position);
    }
    has_pending_tap = true;
    pending_tap_time = p_now;
    pending_tap_position = p_position;
}

void MobileInput::_flush_pending_tap(double p_now)
{
    if (has_pending_tap && p_now - pending_tap_time > double_tap_delay)
    {
        has_pending_tap = false;
        emit_signal("tap", pending_tap_position);
    }
}

void MobileInput::_detect_swipe(const Finger &p_finger)
{
    const Vector2 swipe_vector = p_finger.position - p_finger.start_position;
    if (swipe_vector.length() < swipe_min_distance || p_finger.velocity.length() < swipe_min_velocity)
    {
        return;
    }

    SwipeDirection direction;
    if (Math::abs(swipe_vector.x) > Math::abs(swipe_vector.y))
    {
        direction = swipe_vector.x > 0.0f ? SWIPE_RIGHT : SWIPE_LEFT;
    }
    else
    {
        direction = swipe_vector.y > 0.0f ? SWIPE_DOWN : SWIPE_UP;
    }
    emit_signal("swipe", (int)direction, p_finger.start_position, p_finger.velocity);
}

void MobileInput::_update_pinch()
{
    if (!pinching || get_finger_count() != 2)
    {
        return;
    }

    const Finger &a = fingers.begin()->second;
    const Finger &b = std::next(fingers.begin())->second;
    const float distance = a.position.distance_to(b.position);
    if (Math::abs(distance - last_pinch_distance) < PINCH_MIN_STEP)
    {
        return;
    }
    last_pinch_distance = distance;
    emit_signal("pinch", distance / initial_pinch_distance, (a.position + b.position) * 0.5f);
}

void MobileInput::_update_long_press(double p_now)
{
    for (auto &[index, finger] : fingers)
    {
        if (finger.long_pressed || p_now - finger.press_time < long_press_duration ||
            finger.start_position.distance_to(finger.position) > tap_max_distance)
        {
            continue;
        }
        finger.long_pressed = true;
        emit_signal("long_press", index, finger.position);
    }
}

#pragma endregion

#pragma region Capteurs

void MobileInput::_process(double /*p_delta*/)
{
    const double now = _now();
    _flush_pending_tap(now);
    // Un seul doigt : l'appui long n'a pas de sens pendant un geste à plusieurs doigts.
    if (get_finger_count() == 1)
    {
        _update_long_press(now);
    }

    Input *input = Input::get_singleton();
    _update_sensor(SENSOR_GYROSCOPE, input->get_gyroscope());
    _update_sensor(SENSOR_ACCELEROMETER, input->get_accelerometer());
    _update_sensor(SENSOR_GRAVITY, input->get_gravity());
    _update_sensor(SENSOR_MAGNETOMETER, input->get_magnetometer());
}

void MobileInput::_update_sensor(Sensor p_sensor, const Vector3 &p_raw)
{
    SensorState &sensor = sensors[p_sensor];
    if (!sensor.enabled)
    {
        return;
    }

    Vector3 value = p_raw * sensor.sensitivity;
    // Zone morte sphérique : sur la longueur du vecteur, pas axe par axe.
    if (value.length() < sensor_dead_zone)
    {
        value = Vector3();
    }

    const String name = SENSOR_NAMES[p_sensor];
    emit_signal(name + String("_updated"), value);
    if (vector3_differs(sensor.value, value))
    {
        sensor.value = value;
        emit_signal(name + String("_changed"), value);
    }
}

void MobileInput::set_sensor_enabled(Sensor p_sensor, bool p_enabled)
{
    ERR_FAIL_INDEX(p_sensor, SENSOR_MAX);
    sensors[p_sensor].enabled = p_enabled;
    if (!p_enabled)
    {
        sensors[p_sensor].value = Vector3();
    }
}

bool MobileInput::is_sensor_enabled(Sensor p_sensor) const
{
    ERR_FAIL_INDEX_V(p_sensor, SENSOR_MAX, false);
    return sensors[p_sensor].enabled;
}

void MobileInput::set_sensor_sensitivity(Sensor p_sensor, float p_sensitivity)
{
    ERR_FAIL_INDEX(p_sensor, SENSOR_MAX);
    sensors[p_sensor].sensitivity = p_sensitivity;
}

float MobileInput::get_sensor_sensitivity(Sensor p_sensor) const
{
    ERR_FAIL_INDEX_V(p_sensor, SENSOR_MAX, 0.0f);
    return sensors[p_sensor].sensitivity;
}

Vector3 MobileInput::get_sensor_value(Sensor p_sensor) const
{
    ERR_FAIL_INDEX_V(p_sensor, SENSOR_MAX, Vector3());
    return sensors[p_sensor].value;
}

#pragma endregion

#pragma region Lecture

Variant MobileInput::get_finger_position(int p_index) const
{
    auto it = fingers.find(p_index);
    return it == fingers.end() ? Variant() : Variant(it->second.position);
}

Variant MobileInput::get_primary_finger_position() const
{
    return fingers.empty() ? Variant() : Variant(fingers.begin()->second.position);
}

Dictionary MobileInput::get_fingers() const
{
    Dictionary result;
    for (const auto &[index, finger] : fingers)
    {
        Dictionary data;
        data["position"] = finger.position;
        data["start_position"] = finger.start_position;
        data["velocity"] = finger.velocity;
        data["press_time"] = finger.press_time;
        result[index] = data;
    }
    return result;
}

void MobileInput::reset_touches()
{
    has_pending_tap = false;
    if (fingers.empty())
    {
        return;
    }
    fingers.clear();
    _on_finger_count_changed();
}

#pragma endregion
