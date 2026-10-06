#include "character_registry.hpp"
#include "character_controllers.hpp"
#include "character_core.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

LocalVector<ObjectID> CharacterRegister::characters;

void CharacterRegister::register_character(Node *p_character)
{
    ObjectID id(p_character->get_instance_id());
    if (characters.find(id) < 0)
    {
        characters.push_back(id);
    }
}

void CharacterRegister::unregister_character(Node *p_character)
{
    int64_t index = characters.find(ObjectID(p_character->get_instance_id()));
    if (index >= 0)
    {
        characters.remove_at(index);
    }
}

Array CharacterRegister::get_characters()
{
    Array result;
    for (uint32_t i = 0; i < characters.size(); i++)
    {
        if (Node *character = Object::cast_to<Node>(ObjectDB::get_instance(characters[i])))
        {
            result.push_back(character);
        }
    }
    return result;
}

Array CharacterRegister::get_faction_members(const StringName &p_faction)
{
    Array result;
    Array all = get_characters();
    for (int i = 0; i < all.size(); i++)
    {
        CharacterAPI *character = character_api_of(Object::cast_to<Node>(all[i]));
        if (character && character->get_faction() == p_faction)
        {
            result.push_back(all[i]);
        }
    }
    return result;
}

Array CharacterRegister::get_in_radius(Node *p_from, double p_radius, bool p_hostile_only, bool p_alive_only)
{
    Array result;
    if (!p_from)
    {
        return result;
    }
    CharacterAPI *from = character_api_of(p_from);
    Vector3 origin = node_world_position(p_from);
    Array all = get_characters();
    for (int i = 0; i < all.size(); i++)
    {
        Node *node = Object::cast_to<Node>(all[i]);
        CharacterAPI *character = character_api_of(node);
        if (!character || node == p_from || (p_alive_only && !character->is_alive()))
        {
            continue;
        }
        if (p_hostile_only && (!from || !from->is_hostile_to(node)))
        {
            continue;
        }
        if (p_radius < 0.0 || origin.distance_to(character->get_world_position()) <= p_radius)
        {
            result.push_back(node);
        }
    }
    return result;
}

Node *CharacterRegister::get_nearest(Node *p_from, double p_max_distance, bool p_hostile_only, bool p_alive_only)
{
    Array candidates = get_in_radius(p_from, p_max_distance, p_hostile_only, p_alive_only);
    Vector3 origin = node_world_position(p_from);
    Node *nearest = nullptr;
    double best = 0.0;
    for (int i = 0; i < candidates.size(); i++)
    {
        Node *node = Object::cast_to<Node>(candidates[i]);
        double distance = origin.distance_to(node_world_position(node));
        if (!nearest || distance < best)
        {
            nearest = node;
            best = distance;
        }
    }
    return nearest;
}

Array CharacterRegister::get_players()
{
    Array result;
    Array all = get_characters();
    for (int i = 0; i < all.size(); i++)
    {
        Node *node = Object::cast_to<Node>(all[i]);
        for (int c = 0; c < node->get_child_count(); c++)
        {
            if (Object::cast_to<PlayerController>(node->get_child(c)))
            {
                result.push_back(node);
                break;
            }
        }
    }
    return result;
}

Node *CharacterRegister::get_first_player()
{
    Array players = get_players();
    return players.is_empty() ? nullptr : Object::cast_to<Node>(players[0]);
}

void CharacterRegister::_bind_methods()
{
    ClassDB::bind_static_method("CharacterRegister", D_METHOD("get_characters"), &CharacterRegister::get_characters);
    ClassDB::bind_static_method("CharacterRegister", D_METHOD("get_faction_members", "faction"), &CharacterRegister::get_faction_members);
    ClassDB::bind_static_method("CharacterRegister", D_METHOD("get_nearest", "from", "max_distance", "hostile_only", "alive_only"), &CharacterRegister::get_nearest, DEFVAL(-1.0), DEFVAL(false), DEFVAL(true));
    ClassDB::bind_static_method("CharacterRegister", D_METHOD("get_in_radius", "from", "radius", "hostile_only", "alive_only"), &CharacterRegister::get_in_radius, DEFVAL(false), DEFVAL(true));
    ClassDB::bind_static_method("CharacterRegister", D_METHOD("get_players"), &CharacterRegister::get_players);
    ClassDB::bind_static_method("CharacterRegister", D_METHOD("get_first_player"), &CharacterRegister::get_first_player);
}
