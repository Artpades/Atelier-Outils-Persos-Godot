
#include "save_config.hpp"
#include "save_data.hpp"
#include "save_data_binder.hpp"
#include "save_file_access.hpp"
#include "save_manager.hpp"
#include "save_serializer.hpp"

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

    GDREGISTER_CLASS(SaveConfig);
    GDREGISTER_CLASS(SaveData);
    GDREGISTER_CLASS(SaveDataBinder);
    GDREGISTER_ABSTRACT_CLASS(SaveSerializer);
    GDREGISTER_ABSTRACT_CLASS(SaveFileAccess);
    GDREGISTER_ABSTRACT_CLASS(SaveManager);

    godot::print_line_rich("[color=dark_green]💾Module de sauvegarde et de debug chargé avec succès ![/color]");
}

static void uninitialize(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich("[color=dark_red]💾Module de sauvegarde déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT save_system_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize);
        init_obj.register_terminator(uninitialize);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
