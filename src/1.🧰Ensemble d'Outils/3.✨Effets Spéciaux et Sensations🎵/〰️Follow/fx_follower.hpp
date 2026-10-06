#ifndef FX_FOLLOWER_HPP
#define FX_FOLLOWER_HPP

#include "1.🧰Ensemble d'Outils/3.✨Effets Spéciaux et Sensations🎵/effect.hpp"

#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot
{
class Node2D;
class Node3D;

/// 〰️ Déplace la cible au centre des nœuds suivis (chemins relatifs au PlayerFX).
/// Sur une Camera2D, peut aussi régler le zoom pour garder tous les nœuds suivis à l'écran.
class FXFollower : public Effect
{
    GDCLASS(FXFollower, Effect)

  public:
    void set_targets(const TypedArray<NodePath> &p_value) { targets = p_value; }
    TypedArray<NodePath> get_targets() const { return targets; }
    void set_use_interpolation(bool p_value) { use_interpolation = p_value; }
    bool get_use_interpolation() const { return use_interpolation; }
    void set_follow_speed(float p_value) { follow_speed = p_value; }
    float get_follow_speed() const { return follow_speed; }
    void set_auto_zoom_2d(bool p_value) { auto_zoom_2d = p_value; }
    bool get_auto_zoom_2d() const { return auto_zoom_2d; }
    void set_min_zoom(const Vector2 &p_value) { min_zoom = p_value; }
    Vector2 get_min_zoom() const { return min_zoom; }
    void set_max_zoom(const Vector2 &p_value) { max_zoom = p_value; }
    Vector2 get_max_zoom() const { return max_zoom; }
    void set_zoom_margin(float p_value) { zoom_margin = p_value; }
    float get_zoom_margin() const { return zoom_margin; }

  protected:
    static void _bind_methods();

    void _process(double p_delta, Node *p_target) override;

  private:
    TypedArray<NodePath> targets;
    bool use_interpolation = true;
    float follow_speed = 5.0f;
    // Zoom de la Camera2D
    bool auto_zoom_2d = true;
    Vector2 min_zoom = Vector2(0.5f, 0.5f);
    Vector2 max_zoom = Vector2(2.0f, 2.0f);
    float zoom_margin = 100.0f;

    void _update_2d(Node2D *p_node, float p_delta);
    void _update_3d(Node3D *p_node, float p_delta);
    float _weight(float p_delta) const;
};
} // namespace godot

#endif // FX_FOLLOWER_HPP
