#ifndef INPUT_BUTTON_ACTION_HPP
#define INPUT_BUTTON_ACTION_HPP

#include "input_action.hpp"

#include <godot_cpp/classes/global_constants.hpp>

namespace godot
{
/// 🔘 Action tout-ou-rien : touche, bouton de souris, bouton de manette ou axe de manette (gâchette) au-delà
/// de la zone morte. Émet pressed / held / multi_pressed / released.
class InputButtonAction : public InputAction
{
    GDCLASS(InputButtonAction, InputAction)

  public:
    void _process(double p_delta) override;

    PackedStringArray get_binding_properties() const override;

    /// Vrai si l'une des entrées affectées est enfoncée sur p_device (device_id si négatif).
    bool is_button_pressed(int p_device = -1) const;

    void set_key(Key p_key) { key = p_key; }
    Key get_key() const { return key; }

    void set_mouse_button(MouseButton p_button) { mouse_button = p_button; }
    MouseButton get_mouse_button() const { return mouse_button; }

    void set_gamepad_button(JoyButton p_button) { gamepad_button = p_button; }
    JoyButton get_gamepad_button() const { return gamepad_button; }

    void set_gamepad_axis(JoyAxis p_axis) { gamepad_axis = p_axis; }
    JoyAxis get_gamepad_axis() const { return gamepad_axis; }

    void set_held_delay(float p_delay) { held_delay = p_delay; }
    float get_held_delay() const { return held_delay; }

    void set_multi_press_delay(float p_delay) { multi_press_delay = p_delay; }
    float get_multi_press_delay() const { return multi_press_delay; }

    void set_multi_press_count(int p_count) { multi_press_count = p_count; }
    int get_multi_press_count() const { return multi_press_count; }

  protected:
    static void _bind_methods();

  private:
    void release();
    void draw_debug();

    Key key = KEY_NONE;
    MouseButton mouse_button = MOUSE_BUTTON_NONE;
    JoyButton gamepad_button = JOY_BUTTON_INVALID;
    JoyAxis gamepad_axis = JOY_AXIS_INVALID;

    /// Délai d'appui avant l'émission de held (0 : dès la première frame).
    float held_delay = 0.0f;
    /// Fenêtre pendant laquelle les pressions successives sont comptées pour multi_pressed.
    float multi_press_delay = 0.3f;
    /// Nombre de pressions requises dans la fenêtre pour émettre multi_pressed.
    int multi_press_count = 2;

    bool was_pressed = false;
    float held_timer = 0.0f;
    bool held_emitted = false;
    int press_count = 0;
    float multi_press_timer = 0.0f;
};
} // namespace godot

#endif // INPUT_BUTTON_ACTION_HPP
