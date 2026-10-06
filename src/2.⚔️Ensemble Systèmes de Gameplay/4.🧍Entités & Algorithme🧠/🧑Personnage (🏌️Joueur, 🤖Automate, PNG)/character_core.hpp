#ifndef CHARACTER_CORE_HPP
#define CHARACTER_CORE_HPP

#include "../🌟Système de Capacité/ability_system.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot
{
class HealthComponent;

/// Interface commune de Character2D et Character3D (C++ uniquement) : les contrôleurs, Interactable et les
/// feuilles d'arbre de comportement pilotent un personnage sans savoir s'il est en 2D ou en 3D.
/// Positions en Vector3 (en 2D : x, y, 0) ; entrée de déplacement en Vector2 (2D : x, y ; 3D : x, z du monde).
class CharacterAPI
{
  public:
    virtual ~CharacterAPI() = default;
    virtual Node *as_node() = 0;
    virtual bool is_3d() const = 0;

    virtual void set_move_input(const Vector2 &p_input) = 0;
    virtual Vector2 get_move_input() const = 0;
    virtual void request_jump() = 0;
    virtual void release_jump() = 0;
    virtual void set_running(bool p_running) = 0;
    /// Composant de mouvement qui déplace le personnage en ce moment (nullptr s'il n'en a pas).
    virtual Node *get_active_movement() const = 0;

    virtual Vector3 get_world_position() const = 0;
    /// Entrée de déplacement (normalisée) pour aller vers un point.
    virtual Vector2 direction_input_to(const Vector3 &p_point) const = 0;
    /// Distance dans le plan de déplacement (2D vue de côté : x ; 2D vue de dessus : x, y ; 3D : x, z).
    virtual double planar_distance_to(const Vector3 &p_point) const = 0;
    virtual void face_toward(const Vector3 &p_point) = 0;

    virtual bool is_alive() const = 0;
    virtual bool is_movement_blocked() const = 0;
    virtual StringName get_faction() const = 0;
    virtual bool is_hostile_to(Node *p_other) const = 0;
    virtual AbilitySystem *get_ability_system() const = 0;
    virtual HealthComponent *get_health_component() const = 0;
};

/// Personnage (2D ou 3D) d'un nœud, nullptr si ce n'en est pas un.
CharacterAPI *character_api_of(Node *p_node);
/// Position d'un nœud quelconque en Vector3 (Node2D : x, y, 0 ; Node3D : global_position ; sinon zéro).
Vector3 node_world_position(const Node *p_node);

/// Réglages et état communs aux deux personnages (classe C++ interne).
/// Un personnage n'est qu'un conteneur de composants facultatifs : vie (HealthComponent), déplacement
/// (composants du module Mouvement & Physique, pilotés par nom de méthode), capacités (AbilitySystem),
/// contrôleurs, comportements... Ses propriétés propres sont son identité et sa faction.
struct CharacterCore
{
    String display_name;
    StringName faction = "neutre";
    PackedStringArray hostile_factions;
    PackedStringArray movement_blocking_tags;
    bool expose_components = true;

    Vector2 move_input;
    bool running = false;
    bool jump_held = false;
    Vector2 facing = Vector2(1, 0);

    CharacterCore();

    /// Composants : enfants directs qui ne sont pas des nœuds spatiaux (Node2D, Node3D, Control...).
    static bool is_component(const Node *p_child);
    static TypedArray<Node> get_components(const Node *p_owner);
    static Node *get_component(const Node *p_owner, const StringName &p_class);

    HealthComponent *find_health(const Node *p_owner) const;
    AbilitySystem *find_ability_system(const Node *p_owner) const;
    bool is_alive(const Node *p_owner) const;
    bool is_hostile_to(const Node *p_owner, Node *p_other) const;
    bool is_movement_blocked(const Node *p_owner) const;

    // 🌀 Déplacement : transmis au MovementDirector, sinon à chaque MovementComponent (module Mouvement & Physique).
    Node *find_movement_director(const Node *p_owner) const;
    Node *find_active_movement(const Node *p_owner) const;
    void forward_movement(Node *p_owner);
    void forward_call(Node *p_owner, const StringName &p_method);

    // 🧩 Propriétés des composants, éditables depuis le personnage (« components/<nom>/<propriété> »).
    void get_component_property_list(const Node *p_owner, List<PropertyInfo> *p_list) const;
    bool set_component_property(Node *p_owner, const StringName &p_name, const Variant &p_value);
    bool get_component_property(const Node *p_owner, const StringName &p_name, Variant &r_value) const;
};
} // namespace godot

/// Déclarations communes (dans la classe Character2D / Character3D).
#define CHARACTER_COMMON_DECLARATIONS()                                                                             \
  public:                                                                                                           \
    void set_display_name(const String &p_name) { core.display_name = p_name; }                                     \
    String get_display_name() const { return core.display_name; }                                                   \
    void set_faction(const StringName &p_faction) { core.faction = p_faction; }                                     \
    StringName get_faction() const override { return core.faction; }                                               \
    void set_hostile_factions(const PackedStringArray &p_factions) { core.hostile_factions = p_factions; }           \
    PackedStringArray get_hostile_factions() const { return core.hostile_factions; }                                \
    void set_movement_blocking_tags(const PackedStringArray &p_tags) { core.movement_blocking_tags = p_tags; }       \
    PackedStringArray get_movement_blocking_tags() const { return core.movement_blocking_tags; }                    \
    void set_expose_components(bool p_expose)                                                                       \
    {                                                                                                               \
        core.expose_components = p_expose;                                                                          \
        notify_property_list_changed();                                                                             \
    }                                                                                                               \
    bool get_expose_components() const { return core.expose_components; }                                           \
                                                                                                                    \
    Node *as_node() override { return this; }                                                                       \
    void set_move_input(const Vector2 &p_input) override                                                            \
    {                                                                                                               \
        core.move_input = p_input.limit_length(1.0f);                                                               \
        core.forward_movement(this);                                                                                \
    }                                                                                                               \
    Vector2 get_move_input() const override { return core.move_input; }                                             \
    void request_jump() override                                                                                    \
    {                                                                                                               \
        core.jump_held = true;                                                                                      \
        if (!core.is_movement_blocked(this))                                                                        \
        {                                                                                                           \
            core.forward_call(this, "jump");                                                                        \
        }                                                                                                           \
    }                                                                                                               \
    void release_jump() override                                                                                    \
    {                                                                                                               \
        core.jump_held = false;                                                                                     \
        core.forward_call(this, "release_jump");                                                                    \
    }                                                                                                               \
    void set_running(bool p_running) override                                                                       \
    {                                                                                                               \
        core.running = p_running;                                                                                   \
        core.forward_movement(this);                                                                                \
    }                                                                                                               \
    bool is_running() const { return core.running; }                                                                \
    Node *get_active_movement() const override { return core.find_active_movement(this); }                         \
    Node *get_movement_director() const { return core.find_movement_director(this); }                               \
    Vector2 get_facing() const;                                                                                     \
    bool is_alive() const override { return core.is_alive(this); }                                                  \
    bool is_movement_blocked() const override { return core.is_movement_blocked(this); }                            \
    bool is_hostile_to(Node *p_other) const override { return core.is_hostile_to(this, p_other); }                  \
    AbilitySystem *get_ability_system() const override { return core.find_ability_system(this); }                   \
    HealthComponent *get_health_component() const override { return core.find_health(this); }                       \
    TypedArray<Node> get_components() const { return CharacterCore::get_components(this); }                         \
    Node *get_component(const StringName &p_class) const { return CharacterCore::get_component(this, p_class); }    \
    double take_damage(double p_amount, Object *p_source = nullptr);                                                \
    double heal(double p_amount);                                                                                   \
    double get_health() const;                                                                                      \
    double get_max_health() const;                                                                                  \
    Vector3 get_world_position() const override;                                                                    \
    Vector2 direction_input_to(const Vector3 &p_point) const override;                                              \
    double planar_distance_to(const Vector3 &p_point) const override;                                               \
    void face_toward(const Vector3 &p_point) override;                                                              \
                                                                                                                    \
  protected:                                                                                                        \
    void _get_property_list(List<PropertyInfo> *p_list) const { core.get_component_property_list(this, p_list); }   \
    bool _set(const StringName &p_name, const Variant &p_value) { return core.set_component_property(this, p_name, p_value); } \
    bool _get(const StringName &p_name, Variant &r_value) const { return core.get_component_property(this, p_name, r_value); } \
    CharacterCore core;

namespace godot
{
/// Liaisons communes (propriétés, méthodes) de Character2D / Character3D.
/// (Macros ADD_* inutilisables dans un modèle : appels ClassDB explicites.)
template <typename T>
void bind_character_common()
{
    ClassDB::bind_method(D_METHOD("set_display_name", "name"), &T::set_display_name);
    ClassDB::bind_method(D_METHOD("get_display_name"), &T::get_display_name);
    ClassDB::bind_method(D_METHOD("set_faction", "faction"), &T::set_faction);
    ClassDB::bind_method(D_METHOD("get_faction"), &T::get_faction);
    ClassDB::bind_method(D_METHOD("set_hostile_factions", "factions"), &T::set_hostile_factions);
    ClassDB::bind_method(D_METHOD("get_hostile_factions"), &T::get_hostile_factions);
    ClassDB::bind_method(D_METHOD("set_movement_blocking_tags", "tags"), &T::set_movement_blocking_tags);
    ClassDB::bind_method(D_METHOD("get_movement_blocking_tags"), &T::get_movement_blocking_tags);
    ClassDB::bind_method(D_METHOD("set_expose_components", "expose"), &T::set_expose_components);
    ClassDB::bind_method(D_METHOD("get_expose_components"), &T::get_expose_components);

    ClassDB::bind_method(D_METHOD("set_move_input", "input"), &T::set_move_input);
    ClassDB::bind_method(D_METHOD("get_move_input"), &T::get_move_input);
    ClassDB::bind_method(D_METHOD("request_jump"), &T::request_jump);
    ClassDB::bind_method(D_METHOD("release_jump"), &T::release_jump);
    ClassDB::bind_method(D_METHOD("set_running", "running"), &T::set_running);
    ClassDB::bind_method(D_METHOD("is_running"), &T::is_running);
    ClassDB::bind_method(D_METHOD("get_active_movement"), &T::get_active_movement);
    ClassDB::bind_method(D_METHOD("get_movement_director"), &T::get_movement_director);
    ClassDB::bind_method(D_METHOD("get_facing"), &T::get_facing);
    ClassDB::bind_method(D_METHOD("is_alive"), &T::is_alive);
    ClassDB::bind_method(D_METHOD("is_movement_blocked"), &T::is_movement_blocked);
    ClassDB::bind_method(D_METHOD("is_hostile_to", "other"), &T::is_hostile_to);
    ClassDB::bind_method(D_METHOD("get_ability_system"), &T::get_ability_system);
    ClassDB::bind_method(D_METHOD("get_health_component"), &T::get_health_component);
    ClassDB::bind_method(D_METHOD("get_components"), &T::get_components);
    ClassDB::bind_method(D_METHOD("get_component", "class_name"), &T::get_component);
    ClassDB::bind_method(D_METHOD("take_damage", "amount", "source"), &T::take_damage, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("heal", "amount"), &T::heal);
    ClassDB::bind_method(D_METHOD("get_health"), &T::get_health);
    ClassDB::bind_method(D_METHOD("get_max_health"), &T::get_max_health);
    ClassDB::bind_method(D_METHOD("get_world_position"), &T::get_world_position);
    ClassDB::bind_method(D_METHOD("direction_input_to", "point"), &T::direction_input_to);
    ClassDB::bind_method(D_METHOD("planar_distance_to", "point"), &T::planar_distance_to);
    ClassDB::bind_method(D_METHOD("face_toward", "point"), &T::face_toward);

    StringName cls = T::get_class_static();
    ClassDB::add_property_group(cls, U"🪪 Identité", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
    ClassDB::add_property(cls, PropertyInfo(Variant::STRING_NAME, "faction"), "set_faction", "get_faction");
    ClassDB::add_property(cls, PropertyInfo(Variant::PACKED_STRING_ARRAY, "hostile_factions"), "set_hostile_factions", "get_hostile_factions");
    ClassDB::add_property_group(cls, U"🧩 Composants", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::PACKED_STRING_ARRAY, "movement_blocking_tags"), "set_movement_blocking_tags", "get_movement_blocking_tags");
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "expose_components"), "set_expose_components", "get_expose_components");
}
} // namespace godot

#endif // CHARACTER_CORE_HPP
