#include "visuals_register_resource.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void VisualsRegister::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_node_visual", "path"), &VisualsRegister::set_node_visual);
    ClassDB::bind_method(D_METHOD("get_node_visual"), &VisualsRegister::get_node_visual);
    ClassDB::bind_method(D_METHOD("set_texture", "texture"), &VisualsRegister::set_texture);
    ClassDB::bind_method(D_METHOD("get_texture"), &VisualsRegister::get_texture);
    ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &VisualsRegister::set_mesh);
    ClassDB::bind_method(D_METHOD("get_mesh"), &VisualsRegister::get_mesh);
    ClassDB::bind_method(D_METHOD("set_modeling_import", "scene"), &VisualsRegister::set_modeling_import);
    ClassDB::bind_method(D_METHOD("get_modeling_import"), &VisualsRegister::get_modeling_import);

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "node_visual"), "set_node_visual", "get_node_visual");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_mesh", "get_mesh");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "modeling_import", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_modeling_import", "get_modeling_import");
}
