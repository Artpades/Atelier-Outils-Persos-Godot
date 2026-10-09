#ifndef FX_AUDIO_HPP
#define FX_AUDIO_HPP

#include "1.🧰Ensemble d'Outils/3.✨Effets Spéciaux et Sensations🎵/effect.hpp"

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot
{
/// 🔊 Joue un son au jeu de l'effet, avec une hauteur éventuellement aléatoire.
/// Le lecteur est créé une fois sous la cible (nœud interne) puis réutilisé ; positional le spatialise
/// (AudioStreamPlayer2D / 3D selon la cible).
class FXAudio : public Effect
{
    GDCLASS(FXAudio, Effect)

  public:
    void set_stream(const Ref<AudioStream> &p_value) { stream = p_value; }
    Ref<AudioStream> get_stream() const { return stream; }
    void set_bus(const StringName &p_value) { bus = p_value; }
    StringName get_bus() const { return bus; }
    void set_volume_db(float p_value) { volume_db = p_value; }
    float get_volume_db() const { return volume_db; }
    void set_pitch_scale(float p_value) { pitch_scale = p_value; }
    float get_pitch_scale() const { return pitch_scale; }
    void set_positional(bool p_value) { positional = p_value; }
    bool get_positional() const { return positional; }
    void set_stop_sound_on_stop(bool p_value) { stop_sound_on_stop = p_value; }
    bool get_stop_sound_on_stop() const { return stop_sound_on_stop; }
    void set_randomize_pitch_scale(bool p_value) { randomize_pitch_scale = p_value; }
    bool get_randomize_pitch_scale() const { return randomize_pitch_scale; }
    void set_min_pitch(float p_value) { min_pitch = p_value; }
    float get_min_pitch() const { return min_pitch; }
    void set_max_pitch(float p_value) { max_pitch = p_value; }
    float get_max_pitch() const { return max_pitch; }

    /// Lecteur audio créé sous la cible (nul avant le premier jeu).
    Node *get_audio_player() const;

  protected:
    static void _bind_methods();

    void _play(Node *p_target) override;
    void _stop(Node *p_target) override;

  private:
    Ref<AudioStream> stream;
    StringName bus = "Master";
    float volume_db = 0.0f;
    float pitch_scale = 1.0f;
    bool positional = false;
    bool stop_sound_on_stop = false;
    // Hauteur aléatoire
    bool randomize_pitch_scale = false;
    float min_pitch = 0.9f;
    float max_pitch = 1.1f;

    ObjectID audio_player_id; // lecteur interne créé sous la dernière cible

    /// Lecteur existant sous p_target, sinon créé.
    Node *_get_or_create_player(Node *p_target);
};
} // namespace godot

#endif // FX_AUDIO_HPP
