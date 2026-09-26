#ifndef INPUT_AXIS_ACTION_HPP
#define INPUT_AXIS_ACTION_HPP

#include "input_action.hpp"

#include <godot_cpp/classes/global_constants.hpp>

namespace godot
{
/// ↔️ Action à un axe, valeur dans [-1, 1] : axe de manette, ou paire de touches / boutons (positif, négatif).
/// L'axe de manette est prioritaire dès qu'il sort de la zone morte.
class InputAxisAction : public InputAction
{
    GDCLASS(InputAxisAction, InputAction)

  public:
    void _process(double p_delta) override;

    PackedStringArray get_binding_properties() const override;

    /// Valeur de l'axe sur p_device (device_id si négatif), inversée si invert_axis. 0 si l'action est désactivée.
    float get_axis_value(int p_device = -1) const;

    void set_positive_key(Key p_key) { positive_key = p_key; }
    Key get_positive_key() const { return positive_key; }

    void set_negative_key(Key p_key) { negative_key = p_key; }
    Key get_negative_key() const { return negative_key; }

    void set_gamepad_axis(JoyAxis p_axis) { gamepad_axis = p_axis; }
    JoyAxis get_gamepad_axis() const { return gamepad_axis; }

    void set_positive_gamepad_button(JoyButton p_button) { positive_gamepad_button = p_button; }
    JoyButton get_positive_gamepad_button() const { return positive_gamepad_button; }

    void set_negative_gamepad_button(JoyButton p_button) { negative_gamepad_button = p_button; }
    JoyButton get_negative_gamepad_button() const { return negative_gamepad_button; }

    void set_invert_axis(bool p_invert) { invert_axis = p_invert; }
    bool get_invert_axis() const { return invert_axis; }

  protected:
    static void _bind_methods();

  private:
    void draw_debug(float p_value);

    Key positive_key = KEY_NONE;
    Key negative_key = KEY_NONE;
    JoyAxis gamepad_axis = JOY_AXIS_INVALID;
    JoyButton positive_gamepad_button = JOY_BUTTON_INVALID;
    JoyButton negative_gamepad_button = JOY_BUTTON_INVALID;
    bool invert_axis = false;

    float last_emitted_value = 0.0f;
    float previous_value = 0.0f;
    float last_emitted_velocity = 0.0f;
};
} // namespace godot

#endif // INPUT_AXIS_ACTION_HPP
