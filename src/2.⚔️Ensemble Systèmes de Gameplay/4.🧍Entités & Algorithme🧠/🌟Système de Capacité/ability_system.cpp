#include "ability_system.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

// ⚙️ Configuration

void AbilitySystem::set_attribute_definitions(const TypedArray<AttributeDefinition> &p_definitions) { attribute_definitions = p_definitions; }

void AbilitySystem::set_granted_abilities(const TypedArray<Ability> &p_abilities) { granted_abilities = p_abilities; }

void AbilitySystem::set_manual_update(bool p_manual)
{
    manual_update = p_manual;
    if (is_inside_tree() && !Engine::get_singleton()->is_editor_hint())
    {
        set_process(!manual_update);
    }
}

void AbilitySystem::initialize()
{
    attributes.clear();
    attribute_order.clear();
    tag_counts.clear();
    effects.clear();
    abilities.clear();
    ability_order.clear();
    initialized = true;
    for (int i = 0; i < attribute_definitions.size(); i++)
    {
        add_attribute(attribute_definitions[i]);
    }
    for (const String &tag : initial_tags)
    {
        add_tag(tag);
    }
    for (int i = 0; i < granted_abilities.size(); i++)
    {
        grant_ability(granted_abilities[i]);
    }
    for (int i = 0; i < initial_effects.size(); i++)
    {
        apply_effect(initial_effects[i], this);
    }
}

// 📊 Attributs

void AbilitySystem::add_attribute(const Ref<AttributeDefinition> &p_definition)
{
    ERR_FAIL_COND_MSG(p_definition.is_null() || String(p_definition->get_attribute_name()).is_empty(), "AbilitySystem : définition d'attribut vide ou sans nom.");
    StringName name = p_definition->get_attribute_name();
    if (!attributes.has(name))
    {
        attribute_order.push_back(name);
    }
    AttributeState state;
    state.definition = p_definition;
    attributes[name] = state;
    attributes[name].base = _clamp(attributes[name], p_definition->get_base_value());
    attributes[name].current = attributes[name].base;
    _recompute(name);
}

double AbilitySystem::get_value(const StringName &p_name) const
{
    HashMap<StringName, AttributeState>::ConstIterator found = attributes.find(p_name);
    return found != attributes.end() ? found->value.current : 0.0;
}

double AbilitySystem::get_base_value(const StringName &p_name) const
{
    HashMap<StringName, AttributeState>::ConstIterator found = attributes.find(p_name);
    return found != attributes.end() ? found->value.base : 0.0;
}

void AbilitySystem::set_base_value(const StringName &p_name, double p_value)
{
    ERR_FAIL_COND_MSG(!attributes.has(p_name), String(U"AbilitySystem : attribut inconnu « ") + String(p_name) + U" ».");
    attributes[p_name].base = _clamp(attributes[p_name], p_value);
    _recompute(p_name);
}

PackedStringArray AbilitySystem::get_attribute_names() const
{
    PackedStringArray names;
    for (const StringName &name : attribute_order)
    {
        names.push_back(name);
    }
    return names;
}

double AbilitySystem::_clamp(const AttributeState &p_state, double p_value) const
{
    if (p_state.definition.is_null())
    {
        return p_value;
    }
    double value = CLAMP(p_value, p_state.definition->get_min_value(), p_state.definition->get_max_value());
    StringName max_attribute = p_state.definition->get_max_attribute();
    if (!String(max_attribute).is_empty() && attributes.has(max_attribute))
    {
        value = MIN(value, get_value(max_attribute));
    }
    return value;
}

void AbilitySystem::_recompute(const StringName &p_name)
{
    // Garde-fou : bornes circulaires (a ≤ b et b ≤ a) entre attributs.
    if (!attributes.has(p_name) || recompute_depth > 8)
    {
        return;
    }
    recompute_depth++;
    double add = 0.0;
    double multiply = 1.0;
    bool overridden = false;
    double override_value = 0.0;
    for (uint32_t i = 0; i < effects.size(); i++)
    {
        const ActiveEffect &active = effects[i];
        if (active.effect->is_periodic())
        {
            continue; // un effet périodique modifie la base à chaque période, pas la valeur courante
        }
        TypedArray<AttributeModifier> modifiers = active.effect->get_modifiers();
        for (int m = 0; m < modifiers.size(); m++)
        {
            Ref<AttributeModifier> modifier = modifiers[m];
            if (modifier.is_null() || modifier->get_attribute() != p_name)
            {
                continue;
            }
            double magnitude = modifier->compute_magnitude(Object::cast_to<AbilitySystem>(ObjectDB::get_instance(active.source)), this, active.level, active.stacks);
            switch (modifier->get_operation())
            {
            case AttributeModifier::OPERATION_ADD:
                add += magnitude * active.stacks;
                break;
            case AttributeModifier::OPERATION_MULTIPLY:
                multiply *= Math::pow(magnitude, double(active.stacks));
                break;
            case AttributeModifier::OPERATION_OVERRIDE:
                overridden = true;
                override_value = magnitude;
                break;
            }
        }
    }
    AttributeState &state = attributes[p_name];
    double old_value = state.current;
    double new_value = _clamp(state, overridden ? override_value : (state.base + add) * multiply);
    state.current = new_value;
    double min_value = state.definition.is_valid() ? state.definition->get_min_value() : 0.0;

    // Attributs bornés par celui-ci (vie ≤ vie_max) : base et valeur recalculées.
    for (const StringName &other : attribute_order)
    {
        AttributeState &dependent = attributes[other];
        if (other != p_name && dependent.definition.is_valid() && dependent.definition->get_max_attribute() == p_name)
        {
            dependent.base = _clamp(dependent, dependent.base);
            _recompute(other);
        }
    }
    recompute_depth--;

    if (!Math::is_equal_approx(old_value, new_value))
    {
        emit_signal("attribute_changed", p_name, old_value, new_value);
        if (new_value <= min_value && old_value > min_value)
        {
            emit_signal("attribute_depleted", p_name);
        }
    }
}

void AbilitySystem::_recompute_all()
{
    LocalVector<StringName> names = attribute_order; // copie : un signal peut ajouter un attribut
    for (const StringName &name : names)
    {
        _recompute(name);
    }
}

// 🏷️ Tags

bool AbilitySystem::_tag_matches(const String &p_owned, const String &p_query) { return p_owned == p_query || p_owned.begins_with(p_query + String(".")); }

void AbilitySystem::add_tag(const String &p_tag, int p_count)
{
    if (p_tag.is_empty() || p_count <= 0)
    {
        return;
    }
    int before = tag_counts.has(p_tag) ? tag_counts[p_tag] : 0;
    tag_counts[p_tag] = before + p_count;
    if (before == 0)
    {
        emit_signal("tag_changed", p_tag, true);
    }
}

void AbilitySystem::remove_tag(const String &p_tag, int p_count)
{
    if (!tag_counts.has(p_tag))
    {
        return;
    }
    int remaining = tag_counts[p_tag] - p_count;
    if (remaining > 0)
    {
        tag_counts[p_tag] = remaining;
        return;
    }
    tag_counts.erase(p_tag);
    emit_signal("tag_changed", p_tag, false);
}

bool AbilitySystem::has_tag(const String &p_tag) const
{
    for (const KeyValue<String, int> &entry : tag_counts)
    {
        if (entry.value > 0 && _tag_matches(entry.key, p_tag))
        {
            return true;
        }
    }
    return false;
}

bool AbilitySystem::has_any_tags(const PackedStringArray &p_tags) const
{
    for (const String &tag : p_tags)
    {
        if (has_tag(tag))
        {
            return true;
        }
    }
    return false;
}

bool AbilitySystem::has_all_tags(const PackedStringArray &p_tags) const
{
    for (const String &tag : p_tags)
    {
        if (!has_tag(tag))
        {
            return false;
        }
    }
    return true;
}

PackedStringArray AbilitySystem::get_tags() const
{
    PackedStringArray tags;
    for (const KeyValue<String, int> &entry : tag_counts)
    {
        tags.push_back(entry.key);
    }
    tags.sort();
    return tags;
}

void AbilitySystem::_grant_tags(const PackedStringArray &p_tags, int p_sign)
{
    for (const String &tag : p_tags)
    {
        if (p_sign > 0)
        {
            add_tag(tag);
        }
        else
        {
            remove_tag(tag);
        }
    }
}

// ✨ Effets

void AbilitySystem::_apply_to_base(const Ref<GameplayEffect> &p_effect, AbilitySystem *p_source, double p_level, int p_stacks)
{
    TypedArray<AttributeModifier> modifiers = p_effect->get_modifiers();
    for (int m = 0; m < modifiers.size(); m++)
    {
        Ref<AttributeModifier> modifier = modifiers[m];
        if (modifier.is_null() || !attributes.has(modifier->get_attribute()))
        {
            continue;
        }
        StringName name = modifier->get_attribute();
        double magnitude = modifier->compute_magnitude(p_source, this, p_level, p_stacks);
        double base = get_base_value(name);
        switch (modifier->get_operation())
        {
        case AttributeModifier::OPERATION_ADD:
            base += magnitude * p_stacks;
            break;
        case AttributeModifier::OPERATION_MULTIPLY:
            base *= Math::pow(magnitude, double(p_stacks));
            break;
        case AttributeModifier::OPERATION_OVERRIDE:
            base = magnitude;
            break;
        }
        set_base_value(name, base);
    }
}

int AbilitySystem::_find_effect(const Ref<GameplayEffect> &p_effect) const
{
    for (uint32_t i = 0; i < effects.size(); i++)
    {
        if (effects[i].effect == p_effect)
        {
            return int(i);
        }
    }
    return -1;
}

int AbilitySystem::apply_effect(const Ref<GameplayEffect> &p_effect, Object *p_source, double p_level)
{
    if (p_effect.is_null())
    {
        return -1;
    }
    if (!has_all_tags(p_effect->get_required_tags()) || has_any_tags(p_effect->get_blocked_by_tags()))
    {
        emit_signal("effect_blocked", p_effect);
        return -1;
    }
    for (const String &tag : p_effect->get_remove_effects_with_tags())
    {
        remove_effects_with_tag(tag);
    }
    AbilitySystem *source = Object::cast_to<AbilitySystem>(p_source);

    if (p_effect->get_duration_policy() == GameplayEffect::DURATION_INSTANT)
    {
        _apply_to_base(p_effect, source, p_level, 1);
        p_effect->notify_applied(this, p_source);
        emit_signal("effect_applied", p_effect, 0);
        return 0;
    }

    if (p_effect->get_stacking() != GameplayEffect::STACKING_SEPARATE)
    {
        int index = _find_effect(p_effect);
        if (index >= 0)
        {
            ActiveEffect &active = effects[index];
            if (p_effect->get_stacking() == GameplayEffect::STACKING_STACK)
            {
                active.stacks = MIN(active.stacks + 1, p_effect->get_max_stacks());
            }
            active.remaining = p_effect->get_duration();
            int handle = active.handle;
            _recompute_all();
            emit_signal("effect_applied", p_effect, handle);
            return handle;
        }
    }

    ActiveEffect active;
    active.handle = next_handle++;
    active.effect = p_effect;
    active.source = p_source ? ObjectID(p_source->get_instance_id()) : ObjectID();
    active.level = p_level;
    active.remaining = p_effect->get_duration();
    effects.push_back(active);
    _grant_tags(p_effect->get_granted_tags(), 1);
    if (p_effect->is_periodic() && p_effect->get_execute_period_on_apply())
    {
        _apply_to_base(p_effect, source, p_level, 1);
        p_effect->notify_period(this);
    }
    _recompute_all();
    p_effect->notify_applied(this, p_source);
    emit_signal("effect_applied", p_effect, active.handle);
    return active.handle;
}

int AbilitySystem::apply_effect_to(const Ref<GameplayEffect> &p_effect, Node *p_target, double p_level)
{
    AbilitySystem *target = find_on(p_target);
    return target ? target->apply_effect(p_effect, this, p_level) : -1;
}

void AbilitySystem::_remove_effect_at(int p_index)
{
    ActiveEffect removed = effects[p_index];
    effects.remove_at(p_index);
    _grant_tags(removed.effect->get_granted_tags(), -1);
    _recompute_all();
    removed.effect->notify_removed(this);
    emit_signal("effect_removed", removed.effect, removed.handle);
}

bool AbilitySystem::remove_effect(int p_handle)
{
    for (uint32_t i = 0; i < effects.size(); i++)
    {
        if (effects[i].handle == p_handle)
        {
            _remove_effect_at(int(i));
            return true;
        }
    }
    return false;
}

int AbilitySystem::remove_effects_with_tag(const String &p_tag)
{
    int count = 0;
    for (int64_t i = int64_t(effects.size()) - 1; i >= 0; i--)
    {
        if (i >= int64_t(effects.size()))
        {
            continue; // un signal a retiré d'autres effets
        }
        Ref<GameplayEffect> effect = effects[i].effect;
        bool matches = false;
        for (const String &tag : effect->get_effect_tags())
        {
            matches = matches || _tag_matches(tag, p_tag);
        }
        for (const String &tag : effect->get_granted_tags())
        {
            matches = matches || _tag_matches(tag, p_tag);
        }
        if (matches)
        {
            _remove_effect_at(int(i));
            count++;
        }
    }
    return count;
}

bool AbilitySystem::has_effect(const Ref<GameplayEffect> &p_effect) const { return _find_effect(p_effect) >= 0; }

int AbilitySystem::get_effect_stacks(const Ref<GameplayEffect> &p_effect) const
{
    int index = _find_effect(p_effect);
    return index >= 0 ? effects[index].stacks : 0;
}

double AbilitySystem::get_effect_remaining(int p_handle) const
{
    for (uint32_t i = 0; i < effects.size(); i++)
    {
        if (effects[i].handle == p_handle)
        {
            return effects[i].effect->get_duration_policy() == GameplayEffect::DURATION_TIMED ? effects[i].remaining : -1.0;
        }
    }
    return 0.0;
}

Array AbilitySystem::get_active_effects() const
{
    Array result;
    for (uint32_t i = 0; i < effects.size(); i++)
    {
        Dictionary entry;
        entry["handle"] = effects[i].handle;
        entry["effect"] = effects[i].effect;
        entry["remaining"] = effects[i].effect->get_duration_policy() == GameplayEffect::DURATION_TIMED ? effects[i].remaining : -1.0;
        entry["stacks"] = effects[i].stacks;
        entry["source"] = ObjectDB::get_instance(effects[i].source);
        result.push_back(entry);
    }
    return result;
}

// 🌟 Capacités

void AbilitySystem::grant_ability(const Ref<Ability> &p_ability)
{
    ERR_FAIL_COND(p_ability.is_null());
    StringName id = p_ability->get_effective_id();
    ERR_FAIL_COND_MSG(String(id).is_empty(), "AbilitySystem : capacité sans identifiant (ability_id).");
    if (!abilities.has(id))
    {
        ability_order.push_back(id);
    }
    AbilityState state;
    state.ability = p_ability;
    state.charges = p_ability->get_max_charges();
    abilities[id] = state;
    emit_signal("ability_granted", id);
}

void AbilitySystem::revoke_ability(const StringName &p_id)
{
    if (!abilities.has(p_id))
    {
        return;
    }
    cancel_ability(p_id);
    abilities.erase(p_id);
    int64_t index = ability_order.find(p_id);
    if (index >= 0)
    {
        ability_order.remove_at(index);
    }
}

PackedStringArray AbilitySystem::get_ability_ids() const
{
    PackedStringArray ids;
    for (const StringName &id : ability_order)
    {
        ids.push_back(id);
    }
    return ids;
}

Ref<Ability> AbilitySystem::get_ability(const StringName &p_id) const
{
    HashMap<StringName, AbilityState>::ConstIterator found = abilities.find(p_id);
    return found != abilities.end() ? found->value.ability : Ref<Ability>();
}

String AbilitySystem::get_activation_failure(const StringName &p_id, Node *p_target)
{
    if (!abilities.has(p_id))
    {
        return "unknown";
    }
    const AbilityState &state = abilities[p_id];
    Ref<Ability> ability = state.ability;
    if (state.active)
    {
        return "active";
    }
    if (state.charges <= 0)
    {
        return "cooldown";
    }
    if (has_any_tags(ability->get_activation_blocked_tags()))
    {
        return "blocked";
    }
    if (!has_all_tags(ability->get_activation_required_tags()))
    {
        return "missing_tags";
    }
    Dictionary costs = ability->get_costs();
    Array keys = costs.keys();
    for (int i = 0; i < keys.size(); i++)
    {
        StringName attribute = keys[i];
        if (!has_attribute(attribute) || get_value(attribute) < double(costs[keys[i]]))
        {
            return "cost";
        }
    }
    if (ability->get_requires_target() && !p_target)
    {
        return "no_target";
    }
    if (!ability->script_can_activate(this, p_target))
    {
        return "script";
    }
    return String();
}

bool AbilitySystem::activate(const StringName &p_id, Node *p_target)
{
    String failure = get_activation_failure(p_id, p_target);
    if (!failure.is_empty())
    {
        emit_signal("ability_failed", p_id, failure);
        return false;
    }
    Ref<Ability> ability = abilities[p_id].ability;

    // Coûts, charge et recharge
    Dictionary costs = ability->get_costs();
    Array keys = costs.keys();
    for (int i = 0; i < keys.size(); i++)
    {
        add_to_base(keys[i], -double(costs[keys[i]]));
    }
    {
        AbilityState &state = abilities[p_id];
        if (ability->get_cooldown() > 0.0)
        {
            state.charges--;
            if (state.cooldown <= 0.0)
            {
                state.cooldown = ability->get_cooldown();
            }
        }
    }
    for (const String &tag : ability->get_cancel_abilities_with_tags())
    {
        cancel_abilities_with_tag(tag);
    }
    _grant_tags(ability->get_activation_owned_tags(), 1);
    {
        AbilityState &state = abilities[p_id];
        state.active = true;
        state.committed = false;
        state.elapsed = 0.0;
        state.target = p_target ? ObjectID(p_target->get_instance_id()) : ObjectID();
    }
    emit_signal("ability_activated", p_id);
    ability->notify_activate(this, p_target);

    if (abilities.has(p_id) && abilities[p_id].active && ability->get_cast_time() <= 0.0)
    {
        _commit(p_id);
    }
    if (abilities.has(p_id) && abilities[p_id].active && abilities[p_id].committed && ability->get_active_duration() == 0.0)
    {
        _finish(p_id, false);
    }
    return true;
}

void AbilitySystem::_commit(const StringName &p_id)
{
    AbilityState &state = abilities[p_id];
    state.committed = true;
    Ref<Ability> ability = state.ability;
    Node *target = Object::cast_to<Node>(ObjectDB::get_instance(state.target));
    double level = ability->get_level();
    TypedArray<GameplayEffect> on_self = ability->get_effects_on_self();
    for (int i = 0; i < on_self.size(); i++)
    {
        apply_effect(on_self[i], this, level);
    }
    if (AbilitySystem *target_system = find_on(target))
    {
        TypedArray<GameplayEffect> on_target = ability->get_effects_on_target();
        for (int i = 0; i < on_target.size(); i++)
        {
            target_system->apply_effect(on_target[i], this, level);
        }
    }
    ability->notify_commit(this, target);
    emit_signal("ability_committed", p_id);
}

void AbilitySystem::_finish(const StringName &p_id, bool p_cancelled)
{
    if (!abilities.has(p_id) || !abilities[p_id].active)
    {
        return;
    }
    abilities[p_id].active = false;
    Ref<Ability> ability = abilities[p_id].ability;
    _grant_tags(ability->get_activation_owned_tags(), -1);
    ability->notify_end(this, p_cancelled);
    emit_signal("ability_ended", p_id, p_cancelled);
}

void AbilitySystem::end_ability(const StringName &p_id)
{
    if (!abilities.has(p_id) || !abilities[p_id].active)
    {
        return;
    }
    if (!abilities[p_id].committed)
    {
        _commit(p_id); // fin demandée pendant l'incantation : les effets sont tout de même appliqués
    }
    _finish(p_id, false);
}

void AbilitySystem::cancel_ability(const StringName &p_id) { _finish(p_id, true); }

int AbilitySystem::cancel_abilities_with_tag(const String &p_tag)
{
    int count = 0;
    LocalVector<StringName> ids = ability_order;
    for (const StringName &id : ids)
    {
        if (!abilities.has(id) || !abilities[id].active)
        {
            continue;
        }
        for (const String &tag : abilities[id].ability->get_ability_tags())
        {
            if (_tag_matches(tag, p_tag))
            {
                cancel_ability(id);
                count++;
                break;
            }
        }
    }
    return count;
}

bool AbilitySystem::is_ability_active(const StringName &p_id) const
{
    HashMap<StringName, AbilityState>::ConstIterator found = abilities.find(p_id);
    return found != abilities.end() && found->value.active;
}

double AbilitySystem::get_cooldown_remaining(const StringName &p_id) const
{
    HashMap<StringName, AbilityState>::ConstIterator found = abilities.find(p_id);
    return found != abilities.end() && found->value.charges <= 0 ? found->value.cooldown : 0.0;
}

int AbilitySystem::get_charges(const StringName &p_id) const
{
    HashMap<StringName, AbilityState>::ConstIterator found = abilities.find(p_id);
    return found != abilities.end() ? found->value.charges : 0;
}

void AbilitySystem::reset_cooldown(const StringName &p_id)
{
    if (abilities.has(p_id))
    {
        abilities[p_id].charges = abilities[p_id].ability->get_max_charges();
        abilities[p_id].cooldown = 0.0;
    }
}

AbilitySystem *AbilitySystem::find_on(Node *p_node)
{
    if (!p_node)
    {
        return nullptr;
    }
    if (AbilitySystem *system = Object::cast_to<AbilitySystem>(p_node))
    {
        return system;
    }
    for (int i = 0; i < p_node->get_child_count(); i++)
    {
        if (AbilitySystem *system = Object::cast_to<AbilitySystem>(p_node->get_child(i)))
        {
            return system;
        }
    }
    if (p_node->has_method("get_ability_system"))
    {
        return Object::cast_to<AbilitySystem>(p_node->call("get_ability_system"));
    }
    return nullptr;
}

// ⏱️ Mise à jour

void AbilitySystem::update(double p_delta)
{
    // Effets : périodes et durées. On parcourt par identifiant : un signal peut retirer des effets.
    LocalVector<int> handles;
    for (uint32_t i = 0; i < effects.size(); i++)
    {
        handles.push_back(effects[i].handle);
    }
    for (int handle : handles)
    {
        int index = -1;
        for (uint32_t i = 0; i < effects.size(); i++)
        {
            if (effects[i].handle == handle)
            {
                index = int(i);
            }
        }
        if (index < 0)
        {
            continue;
        }
        Ref<GameplayEffect> effect = effects[index].effect;
        if (effect->is_periodic())
        {
            effects[index].period_timer += p_delta;
            int ticks = 0;
            while (effects[index].period_timer >= effect->get_period() && ticks < 100)
            {
                effects[index].period_timer -= effect->get_period();
                ticks++;
            }
            AbilitySystem *source = Object::cast_to<AbilitySystem>(ObjectDB::get_instance(effects[index].source));
            double level = effects[index].level;
            int stacks = effects[index].stacks;
            for (int t = 0; t < ticks; t++)
            {
                _apply_to_base(effect, source, level, stacks);
                effect->notify_period(this);
            }
        }
        if (effect->get_duration_policy() == GameplayEffect::DURATION_TIMED)
        {
            for (uint32_t i = 0; i < effects.size(); i++)
            {
                if (effects[i].handle == handle)
                {
                    effects[i].remaining -= p_delta;
                    if (effects[i].remaining <= 1e-9)
                    {
                        _remove_effect_at(int(i));
                    }
                    break;
                }
            }
        }
    }

    // Recharges et capacités actives
    LocalVector<StringName> ids = ability_order;
    for (const StringName &id : ids)
    {
        if (!abilities.has(id))
        {
            continue;
        }
        AbilityState &state = abilities[id];
        Ref<Ability> ability = state.ability;
        if (state.charges < ability->get_max_charges() && state.cooldown > 0.0)
        {
            state.cooldown -= p_delta;
            if (state.cooldown <= 1e-9)
            {
                state.charges++;
                state.cooldown = state.charges < ability->get_max_charges() ? ability->get_cooldown() : 0.0;
                emit_signal("cooldown_ready", id);
            }
        }
        if (!abilities.has(id) || !abilities[id].active)
        {
            continue;
        }
        abilities[id].elapsed += p_delta;
        if (!abilities[id].committed && abilities[id].elapsed >= ability->get_cast_time())
        {
            _commit(id);
        }
        if (!abilities.has(id) || !abilities[id].active)
        {
            continue;
        }
        ability->notify_tick(this, p_delta);
        if (abilities.has(id) && abilities[id].active && abilities[id].committed && ability->get_active_duration() >= 0.0 && abilities[id].elapsed >= ability->get_cast_time() + ability->get_active_duration())
        {
            _finish(id, false);
        }
    }
}

void AbilitySystem::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY && !Engine::get_singleton()->is_editor_hint())
    {
        if (!initialized)
        {
            initialize();
        }
        set_process(!manual_update);
    }
    else if (p_what == NOTIFICATION_PROCESS)
    {
        update(get_process_delta_time());
    }
}

// 🔗 Liaisons

void AbilitySystem::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_attribute_definitions", "definitions"), &AbilitySystem::set_attribute_definitions);
    ClassDB::bind_method(D_METHOD("get_attribute_definitions"), &AbilitySystem::get_attribute_definitions);
    ClassDB::bind_method(D_METHOD("set_granted_abilities", "abilities"), &AbilitySystem::set_granted_abilities);
    ClassDB::bind_method(D_METHOD("get_granted_abilities"), &AbilitySystem::get_granted_abilities);
    ClassDB::bind_method(D_METHOD("set_initial_tags", "tags"), &AbilitySystem::set_initial_tags);
    ClassDB::bind_method(D_METHOD("get_initial_tags"), &AbilitySystem::get_initial_tags);
    ClassDB::bind_method(D_METHOD("set_initial_effects", "effects"), &AbilitySystem::set_initial_effects);
    ClassDB::bind_method(D_METHOD("get_initial_effects"), &AbilitySystem::get_initial_effects);
    ClassDB::bind_method(D_METHOD("set_manual_update", "manual"), &AbilitySystem::set_manual_update);
    ClassDB::bind_method(D_METHOD("is_manual_update"), &AbilitySystem::is_manual_update);
    ClassDB::bind_method(D_METHOD("initialize"), &AbilitySystem::initialize);
    ClassDB::bind_method(D_METHOD("update", "delta"), &AbilitySystem::update);

    ClassDB::bind_method(D_METHOD("has_attribute", "name"), &AbilitySystem::has_attribute);
    ClassDB::bind_method(D_METHOD("get_value", "name"), &AbilitySystem::get_value);
    ClassDB::bind_method(D_METHOD("get_base_value", "name"), &AbilitySystem::get_base_value);
    ClassDB::bind_method(D_METHOD("set_base_value", "name", "value"), &AbilitySystem::set_base_value);
    ClassDB::bind_method(D_METHOD("add_to_base", "name", "amount"), &AbilitySystem::add_to_base);
    ClassDB::bind_method(D_METHOD("get_attribute_names"), &AbilitySystem::get_attribute_names);
    ClassDB::bind_method(D_METHOD("add_attribute", "definition"), &AbilitySystem::add_attribute);

    ClassDB::bind_method(D_METHOD("add_tag", "tag", "count"), &AbilitySystem::add_tag, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("remove_tag", "tag", "count"), &AbilitySystem::remove_tag, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("has_tag", "tag"), &AbilitySystem::has_tag);
    ClassDB::bind_method(D_METHOD("has_any_tags", "tags"), &AbilitySystem::has_any_tags);
    ClassDB::bind_method(D_METHOD("has_all_tags", "tags"), &AbilitySystem::has_all_tags);
    ClassDB::bind_method(D_METHOD("get_tags"), &AbilitySystem::get_tags);

    ClassDB::bind_method(D_METHOD("apply_effect", "effect", "source", "level"), &AbilitySystem::apply_effect, DEFVAL(Variant()), DEFVAL(1.0));
    ClassDB::bind_method(D_METHOD("apply_effect_to", "effect", "target", "level"), &AbilitySystem::apply_effect_to, DEFVAL(1.0));
    ClassDB::bind_method(D_METHOD("remove_effect", "handle"), &AbilitySystem::remove_effect);
    ClassDB::bind_method(D_METHOD("remove_effects_with_tag", "tag"), &AbilitySystem::remove_effects_with_tag);
    ClassDB::bind_method(D_METHOD("has_effect", "effect"), &AbilitySystem::has_effect);
    ClassDB::bind_method(D_METHOD("get_effect_stacks", "effect"), &AbilitySystem::get_effect_stacks);
    ClassDB::bind_method(D_METHOD("get_effect_remaining", "handle"), &AbilitySystem::get_effect_remaining);
    ClassDB::bind_method(D_METHOD("get_active_effects"), &AbilitySystem::get_active_effects);

    ClassDB::bind_method(D_METHOD("grant_ability", "ability"), &AbilitySystem::grant_ability);
    ClassDB::bind_method(D_METHOD("revoke_ability", "id"), &AbilitySystem::revoke_ability);
    ClassDB::bind_method(D_METHOD("has_ability", "id"), &AbilitySystem::has_ability);
    ClassDB::bind_method(D_METHOD("get_ability_ids"), &AbilitySystem::get_ability_ids);
    ClassDB::bind_method(D_METHOD("get_ability", "id"), &AbilitySystem::get_ability);
    ClassDB::bind_method(D_METHOD("get_activation_failure", "id", "target"), &AbilitySystem::get_activation_failure, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("can_activate", "id", "target"), &AbilitySystem::can_activate, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("activate", "id", "target"), &AbilitySystem::activate, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("end_ability", "id"), &AbilitySystem::end_ability);
    ClassDB::bind_method(D_METHOD("cancel_ability", "id"), &AbilitySystem::cancel_ability);
    ClassDB::bind_method(D_METHOD("cancel_abilities_with_tag", "tag"), &AbilitySystem::cancel_abilities_with_tag);
    ClassDB::bind_method(D_METHOD("is_ability_active", "id"), &AbilitySystem::is_ability_active);
    ClassDB::bind_method(D_METHOD("get_cooldown_remaining", "id"), &AbilitySystem::get_cooldown_remaining);
    ClassDB::bind_method(D_METHOD("get_charges", "id"), &AbilitySystem::get_charges);
    ClassDB::bind_method(D_METHOD("reset_cooldown", "id"), &AbilitySystem::reset_cooldown);
    ClassDB::bind_static_method("AbilitySystem", D_METHOD("find_on", "node"), &AbilitySystem::find_on);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "attribute_definitions", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":AttributeDefinition"), "set_attribute_definitions", "get_attribute_definitions");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "granted_abilities", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":Ability"), "set_granted_abilities", "get_granted_abilities");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "initial_tags"), "set_initial_tags", "get_initial_tags");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "initial_effects", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":GameplayEffect"), "set_initial_effects", "get_initial_effects");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "manual_update"), "set_manual_update", "is_manual_update");

    ADD_SIGNAL(MethodInfo("attribute_changed", PropertyInfo(Variant::STRING_NAME, "attribute"), PropertyInfo(Variant::FLOAT, "old_value"), PropertyInfo(Variant::FLOAT, "new_value")));
    ADD_SIGNAL(MethodInfo("attribute_depleted", PropertyInfo(Variant::STRING_NAME, "attribute")));
    ADD_SIGNAL(MethodInfo("tag_changed", PropertyInfo(Variant::STRING, "tag"), PropertyInfo(Variant::BOOL, "present")));
    ADD_SIGNAL(MethodInfo("effect_applied", PropertyInfo(Variant::OBJECT, "effect", PROPERTY_HINT_RESOURCE_TYPE, "GameplayEffect"), PropertyInfo(Variant::INT, "handle")));
    ADD_SIGNAL(MethodInfo("effect_removed", PropertyInfo(Variant::OBJECT, "effect", PROPERTY_HINT_RESOURCE_TYPE, "GameplayEffect"), PropertyInfo(Variant::INT, "handle")));
    ADD_SIGNAL(MethodInfo("effect_blocked", PropertyInfo(Variant::OBJECT, "effect", PROPERTY_HINT_RESOURCE_TYPE, "GameplayEffect")));
    ADD_SIGNAL(MethodInfo("ability_granted", PropertyInfo(Variant::STRING_NAME, "id")));
    ADD_SIGNAL(MethodInfo("ability_activated", PropertyInfo(Variant::STRING_NAME, "id")));
    ADD_SIGNAL(MethodInfo("ability_committed", PropertyInfo(Variant::STRING_NAME, "id")));
    ADD_SIGNAL(MethodInfo("ability_ended", PropertyInfo(Variant::STRING_NAME, "id"), PropertyInfo(Variant::BOOL, "cancelled")));
    ADD_SIGNAL(MethodInfo("ability_failed", PropertyInfo(Variant::STRING_NAME, "id"), PropertyInfo(Variant::STRING, "reason")));
    ADD_SIGNAL(MethodInfo("cooldown_ready", PropertyInfo(Variant::STRING_NAME, "id")));
}
