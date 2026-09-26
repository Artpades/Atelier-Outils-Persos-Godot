#include "save_config.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void SaveConfig::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_save_directory", "directory"), &SaveConfig::set_save_directory);
    ClassDB::bind_method(D_METHOD("get_save_directory"), &SaveConfig::get_save_directory);
    ClassDB::bind_method(D_METHOD("set_extension", "extension"), &SaveConfig::set_extension);
    ClassDB::bind_method(D_METHOD("get_extension"), &SaveConfig::get_extension);
    ClassDB::bind_method(D_METHOD("set_encrypted_extension", "extension"), &SaveConfig::set_encrypted_extension);
    ClassDB::bind_method(D_METHOD("get_encrypted_extension"), &SaveConfig::get_encrypted_extension);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "save_directory"), "set_save_directory", "get_save_directory");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "extension"), "set_extension", "get_extension");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "encrypted_extension"), "set_encrypted_extension", "get_encrypted_extension");
}
