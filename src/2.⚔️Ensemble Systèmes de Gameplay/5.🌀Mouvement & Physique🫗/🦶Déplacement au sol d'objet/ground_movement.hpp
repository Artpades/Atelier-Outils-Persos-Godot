#ifndef GROUND_MOVEMENT_HPP
#define GROUND_MOVEMENT_HPP

#include "../🧩Commun/movement_component.hpp"

#include <godot_cpp/variant/node_path.hpp>

namespace godot
{
/// Réglages communs aux déplacements au sol 2D et 3D (classe C++ interne).
struct GroundSettings
{
    double max_speed = 200.0;
    double run_multiplier = 1.6;
    double acceleration = 1500.0;
    double deceleration = 2000.0;
    double air_control = 0.6;
    double jump_velocity = 420.0;
    double gravity_scale = 1.0;
    double max_fall_speed = 1500.0;
    int max_jumps = 1;
    double coyote_time = 0.1;
    double jump_cut = 0.5;

    // État
    double coyote = 0.0;
    int jumps_used = 0;
    bool jump_cut_done = true;
    bool was_on_floor = false;

    /// Saut possible (sol, temps de grâce ou saut en l'air restant) : met à jour le compteur.
    bool try_jump(bool p_on_floor);
};

/// Accesseurs des réglages communs (dans GroundMovement2D / GroundMovement3D).
#define GROUND_ACCESSORS()                                                            \
    void set_max_speed(double p_value) { ground.max_speed = MAX(0.0, p_value); } \
    double get_max_speed() const { return ground.max_speed; } \
    void set_run_multiplier(double p_value) { ground.run_multiplier = MAX(0.0, p_value); } \
    double get_run_multiplier() const { return ground.run_multiplier; } \
    void set_acceleration(double p_value) { ground.acceleration = MAX(0.0, p_value); } \
    double get_acceleration() const { return ground.acceleration; } \
    void set_deceleration(double p_value) { ground.deceleration = MAX(0.0, p_value); } \
    double get_deceleration() const { return ground.deceleration; } \
    void set_air_control(double p_value) { ground.air_control = CLAMP(p_value, 0.0, 1.0); } \
    double get_air_control() const { return ground.air_control; } \
    void set_jump_velocity(double p_value) { ground.jump_velocity = MAX(0.0, p_value); } \
    double get_jump_velocity() const { return ground.jump_velocity; } \
    void set_gravity_scale(double p_value) { ground.gravity_scale = p_value; } \
    double get_gravity_scale() const { return ground.gravity_scale; } \
    void set_max_fall_speed(double p_value) { ground.max_fall_speed = MAX(0.0, p_value); } \
    double get_max_fall_speed() const { return ground.max_fall_speed; } \
    void set_coyote_time(double p_value) { ground.coyote_time = MAX(0.0, p_value); } \
    double get_coyote_time() const { return ground.coyote_time; } \
    void set_jump_cut(double p_value) { ground.jump_cut = CLAMP(p_value, 0.0, 1.0); } \
    double get_jump_cut() const { return ground.jump_cut; } \
    void set_max_jumps(int p_value) { ground.max_jumps = MAX(0, p_value); } \
    int get_max_jumps() const { return ground.max_jumps; }

/// 🦶 Déplacement au sol 2D : plateforme (gravité, sauts multiples, temps de grâce, hauteur variable) ou vue
/// de dessus. Corps : CharacterBody2D (complet) ou RigidBody2D (vitesse pilotée). Le visuel (visual_path) se
/// retourne selon la direction (flip_h ou scale.x). Les zones de force (ForceZone2D) et le recul
/// (Knockback2D, commandes verrouillées) sont pris en compte.
class GroundMovement2D : public MovementComponent
{
    GDCLASS(GroundMovement2D, MovementComponent)

  public:
    enum Mode
    {
        MODE_PLATFORMER,
        MODE_TOP_DOWN,
    };

    GROUND_ACCESSORS()
    void set_mode(Mode p_mode) { mode = p_mode; }
    Mode get_mode() const { return mode; }
    void set_visual_path(const NodePath &p_path) { visual_path = p_path; }
    NodePath get_visual_path() const { return visual_path; }
    Vector2 get_facing() const { return facing; }
    bool is_on_floor() const;

  protected:
    static void _bind_methods();
    void _physics_step(double p_delta) override;
    String _debug_text() const override;
    void _draw_debug(DebugGizmos &p_debug) override;

    GroundSettings ground;

  private:
    Mode mode = MODE_PLATFORMER;
    NodePath visual_path;
    Vector2 facing = Vector2(1, 0);

    void _update_visual();
};

/// 🦶 Déplacement au sol 3D (CharacterBody3D ou RigidBody3D) : gravité et sauts selon Y, entrée dans le plan
/// XZ — relative à la caméra courante si camera_relative (joueur), sinon en coordonnées du monde (IA).
/// Le visuel (visual_path, sinon le corps) se tourne vers la marche.
class GroundMovement3D : public MovementComponent
{
    GDCLASS(GroundMovement3D, MovementComponent)

  public:
    GroundMovement3D();

    GROUND_ACCESSORS()
    void set_camera_relative(bool p_relative) { camera_relative = p_relative; }
    bool get_camera_relative() const { return camera_relative; }
    void set_rotate_toward_movement(bool p_rotate) { rotate_toward_movement = p_rotate; }
    bool get_rotate_toward_movement() const { return rotate_toward_movement; }
    void set_turn_speed(double p_speed) { turn_speed = MAX(0.0, p_speed); }
    double get_turn_speed() const { return turn_speed; }
    void set_visual_path(const NodePath &p_path) { visual_path = p_path; }
    NodePath get_visual_path() const { return visual_path; }
    /// Direction de marche dans le monde (x, z).
    Vector2 get_facing() const { return facing; }
    /// Entrée convertie en direction du monde (x, z) selon camera_relative.
    Vector2 get_world_input() const;
    bool is_on_floor() const;

  protected:
    static void _bind_methods();
    void _physics_step(double p_delta) override;
    String _debug_text() const override;
    void _draw_debug(DebugGizmos &p_debug) override;

    GroundSettings ground;

  private:
    bool camera_relative = false;
    bool rotate_toward_movement = true;
    double turn_speed = 12.0;
    NodePath visual_path;
    Vector2 facing = Vector2(0, -1);
};
} // namespace godot

VARIANT_ENUM_CAST(GroundMovement2D::Mode);

#endif // GROUND_MOVEMENT_HPP
