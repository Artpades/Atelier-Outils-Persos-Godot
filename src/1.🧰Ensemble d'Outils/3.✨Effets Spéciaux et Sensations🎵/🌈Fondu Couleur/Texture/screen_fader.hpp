#ifndef SCREEN_FADER_HPP
#define SCREEN_FADER_HPP

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/tween.hpp>

namespace godot
{
class ColorRect;

/// 🌈 Fondu de l'écran entier vers une couleur ou une texture (transitions de scène, mort du joueur...).
/// progress va de 0 (écran visible) à 1 (écran couvert). Avec un masque (texture en niveaux de gris),
/// l'écran se couvre progressivement des zones sombres du masque vers les zones claires (volet, dissolution).
class ScreenFader : public CanvasLayer
{
    GDCLASS(ScreenFader, CanvasLayer)

  public:
    /// Couvre l'écran (progress -> 1). p_duration < 0 : duration.
    void fade_out(double p_duration = -1.0);
    /// Découvre l'écran (progress -> 0). p_duration < 0 : duration.
    void fade_in(double p_duration = -1.0);
    /// Anime progress jusqu'à p_progress.
    void fade_to(double p_progress, double p_duration = -1.0);
    bool is_fading() const;
    bool is_covered() const { return progress >= 1.0; }

    void set_progress(double p_value);
    double get_progress() const { return progress; }
    void set_color(const Color &p_value);
    Color get_color() const { return color; }
    void set_texture(const Ref<Texture2D> &p_value);
    Ref<Texture2D> get_texture() const { return texture; }
    void set_mask(const Ref<Texture2D> &p_value);
    Ref<Texture2D> get_mask() const { return mask; }
    void set_mask_smoothness(double p_value);
    double get_mask_smoothness() const { return mask_smoothness; }
    void set_duration(double p_value) { duration = p_value; }
    double get_duration() const { return duration; }
    void set_transition(Tween::TransitionType p_value) { transition = p_value; }
    Tween::TransitionType get_transition() const { return transition; }
    void set_ease_type(Tween::EaseType p_value) { ease_type = p_value; }
    Tween::EaseType get_ease_type() const { return ease_type; }
    void set_block_input(bool p_value);
    bool get_block_input() const { return block_input; }
    void set_always_process(bool p_value) { always_process = p_value; }
    bool get_always_process() const { return always_process; }

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    double progress = 0.0;
    Color color = Color(0, 0, 0, 1);
    Ref<Texture2D> texture; // remplace la couleur unie (teintée par color)
    Ref<Texture2D> mask;    // forme de la transition (niveaux de gris)
    double mask_smoothness = 0.1;
    double duration = 0.5;
    Tween::TransitionType transition = Tween::TRANS_SINE;
    Tween::EaseType ease_type = Tween::EASE_IN_OUT;
    bool block_input = true;    // l'écran couvert (même en partie) arrête les clics
    bool always_process = true; // insensible à la pause et à Engine.time_scale

    ColorRect *overlay = nullptr; // enfant interne
    Ref<ShaderMaterial> material;
    Ref<Tween> tween;

    void _build_overlay();
    void _update_overlay();
    void _on_fade_finished();
};
} // namespace godot

#endif // SCREEN_FADER_HPP
