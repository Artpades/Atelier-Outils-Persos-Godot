#ifndef KNOCKBACK_HPP
#define KNOCKBACK_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot
{
/// 🌬️ Recul d'un corps 2D (enfant d'un CharacterBody2D ou d'un RigidBody2D).
///
/// apply_knockback(direction, multiplicateur) / apply_knockback_from(point) :
///   - CharacterBody2D : la vitesse est remplacée par le recul, et les composants de mouvement ignorent les
///     commandes pendant duration (contrôle verrouillé, freinage par friction). Sans composant de mouvement
///     actif, le recul déplace lui-même le corps.
///   - RigidBody2D : impulsion (ou force continue pendant duration) et, si add_torque, rotation.
class Knockback2D : public Node
{
    GDCLASS(Knockback2D, Node)

  public:
    enum RigidMode
    {
        RIGID_IMPULSE,
        RIGID_CONTINUOUS,
    };

    void set_enabled(bool p_enabled) { enabled = p_enabled; }
    bool is_enabled() const { return enabled; }
    void set_base_force(double p_force) { base_force = MAX(0.0, p_force); }
    double get_base_force() const { return base_force; }
    void set_duration(double p_duration) { duration = MAX(0.0, p_duration); }
    double get_duration() const { return duration; }
    void set_friction(double p_friction) { friction = MAX(0.0, p_friction); }
    double get_friction() const { return friction; }
    void set_gravity_affect(bool p_affect) { gravity_affect = p_affect; }
    bool get_gravity_affect() const { return gravity_affect; }
    void set_upward_bias(double p_bias) { upward_bias = p_bias; }
    double get_upward_bias() const { return upward_bias; }
    void set_rigid_mode(RigidMode p_mode) { rigid_mode = p_mode; }
    RigidMode get_rigid_mode() const { return rigid_mode; }
    void set_add_torque(bool p_add) { add_torque = p_add; }
    bool get_add_torque() const { return add_torque; }
    void set_torque_factor(double p_factor) { torque_factor = p_factor; }
    double get_torque_factor() const { return torque_factor; }

    void apply_knockback(const Vector2 &p_direction, double p_multiplier = 1.0);
    /// Recul dans la direction opposée à p_point (position de l'attaquant, d'une explosion...).
    void apply_knockback_from(const Vector2 &p_point, double p_multiplier = 1.0);
    bool is_knocked() const { return lock_remaining > 0.0; }
    double get_lock_remaining() const { return lock_remaining; }

    /// Recul en cours sur p_body (cherché parmi ses enfants) ; nullptr sinon.
    static Knockback2D *active_on(Node *p_body);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    bool enabled = true;
    double base_force = 450.0;
    double duration = 0.3;
    double friction = 900.0;
    bool gravity_affect = true;
    double upward_bias = 0.35;
    RigidMode rigid_mode = RIGID_IMPULSE;
    bool add_torque = true;
    double torque_factor = 0.02;

    double lock_remaining = 0.0;
    Vector2 continuous_velocity;
};

/// 🌬️ Recul d'un corps 3D (CharacterBody3D ou RigidBody3D) : mêmes règles qu'en 2D (Y vers le haut).
class Knockback3D : public Node
{
    GDCLASS(Knockback3D, Node)

  public:
    enum RigidMode
    {
        RIGID_IMPULSE,
        RIGID_CONTINUOUS,
    };

    void set_enabled(bool p_enabled) { enabled = p_enabled; }
    bool is_enabled() const { return enabled; }
    void set_base_force(double p_force) { base_force = MAX(0.0, p_force); }
    double get_base_force() const { return base_force; }
    void set_duration(double p_duration) { duration = MAX(0.0, p_duration); }
    double get_duration() const { return duration; }
    void set_friction(double p_friction) { friction = MAX(0.0, p_friction); }
    double get_friction() const { return friction; }
    void set_gravity_affect(bool p_affect) { gravity_affect = p_affect; }
    bool get_gravity_affect() const { return gravity_affect; }
    void set_upward_bias(double p_bias) { upward_bias = p_bias; }
    double get_upward_bias() const { return upward_bias; }
    void set_rigid_mode(RigidMode p_mode) { rigid_mode = p_mode; }
    RigidMode get_rigid_mode() const { return rigid_mode; }
    void set_add_torque(bool p_add) { add_torque = p_add; }
    bool get_add_torque() const { return add_torque; }
    void set_torque_factor(double p_factor) { torque_factor = p_factor; }
    double get_torque_factor() const { return torque_factor; }

    void apply_knockback(const Vector3 &p_direction, double p_multiplier = 1.0);
    void apply_knockback_from(const Vector3 &p_point, double p_multiplier = 1.0);
    bool is_knocked() const { return lock_remaining > 0.0; }
    double get_lock_remaining() const { return lock_remaining; }

    static Knockback3D *active_on(Node *p_body);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    bool enabled = true;
    double base_force = 8.0;
    double duration = 0.3;
    double friction = 18.0;
    bool gravity_affect = true;
    double upward_bias = 0.3;
    RigidMode rigid_mode = RIGID_IMPULSE;
    bool add_torque = true;
    double torque_factor = 0.5;

    double lock_remaining = 0.0;
    Vector3 continuous_velocity;
};
} // namespace godot

VARIANT_ENUM_CAST(Knockback2D::RigidMode);
VARIANT_ENUM_CAST(Knockback3D::RigidMode);

#endif // KNOCKBACK_HPP
