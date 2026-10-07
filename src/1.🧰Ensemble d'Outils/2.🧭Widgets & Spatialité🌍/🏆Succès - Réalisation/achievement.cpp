#include "achievement.hpp"

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

static String format_number(double p_value)
{
    return Math::is_equal_approx(p_value, Math::round(p_value)) ? String::num_int64(int64_t(Math::round(p_value))) : String::num(p_value, 2);
}

// 🧩 Conditions

void Achievement::set_conditions(const TypedArray<AchievementCondition> &p_conditions)
{
    conditions = p_conditions;
    _connect_conditions();
}

void Achievement::add_condition(const Ref<AchievementCondition> &p_condition)
{
    ERR_FAIL_COND(p_condition.is_null());
    conditions.push_back(p_condition);
    _connect_conditions();
}

Ref<AchievementCondition> Achievement::get_condition(const StringName &p_name) const
{
    for (int64_t i = 0; i < conditions.size(); i++)
    {
        Ref<AchievementCondition> condition = conditions[i];
        if (condition.is_valid() && condition->get_condition_name() == p_name)
        {
            return condition;
        }
    }
    return Ref<AchievementCondition>();
}

void Achievement::_connect_conditions()
{
    const Callable callback = callable_mp(this, &Achievement::_on_condition_changed);
    for (int64_t i = 0; i < connected_conditions.size(); i++)
    {
        Ref<AchievementCondition> condition = connected_conditions[i];
        if (condition.is_valid() && condition->is_connected("changed", callback))
        {
            condition->disconnect("changed", callback);
        }
    }
    // Nouveau tableau (pas une copie partagée de conditions, cf. Array partagé en godot-cpp).
    connected_conditions = TypedArray<AchievementCondition>();
    for (int64_t i = 0; i < conditions.size(); i++)
    {
        Ref<AchievementCondition> condition = conditions[i];
        if (condition.is_valid() && !condition->is_connected("changed", callback))
        {
            condition->connect("changed", callback);
            connected_conditions.push_back(condition);
        }
    }
}

void Achievement::_on_condition_changed()
{
    if (updating || unlocked)
    {
        return;
    }
    if (!evaluate())
    {
        emit_signal("progressed", get_progress());
    }
}

bool Achievement::_conditions_satisfied() const
{
    int valid = 0;
    int completed = 0;
    for (int64_t i = 0; i < conditions.size(); i++)
    {
        Ref<AchievementCondition> condition = conditions[i];
        if (condition.is_null())
        {
            continue;
        }
        valid++;
        completed += condition->is_completed() ? 1 : 0;
    }
    if (valid == 0)
    {
        return false; // sans condition : débloqué seulement par unlock()
    }
    return completion_mode == COMPLETION_ANY ? completed > 0 : completed == valid;
}

int Achievement::get_completed_condition_count() const
{
    int count = 0;
    for (int64_t i = 0; i < conditions.size(); i++)
    {
        Ref<AchievementCondition> condition = conditions[i];
        count += condition.is_valid() && condition->is_completed() ? 1 : 0;
    }
    return count;
}

// 🔓 Déblocage

bool Achievement::notify(const StringName &p_event, const String &p_id, double p_amount)
{
    if (unlocked)
    {
        return false;
    }
    bool progressed = false;
    for (int64_t i = 0; i < conditions.size() && !unlocked; i++)
    {
        Ref<AchievementCondition> condition = conditions[i];
        if (condition.is_valid() && condition->notify(p_event, p_id, p_amount))
        {
            progressed = true;
        }
    }
    return progressed;
}

bool Achievement::evaluate()
{
    if (unlocked || !_conditions_satisfied())
    {
        return false;
    }
    unlocked = true;
    unlock_time = Time::get_singleton()->get_unix_time_from_system();
    emit_signal("progressed", 1.0);
    emit_signal("unlocked");
    return true;
}

void Achievement::unlock()
{
    if (unlocked)
    {
        return;
    }
    updating = true;
    for (int64_t i = 0; i < conditions.size(); i++)
    {
        Ref<AchievementCondition> condition = conditions[i];
        if (condition.is_valid())
        {
            condition->complete();
        }
    }
    updating = false;

    unlocked = true;
    unlock_time = Time::get_singleton()->get_unix_time_from_system();
    emit_signal("progressed", 1.0);
    emit_signal("unlocked");
}

void Achievement::reset()
{
    updating = true;
    for (int64_t i = 0; i < conditions.size(); i++)
    {
        Ref<AchievementCondition> condition = conditions[i];
        if (condition.is_valid())
        {
            condition->reset();
        }
    }
    updating = false;

    unlocked = false;
    unlock_time = 0.0;
    emit_signal("progressed", 0.0);
}

// 📊 Progression

Ref<AchievementCondition> Achievement::_tracked() const
{
    if (progress_tracker == TRACKER_FROM_CONDITION)
    {
        return get_condition(tracked_condition);
    }
    if (progress_tracker == TRACKER_AUTO && conditions.size() == 1)
    {
        return conditions[0];
    }
    return Ref<AchievementCondition>();
}

bool Achievement::has_progress() const
{
    switch (progress_tracker)
    {
    case TRACKER_NONE:
        return false;
    case TRACKER_FROM_CONDITION:
    {
        Ref<AchievementCondition> tracked = _tracked();
        return tracked.is_valid() && tracked->get_target() > 1.0;
    }
    case TRACKER_CONDITIONS_COMPLETED:
        return conditions.size() > 1;
    case TRACKER_AUTO:
    default:
        if (Ref<AchievementCondition> tracked = _tracked(); tracked.is_valid())
        {
            return tracked->get_target() > 1.0;
        }
        return conditions.size() > 1;
    }
}

double Achievement::get_progress() const
{
    if (unlocked)
    {
        return 1.0;
    }
    if (progress_tracker == TRACKER_NONE)
    {
        return 0.0;
    }
    if (Ref<AchievementCondition> tracked = _tracked(); tracked.is_valid())
    {
        return tracked->get_progress();
    }
    if (progress_tracker == TRACKER_AUTO && completion_mode == COMPLETION_ANY)
    {
        // Une seule condition suffit : la plus avancée donne la progression.
        double best = 0.0;
        for (int64_t i = 0; i < conditions.size(); i++)
        {
            Ref<AchievementCondition> condition = conditions[i];
            best = condition.is_valid() ? MAX(best, condition->get_progress()) : best;
        }
        return best;
    }
    return conditions.is_empty() ? 0.0 : double(get_completed_condition_count()) / double(conditions.size());
}

String Achievement::get_progress_text() const
{
    if (!has_progress())
    {
        return String();
    }
    if (Ref<AchievementCondition> tracked = _tracked(); tracked.is_valid())
    {
        return unlocked ? format_number(tracked->get_target()) + "/" + format_number(tracked->get_target()) : tracked->get_progress_text();
    }
    if (progress_tracker == TRACKER_AUTO && completion_mode == COMPLETION_ANY)
    {
        return String::num_int64(int64_t(Math::round(get_progress() * 100.0))) + "%";
    }
    const int64_t total = conditions.size();
    return String::num_int64(unlocked ? total : get_completed_condition_count()) + "/" + String::num_int64(total);
}

// 💾 État

String Achievement::_condition_key(int64_t p_index) const
{
    Ref<AchievementCondition> condition = conditions[p_index];
    return condition.is_valid() && !String(condition->get_condition_name()).is_empty() ? String(condition->get_condition_name()) : String::num_int64(p_index);
}

Dictionary Achievement::get_state() const
{
    Dictionary condition_states;
    for (int64_t i = 0; i < conditions.size(); i++)
    {
        Ref<AchievementCondition> condition = conditions[i];
        if (condition.is_valid())
        {
            condition_states[_condition_key(i)] = condition->get_state();
        }
    }
    Dictionary state;
    state["unlocked"] = unlocked;
    state["unlock_time"] = unlock_time;
    state["conditions"] = condition_states;
    return state;
}

void Achievement::set_state(const Dictionary &p_state)
{
    unlocked = p_state.get("unlocked", false);
    unlock_time = p_state.get("unlock_time", 0.0);
    const Dictionary condition_states = p_state.get("conditions", Dictionary());
    for (int64_t i = 0; i < conditions.size(); i++)
    {
        Ref<AchievementCondition> condition = conditions[i];
        const String key = _condition_key(i);
        if (condition.is_null())
        {
            continue;
        }
        // Condition absente de la sauvegarde (ajoutée depuis) : remise à zéro, sans signal.
        condition->set_state(condition_states.has(key) ? Dictionary(condition_states[key]) : Dictionary());
    }
}

// 🔗 Liaisons

void Achievement::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_id", "id"), &Achievement::set_id);
    ClassDB::bind_method(D_METHOD("get_id"), &Achievement::get_id);
    ClassDB::bind_method(D_METHOD("set_display_name", "name"), &Achievement::set_display_name);
    ClassDB::bind_method(D_METHOD("get_display_name"), &Achievement::get_display_name);
    ClassDB::bind_method(D_METHOD("set_description", "description"), &Achievement::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &Achievement::get_description);
    ClassDB::bind_method(D_METHOD("set_icon", "icon"), &Achievement::set_icon);
    ClassDB::bind_method(D_METHOD("get_icon"), &Achievement::get_icon);
    ClassDB::bind_method(D_METHOD("set_locked_icon", "icon"), &Achievement::set_locked_icon);
    ClassDB::bind_method(D_METHOD("get_locked_icon"), &Achievement::get_locked_icon);
    ClassDB::bind_method(D_METHOD("get_current_icon"), &Achievement::get_current_icon);
    ClassDB::bind_method(D_METHOD("set_category", "category"), &Achievement::set_category);
    ClassDB::bind_method(D_METHOD("get_category"), &Achievement::get_category);
    ClassDB::bind_method(D_METHOD("set_hidden", "hidden"), &Achievement::set_hidden);
    ClassDB::bind_method(D_METHOD("is_hidden"), &Achievement::is_hidden);

    ClassDB::bind_method(D_METHOD("set_conditions", "conditions"), &Achievement::set_conditions);
    ClassDB::bind_method(D_METHOD("get_conditions"), &Achievement::get_conditions);
    ClassDB::bind_method(D_METHOD("add_condition", "condition"), &Achievement::add_condition);
    ClassDB::bind_method(D_METHOD("get_condition", "name"), &Achievement::get_condition);
    ClassDB::bind_method(D_METHOD("set_completion_mode", "mode"), &Achievement::set_completion_mode);
    ClassDB::bind_method(D_METHOD("get_completion_mode"), &Achievement::get_completion_mode);
    ClassDB::bind_method(D_METHOD("set_progress_tracker", "tracker"), &Achievement::set_progress_tracker);
    ClassDB::bind_method(D_METHOD("get_progress_tracker"), &Achievement::get_progress_tracker);
    ClassDB::bind_method(D_METHOD("set_tracked_condition", "name"), &Achievement::set_tracked_condition);
    ClassDB::bind_method(D_METHOD("get_tracked_condition"), &Achievement::get_tracked_condition);

    ClassDB::bind_method(D_METHOD("is_unlocked"), &Achievement::is_unlocked);
    ClassDB::bind_method(D_METHOD("get_unlock_time"), &Achievement::get_unlock_time);
    ClassDB::bind_method(D_METHOD("notify", "event", "id", "amount"), &Achievement::notify, DEFVAL(""), DEFVAL(1.0));
    ClassDB::bind_method(D_METHOD("evaluate"), &Achievement::evaluate);
    ClassDB::bind_method(D_METHOD("unlock"), &Achievement::unlock);
    ClassDB::bind_method(D_METHOD("reset"), &Achievement::reset);
    ClassDB::bind_method(D_METHOD("get_completed_condition_count"), &Achievement::get_completed_condition_count);
    ClassDB::bind_method(D_METHOD("has_progress"), &Achievement::has_progress);
    ClassDB::bind_method(D_METHOD("get_progress"), &Achievement::get_progress);
    ClassDB::bind_method(D_METHOD("get_progress_text"), &Achievement::get_progress_text);
    ClassDB::bind_method(D_METHOD("get_state"), &Achievement::get_state);
    ClassDB::bind_method(D_METHOD("set_state", "state"), &Achievement::set_state);

    BIND_ENUM_CONSTANT(CATEGORY_NONE);
    BIND_ENUM_CONSTANT(CATEGORY_SLAYER);
    BIND_ENUM_CONSTANT(CATEGORY_COLLECTOR);
    BIND_ENUM_CONSTANT(CATEGORY_EXPLORER);
    BIND_ENUM_CONSTANT(CATEGORY_CHALLENGER);
    BIND_ENUM_CONSTANT(COMPLETION_ALL);
    BIND_ENUM_CONSTANT(COMPLETION_ANY);
    BIND_ENUM_CONSTANT(TRACKER_AUTO);
    BIND_ENUM_CONSTANT(TRACKER_NONE);
    BIND_ENUM_CONSTANT(TRACKER_CONDITIONS_COMPLETED);
    BIND_ENUM_CONSTANT(TRACKER_FROM_CONDITION);

    ADD_GROUP(U"🏷️ Présentation", "");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "id"), "set_id", "get_id");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description", "get_description");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "icon", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_icon", "get_icon");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "locked_icon", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_locked_icon", "get_locked_icon");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "category", PROPERTY_HINT_ENUM, "None,Slayer,Collector,Explorer,Challenger"), "set_category", "get_category");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hidden"), "set_hidden", "is_hidden");
    ADD_GROUP(U"✅ Conditions", "");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "conditions", PROPERTY_HINT_ARRAY_TYPE, "AchievementCondition"), "set_conditions", "get_conditions");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "completion_mode", PROPERTY_HINT_ENUM, "All,Any"), "set_completion_mode", "get_completion_mode");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "progress_tracker", PROPERTY_HINT_ENUM, "Auto,None,Conditions Completed,From Condition"), "set_progress_tracker", "get_progress_tracker");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tracked_condition"), "set_tracked_condition", "get_tracked_condition");
    // L'état de jeu (is_unlocked, get_unlock_time) n'est pas une propriété : une propriété « unlocked »
    // masquerait le signal du même nom en GDScript, et l'état ne doit pas être enregistré dans la ressource.

    ADD_SIGNAL(MethodInfo("unlocked"));
    ADD_SIGNAL(MethodInfo("progressed", PropertyInfo(Variant::FLOAT, "progress")));
}
