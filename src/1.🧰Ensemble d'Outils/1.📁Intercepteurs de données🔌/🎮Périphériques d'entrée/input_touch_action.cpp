#include "input_touch_action.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos_2d.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_magnify_gesture.hpp>
#include <godot_cpp/classes/input_event_pan_gesture.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace
{
/// Au-delà, les index de gizmos des doigts empièteraient sur celui du pincement.
constexpr int MAX_TRACKED_FINGERS = 10;
constexpr int PINCH_GIZMO = 1 + MAX_TRACKED_FINGERS;
} // namespace

void InputTouchAction::_process(double p_delta)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (!action_enabled)
    {
        reset_touches();
        return;
    }

    double now = now_seconds();

    if (has_pending_pan)
    {
        emit_signal("pan", pending_pan);
        pending_pan = Vector2();
        has_pending_pan = false;
    }

    if (has_pending_magnify)
    {
        emit_signal("magnify", pending_magnify);
        pending_magnify = 1.0f;
        has_pending_magnify = false;
    }

    // Aucun second tap n'est venu à temps : c'était un tap simple.
    if (has_pending_tap && now - pending_tap_time > double_tap_delay)
    {
        has_pending_tap = false;
        emit_signal("tap", pending_tap_position);
        log_debug("tap : " + String(pending_tap_position));
    }

    if (!multi_touch_gesture)
    {
        for (KeyValue<int, Finger> &E : fingers)
        {
            Finger &finger = E.value;
            if (!finger.moved && !finger.long_pressed && now - finger.press_time >= long_press_duration)
            {
                finger.long_pressed = true;
                emit_signal("long_press", finger.position);
                log_debug("long_press : " + String(finger.position));
            }
        }
    }

    draw_debug();
}

void InputTouchAction::_input(const Ref<InputEvent> &p_event)
{
    if (Engine::get_singleton()->is_editor_hint() || !action_enabled)
    {
        return;
    }

    if (InputEventScreenTouch *touch = Object::cast_to<InputEventScreenTouch>(*p_event))
    {
        handle_touch(touch->get_index(), touch->get_position(), touch->is_pressed());
    }
    else if (InputEventScreenDrag *drag = Object::cast_to<InputEventScreenDrag>(*p_event))
    {
        handle_drag(drag->get_index(), drag->get_position(), drag->get_relative(), drag->get_pressure());
    }
    else if (InputEventPanGesture *pan = Object::cast_to<InputEventPanGesture>(*p_event))
    {
        pending_pan += pan->get_delta();
        has_pending_pan = true;
    }
    else if (InputEventMagnifyGesture *magnify = Object::cast_to<InputEventMagnifyGesture>(*p_event))
    {
        // Facteurs relatifs : ils se composent par multiplication.
        pending_magnify *= magnify->get_factor();
        has_pending_magnify = true;
    }
}

void InputTouchAction::handle_touch(int p_finger, const Vector2 &p_position, bool p_pressed)
{
    double now = now_seconds();

    if (p_pressed)
    {
        if (fingers.has(p_finger) || fingers.size() >= MIN(max_fingers, MAX_TRACKED_FINGERS))
        {
            return;
        }
        Finger finger;
        finger.position = p_position;
        finger.start_position = p_position;
        finger.press_time = now;
        finger.order = next_finger_order++;
        fingers.insert(p_finger, finger);

        if (fingers.size() >= 2)
        {
            multi_touch_gesture = true;
        }
        emit_signal("touch_pressed", p_finger, p_position);
        emit_signal("finger_count_changed", fingers.size());
        log_debug("touch_pressed : " + itos(p_finger) + " " + String(p_position));
        refresh_pinch();
        return;
    }

    HashMap<int, Finger>::Iterator it = fingers.find(p_finger);
    if (it == fingers.end())
    {
        return;
    }
    Finger finger = it->value;
    finger.position = p_position;
    fingers.remove(it);

    emit_signal("touch_released", p_finger, p_position);
    log_debug("touch_released : " + itos(p_finger) + " " + String(p_position));

    if (!multi_touch_gesture)
    {
        detect_tap(finger, p_position, now);
        detect_swipe(finger, p_position, now);
    }
    if (fingers.is_empty())
    {
        multi_touch_gesture = false;
    }

    emit_signal("finger_count_changed", fingers.size());
    refresh_pinch();
}

void InputTouchAction::handle_drag(int p_finger, const Vector2 &p_position, const Vector2 &p_relative, float p_pressure)
{
    HashMap<int, Finger>::Iterator it = fingers.find(p_finger);
    if (it == fingers.end())
    {
        return;
    }
    Finger &finger = it->value;
    finger.position = p_position;
    if (finger.start_position.distance_to(p_position) > tap_max_distance)
    {
        finger.moved = true;
    }

    emit_signal("drag", p_finger, p_position, p_relative, p_pressure);

    if (pinching && (p_finger == pinch_finger_a || p_finger == pinch_finger_b))
    {
        update_pinch();
    }
}

void InputTouchAction::detect_tap(const Finger &p_finger, const Vector2 &p_position, double p_now)
{
    if (p_finger.moved || p_finger.long_pressed || p_now - p_finger.press_time > tap_max_duration || p_finger.start_position.distance_to(p_position) > tap_max_distance)
    {
        return;
    }

    if (!detect_double_tap)
    {
        emit_signal("tap", p_position);
        log_debug("tap : " + String(p_position));
        return;
    }

    if (has_pending_tap && p_now - pending_tap_time <= double_tap_delay && pending_tap_position.distance_to(p_position) <= tap_max_distance * 2.0f)
    {
        has_pending_tap = false;
        emit_signal("double_tap", p_position);
        log_debug("double_tap : " + String(p_position));
        return;
    }

    // Mis en attente : _process l'émettra comme tap simple si aucun second tap n'arrive à temps.
    has_pending_tap = true;
    pending_tap_time = p_now;
    pending_tap_position = p_position;
}

void InputTouchAction::detect_swipe(const Finger &p_finger, const Vector2 &p_position, double p_now)
{
    Vector2 swipe = p_position - p_finger.start_position;
    double duration = p_now - p_finger.press_time;
    if (duration <= 0.0 || swipe.length() < swipe_min_distance)
    {
        return;
    }

    // Vitesse moyenne sur tout le geste : la vitesse du dernier drag retombe à zéro si le doigt marque un arrêt.
    Vector2 velocity = swipe / (float)duration;
    if (velocity.length() < swipe_min_velocity)
    {
        return;
    }

    SwipeDirection direction = direction_of(swipe);
    emit_signal("swipe", direction, p_finger.start_position, velocity);
    log_debug("swipe : " + itos(direction) + " " + String(velocity));
}

void InputTouchAction::refresh_pinch()
{
    if (fingers.size() < 2)
    {
        pinching = false;
        pinch_finger_a = -1;
        pinch_finger_b = -1;
        return;
    }
    if (pinching && fingers.has(pinch_finger_a) && fingers.has(pinch_finger_b))
    {
        return;
    }

    // Les deux doigts les plus anciens.
    int first = -1;
    int second = -1;
    for (const KeyValue<int, Finger> &E : fingers)
    {
        if (first < 0 || E.value.order < fingers[first].order)
        {
            second = first;
            first = E.key;
        }
        else if (second < 0 || E.value.order < fingers[second].order)
        {
            second = E.key;
        }
    }

    pinch_finger_a = first;
    pinch_finger_b = second;
    pinch_initial_distance = fingers[first].position.distance_to(fingers[second].position);
    last_pinch_scale = 1.0f;
    pinching = pinch_initial_distance > 0.0f;
}

void InputTouchAction::update_pinch()
{
    const Vector2 a = fingers[pinch_finger_a].position;
    const Vector2 b = fingers[pinch_finger_b].position;
    float scale = a.distance_to(b) / pinch_initial_distance;
    if (Math::abs(scale - last_pinch_scale) < dirty_threshold)
    {
        return;
    }
    last_pinch_scale = scale;
    emit_signal("pinch", scale, (a + b) * 0.5f);
    log_debug("pinch : " + String::num(scale, 3));
}

void InputTouchAction::reset_touches()
{
    bool had_fingers = !fingers.is_empty();
    fingers.clear();
    multi_touch_gesture = false;
    has_pending_tap = false;
    pinching = false;
    pinch_finger_a = -1;
    pinch_finger_b = -1;
    pending_pan = Vector2();
    has_pending_pan = false;
    pending_magnify = 1.0f;
    has_pending_magnify = false;
    if (had_fingers)
    {
        emit_signal("finger_count_changed", 0);
    }
    draw_debug();
}

PackedInt32Array InputTouchAction::get_finger_indices() const
{
    PackedInt32Array indices;
    for (const KeyValue<int, Finger> &E : fingers)
    {
        indices.push_back(E.key);
    }
    return indices;
}

Vector2 InputTouchAction::get_finger_position(int p_finger) const
{
    HashMap<int, Finger>::ConstIterator it = fingers.find(p_finger);
    return it != fingers.end() ? it->value.position : Vector2();
}

Vector2 InputTouchAction::get_primary_finger_position() const
{
    const Finger *primary = nullptr;
    for (const KeyValue<int, Finger> &E : fingers)
    {
        if (primary == nullptr || E.value.order < primary->order)
        {
            primary = &E.value;
        }
    }
    return primary != nullptr ? primary->position : Vector2();
}

double InputTouchAction::now_seconds() { return Time::get_singleton()->get_ticks_usec() / 1000000.0; }

InputTouchAction::SwipeDirection InputTouchAction::direction_of(const Vector2 &p_vector)
{
    if (p_vector.is_zero_approx())
    {
        return SWIPE_NONE;
    }
    if (Math::abs(p_vector.x) > Math::abs(p_vector.y))
    {
        return p_vector.x > 0.0f ? SWIPE_RIGHT : SWIPE_LEFT;
    }
    // Axe Y de l'écran orienté vers le bas.
    return p_vector.y > 0.0f ? SWIPE_DOWN : SWIPE_UP;
}

void InputTouchAction::draw_debug()
{
    if (!can_debug_draw())
    {
        return;
    }
    draw_debug_label();

    // Un cercle par doigt, à sa position réelle à l'écran (orange après un appui long), et le segment du pincement.
    int slot = 0;
    for (const KeyValue<int, Finger> &E : fingers)
    {
        Color color = E.value.long_pressed ? Color(1.0f, 0.6f, 0.2f) : Color(0.3f, 0.8f, 1.0f);
        Gizmos2D::draw_2D_circle(debug_gizmo(1 + slot), E.value.position, debug_draw_size * 0.5f, color, false, 3.0f);
        slot++;
    }
    for (; slot < MAX_TRACKED_FINGERS; slot++)
    {
        Gizmos2D::clear_2D_gizmos(debug_gizmo(1 + slot));
    }

    if (pinching)
    {
        Gizmos2D::draw_2D_line(debug_gizmo(PINCH_GIZMO), fingers[pinch_finger_a].position, fingers[pinch_finger_b].position, 2.0f, Color(1, 1, 1, 0.6f));
    }
    else
    {
        Gizmos2D::clear_2D_gizmos(debug_gizmo(PINCH_GIZMO));
    }
}

void InputTouchAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_finger_count"), &InputTouchAction::get_finger_count);
    ClassDB::bind_method(D_METHOD("get_finger_indices"), &InputTouchAction::get_finger_indices);
    ClassDB::bind_method(D_METHOD("is_finger_active", "finger"), &InputTouchAction::is_finger_active);
    ClassDB::bind_method(D_METHOD("get_finger_position", "finger"), &InputTouchAction::get_finger_position);
    ClassDB::bind_method(D_METHOD("get_primary_finger_position"), &InputTouchAction::get_primary_finger_position);
    ClassDB::bind_method(D_METHOD("get_pinch_scale"), &InputTouchAction::get_pinch_scale);

    ClassDB::bind_method(D_METHOD("set_tap_max_duration", "duration"), &InputTouchAction::set_tap_max_duration);
    ClassDB::bind_method(D_METHOD("get_tap_max_duration"), &InputTouchAction::get_tap_max_duration);
    ClassDB::bind_method(D_METHOD("set_tap_max_distance", "distance"), &InputTouchAction::set_tap_max_distance);
    ClassDB::bind_method(D_METHOD("get_tap_max_distance"), &InputTouchAction::get_tap_max_distance);
    ClassDB::bind_method(D_METHOD("set_detect_double_tap", "detect"), &InputTouchAction::set_detect_double_tap);
    ClassDB::bind_method(D_METHOD("get_detect_double_tap"), &InputTouchAction::get_detect_double_tap);
    ClassDB::bind_method(D_METHOD("set_double_tap_delay", "delay"), &InputTouchAction::set_double_tap_delay);
    ClassDB::bind_method(D_METHOD("get_double_tap_delay"), &InputTouchAction::get_double_tap_delay);
    ClassDB::bind_method(D_METHOD("set_long_press_duration", "duration"), &InputTouchAction::set_long_press_duration);
    ClassDB::bind_method(D_METHOD("get_long_press_duration"), &InputTouchAction::get_long_press_duration);
    ClassDB::bind_method(D_METHOD("set_swipe_min_distance", "distance"), &InputTouchAction::set_swipe_min_distance);
    ClassDB::bind_method(D_METHOD("get_swipe_min_distance"), &InputTouchAction::get_swipe_min_distance);
    ClassDB::bind_method(D_METHOD("set_swipe_min_velocity", "velocity"), &InputTouchAction::set_swipe_min_velocity);
    ClassDB::bind_method(D_METHOD("get_swipe_min_velocity"), &InputTouchAction::get_swipe_min_velocity);
    ClassDB::bind_method(D_METHOD("set_max_fingers", "max"), &InputTouchAction::set_max_fingers);
    ClassDB::bind_method(D_METHOD("get_max_fingers"), &InputTouchAction::get_max_fingers);

    ADD_GROUP(U"⚙️ Paramètres Tap", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tap_max_duration", PROPERTY_HINT_RANGE, "0.05,1.0,0.05,suffix:s"), "set_tap_max_duration", "get_tap_max_duration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tap_max_distance", PROPERTY_HINT_RANGE, "0,100,1,suffix:px"), "set_tap_max_distance", "get_tap_max_distance");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "detect_double_tap"), "set_detect_double_tap", "get_detect_double_tap");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "double_tap_delay", PROPERTY_HINT_RANGE, "0.1,1.0,0.05,suffix:s"), "set_double_tap_delay", "get_double_tap_delay");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "long_press_duration", PROPERTY_HINT_RANGE, "0.2,3.0,0.05,suffix:s"), "set_long_press_duration", "get_long_press_duration");

    ADD_GROUP(U"⚙️ Paramètres Swipe", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "swipe_min_distance", PROPERTY_HINT_RANGE, "10,500,10,suffix:px"), "set_swipe_min_distance", "get_swipe_min_distance");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "swipe_min_velocity", PROPERTY_HINT_RANGE, "100,5000,50,suffix:px/s"), "set_swipe_min_velocity", "get_swipe_min_velocity");

    ADD_GROUP(U"⚙️ Paramètres Multi-touch", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_fingers", PROPERTY_HINT_RANGE, "1,10,1"), "set_max_fingers", "get_max_fingers");

    ADD_SIGNAL(MethodInfo("touch_pressed", PropertyInfo(Variant::INT, "finger"), PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("touch_released", PropertyInfo(Variant::INT, "finger"), PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("drag", PropertyInfo(Variant::INT, "finger"), PropertyInfo(Variant::VECTOR2, "position"), PropertyInfo(Variant::VECTOR2, "relative"), PropertyInfo(Variant::FLOAT, "pressure")));
    ADD_SIGNAL(MethodInfo("finger_count_changed", PropertyInfo(Variant::INT, "count")));
    ADD_SIGNAL(MethodInfo("tap", PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("double_tap", PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("long_press", PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("swipe", PropertyInfo(Variant::INT, "direction", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CLASS_IS_ENUM, "InputTouchAction.SwipeDirection"),
                          PropertyInfo(Variant::VECTOR2, "start_position"), PropertyInfo(Variant::VECTOR2, "velocity")));
    ADD_SIGNAL(MethodInfo("pinch", PropertyInfo(Variant::FLOAT, "scale"), PropertyInfo(Variant::VECTOR2, "center")));
    ADD_SIGNAL(MethodInfo("pan", PropertyInfo(Variant::VECTOR2, "delta")));
    ADD_SIGNAL(MethodInfo("magnify", PropertyInfo(Variant::FLOAT, "factor")));

    BIND_ENUM_CONSTANT(SWIPE_NONE);
    BIND_ENUM_CONSTANT(SWIPE_LEFT);
    BIND_ENUM_CONSTANT(SWIPE_RIGHT);
    BIND_ENUM_CONSTANT(SWIPE_UP);
    BIND_ENUM_CONSTANT(SWIPE_DOWN);
}
