#include "achievement_condition.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

/// Nombre sans décimales inutiles : 3 → « 3 », 2.5 → « 2.5 ».
static String format_number(double p_value)
{
    return Math::is_equal_approx(p_value, Math::round(p_value)) ? String::num_int64(int64_t(Math::round(p_value))) : String::num(p_value, 2);
}

// 🚩 AchievementCondition

void AchievementCondition::complete()
{
    _set_completed(true);
}

void AchievementCondition::reset()
{
    _set_completed(false);
}

double AchievementCondition::get_progress() const
{
    const double target = get_target();
    if (completed || target <= 0.0)
    {
        return completed ? 1.0 : 0.0;
    }
    return CLAMP(get_value() / target, 0.0, 1.0);
}

String AchievementCondition::get_progress_text() const
{
    // Une valeur au-delà de la cible (compteur dépassé) est affichée plafonnée.
    return format_number(MIN(get_value(), get_target())) + "/" + format_number(get_target());
}

bool AchievementCondition::matches(const StringName &p_event, const String &p_id) const
{
    return !event_name.is_empty() && p_event == event_name && (event_ids.is_empty() || event_ids.has(p_id));
}

bool AchievementCondition::notify(const StringName &p_event, const String &p_id, double p_amount)
{
    if (completed)
    {
        return false;
    }
    // Logique personnalisée d'un script : il décide seul si l'évènement le concerne.
    bool handled = false;
    if (GDVIRTUAL_CALL(_handle_event, p_event, p_id, p_amount, handled))
    {
        return handled;
    }
    if (!matches(p_event, p_id))
    {
        return false;
    }
    _apply_event(p_id, p_amount);
    return true;
}

void AchievementCondition::_apply_event(const String & /*p_id*/, double /*p_amount*/)
{
    complete();
}

void AchievementCondition::_set_completed(bool p_completed)
{
    if (p_completed == completed)
    {
        return;
    }
    completed = p_completed;
    if (completed)
    {
        emit_signal("completed");
    }
    emit_changed();
}

Dictionary AchievementCondition::get_state() const
{
    Dictionary state;
    state["completed"] = completed;
    return state;
}

void AchievementCondition::set_state(const Dictionary &p_state)
{
    completed = p_state.get("completed", false);
}

void AchievementCondition::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_condition_name", "name"), &AchievementCondition::set_condition_name);
    ClassDB::bind_method(D_METHOD("get_condition_name"), &AchievementCondition::get_condition_name);
    ClassDB::bind_method(D_METHOD("set_event_name", "event"), &AchievementCondition::set_event_name);
    ClassDB::bind_method(D_METHOD("get_event_name"), &AchievementCondition::get_event_name);
    ClassDB::bind_method(D_METHOD("set_event_ids", "ids"), &AchievementCondition::set_event_ids);
    ClassDB::bind_method(D_METHOD("get_event_ids"), &AchievementCondition::get_event_ids);
    ClassDB::bind_method(D_METHOD("is_completed"), &AchievementCondition::is_completed);
    ClassDB::bind_method(D_METHOD("complete"), &AchievementCondition::complete);
    ClassDB::bind_method(D_METHOD("reset"), &AchievementCondition::reset);
    ClassDB::bind_method(D_METHOD("get_value"), &AchievementCondition::get_value);
    ClassDB::bind_method(D_METHOD("get_target"), &AchievementCondition::get_target);
    ClassDB::bind_method(D_METHOD("get_progress"), &AchievementCondition::get_progress);
    ClassDB::bind_method(D_METHOD("get_progress_text"), &AchievementCondition::get_progress_text);
    ClassDB::bind_method(D_METHOD("matches", "event", "id"), &AchievementCondition::matches);
    ClassDB::bind_method(D_METHOD("notify", "event", "id", "amount"), &AchievementCondition::notify, DEFVAL(""), DEFVAL(1.0));
    ClassDB::bind_method(D_METHOD("get_state"), &AchievementCondition::get_state);
    ClassDB::bind_method(D_METHOD("set_state", "state"), &AchievementCondition::set_state);

    GDVIRTUAL_BIND(_handle_event, "event", "id", "amount");

    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "condition_name"), "set_condition_name", "get_condition_name");
    ADD_GROUP(U"📣 Évènement", "");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "event_name"), "set_event_name", "get_event_name");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "event_ids"), "set_event_ids", "get_event_ids");

    ADD_SIGNAL(MethodInfo("completed"));
}

// 🔢 AchievementCounterCondition

void AchievementCounterCondition::set_target(double p_target)
{
    target = MAX(p_target, 0.0);
    _refresh();
}

void AchievementCounterCondition::set_value(double p_value)
{
    if (p_value == value)
    {
        return;
    }
    value = p_value;
    if (!completed && value >= target)
    {
        _set_completed(true); // émet `changed`
        return;
    }
    emit_changed();
}

void AchievementCounterCondition::add(double p_amount)
{
    set_value(value + p_amount);
}

void AchievementCounterCondition::complete()
{
    value = MAX(value, target);
    _set_completed(true);
}

void AchievementCounterCondition::reset()
{
    value = 0.0;
    if (completed)
    {
        _set_completed(false);
        return;
    }
    emit_changed();
}

void AchievementCounterCondition::_apply_event(const String & /*p_id*/, double p_amount)
{
    switch (value_mode)
    {
    case VALUE_MODE_MAX:
        set_value(MAX(value, p_amount));
        break;
    case VALUE_MODE_SET:
        set_value(p_amount);
        break;
    case VALUE_MODE_ADD:
    default:
        add(p_amount);
        break;
    }
}

void AchievementCounterCondition::_refresh()
{
    // Une condition remplie le reste : une valeur qui redescend (VALUE_MODE_SET) ne l'annule pas.
    if (!completed && value >= target)
    {
        _set_completed(true);
    }
}

Dictionary AchievementCounterCondition::get_state() const
{
    Dictionary state = AchievementCondition::get_state();
    state["value"] = value;
    return state;
}

void AchievementCounterCondition::set_state(const Dictionary &p_state)
{
    AchievementCondition::set_state(p_state);
    value = p_state.get("value", 0.0);
}

void AchievementCounterCondition::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_target", "target"), &AchievementCounterCondition::set_target);
    ClassDB::bind_method(D_METHOD("set_value", "value"), &AchievementCounterCondition::set_value);
    ClassDB::bind_method(D_METHOD("set_value_mode", "mode"), &AchievementCounterCondition::set_value_mode);
    ClassDB::bind_method(D_METHOD("get_value_mode"), &AchievementCounterCondition::get_value_mode);
    ClassDB::bind_method(D_METHOD("add", "amount"), &AchievementCounterCondition::add, DEFVAL(1.0));

    BIND_ENUM_CONSTANT(VALUE_MODE_ADD);
    BIND_ENUM_CONSTANT(VALUE_MODE_MAX);
    BIND_ENUM_CONSTANT(VALUE_MODE_SET);

    ADD_GROUP(U"🔢 Compteur", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "target", PROPERTY_HINT_RANGE, "0,1000,1,or_greater"), "set_target", "get_target");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "value_mode", PROPERTY_HINT_ENUM, "Add,Max,Set"), "set_value_mode", "get_value_mode");
    // État de jeu : visible dans l'inspecteur (débogage) mais jamais enregistré dans la ressource.
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_value", "get_value");
}

// 🗂️ AchievementCollectionCondition

void AchievementCollectionCondition::set_required_ids(const PackedStringArray &p_ids)
{
    required_ids = p_ids;
    _refresh();
}

PackedStringArray AchievementCollectionCondition::get_missing_ids() const
{
    PackedStringArray missing;
    for (const String &id : required_ids)
    {
        if (!collected_ids.has(id))
        {
            missing.push_back(id);
        }
    }
    return missing;
}

bool AchievementCollectionCondition::collect(const String &p_id)
{
    if (!required_ids.has(p_id) || collected_ids.has(p_id))
    {
        return false;
    }
    collected_ids.push_back(p_id);
    if (!completed && get_missing_ids().is_empty())
    {
        _set_completed(true);
    }
    else
    {
        emit_changed();
    }
    return true;
}

bool AchievementCollectionCondition::matches(const StringName &p_event, const String &p_id) const
{
    // Le filtre est required_ids (event_ids n'est pas utilisé).
    return !get_event_name().is_empty() && p_event == get_event_name() && required_ids.has(p_id);
}

void AchievementCollectionCondition::_apply_event(const String &p_id, double /*p_amount*/)
{
    collect(p_id);
}

void AchievementCollectionCondition::complete()
{
    collected_ids = required_ids.duplicate();
    _set_completed(true);
}

void AchievementCollectionCondition::reset()
{
    collected_ids.clear();
    if (completed)
    {
        _set_completed(false);
        return;
    }
    emit_changed();
}

void AchievementCollectionCondition::_refresh()
{
    if (!completed && !required_ids.is_empty() && get_missing_ids().is_empty())
    {
        _set_completed(true);
    }
}

Dictionary AchievementCollectionCondition::get_state() const
{
    Dictionary state = AchievementCondition::get_state();
    state["collected"] = collected_ids;
    return state;
}

void AchievementCollectionCondition::set_state(const Dictionary &p_state)
{
    AchievementCondition::set_state(p_state);
    collected_ids = p_state.get("collected", PackedStringArray());
}

void AchievementCollectionCondition::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_required_ids", "ids"), &AchievementCollectionCondition::set_required_ids);
    ClassDB::bind_method(D_METHOD("get_required_ids"), &AchievementCollectionCondition::get_required_ids);
    ClassDB::bind_method(D_METHOD("get_collected_ids"), &AchievementCollectionCondition::get_collected_ids);
    ClassDB::bind_method(D_METHOD("get_missing_ids"), &AchievementCollectionCondition::get_missing_ids);
    ClassDB::bind_method(D_METHOD("collect", "id"), &AchievementCollectionCondition::collect);
    ClassDB::bind_method(D_METHOD("has_collected", "id"), &AchievementCollectionCondition::has_collected);

    ADD_GROUP(U"🗂️ Collection", "");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "required_ids"), "set_required_ids", "get_required_ids");
}
