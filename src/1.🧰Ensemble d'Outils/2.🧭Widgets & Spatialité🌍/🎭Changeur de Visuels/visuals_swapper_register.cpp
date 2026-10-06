#include "visuals_asset.hpp"
#include "visuals_register_resource.hpp"
#include "visuals_swapper.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_visuals_swapper(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }

    GDREGISTER_CLASS(VisualsRegister); // @icon res://🖼️Editor Icons/🧭🌍/VisualRegister.png
    GDREGISTER_CLASS(VisualsAsset);    // @icon res://🖼️Editor Icons/🧭🌍/VisualAssets.png
    // Classe d'outil : swap() reste utilisable depuis un script d'éditeur pour prévisualiser une tenue.
    GDREGISTER_CLASS(VisualsSwapper); // @icon res://🖼️Editor Icons/🧭🌍/SpriteSwapper Icon.png

    godot::print_line_rich(U"[color=dark_green]🎭Module de changement de visuels chargé avec succès ![/color]");
}

static void uninitialize_visuals_swapper(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]🎭Module de changement de visuels déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT visuals_swapper_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_visuals_swapper);
        init_obj.register_terminator(uninitialize_visuals_swapper);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
