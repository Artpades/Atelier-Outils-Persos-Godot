#include "save_data.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

SaveData::SaveData()
{
    file_config.instantiate();
}

void SaveData::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_system_id", "id"), &SaveData::set_system_id);
    ClassDB::bind_method(D_METHOD("get_system_id"), &SaveData::get_system_id);
    ClassDB::bind_method(D_METHOD("set_data", "data"), &SaveData::set_data);
    ClassDB::bind_method(D_METHOD("get_data"), &SaveData::get_data);
    ClassDB::bind_method(D_METHOD("set_is_encrypted", "encrypted"), &SaveData::set_is_encrypted);
    ClassDB::bind_method(D_METHOD("get_is_encrypted"), &SaveData::get_is_encrypted);
    ClassDB::bind_method(D_METHOD("set_encryption_key", "key"), &SaveData::set_encryption_key);
    ClassDB::bind_method(D_METHOD("get_encryption_key"), &SaveData::get_encryption_key);
    ClassDB::bind_method(D_METHOD("set_file_config", "config"), &SaveData::set_file_config);
    ClassDB::bind_method(D_METHOD("get_file_config"), &SaveData::get_file_config);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "system_id"), "set_system_id", "get_system_id");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "data"), "set_data", "get_data");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_encrypted"), "set_is_encrypted", "get_is_encrypted");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "encryption_key"), "set_encryption_key", "get_encryption_key");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "file_config", PROPERTY_HINT_RESOURCE_TYPE, "SaveConfig"), "set_file_config", "get_file_config");
}
