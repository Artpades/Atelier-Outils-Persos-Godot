#include "scene_hierarchy_tree.hpp"

#include "../📰Journal/custom_logger.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

static const char32_t *WINDOW_ICON = U"res://🖼️Editor Icons/🧰🧑‍💻/WindowEditor icon.png";

void SceneHierarchyTree::_ready()
{
    refresh_tree();
    connect("item_selected", callable_mp(this, &SceneHierarchyTree::on_item_selected));
}

void SceneHierarchyTree::refresh_tree()
{
    set_hide_root(false);
    set_columns(1);

    Node *root = scene_root_path.is_empty() ? find_scene_root() : get_node_or_null(scene_root_path);
    scene_root_id = root ? ObjectID(root->get_instance_id()) : ObjectID();

    if (!root)
    {
        UtilityFunctions::push_error(String(U"Impossible de trouver le noeud racine de la scène"));
        CustomLogger::log(get_name(), WINDOW_ICON, U"Impossible de trouver le noeud racine de la scène");
        return;
    }

    build_hierarchy();
}

Node *SceneHierarchyTree::find_scene_root() const
{
    if (!is_inside_tree())
    {
        return nullptr;
    }

    // Dans l'éditeur, la racine de l'arbre est l'éditeur lui-même : la scène éditée est enfouie dedans.
    if (!show_editor_nodes && Engine::get_singleton()->is_editor_hint())
    {
        return EditorInterface::get_singleton()->get_edited_scene_root();
    }

    Window *root = get_tree()->get_root();
    if (show_editor_nodes || show_all_non_editor_nodes)
    {
        // Les noeuds de l'éditeur sont filtrés plus tard dans populate_tree.
        return root;
    }

    TypedArray<Node> children = root->get_children();
    for (int64_t i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        if (child && !String(child->get_name()).begins_with("@"))
        {
            return child;
        }
    }
    return root;
}

void SceneHierarchyTree::build_hierarchy()
{
    clear();

    Node *scene_root = Object::cast_to<Node>(ObjectDB::get_instance(scene_root_id));
    if (!scene_root)
    {
        return;
    }

    TreeItem *root = create_item();
    populate_tree(scene_root, root);

    if (auto_expand_all)
    {
        set_collapsed_recursive(root, false);
    }
}

void SceneHierarchyTree::populate_tree(Node *node, TreeItem *tree_item)
{
    tree_item->set_text(0, node->get_name());
    tree_item->set_icon(0, get_icon_for_node(node));
    tree_item->set_custom_color(0, get_color_for_node(node));
    // ObjectID plutôt qu'un pointeur : un noeud libéré ne laisse pas de référence invalide dans l'arbre.
    tree_item->set_metadata(0, static_cast<int64_t>(node->get_instance_id()));

    TypedArray<Node> children = node->get_children();
    for (int64_t i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        if (!child || (!show_editor_nodes && is_editor_node(child)))
        {
            continue;
        }
        populate_tree(child, create_item(tree_item));
    }
}

bool SceneHierarchyTree::is_editor_node(Node *node) const
{
    String node_name = node->get_name();
    return node_name.begins_with("@") || node_name.contains("EditorNode");
}

Color SceneHierarchyTree::get_color_for_node(Node *node) const
{
    if (Object::cast_to<Node3D>(node))
        return Color(0.7f, 0.9f, 1.0f);
    if (Object::cast_to<Node2D>(node))
        return Color(0.7f, 1.0f, 0.7f);
    if (Object::cast_to<Control>(node))
        return Color(1.0f, 0.9f, 0.7f);
    if (Object::cast_to<Timer>(node))
        return Color(1.0f, 0.7f, 0.9f);
    if (Object::cast_to<Viewport>(node))
        return Color(0.9f, 0.9f, 0.9f);
    return Color(1.0f, 1.0f, 1.0f);
}

Ref<Texture2D> SceneHierarchyTree::get_icon_for_node(Node *node) const
{
    // Les icônes de classes n'existent que dans le thème de l'éditeur.
    if (!Engine::get_singleton()->is_editor_hint())
    {
        return Ref<Texture2D>();
    }

    Ref<Theme> theme = EditorInterface::get_singleton()->get_editor_theme();
    if (theme.is_null())
    {
        return Ref<Texture2D>();
    }

    StringName node_class = node->get_class();
    while (!node_class.is_empty() && !theme->has_icon(node_class, "EditorIcons"))
    {
        node_class = ClassDB::get_parent_class(node_class);
    }
    return node_class.is_empty() ? Ref<Texture2D>() : theme->get_icon(node_class, "EditorIcons");
}

Node *SceneHierarchyTree::get_node_from_item(TreeItem *item) const
{
    if (!item)
    {
        return nullptr;
    }
    Variant id = item->get_metadata(0);
    if (id.get_type() != Variant::INT)
    {
        return nullptr;
    }
    return Object::cast_to<Node>(ObjectDB::get_instance(ObjectID(static_cast<uint64_t>(static_cast<int64_t>(id)))));
}

void SceneHierarchyTree::on_item_selected()
{
    Node *selected_node = get_selected_node();
    if (!selected_node)
    {
        return;
    }

    CustomLogger::log(get_name(), WINDOW_ICON,
                      String(U"Noeud sélectionné : [color=cyan]") + selected_node->get_name() + "[/color] (Type: [color=yellow]" + selected_node->get_class() + "[/color])");
    CustomLogger::log(get_name(), WINDOW_ICON, "Chemin : [color=magenta]" + String(selected_node->get_path()) + "[/color]");

    emit_signal("node_selected", selected_node);
}

Node *SceneHierarchyTree::get_selected_node() const
{
    return get_node_from_item(get_selected());
}

TreeItem *SceneHierarchyTree::find_item_for_node(TreeItem *item, Node *node) const
{
    if (!item)
    {
        return nullptr;
    }
    if (get_node_from_item(item) == node)
    {
        return item;
    }
    for (TreeItem *child = item->get_first_child(); child; child = child->get_next())
    {
        if (TreeItem *found = find_item_for_node(child, node))
        {
            return found;
        }
    }
    return nullptr;
}

void SceneHierarchyTree::select_node(Node *node)
{
    TreeItem *item = find_item_for_node(get_root(), node);
    if (!item)
    {
        return;
    }

    for (TreeItem *parent = item->get_parent(); parent; parent = parent->get_parent())
    {
        parent->set_collapsed(false);
    }
    item->select(0);
    scroll_to_item(item);
}

void SceneHierarchyTree::expand_all()
{
    set_collapsed_recursive(get_root(), false);
}

void SceneHierarchyTree::collapse_all()
{
    set_collapsed_recursive(get_root(), true);
}

void SceneHierarchyTree::set_collapsed_recursive(TreeItem *item, bool collapsed)
{
    if (!item)
    {
        return;
    }
    item->set_collapsed(collapsed);
    for (TreeItem *child = item->get_first_child(); child; child = child->get_next())
    {
        set_collapsed_recursive(child, collapsed);
    }
}

void SceneHierarchyTree::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("refresh_tree"), &SceneHierarchyTree::refresh_tree);
    ClassDB::bind_method(D_METHOD("get_selected_node"), &SceneHierarchyTree::get_selected_node);
    ClassDB::bind_method(D_METHOD("select_node", "node"), &SceneHierarchyTree::select_node);
    ClassDB::bind_method(D_METHOD("expand_all"), &SceneHierarchyTree::expand_all);
    ClassDB::bind_method(D_METHOD("collapse_all"), &SceneHierarchyTree::collapse_all);

    ClassDB::bind_method(D_METHOD("set_show_editor_nodes", "value"), &SceneHierarchyTree::set_show_editor_nodes);
    ClassDB::bind_method(D_METHOD("get_show_editor_nodes"), &SceneHierarchyTree::get_show_editor_nodes);
    ClassDB::bind_method(D_METHOD("set_scene_root_path", "value"), &SceneHierarchyTree::set_scene_root_path);
    ClassDB::bind_method(D_METHOD("get_scene_root_path"), &SceneHierarchyTree::get_scene_root_path);
    ClassDB::bind_method(D_METHOD("set_auto_expand_all", "value"), &SceneHierarchyTree::set_auto_expand_all);
    ClassDB::bind_method(D_METHOD("get_auto_expand_all"), &SceneHierarchyTree::get_auto_expand_all);
    ClassDB::bind_method(D_METHOD("set_show_all_non_editor_nodes", "value"), &SceneHierarchyTree::set_show_all_non_editor_nodes);
    ClassDB::bind_method(D_METHOD("get_show_all_non_editor_nodes"), &SceneHierarchyTree::get_show_all_non_editor_nodes);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_editor_nodes"), "set_show_editor_nodes", "get_show_editor_nodes");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "scene_root_path"), "set_scene_root_path", "get_scene_root_path");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_expand_all"), "set_auto_expand_all", "get_auto_expand_all");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_all_non_editor_nodes"), "set_show_all_non_editor_nodes", "get_show_all_non_editor_nodes");

    ADD_SIGNAL(MethodInfo("node_selected", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_NODE_TYPE, "Node")));
}
