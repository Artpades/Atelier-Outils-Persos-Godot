#ifndef INPUT_CIRCLE_ACTION_HPP
#define INPUT_CIRCLE_ACTION_HPP

#include "input_action.hpp"

#include <godot_cpp/classes/global_constants.hpp>

namespace godot
{
/// ⭕ Action 2D, vecteur de longueur ≤ 1 : stick de manette, ou 4 touches / 4 boutons (croix directionnelle).
/// Convention écran : +X à droite, +Y en bas. Le stick est prioritaire dès qu'il sort de la zone morte.
class InputCircleAction : public InputAction
{
    GDCLASS(InputCircleAction, InputAction)

  public:
    enum KeyAxisPreset
    {
        KEY_PRESET_WASD,
        KEY_PRESET_ZQSD,
        KEY_PRESET_ARROW_KEYS,
        KEY_PRESET_CUSTOM,
    };

    enum JoyAxisPreset
    {
        JOY_PRESET_LEFT_STICK,
        JOY_PRESET_RIGHT_STICK,
        JOY_PRESET_CUSTOM,
    };

    enum DpadPreset
    {
        DPAD_PRESET_TRADITIONAL,
        DPAD_PRESET_ABXY,
        DPAD_PRESET_CUSTOM,
    };

    void _process(double p_delta) override;

    PackedStringArray get_binding_properties() const override;

    /// Vecteur courant sur p_device (device_id si négatif). Vector2.ZERO si l'action est désactivée.
    Vector2 get_vector(int p_device = -1) const;

    void set_key_axis_preset(KeyAxisPreset p_preset);
    KeyAxisPreset get_key_axis_preset() const { return key_axis_preset; }

    void set_joy_axis_preset(JoyAxisPreset p_preset);
    JoyAxisPreset get_joy_axis_preset() const { return joy_axis_preset; }

    void set_dpad_preset(DpadPreset p_preset);
    DpadPreset get_dpad_preset() const { return dpad_preset; }

    void set_up_key(Key p_key) { up_key = p_key; }
    Key get_up_key() const { return up_key; }
    void set_down_key(Key p_key) { down_key = p_key; }
    Key get_down_key() const { return down_key; }
    void set_left_key(Key p_key) { left_key = p_key; }
    Key get_left_key() const { return left_key; }
    void set_right_key(Key p_key) { right_key = p_key; }
    Key get_right_key() const { return right_key; }

    void set_joy_axis_x(JoyAxis p_axis) { joy_axis_x = p_axis; }
    JoyAxis get_joy_axis_x() const { return joy_axis_x; }
    void set_joy_axis_y(JoyAxis p_axis) { joy_axis_y = p_axis; }
    JoyAxis get_joy_axis_y() const { return joy_axis_y; }

    void set_dpad_up(JoyButton p_button) { dpad_up = p_button; }
    JoyButton get_dpad_up() const { return dpad_up; }
    void set_dpad_down(JoyButton p_button) { dpad_down = p_button; }
    JoyButton get_dpad_down() const { return dpad_down; }
    void set_dpad_left(JoyButton p_button) { dpad_left = p_button; }
    JoyButton get_dpad_left() const { return dpad_left; }
    void set_dpad_right(JoyButton p_button) { dpad_right = p_button; }
    JoyButton get_dpad_right() const { return dpad_right; }

  protected:
    static void _bind_methods();

  private:
    void draw_debug(const Vector2 &p_value);

    KeyAxisPreset key_axis_preset = KEY_PRESET_CUSTOM;
    JoyAxisPreset joy_axis_preset = JOY_PRESET_CUSTOM;
    DpadPreset dpad_preset = DPAD_PRESET_CUSTOM;

    Key up_key = KEY_NONE;
    Key down_key = KEY_NONE;
    Key left_key = KEY_NONE;
    Key right_key = KEY_NONE;

    JoyAxis joy_axis_x = JOY_AXIS_INVALID;
    JoyAxis joy_axis_y = JOY_AXIS_INVALID;

    JoyButton dpad_up = JOY_BUTTON_INVALID;
    JoyButton dpad_down = JOY_BUTTON_INVALID;
    JoyButton dpad_left = JOY_BUTTON_INVALID;
    JoyButton dpad_right = JOY_BUTTON_INVALID;

    Vector2 last_emitted;
    Vector2 previous_value;
    Vector2 last_emitted_velocity;
    float last_emitted_orientation = 0.0f;
};
} // namespace godot

VARIANT_ENUM_CAST(InputCircleAction::KeyAxisPreset);
VARIANT_ENUM_CAST(InputCircleAction::JoyAxisPreset);
VARIANT_ENUM_CAST(InputCircleAction::DpadPreset);

#endif // INPUT_CIRCLE_ACTION_HPP
