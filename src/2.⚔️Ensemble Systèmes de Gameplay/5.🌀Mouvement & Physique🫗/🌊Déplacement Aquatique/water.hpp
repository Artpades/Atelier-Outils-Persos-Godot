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
/// emportés par le courant ; les personnages y nagent avec SwimMovement2D. debug_mode : contour, surface, courant.
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

    DEBUG_MODE_DECLARATIONS()
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

    DEBUG_MODE_DECLARATIONS()
};

/// Réglages et état communs de la nage (classe C++ interne).
struct SwimSettings
{
    enum Style
    {
        STYLE_SURFACE,     // flottaison : déplacement direct, remontée vers la surface (ancien comportement)
        STYLE_DIRECTIONAL, // Mario Galaxy : le corps file vers la direction commandée, s'oriente et garde son élan
        STYLE_FISH,        // poisson : nage toujours vers l'avant ; la commande oriente, ondulation du corps
    };

    int style = STYLE_DIRECTIONAL;
    double swim_speed = 160.0;
    double acceleration = 500.0;
    double drag = 2.0;
    double buoyancy = 600.0;        // retour à la profondeur d'équilibre (flottaison, surface)
    double float_depth = 10.0;      // profondeur d'équilibre sous la surface
    double surface_tolerance = 14.0;
    double jump_out_velocity = 380.0;
    double stroke_velocity = 160.0; // coup de nage : élan ajouté le long de la direction
    double stroke_cooldown = 0.35;
    double turn_speed = 5.0;        // rad/s
    double run_multiplier = 1.6;
    double cruise_ratio = 0.4;      // poisson : vitesse de croisière (part de swim_speed)
    bool align_to_heading = true;   // incline le visuel (ou le corps) dans le sens de la nage
    double undulation_amplitude = 0.25; // poisson : ondulation du visuel (rad)
    double undulation_frequency = 2.5;  // Hz
    NodePath visual_path;

    // État
    double speed = 0.0;
    double stroke_timer = 0.0;
    double time = 0.0;
    bool initialized = false;
    bool leaping = false; // saut hors de l'eau en cours (trajectoire libre jusqu'à la retombée)
};

/// Accesseurs des réglages communs (dans SwimMovement2D / SwimMovement3D).
#define SWIM_ACCESSORS()                                                                                          \
    void set_style(int p_style) { swim.style = CLAMP(p_style, 0, 2); }                                            \
    int get_style() const { return swim.style; }                                                                  \
    void set_swim_speed(double p_value) { swim.swim_speed = MAX(0.0, p_value); }                                  \
    double get_swim_speed() const { return swim.swim_speed; }                                                     \
    void set_acceleration(double p_value) { swim.acceleration = MAX(0.0, p_value); }                              \
    double get_acceleration() const { return swim.acceleration; }                                                 \
    void set_drag(double p_value) { swim.drag = MAX(0.0, p_value); }                                              \
    double get_drag() const { return swim.drag; }                                                                 \
    void set_buoyancy(double p_value) { swim.buoyancy = MAX(0.0, p_value); }                                      \
    double get_buoyancy() const { return swim.buoyancy; }                                                         \
    void set_float_depth(double p_value) { swim.float_depth = p_value; }                                          \
    double get_float_depth() const { return swim.float_depth; }                                                   \
    void set_surface_tolerance(double p_value) { swim.surface_tolerance = MAX(0.0, p_value); }                    \
    double get_surface_tolerance() const { return swim.surface_tolerance; }                                       \
    void set_jump_out_velocity(double p_value) { swim.jump_out_velocity = MAX(0.0, p_value); }                    \
    double get_jump_out_velocity() const { return swim.jump_out_velocity; }                                       \
    void set_stroke_velocity(double p_value) { swim.stroke_velocity = MAX(0.0, p_value); }                        \
    double get_stroke_velocity() const { return swim.stroke_velocity; }                                           \
    void set_stroke_cooldown(double p_value) { swim.stroke_cooldown = MAX(0.0, p_value); }                        \
    double get_stroke_cooldown() const { return swim.stroke_cooldown; }                                           \
    void set_turn_speed(double p_value) { swim.turn_speed = MAX(0.0, p_value); }                                  \
    double get_turn_speed() const { return swim.turn_speed; }                                                     \
    void set_run_multiplier(double p_value) { swim.run_multiplier = MAX(0.0, p_value); }                          \
    double get_run_multiplier() const { return swim.run_multiplier; }                                             \
    void set_cruise_ratio(double p_value) { swim.cruise_ratio = CLAMP(p_value, 0.0, 1.0); }                       \
    double get_cruise_ratio() const { return swim.cruise_ratio; }                                                 \
    void set_align_to_heading(bool p_align) { swim.align_to_heading = p_align; }                                  \
    bool get_align_to_heading() const { return swim.align_to_heading; }                                           \
    void set_undulation_amplitude(double p_value) { swim.undulation_amplitude = MAX(0.0, p_value); }              \
    double get_undulation_amplitude() const { return swim.undulation_amplitude; }                                 \
    void set_undulation_frequency(double p_value) { swim.undulation_frequency = MAX(0.0, p_value); }              \
    double get_undulation_frequency() const { return swim.undulation_frequency; }                                 \
    void set_visual_path(const NodePath &p_path) { swim.visual_path = p_path; }                                   \
    NodePath get_visual_path() const { return swim.visual_path; }                                                 \
    double get_speed() const { return swim.speed; }

/// 🏊 Nage 2D (CharacterBody2D, vue de côté) dans une WaterArea2D. Trois styles (style) :
///   - Directionnel (défaut, à la Mario Galaxy) : la direction de nage (heading) tourne vers la commande
///     (turn_speed), la vitesse monte le long de cette direction et retombe doucement sans commande ; le saut
///     donne un coup de nage (élan stroke_velocity) sous l'eau et fait sortir de l'eau près de la surface ;
///     la course accélère (run_multiplier). Le visuel s'incline dans le sens de la nage.
///   - Poisson : comme Directionnel, mais le corps nage toujours (vitesse de croisière cruise_ratio) et
///     ondule (undulation_*).
///   - Flottaison : déplacement direct et remontée vers la profondeur d'équilibre (float_depth).
/// À la surface, la nage reste horizontale (on ne sort qu'en sautant). Courant de l'eau et zones de force.
class SwimMovement2D : public MovementComponent
{
    GDCLASS(SwimMovement2D, MovementComponent)

  public:
    SWIM_ACCESSORS()
    Vector2 get_heading() const { return heading; }
    WaterArea2D *get_water() const { return WaterArea2D::find_containing(get_body()); }

  protected:
    static void _bind_methods();
    void _physics_step(double p_delta) override;
    void _on_deactivated() override;
    void _draw_debug(DebugGizmos &p_debug) override;
    String _debug_text() const override;

  private:
    SwimSettings swim;
    Vector2 heading = Vector2(1, 0);
    Vector2 drift;
    Vector2 desired;
    double last_depth = 0.0;

    void _step_surface(class CharacterBody2D *p_character, WaterArea2D *p_water, double p_delta);
    void _orient(double p_delta);
};

/// 🏊 Nage 3D (CharacterBody3D). Mêmes styles qu'en 2D :
///   - Directionnel (Mario Galaxy) : move va vers l'avant / les côtés de la caméra (camera_relative), en
///     suivant son inclinaison (pousser en avant caméra baissée = plonger) ; set_vertical_input monte /
///     descend. Le corps s'oriente vers sa direction de nage (tangage compris).
///   - Poisson : commandes « de poisson » : move.x tourne (lacet), set_vertical_input cabre / pique (tangage),
///     move.y en avant accélère, en arrière ralentit ; nage toujours à la vitesse de croisière.
///   - Flottaison : déplacement horizontal direct, saut maintenu / set_vertical_input pour monter, remontée
///     vers la surface.
class SwimMovement3D : public MovementComponent
{
    GDCLASS(SwimMovement3D, MovementComponent)

  public:
    SwimMovement3D();

    SWIM_ACCESSORS()
    void set_camera_relative(bool p_relative) { camera_relative = p_relative; }
    bool get_camera_relative() const { return camera_relative; }
    Vector3 get_heading() const { return heading; }
    WaterArea3D *get_water() const { return WaterArea3D::find_containing(get_body()); }

  protected:
    static void _bind_methods();
    void _physics_step(double p_delta) override;
    void _on_deactivated() override;
    void _draw_debug(DebugGizmos &p_debug) override;
    String _debug_text() const override;

  private:
    SwimSettings swim;
    bool camera_relative = false;
    Vector3 heading = Vector3(0, 0, -1);
    Vector3 drift;
    Vector3 desired;
    double last_depth = 0.0;

    void _step_surface(class CharacterBody3D *p_character, WaterArea3D *p_water, double p_delta);
    void _orient(double p_delta);
};
} // namespace godot

#endif // WATER_HPP
