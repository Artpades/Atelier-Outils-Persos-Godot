#ifndef MUSIC_LOOPER_HPP
#define MUSIC_LOOPER_HPP

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot
{
class AudioStreamPlayer;

/// 🔁 Lecteur de musique à boucle : joue l'intro (avant loop_start), puis répète la section
/// [loop_start, loop_end] (loop_count fois, ou sans fin), puis la fin du morceau.
///
/// La boucle est exacte à l'échantillon près pour un WAV, et pour un OGG / MP3 qui boucle jusqu'à la fin
/// du fichier : elle est alors confiée au flux audio lui-même (copie du flux, l'original n'est pas modifié).
/// Sinon (OGG / MP3 avec une fin de boucle avant la fin du fichier, autres flux), le retour à loop_start
/// est fait à chaque frame, à quelques millisecondes près.
class MusicLooper : public Node
{
    GDCLASS(MusicLooper, Node)

  public:
    /// Joue depuis p_from_position secondes, avec un fondu d'entrée de p_fade_in secondes.
    void play(double p_from_position = 0.0, double p_fade_in = 0.0);
    /// Arrête, avec un fondu de sortie de p_fade_out secondes.
    void stop(double p_fade_out = 0.0);
    void set_paused(bool p_paused);
    bool is_paused() const;
    bool is_playing() const;
    double get_playback_position() const;
    /// Nombre de retours à loop_start depuis le dernier play().
    int get_loops_done() const { return loops_done; }
    /// Vrai si la boucle en cours est assurée par le flux audio (exacte à l'échantillon).
    bool is_loop_sample_accurate() const { return native_loop; }
    /// loop_end, ou la durée du flux si loop_end vaut 0 (ou la dépasse).
    double get_effective_loop_end() const;

    void set_stream(const Ref<AudioStream> &p_value);
    Ref<AudioStream> get_stream() const { return stream; }
    void set_volume_db(float p_value);
    float get_volume_db() const { return volume_db; }
    void set_bus(const StringName &p_value);
    StringName get_bus() const { return bus; }
    void set_autoplay(bool p_value) { autoplay = p_value; }
    bool get_autoplay() const { return autoplay; }
    void set_loop_enabled(bool p_value) { loop_enabled = p_value; }
    bool get_loop_enabled() const { return loop_enabled; }
    void set_loop_start(double p_value) { loop_start = p_value; }
    double get_loop_start() const { return loop_start; }
    void set_loop_end(double p_value) { loop_end = p_value; }
    double get_loop_end() const { return loop_end; }
    void set_loop_count(int p_value) { loop_count = p_value; }
    int get_loop_count() const { return loop_count; }

    void _process(double p_delta) override;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    Ref<AudioStream> stream;
    float volume_db = 0.0f;
    StringName bus = "Master";
    bool autoplay = false;
    bool loop_enabled = true;
    double loop_start = 0.0;
    double loop_end = 0.0; // 0 : fin du flux
    int loop_count = -1;   // -1 : sans fin

    AudioStreamPlayer *player = nullptr; // enfant interne
    Ref<AudioStream> playback_stream;    // flux joué (copie configurée pour la boucle native)
    bool native_loop = false;
    bool looping = false; // la boucle est-elle encore active (loop_count non atteint)
    int loops_done = 0;
    double last_position = 0.0;
    double fade = 1.0; // facteur linéaire du fondu
    Ref<Tween> fade_tween;

    AudioStreamPlayer *_get_player();
    /// Prépare playback_stream et native_loop selon les réglages de boucle.
    void _prepare_stream();
    /// Plus de boucle : la lecture continue jusqu'à la fin du morceau.
    void _end_looping();
    void _on_loop();
    void _set_fade(double p_fade);
    void _apply_volume();
    void _stop_now();
    void _on_player_finished();

    // Boutons de l'inspecteur
    Callable _get_play_button() { return callable_mp(this, &MusicLooper::_on_play_button); }
    Callable _get_stop_button() { return callable_mp(this, &MusicLooper::_on_stop_button); }
    void _on_play_button() { play(); }
    void _on_stop_button() { stop(); }
};
} // namespace godot

#endif // MUSIC_LOOPER_HPP
