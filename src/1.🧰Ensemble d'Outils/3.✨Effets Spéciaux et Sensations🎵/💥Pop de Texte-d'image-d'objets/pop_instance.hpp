#ifndef POP_INSTANCE_HPP
#define POP_INSTANCE_HPP

#include "pop_style.hpp"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot
{
/// Mouvement d'un pop, commun à la 2D et à la 3D : en pixels, l'axe Y vers le HAUT (converti en 2D).
struct PopMotion
{
    Vector3 offset;   // déplacement depuis le point d'apparition
    Vector3 velocity; // px/s
    float spin = 0.0f;  // rad/s
    float angle = 0.0f; // rad
    double age = 0.0;

    /// Mouvement de départ tiré selon le style (direction, dispersion, vitesse, rotation, rayon de départ).
    static PopMotion launch(const PopStyle &p_style, bool p_3d);
    /// Avance de p_delta secondes : gravité, freinage, sol et rebond, rotation.
    void step(const PopStyle &p_style, double p_delta);
};

/// 💥 Pop 2D (créé par un PopSpawner ou un FXPop) : contenu centré sur son origine, qui jaillit, retombe
/// et disparaît en fondu. Se libère seul à la fin de sa vie, même si son créateur a disparu.
class PopInstance2D : public Node2D
{
    GDCLASS(PopInstance2D, Node2D)

  public:
    void setup(const Ref<PopStyle> &p_style, const Vector2 &p_origin, const PopMotion &p_motion);
    double get_age() const { return motion.age; }
    Vector2 get_velocity() const { return Vector2(motion.velocity.x, -motion.velocity.y); }
    Ref<PopStyle> get_style() const { return style; }

    void _process(double p_delta) override;

  protected:
    static void _bind_methods();

  private:
    Ref<PopStyle> style;
    Vector2 origin;
    PopMotion motion;

    void _update();
};

/// 💥 Pop 3D : comme PopInstance2D, toujours tourné vers la caméra. L'opacité est appliquée aux Label3D,
/// Sprite3D (modulate) et autres GeometryInstance3D (transparency) qu'il contient.
class PopInstance3D : public Node3D
{
    GDCLASS(PopInstance3D, Node3D)

  public:
    void setup(const Ref<PopStyle> &p_style, const Vector3 &p_origin, const PopMotion &p_motion, float p_unit_scale);
    double get_age() const { return motion.age; }
    Vector3 get_velocity() const { return motion.velocity * unit_scale; }
    Ref<PopStyle> get_style() const { return style; }

    void _process(double p_delta) override;

  protected:
    static void _bind_methods();

  private:
    Ref<PopStyle> style;
    Vector3 origin;
    PopMotion motion;
    float unit_scale = 0.01f;

    void _update();
    static void _apply_alpha(Node *p_node, float p_alpha);
};
} // namespace godot

#endif // POP_INSTANCE_HPP
