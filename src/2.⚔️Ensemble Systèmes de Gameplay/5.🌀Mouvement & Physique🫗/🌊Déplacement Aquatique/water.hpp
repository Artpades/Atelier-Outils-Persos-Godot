#ifndef WATER_HPP
#define WATER_HPP

#include "../🧩Commun/movement_component.hpp"

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/area3d.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
/// 🌊 Étendue d'eau 2D (Area2D) : la surface est le bord haut de sa forme (RectangleShape2D), sinon sa
/// position + surface_offset. Les RigidBody2D y flottent (poussée selon la profondeur), y sont freinés et
/// emportés par le courant ; les personnages y nagent avec SwimMovement2D.
class WaterArea2D : public Area2D
{
    GDCLASS(WaterArea2D, Area2D)

  public:
    void set_buoyancy(double p_value) { buoyancy = MAX(0.0, p_value); }
    double get_buoyancy() const { return buoyancy; }
    void set_float_depth(double p_value) { float_depth = MAX(0.001, p_value); }
    double get_float_depth() const { return float_depth; }
    void set_linear_drag(double p_value) { linear_drag = MAX(0.0, p_value); }
    double get_linear_drag() const { return linear_drag; }
    void set_angular_drag(double p_value) { angular_drag = MAX(0.0, p_value); }
    double get_angular_drag() const { return angular_drag; }
    void set_current(const Vector2 &p_current) { current = p_current; }
    Vector2 get_current() const { return current; }
    void set_surface_offset(double p_offset) { surface_offset = p_offset; }
    double get_surface_offset() const { return surface_offset; }

    /// Hauteur (y global) de la surface.
    double get_surface_y() const;
    /// Profondeur d'un point sous la surface (négative au-dessus).
    double get_depth(const Vector2 &p_point) const { return p_point.y - get_surface_y(); }
    /// Eau qui contient p_body (nullptr sinon).
    static WaterArea2D *find_containing(Node *p_body);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    double buoyancy = 2.0;      // poussée à pleine immersion, en multiples de la gravité
    double float_depth = 16.0;  // profondeur de pleine immersion d'un objet
    double linear_drag = 3.0;   // freinage (par seconde)
    double angular_drag = 3.0;
    Vector2 current;            // accélération du courant
    double surface_offset = 0.0;
    static LocalVector<ObjectID> waters;
};

/// 🌊 Étendue d'eau 3D (Area3D) : surface = haut de sa forme (BoxShape3D), sinon position + surface_offset.
class WaterArea3D : public Area3D
{
    GDCLASS(WaterArea3D, Area3D)

  public:
    WaterArea3D();

    void set_buoyancy(double p_value) { buoyancy = MAX(0.0, p_value); }
    double get_buoyancy() const { return buoyancy; }
    void set_float_depth(double p_value) { float_depth = MAX(0.001, p_value); }
    double get_float_depth() const { return float_depth; }
    void set_linear_drag(double p_value) { linear_drag = MAX(0.0, p_value); }
    double get_linear_drag() const { return linear_drag; }
    void set_angular_drag(double p_value) { angular_drag = MAX(0.0, p_value); }
    double get_angular_drag() const { return angular_drag; }
    void set_current(const Vector3 &p_current) { current = p_current; }
    Vector3 get_current() const { return current; }
    void set_surface_offset(double p_offset) { surface_offset = p_offset; }
    double get_surface_offset() const { return surface_offset; }

    double get_surface_y() const;
    double get_depth(const Vector3 &p_point) const { return get_surface_y() - p_point.y; }
    static WaterArea3D *find_containing(Node *p_body);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    double buoyancy = 2.0;
    double float_depth = 0.5;
    double linear_drag = 3.0;
    double angular_drag = 3.0;
    Vector3 current;
    double surface_offset = 0.0;
    static LocalVector<ObjectID> waters;
};

/// Réglages communs de la nage (classe C++ interne).
struct SwimSettings
{
    double swim_speed = 140.0;
    double acceleration = 700.0;
    double drag = 3.0;
    double buoyancy = 600.0;       // remontée sans commande
    double float_depth = 10.0;     // profondeur d'équilibre sous la surface
    double surface_tolerance = 14.0;
    double jump_out_velocity = 380.0;
    double stroke_velocity = 160.0; // poussée d'un saut sous l'eau
};

/// 🏊 Nage 2D (CharacterBody2D) dans une WaterArea2D : déplacement libre (move), remontée vers la surface
/// sans commande, saut hors de l'eau près de la surface, coup de nage (saut) sous l'eau, courant.
class SwimMovement2D : public MovementComponent
{
    GDCLASS(SwimMovement2D, MovementComponent)

  public:
    void set_swim_speed(double p_value) { swim.swim_speed = MAX(0.0, p_value); }
    double get_swim_speed() const { return swim.swim_speed; }
    void set_acceleration(double p_value) { swim.acceleration = MAX(0.0, p_value); }
    double get_acceleration() const { return swim.acceleration; }
    void set_drag(double p_value) { swim.drag = MAX(0.0, p_value); }
    double get_drag() const { return swim.drag; }
    void set_buoyancy(double p_value) { swim.buoyancy = MAX(0.0, p_value); }
    double get_buoyancy() const { return swim.buoyancy; }
    void set_float_depth(double p_value) { swim.float_depth = p_value; }
    double get_float_depth() const { return swim.float_depth; }
    void set_surface_tolerance(double p_value) { swim.surface_tolerance = MAX(0.0, p_value); }
    double get_surface_tolerance() const { return swim.surface_tolerance; }
    void set_jump_out_velocity(double p_value) { swim.jump_out_velocity = MAX(0.0, p_value); }
    double get_jump_out_velocity() const { return swim.jump_out_velocity; }
    void set_stroke_velocity(double p_value) { swim.stroke_velocity = MAX(0.0, p_value); }
    double get_stroke_velocity() const { return swim.stroke_velocity; }

    WaterArea2D *get_water() const { return WaterArea2D::find_containing(get_body()); }

  protected:
    static void _bind_methods();
    void _physics_step(double p_delta) override;

  private:
    SwimSettings swim;
};

/// 🏊 Nage 3D (CharacterBody3D) : move = plan horizontal (relatif à la caméra si camera_relative),
/// set_vertical_input (ou saut maintenu) = monter / descendre.
class SwimMovement3D : public MovementComponent
{
    GDCLASS(SwimMovement3D, MovementComponent)

  public:
    SwimMovement3D();

    void set_swim_speed(double p_value) { swim.swim_speed = MAX(0.0, p_value); }
    double get_swim_speed() const { return swim.swim_speed; }
    void set_acceleration(double p_value) { swim.acceleration = MAX(0.0, p_value); }
    double get_acceleration() const { return swim.acceleration; }
    void set_drag(double p_value) { swim.drag = MAX(0.0, p_value); }
    double get_drag() const { return swim.drag; }
    void set_buoyancy(double p_value) { swim.buoyancy = MAX(0.0, p_value); }
    double get_buoyancy() const { return swim.buoyancy; }
    void set_float_depth(double p_value) { swim.float_depth = p_value; }
    double get_float_depth() const { return swim.float_depth; }
    void set_surface_tolerance(double p_value) { swim.surface_tolerance = MAX(0.0, p_value); }
    double get_surface_tolerance() const { return swim.surface_tolerance; }
    void set_jump_out_velocity(double p_value) { swim.jump_out_velocity = MAX(0.0, p_value); }
    double get_jump_out_velocity() const { return swim.jump_out_velocity; }
    void set_stroke_velocity(double p_value) { swim.stroke_velocity = MAX(0.0, p_value); }
    double get_stroke_velocity() const { return swim.stroke_velocity; }
    void set_camera_relative(bool p_relative) { camera_relative = p_relative; }
    bool get_camera_relative() const { return camera_relative; }

    WaterArea3D *get_water() const { return WaterArea3D::find_containing(get_body()); }

  protected:
    static void _bind_methods();
    void _physics_step(double p_delta) override;

  private:
    SwimSettings swim;
    bool camera_relative = false;
};
} // namespace godot

#endif // WATER_HPP
