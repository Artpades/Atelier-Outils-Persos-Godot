#ifndef FX_CHANGE_SIZE_HPP
#define FX_CHANGE_SIZE_HPP

#include "1.🧰Ensemble d'Outils/3.✨Effets Spéciaux et Sensations🎵/effect.hpp"

#include <godot_cpp/classes/tween.hpp>

namespace godot
{
/// 🦣🫘 Anime l'échelle de la cible jusqu'à final_scale au jeu de l'effet, puis jusqu'à original_value à son
/// arrêt. Fonctionne sur un Control (pivot centré), un Node2D ou un Node3D.
class FXChangeSize : public Effect
{
    GDCLASS(FXChangeSize, Effect)

  public:
    void set_final_scale(float p_value) { final_scale = p_value; }
    float get_final_scale() const { return final_scale; }
    void set_original_value(float p_value) { original_value = p_value; }
    float get_original_value() const { return original_value; }
    void set_duration(float p_value) { duration = p_value; }
    float get_duration() const { return duration; }
    void set_transition(Tween::TransitionType p_value) { transition = p_value; }
    Tween::TransitionType get_transition() const { return transition; }
    void set_ease_type(Tween::EaseType p_value) { ease_type = p_value; }
    Tween::EaseType get_ease_type() const { return ease_type; }

  protected:
    static void _bind_methods();

    void _play(Node *p_target) override;
    void _stop(Node *p_target) override;

  private:
    float final_scale = 1.2f;
    float original_value = 1.0f;
    float duration = 0.5f;
    Tween::TransitionType transition = Tween::TRANS_BACK;
    Tween::EaseType ease_type = Tween::EASE_OUT;

    Ref<Tween> tween; // animation en cours, interrompue par la suivante

    void _tween_scale_to(Node *p_target, float p_scale);
};
} // namespace godot

#endif // FX_CHANGE_SIZE_HPP
