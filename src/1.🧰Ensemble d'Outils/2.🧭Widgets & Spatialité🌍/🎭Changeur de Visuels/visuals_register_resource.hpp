#ifndef VISUALS_REGISTER_RESOURCE_HPP
#define VISUALS_REGISTER_RESOURCE_HPP

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace godot
{
/// 🎭 Association entre un nœud cible et les ressources visuelles à lui affecter.
/// Selon le type du nœud : texture (Sprite2D/3D, Polygon2D, TextureRect), mesh (MeshInstance2D/3D),
/// ou scène instanciée sous le nœud (tout autre type).
class VisualsRegister : public Resource
{
    GDCLASS(VisualsRegister, Resource)

  public:
    void set_node_visual(const NodePath &p_path) { node_visual = p_path; }
    NodePath get_node_visual() const { return node_visual; }
    void set_texture(const Ref<Texture2D> &p_texture) { texture = p_texture; }
    Ref<Texture2D> get_texture() const { return texture; }
    void set_mesh(const Ref<Mesh> &p_mesh) { mesh = p_mesh; }
    Ref<Mesh> get_mesh() const { return mesh; }
    void set_modeling_import(const Ref<PackedScene> &p_scene) { modeling_import = p_scene; }
    Ref<PackedScene> get_modeling_import() const { return modeling_import; }

  protected:
    static void _bind_methods();

  private:
    /// Chemin du nœud récepteur, relatif au nœud de contexte (le VisualsSwapper)
    NodePath node_visual;
    Ref<Texture2D> texture;
    Ref<Mesh> mesh;
    /// Scène instanciée sous le nœud cible, à la place de ses enfants
    Ref<PackedScene> modeling_import;
};
} // namespace godot

#endif // VISUALS_REGISTER_RESOURCE_HPP
