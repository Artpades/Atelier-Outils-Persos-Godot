#include "blackboard.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void Blackboard::set_values(const Dictionary &p_values)
{
    // Clés normalisées en String : StringName et String désignent la même entrée.
    values.clear();
    Array keys = p_values.keys();
    for (int i = 0; i < keys.size(); i++)
    {
        values[String(keys[i])] = p_values[keys[i]];
    }
    emit_changed();
}

void Blackboard::set_value(const String &p_key, const Variant &p_value)
{
    if (values.has(p_key) && values[p_key] == p_value && values[p_key].get_type() == p_value.get_type())
    {
        return;
    }
    values[p_key] = p_value;
    emit_signal("value_changed", p_key, p_value);
}

Variant Blackboard::get_value(const String &p_key, const Variant &p_default) const
{
    return values.has(p_key) ? values[p_key] : p_default;
}

void Blackboard::erase_value(const String &p_key)
{
    if (values.erase(p_key))
    {
        emit_signal("value_changed", p_key, Variant());
    }
}

void Blackboard::clear()
{
    values.clear();
    emit_changed();
}

PackedStringArray Blackboard::get_keys() const
{
    PackedStringArray keys;
    Array raw = values.keys();
    for (int i = 0; i < raw.size(); i++)
    {
        keys.push_back(String(raw[i]));
    }
    return keys;
}

// 🔗 Liaisons

void Blackboard::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_values", "values"), &Blackboard::set_values);
    ClassDB::bind_method(D_METHOD("get_values"), &Blackboard::get_values);
    ClassDB::bind_method(D_METHOD("set_value", "key", "value"), &Blackboard::set_value);
    ClassDB::bind_method(D_METHOD("get_value", "key", "default"), &Blackboard::get_value, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("has_value", "key"), &Blackboard::has_value);
    ClassDB::bind_method(D_METHOD("erase_value", "key"), &Blackboard::erase_value);
    ClassDB::bind_method(D_METHOD("clear"), &Blackboard::clear);
    ClassDB::bind_method(D_METHOD("get_keys"), &Blackboard::get_keys);

    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "values"), "set_values", "get_values");

    ADD_SIGNAL(MethodInfo("value_changed", PropertyInfo(Variant::STRING, "key"), PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)));
}
