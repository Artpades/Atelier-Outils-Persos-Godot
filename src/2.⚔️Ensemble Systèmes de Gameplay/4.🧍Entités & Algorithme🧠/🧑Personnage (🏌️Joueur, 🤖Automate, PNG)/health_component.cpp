#include "health_component.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

AbilitySystem *HealthComponent::get_ability_system() const
{
    if (!ability_system_path.is_empty())
    {
        return Object::cast_to<AbilitySystem>(get_node_or_null(ability_system_path));
    }
    Node *parent = get_parent();
    for (int i = 0; parent && i < parent->get_child_count(); i++)
    {
        if (AbilitySystem *system = Object::cast_to<AbilitySystem>(parent->get_child(i)))
        {
            return system;
        }
    }
    return nullptr;
}

bool HealthComponent::uses_attribute() const
{
    AbilitySystem *system = get_ability_system();
    return system && system->has_attribute(health_attribute);
}

void HealthComponent::set_max_health(double p_value)
{
    max_health = MAX(1.0, p_value);
    health = MIN(health, max_health);
}

double HealthComponent::get_max_health() const
{
    AbilitySystem *system = get_ability_system();
    if (system && system->has_attribute(max_health_attribute))
    {
        return system->get_value(max_health_attribute);
    }
    return max_health;
}

double HealthComponent::get_health() const { return uses_attribute() ? get_ability_system()->get_value(health_attribute) : health; }

double HealthComponent::get_health_ratio() const { return get_health() / MAX(1e-9, get_max_health()); }

void HealthComponent::set_health(double p_value)
{
    if (uses_attribute())
    {
        // L'AbilitySystem émet attribute_changed : relayé par _on_attribute_changed.
        get_ability_system()->set_base_value(health_attribute, p_value);
        return;
    }
    double previous = health;
    health = CLAMP(p_value, 0.0, max_health);
    if (previous != health)
    {
        emit_signal("health_changed", health, max_health);
    }
    if (health <= 0.0 && alive && is_inside_tree())
    {
        kill();
    }
}

double HealthComponent::take_damage(double p_amount, Object *p_source)
{
    if (!alive || p_amount <= 0.0 || invulnerable_remaining > 0.0)
    {
        return 0.0;
    }
    double before = get_health();
    set_health(before - p_amount);
    double applied = before - get_health();
    invulnerable_remaining = invulnerability_time;
    emit_signal("damaged", applied, p_source);
    if (alive && get_health() <= 0.0)
    {
        kill();
    }
    return applied;
}

double HealthComponent::heal(double p_amount)
{
    if (!alive || p_amount <= 0.0)
    {
        return 0.0;
    }
    double before = get_health();
    set_health(MIN(before + p_amount, get_max_health()));
    double applied = get_health() - before;
    if (applied > 0.0)
    {
        emit_signal("healed", applied);
    }
    return applied;
}

void HealthComponent::kill()
{
    if (!alive)
    {
        return;
    }
    alive = false;
    emit_signal("died");
}

void HealthComponent::revive(double p_health)
{
    if (alive)
    {
        return;
    }
    alive = true;
    set_health(p_health < 0.0 ? get_max_health() : p_health);
    emit_signal("revived");
}

void HealthComponent::_on_attribute_changed(const StringName &p_name, double /*p_old*/, double p_new)
{
    if (p_name == health_attribute)
    {
        emit_signal("health_changed", p_new, get_max_health());
        if (p_new <= 0.0 && alive)
        {
            kill();
        }
    }
    else if (p_name == max_health_attribute)
    {
        emit_signal("health_changed", get_health(), p_new);
    }
}

void HealthComponent::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_READY)
    {
        if (AbilitySystem *system = get_ability_system())
        {
            system->connect("attribute_changed", callable_mp(this, &HealthComponent::_on_attribute_changed));
        }
        set_physics_process(true);
    }
    else if (p_what == NOTIFICATION_PHYSICS_PROCESS)
    {
        invulnerable_remaining = MAX(0.0, invulnerable_remaining - get_physics_process_delta_time());
    }
}

void HealthComponent::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_max_health", "value"), &HealthComponent::set_max_health);
    ClassDB::bind_method(D_METHOD("get_max_health"), &HealthComponent::get_max_health);
    ClassDB::bind_method(D_METHOD("set_health", "value"), &HealthComponent::set_health);
    ClassDB::bind_method(D_METHOD("get_health"), &HealthComponent::get_health);
    ClassDB::bind_method(D_METHOD("set_invulnerability_time", "time"), &HealthComponent::set_invulnerability_time);
    ClassDB::bind_method(D_METHOD("get_invulnerability_time"), &HealthComponent::get_invulnerability_time);
    ClassDB::bind_method(D_METHOD("set_health_attribute", "name"), &HealthComponent::set_health_attribute);
    ClassDB::bind_method(D_METHOD("get_health_attribute"), &HealthComponent::get_health_attribute);
    ClassDB::bind_method(D_METHOD("set_max_health_attribute", "name"), &HealthComponent::set_max_health_attribute);
    ClassDB::bind_method(D_METHOD("get_max_health_attribute"), &HealthComponent::get_max_health_attribute);
    ClassDB::bind_method(D_METHOD("set_ability_system_path", "path"), &HealthComponent::set_ability_system_path);
    ClassDB::bind_method(D_METHOD("get_ability_system_path"), &HealthComponent::get_ability_system_path);
    ClassDB::bind_method(D_METHOD("take_damage", "amount", "source"), &HealthComponent::take_damage, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("heal", "amount"), &HealthComponent::heal);
    ClassDB::bind_method(D_METHOD("kill"), &HealthComponent::kill);
    ClassDB::bind_method(D_METHOD("revive", "health"), &HealthComponent::revive, DEFVAL(-1.0));
    ClassDB::bind_method(D_METHOD("is_alive"), &HealthComponent::is_alive);
    ClassDB::bind_method(D_METHOD("is_invulnerable"), &HealthComponent::is_invulnerable);
    ClassDB::bind_method(D_METHOD("get_health_ratio"), &HealthComponent::get_health_ratio);
    ClassDB::bind_method(D_METHOD("get_ability_system"), &HealthComponent::get_ability_system);
    ClassDB::bind_method(D_METHOD("uses_attribute"), &HealthComponent::uses_attribute);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_health", PROPERTY_HINT_RANGE, "1,10000,1,or_greater"), "set_max_health", "get_max_health");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "health", PROPERTY_HINT_RANGE, "0,10000,1,or_greater"), "set_health", "get_health");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "invulnerability_time", PROPERTY_HINT_RANGE, "0,5,0.01,or_greater,suffix:s"), "set_invulnerability_time", "get_invulnerability_time");
    ADD_GROUP(U"🌟 Attribut (AbilitySystem)", "");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "ability_system_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "AbilitySystem"), "set_ability_system_path", "get_ability_system_path");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "health_attribute"), "set_health_attribute", "get_health_attribute");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "max_health_attribute"), "set_max_health_attribute", "get_max_health_attribute");

    ADD_SIGNAL(MethodInfo("health_changed", PropertyInfo(Variant::FLOAT, "health"), PropertyInfo(Variant::FLOAT, "max_health")));
    ADD_SIGNAL(MethodInfo("damaged", PropertyInfo(Variant::FLOAT, "amount"), PropertyInfo(Variant::OBJECT, "source")));
    ADD_SIGNAL(MethodInfo("healed", PropertyInfo(Variant::FLOAT, "amount")));
    ADD_SIGNAL(MethodInfo("died"));
    ADD_SIGNAL(MethodInfo("revived"));
}
