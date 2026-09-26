#ifndef PLAYER_INPUT_HPP
#define PLAYER_INPUT_HPP

#include "input_action.hpp"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// 🕹️ Un joueur : fixe le périphérique (device_number) et le filtre (detector_type) de toutes les InputAction
/// descendantes, peut basculer automatiquement clavier/souris ↔ manette, et pilote la vibration.
class PlayerInput : public Node
{
    GDCLASS(PlayerInput, Node)

  public:
    void _ready() override;
    void _input(const Ref<InputEvent> &p_event) override;

    /// Toutes les InputAction descendantes, dans l'ordre de l'arbre.
    TypedArray<InputAction> get_all_actions() const;

    String get_controller_name() const;
    String get_joy_guid() const;
    Dictionary get_joy_info() const;

    void start_vibration(float p_weak_magnitude = 1.0f, float p_strong_magnitude = 1.0f, float p_duration = 0.0f);
    void stop_vibration();

    void set_device_number(int p_device);
    int get_device_number() const { return device_number; }

    void set_detector_type(InputAction::DeviceDetectorType p_type);
    InputAction::DeviceDetectorType get_detector_type() const { return detector_type; }

    void set_switch_detector_on_device_input(bool p_switch) { switch_detector_on_device_input = p_switch; }
    bool get_switch_detector_on_device_input() const { return switch_detector_on_device_input; }

    void set_joy_input_threshold(float p_threshold) { joy_input_threshold = p_threshold; }
    float get_joy_input_threshold() const { return joy_input_threshold; }

    void set_debug_log(bool p_enabled) { debug_log = p_enabled; }
    bool get_debug_log() const { return debug_log; }

  protected:
    static void _bind_methods();

  private:
    void propagate_to_actions();
    static void collect_actions(const Node *p_node, TypedArray<InputAction> &r_actions);

    int device_number = 0;
    InputAction::DeviceDetectorType detector_type = InputAction::DETECTOR_ALL_DEVICES;
    bool switch_detector_on_device_input = false;
    float joy_input_threshold = 0.2f;
    bool debug_log = false;
};
} // namespace godot

#endif // PLAYER_INPUT_HPP
