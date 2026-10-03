#include "🐛Gizmos/gizmos_2d.hpp"
#include "🐛Gizmos/gizmos_3d.hpp"

#include "📰Journal Console/custom_logger.hpp"

#include "⚙️Générateur de Gestionnaire/manager_builder.hpp"

#include "🪟Fenêtre d'édition/window_editor.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/editor_test.hpp"

using namespace godot;

// Enregistrement des outils

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        GDREGISTER_CLASS(EditorTest);

        // Création de l'instance pour exécuter la méthode
        EditorTest *test_instance = memnew(EditorTest);
        godot::Callable(test_instance, "creer_bouton_editeur").call_deferred();
    }

    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }
    GDREGISTER_CLASS(L);

    GDREGISTER_CLASS(ManagerBuilder);
    GDREGISTER_CLASS(WindowEditor);
    GDREGISTER_CLASS(Gizmos2D);
    GDREGISTER_CLASS(Gizmos3D);

    godot::print_line_rich("[color=dark_green]🛠️Module d'outils d'edition et de debug chargé avec succès ![/color]");
}

static void uninitialize(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich("[color=dark_red]🛠️Module d'outils d'edition et de debug déchargé avec succès ![/color]");
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
