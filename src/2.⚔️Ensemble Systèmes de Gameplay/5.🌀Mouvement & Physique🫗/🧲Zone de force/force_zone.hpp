#ifndef FORCE_ZONE_HPP
#define FORCE_ZONE_HPP

#include "../🧩Commun/debug_gizmos.hpp"

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/area3d.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
/// Réglages communs aux zones de force 2D et 3D (classe C++ interne).
struct ForceZoneSettings
{
    enum Mode
    {
        MODE_DIRECTIONAL, // vent, tapis roulant, courant : direction fixe
        MODE_ATTRACT,     // vers le centre (force négative : repousse)
        MODE_VORTEX,      // tourbillon autour du centre
        MODE_DRAG,        // freine (s'oppose à la vitesse)
    };
    enum Falloff
    {
        FALLOFF_NONE,
        FALLOFF_LINEAR,         // 1 au centre → 0 à falloff_radius
        FALLOFF_INVERSE_SQUARE, // 1 / (1 + (d / falloff_radius)²)
    };

    bool enabled = true;
    Mode mode = MODE_DIRECTIONAL;
    double strength = 600.0;
    bool local_direction = true; // la direction tourne avec la zone
    Falloff falloff = FALLOFF_NONE;
    double falloff_radius = 200.0;
    double turbulence = 0.0;      // variation aléatoire douce de l'intensité (0 à 1)
    double turbulence_speed = 1.5;
    bool affect_rigid_bodies = true;
    bool affect_characters = true;

    double attenuation(double p_distance) const;
    double turbulence_factor(double p_time, double p_seed) const;
};

/// 🧲 Zone de force 2D (Area2D) : vent, aimant, tourbillon, freinage. Les RigidBody2D qui la traversent
/// reçoivent la force (masse × accélération) ; les CharacterBody2D la subissent par leurs composants de
/// mouvement (MovementComponent.external_acceleration_2d). Forme : CollisionShape2D enfant, comme toute Area2D.
class ForceZone2D : public Area2D
{
    GDCLASS(ForceZone2D, Area2D)

  public:
    ForceZone2D();

    void set_enabled(bool p_enabled) { settings.enabled = p_enabled; }
    bool is_enabled() const { return settings.enabled; }
    void set_mode(int p_mode) { settings.mode = ForceZoneSettings::Mode(p_mode); }
    int get_mode() const { return settings.mode; }
    void set_strength(double p_strength) { settings.strength = p_strength; }
    double get_strength() const { return settings.strength; }
    void set_direction(const Vector2 &p_direction) { direction = p_direction; }
    Vector2 get_direction() const { return direction; }
    void set_local_direction(bool p_local) { settings.local_direction = p_local; }
    bool get_local_direction() const { return settings.local_direction; }
    void set_falloff(int p_falloff) { settings.falloff = ForceZoneSettings::Falloff(p_falloff); }
    int get_falloff() const { return settings.falloff; }
    void set_falloff_radius(double p_radius) { settings.falloff_radius = MAX(0.001, p_radius); }
    double get_falloff_radius() const { return settings.falloff_radius; }
    void set_turbulence(double p_value) { settings.turbulence = CLAMP(p_value, 0.0, 1.0); }
    double get_turbulence() const { return settings.turbulence; }
    void set_turbulence_speed(double p_speed) { settings.turbulence_speed = MAX(0.0, p_speed); }
    double get_turbulence_speed() const { return settings.turbulence_speed; }
    void set_affect_rigid_bodies(bool p_affect) { settings.affect_rigid_bodies = p_affect; }
    bool get_affect_rigid_bodies() const { return settings.affect_rigid_bodies; }
    void set_affect_characters(bool p_affect) { settings.affect_characters = p_affect; }
    bool get_affect_characters() const { return settings.affect_characters; }

    /// Accélération qu'un corps situé en p_point (vitesse p_velocity) subit dans cette zone.
    Vector2 compute_acceleration(const Vector2 &p_point, const Vector2 &p_velocity) const;
    /// Somme des accélérations des zones (actives, qui affectent les personnages) qui contiennent p_body.
    static Vector2 get_acceleration_on(Node *p_body);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    ForceZoneSettings settings;
    Vector2 direction = Vector2(1, 0);
    double elapsed = 0.0;
    static LocalVector<ObjectID> zones;

    DEBUG_MODE_DECLARATIONS()
    void _draw_debug();
};

/// 🧲 Zone de force 3D (Area3D) : mêmes modes qu'en 2D ; le tourbillon tourne autour de l'axe Y local.
class ForceZone3D : public Area3D
{
    GDCLASS(ForceZone3D, Area3D)

  public:
    ForceZone3D();

    void set_enabled(bool p_enabled) { settings.enabled = p_enabled; }
    bool is_enabled() const { return settings.enabled; }
    void set_mode(int p_mode) { settings.mode = ForceZoneSettings::Mode(p_mode); }
    int get_mode() const { return settings.mode; }
    void set_strength(double p_strength) { settings.strength = p_strength; }
    double get_strength() const { return settings.strength; }
    void set_direction(const Vector3 &p_direction) { direction = p_direction; }
    Vector3 get_direction() const { return direction; }
    void set_local_direction(bool p_local) { settings.local_direction = p_local; }
    bool get_local_direction() const { return settings.local_direction; }
    void set_falloff(int p_falloff) { settings.falloff = ForceZoneSettings::Falloff(p_falloff); }
    int get_falloff() const { return settings.falloff; }
    void set_falloff_radius(double p_radius) { settings.falloff_radius = MAX(0.001, p_radius); }
    double get_falloff_radius() const { return settings.falloff_radius; }
    void set_turbulence(double p_value) { settings.turbulence = CLAMP(p_value, 0.0, 1.0); }
    double get_turbulence() const { return settings.turbulence; }
    void set_turbulence_speed(double p_speed) { settings.turbulence_speed = MAX(0.0, p_speed); }
    double get_turbulence_speed() const { return settings.turbulence_speed; }
    void set_affect_rigid_bodies(bool p_affect) { settings.affect_rigid_bodies = p_affect; }
    bool get_affect_rigid_bodies() const { return settings.affect_rigid_bodies; }
    void set_affect_characters(bool p_affect) { settings.affect_characters = p_affect; }
    bool get_affect_characters() const { return settings.affect_characters; }

    Vector3 compute_acceleration(const Vector3 &p_point, const Vector3 &p_velocity) const;
    static Vector3 get_acceleration_on(Node *p_body);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    ForceZoneSettings settings;
    Vector3 direction = Vector3(1, 0, 0);
    double elapsed = 0.0;
    static LocalVector<ObjectID> zones;

    DEBUG_MODE_DECLARATIONS()
    void _draw_debug();
};
} // namespace godot

#endif // FORCE_ZONE_HPP
