#include "fx_audio.hpp"

#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_player2d.hpp>
#include <godot_cpp/classes/audio_stream_player3d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void FXAudio::_play(Node *p_target)
{
    if (stream.is_null() || !p_target)
    {
        return;
    }

    Node *player = _get_or_create_player(p_target);
    // Réglages appliqués à chaque jeu : ils ont pu changer depuis la création du lecteur
    const float pitch = randomize_pitch_scale ? (float)UtilityFunctions::randf_range(min_pitch, max_pitch) : pitch_scale;
    player->set("stream", stream);
    player->set("bus", bus);
    player->set("volume_db", volume_db);
    player->set("pitch_scale", pitch);
    player->call("play");
}

void FXAudio::_stop(Node * /*p_target*/)
{
    Node *player = get_audio_player();
    if (stop_sound_on_stop && player)
    {
        player->call("stop");
    }
}

Node *FXAudio::get_audio_player() const { return Object::cast_to<Node>(ObjectDB::get_instance(audio_player_id)); }

Node *FXAudio::_get_or_create_player(Node *p_target)
{
    // Nouveau lecteur si la cible a changé ou si l'ancien a été libéré avec elle
    Node *player = get_audio_player();
    if (player && player->get_parent() == p_target)
    {
        return player;
    }

    if (positional && Object::cast_to<Node2D>(p_target))
    {
        player = memnew(AudioStreamPlayer2D);
    }
    else if (positional && Object::cast_to<Node3D>(p_target))
    {
        player = memnew(AudioStreamPlayer3D);
    }
    else
    {
        player = memnew(AudioStreamPlayer);
    }
    player->set_name("FXAudioPlayer");
    // Nœud interne : n'apparaît pas dans l'arbre de scène édité et n'est pas enregistré
    p_target->add_child(player, false, Node::INTERNAL_MODE_BACK);
    audio_player_id = player->get_instance_id();
    return player;
}

// 🔗 Liaisons

void FXAudio::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_stream", "value"), &FXAudio::set_stream);
    ClassDB::bind_method(D_METHOD("get_stream"), &FXAudio::get_stream);
    ClassDB::bind_method(D_METHOD("set_bus", "value"), &FXAudio::set_bus);
    ClassDB::bind_method(D_METHOD("get_bus"), &FXAudio::get_bus);
    ClassDB::bind_method(D_METHOD("set_volume_db", "value"), &FXAudio::set_volume_db);
    ClassDB::bind_method(D_METHOD("get_volume_db"), &FXAudio::get_volume_db);
    ClassDB::bind_method(D_METHOD("set_pitch_scale", "value"), &FXAudio::set_pitch_scale);
    ClassDB::bind_method(D_METHOD("get_pitch_scale"), &FXAudio::get_pitch_scale);
    ClassDB::bind_method(D_METHOD("set_positional", "value"), &FXAudio::set_positional);
    ClassDB::bind_method(D_METHOD("get_positional"), &FXAudio::get_positional);
    ClassDB::bind_method(D_METHOD("set_stop_sound_on_stop", "value"), &FXAudio::set_stop_sound_on_stop);
    ClassDB::bind_method(D_METHOD("get_stop_sound_on_stop"), &FXAudio::get_stop_sound_on_stop);
    ClassDB::bind_method(D_METHOD("set_randomize_pitch_scale", "value"), &FXAudio::set_randomize_pitch_scale);
    ClassDB::bind_method(D_METHOD("get_randomize_pitch_scale"), &FXAudio::get_randomize_pitch_scale);
    ClassDB::bind_method(D_METHOD("set_min_pitch", "value"), &FXAudio::set_min_pitch);
    ClassDB::bind_method(D_METHOD("get_min_pitch"), &FXAudio::get_min_pitch);
    ClassDB::bind_method(D_METHOD("set_max_pitch", "value"), &FXAudio::set_max_pitch);
    ClassDB::bind_method(D_METHOD("get_max_pitch"), &FXAudio::get_max_pitch);
    ClassDB::bind_method(D_METHOD("get_audio_player"), &FXAudio::get_audio_player);

    ADD_GROUP("Audio", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"), "set_stream", "get_stream");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "bus"), "set_bus", "get_bus");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_db", PROPERTY_HINT_RANGE, "-80,24,0.1,suffix:dB"), "set_volume_db", "get_volume_db");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch_scale", PROPERTY_HINT_RANGE, "0.1,4,0.01"), "set_pitch_scale", "get_pitch_scale");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "positional"), "set_positional", "get_positional");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "stop_sound_on_stop"), "set_stop_sound_on_stop", "get_stop_sound_on_stop");

    ADD_GROUP("Pitch Randomization", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "randomize_pitch_scale"), "set_randomize_pitch_scale", "get_randomize_pitch_scale");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_pitch", PROPERTY_HINT_RANGE, "0.1,4,0.01"), "set_min_pitch", "get_min_pitch");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_pitch", PROPERTY_HINT_RANGE, "0.1,4,0.01"), "set_max_pitch", "get_max_pitch");
}
