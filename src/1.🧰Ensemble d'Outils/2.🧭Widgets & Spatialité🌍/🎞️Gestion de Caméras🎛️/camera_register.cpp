#include "camera_manager.hpp"
#include "camera_panner.hpp"
#include "orbital_camera.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_camera(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }

    // Classes « runtime » : leur code ne s'exécute pas dans l'éditeur (pas de déplacement de caméra
    // ni de singleton créé dans la scène éditée), seules leurs propriétés y sont modifiables.
    GDREGISTER_RUNTIME_CLASS(CameraManager); // @icon res://🖼️Editor Icons/🎥🎞️/CameraManager Icon.png
    GDREGISTER_RUNTIME_CLASS(CameraPanner);  // @icon res://🖼️Editor Icons/🎥🎞️/VirtualCamera Icon.png
    GDREGISTER_RUNTIME_CLASS(OrbitalCamera); // @icon res://🖼️Editor Icons/🎥🎞️/OrbitalCamera Icon.png

    godot::print_line_rich(U"[color=dark_green]🎞️Module de gestion de caméras chargé avec succès ![/color]");
}

static void uninitialize_camera(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]🎞️Module de gestion de caméras déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT camera_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_camera);
        init_obj.register_terminator(uninitialize_camera);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
