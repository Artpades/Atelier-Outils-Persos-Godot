#include "layout_linker.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

void LayoutLinker::_bind_methods()
{
    ClassDB::bind_static_method("LayoutLinker", D_METHOD("get_singleton"), &LayoutLinker::get_singleton);
    ClassDB::bind_method(D_METHOD("is_started"), &LayoutLinker::is_started);
    ClassDB::bind_method(D_METHOD("get_applied_count"), &LayoutLinker::get_applied_count);
}

void LayoutLinker::create()
{
    if (singleton || Engine::get_singleton()->is_editor_hint())
    {
        return; // dans l'éditeur, aucun lien ne doit se déclencher
    }
    singleton = memnew(LayoutLinker);
    // Le SceneTree n'existe pas encore au chargement du module : branchement à la première image.
    callable_mp(singleton, &LayoutLinker::start).call_deferred();
}

void LayoutLinker::destroy()
{
    if (singleton)
    {
        memdelete(singleton);
        singleton = nullptr;
    }
}

LayoutLinker *LayoutLinker::get_singleton() { return singleton; }

void LayoutLinker::start()
{
    SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (started || !tree)
    {
        return;
    }
    started = true;
    database = LayoutDatabase::load_project_database();
    if (database.is_null())
    {
        return; // pas de base : rien à appliquer
    }
    tree->connect("node_added", callable_mp(this, &LayoutLinker::_on_node_added));
    _apply_tree(tree->get_root()); // nœuds ajoutés avant le branchement
}

bool LayoutLinker::is_started() const { return started; }
int64_t LayoutLinker::get_applied_count() const { return applied_count; }

void LayoutLinker::_apply(Node *p_node)
{
    Ref<TypeLayout> layout = database.is_valid() ? database->find_layout(p_node) : Ref<TypeLayout>();
    Ref<DataRegistry> registry = layout.is_valid() ? layout->get_data_registry() : Ref<DataRegistry>();
    if (registry.is_null())
    {
        return;
    }
    registry->apply_links(p_node); // idempotent
    applied_count++;
}

void LayoutLinker::_apply_tree(Node *p_node)
{
    if (!p_node)
    {
        return;
    }
    _apply(p_node);
    for (int i = 0; i < p_node->get_child_count(); i++)
    {
        _apply_tree(p_node->get_child(i));
    }
}

void LayoutLinker::_on_node_added(Node *p_node) { _apply(p_node); }
