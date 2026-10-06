#ifndef MOVEMENT_COMPONENT_HPP
#define MOVEMENT_COMPONENT_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot
{
/// 🌀 Base des composants de mouvement (sol, vol, nage, mur, corde...), à placer sous le corps qu'ils
/// déplacent (CharacterBody2D/3D, et RigidBody2D/3D pour certains).
///
/// Tous reçoivent la même « intention » : move(Vector2), jump() / release_jump(), set_running(),
/// set_vertical_input() (monter / descendre en vol, à la nage, sur un mur). Un MovementDirector la transmet
/// à tous les composants du corps et n'active (active) que ceux du contexte courant ; sans directeur, un
/// composant est actif par défaut. Un composant inactif ne fait rien.
/// Par script : redéfinir _movement_step(delta) (appelé à chaque image physique quand le composant est actif).
class MovementComponent : public Node
{
    GDCLASS(MovementComponent, Node)

  public:
    void set_active(bool p_active);
    bool is_active() const { return active; }

    void move(const Vector2 &p_input) { move_input = p_input.limit_length(1.0f); }
    Vector2 get_move_input() const { return move_input; }
    void jump();
    void release_jump() { jump_held = false; }
    bool is_jump_held() const { return jump_held; }
    void set_running(bool p_running) { running = p_running; }
    bool is_running() const { return running; }
    void set_vertical_input(float p_value) { vertical_input = CLAMP(p_value, -1.0f, 1.0f); }
    float get_vertical_input() const { return vertical_input; }
    void set_jump_buffer_time(double p_time) { jump_buffer_time = MAX(0.0, p_time); }
    double get_jump_buffer_time() const { return jump_buffer_time; }

    /// Saut demandé il y a moins de jump_buffer_time ? (consume = l'oublier).
    bool has_jump_request(bool p_consume = false);
    Node *get_body() const { return get_parent(); }

    /// Accélération externe (zones de force, courants d'eau) subie par un corps.
    static Vector2 external_acceleration_2d(Node *p_body);
    static Vector3 external_acceleration_3d(Node *p_body);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

    /// Étape native des classes C++ (si actif).
    virtual void _physics_step(double /*p_delta*/) {}
    virtual void _on_activated() {}
    virtual void _on_deactivated() {}

    GDVIRTUAL1(_movement_step, double)

    Vector2 move_input;
    bool jump_held = false;
    bool running = false;
    float vertical_input = 0.0f;
    double jump_buffer_time = 0.12;
    double jump_buffer = 0.0;

  private:
    bool active = true;
};
} // namespace godot

#endif // MOVEMENT_COMPONENT_HPP
