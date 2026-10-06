#ifndef FX_FADE_HPP
#define FX_FADE_HPP

#include "1.🧰Ensemble d'Outils/3.✨Effets Spéciaux et Sensations🎵/effect.hpp"

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/tween.hpp>

namespace godot
{
/// 🌈 Fondu d'une couleur ou d'une texture de la cible, au jeu de l'effet (et retour à son arrêt).
///  - Couleur : anime la propriété color_property (modulate, self_modulate, color, light_color,
///    material_override:albedo_color...) jusqu'à color.
///  - Texture : fondu enchaîné de la propriété texture_property jusqu'à texture. Sur un CanvasItem
///    (Sprite2D, TextureRect...), les deux textures sont mélangées par un shader le temps du fondu ;
///    sur un autre nœud qui a un modulate (Sprite3D...), la cible s'efface, change de texture et réapparaît.
class FXFade : public Effect
{
    GDCLASS(FXFade, Effect)

  public:
    enum FadeMode
    {
        FADE_COLOR,
        FADE_TEXTURE,
    };

    void set_mode(FadeMode p_value) { mode = p_value; }
    FadeMode get_mode() const { return mode; }
    void set_color(const Color &p_value) { color = p_value; }
    Color get_color() const { return color; }
    void set_color_property(const String &p_value) { color_property = p_value; }
    String get_color_property() const { return color_property; }
    void set_texture(const Ref<Texture2D> &p_value) { texture = p_value; }
    Ref<Texture2D> get_texture() const { return texture; }
    void set_texture_property(const String &p_value) { texture_property = p_value; }
    String get_texture_property() const { return texture_property; }
    void set_duration(float p_value) { duration = p_value; }
    float get_duration() const { return duration; }
    void set_transition(Tween::TransitionType p_value) { transition = p_value; }
    Tween::TransitionType get_transition() const { return transition; }
    void set_ease_type(Tween::EaseType p_value) { ease_type = p_value; }
    Tween::EaseType get_ease_type() const { return ease_type; }
    void set_revert_on_stop(bool p_value) { revert_on_stop = p_value; }
    bool get_revert_on_stop() const { return revert_on_stop; }

  protected:
    static void _bind_methods();

    void _play(Node *p_target) override;
    void _stop(Node *p_target) override;

  private:
    FadeMode mode = FADE_COLOR;
    Color color = Color(1, 1, 1, 0);
    String color_property = "modulate";
    Ref<Texture2D> texture;
    String texture_property = "texture";
    float duration = 0.5f;
    Tween::TransitionType transition = Tween::TRANS_SINE;
    Tween::EaseType ease_type = Tween::EASE_IN_OUT;
    bool revert_on_stop = true;

    // État du fondu
    Color original_color;
    Ref<Texture2D> original_texture;
    Ref<Tween> tween;
    // Fondu enchaîné par shader (CanvasItem)
    Ref<ShaderMaterial> crossfade_material;
    Ref<Material> material_before_crossfade;
    Ref<Texture2D> pending_texture;
    ObjectID crossfade_target_id;

    void _fade_color_to(Node *p_target, const Color &p_color);
    void _fade_texture_to(Node *p_target, const Ref<Texture2D> &p_texture);
    /// Termine le fondu enchaîné en cours : texture finale posée, matériau d'origine rendu.
    void _finish_crossfade();
    Ref<Tween> _new_tween(Node *p_target);
};
} // namespace godot

VARIANT_ENUM_CAST(FXFade::FadeMode);

#endif // FX_FADE_HPP
