#include "pool_spawner.hpp"
#include "spawn_entry.hpp"
#include "spawner.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_spawner(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }

    // Ordre : une classe parente avant ses classes filles.
    GDREGISTER_CLASS(SpawnEntry);     // @icon res://🖼️Editor Icons/🧭🌍/SpawnerEntry Icon.png
    GDREGISTER_CLASS(SpawnPoolEntry); // @icon res://🖼️Editor Icons/🧭🌍/SpawnerEntry Icon.png
    // Classes « runtime » : pas de pré-allocation du pool dans la scène éditée.
    GDREGISTER_RUNTIME_CLASS(Spawner);     // @icon res://🖼️Editor Icons/🧭🌍/Spawner Icon.png
    GDREGISTER_RUNTIME_CLASS(PoolSpawner); // @icon res://🖼️Editor Icons/🧭🌍/Spawner Icon.png

    godot::print_line_rich(U"[color=dark_green]🏭Module de génération d'objets chargé avec succès ![/color]");
}

static void uninitialize_spawner(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]🏭Module de génération d'objets déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT spawner_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_spawner);
        init_obj.register_terminator(uninitialize_spawner);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
