#include "ability_resources.hpp"
#include "ability_system.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
String tags_hint() { return String::num_int64(Variant::STRING) + "/" + String::num_int64(PROPERTY_HINT_NONE) + ":"; }
} // namespace

// 📊 AttributeDefinition

void AttributeDefinition::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_attribute_name", "name"), &AttributeDefinition::set_attribute_name);
    ClassDB::bind_method(D_METHOD("get_attribute_name"), &AttributeDefinition::get_attribute_name);
    ClassDB::bind_method(D_METHOD("set_base_value", "value"), &AttributeDefinition::set_base_value);
    ClassDB::bind_method(D_METHOD("get_base_value"), &AttributeDefinition::get_base_value);
    ClassDB::bind_method(D_METHOD("set_min_value", "value"), &AttributeDefinition::set_min_value);
    ClassDB::bind_method(D_METHOD("get_min_value"), &AttributeDefinition::get_min_value);
    ClassDB::bind_method(D_METHOD("set_max_value", "value"), &AttributeDefinition::set_max_value);
    ClassDB::bind_method(D_METHOD("get_max_value"), &AttributeDefinition::get_max_value);
    ClassDB::bind_method(D_METHOD("set_max_attribute", "name"), &AttributeDefinition::set_max_attribute);
    ClassDB::bind_method(D_METHOD("get_max_attribute"), &AttributeDefinition::get_max_attribute);

    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "attribute_name"), "set_attribute_name", "get_attribute_name");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_value"), "set_base_value", "get_base_value");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_value"), "set_min_value", "get_min_value");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_value"), "set_max_value", "get_max_value");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "max_attribute"), "set_max_attribute", "get_max_attribute");
}

// ➕ AttributeModifier

void AttributeModifier::set_magnitude_expression(const String &p_expression)
{
    magnitude_expression = p_expression;
    expression.unref();
    error = String();
    emit_changed();
}

double AttributeModifier::compute_magnitude(AbilitySystem *p_source, AbilitySystem *p_target, double p_level, int p_stacks)
{
    if (magnitude_expression.strip_edges().is_empty())
    {
        return magnitude + magnitude_per_level * (p_level - 1.0);
    }
    if (expression.is_null())
    {
        expression.instantiate();
        PackedStringArray names;
        names.push_back("source");
        names.push_back("target");
        names.push_back("level");
        names.push_back("stacks");
        if (expression->parse(magnitude_expression, names) != OK)
        {
            error = expression->get_error_text();
            UtilityFunctions::push_warning(U"AttributeModifier : expression invalide « " + magnitude_expression + U" » : " + error);
        }
    }
    if (!error.is_empty())
    {
        return 0.0;
    }
    Array inputs;
    inputs.push_back(p_source);
    inputs.push_back(p_target);
    inputs.push_back(p_level);
    inputs.push_back(p_stacks);
    Variant result = expression->execute(inputs, p_target, false);
    if (expression->has_execute_failed())
    {
        UtilityFunctions::push_warning(U"AttributeModifier : échec de « " + magnitude_expression + U" » : " + expression->get_error_text());
        return 0.0;
    }
    return double(result);
}

void AttributeModifier::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_attribute", "attribute"), &AttributeModifier::set_attribute);
    ClassDB::bind_method(D_METHOD("get_attribute"), &AttributeModifier::get_attribute);
    ClassDB::bind_method(D_METHOD("set_operation", "operation"), &AttributeModifier::set_operation);
    ClassDB::bind_method(D_METHOD("get_operation"), &AttributeModifier::get_operation);
    ClassDB::bind_method(D_METHOD("set_magnitude", "magnitude"), &AttributeModifier::set_magnitude);
    ClassDB::bind_method(D_METHOD("get_magnitude"), &AttributeModifier::get_magnitude);
    ClassDB::bind_method(D_METHOD("set_magnitude_per_level", "value"), &AttributeModifier::set_magnitude_per_level);
    ClassDB::bind_method(D_METHOD("get_magnitude_per_level"), &AttributeModifier::get_magnitude_per_level);
    ClassDB::bind_method(D_METHOD("set_magnitude_expression", "expression"), &AttributeModifier::set_magnitude_expression);
    ClassDB::bind_method(D_METHOD("get_magnitude_expression"), &AttributeModifier::get_magnitude_expression);
    ClassDB::bind_method(D_METHOD("compute_magnitude", "source", "target", "level", "stacks"), &AttributeModifier::compute_magnitude, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("get_error"), &AttributeModifier::get_error);

    BIND_ENUM_CONSTANT(OPERATION_ADD);
    BIND_ENUM_CONSTANT(OPERATION_MULTIPLY);
    BIND_ENUM_CONSTANT(OPERATION_OVERRIDE);

    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "attribute"), "set_attribute", "get_attribute");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "operation", PROPERTY_HINT_ENUM, "Add,Multiply,Override"), "set_operation", "get_operation");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "magnitude"), "set_magnitude", "get_magnitude");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "magnitude_per_level"), "set_magnitude_per_level", "get_magnitude_per_level");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "magnitude_expression", PROPERTY_HINT_EXPRESSION), "set_magnitude_expression", "get_magnitude_expression");
}

// ✨ GameplayEffect

void GameplayEffect::notify_applied(AbilitySystem *p_system, Object *p_source) { GDVIRTUAL_CALL(_on_applied, p_system, p_source); }
void GameplayEffect::notify_removed(AbilitySystem *p_system) { GDVIRTUAL_CALL(_on_removed, p_system); }
void GameplayEffect::notify_period(AbilitySystem *p_system) { GDVIRTUAL_CALL(_on_period, p_system); }

void GameplayEffect::_validate_property(PropertyInfo &p_property) const
{
    // Champs sans objet pour un effet instantané : masqués dans l'inspecteur.
    if (duration_policy == DURATION_INSTANT && (p_property.name == StringName("period") || p_property.name == StringName("execute_period_on_apply") || p_property.name == StringName("granted_tags") || p_property.name == StringName("stacking") || p_property.name == StringName("max_stacks")))
    {
        p_property.usage = PROPERTY_USAGE_NO_EDITOR;
    }
    if (duration_policy != DURATION_TIMED && p_property.name == StringName("duration"))
    {
        p_property.usage = PROPERTY_USAGE_NO_EDITOR;
    }
}

void GameplayEffect::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_duration_policy", "policy"), &GameplayEffect::set_duration_policy);
    ClassDB::bind_method(D_METHOD("get_duration_policy"), &GameplayEffect::get_duration_policy);
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &GameplayEffect::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &GameplayEffect::get_duration);
    ClassDB::bind_method(D_METHOD("set_period", "period"), &GameplayEffect::set_period);
    ClassDB::bind_method(D_METHOD("get_period"), &GameplayEffect::get_period);
    ClassDB::bind_method(D_METHOD("set_execute_period_on_apply", "execute"), &GameplayEffect::set_execute_period_on_apply);
    ClassDB::bind_method(D_METHOD("get_execute_period_on_apply"), &GameplayEffect::get_execute_period_on_apply);
    ClassDB::bind_method(D_METHOD("set_modifiers", "modifiers"), &GameplayEffect::set_modifiers);
    ClassDB::bind_method(D_METHOD("get_modifiers"), &GameplayEffect::get_modifiers);
    ClassDB::bind_method(D_METHOD("set_effect_tags", "tags"), &GameplayEffect::set_effect_tags);
    ClassDB::bind_method(D_METHOD("get_effect_tags"), &GameplayEffect::get_effect_tags);
    ClassDB::bind_method(D_METHOD("set_granted_tags", "tags"), &GameplayEffect::set_granted_tags);
    ClassDB::bind_method(D_METHOD("get_granted_tags"), &GameplayEffect::get_granted_tags);
    ClassDB::bind_method(D_METHOD("set_required_tags", "tags"), &GameplayEffect::set_required_tags);
    ClassDB::bind_method(D_METHOD("get_required_tags"), &GameplayEffect::get_required_tags);
    ClassDB::bind_method(D_METHOD("set_blocked_by_tags", "tags"), &GameplayEffect::set_blocked_by_tags);
    ClassDB::bind_method(D_METHOD("get_blocked_by_tags"), &GameplayEffect::get_blocked_by_tags);
    ClassDB::bind_method(D_METHOD("set_remove_effects_with_tags", "tags"), &GameplayEffect::set_remove_effects_with_tags);
    ClassDB::bind_method(D_METHOD("get_remove_effects_with_tags"), &GameplayEffect::get_remove_effects_with_tags);
    ClassDB::bind_method(D_METHOD("set_stacking", "stacking"), &GameplayEffect::set_stacking);
    ClassDB::bind_method(D_METHOD("get_stacking"), &GameplayEffect::get_stacking);
    ClassDB::bind_method(D_METHOD("set_max_stacks", "max"), &GameplayEffect::set_max_stacks);
    ClassDB::bind_method(D_METHOD("get_max_stacks"), &GameplayEffect::get_max_stacks);
    ClassDB::bind_method(D_METHOD("is_periodic"), &GameplayEffect::is_periodic);

    GDVIRTUAL_BIND(_on_applied, "system", "source");
    GDVIRTUAL_BIND(_on_removed, "system");
    GDVIRTUAL_BIND(_on_period, "system");

    BIND_ENUM_CONSTANT(DURATION_INSTANT);
    BIND_ENUM_CONSTANT(DURATION_TIMED);
    BIND_ENUM_CONSTANT(DURATION_INFINITE);
    BIND_ENUM_CONSTANT(STACKING_SEPARATE);
    BIND_ENUM_CONSTANT(STACKING_REFRESH);
    BIND_ENUM_CONSTANT(STACKING_STACK);

    ADD_GROUP(U"⏱️ Durée", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "duration_policy", PROPERTY_HINT_ENUM, "Instant,Timed,Infinite"), "set_duration_policy", "get_duration_policy");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0,600,0.01,or_greater,suffix:s"), "set_duration", "get_duration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "period", PROPERTY_HINT_RANGE, "0,60,0.01,or_greater,suffix:s"), "set_period", "get_period");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "execute_period_on_apply"), "set_execute_period_on_apply", "get_execute_period_on_apply");
    ADD_GROUP(U"➕ Modificateurs", "");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "modifiers", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":AttributeModifier"), "set_modifiers", "get_modifiers");
    ADD_GROUP(U"🏷️ Tags", "");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "effect_tags", PROPERTY_HINT_TYPE_STRING, tags_hint()), "set_effect_tags", "get_effect_tags");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "granted_tags", PROPERTY_HINT_TYPE_STRING, tags_hint()), "set_granted_tags", "get_granted_tags");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "required_tags", PROPERTY_HINT_TYPE_STRING, tags_hint()), "set_required_tags", "get_required_tags");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "blocked_by_tags", PROPERTY_HINT_TYPE_STRING, tags_hint()), "set_blocked_by_tags", "get_blocked_by_tags");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "remove_effects_with_tags", PROPERTY_HINT_TYPE_STRING, tags_hint()), "set_remove_effects_with_tags", "get_remove_effects_with_tags");
    ADD_GROUP(U"📚 Cumul", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "stacking", PROPERTY_HINT_ENUM, "Separate,Refresh,Stack"), "set_stacking", "get_stacking");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_stacks", PROPERTY_HINT_RANGE, "1,99,1,or_greater"), "set_max_stacks", "get_max_stacks");
}

// 🌟 Ability

StringName Ability::get_effective_id() const
{
    if (!String(ability_id).is_empty())
    {
        return ability_id;
    }
    if (!get_name().is_empty())
    {
        return StringName(get_name());
    }
    return StringName(get_path().get_file().get_basename());
}

bool Ability::script_can_activate(AbilitySystem *p_system, Node *p_target)
{
    bool allowed = true;
    return !GDVIRTUAL_CALL(_can_activate, p_system, p_target, allowed) || allowed;
}

void Ability::notify_activate(AbilitySystem *p_system, Node *p_target) { GDVIRTUAL_CALL(_on_activate, p_system, p_target); }
void Ability::notify_commit(AbilitySystem *p_system, Node *p_target) { GDVIRTUAL_CALL(_on_commit, p_system, p_target); }
void Ability::notify_tick(AbilitySystem *p_system, double p_delta) { GDVIRTUAL_CALL(_on_tick, p_system, p_delta); }
void Ability::notify_end(AbilitySystem *p_system, bool p_cancelled) { GDVIRTUAL_CALL(_on_end, p_system, p_cancelled); }

void Ability::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_ability_id", "id"), &Ability::set_ability_id);
    ClassDB::bind_method(D_METHOD("get_ability_id"), &Ability::get_ability_id);
    ClassDB::bind_method(D_METHOD("get_effective_id"), &Ability::get_effective_id);
    ClassDB::bind_method(D_METHOD("set_display_name", "name"), &Ability::set_display_name);
    ClassDB::bind_method(D_METHOD("get_display_name"), &Ability::get_display_name);
    ClassDB::bind_method(D_METHOD("set_icon", "icon"), &Ability::set_icon);
    ClassDB::bind_method(D_METHOD("get_icon"), &Ability::get_icon);
    ClassDB::bind_method(D_METHOD("set_cooldown", "cooldown"), &Ability::set_cooldown);
    ClassDB::bind_method(D_METHOD("get_cooldown"), &Ability::get_cooldown);
    ClassDB::bind_method(D_METHOD("set_max_charges", "charges"), &Ability::set_max_charges);
    ClassDB::bind_method(D_METHOD("get_max_charges"), &Ability::get_max_charges);
    ClassDB::bind_method(D_METHOD("set_costs", "costs"), &Ability::set_costs);
    ClassDB::bind_method(D_METHOD("get_costs"), &Ability::get_costs);
    ClassDB::bind_method(D_METHOD("set_cast_time", "time"), &Ability::set_cast_time);
    ClassDB::bind_method(D_METHOD("get_cast_time"), &Ability::get_cast_time);
    ClassDB::bind_method(D_METHOD("set_active_duration", "duration"), &Ability::set_active_duration);
    ClassDB::bind_method(D_METHOD("get_active_duration"), &Ability::get_active_duration);
    ClassDB::bind_method(D_METHOD("set_level", "level"), &Ability::set_level);
    ClassDB::bind_method(D_METHOD("get_level"), &Ability::get_level);
    ClassDB::bind_method(D_METHOD("set_effects_on_self", "effects"), &Ability::set_effects_on_self);
    ClassDB::bind_method(D_METHOD("get_effects_on_self"), &Ability::get_effects_on_self);
    ClassDB::bind_method(D_METHOD("set_effects_on_target", "effects"), &Ability::set_effects_on_target);
    ClassDB::bind_method(D_METHOD("get_effects_on_target"), &Ability::get_effects_on_target);
    ClassDB::bind_method(D_METHOD("set_requires_target", "requires"), &Ability::set_requires_target);
    ClassDB::bind_method(D_METHOD("get_requires_target"), &Ability::get_requires_target);
    ClassDB::bind_method(D_METHOD("set_ability_tags", "tags"), &Ability::set_ability_tags);
    ClassDB::bind_method(D_METHOD("get_ability_tags"), &Ability::get_ability_tags);
    ClassDB::bind_method(D_METHOD("set_activation_required_tags", "tags"), &Ability::set_activation_required_tags);
    ClassDB::bind_method(D_METHOD("get_activation_required_tags"), &Ability::get_activation_required_tags);
    ClassDB::bind_method(D_METHOD("set_activation_blocked_tags", "tags"), &Ability::set_activation_blocked_tags);
    ClassDB::bind_method(D_METHOD("get_activation_blocked_tags"), &Ability::get_activation_blocked_tags);
    ClassDB::bind_method(D_METHOD("set_activation_owned_tags", "tags"), &Ability::set_activation_owned_tags);
    ClassDB::bind_method(D_METHOD("get_activation_owned_tags"), &Ability::get_activation_owned_tags);
    ClassDB::bind_method(D_METHOD("set_cancel_abilities_with_tags", "tags"), &Ability::set_cancel_abilities_with_tags);
    ClassDB::bind_method(D_METHOD("get_cancel_abilities_with_tags"), &Ability::get_cancel_abilities_with_tags);

    GDVIRTUAL_BIND(_can_activate, "system", "target");
    GDVIRTUAL_BIND(_on_activate, "system", "target");
    GDVIRTUAL_BIND(_on_commit, "system", "target");
    GDVIRTUAL_BIND(_on_tick, "system", "delta");
    GDVIRTUAL_BIND(_on_end, "system", "cancelled");

    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "ability_id"), "set_ability_id", "get_ability_id");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "icon", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_icon", "get_icon");
    ADD_GROUP(U"⏳ Activation", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cooldown", PROPERTY_HINT_RANGE, "0,600,0.01,or_greater,suffix:s"), "set_cooldown", "get_cooldown");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_charges", PROPERTY_HINT_RANGE, "1,20,1,or_greater"), "set_max_charges", "get_max_charges");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "costs", PROPERTY_HINT_DICTIONARY_TYPE, "StringName;float"), "set_costs", "get_costs");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cast_time", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater,suffix:s"), "set_cast_time", "get_cast_time");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "active_duration", PROPERTY_HINT_RANGE, "-1,60,0.01,or_greater,suffix:s"), "set_active_duration", "get_active_duration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "level", PROPERTY_HINT_RANGE, "1,100,1,or_greater"), "set_level", "get_level");
    ADD_GROUP(U"✨ Effets", "");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "effects_on_self", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":GameplayEffect"), "set_effects_on_self", "get_effects_on_self");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "effects_on_target", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":GameplayEffect"), "set_effects_on_target", "get_effects_on_target");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "requires_target"), "set_requires_target", "get_requires_target");
    ADD_GROUP(U"🏷️ Tags", "");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "ability_tags", PROPERTY_HINT_TYPE_STRING, tags_hint()), "set_ability_tags", "get_ability_tags");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "activation_required_tags", PROPERTY_HINT_TYPE_STRING, tags_hint()), "set_activation_required_tags", "get_activation_required_tags");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "activation_blocked_tags", PROPERTY_HINT_TYPE_STRING, tags_hint()), "set_activation_blocked_tags", "get_activation_blocked_tags");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "activation_owned_tags", PROPERTY_HINT_TYPE_STRING, tags_hint()), "set_activation_owned_tags", "get_activation_owned_tags");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "cancel_abilities_with_tags", PROPERTY_HINT_TYPE_STRING, tags_hint()), "set_cancel_abilities_with_tags", "get_cancel_abilities_with_tags");
}
