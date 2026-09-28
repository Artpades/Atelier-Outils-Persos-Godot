#include "commun/module_entry.hpp"

#include "gizmos_2d.hpp"
#include "gizmos_3d.hpp"

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_CLASS(Gizmos2D);
    GDREGISTER_CLASS(Gizmos3D);
}

void uninitialize_module(ModuleInitializationLevel p_level) {}

GAMEPLAYTOOLKIT_MODULE(gizmos, U"🐛 Gizmos")
