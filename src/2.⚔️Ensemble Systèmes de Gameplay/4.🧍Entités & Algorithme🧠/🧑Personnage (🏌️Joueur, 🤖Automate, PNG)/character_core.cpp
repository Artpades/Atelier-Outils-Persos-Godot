#include "character_core.hpp"
#include "character_2d.hpp"
#include "character_3d.hpp"
#include "health_component.hpp"

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/script.hpp>

using namespace godot;

CharacterAPI *godot::character_api_of(Node *p_node)
{
    if (Character2D *character = Object::cast_to<Character2D>(p_node))
    {
        return character;
    }
    if (Character3D *character = Object::cast_to<Character3D>(p_node))
    {
        return character;
    }
    return nullptr;
}

Vector3 godot::node_world_position(const Node *p_node)
{
    if (const Node2D *node_2d = Object::cast_to<Node2D>(p_node))
    {
        Vector2 position = node_2d->is_inside_tree() ? node_2d->get_global_position() : node_2d->get_position();
        return Vector3(position.x, position.y, 0.0f);
    }
    if (const Node3D *node_3d = Object::cast_to<Node3D>(p_node))
    {
        return node_3d->is_inside_tree() ? node_3d->get_global_position() : node_3d->get_position();
    }
    return Vector3();
}

CharacterCore::CharacterCore()
{
    movement_blocking_tags.push_back("etat.etourdi");
    movement_blocking_tags.push_back("etat.immobile");
}

// 🧩 Composants

bool CharacterCore::is_component(const Node *p_child) { return p_child && !Object::cast_to<CanvasItem>(p_child) && !Object::cast_to<Node3D>(p_child); }

TypedArray<Node> CharacterCore::get_components(const Node *p_owner)
{
    TypedArray<Node> result;
    for (int i = 0; p_owner && i < p_owner->get_child_count(); i++)
    {
        Node *child = p_owner->get_child(i);
        if (is_component(child))
        {
            result.push_back(child);
        }
    }
    return result;
}

Node *CharacterCore::get_component(const Node *p_owner, const StringName &p_class)
{
    for (int i = 0; p_owner && i < p_owner->get_child_count(); i++)
    {
        Node *child = p_owner->get_child(i);
        if (!is_component(child))
        {
            continue;
        }
        Ref<Script> script = child->get_script();
        if (child->is_class(p_class) || (script.is_valid() && script->get_global_name() == p_class))
        {
            return child;
        }
    }
    return nullptr;
}

HealthComponent *CharacterCore::find_health(const Node *p_owner) const { return Object::cast_to<HealthComponent>(get_component(p_owner, "HealthComponent")); }

AbilitySystem *CharacterCore::find_ability_system(const Node *p_owner) const { return Object::cast_to<AbilitySystem>(get_component(p_owner, "AbilitySystem")); }

bool CharacterCore::is_alive(const Node *p_owner) const
{
    HealthComponent *health = find_health(p_owner);
    return !health || health->is_alive(); // sans composant de vie, toujours « vivant »
}

bool CharacterCore::is_hostile_to(const Node *p_owner, Node *p_other) const
{
    CharacterAPI *other = character_api_of(p_other);
    if (!other || p_other == p_owner)
    {
        return false;
    }
    if (hostile_factions.has(other->get_faction()))
    {
        return true;
    }
    PackedStringArray their_hostiles = p_other->call("get_hostile_factions");
    return their_hostiles.has(faction);
}

bool CharacterCore::is_movement_blocked(const Node *p_owner) const
{
    if (!is_alive(p_owner))
    {
        return true;
    }
    AbilitySystem *system = find_ability_system(p_owner);
    return system && system->has_any_tags(movement_blocking_tags);
}

// 🌀 Déplacement (module Mouvement & Physique, par nom de classe et de méthode : aucun lien de code)

Node *CharacterCore::find_movement_director(const Node *p_owner) const { return get_component(p_owner, "MovementDirector"); }

Node *CharacterCore::find_active_movement(const Node *p_owner) const
{
    Node *fallback = nullptr;
    for (int i = 0; p_owner && i < p_owner->get_child_count(); i++)
    {
        Node *child = p_owner->get_child(i);
        if (!child->is_class("MovementComponent"))
        {
            continue;
        }
        if (bool(child->get("active")))
        {
            return child;
        }
        fallback = fallback ? fallback : child;
    }
    return fallback;
}

void CharacterCore::forward_movement(Node *p_owner)
{
    bool blocked = is_movement_blocked(p_owner);
    Vector2 input = blocked ? Vector2() : move_input;
    bool run = running && !blocked;
    if (forwarded_once && input == forwarded_input && run == forwarded_running)
    {
        return; // rien de nouveau : l'intention donnée ailleurs (directement au directeur) est conservée
    }
    forwarded_once = true;
    forwarded_input = input;
    forwarded_running = run;
    if (Node *director = find_movement_director(p_owner))
    {
        director->call("move", input);
        director->call("set_running", run);
        return;
    }
    for (int i = 0; i < p_owner->get_child_count(); i++)
    {
        Node *child = p_owner->get_child(i);
        if (child->is_class("MovementComponent"))
        {
            child->call("move", input);
            child->call("set_running", run);
        }
    }
}

void CharacterCore::forward_call(Node *p_owner, const StringName &p_method)
{
    if (Node *director = find_movement_director(p_owner))
    {
        director->call(p_method);
        return;
    }
    for (int i = 0; i < p_owner->get_child_count(); i++)
    {
        Node *child = p_owner->get_child(i);
        if (child->is_class("MovementComponent"))
        {
            child->call(p_method);
        }
    }
}

// 🧩 Propriétés des composants exposées sur le personnage

namespace
{
const char *const PREFIX = "components/";

bool is_node_property(const String &p_name)
{
    static PackedStringArray node_properties;
    if (node_properties.is_empty())
    {
        TypedArray<Dictionary> list = ClassDBSingleton::get_singleton()->class_get_property_list("Node", false);
        for (int i = 0; i < list.size(); i++)
        {
            node_properties.push_back(Dictionary(list[i])["name"]);
        }
        node_properties.push_back("script");
    }
    return node_properties.has(p_name);
}
} // namespace

void CharacterCore::get_component_property_list(const Node *p_owner, List<PropertyInfo> *p_list) const
{
    if (!expose_components)
    {
        return;
    }
    TypedArray<Node> components = get_components(p_owner);
    for (int c = 0; c < components.size(); c++)
    {
        Node *component = Object::cast_to<Node>(components[c]);
        String prefix = String(PREFIX) + String(component->get_name()) + "/";
        p_list->push_back(PropertyInfo(Variant::NIL, String(U"🧩 ") + String(component->get_name()) + " (" + component->get_class() + ")", PROPERTY_HINT_NONE, prefix, PROPERTY_USAGE_GROUP));
        TypedArray<Dictionary> properties = component->get_property_list();
        for (int i = 0; i < properties.size(); i++)
        {
            Dictionary property = properties[i];
            int usage = property["usage"];
            String name = property["name"];
            if (!(usage & PROPERTY_USAGE_EDITOR) || (usage & (PROPERTY_USAGE_CATEGORY | PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP)) || is_node_property(name))
            {
                continue;
            }
            // Éditable ici, enregistrée par le composant lui-même (pas de doublon dans la scène).
            p_list->push_back(PropertyInfo(Variant::Type(int(property["type"])), prefix + name, PropertyHint(int(property["hint"])), property["hint_string"], PROPERTY_USAGE_EDITOR, property["class_name"]));
        }
    }
}

bool CharacterCore::set_component_property(Node *p_owner, const StringName &p_name, const Variant &p_value)
{
    String name = p_name;
    if (!name.begins_with(PREFIX))
    {
        return false;
    }
    PackedStringArray parts = name.substr(String(PREFIX).length()).split("/", true, 1);
    Node *component = parts.size() == 2 ? p_owner->get_node_or_null(NodePath(parts[0])) : nullptr;
    if (!component || !is_component(component))
    {
        return false;
    }
    component->set(parts[1], p_value);
    return true;
}

bool CharacterCore::get_component_property(const Node *p_owner, const StringName &p_name, Variant &r_value) const
{
    String name = p_name;
    if (!name.begins_with(PREFIX))
    {
        return false;
    }
    PackedStringArray parts = name.substr(String(PREFIX).length()).split("/", true, 1);
    Node *component = parts.size() == 2 ? p_owner->get_node_or_null(NodePath(parts[0])) : nullptr;
    if (!component || !is_component(component))
    {
        return false;
    }
    r_value = component->get(parts[1]);
    return true;
}
