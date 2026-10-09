#ifndef AERODYNAMIC_MOVEMENT_HPP
#define AERODYNAMIC_MOVEMENT_HPP

#include "../🧩Commun/movement_component.hpp"

namespace godot
{
/// Réglages communs de l'aérodynamique (classe C++ interne).
struct AeroSettings
{
    int mode = 0; // 0 : plané, 1 : vol libre
    double glide_fall_speed = 70.0;
    double glide_speed = 260.0;
    double glide_acceleration = 500.0;
    double glide_gravity_scale = 0.35;
    double fly_speed = 260.0;
    double fly_acceleration = 900.0;
    double fly_drag = 1.5;
    double wind_influence = 2.0;
    int max_flaps = 0;
    double flap_velocity = 250.0;
    // Corps rigides (passif)
    double drag_coefficient = 0.002;
    double lift_coefficient = 0.0;
    double flutter_frequency = 0.0;

    /// Battements restants (rechargés seulement par _on_activated) et temps écoulé (oscillation de la portance).
    int flaps_left = 0;
    double time = 0.0;
};

#define AERO_ACCESSORS()                                                                                            \
    void set_mode(int p_mode) { aero.mode = CLAMP(p_mode, 0, 1); }                                                  \
    int get_mode() const { return aero.mode; }                                                                      \
    void set_glide_fall_speed(double p_value) { aero.glide_fall_speed = MAX(0.0, p_value); }                        \
    double get_glide_fall_speed() const { return aero.glide_fall_speed; }                                           \
    void set_glide_speed(double p_value) { aero.glide_speed = MAX(0.0, p_value); }                                  \
    double get_glide_speed() const { return aero.glide_speed; }                                                     \
    void set_glide_acceleration(double p_value) { aero.glide_acceleration = MAX(0.0, p_value); }                    \
    double get_glide_acceleration() const { return aero.glide_acceleration; }                                       \
    void set_glide_gravity_scale(double p_value) { aero.glide_gravity_scale = p_value; }                            \
    double get_glide_gravity_scale() const { return aero.glide_gravity_scale; }                                     \
    void set_fly_speed(double p_value) { aero.fly_speed = MAX(0.0, p_value); }                                      \
    double get_fly_speed() const { return aero.fly_speed; }                                                         \
    void set_fly_acceleration(double p_value) { aero.fly_acceleration = MAX(0.0, p_value); }                        \
    double get_fly_acceleration() const { return aero.fly_acceleration; }                                           \
    void set_fly_drag(double p_value) { aero.fly_drag = MAX(0.0, p_value); }                                        \
    double get_fly_drag() const { return aero.fly_drag; }                                                           \
    void set_wind_influence(double p_value) { aero.wind_influence = p_value; }                                      \
    double get_wind_influence() const { return aero.wind_influence; }                                               \
    void set_max_flaps(int p_value) { aero.max_flaps = MAX(0, p_value); }                                           \
    int get_max_flaps() const { return aero.max_flaps; }                                                            \
    void set_flap_velocity(double p_value) { aero.flap_velocity = MAX(0.0, p_value); }                              \
    double get_flap_velocity() const { return aero.flap_velocity; }                                                 \
    void set_drag_coefficient(double p_value) { aero.drag_coefficient = MAX(0.0, p_value); }                        \
    double get_drag_coefficient() const { return aero.drag_coefficient; }                                           \
    void set_lift_coefficient(double p_value) { aero.lift_coefficient = p_value; }                                  \
    double get_lift_coefficient() const { return aero.lift_coefficient; }                                           \
    void set_flutter_frequency(double p_value) { aero.flutter_frequency = MAX(0.0, p_value); }                      \
    double get_flutter_frequency() const { return aero.flutter_frequency; }                                         \
    int get_flaps_left() const { return aero.flaps_left; }

/// 🪂 Physique aérodynamique 2D.
///   - CharacterBody2D : plané (chute plafonnée à glide_fall_speed, gravité réduite, battements d'ailes avec
///     le saut si max_flaps > 0) ou vol libre (move dans toutes les directions, freinage).
///     Le vent des ForceZone2D compte wind_influence fois plus.
///   - RigidBody2D (passif, même inactif) : traînée quadratique et portance perpendiculaire à la vitesse,
///     qui oscille si flutter_frequency > 0 (feuille, papier).
class AerodynamicMovement2D : public MovementComponent
{
    GDCLASS(AerodynamicMovement2D, MovementComponent)

  public:
    /// Plané (gravité réduite, battements) ou vol libre (sans gravité).
    enum Mode
    {
        MODE_GLIDE,
        MODE_FLY,
    };

    AERO_ACCESSORS()

  protected:
    static void _bind_methods();
    /// Corps rigide : traînée et portance appliquées à chaque image physique, même composant inactif.
    void _notification(int p_what);
    void _physics_step(double p_delta) override;
    String _debug_text() const override;
    void _draw_debug(DebugGizmos &p_debug) override;
    void _on_activated() override { aero.flaps_left = aero.max_flaps; }

  private:
    AeroSettings aero;
};

/// 🪂 Physique aérodynamique 3D : mêmes modes ; en vol, set_vertical_input (ou saut maintenu) fait monter.
class AerodynamicMovement3D : public MovementComponent
{
    GDCLASS(AerodynamicMovement3D, MovementComponent)

  public:
    /// Plané (gravité réduite, battements) ou vol libre (sans gravité).
    enum Mode
    {
        MODE_GLIDE,
        MODE_FLY,
    };

    AerodynamicMovement3D();
    AERO_ACCESSORS()
    void set_camera_relative(bool p_relative) { camera_relative = p_relative; }
    bool get_camera_relative() const { return camera_relative; }

  protected:
    static void _bind_methods();
    /// Corps rigide : traînée et portance appliquées à chaque image physique, même composant inactif.
    void _notification(int p_what);
    void _physics_step(double p_delta) override;
    String _debug_text() const override;
    void _draw_debug(DebugGizmos &p_debug) override;
    void _on_activated() override { aero.flaps_left = aero.max_flaps; }

  private:
    AeroSettings aero;
    bool camera_relative = false;
};
} // namespace godot

#endif // AERODYNAMIC_MOVEMENT_HPP
