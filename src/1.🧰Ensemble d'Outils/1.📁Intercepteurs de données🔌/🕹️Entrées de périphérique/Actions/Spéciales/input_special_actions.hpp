#ifndef INPUT_SPECIAL_ACTIONS_HPP
#define INPUT_SPECIAL_ACTIONS_HPP

#include "../input_action.hpp"

namespace godot
{
/// 🖱️ Action souris / stylet : position dans le viewport (normalisée de 0 à 1 si normalize_position),
/// mouvement relatif par image (× motion_sensitivity), pression du stylet, molette.
/// Évènements : position_changed(position), motion(relative), pressure_changed(pressure), scrolled(delta).
/// Valeur (get_value) : le mouvement de la dernière image.
class InputMouseAction : public InputAction
{
    GDCLASS(InputMouseAction, InputAction)

  public:
    void set_motion_sensitivity(double p_sensitivity) { motion_sensitivity = p_sensitivity; }
    double get_motion_sensitivity() const { return motion_sensitivity; }
    void set_position_threshold(double p_threshold) { position_threshold = MAX(0.0, p_threshold); }
    double get_position_threshold() const { return position_threshold; }
    void set_normalize_position(bool p_normalize) { normalize_position = p_normalize; }
    bool get_normalize_position() const { return normalize_position; }
    void set_invert_y(bool p_invert) { invert_y = p_invert; }
    bool get_invert_y() const { return invert_y; }

    Vector2 get_mouse_position() const { return position; }
    Vector2 get_motion() const { return motion; }
    double get_pressure() const { return pressure; }

    Variant get_value() const override { return motion; }
    bool is_active() const override { return motion != Vector2(); }

  protected:
    static void _bind_methods();
    void _update(double p_delta) override;
    void _handle_event(const Ref<InputEvent> &p_event) override;
    void _release() override;
    void _apply_simulation(const Variant &p_value, double p_delta) override;

  private:
    double motion_sensitivity = 1.0;
    double position_threshold = 0.5;
    bool normalize_position = false;
    bool invert_y = false;

    Vector2 position;
    Vector2 last_emitted_position;
    bool position_initialized = false;
    Vector2 accumulated_motion;
    Vector2 motion;
    Vector2 accumulated_scroll;
    double pressure = 0.0;
    double current_pressure = 0.0;

    void _emit_motion(const Vector2 &p_motion);
};

/// 🧭 Action gyroscope / accéléromètre de manette (capteurs de mouvement, Godot 4.7+). Les capteurs sont
/// activés automatiquement (auto_enable_sensors). Sans capteurs, la valeur reste nulle.
/// Évènements : gyroscope_changed(rotation_rate) (rad/s × sensitivity), accelerometer_changed(acceleration).
class InputGyroAction : public InputAction
{
    GDCLASS(InputGyroAction, InputAction)

  public:
    void set_auto_enable_sensors(bool p_enable) { auto_enable_sensors = p_enable; }
    bool get_auto_enable_sensors() const { return auto_enable_sensors; }
    void set_sensitivity(double p_sensitivity) { sensitivity = p_sensitivity; }
    double get_sensitivity() const { return sensitivity; }
    void set_track_accelerometer(bool p_track) { track_accelerometer = p_track; }
    bool get_track_accelerometer() const { return track_accelerometer; }

    Vector3 get_gyroscope() const { return gyroscope; }
    Vector3 get_accelerometer() const { return accelerometer; }
    /// Vrai si le moteur et la manette fournissent des capteurs de mouvement.
    bool has_sensors() const;
    void set_sensors_enabled(bool p_enabled);

    Variant get_value() const override { return gyroscope; }
    bool is_active() const override { return gyroscope.length_squared() > 1e-6f; }

  protected:
    static void _bind_methods();
    void _update(double p_delta) override;
    void _release() override;
    void _apply_simulation(const Variant &p_value, double p_delta) override;

  private:
    bool auto_enable_sensors = true;
    double sensitivity = 1.0;
    bool track_accelerometer = false;

    Vector3 gyroscope;
    Vector3 accelerometer;
    Vector3 last_emitted_gyroscope;
    Vector3 last_emitted_accelerometer;

    void _process(const Vector3 &p_gyroscope, const Vector3 &p_accelerometer);
};
} // namespace godot

#endif // INPUT_SPECIAL_ACTIONS_HPP
