#ifndef SCENE_HIERARCHY_TREE_HPP
#define SCENE_HIERARCHY_TREE_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace godot
{
// 🌳 Affiche la hiérarchie des noeuds de la scène, et optionnellement ceux de l'éditeur.
class SceneHierarchyTree : public Tree
{
    GDCLASS(SceneHierarchyTree, Tree)

  private:
    bool show_editor_nodes = false;
    NodePath scene_root_path;
    bool auto_expand_all = false;
    bool show_all_non_editor_nodes = true;

    ObjectID scene_root_id;

    Node *find_scene_root() const;
    void build_hierarchy();
    void populate_tree(Node *node, TreeItem *tree_item);
    bool is_editor_node(Node *node) const;
    Color get_color_for_node(Node *node) const;
    Ref<Texture2D> get_icon_for_node(Node *node) const;
    Node *get_node_from_item(TreeItem *item) const;
    TreeItem *find_item_for_node(TreeItem *item, Node *node) const;
    void on_item_selected();

    static void set_collapsed_recursive(TreeItem *item, bool collapsed);

  protected:
    static void _bind_methods();

  public:
    void _ready() override;

    void refresh_tree();
    Node *get_selected_node() const;
    void select_node(Node *node);
    void expand_all();
    void collapse_all();

    void set_show_editor_nodes(bool value) { show_editor_nodes = value; }
    bool get_show_editor_nodes() const { return show_editor_nodes; }
    void set_scene_root_path(const NodePath &value) { scene_root_path = value; }
    NodePath get_scene_root_path() const { return scene_root_path; }
    void set_auto_expand_all(bool value) { auto_expand_all = value; }
    bool get_auto_expand_all() const { return auto_expand_all; }
    void set_show_all_non_editor_nodes(bool value) { show_all_non_editor_nodes = value; }
    bool get_show_all_non_editor_nodes() const { return show_all_non_editor_nodes; }
};
} // namespace godot

#endif // SCENE_HIERARCHY_TREE_HPP
