#ifndef INPUT_SPHERE_ACTION_HPP
#define INPUT_SPHERE_ACTION_HPP

#include "input_action.hpp"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot
{
/// 🌐 Action 3D, vecteur de longueur ≤ 1 : trois axes de manette, ou 6 touches / 6 boutons.
/// Convention : +X à droite, +Y en haut, +Z vers l'avant de l'entrée (forward_key).
class InputSphereAction : public InputAction
{
    GDCLASS(InputSphereAction, InputAction)

  public:
    void _process(double p_delta) override;

    PackedStringArray get_binding_properties() const override;

    /// Vecteur courant sur p_device (device_id si négatif). Vector3.ZERO si l'action est désactivée.
    Vector3 get_vector(int p_device = -1) const;

    void set_left_key(Key p_key) { left_key = p_key; }
    Key get_left_key() const { return left_key; }
    void set_right_key(Key p_key) { right_key = p_key; }
    Key get_right_key() const { return right_key; }
    void set_up_key(Key p_key) { up_key = p_key; }
    Key get_up_key() const { return up_key; }
    void set_down_key(Key p_key) { down_key = p_key; }
    Key get_down_key() const { return down_key; }
    void set_forward_key(Key p_key) { forward_key = p_key; }
    Key get_forward_key() const { return forward_key; }
    void set_backward_key(Key p_key) { backward_key = p_key; }
    Key get_backward_key() const { return backward_key; }

    void set_joy_axis_x(JoyAxis p_axis) { joy_axis_x = p_axis; }
    JoyAxis get_joy_axis_x() const { return joy_axis_x; }
    void set_joy_axis_y(JoyAxis p_axis) { joy_axis_y = p_axis; }
    JoyAxis get_joy_axis_y() const { return joy_axis_y; }
    void set_joy_axis_z(JoyAxis p_axis) { joy_axis_z = p_axis; }
    JoyAxis get_joy_axis_z() const { return joy_axis_z; }

    void set_dpad_left(JoyButton p_button) { dpad_left = p_button; }
    JoyButton get_dpad_left() const { return dpad_left; }
    void set_dpad_right(JoyButton p_button) { dpad_right = p_button; }
    JoyButton get_dpad_right() const { return dpad_right; }
    void set_dpad_up(JoyButton p_button) { dpad_up = p_button; }
    JoyButton get_dpad_up() const { return dpad_up; }
    void set_dpad_down(JoyButton p_button) { dpad_down = p_button; }
    JoyButton get_dpad_down() const { return dpad_down; }
    void set_dpad_forward(JoyButton p_button) { dpad_forward = p_button; }
    JoyButton get_dpad_forward() const { return dpad_forward; }
    void set_dpad_backward(JoyButton p_button) { dpad_backward = p_button; }
    JoyButton get_dpad_backward() const { return dpad_backward; }

  protected:
    static void _bind_methods();

  private:
    void draw_debug(const Vector3 &p_value);

    Key left_key = KEY_NONE;
    Key right_key = KEY_NONE;
    Key up_key = KEY_NONE;
    Key down_key = KEY_NONE;
    Key forward_key = KEY_NONE;
    Key backward_key = KEY_NONE;

    JoyAxis joy_axis_x = JOY_AXIS_INVALID;
    JoyAxis joy_axis_y = JOY_AXIS_INVALID;
    JoyAxis joy_axis_z = JOY_AXIS_INVALID;

    JoyButton dpad_left = JOY_BUTTON_INVALID;
    JoyButton dpad_right = JOY_BUTTON_INVALID;
    JoyButton dpad_up = JOY_BUTTON_INVALID;
    JoyButton dpad_down = JOY_BUTTON_INVALID;
    JoyButton dpad_forward = JOY_BUTTON_INVALID;
    JoyButton dpad_backward = JOY_BUTTON_INVALID;

    Vector3 last_emitted;
    Vector3 previous_value;
    Vector3 last_emitted_velocity;
    Vector3 last_emitted_orientation;
};
} // namespace godot

#endif // INPUT_SPHERE_ACTION_HPP
