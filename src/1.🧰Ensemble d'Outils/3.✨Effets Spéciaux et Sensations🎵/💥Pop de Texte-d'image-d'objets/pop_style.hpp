#ifndef POP_STYLE_HPP
#define POP_STYLE_HPP

#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot
{
/// 💥 Réglages d'un pop (texte, image et/ou objet qui jaillit d'un point) : mouvement, animation, apparence.
/// Les distances et vitesses sont en pixels ; en 3D, elles sont converties par l'unit_scale_3d du PopSpawner
/// (0,01 par défaut : 100 px = 1 m), si bien qu'un même style sert en 2D et en 3D.
class PopStyle : public Resource
{
    GDCLASS(PopStyle, Resource)

  public:
    /// Échelle du pop à p_age secondes (apparition, puis réduction en fin de vie, ou scale_curve).
    float get_scale_at(double p_age) const;
    /// Opacité du pop à p_age secondes (fondu sur les fade_out_duration dernières secondes).
    float get_alpha_at(double p_age) const;

#define POP_STYLE_PROPERTY(m_type, m_name)                       \
    void set_##m_name(const m_type &p_value) { m_name = p_value; } \
    m_type get_##m_name() const { return m_name; }

    // Mouvement
    POP_STYLE_PROPERTY(double, lifetime)
    POP_STYLE_PROPERTY(float, speed_min)
    POP_STYLE_PROPERTY(float, speed_max)
    POP_STYLE_PROPERTY(float, direction_degrees)
    POP_STYLE_PROPERTY(float, spread_degrees)
    POP_STYLE_PROPERTY(float, gravity)
    POP_STYLE_PROPERTY(float, damping)
    POP_STYLE_PROPERTY(float, spawn_radius)
    POP_STYLE_PROPERTY(bool, use_floor)
    POP_STYLE_PROPERTY(float, floor_offset)
    POP_STYLE_PROPERTY(float, bounce)
    POP_STYLE_PROPERTY(float, floor_friction)
    POP_STYLE_PROPERTY(float, spin_min)
    POP_STYLE_PROPERTY(float, spin_max)
    // Animation
    POP_STYLE_PROPERTY(double, pop_in_duration)
    POP_STYLE_PROPERTY(float, start_scale)
    POP_STYLE_PROPERTY(float, pop_scale)
    POP_STYLE_PROPERTY(float, end_scale)
    POP_STYLE_PROPERTY(double, fade_out_duration)
    POP_STYLE_PROPERTY(Ref<Curve>, scale_curve)
    // Apparence
    POP_STYLE_PROPERTY(Ref<Font>, font)
    POP_STYLE_PROPERTY(int, font_size)
    POP_STYLE_PROPERTY(Color, text_color)
    POP_STYLE_PROPERTY(Color, positive_color)
    POP_STYLE_PROPERTY(Color, negative_color)
    POP_STYLE_PROPERTY(int, outline_size)
    POP_STYLE_PROPERTY(Color, outline_color)
    POP_STYLE_PROPERTY(Vector2, icon_size)
    POP_STYLE_PROPERTY(float, spacing)
    POP_STYLE_PROPERTY(int, z_index)
    POP_STYLE_PROPERTY(bool, always_on_top_3d)

#undef POP_STYLE_PROPERTY

  protected:
    static void _bind_methods();

  private:
    // Mouvement
    double lifetime = 1.0;
    float speed_min = 150.0f;
    float speed_max = 250.0f;
    float direction_degrees = 0.0f; // 0 : vers le haut, 90 : vers la droite (en 3D : inclinaison)
    float spread_degrees = 40.0f;   // dispersion autour de la direction
    float gravity = 600.0f;         // px/s², vers le bas
    float damping = 0.0f;           // freinage (par seconde)
    float spawn_radius = 0.0f;      // départ dans un disque autour du point d'apparition
    bool use_floor = false;
    float floor_offset = 40.0f;  // sol à cette distance sous le point d'apparition (atteint par le centre du pop)
    float bounce = 0.5f;         // restitution au rebond (0 : pas de rebond)
    float floor_friction = 0.6f; // vitesse horizontale conservée à chaque rebond
    float spin_min = 0.0f;       // rotation (degrés / s), tirée entre min et max
    float spin_max = 0.0f;
    // Animation
    double pop_in_duration = 0.18;
    float start_scale = 0.2f;
    float pop_scale = 1.4f; // dépassement de l'apparition, avant de revenir à 1
    float end_scale = 0.6f;
    double fade_out_duration = 0.3;
    Ref<Curve> scale_curve; // facultative : échelle selon l'âge normalisé (remplace apparition et réduction)
    // Apparence
    Ref<Font> font;
    int font_size = 32;
    Color text_color = Color(1, 1, 1, 1);
    Color positive_color = Color(0.4f, 1.0f, 0.45f, 1);
    Color negative_color = Color(1.0f, 0.35f, 0.3f, 1);
    int outline_size = 8;
    Color outline_color = Color(0, 0, 0, 1);
    Vector2 icon_size = Vector2(32, 32);
    float spacing = 4.0f; // entre l'image et le texte
    int z_index = 100;    // 2D : au-dessus du décor
    bool always_on_top_3d = true;
};
} // namespace godot

#endif // POP_STYLE_HPP
