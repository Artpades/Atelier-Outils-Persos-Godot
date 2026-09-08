#include "register.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🧩Générateur de Widget d'éditions/Callback pour les type/property_bool_callback.hpp"
#include "gdextension_interface.h"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/godot.hpp"

// Ajouter les Includes des classes que vous souhaitez enregistrer ici
#include "test_example.hpp"
// #include "🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🪛Objets
// d'édition/🪟Fenêtre d'édition/windoweditor.hpp" #include "🧰Ensemble
// d'Outils/1.📁Intercepteurs de données/💾Manipulateur de données🔐/⚙️Générateur
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🪟Fenêtre d'édition/window_editor.hpp"
// de Gestionnaire/manager_builder_wrapper.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos_2d.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos_3d.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🧩Générateur de Widget d'éditions/widget_property_modifier_builder.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🧩Générateur de Widget d'éditions/Callback pour les type/property_bool_callback.hpp"

using namespace godot;

void initialize(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    // Implentation de la classe TestScript dans le registre de classes de Godot
    GDREGISTER_CLASS(WindowEditor);
    GDREGISTER_CLASS(Gizmos2D);
    GDREGISTER_CLASS(Gizmos3D);
    GDREGISTER_CLASS(WidgetPropertyModifierBuilder);

    GDREGISTER_CLASS(PropertyBoolCallback);

    GDREGISTER_CLASS(TestExample);

    godot::print_line_rich("[color=dark_green]Module d'extension chargé avec succès ![/color]");
}

void uninitialize(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    godot::print_line_rich("[color=dark_red]Module d'extension déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize);
        init_obj.register_terminator(uninitialize);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}