#include "commun/module_entry.hpp"

#include "scene_hierarchy_tree.hpp"

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_CLASS(SceneHierarchyTree);
}

void uninitialize_module(ModuleInitializationLevel p_level) {}

GAMEPLAYTOOLKIT_MODULE(scene_hierarchy, U"🌳 Hiérarchie de scène")
