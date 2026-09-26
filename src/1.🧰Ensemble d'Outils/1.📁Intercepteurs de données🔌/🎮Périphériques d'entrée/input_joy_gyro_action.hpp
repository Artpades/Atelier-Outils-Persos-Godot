#ifndef INPUT_JOY_GYRO_ACTION_HPP
#define INPUT_JOY_GYRO_ACTION_HPP

#include "input_action.hpp"

#include <godot_cpp/variant/vector3.hpp>

namespace godot
{
/// 🌀 Capteurs de mouvement d'une manette (gyroscope, et accéléromètre en option).
///
/// Les méthodes Input::get_joy_gyroscope() & co. sont apparues avec Godot 4.7, alors que les bindings
/// godot-cpp du projet sont générés depuis l'API 4.6 : elles sont donc appelées dynamiquement (Object::call).
class InputJoyGyroAction : public InputAction
{
    GDCLASS(InputJoyGyroAction, InputAction)

  public:
    void _process(double p_delta) override;

    /// Vitesse de rotation (rad/s) multipliée par gyro_sensitivity. Vector3.ZERO si désactivée.
    Vector3 get_gyroscope() const;
    /// Accélération brute (m/s²). Vector3.ZERO si désactivée.
    Vector3 get_accelerometer() const;
    /// Active ou coupe les capteurs de la manette device_id.
    void set_sensors_enabled(bool p_enabled);
    /// Vrai si la manette device_id possède des capteurs de mouvement.
    bool has_sensors() const;

    void set_auto_enable_sensors(bool p_enabled) { auto_enable_sensors = p_enabled; }
    bool get_auto_enable_sensors() const { return auto_enable_sensors; }

    void set_gyro_sensitivity(float p_sensitivity) { gyro_sensitivity = p_sensitivity; }
    float get_gyro_sensitivity() const { return gyro_sensitivity; }

    void set_track_accelerometer(bool p_track) { track_accelerometer = p_track; }
    bool get_track_accelerometer() const { return track_accelerometer; }

  protected:
    static void _bind_methods();

  private:
    void draw_debug(const Vector3 &p_gyro);

    bool auto_enable_sensors = true;
    float gyro_sensitivity = 1.0f;
    bool track_accelerometer = false;

    Vector3 last_emitted_gyro;
    Vector3 last_emitted_accel;
    bool gyro_initialized = false;
    bool accel_initialized = false;
    /// Manette pour laquelle l'absence de capteurs a déjà été signalée (évite un message par frame).
    int warned_device = -1;
};
} // namespace godot

#endif // INPUT_JOY_GYRO_ACTION_HPP
