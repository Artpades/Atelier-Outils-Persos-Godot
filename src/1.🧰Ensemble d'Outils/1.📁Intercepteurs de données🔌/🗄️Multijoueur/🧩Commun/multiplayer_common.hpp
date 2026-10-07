#ifndef MULTIPLAYER_COMMON_HPP
#define MULTIPLAYER_COMMON_HPP

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>

namespace godot
{
/// Fonctions partagées par les classes du module Multijoueur (en-tête seul).
namespace multiplayer_common
{
/// Version du protocole du module (découverte + poignée de main + paquets). À incrémenter à chaque
/// changement incompatible : deux appareils de protocoles différents ne peuvent pas se connecter.
static constexpr int PROTOCOL_VERSION = 1;

/// Identifiant du jeu par défaut : application/config/name du projet.
inline String default_game_id()
{
    return ProjectSettings::get_singleton()->get_setting("application/config/name", "");
}

/// Version du jeu par défaut : application/config/version du projet.
inline String default_game_version()
{
    return ProjectSettings::get_singleton()->get_setting("application/config/version", "");
}

/// Nom d'appareil par défaut : l'utilisateur du système, sinon le modèle, sinon « Joueur ».
inline String default_device_name()
{
    OS *os = OS::get_singleton();
    for (const char *variable : {"USER", "USERNAME"})
    {
        const String value = os->get_environment(variable);
        if (!value.is_empty())
        {
            return value;
        }
    }
    const String model = os->get_model_name();
    return model.is_empty() || model == "GenericDevice" ? String("Joueur") : model;
}

/// Système d'exploitation et matériel de cet appareil : os, os_version, distribution, model, engine_version.
inline Dictionary local_system_info()
{
    OS *os = OS::get_singleton();
    Dictionary info;
    info["os"] = os->get_name();
    info["os_version"] = os->get_version();
    info["distribution"] = os->get_distribution_name();
    info["model"] = os->get_model_name();
    info["engine_version"] = Engine::get_singleton()->get_version_info().get("string", "");
    return info;
}

/// Lit un entier 32 bits non signé (petit-boutiste) à p_offset.
inline uint32_t read_u32(const PackedByteArray &p_bytes, int64_t p_offset)
{
    uint32_t value = 0;
    for (int i = 3; i >= 0; i--)
    {
        value = (value << 8) | p_bytes[p_offset + i];
    }
    return value;
}

/// Ajoute un entier 32 bits (petit-boutiste) à p_bytes.
inline void append_u32(PackedByteArray &p_bytes, uint32_t p_value)
{
    for (int i = 0; i < 4; i++)
    {
        p_bytes.push_back((p_value >> (8 * i)) & 0xFF);
    }
}
} // namespace multiplayer_common
} // namespace godot

#endif // MULTIPLAYER_COMMON_HPP
