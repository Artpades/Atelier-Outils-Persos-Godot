#ifndef INPUT_TOUCH_ACTION_HPP
#define INPUT_TOUCH_ACTION_HPP

#include "input_action.hpp"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

namespace godot
{
/// 📱 Écran tactile : touchers, glissements, multi-touch et gestes (tap, double tap, appui long, swipe,
/// pincement) ; plus les gestes de pavé tactile (pan, magnify).
///
/// Le tactile est un périphérique à part : detector_type et device_id sont ignorés.
/// Pour tester à la souris sur PC : ProjectSettings → input_devices/pointing/emulate_touch_from_mouse.
class InputTouchAction : public InputAction
{
    GDCLASS(InputTouchAction, InputAction)

  public:
    enum SwipeDirection
    {
        SWIPE_NONE,
        SWIPE_LEFT,
        SWIPE_RIGHT,
        SWIPE_UP,
        SWIPE_DOWN,
    };

    void _process(double p_delta) override;
    void _input(const Ref<InputEvent> &p_event) override;

    /// Nombre de doigts actuellement suivis.
    int get_finger_count() const { return fingers.size(); }
    /// Index des doigts actuellement suivis.
    PackedInt32Array get_finger_indices() const;
    bool is_finger_active(int p_finger) const { return fingers.has(p_finger); }
    /// Position du doigt p_finger, Vector2.ZERO s'il n'est pas actif.
    Vector2 get_finger_position(int p_finger) const;
    /// Position du plus ancien doigt encore posé, Vector2.ZERO si aucun.
    Vector2 get_primary_finger_position() const;
    /// Rapport entre l'écart actuel des deux doigts du pincement et leur écart initial (1.0 hors pincement).
    float get_pinch_scale() const { return pinching ? last_pinch_scale : 1.0f; }

    void set_tap_max_duration(float p_duration) { tap_max_duration = p_duration; }
    float get_tap_max_duration() const { return tap_max_duration; }

    void set_tap_max_distance(float p_distance) { tap_max_distance = p_distance; }
    float get_tap_max_distance() const { return tap_max_distance; }

    void set_detect_double_tap(bool p_detect) { detect_double_tap = p_detect; }
    bool get_detect_double_tap() const { return detect_double_tap; }

    void set_double_tap_delay(float p_delay) { double_tap_delay = p_delay; }
    float get_double_tap_delay() const { return double_tap_delay; }

    void set_long_press_duration(float p_duration) { long_press_duration = p_duration; }
    float get_long_press_duration() const { return long_press_duration; }

    void set_swipe_min_distance(float p_distance) { swipe_min_distance = p_distance; }
    float get_swipe_min_distance() const { return swipe_min_distance; }

    void set_swipe_min_velocity(float p_velocity) { swipe_min_velocity = p_velocity; }
    float get_swipe_min_velocity() const { return swipe_min_velocity; }

    void set_max_fingers(int p_max) { max_fingers = p_max; }
    int get_max_fingers() const { return max_fingers; }

  protected:
    static void _bind_methods();

  private:
    struct Finger
    {
        Vector2 position;
        Vector2 start_position;
        double press_time = 0.0;
        /// Ordre d'arrivée, pour retrouver le doigt principal (HashMap ne garantit pas l'ordre).
        uint64_t order = 0;
        /// Sorti de tap_max_distance : ni tap ni appui long possible.
        bool moved = false;
        bool long_pressed = false;
    };

    void handle_touch(int p_finger, const Vector2 &p_position, bool p_pressed);
    void handle_drag(int p_finger, const Vector2 &p_position, const Vector2 &p_relative, float p_pressure);
    void detect_tap(const Finger &p_finger, const Vector2 &p_position, double p_now);
    void detect_swipe(const Finger &p_finger, const Vector2 &p_position, double p_now);
    /// Choisit les deux doigts du pincement (ou l'arrête) quand le nombre de doigts change.
    void refresh_pinch();
    void update_pinch();
    /// Oublie tous les doigts et gestes en cours (action désactivée).
    void reset_touches();
    void draw_debug();

    static double now_seconds();
    static SwipeDirection direction_of(const Vector2 &p_vector);

    float tap_max_duration = 0.3f;
    float tap_max_distance = 20.0f;
    bool detect_double_tap = true;
    float double_tap_delay = 0.3f;
    float long_press_duration = 0.5f;
    float swipe_min_distance = 100.0f;
    float swipe_min_velocity = 500.0f;
    int max_fingers = 5;

    HashMap<int, Finger> fingers;
    uint64_t next_finger_order = 0;
    /// Vrai dès que deux doigts ont été posés ensemble, jusqu'au dernier relâché : pas de tap ni de swipe.
    bool multi_touch_gesture = false;

    bool has_pending_tap = false;
    double pending_tap_time = 0.0;
    Vector2 pending_tap_position;

    bool pinching = false;
    int pinch_finger_a = -1;
    int pinch_finger_b = -1;
    float pinch_initial_distance = 0.0f;
    float last_pinch_scale = 1.0f;

    Vector2 pending_pan;
    bool has_pending_pan = false;
    float pending_magnify = 1.0f;
    bool has_pending_magnify = false;
};
} // namespace godot

VARIANT_ENUM_CAST(InputTouchAction::SwipeDirection);

#endif // INPUT_TOUCH_ACTION_HPP
