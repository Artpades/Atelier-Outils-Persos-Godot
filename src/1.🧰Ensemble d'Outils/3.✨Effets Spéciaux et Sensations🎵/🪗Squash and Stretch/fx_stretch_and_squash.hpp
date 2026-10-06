#ifndef FX_STRETCH_AND_SQUASH_HPP
#define FX_STRETCH_AND_SQUASH_HPP

#include "1.🧰Ensemble d'Outils/3.✨Effets Spéciaux et Sensations🎵/effect.hpp"

#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot
{
class Control;
class Node3D;

/// 🪗 Étire la cible dans le sens de son mouvement (selon sa vitesse) et l'écrase lors d'un impact
/// (chute brutale de vitesse). Fonctionne sur un Control, un Node2D ou un Node3D.
class FXStretchAndSquash : public Effect
{
    GDCLASS(FXStretchAndSquash, Effect)

  public:
    void set_max_speed(float p_value) { max_speed = p_value; }
    float get_max_speed() const { return max_speed; }
    void set_max_stretch(float p_value) { max_stretch = p_value; }
    float get_max_stretch() const { return max_stretch; }
    void set_smooth_speed(float p_value) { smooth_speed = p_value; }
    float get_smooth_speed() const { return smooth_speed; }
    void set_orient_to_velocity(bool p_value) { orient_to_velocity = p_value; }
    bool get_orient_to_velocity() const { return orient_to_velocity; }
    void set_enable_impact_squash(bool p_value) { enable_impact_squash = p_value; }
    bool get_enable_impact_squash() const { return enable_impact_squash; }
    void set_impact_threshold(float p_value) { impact_threshold = p_value; }
    float get_impact_threshold() const { return impact_threshold; }
    void set_max_impact_squash(float p_value) { max_impact_squash = p_value; }
    float get_max_impact_squash() const { return max_impact_squash; }
    void set_impact_recovery_speed(float p_value) { impact_recovery_speed = p_value; }
    float get_impact_recovery_speed() const { return impact_recovery_speed; }

  protected:
    static void _bind_methods();

    void _play(Node *p_target) override;
    void _process(double p_delta, Node *p_target) override;
    void _stop(Node *p_target) override;

  private:
    // Étirement
    float max_speed = 600.0f;
    float max_stretch = 0.5f;
    float smooth_speed = 15.0f;
    bool orient_to_velocity = false;
    // Impact / atterrissage
    bool enable_impact_squash = true;
    float impact_threshold = 300.0f; // seuil de décélération pour déclencher l'impact
    float max_impact_squash = 0.4f;
    float impact_recovery_speed = 12.0f;

    Vector2 last_pos_2d;
    Vector2 last_velocity_2d;
    Vector2 initial_scale_2d;
    float initial_rotation_2d = 0.0f;

    Vector3 last_pos_3d;
    Vector3 last_velocity_3d;
    Vector3 initial_scale_3d;
    Basis initial_basis_3d;

    float current_impact_squash_2d = 0.0f;
    float current_impact_squash_3d = 0.0f;
    Vector2 last_impact_dir_2d;
    Vector3 last_impact_dir_3d;

    bool is_first_frame = true;

    static Vector2 _get_control_pivot_position(Control *p_control);
    /// Vitesse depuis la frame précédente (rien à la première frame), puis déformation.
    void _process_2d(Node *p_target, const Vector2 &p_current_pos, float p_delta);
    void _apply_deformation_2d(Node *p_target, const Vector2 &p_velocity, float p_delta);
    void _process_3d(Node3D *p_node, float p_delta);
};
} // namespace godot

#endif // FX_STRETCH_AND_SQUASH_HPP
