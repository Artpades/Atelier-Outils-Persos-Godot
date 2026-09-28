#include "commun/module_entry.hpp"

#include "editor_test.hpp"
#include "test_example.hpp"

#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level)
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

    GDREGISTER_CLASS(TestExample);
}

void uninitialize_module(ModuleInitializationLevel p_level) {}

GAMEPLAYTOOLKIT_MODULE(exemples, U"🧪 Exemples")
