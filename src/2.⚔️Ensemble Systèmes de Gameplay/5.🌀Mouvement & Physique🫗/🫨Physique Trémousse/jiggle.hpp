#ifndef JIGGLE_HPP
#define JIGGLE_HPP

#include "../🧩Commun/debug_gizmos.hpp"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>

namespace godot
{
/// Réglages communs du trémoussement (classe C++ interne).
struct JiggleSettings
{
    bool enabled = true;
    double stiffness = 140.0;      // raideur du ressort (rappel vers la position de repos)
    double damping = 7.0;          // amortissement (par seconde)
    double max_offset = 24.0;      // décalage maximal par rapport au repos
    double lean_amount = 0.02;     // inclinaison par unité de décalage (radians)
    double squash_amount = 0.0015; // étirement par unité de vitesse
    double max_stretch = 0.35;
};

#define JIGGLE_ACCESSORS()                                                                            \
    void set_enabled(bool p_enabled) { jiggle.enabled = p_enabled; }                                  \
    bool is_enabled() const { return jiggle.enabled; }                                                \
    void set_stiffness(double p_value) { jiggle.stiffness = MAX(0.0, p_value); }                      \
    double get_stiffness() const { return jiggle.stiffness; }                                         \
    void set_damping(double p_value) { jiggle.damping = MAX(0.0, p_value); }                          \
    double get_damping() const { return jiggle.damping; }                                             \
    void set_max_offset(double p_value) { jiggle.max_offset = MAX(0.0, p_value); }                    \
    double get_max_offset() const { return jiggle.max_offset; }                                       \
    void set_lean_amount(double p_value) { jiggle.lean_amount = p_value; }                            \
    double get_lean_amount() const { return jiggle.lean_amount; }                                     \
    void set_squash_amount(double p_value) { jiggle.squash_amount = MAX(0.0, p_value); }              \
    double get_squash_amount() const { return jiggle.squash_amount; }                                 \
    void set_max_stretch(double p_value) { jiggle.max_stretch = CLAMP(p_value, 0.0, 0.95); }          \
    double get_max_stretch() const { return jiggle.max_stretch; }

/// 🫨 Trémoussement 2D : ce nœud (et ses enfants visuels) suit sa position de repos avec un ressort amorti —
/// il traîne, rebondit, s'incline et s'étire quand son parent bouge (gelée, antenne, cheveux, chapeau).
/// impulse(vitesse) le secoue. gravity fait pendre la masse.
class Jiggle2D : public Node2D
{
    GDCLASS(Jiggle2D, Node2D)

  public:
    JIGGLE_ACCESSORS()
    void set_gravity(const Vector2 &p_gravity) { gravity = p_gravity; }
    Vector2 get_gravity() const { return gravity; }

    void impulse(const Vector2 &p_velocity) { simulated_velocity += p_velocity; }
    void reset();
    Vector2 get_offset() const { return offset; }

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    JiggleSettings jiggle;
    Vector2 gravity;
    /// Pose de repos capturée au _ready, puis position / vitesse de la masse simulée (globales) et décalage borné.
    Vector2 rest_position;
    double rest_rotation = 0.0;
    Vector2 rest_scale = Vector2(1, 1);
    Vector2 simulated_position;
    Vector2 simulated_velocity;
    Vector2 offset;
    bool initialized = false;

    void _step(double p_delta);

    DEBUG_MODE_DECLARATIONS()
    void _draw_debug();
};

/// 🫨 Trémoussement 3D : même principe ; l'inclinaison tourne autour de l'axe perpendiculaire au décalage.
class Jiggle3D : public Node3D
{
    GDCLASS(Jiggle3D, Node3D)

  public:
    Jiggle3D();
    JIGGLE_ACCESSORS()
    void set_gravity(const Vector3 &p_gravity) { gravity = p_gravity; }
    Vector3 get_gravity() const { return gravity; }

    void impulse(const Vector3 &p_velocity) { simulated_velocity += p_velocity; }
    void reset();
    Vector3 get_offset() const { return offset; }

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    JiggleSettings jiggle;
    Vector3 gravity;
    /// Pose de repos capturée au _ready, puis position / vitesse de la masse simulée (globales) et décalage borné.
    Transform3D rest_transform;
    Vector3 simulated_position;
    Vector3 simulated_velocity;
    Vector3 offset;
    bool initialized = false;

    void _step(double p_delta);

    DEBUG_MODE_DECLARATIONS()
    void _draw_debug();
};
} // namespace godot

#endif // JIGGLE_HPP
