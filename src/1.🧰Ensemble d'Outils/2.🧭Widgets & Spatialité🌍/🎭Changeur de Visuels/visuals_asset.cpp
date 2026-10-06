#include "visuals_asset.hpp"

#include <godot_cpp/classes/mesh_instance2d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/sprite3d.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void VisualsAsset::swap_visuals(Node *p_context, const Ref<VisualsRegister> &p_register)
{
    if (!p_context || p_register.is_null() || p_register->get_node_visual().is_empty())
    {
        return;
    }

    Node *target = p_context->get_node_or_null(p_register->get_node_visual());
    if (!target)
    {
        return;
    }

    const Ref<Texture2D> texture = p_register->get_texture();
    const Ref<Mesh> mesh = p_register->get_mesh();

    // Nœuds à texture : 2D, 3D et interface
    if (Sprite2D *sprite_2d = Object::cast_to<Sprite2D>(target))
    {
        if (texture.is_valid())
        {
            sprite_2d->set_texture(texture);
        }
    }
    else if (Sprite3D *sprite_3d = Object::cast_to<Sprite3D>(target))
    {
        if (texture.is_valid())
        {
            sprite_3d->set_texture(texture);
        }
    }
    else if (Polygon2D *polygon_2d = Object::cast_to<Polygon2D>(target))
    {
        if (texture.is_valid())
        {
            polygon_2d->set_texture(texture);
        }
    }
    else if (TextureRect *texture_rect = Object::cast_to<TextureRect>(target))
    {
        if (texture.is_valid())
        {
            texture_rect->set_texture(texture);
        }
    }
    // Instances de maillage (2D et 3D)
    else if (MeshInstance3D *mesh_instance_3d = Object::cast_to<MeshInstance3D>(target))
    {
        if (mesh.is_valid())
        {
            mesh_instance_3d->set_mesh(mesh);
        }
    }
    else if (MeshInstance2D *mesh_instance_2d = Object::cast_to<MeshInstance2D>(target))
    {
        if (mesh.is_valid())
        {
            mesh_instance_2d->set_mesh(mesh);
        }
    }
    // Autre type : le nœud sert de conteneur, son contenu est remplacé par la scène
    else if (p_register->get_modeling_import().is_valid())
    {
        Node *instance = p_register->get_modeling_import()->instantiate();
        if (!instance)
        {
            return;
        }
        for (int64_t i = target->get_child_count() - 1; i >= 0; i--)
        {
            target->get_child(i)->queue_free();
        }
        target->add_child(instance);
    }
}

void VisualsAsset::apply(Node *p_context) const
{
    for (int64_t i = 0; i < visuals_registers.size(); i++)
    {
        swap_visuals(p_context, visuals_registers[i]);
    }
}

void VisualsAsset::_bind_methods()
{
    ClassDB::bind_static_method("VisualsAsset", D_METHOD("swap_visuals", "context", "register"), &VisualsAsset::swap_visuals);
    ClassDB::bind_method(D_METHOD("apply", "context"), &VisualsAsset::apply);

    ClassDB::bind_method(D_METHOD("set_suit_type_name", "name"), &VisualsAsset::set_suit_type_name);
    ClassDB::bind_method(D_METHOD("get_suit_type_name"), &VisualsAsset::get_suit_type_name);
    ClassDB::bind_method(D_METHOD("set_visuals_registers", "registers"), &VisualsAsset::set_visuals_registers);
    ClassDB::bind_method(D_METHOD("get_visuals_registers"), &VisualsAsset::get_visuals_registers);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "suit_type_name"), "set_suit_type_name", "get_suit_type_name");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "visuals_registers", PROPERTY_HINT_ARRAY_TYPE, "VisualsRegister"), "set_visuals_registers", "get_visuals_registers");
}
