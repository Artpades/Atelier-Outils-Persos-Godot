#include "Actions/Axiales/input_axis_actions.hpp"
#include "Actions/Spéciales/input_special_actions.hpp"
#include "Actions/input_action.hpp"
#include "Actions/input_button_action.hpp"

#include "Gestionnaires/input_context.hpp"
#include "Gestionnaires/input_debugger.hpp"
#include "Gestionnaires/input_rebinder.hpp"
#include "Gestionnaires/player_input.hpp"
#include "Gestionnaires/player_input_ui.hpp"

#include "🧑‍💻Éditeur/input_editor_plugin.hpp"
#include "🧑‍💻Éditeur/input_editor_window.hpp"

#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_input_devices(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        // Éditeur : bouton de l'inspecteur, fenêtre « Éditeur d'entrées », débogage du jeu en cours (classes internes).
        GDREGISTER_INTERNAL_CLASS(InputActionMeter);
        GDREGISTER_INTERNAL_CLASS(InputEditorWindow);
        GDREGISTER_INTERNAL_CLASS(InputInspectorPlugin);
        GDREGISTER_INTERNAL_CLASS(InputDebuggerPlugin);
        GDREGISTER_INTERNAL_CLASS(InputEditorPlugin);
        EditorPlugins::add_by_type<InputEditorPlugin>();
        return;
    }
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }

    // 🕹️ Actions
    GDREGISTER_CLASS(InputBinding);                // @icon res://🖼️Editor Icons/🎮🔌/SignalEmitter icon.png
    GDREGISTER_VIRTUAL_CLASS(InputAction);         // @icon res://🖼️Editor Icons/🎮🔌/InputAction Icon.png
    GDREGISTER_CLASS(InputButtonAction);           // @icon res://🖼️Editor Icons/🎮🔌/InputButtonAction Icon.png
    GDREGISTER_CLASS(InputAxisAction);             // @icon res://🖼️Editor Icons/🎮🔌/InputAxisAction Icon.png
    GDREGISTER_CLASS(InputCircleAction);           // @icon res://🖼️Editor Icons/🎮🔌/InputVector2Action Icon.png
    GDREGISTER_CLASS(InputSphereAction);           // @icon res://🖼️Editor Icons/🎮🔌/InputVector3Action Icon-export.png
    GDREGISTER_CLASS(InputMouseAction);            // @icon res://🖼️Editor Icons/🎮🔌/InputMouseAction Icon.png
    GDREGISTER_CLASS(InputGyroAction);             // @icon res://🖼️Editor Icons/🎮🔌/JoyPadMotion Icon.png

    // 🗂️ Gestionnaires
    GDREGISTER_CLASS(InputContext);                // @icon res://🖼️Editor Icons/🎮🔌/InputContexte Icon.png
    GDREGISTER_CLASS(PlayerInput);                 // @icon res://🖼️Editor Icons/🎮🔌/InputManager Icon.png
    GDREGISTER_CLASS(InputRebinder);               // @icon res://🖼️Editor Icons/🎮🔌/InputMapBinding Icon.png
    GDREGISTER_CLASS(PlayerInputUI);               // @icon res://🖼️Editor Icons/🎮🔌/ButtonAssigned Icon.png
    GDREGISTER_INTERNAL_CLASS(InputDebugger);

    InputDebugger::create();

    godot::print_line_rich(U"[color=dark_green]🕹️Module Entrées de périphérique chargé avec succès ![/color]");
}

static void uninitialize_input_devices(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    InputDebugger::destroy();
    godot::print_line_rich(U"[color=dark_red]🕹️Module Entrées de périphérique déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT input_devices_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_input_devices);
        init_obj.register_terminator(uninitialize_input_devices);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
