#include "mobile_input.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }

    GDREGISTER_CLASS(MobileInput); // @icon res://🖼️Editor Icons/🎮🔌/InputScreenSphere Icon.png

    godot::print_line_rich(U"[color=dark_green]📱Module Entrées mobiles chargé avec succès ![/color]");
}

static void uninitialize(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]📱Module Entrées mobiles déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT mobile_input_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize);
        init_obj.register_terminator(uninitialize);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
