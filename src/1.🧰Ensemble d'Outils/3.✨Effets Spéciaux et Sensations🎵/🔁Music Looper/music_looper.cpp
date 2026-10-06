#include "music_looper.hpp"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/audio_stream_mp3.hpp>
#include <godot_cpp/classes/audio_stream_ogg_vorbis.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/callback_tweener.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/method_tweener.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void MusicLooper::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY && autoplay && !Engine::get_singleton()->is_editor_hint())
    {
        play();
    }
}

// 🎯 API publique

void MusicLooper::play(double p_from_position, double p_fade_in)
{
    AudioStreamPlayer *audio = _get_player();
    _prepare_stream();
    audio->set_stream(playback_stream);
    if (playback_stream.is_null())
    {
        return;
    }

    loops_done = 0;
    last_position = p_from_position;
    if (fade_tween.is_valid())
    {
        fade_tween->kill();
        fade_tween.unref();
    }
    if (p_fade_in > 0.0 && is_inside_tree())
    {
        _set_fade(0.0);
        fade_tween = create_tween();
        fade_tween->tween_method(callable_mp(this, &MusicLooper::_set_fade), 0.0, 1.0, p_fade_in);
    }
    else
    {
        _set_fade(1.0);
    }
    audio->play(p_from_position);
}

void MusicLooper::stop(double p_fade_out)
{
    if (fade_tween.is_valid())
    {
        fade_tween->kill();
        fade_tween.unref();
    }
    if (p_fade_out > 0.0 && is_playing() && is_inside_tree())
    {
        fade_tween = create_tween();
        fade_tween->tween_method(callable_mp(this, &MusicLooper::_set_fade), fade, 0.0, p_fade_out);
        fade_tween->tween_callback(callable_mp(this, &MusicLooper::_stop_now));
        return;
    }
    _stop_now();
}

void MusicLooper::_stop_now()
{
    fade_tween.unref();
    if (player)
    {
        player->stop();
    }
}

void MusicLooper::set_paused(bool p_paused) { _get_player()->set_stream_paused(p_paused); }

bool MusicLooper::is_paused() const { return player && player->get_stream_paused(); }

bool MusicLooper::is_playing() const { return player && player->is_playing(); }

double MusicLooper::get_playback_position() const { return player ? player->get_playback_position() : 0.0; }

double MusicLooper::get_effective_loop_end() const
{
    const double length = stream.is_valid() ? stream->get_length() : 0.0;
    return loop_end > 0.0 && (length <= 0.0 || loop_end < length) ? loop_end : length;
}

// 🔁 Boucle

void MusicLooper::_prepare_stream()
{
    native_loop = false;
    looping = loop_enabled && loop_count != 0 && stream.is_valid();
    playback_stream = stream;
    if (!looping)
    {
        return;
    }

    const double length = stream->get_length();
    const double end = get_effective_loop_end();
    if (end <= loop_start)
    {
        WARN_PRINT(String(U"[MusicLooper] loop_start (") + String::num(loop_start) + String(U" s) doit précéder la fin de boucle (") + String::num(end) + U" s) : pas de boucle.");
        looping = false;
        return;
    }
    const bool loops_to_stream_end = length > 0.0 && end >= length - 0.001;

    // WAV : boucle native quelle que soit la section (en échantillons)
    Ref<AudioStreamWAV> wav = stream;
    if (wav.is_valid())
    {
        Ref<AudioStreamWAV> copy = wav->duplicate();
        const double mix_rate = copy->get_mix_rate();
        copy->set_loop_mode(AudioStreamWAV::LOOP_FORWARD);
        copy->set_loop_begin((int32_t)Math::round(loop_start * mix_rate));
        copy->set_loop_end((int32_t)Math::round(end * mix_rate));
        playback_stream = copy;
        native_loop = true;
        return;
    }

    // OGG / MP3 : boucle native seulement jusqu'à la fin du fichier ; sinon, boucle propre désactivée
    // (le retour à loop_start est fait à chaque frame)
    Ref<AudioStreamOggVorbis> ogg = stream;
    if (ogg.is_valid())
    {
        Ref<AudioStreamOggVorbis> copy = ogg->duplicate();
        copy->set_loop(loops_to_stream_end);
        copy->set_loop_offset(loop_start);
        playback_stream = copy;
        native_loop = loops_to_stream_end;
        return;
    }
    Ref<AudioStreamMP3> mp3 = stream;
    if (mp3.is_valid())
    {
        Ref<AudioStreamMP3> copy = mp3->duplicate();
        copy->set_loop(loops_to_stream_end);
        copy->set_loop_offset(loop_start);
        playback_stream = copy;
        native_loop = loops_to_stream_end;
    }
}

void MusicLooper::_process(double /*p_delta*/)
{
    if (!player || !player->is_playing() || player->get_stream_paused())
    {
        return;
    }

    // Position réellement entendue : la dernière position mixée plus le temps écoulé depuis
    const double position = player->get_playback_position() + AudioServer::get_singleton()->get_time_since_last_mix();
    if (native_loop)
    {
        // Le flux est revenu en arrière tout seul : une boucle de plus
        if (looping && position + 0.05 < last_position)
        {
            _on_loop();
        }
    }
    else if (looping)
    {
        const double end = get_effective_loop_end();
        if (position >= end)
        {
            const double section = end - loop_start;
            player->seek(loop_start + Math::fmod(position - end, section));
            _on_loop();
            last_position = loop_start;
            return;
        }
    }
    last_position = position;
}

void MusicLooper::_on_loop()
{
    loops_done++;
    if (loop_count >= 0 && loops_done >= loop_count)
    {
        _end_looping();
    }
    emit_signal("looped", loops_done);
}

void MusicLooper::_end_looping()
{
    looping = false;
    if (!native_loop)
    {
        return;
    }
    // Le flux en cours de lecture relit ses réglages de boucle : il continue jusqu'à la fin
    Ref<AudioStreamWAV> wav = playback_stream;
    if (wav.is_valid())
    {
        wav->set_loop_mode(AudioStreamWAV::LOOP_DISABLED);
    }
    Ref<AudioStreamOggVorbis> ogg = playback_stream;
    if (ogg.is_valid())
    {
        ogg->set_loop(false);
    }
    Ref<AudioStreamMP3> mp3 = playback_stream;
    if (mp3.is_valid())
    {
        mp3->set_loop(false);
    }
}

// 🔊 Lecteur et volume

AudioStreamPlayer *MusicLooper::_get_player()
{
    if (!player)
    {
        player = memnew(AudioStreamPlayer);
        player->set_name("MusicPlayer");
        player->set_bus(bus);
        player->connect("finished", callable_mp(this, &MusicLooper::_on_player_finished));
        add_child(player, false, Node::INTERNAL_MODE_FRONT);
        _apply_volume();
    }
    return player;
}

void MusicLooper::_set_fade(double p_fade)
{
    fade = p_fade;
    _apply_volume();
}

void MusicLooper::_apply_volume()
{
    if (player)
    {
        // Fondu en amplitude linéaire : plus naturel à l'oreille qu'un fondu en dB
        player->set_volume_db(volume_db + (float)Math::linear2db(Math::max(fade, 0.0001)));
    }
}

void MusicLooper::_on_player_finished() { emit_signal("finished"); }

// 🎛️ Réglages

void MusicLooper::set_stream(const Ref<AudioStream> &p_value)
{
    const bool was_playing = is_playing();
    stream = p_value;
    if (was_playing)
    {
        play(); // nouveau morceau : reprise depuis le début
    }
}

void MusicLooper::set_volume_db(float p_value)
{
    volume_db = p_value;
    _apply_volume();
}

void MusicLooper::set_bus(const StringName &p_value)
{
    bus = p_value;
    if (player)
    {
        player->set_bus(bus);
    }
}

// 🔗 Liaisons

void MusicLooper::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("play", "from_position", "fade_in"), &MusicLooper::play, DEFVAL(0.0), DEFVAL(0.0));
    ClassDB::bind_method(D_METHOD("stop", "fade_out"), &MusicLooper::stop, DEFVAL(0.0));
    ClassDB::bind_method(D_METHOD("set_paused", "paused"), &MusicLooper::set_paused);
    ClassDB::bind_method(D_METHOD("is_paused"), &MusicLooper::is_paused);
    ClassDB::bind_method(D_METHOD("is_playing"), &MusicLooper::is_playing);
    ClassDB::bind_method(D_METHOD("get_playback_position"), &MusicLooper::get_playback_position);
    ClassDB::bind_method(D_METHOD("get_loops_done"), &MusicLooper::get_loops_done);
    ClassDB::bind_method(D_METHOD("is_loop_sample_accurate"), &MusicLooper::is_loop_sample_accurate);
    ClassDB::bind_method(D_METHOD("get_effective_loop_end"), &MusicLooper::get_effective_loop_end);
    ClassDB::bind_method(D_METHOD("set_stream", "value"), &MusicLooper::set_stream);
    ClassDB::bind_method(D_METHOD("get_stream"), &MusicLooper::get_stream);
    ClassDB::bind_method(D_METHOD("set_volume_db", "value"), &MusicLooper::set_volume_db);
    ClassDB::bind_method(D_METHOD("get_volume_db"), &MusicLooper::get_volume_db);
    ClassDB::bind_method(D_METHOD("set_bus", "value"), &MusicLooper::set_bus);
    ClassDB::bind_method(D_METHOD("get_bus"), &MusicLooper::get_bus);
    ClassDB::bind_method(D_METHOD("set_autoplay", "value"), &MusicLooper::set_autoplay);
    ClassDB::bind_method(D_METHOD("get_autoplay"), &MusicLooper::get_autoplay);
    ClassDB::bind_method(D_METHOD("set_loop_enabled", "value"), &MusicLooper::set_loop_enabled);
    ClassDB::bind_method(D_METHOD("get_loop_enabled"), &MusicLooper::get_loop_enabled);
    ClassDB::bind_method(D_METHOD("set_loop_start", "value"), &MusicLooper::set_loop_start);
    ClassDB::bind_method(D_METHOD("get_loop_start"), &MusicLooper::get_loop_start);
    ClassDB::bind_method(D_METHOD("set_loop_end", "value"), &MusicLooper::set_loop_end);
    ClassDB::bind_method(D_METHOD("get_loop_end"), &MusicLooper::get_loop_end);
    ClassDB::bind_method(D_METHOD("set_loop_count", "value"), &MusicLooper::set_loop_count);
    ClassDB::bind_method(D_METHOD("get_loop_count"), &MusicLooper::get_loop_count);
    ClassDB::bind_method(D_METHOD("_get_play_button"), &MusicLooper::_get_play_button);
    ClassDB::bind_method(D_METHOD("_get_stop_button"), &MusicLooper::_get_stop_button);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"), "set_stream", "get_stream");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_db", PROPERTY_HINT_RANGE, "-80,24,0.1,suffix:dB"), "set_volume_db", "get_volume_db");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "bus"), "set_bus", "get_bus");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");
    ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "listen", PROPERTY_HINT_TOOL_BUTTON, U"Écouter,Play", PROPERTY_USAGE_EDITOR), "", "_get_play_button");
    ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "stop_listening", PROPERTY_HINT_TOOL_BUTTON, U"Arrêter,Stop", PROPERTY_USAGE_EDITOR), "", "_get_stop_button");

    ADD_GROUP("Loop", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loop_enabled"), "set_loop_enabled", "get_loop_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "loop_start", PROPERTY_HINT_RANGE, "0,600,0.001,or_greater,suffix:s"), "set_loop_start", "get_loop_start");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "loop_end", PROPERTY_HINT_RANGE, "0,600,0.001,or_greater,suffix:s"), "set_loop_end", "get_loop_end");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "loop_count", PROPERTY_HINT_RANGE, "-1,100,1,or_greater"), "set_loop_count", "get_loop_count");

    ADD_SIGNAL(MethodInfo("looped", PropertyInfo(Variant::INT, "loops_done")));
    ADD_SIGNAL(MethodInfo("finished"));
}
