#ifndef MODULE_ENTRY_HPP
#define MODULE_ENTRY_HPP

#include "gdextension_interface.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

// 🚪 Point d'entrée d'un module GDExtension.
//
// Chaque module définit, dans son register_types.cpp :
//   void initialize_module(ModuleInitializationLevel p_level)   -> enregistre ses classes
//   void uninitialize_module(ModuleInitializationLevel p_level)
// puis appelle GAMEPLAYTOOLKIT_MODULE(<id>, U"<nom lisible>").
//
// Le symbole d'entrée vaut gameplaytoolkit_<id>_init : c'est celui que le Sconstruct écrit dans le
// .gdextension du module (l'<id> doit donc être celui de module.json).
#define GAMEPLAYTOOLKIT_MODULE(p_id, p_name)                                                                                                                                 \
    static void _module_initialize(godot::ModuleInitializationLevel p_level)                                                                                                \
    {                                                                                                                                                                        \
        initialize_module(p_level);                                                                                                                                          \
        if (p_level == godot::MODULE_INITIALIZATION_LEVEL_SCENE)                                                                                                             \
        {                                                                                                                                                                    \
            godot::print_line_rich(godot::String(U"[color=dark_green]Module « ") + godot::String(p_name) + godot::String(U" » chargé ![/color]"));                          \
        }                                                                                                                                                                    \
    }                                                                                                                                                                        \
    static void _module_uninitialize(godot::ModuleInitializationLevel p_level)                                                                                              \
    {                                                                                                                                                                        \
        uninitialize_module(p_level);                                                                                                                                        \
        if (p_level == godot::MODULE_INITIALIZATION_LEVEL_SCENE)                                                                                                             \
        {                                                                                                                                                                    \
            godot::print_line_rich(godot::String(U"[color=dark_red]Module « ") + godot::String(p_name) + godot::String(U" » déchargé ![/color]"));                          \
        }                                                                                                                                                                    \
    }                                                                                                                                                                        \
    extern "C"                                                                                                                                                               \
    {                                                                                                                                                                        \
        GDExtensionBool GDE_EXPORT gameplaytoolkit_##p_id##_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library,             \
                                                                 GDExtensionInitialization *r_initialization)                                                               \
        {                                                                                                                                                                    \
            godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);                                                               \
            init_obj.register_initializer(_module_initialize);                                                                                                             \
            init_obj.register_terminator(_module_uninitialize);                                                                                                            \
            init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);                                                                  \
            return init_obj.init();                                                                                                                                          \
        }                                                                                                                                                                    \
    }

#endif // MODULE_ENTRY_HPP
