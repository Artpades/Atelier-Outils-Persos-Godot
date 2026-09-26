#include "input_mouse_action.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos_2d.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void InputMouseAction::_process(double p_delta)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    // Désactivée : les valeurs retombent à zéro (et sont émises une fois), comme les autres actions.
    Vector2 position = get_mouse_position();
    if (position.distance_squared_to(last_emitted_position) > position_threshold * position_threshold)
    {
        last_emitted_position = position;
        emit_signal("position_emitted", position);
    }

    Vector2 motion = get_accumulated_motion();
    if (motion != last_emitted_motion)
    {
        last_emitted_motion = motion;
        emit_signal("motion_emitted", motion);
        log_debug("motion_emitted : " + String(motion));
    }
    accumulated_motion = Vector2();

    float pressure = get_pressure();
    if (!Math::is_equal_approx(pressure, last_emitted_pressure))
    {
        last_emitted_pressure = pressure;
        emit_signal("pressure_changed", pressure);
    }

    draw_debug(motion);
}

void InputMouseAction::_input(const Ref<InputEvent> &p_event)
{
    Ref<InputEventMouseMotion> motion = p_event;
    if (motion.is_valid())
    {
        accumulated_motion += motion->get_relative();
        current_pressure = motion->get_pressure();
    }
}

Vector2 InputMouseAction::get_mouse_position() const
{
    Viewport *viewport = is_inside_tree() ? get_viewport() : nullptr;
    if (!action_enabled || viewport == nullptr)
    {
        return Vector2();
    }

    Vector2 position = viewport->get_mouse_position();
    if (normalize_position)
    {
        Vector2 size = viewport->get_visible_rect().size;
        if (size.x > 0.0f && size.y > 0.0f)
        {
            position /= size;
        }
    }
    return position;
}

Vector2 InputMouseAction::get_accumulated_motion() const { return action_enabled ? accumulated_motion * motion_sensitivity : Vector2(); }

float InputMouseAction::get_pressure() const { return action_enabled ? current_pressure : 0.0f; }

void InputMouseAction::draw_debug(const Vector2 &p_motion)
{
    if (!can_debug_draw())
    {
        return;
    }
    draw_debug_label();
    Gizmos2D::draw_2D_circle(debug_gizmo(1), debug_draw_position, debug_draw_size * 0.5f, Color(1, 1, 1, 0.5f), false, 2.0f);
    Gizmos2D::draw_2D_arrow(debug_gizmo(2), debug_draw_position, debug_draw_position + p_motion.limit_length(debug_draw_size * 0.5f), 8.0f, 2.0f, Color(0.3f, 0.8f, 1.0f));
}

void InputMouseAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_mouse_position"), &InputMouseAction::get_mouse_position);
    ClassDB::bind_method(D_METHOD("get_accumulated_motion"), &InputMouseAction::get_accumulated_motion);
    ClassDB::bind_method(D_METHOD("get_pressure"), &InputMouseAction::get_pressure);
    ClassDB::bind_method(D_METHOD("set_motion_sensitivity", "sensitivity"), &InputMouseAction::set_motion_sensitivity);
    ClassDB::bind_method(D_METHOD("get_motion_sensitivity"), &InputMouseAction::get_motion_sensitivity);
    ClassDB::bind_method(D_METHOD("set_position_threshold", "threshold"), &InputMouseAction::set_position_threshold);
    ClassDB::bind_method(D_METHOD("get_position_threshold"), &InputMouseAction::get_position_threshold);
    ClassDB::bind_method(D_METHOD("set_normalize_position", "normalize"), &InputMouseAction::set_normalize_position);
    ClassDB::bind_method(D_METHOD("get_normalize_position"), &InputMouseAction::get_normalize_position);

    ADD_GROUP(U"⚙️ Paramètres", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "motion_sensitivity", PROPERTY_HINT_RANGE, "0.1,5.0,0.1"), "set_motion_sensitivity", "get_motion_sensitivity");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "position_threshold", PROPERTY_HINT_RANGE, "0.0,10.0,0.5,suffix:px"), "set_position_threshold", "get_position_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "normalize_position"), "set_normalize_position", "get_normalize_position");

    ADD_SIGNAL(MethodInfo("position_emitted", PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("motion_emitted", PropertyInfo(Variant::VECTOR2, "motion")));
    ADD_SIGNAL(MethodInfo("pressure_changed", PropertyInfo(Variant::FLOAT, "pressure")));
}
