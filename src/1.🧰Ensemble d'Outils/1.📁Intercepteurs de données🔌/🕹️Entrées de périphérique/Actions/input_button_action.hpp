#ifndef INPUT_BUTTON_ACTION_HPP
#define INPUT_BUTTON_ACTION_HPP

#include "input_action.hpp"

namespace godot
{
/// 🔘 Action bouton : touche, bouton de souris, bouton de manette, gâchette / axe poussé, ou action de
/// l'InputMap de Godot (input_map_action, facultatif). Enfoncé si l'une de ces entrées l'est.
///
/// Évènements : pressed, released, toggled(pressed) (pratique pour set_running(bool)), held (maintenu
/// held_delay secondes), multi_pressed(count) (multi_press_count appuis espacés de moins de multi_press_delay).
class InputButtonAction : public InputAction
{
    GDCLASS(InputButtonAction, InputAction)

  public:
    enum AxisDirection
    {
        AXIS_BOTH,
        AXIS_POSITIVE,
        AXIS_NEGATIVE,
    };

    void set_key(int p_key) { key = p_key; }
    int get_key() const { return key; }
    void set_mouse_button(int p_button) { mouse_button = p_button; }
    int get_mouse_button() const { return mouse_button; }
    void set_joy_button(int p_button) { joy_button = p_button; }
    int get_joy_button() const { return joy_button; }
    void set_joy_axis(int p_axis) { joy_axis = p_axis; }
    int get_joy_axis() const { return joy_axis; }
    void set_joy_axis_direction(AxisDirection p_direction) { joy_axis_direction = p_direction; }
    AxisDirection get_joy_axis_direction() const { return joy_axis_direction; }
    void set_input_map_action(const StringName &p_action) { input_map_action = p_action; }
    StringName get_input_map_action() const { return input_map_action; }
    void set_held_delay(double p_delay) { held_delay = MAX(0.0, p_delay); }
    double get_held_delay() const { return held_delay; }
    void set_multi_press_delay(double p_delay) { multi_press_delay = MAX(0.0, p_delay); }
    double get_multi_press_delay() const { return multi_press_delay; }
    void set_multi_press_count(int p_count) { multi_press_count = MAX(2, p_count); }
    int get_multi_press_count() const { return multi_press_count; }

    bool is_pressed() const { return pressed; }
    bool is_just_pressed() const { return just_pressed; }
    bool is_just_released() const { return just_released; }
    double get_held_time() const { return pressed ? held_time : 0.0; }

    Variant get_value() const override { return pressed; }
    bool is_active() const override { return pressed; }
    PackedStringArray get_device_properties() const override;
    TypedArray<Dictionary> get_rebind_slots() const override;
    bool apply_rebind(const StringName &p_slot, const Ref<InputEvent> &p_event, double p_joy_threshold) override;
    String get_slot_text(const StringName &p_slot) const override;

  protected:
    static void _bind_methods();
    void _update(double p_delta) override;
    void _release() override;
    void _apply_simulation(const Variant &p_value, double p_delta) override;

  private:
    int key = 0;
    int mouse_button = 0;
    int joy_button = -1;
    int joy_axis = -1;
    AxisDirection joy_axis_direction = AXIS_POSITIVE;
    StringName input_map_action;
    double held_delay = 0.4;
    double multi_press_delay = 0.3;
    int multi_press_count = 2;

    bool pressed = false;
    bool just_pressed = false;
    bool just_released = false;
    double held_time = 0.0;
    bool held_emitted = false;
    int press_count = 0;
    double multi_press_timer = 0.0;

    bool _read() const;
    void _process(bool p_down, double p_delta);
};
} // namespace godot

VARIANT_ENUM_CAST(InputButtonAction::AxisDirection);

#endif // INPUT_BUTTON_ACTION_HPP
