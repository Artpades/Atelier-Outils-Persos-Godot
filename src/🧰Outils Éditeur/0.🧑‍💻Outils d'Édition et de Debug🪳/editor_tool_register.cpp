#include "🧰Outils Éditeur/0.🧑‍💻Outils d'Édition et de Debug🪳/🐛Gizmos/gizmos_2d.hpp"
#include "🧰Outils Éditeur/0.🧑‍💻Outils d'Édition et de Debug🪳/🐛Gizmos/gizmos_3d.hpp"
#include "🪟Fenêtre d'édition/window_editor.hpp"

using namespace godot;

static void initialize(ModuleInitializationLevel p_level)
{

    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }

    GDREGISTER_CLASS(WindowEditor);
    GDREGISTER_CLASS(Gizmos2D);
    GDREGISTER_CLASS(Gizmos3D);

    godot::print_line_rich(U"[color=dark_green]🛠️Module 🧑‍💻d'outils d'edition et de debug chargé avec succès ![/color]");
}

static void uninitialize(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]🛠️Module 🧑‍💻d'outils d'edition et de debug déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT tool_editor_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize);
        init_obj.register_terminator(uninitialize);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}