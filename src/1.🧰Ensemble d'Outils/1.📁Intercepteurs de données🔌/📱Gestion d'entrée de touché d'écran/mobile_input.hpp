#ifndef MOBILE_INPUT_HPP
#define MOBILE_INPUT_HPP

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <map>

namespace godot
{
/// 📱 Entrées d'appareil mobile : écran tactile (toucher, glisser, multi-touch), gestes (tap, double tap,
/// appui long, swipe, pincement, pan et magnify du pavé tactile) et capteurs de mouvement (gyroscope,
/// accéléromètre, gravité, magnétomètre). Les signaux tactiles sont émis dès la réception de l'évènement ;
/// les capteurs sont lus à chaque image.
class MobileInput : public Node
{
    GDCLASS(MobileInput, Node)

  public:
    /// Direction d'un swipe : axe dominant du déplacement entre l'appui et le relâchement.
    enum SwipeDirection
    {
        SWIPE_NONE,
        SWIPE_LEFT,
        SWIPE_RIGHT,
        SWIPE_UP,
        SWIPE_DOWN,
    };

    /// Capteurs lus à chaque image (indices de `sensors` et des propriétés use_* / *_sensitivity).
    enum Sensor
    {
        SENSOR_GYROSCOPE,
        SENSOR_ACCELEROMETER,
        SENSOR_GRAVITY,
        SENSOR_MAGNETOMETER,
        SENSOR_MAX,
    };

  private:
    /// État d'un doigt posé sur l'écran.
    struct Finger
    {
        Vector2 position;
        Vector2 start_position;
        Vector2 velocity;
        double press_time = 0.0;
        /// Vrai une fois l'appui long signalé : le relâchement ne compte plus comme un tap.
        bool long_pressed = false;
        /// Vrai si un autre doigt a été posé en même temps : le relâchement ne produit ni tap ni swipe.
        bool in_multi_touch = false;
    };

    /// Réglages et dernière valeur d'un capteur.
    struct SensorState
    {
        bool enabled = true;
        float sensitivity = 1.0f;
        Vector3 value;
    };

#pragma region Réglages tactiles
    float tap_max_duration = 0.3f;
    float tap_max_distance = 20.0f;
    float double_tap_delay = 0.4f;
    float long_press_duration = 0.6f;
    float swipe_min_distance = 100.0f;
    float swipe_min_velocity = 500.0f;
    int max_fingers = 5;
#pragma endregion

    float sensor_dead_zone = 0.15f;
    SensorState sensors[SENSOR_MAX];

#pragma region État tactile
    /// Doigts actifs, triés par index (le premier est le doigt principal).
    std::map<int, Finger> fingers;

    bool has_pending_tap = false;
    double pending_tap_time = 0.0;
    Vector2 pending_tap_position;

    bool pinching = false;
    float initial_pinch_distance = 0.0f;
    float last_pinch_distance = 0.0f;
#pragma endregion

    /// Horloge réelle en secondes (indépendante de la pause et de l'échelle de temps).
    static double _now();

    void _handle_screen_touch(const Ref<InputEventScreenTouch> &p_event);
    void _handle_screen_drag(const Ref<InputEventScreenDrag> &p_event);
    void _on_finger_count_changed();
    void _detect_tap(const Finger &p_finger, const Vector2 &p_position, double p_now);
    void _detect_swipe(const Finger &p_finger);
    void _update_pinch();
    void _update_long_press(double p_now);
    void _flush_pending_tap(double p_now);

    void _update_sensor(Sensor p_sensor, const Vector3 &p_raw);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  public:
    MobileInput();

    void _input(const Ref<InputEvent> &p_event) override;
    void _process(double p_delta) override;

    /// Traite un évènement d'entrée (tactile, pan, magnify) comme s'il venait de _input.
    void handle_input_event(const Ref<InputEvent> &p_event);

#pragma region Getter / Setter
    void set_tap_max_duration(float p_value) { tap_max_duration = p_value; }
    float get_tap_max_duration() const { return tap_max_duration; }
    void set_tap_max_distance(float p_value) { tap_max_distance = p_value; }
    float get_tap_max_distance() const { return tap_max_distance; }
    void set_double_tap_delay(float p_value) { double_tap_delay = p_value; }
    float get_double_tap_delay() const { return double_tap_delay; }
    void set_long_press_duration(float p_value) { long_press_duration = p_value; }
    float get_long_press_duration() const { return long_press_duration; }
    void set_swipe_min_distance(float p_value) { swipe_min_distance = p_value; }
    float get_swipe_min_distance() const { return swipe_min_distance; }
    void set_swipe_min_velocity(float p_value) { swipe_min_velocity = p_value; }
    float get_swipe_min_velocity() const { return swipe_min_velocity; }
    void set_max_fingers(int p_value) { max_fingers = p_value; }
    int get_max_fingers() const { return max_fingers; }

    void set_sensor_dead_zone(float p_value) { sensor_dead_zone = p_value; }
    float get_sensor_dead_zone() const { return sensor_dead_zone; }

    void set_sensor_enabled(Sensor p_sensor, bool p_enabled);
    bool is_sensor_enabled(Sensor p_sensor) const;
    void set_sensor_sensitivity(Sensor p_sensor, float p_sensitivity);
    float get_sensor_sensitivity(Sensor p_sensor) const;
#pragma endregion

#pragma region Lecture
    int get_finger_count() const { return (int)fingers.size(); }
    bool has_finger(int p_index) const { return fingers.count(p_index) > 0; }
    /// Position du doigt p_index, ou null s'il n'est pas posé.
    Variant get_finger_position(int p_index) const;
    /// Position du doigt actif de plus petit index, ou null si aucun doigt n'est posé.
    Variant get_primary_finger_position() const;
    /// { index: { position, start_position, velocity, press_time } } pour chaque doigt actif.
    Dictionary get_fingers() const;
    bool is_pinching() const { return pinching; }

    /// Dernière valeur du capteur (sensibilité et zone morte appliquées).
    Vector3 get_sensor_value(Sensor p_sensor) const;
    Vector3 get_gyroscope() const { return sensors[SENSOR_GYROSCOPE].value; }
    Vector3 get_accelerometer() const { return sensors[SENSOR_ACCELEROMETER].value; }
    Vector3 get_gravity() const { return sensors[SENSOR_GRAVITY].value; }
    Vector3 get_magnetometer() const { return sensors[SENSOR_MAGNETOMETER].value; }

    /// Oublie les doigts posés et les gestes en cours (ex. à la perte du focus de la fenêtre).
    void reset_touches();
#pragma endregion
};
} // namespace godot

VARIANT_ENUM_CAST(MobileInput::SwipeDirection);
VARIANT_ENUM_CAST(MobileInput::Sensor);

#endif // MOBILE_INPUT_HPP
