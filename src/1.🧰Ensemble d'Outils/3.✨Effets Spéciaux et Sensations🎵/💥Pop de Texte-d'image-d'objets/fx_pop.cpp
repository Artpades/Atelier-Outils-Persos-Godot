#include "fx_pop.hpp"

#include "pop_spawner.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void FXPop::_play(Node *p_target)
{
    if (!p_target || !p_target->is_inside_tree())
    {
        return;
    }

    PopRequest request;
    request.text = text;
    request.texture = texture;
    request.scene = scene;
    request.color = color;
    request.offset = offset;
    request.unit_scale_3d = unit_scale_3d;
    request.screen_space_3d = screen_space_3d;
    for (int i = 0; i < count; i++)
    {
        PopSpawner::create_pop(p_target, style, request);
    }
}

void FXPop::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_style", "value"), &FXPop::set_style);
    ClassDB::bind_method(D_METHOD("get_style"), &FXPop::get_style);
    ClassDB::bind_method(D_METHOD("set_text", "value"), &FXPop::set_text);
    ClassDB::bind_method(D_METHOD("get_text"), &FXPop::get_text);
    ClassDB::bind_method(D_METHOD("set_texture", "value"), &FXPop::set_texture);
    ClassDB::bind_method(D_METHOD("get_texture"), &FXPop::get_texture);
    ClassDB::bind_method(D_METHOD("set_scene", "value"), &FXPop::set_scene);
    ClassDB::bind_method(D_METHOD("get_scene"), &FXPop::get_scene);
    ClassDB::bind_method(D_METHOD("set_color", "value"), &FXPop::set_color);
    ClassDB::bind_method(D_METHOD("get_color"), &FXPop::get_color);
    ClassDB::bind_method(D_METHOD("set_count", "value"), &FXPop::set_count);
    ClassDB::bind_method(D_METHOD("get_count"), &FXPop::get_count);
    ClassDB::bind_method(D_METHOD("set_offset", "value"), &FXPop::set_offset);
    ClassDB::bind_method(D_METHOD("get_offset"), &FXPop::get_offset);
    ClassDB::bind_method(D_METHOD("set_unit_scale_3d", "value"), &FXPop::set_unit_scale_3d);
    ClassDB::bind_method(D_METHOD("get_unit_scale_3d"), &FXPop::get_unit_scale_3d);
    ClassDB::bind_method(D_METHOD("set_screen_space_3d", "value"), &FXPop::set_screen_space_3d);
    ClassDB::bind_method(D_METHOD("get_screen_space_3d"), &FXPop::get_screen_space_3d);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "style", PROPERTY_HINT_RESOURCE_TYPE, "PopStyle"), "set_style", "get_style");

    ADD_GROUP("Content", "");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_scene", "get_scene");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "count", PROPERTY_HINT_RANGE, "1,50,1,or_greater"), "set_count", "get_count");

    ADD_GROUP("Placement", "");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "offset"), "set_offset", "get_offset");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "unit_scale_3d", PROPERTY_HINT_RANGE, "0.0001,1,0.0001,or_greater,suffix:m/px"), "set_unit_scale_3d", "get_unit_scale_3d");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "screen_space_3d"), "set_screen_space_3d", "get_screen_space_3d");
}
