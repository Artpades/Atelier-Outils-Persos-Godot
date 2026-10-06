#ifndef FX_SHAKE_HPP
#define FX_SHAKE_HPP

#include "1.🧰Ensemble d'Outils/3.✨Effets Spéciaux et Sensations🎵/effect.hpp"

#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot
{
/// 💥 Secoue la position de la cible (bruit Simplex) pendant duration secondes, puis la remet en place.
/// Fonctionne sur un Control, un Node2D ou un Node3D. duration à 0 : secousse jusqu'à stop_fx().
class FXShake : public Effect
{
    GDCLASS(FXShake, Effect)

  public:
    FXShake();

    void set_duration(float p_value) { duration = p_value; }
    float get_duration() const { return duration; }
    void set_frequency(float p_value) { frequency = p_value; }
    float get_frequency() const { return frequency; }
    void set_max_amplitude(float p_value) { max_amplitude = p_value; }
    float get_max_amplitude() const { return max_amplitude; }
    void set_decrease_over_time(bool p_value) { decrease_over_time = p_value; }
    bool get_decrease_over_time() const { return decrease_over_time; }

  protected:
    static void _bind_methods();

    void _play(Node *p_target) override;
    void _process(double p_delta, Node *p_target) override;
    void _stop(Node *p_target) override;

  private:
    float duration = 0.5f;
    float frequency = 15.0f; // oscillations par seconde (environ)
    float max_amplitude = 10.0f;
    bool decrease_over_time = true;

    double timer = 0.0;
    Vector2 initial_position_2d;
    Vector3 initial_position_3d;
    Ref<FastNoiseLite> noise;

    /// Bruit dans [-1, 1] sur l'axe p_axis (0, 1, 2 : courbes indépendantes).
    float _sample(int p_axis) const;
};
} // namespace godot

#endif // FX_SHAKE_HPP
