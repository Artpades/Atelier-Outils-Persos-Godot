#ifndef WALL_MOVEMENT_HPP
#define WALL_MOVEMENT_HPP

#include "../🧩Commun/movement_component.hpp"

namespace godot
{
/// Réglages communs aux déplacements muraux (classe C++ interne).
struct WallSettings
{
    double slide_speed = 110.0;       // vitesse de chute maximale en glissade
    bool require_input_toward_wall = true;
    double jump_away_velocity = 320.0;
    double jump_up_velocity = 420.0;
    bool can_climb = false;
    double climb_speed = 120.0;
    double air_speed = 200.0;          // contrôle horizontal hors du mur
    double air_acceleration = 900.0;
    double stick_velocity = 40.0;      // légère poussée vers le mur pour rester en contact
};

#define WALL_ACCESSORS()                                                                                        \
    void set_slide_speed(double p_value) { wall.slide_speed = MAX(0.0, p_value); }                              \
    double get_slide_speed() const { return wall.slide_speed; }                                                 \
    void set_require_input_toward_wall(bool p_value) { wall.require_input_toward_wall = p_value; }              \
    bool get_require_input_toward_wall() const { return wall.require_input_toward_wall; }                       \
    void set_jump_away_velocity(double p_value) { wall.jump_away_velocity = MAX(0.0, p_value); }                \
    double get_jump_away_velocity() const { return wall.jump_away_velocity; }                                   \
    void set_jump_up_velocity(double p_value) { wall.jump_up_velocity = MAX(0.0, p_value); }                    \
    double get_jump_up_velocity() const { return wall.jump_up_velocity; }                                       \
    void set_can_climb(bool p_value) { wall.can_climb = p_value; }                                              \
    bool get_can_climb() const { return wall.can_climb; }                                                       \
    void set_climb_speed(double p_value) { wall.climb_speed = MAX(0.0, p_value); }                              \
    double get_climb_speed() const { return wall.climb_speed; }                                                 \
    void set_air_speed(double p_value) { wall.air_speed = MAX(0.0, p_value); }                                  \
    double get_air_speed() const { return wall.air_speed; }                                                     \
    void set_air_acceleration(double p_value) { wall.air_acceleration = MAX(0.0, p_value); }                    \
    double get_air_acceleration() const { return wall.air_acceleration; }                                       \
    void set_stick_velocity(double p_value) { wall.stick_velocity = MAX(0.0, p_value); }                        \
    double get_stick_velocity() const { return wall.stick_velocity; }

/// 🧗 Déplacement mural 2D (CharacterBody2D, vue de côté) : glissade ralentie le long d'un mur (en poussant
/// vers lui si require_input_toward_wall), saut mural (loin du mur et vers le haut), escalade (can_climb :
/// move.y monte / descend sans gravité).
class WallMovement2D : public MovementComponent
{
    GDCLASS(WallMovement2D, MovementComponent)

  public:
    WALL_ACCESSORS()
    bool is_touching_wall() const;
    bool is_sliding() const { return sliding; }
    bool is_climbing() const { return climbing; }

  protected:
    static void _bind_methods();
    void _physics_step(double p_delta) override;
    String _debug_text() const override;
    void _draw_debug(DebugGizmos &p_debug) override;

  private:
    WallSettings wall;
    /// État calculé à la dernière image physique (is_sliding / is_climbing).
    bool sliding = false;
    bool climbing = false;
    /// Normale du dernier mur touché (sens du saut mural).
    Vector2 last_normal;
};

/// 🧗 Déplacement mural 3D (CharacterBody3D) : glissade, saut mural selon la normale du mur, escalade
/// (set_vertical_input ou move.y vers l'avant).
class WallMovement3D : public MovementComponent
{
    GDCLASS(WallMovement3D, MovementComponent)

  public:
    WallMovement3D();

    WALL_ACCESSORS()
    bool is_touching_wall() const;
    bool is_sliding() const { return sliding; }
    bool is_climbing() const { return climbing; }

  protected:
    static void _bind_methods();
    void _physics_step(double p_delta) override;
    String _debug_text() const override;
    void _draw_debug(DebugGizmos &p_debug) override;

  private:
    WallSettings wall;
    /// État calculé à la dernière image physique (is_sliding / is_climbing).
    bool sliding = false;
    bool climbing = false;
};
} // namespace godot

#endif // WALL_MOVEMENT_HPP
