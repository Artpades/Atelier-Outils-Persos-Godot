#include "commun/module_entry.hpp"

#include "camera_manager.hpp"

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_CLASS(CameraManager);
}

void uninitialize_module(ModuleInitializationLevel p_level) {}

GAMEPLAYTOOLKIT_MODULE(camera, U"🎞️ Gestion de Caméras")
