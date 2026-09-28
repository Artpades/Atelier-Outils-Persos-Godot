#include "commun/module_entry.hpp"

#include "input_action.hpp"
#include "input_axis_action.hpp"
#include "input_button_action.hpp"
#include "input_circle_action.hpp"
#include "input_context.hpp"
#include "input_joy_gyro_action.hpp"
#include "input_mouse_action.hpp"
#include "input_touch_action.hpp"
#include "input_rebinder.hpp"
#include "input_sphere_action.hpp"
#include "player_input.hpp"
#include "player_input_ui.hpp"

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_ABSTRACT_CLASS(InputAction);
    GDREGISTER_CLASS(InputButtonAction);
    GDREGISTER_CLASS(InputAxisAction);
    GDREGISTER_CLASS(InputCircleAction);
    GDREGISTER_CLASS(InputSphereAction);
    GDREGISTER_CLASS(InputJoyGyroAction);
    GDREGISTER_CLASS(InputMouseAction);
    GDREGISTER_CLASS(InputTouchAction);
    GDREGISTER_CLASS(InputContext);
    GDREGISTER_CLASS(PlayerInput);
    GDREGISTER_CLASS(InputRebinder);
    GDREGISTER_CLASS(PlayerInputUI);
}

void uninitialize_module(ModuleInitializationLevel p_level) {}

GAMEPLAYTOOLKIT_MODULE(input, U"🎮 Périphériques d'entrée")
