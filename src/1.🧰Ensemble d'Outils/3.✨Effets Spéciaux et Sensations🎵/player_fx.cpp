#include "player_fx.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// 🎯 API publique

void PlayerFX::play(Node *p_node)
{
    Node *target = p_node ? p_node : get_default_target();
    current_target_id = target ? ObjectID(target->get_instance_id()) : ObjectID();

    _link_effects(); // le tableau a pu être modifié en place depuis set_special_effects
    for (int64_t i = 0; i < special_effects.size(); i++)
    {
        Ref<Effect> effect = special_effects[i];
        if (effect.is_valid())
        {
            effect->play_fx(target);
        }
    }
}

void PlayerFX::stop(Node *p_node)
{
    Node *target = p_node ? p_node : _get_current_target();
    for (int64_t i = 0; i < special_effects.size(); i++)
    {
        Ref<Effect> effect = special_effects[i];
        if (effect.is_valid())
        {
            effect->stop_fx(target);
        }
    }
}

// 🔄 Mise à jour

void PlayerFX::_process(double p_delta)
{
    Node *target = _get_current_target();
    for (int64_t i = 0; i < special_effects.size(); i++)
    {
        Ref<Effect> effect = special_effects[i];
        if (effect.is_valid())
        {
            effect->process_fx(p_delta, target);
        }
    }
}

void PlayerFX::_physics_process(double p_delta)
{
    Node *target = _get_current_target();
    for (int64_t i = 0; i < special_effects.size(); i++)
    {
        Ref<Effect> effect = special_effects[i];
        if (effect.is_valid())
        {
            effect->physics_process_fx(p_delta, target);
        }
    }
}

// 🎛️ Cible et effets

void PlayerFX::set_node_to_play_effects(Node *p_node) { node_to_play_effects_id = p_node ? ObjectID(p_node->get_instance_id()) : ObjectID(); }

Node *PlayerFX::get_node_to_play_effects() const { return Object::cast_to<Node>(ObjectDB::get_instance(node_to_play_effects_id)); }

Node *PlayerFX::get_default_target() const
{
    Node *node = get_node_to_play_effects();
    return node ? node : get_parent();
}

Node *PlayerFX::_get_current_target() const
{
    // Sans play() préalable (ou si la cible a été libérée) : la cible par défaut
    Node *target = Object::cast_to<Node>(ObjectDB::get_instance(current_target_id));
    return target ? target : get_default_target();
}

void PlayerFX::set_special_effects(const TypedArray<Effect> &p_effects)
{
    special_effects = p_effects;
    _link_effects();
}

void PlayerFX::_link_effects()
{
    for (int64_t i = 0; i < special_effects.size(); i++)
    {
        Ref<Effect> effect = special_effects[i];
        if (effect.is_valid())
        {
            effect->set_player_fx(this);
        }
    }
}

// 🔗 Liaisons

void PlayerFX::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("play", "node"), &PlayerFX::play, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("stop", "node"), &PlayerFX::stop, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("set_node_to_play_effects", "node"), &PlayerFX::set_node_to_play_effects);
    ClassDB::bind_method(D_METHOD("get_node_to_play_effects"), &PlayerFX::get_node_to_play_effects);
    ClassDB::bind_method(D_METHOD("set_special_effects", "effects"), &PlayerFX::set_special_effects);
    ClassDB::bind_method(D_METHOD("get_special_effects"), &PlayerFX::get_special_effects);
    ClassDB::bind_method(D_METHOD("get_default_target"), &PlayerFX::get_default_target);
    ClassDB::bind_method(D_METHOD("_get_play_button"), &PlayerFX::_get_play_button);
    ClassDB::bind_method(D_METHOD("_get_stop_button"), &PlayerFX::_get_stop_button);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "node_to_play_effects", PROPERTY_HINT_NODE_TYPE, "Node"), "set_node_to_play_effects", "get_node_to_play_effects");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "special_effects", PROPERTY_HINT_ARRAY_TYPE, "Effect"), "set_special_effects", "get_special_effects");
    ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "play_effect", PROPERTY_HINT_TOOL_BUTTON, U"Jouer effet,PlaceholderMaterial", PROPERTY_USAGE_EDITOR), "", "_get_play_button");
    ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "stop_effect", PROPERTY_HINT_TOOL_BUTTON, U"Arrêter effet,Stop", PROPERTY_USAGE_EDITOR), "", "_get_stop_button");
}
