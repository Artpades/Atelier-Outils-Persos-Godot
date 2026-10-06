#ifndef INPUT_AXIS_ACTIONS_HPP
#define INPUT_AXIS_ACTIONS_HPP

#include "../input_action.hpp"

namespace godot
{
/// ↔️ Action axe (1D, de -1 à 1) : axe de manette (zone morte remise à l'échelle), et/ou deux touches et deux
/// boutons de manette (+ / −). L'axe de manette, s'il est hors de sa zone morte, a priorité.
/// Évènements : value_changed(value), velocity_changed(velocity) (variation par seconde).
class InputAxisAction : public InputAction
{
    GDCLASS(InputAxisAction, InputAction)

  public:
    void set_positive_key(int p_key) { positive_key = p_key; }
    int get_positive_key() const { return positive_key; }
    void set_negative_key(int p_key) { negative_key = p_key; }
    int get_negative_key() const { return negative_key; }
    void set_joy_axis(int p_axis) { joy_axis = p_axis; }
    int get_joy_axis() const { return joy_axis; }
    void set_positive_joy_button(int p_button) { positive_joy_button = p_button; }
    int get_positive_joy_button() const { return positive_joy_button; }
    void set_negative_joy_button(int p_button) { negative_joy_button = p_button; }
    int get_negative_joy_button() const { return negative_joy_button; }
    void set_invert(bool p_invert) { invert = p_invert; }
    bool get_invert() const { return invert; }

    double get_axis() const { return value; }
    double get_velocity() const { return velocity; }

    Variant get_value() const override { return value; }
    bool is_active() const override { return value != 0.0; }
    PackedStringArray get_device_properties() const override;
    TypedArray<Dictionary> get_rebind_slots() const override;
    bool apply_rebind(const StringName &p_slot, const Ref<InputEvent> &p_event, double p_joy_threshold) override;

  protected:
    static void _bind_methods();
    void _update(double p_delta) override;
    void _release() override;
    void _apply_simulation(const Variant &p_value, double p_delta) override;

  private:
    int positive_key = 0;
    int negative_key = 0;
    int joy_axis = -1;
    int positive_joy_button = -1;
    int negative_joy_button = -1;
    bool invert = false;

    double value = 0.0;
    double velocity = 0.0;
    double last_emitted = 0.0;
    double last_emitted_velocity = 0.0;

    void _process(double p_value, double p_delta);
};

/// 🕹️ Action cercle (Vector2, longueur ≤ 1) : stick de manette (zone morte radiale), 4 touches, 4 boutons
/// (croix directionnelle...). Le stick, hors de sa zone morte, a priorité. Les préréglages (key_preset,
/// joy_preset, dpad_preset) remplissent les entrées ; ils indiquent « Personnalisé » dès qu'elles diffèrent.
/// Avec physical_keys (défaut), le préréglage WASD correspond aux positions ZQSD d'un clavier AZERTY.
/// y vers le bas, comme Input.get_vector (invert_y pour l'inverser).
/// Évènements : value_changed(value), velocity_changed(velocity), orientation_changed(angle) (radians).
class InputCircleAction : public InputAction
{
    GDCLASS(InputCircleAction, InputAction)

  public:
    enum KeyPreset
    {
        KEYS_CUSTOM,
        KEYS_WASD,
        KEYS_ZQSD,
        KEYS_ARROWS,
    };
    enum JoyPreset
    {
        JOY_CUSTOM,
        JOY_LEFT_STICK,
        JOY_RIGHT_STICK,
    };
    enum DpadPreset
    {
        DPAD_CUSTOM,
        DPAD_CROSS,
        DPAD_FACE_BUTTONS,
    };

    InputCircleAction();

    void set_key_preset(KeyPreset p_preset);
    KeyPreset get_key_preset() const;
    void set_joy_preset(JoyPreset p_preset);
    JoyPreset get_joy_preset() const;
    void set_dpad_preset(DpadPreset p_preset);
    DpadPreset get_dpad_preset() const;

    void set_up_key(int p_key) { up_key = p_key; }
    int get_up_key() const { return up_key; }
    void set_down_key(int p_key) { down_key = p_key; }
    int get_down_key() const { return down_key; }
    void set_left_key(int p_key) { left_key = p_key; }
    int get_left_key() const { return left_key; }
    void set_right_key(int p_key) { right_key = p_key; }
    int get_right_key() const { return right_key; }
    void set_joy_axis_x(int p_axis) { joy_axis_x = p_axis; }
    int get_joy_axis_x() const { return joy_axis_x; }
    void set_joy_axis_y(int p_axis) { joy_axis_y = p_axis; }
    int get_joy_axis_y() const { return joy_axis_y; }
    void set_dpad_up(int p_button) { dpad_up = p_button; }
    int get_dpad_up() const { return dpad_up; }
    void set_dpad_down(int p_button) { dpad_down = p_button; }
    int get_dpad_down() const { return dpad_down; }
    void set_dpad_left(int p_button) { dpad_left = p_button; }
    int get_dpad_left() const { return dpad_left; }
    void set_dpad_right(int p_button) { dpad_right = p_button; }
    int get_dpad_right() const { return dpad_right; }
    void set_invert_y(bool p_invert) { invert_y = p_invert; }
    bool get_invert_y() const { return invert_y; }

    Vector2 get_vector() const { return value; }
    Vector2 get_velocity() const { return velocity; }
    double get_orientation() const { return orientation; }

    Variant get_value() const override { return value; }
    bool is_active() const override { return value != Vector2(); }
    PackedStringArray get_device_properties() const override;
    TypedArray<Dictionary> get_rebind_slots() const override;
    bool apply_rebind(const StringName &p_slot, const Ref<InputEvent> &p_event, double p_joy_threshold) override;

  protected:
    static void _bind_methods();
    void _update(double p_delta) override;
    void _release() override;
    void _apply_simulation(const Variant &p_value, double p_delta) override;

  private:
    int up_key = 0;
    int down_key = 0;
    int left_key = 0;
    int right_key = 0;
    int joy_axis_x = -1;
    int joy_axis_y = -1;
    int dpad_up = -1;
    int dpad_down = -1;
    int dpad_left = -1;
    int dpad_right = -1;
    bool invert_y = false;

    Vector2 value;
    Vector2 velocity;
    double orientation = 0.0;
    Vector2 last_emitted;
    Vector2 last_emitted_velocity;
    double last_emitted_orientation = 0.0;

    void _process(const Vector2 &p_value, double p_delta);
};

/// 🌐 Action sphère (Vector3, longueur ≤ 1) : 3 axes de manette, 6 touches, 6 boutons. Convention Godot :
/// avant = -Z, haut = +Y, droite = +X. Le joystick, hors de sa zone morte, a priorité.
/// Évènements : value_changed(value), velocity_changed(velocity), orientation_changed(orientation)
/// (tangage x, lacet y, en radians).
class InputSphereAction : public InputAction
{
    GDCLASS(InputSphereAction, InputAction)

  public:
    InputSphereAction();

#define SPHERE_INPUT(m_name, m_default) \
  public:                               \
    void set_##m_name(int p_value) { m_name = p_value; } \
    int get_##m_name() const { return m_name; }          \
                                                          \
  private:                                                \
    int m_name = m_default;

    SPHERE_INPUT(left_key, 0)
    SPHERE_INPUT(right_key, 0)
    SPHERE_INPUT(up_key, 0)
    SPHERE_INPUT(down_key, 0)
    SPHERE_INPUT(forward_key, 0)
    SPHERE_INPUT(backward_key, 0)
    SPHERE_INPUT(joy_axis_x, -1)
    SPHERE_INPUT(joy_axis_y, -1)
    SPHERE_INPUT(joy_axis_z, -1)
    SPHERE_INPUT(dpad_left, -1)
    SPHERE_INPUT(dpad_right, -1)
    SPHERE_INPUT(dpad_up, -1)
    SPHERE_INPUT(dpad_down, -1)
    SPHERE_INPUT(dpad_forward, -1)
    SPHERE_INPUT(dpad_backward, -1)
#undef SPHERE_INPUT

  public:
    void set_invert_y(bool p_invert) { invert_y = p_invert; }
    bool get_invert_y() const { return invert_y; }
    Vector3 get_vector() const { return value; }
    Vector3 get_velocity() const { return velocity; }
    Vector3 get_orientation() const { return orientation; }

    Variant get_value() const override { return value; }
    bool is_active() const override { return value != Vector3(); }
    PackedStringArray get_device_properties() const override;
    TypedArray<Dictionary> get_rebind_slots() const override;
    bool apply_rebind(const StringName &p_slot, const Ref<InputEvent> &p_event, double p_joy_threshold) override;

  protected:
    static void _bind_methods();
    void _update(double p_delta) override;
    void _release() override;
    void _apply_simulation(const Variant &p_value, double p_delta) override;

  private:
    bool invert_y = false;
    Vector3 value;
    Vector3 velocity;
    Vector3 orientation;
    Vector3 last_emitted;
    Vector3 last_emitted_velocity;
    Vector3 last_emitted_orientation;

    void _process(const Vector3 &p_value, double p_delta);
};
} // namespace godot

VARIANT_ENUM_CAST(InputCircleAction::KeyPreset);
VARIANT_ENUM_CAST(InputCircleAction::JoyPreset);
VARIANT_ENUM_CAST(InputCircleAction::DpadPreset);

#endif // INPUT_AXIS_ACTIONS_HPP
