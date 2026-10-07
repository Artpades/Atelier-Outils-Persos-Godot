#ifndef DRAWING_BRUSH_HPP
#define DRAWING_BRUSH_HPP

#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/texture2d.hpp>

#include <vector>

namespace godot
{
/// 🖌️ Réglages d'un pinceau du système de dessin (l'équivalent d'un « sous-outil » de Clip Studio Paint).
///
/// Un trait est une suite de « tampons » (dabs) espacés de spacing × taille. Chaque tampon est la forme du pinceau
/// (ronde avec dureté, ou tip_texture), multipliée par la texture de grain (texture) et par l'opacité.
/// Dans un même trait, les tampons ne s'accumulent pas au-delà de l'opacité du trait : la densité (flow) règle la
/// vitesse à laquelle ils s'en approchent (1 = encre, faible = aérographe).
///
/// Dynamique : la taille et l'opacité peuvent dépendre de la pression du stylet et/ou de la vitesse du trait
/// (size_dynamics, opacity_dynamics). L'entrée (pression, vitesse normalisée par velocity_max) passe par une courbe
/// facultative, puis donne un facteur entre le minimum (size_min, opacity_min) et 1.
///
/// Shader : avec un material (ShaderMaterial canvas_item), le trait en cours est affiché et appliqué au calque à
/// travers ce shader (rendu GPU par DrawingCanvas). TEXTURE y contient le trait (couleur du pinceau, alpha =
/// couverture) et VERTEX, dans vertex(), la position en pixels du dessin.
class DrawingBrush : public Resource
{
    GDCLASS(DrawingBrush, Resource)

  public:
    enum BrushBlend
    {
        BRUSH_BLEND_NORMAL, // peint par-dessus
        BRUSH_BLEND_BEHIND, // peint derrière les pixels existants (« Arrière » de CSP)
        BRUSH_BLEND_ERASE,  // efface (gomme)
    };

    /// Entrées pouvant piloter la taille ou l'opacité (combinables).
    enum Dynamic
    {
        DYNAMIC_PRESSURE = 1, // pression du stylet (1 à la souris)
        DYNAMIC_VELOCITY = 2, // vitesse du trait
    };

    enum Preset
    {
        PRESET_PEN,          // « Plume G » : encrage net, taille à la pression
        PRESET_PENCIL,       // crayon : grain de papier, opacité à la pression
        PRESET_MARKER,       // feutre : aucune dynamique, bord net
        PRESET_AIRBRUSH,     // aérographe : très doux, faible densité
        PRESET_WATERCOLOR,   // lavis : doux, opacité à la pression, la vitesse affine le trait
        PRESET_INK_VELOCITY, // encre à la vitesse : plus le trait est rapide, plus il est fin
        PRESET_HARD_ERASER,  // gomme dure
        PRESET_SOFT_ERASER,  // gomme douce
    };

    /// Couverture (0-1) d'une image échantillonnée (forme ou grain), mise en cache.
    struct Coverage
    {
        int width = 0;
        int height = 0;
        std::vector<float> values;
        bool is_empty() const { return values.empty(); }
        /// Échantillon bilinéaire en coordonnées normalisées (0-1) ; hors image : 0 (ou répété si p_wrap).
        float sample(float p_u, float p_v, bool p_wrap) const;
    };

    static Ref<DrawingBrush> create_preset(Preset p_preset);
    /// Texture de grain de papier répétable (bruit), en niveaux de gris : p_contrast > 1 accentue le grain.
    static Ref<ImageTexture> create_grain_texture(int p_size = 128, int p_seed = 7, float p_contrast = 1.6f);

    // ⚙️ Forme
    void set_size(float p_size);
    float get_size() const { return size; }
    void set_opacity(float p_opacity);
    float get_opacity() const { return opacity; }
    void set_flow(float p_flow);
    float get_flow() const { return flow; }
    void set_hardness(float p_hardness);
    float get_hardness() const { return hardness; }
    void set_spacing(float p_spacing);
    float get_spacing() const { return spacing; }
    void set_angle(float p_degrees);
    float get_angle() const { return angle; }
    void set_roundness(float p_roundness);
    float get_roundness() const { return roundness; }
    void set_follow_direction(bool p_enabled);
    bool get_follow_direction() const { return follow_direction; }
    void set_tip_texture(const Ref<Texture2D> &p_texture);
    Ref<Texture2D> get_tip_texture() const { return tip_texture; }
    void set_blend_mode(BrushBlend p_mode);
    BrushBlend get_blend_mode() const { return blend_mode; }

    // 🧻 Texture de grain
    void set_texture(const Ref<Texture2D> &p_texture);
    Ref<Texture2D> get_texture() const { return texture; }
    void set_texture_scale(float p_scale);
    float get_texture_scale() const { return texture_scale; }
    void set_texture_strength(float p_strength);
    float get_texture_strength() const { return texture_strength; }

    // ✨ Shader
    void set_material(const Ref<ShaderMaterial> &p_material);
    Ref<ShaderMaterial> get_material() const { return material; }

    // 📈 Dynamique
    void set_size_dynamics(int p_flags);
    int get_size_dynamics() const { return size_dynamics; }
    void set_size_min(float p_min);
    float get_size_min() const { return size_min; }
    void set_opacity_dynamics(int p_flags);
    int get_opacity_dynamics() const { return opacity_dynamics; }
    void set_opacity_min(float p_min);
    float get_opacity_min() const { return opacity_min; }
    void set_pressure_curve(const Ref<Curve> &p_curve);
    Ref<Curve> get_pressure_curve() const { return pressure_curve; }
    void set_velocity_curve(const Ref<Curve> &p_curve);
    Ref<Curve> get_velocity_curve() const { return velocity_curve; }
    void set_velocity_max(float p_velocity);
    float get_velocity_max() const { return velocity_max; }

    // 🪶 Stabilisation
    void set_stabilization(int p_amount);
    int get_stabilization() const { return stabilization; }

    /// Facteur (size_min-1) appliqué à la taille pour une pression (0-1) et une vitesse (pixels/s).
    float get_size_factor(float p_pressure, float p_velocity) const;
    /// Facteur (opacity_min-1) appliqué à l'opacité.
    float get_opacity_factor(float p_pressure, float p_velocity) const;
    /// Diamètre et opacité d'un tampon.
    float get_dab_size(float p_pressure, float p_velocity) const { return size * get_size_factor(p_pressure, p_velocity); }
    float get_dab_opacity(float p_pressure, float p_velocity) const { return opacity * get_opacity_factor(p_pressure, p_velocity); }
    /// Couverture (0-1) de la forme du pinceau en un point local (en pixels, rapporté au centre, sans rotation) pour
    /// un rayon donné. Ronde (dureté) ou tip_texture.
    float get_tip_coverage(float p_local_x, float p_local_y, float p_radius) const;
    /// Facteur du grain (0-1) en une position du dessin (1 sans texture).
    float get_grain(float p_x, float p_y) const;
    bool has_tip_texture() const { return !tip_cache.is_empty(); }

    /// Couverture d'une texture : alpha si elle en a, sinon 1 - luminance (forme noire sur fond blanc).
    static Coverage make_coverage(const Ref<Texture2D> &p_texture, bool p_luminance_only);

  protected:
    static void _bind_methods();
    void _validate_property(PropertyInfo &p_property) const;

  private:
    float size = 12.0f;
    float opacity = 1.0f;
    float flow = 1.0f;
    float hardness = 0.9f;
    float spacing = 0.1f;
    float angle = 0.0f;
    float roundness = 1.0f;
    bool follow_direction = false;
    Ref<Texture2D> tip_texture;
    BrushBlend blend_mode = BRUSH_BLEND_NORMAL;

    Ref<Texture2D> texture;
    float texture_scale = 1.0f;
    float texture_strength = 1.0f;

    Ref<ShaderMaterial> material;

    int size_dynamics = DYNAMIC_PRESSURE;
    float size_min = 0.2f;
    int opacity_dynamics = 0;
    float opacity_min = 0.0f;
    Ref<Curve> pressure_curve;
    Ref<Curve> velocity_curve;
    float velocity_max = 2500.0f;

    int stabilization = 6;

    Coverage tip_cache;
    Coverage grain_cache;

    float _dynamic_input(int p_flags, float p_pressure, float p_velocity) const;
    void _update_tip_cache();
    void _update_grain_cache();
};
} // namespace godot

VARIANT_ENUM_CAST(DrawingBrush::BrushBlend);
VARIANT_ENUM_CAST(DrawingBrush::Dynamic);
VARIANT_ENUM_CAST(DrawingBrush::Preset);

#endif // DRAWING_BRUSH_HPP
