#include "🤾Système de Comportement/🧩Commun/behavior_debugger.hpp"
#include "🤾Système de Comportement/🧩Commun/blackboard.hpp"

#include "🤾Système de Comportement/🤖HFSM/state.hpp"
#include "🤾Système de Comportement/🤖HFSM/state_machine.hpp"
#include "🤾Système de Comportement/🤖HFSM/state_transition.hpp"

#include "🤾Système de Comportement/🌿Arbre de Comportement/behavior_tree.hpp"
#include "🤾Système de Comportement/🌿Arbre de Comportement/bt_composites.hpp"
#include "🤾Système de Comportement/🌿Arbre de Comportement/bt_decorators.hpp"
#include "🤾Système de Comportement/🌿Arbre de Comportement/bt_leaves.hpp"
#include "🤾Système de Comportement/🌿Arbre de Comportement/bt_node.hpp"

#include "🤾Système de Comportement/🗺️Graphe de comportement/behavior_graph_view.hpp"

#include "🌟Système de Capacité/ability_resources.hpp"
#include "🌟Système de Capacité/ability_system.hpp"

#include "💬Système de Dialogue/dialogue_box.hpp"
#include "💬Système de Dialogue/dialogue_resource.hpp"
#include "💬Système de Dialogue/dialogue_system.hpp"
#include "💬Système de Dialogue/🧑‍💻Éditeur/dialogue_editor_window.hpp"

#include "🧑Personnage (🏌️Joueur, 🤖Automate, PNG)/bt_character_leaves.hpp"
#include "🧑Personnage (🏌️Joueur, 🤖Automate, PNG)/character_2d.hpp"
#include "🧑Personnage (🏌️Joueur, 🤖Automate, PNG)/character_3d.hpp"
#include "🧑Personnage (🏌️Joueur, 🤖Automate, PNG)/character_controllers.hpp"
#include "🧑Personnage (🏌️Joueur, 🤖Automate, PNG)/character_registry.hpp"
#include "🧑Personnage (🏌️Joueur, 🤖Automate, PNG)/health_component.hpp"
#include "🧑Personnage (🏌️Joueur, 🤖Automate, PNG)/interactable.hpp"
#include "🤾Système de Comportement/🗺️Graphe de comportement/🧑‍💻Éditeur/behavior_editor_plugin.hpp"
#include "🤾Système de Comportement/🗺️Graphe de comportement/🧑‍💻Éditeur/behavior_editor_window.hpp"

#include <godot_cpp/classes/editor_plugin_registration.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_entity_algorithm(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        // Éditeur : bouton de l'inspecteur, fenêtre TechTree / MindMap, débogage du jeu en cours (classes internes).
        GDREGISTER_INTERNAL_CLASS(BehaviorInspectorPlugin);
        GDREGISTER_INTERNAL_CLASS(BehaviorDebuggerPlugin);
        GDREGISTER_INTERNAL_CLASS(BehaviorEditorWindow);
        GDREGISTER_INTERNAL_CLASS(DialogueEditorWindow);
        GDREGISTER_INTERNAL_CLASS(BehaviorEditorPlugin);
        // L'éditeur instancie et libère le plugin ; godot-cpp le retire au déchargement du niveau EDITOR.
        EditorPlugins::add_by_type<BehaviorEditorPlugin>();
        return;
    }
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }

    // Ordre : une classe parente avant ses classes filles.
    GDREGISTER_CLASS(Blackboard); // @icon res://🖼️Editor Icons/🧍🧠/Blackboard Icon.svg
    GDREGISTER_INTERNAL_CLASS(BehaviorDebugger);

    // 🤖 Machine à états hiérarchique
    GDREGISTER_CLASS(StateTransition); // @icon res://🖼️Editor Icons/🧍🧠/StateTransition Icon.svg
    GDREGISTER_CLASS(State);           // @icon res://🖼️Editor Icons/🧍🧠/State Icon.png
    GDREGISTER_CLASS(StateMachine);    // @icon res://🖼️Editor Icons/🧍🧠/StateMachine Icon.png

    // 🌿 Arbre de comportement
    GDREGISTER_VIRTUAL_CLASS(BTNode);                // @icon res://🖼️Editor Icons/🧍🧠/BTAction Icon.svg
    GDREGISTER_VIRTUAL_CLASS(BTComposite);           // @icon res://🖼️Editor Icons/🧍🧠/BTSequence Icon.svg
    GDREGISTER_ABSTRACT_CLASS(BTOrderedComposite);   // @icon res://🖼️Editor Icons/🧍🧠/BTSequence Icon.svg
    GDREGISTER_CLASS(BTSequence);                    // @icon res://🖼️Editor Icons/🧍🧠/BTSequence Icon.svg
    GDREGISTER_CLASS(BTSelector);                    // @icon res://🖼️Editor Icons/🧍🧠/BTSelector Icon.svg
    GDREGISTER_CLASS(BTParallel);                    // @icon res://🖼️Editor Icons/🧍🧠/BTParallel Icon.svg
    GDREGISTER_VIRTUAL_CLASS(BTDecorator);           // @icon res://🖼️Editor Icons/🧍🧠/BTDecorator Icon.svg
    GDREGISTER_CLASS(BTInverter);                    // @icon res://🖼️Editor Icons/🧍🧠/BTDecorator Icon.svg
    GDREGISTER_CLASS(BTForceResult);                 // @icon res://🖼️Editor Icons/🧍🧠/BTDecorator Icon.svg
    GDREGISTER_CLASS(BTRepeater);                    // @icon res://🖼️Editor Icons/🧍🧠/BTDecorator Icon.svg
    GDREGISTER_CLASS(BTCooldown);                    // @icon res://🖼️Editor Icons/🧍🧠/BTDecorator Icon.svg
    GDREGISTER_CLASS(BTTimeLimit);                   // @icon res://🖼️Editor Icons/🧍🧠/BTDecorator Icon.svg
    GDREGISTER_CLASS(BTAction);                      // @icon res://🖼️Editor Icons/🧍🧠/BTAction Icon.svg
    GDREGISTER_CLASS(BTCondition);                   // @icon res://🖼️Editor Icons/🧍🧠/BTCondition Icon.svg
    GDREGISTER_CLASS(BTWait);                        // @icon res://🖼️Editor Icons/🧍🧠/BTAction Icon.svg
    GDREGISTER_CLASS(BTSetValue);                    // @icon res://🖼️Editor Icons/🧍🧠/BTAction Icon.svg
    GDREGISTER_CLASS(BTCallMethod);                  // @icon res://🖼️Editor Icons/🧍🧠/BTAction Icon.svg
    GDREGISTER_CLASS(BTSendEvent);                   // @icon res://🖼️Editor Icons/🧍🧠/BTAction Icon.svg
    GDREGISTER_CLASS(BehaviorTree);                  // @icon res://🖼️Editor Icons/🧍🧠/Automate Icon.png

    // 🗺️ Vue en graphe : débogage en jeu, et cœur de la fenêtre d'édition
    GDREGISTER_CLASS(BehaviorGraphView); // @icon res://🖼️Editor Icons/🧰🧑‍💻/TechTree icon.png

    // 🌟 Système de capacités
    GDREGISTER_CLASS(AttributeDefinition); // @icon res://🖼️Editor Icons/🧑‍🔧🏗️/AbilityEffect Icon.png
    GDREGISTER_CLASS(AttributeModifier);   // @icon res://🖼️Editor Icons/🧑‍🔧🏗️/AbilityEffect Icon.png
    GDREGISTER_CLASS(GameplayEffect);      // @icon res://🖼️Editor Icons/🧑‍🔧🏗️/AbilityEffect Icon.png
    GDREGISTER_CLASS(Ability);             // @icon res://🖼️Editor Icons/🧑‍🔧🏗️/Ability Icon.png
    GDREGISTER_CLASS(AbilitySystem);       // @icon res://🖼️Editor Icons/🧑‍🔧🏗️/AbilitySystem Icon.png

    // 💬 Système de dialogue
    GDREGISTER_VIRTUAL_CLASS(DialogueNode); // @icon res://🖼️Editor Icons/🧍🧠/Dialogue Icon.png
    GDREGISTER_CLASS(DialogueLine);         // @icon res://🖼️Editor Icons/🧍🧠/Dialogue Icon.png
    GDREGISTER_CLASS(DialogueOption);       // @icon res://🖼️Editor Icons/🧍🧠/Dialogue Icon.png
    GDREGISTER_CLASS(DialogueChoice);       // @icon res://🖼️Editor Icons/🧍🧠/Dialogue Icon.png
    GDREGISTER_CLASS(DialogueBranch);       // @icon res://🖼️Editor Icons/🧍🧠/Dialogue Icon.png
    GDREGISTER_CLASS(DialogueAction);       // @icon res://🖼️Editor Icons/🧍🧠/Dialogue Icon.png
    GDREGISTER_CLASS(DialogueEnd);          // @icon res://🖼️Editor Icons/🧍🧠/EmptyDialogue Icon.png
    GDREGISTER_CLASS(DialogueResource);     // @icon res://🖼️Editor Icons/🧍🧠/Dialogue Icon.png
    GDREGISTER_CLASS(DialogueSystem);       // @icon res://🖼️Editor Icons/🧍🧠/Dialogue System Icon.png
    GDREGISTER_CLASS(DialogueBox);          // @icon res://🖼️Editor Icons/🧍🧠/EmptyDialogue Icon.png

    // 🧑 Personnages (joueur, automate, PNG)
    GDREGISTER_CLASS(Character2D);                // @icon res://🖼️Editor Icons/🧍🧠/Character Icon.png
    GDREGISTER_CLASS(Character3D);                // @icon res://🖼️Editor Icons/🧍🧠/Character Icon.png
    GDREGISTER_CLASS(HealthComponent);            // @icon res://🖼️Editor Icons/🧍🧠/Character Icon.png
    GDREGISTER_ABSTRACT_CLASS(CharacterRegister); // @icon res://🖼️Editor Icons/🧍🧠/CharacterRegister Icon.png
    GDREGISTER_CLASS(Interactable);               // @icon res://🖼️Editor Icons/🧍🧠/Dialogue Icon.png
    GDREGISTER_CLASS(PlayerController);           // @icon res://🖼️Editor Icons/🧍🧠/Player Icon.png
    GDREGISTER_CLASS(AutomateController);         // @icon res://🖼️Editor Icons/🧍🧠/Automate Icon.png
    GDREGISTER_CLASS(NPCController);              // @icon res://🖼️Editor Icons/🧍🧠/Character Icon.png
    GDREGISTER_CLASS(BTFindTarget);               // @icon res://🖼️Editor Icons/🧍🧠/BTCondition Icon.svg
    GDREGISTER_CLASS(BTMoveTo);                   // @icon res://🖼️Editor Icons/🧍🧠/BTAction Icon.svg
    GDREGISTER_CLASS(BTUseAbility);               // @icon res://🖼️Editor Icons/🧍🧠/BTAction Icon.svg

    BehaviorDebugger::create();

    godot::print_line_rich(U"[color=dark_green]🧍Module Entités & Algorithme (comportement, personnages, dialogue, capacités) chargé avec succès ![/color]");
}

static void uninitialize_entity_algorithm(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    BehaviorDebugger::destroy();
    godot::print_line_rich(U"[color=dark_red]🧍Module Entités & Algorithme déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT entity_algorithm_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_entity_algorithm);
        init_obj.register_terminator(uninitialize_entity_algorithm);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
