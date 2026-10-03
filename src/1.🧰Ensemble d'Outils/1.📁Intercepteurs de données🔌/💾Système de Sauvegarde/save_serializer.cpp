#include "save_serializer.hpp"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

String SaveSerializer::serialize(const Ref<SaveData> &p_save_data)
{
    ERR_FAIL_COND_V_MSG(p_save_data.is_null(), String(), "SaveSerializer : SaveData nul.");

    Dictionary dict;
    dict["system_id"] = p_save_data->get_system_id();
    dict["data"] = p_save_data->get_data();
    return JSON::stringify(dict);
}

Ref<SaveData> SaveSerializer::deserialize(const String &p_json)
{
    Variant parsed = JSON::parse_string(p_json);
    if (parsed.get_type() != Variant::DICTIONARY)
    {
        return Ref<SaveData>();
    }

    Dictionary dict = parsed;
    Ref<SaveData> save_data;
    save_data.instantiate();
    save_data->set_system_id(dict.get("system_id", String()));

    Variant data = dict.get("data", Dictionary());
    if (data.get_type() == Variant::DICTIONARY)
    {
        save_data->set_data(data);
    }
    return save_data;
}

void SaveSerializer::_bind_methods()
{
    ClassDB::bind_static_method("SaveSerializer", D_METHOD("serialize", "save_data"), &SaveSerializer::serialize);
    ClassDB::bind_static_method("SaveSerializer", D_METHOD("deserialize", "json"), &SaveSerializer::deserialize);
}