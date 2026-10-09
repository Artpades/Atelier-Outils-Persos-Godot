#ifndef ROPE_HPP
#define ROPE_HPP

#include "../🧩Commun/movement_component.hpp"

#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
/// 🪢 Corde 2D simulée (intégration de Verlet + contraintes de longueur) entre ce nœud (ou start_anchor_path)
/// et, facultativement, un nœud d'arrivée (end_anchor_path). Collisions facultatives avec le monde physique.
/// Des corps peuvent s'y accrocher (attach_body) : ils suivent la corde à une position donnée (index
/// fractionnaire de point) et l'alourdissent. Dessinée par _draw (width, color).
class Rope2D : public Node2D
{
    GDCLASS(Rope2D, Node2D)

  public:
    void set_point_count(int p_count);
    int get_point_count() const { return point_count; }
    void set_rope_length(double p_length) { rope_length = MAX(1.0, p_length); }
    double get_rope_length() const { return rope_length; }
    void set_gravity(const Vector2 &p_gravity) { gravity = p_gravity; }
    Vector2 get_gravity() const { return gravity; }
    void set_damping(double p_damping) { damping = CLAMP(p_damping, 0.0, 1.0); }
    double get_damping() const { return damping; }
    void set_iterations(int p_iterations) { iterations = CLAMP(p_iterations, 1, 100); }
    int get_iterations() const { return iterations; }
    void set_start_anchor_path(const NodePath &p_path) { start_anchor_path = p_path; }
    NodePath get_start_anchor_path() const { return start_anchor_path; }
    void set_end_anchor_path(const NodePath &p_path) { end_anchor_path = p_path; }
    NodePath get_end_anchor_path() const { return end_anchor_path; }
    void set_collide(bool p_collide) { collide = p_collide; }
    bool get_collide() const { return collide; }
    void set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }
    uint32_t get_collision_mask() const { return collision_mask; }
    void set_attached_weight(double p_weight) { attached_weight = MAX(0.0, p_weight); }
    double get_attached_weight() const { return attached_weight; }
    void set_width(double p_width) { width = MAX(0.0, p_width); queue_redraw(); }
    double get_width() const { return width; }
    void set_color(const Color &p_color) { color = p_color; queue_redraw(); }
    Color get_color() const { return color; }

    void reset_points();
    PackedVector2Array get_points() const;
    double get_segment_length() const { return rope_length / MAX(1, point_count - 1); }
    /// Position (globale) et vitesse le long de la corde, p_position étant un index fractionnaire de point.
    Vector2 get_position_at(double p_position) const;
    Vector2 get_velocity_at(double p_position) const;
    /// Index fractionnaire du point de la corde le plus proche de p_point (global).
    double get_closest_position(const Vector2 &p_point) const;
    /// Ajoute une vitesse aux points autour de p_position.
    void apply_impulse_at(double p_position, const Vector2 &p_velocity);

    void attach_body(Node *p_body, double p_position);
    void detach_body(Node *p_body);
    bool has_body(Node *p_body) const;
    double get_attached_position(Node *p_body) const;
    void set_attached_position(Node *p_body, double p_position);

    static Rope2D *find_rope_of(Node *p_body);
    /// Corde la plus proche de p_point à moins de p_max_distance ; r_position reçoit la position sur la corde.
    static Rope2D *find_nearest(const Vector2 &p_point, double p_max_distance, double &r_position);
    static Variant find_nearest_info(const Vector2 &p_point, double p_max_distance);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    struct Attachment
    {
        ObjectID body;
        double position = 0.0;
    };

    int point_count = 16;
    double rope_length = 200.0;
    Vector2 gravity = Vector2(0, 980);
    double damping = 0.01;
    int iterations = 12;
    NodePath start_anchor_path;
    NodePath end_anchor_path;
    bool collide = false;
    uint32_t collision_mask = 1;
    double attached_weight = 1.5;
    double width = 3.0;
    Color color = Color(0.8, 0.65, 0.4);

    /// Positions courantes et précédentes (Verlet) des points, corps accrochés, dernier pas de temps (vitesses).
    LocalVector<Vector2> points;
    LocalVector<Vector2> previous;
    LocalVector<Attachment> attachments;
    double last_delta = 1.0 / 60.0;
    /// Registre des cordes dans l'arbre (en jeu), parcouru par find_rope_of / find_nearest.
    static LocalVector<ObjectID> ropes;

    Vector2 _start_position() const;
    Node2D *_end_anchor() const;
    /// Une image : Verlet + poids des accroches, ancrages, contraintes de longueur, collisions, puis déplacement des corps accrochés.
    void _simulate(double p_delta);

    DEBUG_MODE_DECLARATIONS()
    void _draw_debug();
};

/// 🪢 Corde 3D : même simulation qu'en 2D, rendue en ligne (ImmediateMesh) de couleur color.
class Rope3D : public Node3D
{
    GDCLASS(Rope3D, Node3D)

  public:
    Rope3D();

    void set_point_count(int p_count);
    int get_point_count() const { return point_count; }
    void set_rope_length(double p_length) { rope_length = MAX(0.01, p_length); }
    double get_rope_length() const { return rope_length; }
    void set_gravity(const Vector3 &p_gravity) { gravity = p_gravity; }
    Vector3 get_gravity() const { return gravity; }
    void set_damping(double p_damping) { damping = CLAMP(p_damping, 0.0, 1.0); }
    double get_damping() const { return damping; }
    void set_iterations(int p_iterations) { iterations = CLAMP(p_iterations, 1, 100); }
    int get_iterations() const { return iterations; }
    void set_start_anchor_path(const NodePath &p_path) { start_anchor_path = p_path; }
    NodePath get_start_anchor_path() const { return start_anchor_path; }
    void set_end_anchor_path(const NodePath &p_path) { end_anchor_path = p_path; }
    NodePath get_end_anchor_path() const { return end_anchor_path; }
    void set_collide(bool p_collide) { collide = p_collide; }
    bool get_collide() const { return collide; }
    void set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }
    uint32_t get_collision_mask() const { return collision_mask; }
    void set_attached_weight(double p_weight) { attached_weight = MAX(0.0, p_weight); }
    double get_attached_weight() const { return attached_weight; }
    void set_color(const Color &p_color);
    Color get_color() const { return color; }

    void reset_points();
    PackedVector3Array get_points() const;
    double get_segment_length() const { return rope_length / MAX(1, point_count - 1); }
    Vector3 get_position_at(double p_position) const;
    Vector3 get_velocity_at(double p_position) const;
    double get_closest_position(const Vector3 &p_point) const;
    void apply_impulse_at(double p_position, const Vector3 &p_velocity);

    void attach_body(Node *p_body, double p_position);
    void detach_body(Node *p_body);
    bool has_body(Node *p_body) const;
    double get_attached_position(Node *p_body) const;
    void set_attached_position(Node *p_body, double p_position);

    static Rope3D *find_rope_of(Node *p_body);
    static Rope3D *find_nearest(const Vector3 &p_point, double p_max_distance, double &r_position);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    struct Attachment
    {
        ObjectID body;
        double position = 0.0;
    };

    int point_count = 16;
    double rope_length = 4.0;
    Vector3 gravity = Vector3(0, -9.8, 0);
    double damping = 0.01;
    int iterations = 12;
    NodePath start_anchor_path;
    NodePath end_anchor_path;
    bool collide = false;
    uint32_t collision_mask = 1;
    double attached_weight = 1.5;
    Color color = Color(0.8, 0.65, 0.4);

    /// Positions courantes et précédentes (Verlet) des points, corps accrochés, dernier pas de temps (vitesses).
    LocalVector<Vector3> points;
    LocalVector<Vector3> previous;
    LocalVector<Attachment> attachments;
    double last_delta = 1.0 / 60.0;
    MeshInstance3D *mesh_instance = nullptr; // enfant interne possédé
    Ref<ImmediateMesh> mesh;
    Ref<StandardMaterial3D> material;
    /// Registre des cordes dans l'arbre (en jeu), parcouru par find_rope_of / find_nearest.
    static LocalVector<ObjectID> ropes;

    Vector3 _start_position() const;
    Node3D *_end_anchor() const;
    /// Une image : Verlet + poids des accroches, ancrages, contraintes de longueur, collisions, puis déplacement des corps accrochés.
    void _simulate(double p_delta);
    void _redraw();

    DEBUG_MODE_DECLARATIONS()
    void _draw_debug();
};

/// 🧗‍♂️ Déplacement sur corde 2D : try_grab() s'accroche à la corde la plus proche (grab_distance), puis,
/// actif (contexte « corde » du MovementDirector), move.y grimpe / descend, move.x balance, et le saut lâche
/// la corde avec un élan. Le corps (CharacterBody2D) suit la corde.
class RopeMovement2D : public MovementComponent
{
    GDCLASS(RopeMovement2D, MovementComponent)

  public:
    void set_grab_distance(double p_value) { grab_distance = MAX(0.0, p_value); }
    double get_grab_distance() const { return grab_distance; }
    void set_climb_speed(double p_value) { climb_speed = MAX(0.0, p_value); }
    double get_climb_speed() const { return climb_speed; }
    void set_swing_force(double p_value) { swing_force = MAX(0.0, p_value); }
    double get_swing_force() const { return swing_force; }
    void set_jump_up_velocity(double p_value) { jump_up_velocity = MAX(0.0, p_value); }
    double get_jump_up_velocity() const { return jump_up_velocity; }
    void set_regrab_delay(double p_value) { regrab_delay = MAX(0.0, p_value); }
    double get_regrab_delay() const { return regrab_delay; }

    bool try_grab();
    /// Décroche le corps avec la vitesse de la corde (plus jump_up_velocity vers le haut si p_jump).
    void release(bool p_jump = false);
    bool is_attached() const { return Rope2D::find_rope_of(get_body()) != nullptr; }
    Rope2D *get_rope() const { return Rope2D::find_rope_of(get_body()); }

  protected:
    static void _bind_methods();
    void _physics_step(double p_delta) override;
    String _debug_text() const override;
    void _draw_debug(DebugGizmos &p_debug) override;

  private:
    double grab_distance = 40.0;
    double climb_speed = 110.0;
    double swing_force = 2000.0;
    double jump_up_velocity = 320.0;
    double regrab_delay = 0.3;
    /// Instant du dernier lâcher (Time, temps réel) pour regrab_delay.
    uint64_t released_msec = 0;
};

/// 🧗‍♂️ Déplacement sur corde 3D : set_vertical_input (ou -move.y) grimpe, move (x, z du monde) balance.
class RopeMovement3D : public MovementComponent
{
    GDCLASS(RopeMovement3D, MovementComponent)

  public:
    RopeMovement3D();

    void set_grab_distance(double p_value) { grab_distance = MAX(0.0, p_value); }
    double get_grab_distance() const { return grab_distance; }
    void set_climb_speed(double p_value) { climb_speed = MAX(0.0, p_value); }
    double get_climb_speed() const { return climb_speed; }
    void set_swing_force(double p_value) { swing_force = MAX(0.0, p_value); }
    double get_swing_force() const { return swing_force; }
    void set_jump_up_velocity(double p_value) { jump_up_velocity = MAX(0.0, p_value); }
    double get_jump_up_velocity() const { return jump_up_velocity; }
    void set_regrab_delay(double p_value) { regrab_delay = MAX(0.0, p_value); }
    double get_regrab_delay() const { return regrab_delay; }

    bool try_grab();
    /// Décroche le corps avec la vitesse de la corde (plus jump_up_velocity vers le haut si p_jump).
    void release(bool p_jump = false);
    bool is_attached() const { return Rope3D::find_rope_of(get_body()) != nullptr; }
    Rope3D *get_rope() const { return Rope3D::find_rope_of(get_body()); }

  protected:
    static void _bind_methods();
    void _physics_step(double p_delta) override;
    String _debug_text() const override;
    void _draw_debug(DebugGizmos &p_debug) override;

  private:
    double grab_distance = 1.0;
    double climb_speed = 2.0;
    double swing_force = 30.0;
    double jump_up_velocity = 4.5;
    double regrab_delay = 0.3;
    /// Instant du dernier lâcher (Time, temps réel) pour regrab_delay.
    uint64_t released_msec = 0;
};
} // namespace godot

#endif // ROPE_HPP
