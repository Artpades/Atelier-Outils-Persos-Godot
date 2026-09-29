#include "logger_register.hpp"
#include "custom_logger.hpp"

#include <godot_cpp/core/print_string.hpp>

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_logger(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }
    GDREGISTER_CLASS(L);
    godot::print_line_rich("[color=dark_green]📰Module de Journalisation chargé avec succès ![/color]");
}

static void uninitialize_logger(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich("[color=dark_red]📰Module de Journalisation déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT logger_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_logger);
        init_obj.register_terminator(uninitialize_logger);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
