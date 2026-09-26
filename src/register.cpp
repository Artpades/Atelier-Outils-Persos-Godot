#include "register.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/editor_test.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/⚙️Générateur de Gestionnaire/manager_builder.hpp"

#include "gdextension_interface.h"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/core/class_db.hpp"
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

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/✋Handlers/handlers_manager.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal/custom_logger.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🌳Hiérarchie de scène/scene_hierarchy_tree.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🧩Générateur de Widget d'éditions/widget_property_modifier_builder.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🧩Générateur de Widget d'éditions/Callback pour les type/property_callback.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🧩Générateur de Widget d'éditions/🔍Inspecteur procédural/inspector_binding.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🧩Générateur de Widget d'éditions/🔍Inspecteur procédural/procedural_inspector.hpp"

#include "1.🧰Ensemble d'Outils/1.📁Intercepteurs de données🔌/💾Manipulateur de données🔐/save_config.hpp"
#include "1.🧰Ensemble d'Outils/1.📁Intercepteurs de données🔌/💾Manipulateur de données🔐/save_data.hpp"
#include "1.🧰Ensemble d'Outils/1.📁Intercepteurs de données🔌/💾Manipulateur de données🔐/save_data_binder.hpp"
#include "1.🧰Ensemble d'Outils/1.📁Intercepteurs de données🔌/💾Manipulateur de données🔐/save_file_access.hpp"
#include "1.🧰Ensemble d'Outils/1.📁Intercepteurs de données🔌/💾Manipulateur de données🔐/save_manager.hpp"
#include "1.🧰Ensemble d'Outils/1.📁Intercepteurs de données🔌/💾Manipulateur de données🔐/save_serializer.hpp"

using namespace godot;

void initialize(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        GDREGISTER_CLASS(EditorTest);

        // Le niveau EDITOR est aussi initialisé quand on lance une scène depuis l'éditeur (même binaire, sans
        // éditeur) : un EditorPlugin n'y est pas instanciable, et memnew planterait le jeu.
        if (!Engine::get_singleton()->is_editor_hint())
        {
            return;
        }

        // Création de l'instance pour exécuter la méthode
        EditorTest *test_instance = memnew(EditorTest);
        godot::Callable(test_instance, "creer_bouton_editeur").call_deferred();
    }
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    // Implentation de la classe TestScript dans le registre de classes de Godot
    GDREGISTER_CLASS(ManagerBuilder);
    GDREGISTER_CLASS(WindowEditor);
    GDREGISTER_CLASS(Gizmos2D);
    GDREGISTER_CLASS(Gizmos3D);

    GDREGISTER_CLASS(HandlersManager);

    GDREGISTER_ABSTRACT_CLASS(CustomLogger);
    GDREGISTER_CLASS(SceneHierarchyTree);

    GDREGISTER_CLASS(WidgetPropertyModifierBuilder);

    GDREGISTER_CLASS(PropertyCallback);

    GDREGISTER_INTERNAL_CLASS(InspectorBinding);
    GDREGISTER_CLASS(ProceduralInspector);

    GDREGISTER_CLASS(SaveConfig);
    GDREGISTER_CLASS(SaveData);
    GDREGISTER_CLASS(SaveDataBinder);
    GDREGISTER_ABSTRACT_CLASS(SaveSerializer);
    GDREGISTER_ABSTRACT_CLASS(SaveFileAccess);
    GDREGISTER_ABSTRACT_CLASS(SaveManager);

    GDREGISTER_CLASS(TestExample);

    godot::print_line_rich("[color=dark_green]Module d'extension pour scène chargé avec succès ![/color]");
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