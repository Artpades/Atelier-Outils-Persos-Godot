#include "drag_and_drop_system.hpp"
#include "draggable_component.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_drag_and_drop(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }

    GDREGISTER_CLASS(DraggableComponent); // @icon res://🖼️Editor Icons/🧭🌍/Grabbale Icon.png
    // Classe « runtime » : pas de glisser-déposer (ni de lecture de la souris) dans la scène éditée.
    GDREGISTER_RUNTIME_CLASS(DragAndDropSystem); // @icon res://🖼️Editor Icons/🧭🌍/Point And Click Icon.png

    godot::print_line_rich(U"[color=dark_green]✊📦🤚Module de glisser-déposer chargé avec succès ![/color]");
}

static void uninitialize_drag_and_drop(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]✊📦🤚Module de glisser-déposer déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT drag_and_drop_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_drag_and_drop);
        init_obj.register_terminator(uninitialize_drag_and_drop);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
