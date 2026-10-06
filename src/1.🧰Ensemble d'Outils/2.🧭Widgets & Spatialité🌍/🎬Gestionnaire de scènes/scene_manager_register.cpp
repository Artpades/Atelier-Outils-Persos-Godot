#include "scene_manager.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_scene_manager(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }

    // Classe « runtime » : pas de singleton enregistré ni de signal connecté dans l'éditeur.
    GDREGISTER_RUNTIME_CLASS(SceneManager); // @icon res://🖼️Editor Icons/🧭🌍/Scene Manager Icon.png

    godot::print_line_rich(U"[color=dark_green]🎬Module de gestion de scènes chargé avec succès ![/color]");
}

static void uninitialize_scene_manager(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]🎬Module de gestion de scènes déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT scene_manager_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_scene_manager);
        init_obj.register_terminator(uninitialize_scene_manager);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
