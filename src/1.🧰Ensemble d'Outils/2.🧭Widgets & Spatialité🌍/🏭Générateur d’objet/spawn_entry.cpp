#include "spawn_entry.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void SpawnEntry::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_positions", "positions"), &SpawnEntry::set_positions);
    ClassDB::bind_method(D_METHOD("get_positions"), &SpawnEntry::get_positions);
    ClassDB::bind_method(D_METHOD("set_scenes", "scenes"), &SpawnEntry::set_scenes);
    ClassDB::bind_method(D_METHOD("get_scenes"), &SpawnEntry::get_scenes);
    ClassDB::bind_method(D_METHOD("set_positions_selection_mode", "mode"), &SpawnEntry::set_positions_selection_mode);
    ClassDB::bind_method(D_METHOD("get_positions_selection_mode"), &SpawnEntry::get_positions_selection_mode);
    ClassDB::bind_method(D_METHOD("set_scenes_selection_mode", "mode"), &SpawnEntry::set_scenes_selection_mode);
    ClassDB::bind_method(D_METHOD("get_scenes_selection_mode"), &SpawnEntry::get_scenes_selection_mode);

    BIND_ENUM_CONSTANT(SELECTION_SEQUENTIAL);
    BIND_ENUM_CONSTANT(SELECTION_RANDOM);
    BIND_ENUM_CONSTANT(SELECTION_ALL);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "positions", PROPERTY_HINT_ARRAY_TYPE, "NodePath"), "set_positions", "get_positions");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "scenes", PROPERTY_HINT_ARRAY_TYPE, "PackedScene"), "set_scenes", "get_scenes");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "positions_selection_mode", PROPERTY_HINT_ENUM, "Sequential,Random,All"), "set_positions_selection_mode", "get_positions_selection_mode");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "scenes_selection_mode", PROPERTY_HINT_ENUM, "Sequential,Random,All"), "set_scenes_selection_mode", "get_scenes_selection_mode");
}

void SpawnPoolEntry::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_initial_pool_size_per_scene", "size"), &SpawnPoolEntry::set_initial_pool_size_per_scene);
    ClassDB::bind_method(D_METHOD("get_initial_pool_size_per_scene"), &SpawnPoolEntry::get_initial_pool_size_per_scene);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "initial_pool_size_per_scene", PROPERTY_HINT_RANGE, "0,100,1,or_greater"), "set_initial_pool_size_per_scene", "get_initial_pool_size_per_scene");
}
