#ifndef ABILITY_SYSTEM_HPP
#define ABILITY_SYSTEM_HPP

#include "ability_resources.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
/// 🌟 Système de capacités d'une entité (composant à placer sous le personnage, le monstre, la tourelle...).
///
/// Attributs : valeur de base (modifiée par les effets instantanés et périodiques) et valeur courante
///   = (base + Σ ajouts) × Π multiplicateurs, ou la dernière valeur imposée, bornée par la définition.
/// Tags : comptés (plusieurs sources peuvent accorder le même tag) et hiérarchiques : has_tag("etat")
///   est vrai si « etat.etourdi » est présent.
/// Effets : apply_effect renvoie un identifiant (handle) pour les effets à durée, 0 pour un effet instantané,
///   -1 s'il est bloqué.
/// Capacités : activate(id, cible) vérifie les conditions, paie le coût, consomme une charge, applique les
///   effets (après cast_time), puis termine la capacité.
class AbilitySystem : public Node
{
    GDCLASS(AbilitySystem, Node)

  public:
    // ⚙️ Configuration
    void set_attribute_definitions(const TypedArray<AttributeDefinition> &p_definitions);
    TypedArray<AttributeDefinition> get_attribute_definitions() const { return attribute_definitions; }
    void set_granted_abilities(const TypedArray<Ability> &p_abilities);
    TypedArray<Ability> get_granted_abilities() const { return granted_abilities; }
    void set_initial_tags(const PackedStringArray &p_tags) { initial_tags = p_tags; }
    PackedStringArray get_initial_tags() const { return initial_tags; }
    void set_initial_effects(const TypedArray<GameplayEffect> &p_effects) { initial_effects = p_effects; }
    TypedArray<GameplayEffect> get_initial_effects() const { return initial_effects; }
    void set_manual_update(bool p_manual);
    bool is_manual_update() const { return manual_update; }

    /// (Re)construit attributs, tags, effets et capacités depuis la configuration (fait automatiquement à _ready).
    void initialize();
    /// Avance les effets, recharges et capacités actives (appelé à chaque image sauf en manual_update).
    void update(double p_delta);

    // 📊 Attributs
    bool has_attribute(const StringName &p_name) const { return attributes.has(p_name); }
    double get_value(const StringName &p_name) const;
    double get_base_value(const StringName &p_name) const;
    void set_base_value(const StringName &p_name, double p_value);
    /// Ajoute (positif) ou retire (négatif) à la valeur de base : raccourci pour dégâts et soins.
    void add_to_base(const StringName &p_name, double p_amount) { set_base_value(p_name, get_base_value(p_name) + p_amount); }
    PackedStringArray get_attribute_names() const;
    void add_attribute(const Ref<AttributeDefinition> &p_definition);

    // 🏷️ Tags
    void add_tag(const String &p_tag, int p_count = 1);
    void remove_tag(const String &p_tag, int p_count = 1);
    bool has_tag(const String &p_tag) const;
    bool has_any_tags(const PackedStringArray &p_tags) const;
    bool has_all_tags(const PackedStringArray &p_tags) const;
    PackedStringArray get_tags() const;

    // ✨ Effets
    int apply_effect(const Ref<GameplayEffect> &p_effect, Object *p_source = nullptr, double p_level = 1.0);
    /// Applique un effet à une autre entité (son AbilitySystem), avec ce système comme source.
    int apply_effect_to(const Ref<GameplayEffect> &p_effect, Node *p_target, double p_level = 1.0);
    bool remove_effect(int p_handle);
    int remove_effects_with_tag(const String &p_tag);
    bool has_effect(const Ref<GameplayEffect> &p_effect) const;
    int get_effect_stacks(const Ref<GameplayEffect> &p_effect) const;
    double get_effect_remaining(int p_handle) const;
    /// [{ "handle", "effect", "remaining", "stacks", "source" }]
    Array get_active_effects() const;

    // 🌟 Capacités
    void grant_ability(const Ref<Ability> &p_ability);
    void revoke_ability(const StringName &p_id);
    bool has_ability(const StringName &p_id) const { return abilities.has(p_id); }
    PackedStringArray get_ability_ids() const;
    Ref<Ability> get_ability(const StringName &p_id) const;
    /// Raison d'un refus ("" si l'activation est possible) : unknown, active, cooldown, blocked, missing_tags,
    /// cost, no_target, script.
    String get_activation_failure(const StringName &p_id, Node *p_target = nullptr);
    bool can_activate(const StringName &p_id, Node *p_target = nullptr) { return get_activation_failure(p_id, p_target).is_empty(); }
    bool activate(const StringName &p_id, Node *p_target = nullptr);
    /// Termine normalement une capacité active (active_duration = -1, ou fin anticipée).
    void end_ability(const StringName &p_id);
    void cancel_ability(const StringName &p_id);
    int cancel_abilities_with_tag(const String &p_tag);
    bool is_ability_active(const StringName &p_id) const;
    double get_cooldown_remaining(const StringName &p_id) const;
    int get_charges(const StringName &p_id) const;
    void reset_cooldown(const StringName &p_id);

    /// AbilitySystem d'un nœud : lui-même, un enfant direct, ou celui renvoyé par sa méthode get_ability_system().
    static AbilitySystem *find_on(Node *p_node);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    struct AttributeState
    {
        Ref<AttributeDefinition> definition;
        double base = 0.0;
        double current = 0.0;
    };
    struct ActiveEffect
    {
        int handle = 0;
        Ref<GameplayEffect> effect;
        ObjectID source;
        double level = 1.0;
        double remaining = 0.0;
        double period_timer = 0.0;
        int stacks = 1;
    };
    struct AbilityState
    {
        Ref<Ability> ability;
        double cooldown = 0.0; // temps avant la prochaine charge
        int charges = 1;
        bool active = false;
        bool committed = false;
        double elapsed = 0.0;
        ObjectID target;
    };

    TypedArray<AttributeDefinition> attribute_definitions;
    TypedArray<Ability> granted_abilities;
    PackedStringArray initial_tags;
    TypedArray<GameplayEffect> initial_effects;
    bool manual_update = false;
    bool initialized = false;

    HashMap<StringName, AttributeState> attributes;
    LocalVector<StringName> attribute_order;
    HashMap<String, int> tag_counts;
    LocalVector<ActiveEffect> effects;
    int next_handle = 1;
    int recompute_depth = 0;
    HashMap<StringName, AbilityState> abilities;
    LocalVector<StringName> ability_order;

    double _clamp(const AttributeState &p_state, double p_value) const;
    void _recompute(const StringName &p_name);
    void _recompute_all();
    void _apply_to_base(const Ref<GameplayEffect> &p_effect, AbilitySystem *p_source, double p_level, int p_stacks);
    void _grant_tags(const PackedStringArray &p_tags, int p_sign);
    int _find_effect(const Ref<GameplayEffect> &p_effect) const;
    void _remove_effect_at(int p_index);
    void _commit(const StringName &p_id);
    void _finish(const StringName &p_id, bool p_cancelled);
    static bool _tag_matches(const String &p_owned, const String &p_query);
};
} // namespace godot

#endif // ABILITY_SYSTEM_HPP
