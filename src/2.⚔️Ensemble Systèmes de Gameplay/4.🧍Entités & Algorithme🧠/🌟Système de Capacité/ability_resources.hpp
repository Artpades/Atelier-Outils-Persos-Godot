#ifndef ABILITY_RESOURCES_HPP
#define ABILITY_RESOURCES_HPP

#include <godot_cpp/classes/expression.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
class AbilitySystem;

/// 📊 Définition d'un attribut (vie, mana, force...) d'un AbilitySystem : valeur de base et bornes.
/// max_attribute borne la valeur par un autre attribut (ex. vie ≤ vie_max).
class AttributeDefinition : public Resource
{
    GDCLASS(AttributeDefinition, Resource)

  public:
    void set_attribute_name(const StringName &p_name) { attribute_name = p_name; emit_changed(); }
    StringName get_attribute_name() const { return attribute_name; }
    void set_base_value(double p_value) { base_value = p_value; emit_changed(); }
    double get_base_value() const { return base_value; }
    void set_min_value(double p_value) { min_value = p_value; emit_changed(); }
    double get_min_value() const { return min_value; }
    void set_max_value(double p_value) { max_value = p_value; emit_changed(); }
    double get_max_value() const { return max_value; }
    void set_max_attribute(const StringName &p_name) { max_attribute = p_name; emit_changed(); }
    StringName get_max_attribute() const { return max_attribute; }

  protected:
    static void _bind_methods();

  private:
    StringName attribute_name;
    double base_value = 100.0;
    double min_value = 0.0;
    double max_value = 1000000.0;
    StringName max_attribute;
};

/// ➕ Modification d'un attribut par un GameplayEffect.
/// Valeur : magnitude (+ magnitude_per_level par niveau au-delà de 1), ou magnitude_expression si elle est
/// renseignée. Entrées de l'expression : source et target (AbilitySystem, ex. « source.get_value("force") * 2 »),
/// level et stacks.
class AttributeModifier : public Resource
{
    GDCLASS(AttributeModifier, Resource)

  public:
    enum Operation
    {
        OPERATION_ADD,      // + valeur
        OPERATION_MULTIPLY, // × valeur (1.5 = +50 %)
        OPERATION_OVERRIDE, // = valeur
    };

    void set_attribute(const StringName &p_attribute) { attribute = p_attribute; emit_changed(); }
    StringName get_attribute() const { return attribute; }
    void set_operation(Operation p_operation) { operation = p_operation; emit_changed(); }
    Operation get_operation() const { return operation; }
    void set_magnitude(double p_magnitude) { magnitude = p_magnitude; emit_changed(); }
    double get_magnitude() const { return magnitude; }
    void set_magnitude_per_level(double p_value) { magnitude_per_level = p_value; emit_changed(); }
    double get_magnitude_per_level() const { return magnitude_per_level; }
    void set_magnitude_expression(const String &p_expression);
    String get_magnitude_expression() const { return magnitude_expression; }

    /// Valeur de la modification pour une source, une cible et un niveau donnés.
    double compute_magnitude(AbilitySystem *p_source, AbilitySystem *p_target, double p_level, int p_stacks = 1);
    String get_error() const { return error; }

  protected:
    static void _bind_methods();

  private:
    StringName attribute;
    Operation operation = OPERATION_ADD;
    double magnitude = 0.0;
    double magnitude_per_level = 0.0;
    String magnitude_expression;
    /// Expression analysée une seule fois (remise à zéro par set_magnitude_expression).
    Ref<Expression> expression; // cache d'analyse
    /// Erreur d'analyse de magnitude_expression (la valeur vaut alors 0).
    String error;
};

/// ✨ Effet de jeu appliqué à un AbilitySystem : dégâts, soin, bonus temporaire, poison, étourdissement...
///   - INSTANT : modifie la valeur de BASE des attributs, une fois.
///   - DURATION / INFINITE : modificateurs temporaires (retirés à la fin) et tags accordés tant qu'il est actif ;
///     avec period > 0, les modificateurs s'appliquent à la base à chaque période (poison, régénération).
/// required_tags / blocked_by_tags conditionnent l'application ; remove_effects_with_tags retire d'autres effets.
/// Script : _on_applied(system, source), _on_removed(system), _on_period(system).
class GameplayEffect : public Resource
{
    GDCLASS(GameplayEffect, Resource)

  public:
    /// Instantané (base modifiée une fois), à durée limitée, ou sans fin.
    enum DurationPolicy
    {
        DURATION_INSTANT,
        DURATION_TIMED,
        DURATION_INFINITE,
    };
    /// Réaction à une nouvelle application de la même ressource déjà active.
    enum StackingPolicy
    {
        STACKING_SEPARATE, // chaque application est une instance indépendante
        STACKING_REFRESH,  // une seule instance, durée relancée
        STACKING_STACK,    // une seule instance, cumul (jusqu'à max_stacks) et durée relancée
    };

    void set_duration_policy(DurationPolicy p_policy) { duration_policy = p_policy; notify_property_list_changed(); emit_changed(); }
    DurationPolicy get_duration_policy() const { return duration_policy; }
    void set_duration(double p_duration) { duration = MAX(0.0, p_duration); emit_changed(); }
    double get_duration() const { return duration; }
    void set_period(double p_period) { period = MAX(0.0, p_period); emit_changed(); }
    double get_period() const { return period; }
    void set_execute_period_on_apply(bool p_execute) { execute_period_on_apply = p_execute; emit_changed(); }
    bool get_execute_period_on_apply() const { return execute_period_on_apply; }
    void set_modifiers(const TypedArray<AttributeModifier> &p_modifiers) { modifiers = p_modifiers; emit_changed(); }
    TypedArray<AttributeModifier> get_modifiers() const { return modifiers; }
    void set_effect_tags(const PackedStringArray &p_tags) { effect_tags = p_tags; emit_changed(); }
    PackedStringArray get_effect_tags() const { return effect_tags; }
    void set_granted_tags(const PackedStringArray &p_tags) { granted_tags = p_tags; emit_changed(); }
    PackedStringArray get_granted_tags() const { return granted_tags; }
    void set_required_tags(const PackedStringArray &p_tags) { required_tags = p_tags; emit_changed(); }
    PackedStringArray get_required_tags() const { return required_tags; }
    void set_blocked_by_tags(const PackedStringArray &p_tags) { blocked_by_tags = p_tags; emit_changed(); }
    PackedStringArray get_blocked_by_tags() const { return blocked_by_tags; }
    void set_remove_effects_with_tags(const PackedStringArray &p_tags) { remove_effects_with_tags = p_tags; emit_changed(); }
    PackedStringArray get_remove_effects_with_tags() const { return remove_effects_with_tags; }
    void set_stacking(StackingPolicy p_stacking) { stacking = p_stacking; emit_changed(); }
    StackingPolicy get_stacking() const { return stacking; }
    void set_max_stacks(int p_max) { max_stacks = MAX(1, p_max); emit_changed(); }
    int get_max_stacks() const { return max_stacks; }

    bool is_periodic() const { return duration_policy != DURATION_INSTANT && period > 0.0; }

    // Appels des scripts (par AbilitySystem)
    /// Appellent les méthodes virtuelles de script _on_applied / _on_removed / _on_period.
    void notify_applied(AbilitySystem *p_system, Object *p_source);
    void notify_removed(AbilitySystem *p_system);
    void notify_period(AbilitySystem *p_system);

  protected:
    static void _bind_methods();
    void _validate_property(PropertyInfo &p_property) const;

    GDVIRTUAL2(_on_applied, Node *, Object *)
    GDVIRTUAL1(_on_removed, Node *)
    GDVIRTUAL1(_on_period, Node *)

  private:
    DurationPolicy duration_policy = DURATION_INSTANT;
    double duration = 1.0;
    double period = 0.0;
    bool execute_period_on_apply = false;
    TypedArray<AttributeModifier> modifiers;
    PackedStringArray effect_tags;
    PackedStringArray granted_tags;
    PackedStringArray required_tags;
    PackedStringArray blocked_by_tags;
    PackedStringArray remove_effects_with_tags;
    StackingPolicy stacking = STACKING_SEPARATE;
    int max_stacks = 1;
};

/// 🌟 Capacité (attaque, sort, esquive...) accordée à un AbilitySystem.
///   - Activation : coûts (attribut → quantité), charges et recharge, tags requis / bloquants.
///   - Déroulement : cast_time puis application des effets (sur soi et sur la cible), puis fin après
///     active_duration (0 = immédiate, -1 = jusqu'à end_ability / cancel).
///   - Tags : ability_tags (la décrivent), activation_owned_tags (accordés pendant l'activation),
///     cancel_abilities_with_tags (annule les capacités actives qui les portent).
/// Script : _can_activate(system, target) -> bool, _on_activate(system, target), _on_commit(system, target),
/// _on_tick(system, delta), _on_end(system, cancelled).
class Ability : public Resource
{
    GDCLASS(Ability, Resource)

  public:
    void set_ability_id(const StringName &p_id) { ability_id = p_id; emit_changed(); }
    StringName get_ability_id() const { return ability_id; }
    /// Identifiant effectif : ability_id, sinon resource_name, sinon le nom du fichier.
    StringName get_effective_id() const;
    void set_display_name(const String &p_name) { display_name = p_name; emit_changed(); }
    String get_display_name() const { return display_name; }
    void set_icon(const Ref<Resource> &p_icon) { icon = p_icon; emit_changed(); }
    Ref<Resource> get_icon() const { return icon; }

    void set_cooldown(double p_cooldown) { cooldown = MAX(0.0, p_cooldown); emit_changed(); }
    double get_cooldown() const { return cooldown; }
    void set_max_charges(int p_charges) { max_charges = MAX(1, p_charges); emit_changed(); }
    int get_max_charges() const { return max_charges; }
    void set_costs(const Dictionary &p_costs) { costs = p_costs; emit_changed(); }
    Dictionary get_costs() const { return costs; }
    void set_cast_time(double p_time) { cast_time = MAX(0.0, p_time); emit_changed(); }
    double get_cast_time() const { return cast_time; }
    void set_active_duration(double p_duration) { active_duration = p_duration; emit_changed(); }
    double get_active_duration() const { return active_duration; }
    void set_level(double p_level) { level = p_level; emit_changed(); }
    double get_level() const { return level; }

    void set_effects_on_self(const TypedArray<GameplayEffect> &p_effects) { effects_on_self = p_effects; emit_changed(); }
    TypedArray<GameplayEffect> get_effects_on_self() const { return effects_on_self; }
    void set_effects_on_target(const TypedArray<GameplayEffect> &p_effects) { effects_on_target = p_effects; emit_changed(); }
    TypedArray<GameplayEffect> get_effects_on_target() const { return effects_on_target; }
    void set_requires_target(bool p_requires) { requires_target = p_requires; emit_changed(); }
    bool get_requires_target() const { return requires_target; }

    void set_ability_tags(const PackedStringArray &p_tags) { ability_tags = p_tags; emit_changed(); }
    PackedStringArray get_ability_tags() const { return ability_tags; }
    void set_activation_required_tags(const PackedStringArray &p_tags) { activation_required_tags = p_tags; emit_changed(); }
    PackedStringArray get_activation_required_tags() const { return activation_required_tags; }
    void set_activation_blocked_tags(const PackedStringArray &p_tags) { activation_blocked_tags = p_tags; emit_changed(); }
    PackedStringArray get_activation_blocked_tags() const { return activation_blocked_tags; }
    void set_activation_owned_tags(const PackedStringArray &p_tags) { activation_owned_tags = p_tags; emit_changed(); }
    PackedStringArray get_activation_owned_tags() const { return activation_owned_tags; }
    void set_cancel_abilities_with_tags(const PackedStringArray &p_tags) { cancel_abilities_with_tags = p_tags; emit_changed(); }
    PackedStringArray get_cancel_abilities_with_tags() const { return cancel_abilities_with_tags; }

    // Appels des scripts (par AbilitySystem)
    /// _can_activate du script (vrai sans surcharge), puis notifications _on_activate, _on_commit, _on_tick, _on_end.
    bool script_can_activate(AbilitySystem *p_system, Node *p_target);
    void notify_activate(AbilitySystem *p_system, Node *p_target);
    void notify_commit(AbilitySystem *p_system, Node *p_target);
    void notify_tick(AbilitySystem *p_system, double p_delta);
    void notify_end(AbilitySystem *p_system, bool p_cancelled);

  protected:
    static void _bind_methods();

    GDVIRTUAL2R(bool, _can_activate, Node *, Node *)
    GDVIRTUAL2(_on_activate, Node *, Node *)
    GDVIRTUAL2(_on_commit, Node *, Node *)
    GDVIRTUAL2(_on_tick, Node *, double)
    GDVIRTUAL2(_on_end, Node *, bool)

  private:
    StringName ability_id;
    String display_name;
    Ref<Resource> icon;
    double cooldown = 1.0;
    int max_charges = 1;
    Dictionary costs;
    double cast_time = 0.0;
    double active_duration = 0.0;
    double level = 1.0;
    TypedArray<GameplayEffect> effects_on_self;
    TypedArray<GameplayEffect> effects_on_target;
    bool requires_target = false;
    PackedStringArray ability_tags;
    PackedStringArray activation_required_tags;
    PackedStringArray activation_blocked_tags;
    PackedStringArray activation_owned_tags;
    PackedStringArray cancel_abilities_with_tags;
};
} // namespace godot

VARIANT_ENUM_CAST(AttributeModifier::Operation);
VARIANT_ENUM_CAST(GameplayEffect::DurationPolicy);
VARIANT_ENUM_CAST(GameplayEffect::StackingPolicy);

#endif // ABILITY_RESOURCES_HPP
