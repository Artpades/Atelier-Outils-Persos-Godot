#ifndef FX_TARGETER_HPP
#define FX_TARGETER_HPP

#include "1.🧰Ensemble d'Outils/3.✨Effets Spéciaux et Sensations🎵/effect.hpp"

#include <godot_cpp/variant/node_path.hpp>

namespace godot
{
class Node2D;
class Node3D;

/// 🌻 Oriente la cible vers le centre des nœuds visés (chemins relatifs au PlayerFX).
/// Sur une Camera3D, peut aussi régler sa distance pour garder tous les nœuds visés dans le champ.
class FXTargeter : public Effect
{
    GDCLASS(FXTargeter, Effect)

  public:
    void set_nodes_to_target(const TypedArray<NodePath> &p_value) { nodes_to_target = p_value; }
    TypedArray<NodePath> get_nodes_to_target() const { return nodes_to_target; }
    void set_add_lerp(bool p_value) { add_lerp = p_value; }
    bool get_add_lerp() const { return add_lerp; }
    void set_lerp_speed(float p_value) { lerp_speed = p_value; }
    float get_lerp_speed() const { return lerp_speed; }
    void set_enable_camera_3d_depth_control(bool p_value) { enable_camera_3d_depth_control = p_value; }
    bool get_enable_camera_3d_depth_control() const { return enable_camera_3d_depth_control; }
    void set_min_distance(float p_value) { min_distance = p_value; }
    float get_min_distance() const { return min_distance; }
    void set_max_distance(float p_value) { max_distance = p_value; }
    float get_max_distance() const { return max_distance; }
    void set_depth_margin(float p_value) { depth_margin = p_value; }
    float get_depth_margin() const { return depth_margin; }

  protected:
    static void _bind_methods();

    void _process(double p_delta, Node *p_target) override;

  private:
    TypedArray<NodePath> nodes_to_target;
    // Interpolation
    bool add_lerp = false;
    float lerp_speed = 5.0f;
    // Distance de la Camera3D
    bool enable_camera_3d_depth_control = true;
    float min_distance = 5.0f;
    float max_distance = 50.0f;
    float depth_margin = 1.5f;

    /// Oriente la cible vers le centre des nœuds visés (et place la Camera3D en 3D).
    void _process_2d(Node2D *p_node, float p_delta);
    void _process_3d(Node3D *p_node, float p_delta);
};
} // namespace godot

#endif // FX_TARGETER_HPP
