#ifndef INPUT_REBINDER_HPP
#define INPUT_REBINDER_HPP

#include "input_action.hpp"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// 🔁 Réaffectation à chaud : écoute la prochaine entrée et l'écrit dans l'emplacement (slot) demandé d'une
/// InputAction. Annulable par cancel_key ou après listening_timeout secondes.
class InputRebinder : public Node
{
    GDCLASS(InputRebinder, Node)

  public:
    enum TargetSlot
    {
        // InputButtonAction
        SLOT_BUTTON_KBM,
        SLOT_BUTTON_GAMEPAD,

        // InputAxisAction
        SLOT_AXIS_POSITIVE_KEY,
        SLOT_AXIS_NEGATIVE_KEY,
        SLOT_AXIS_GAMEPAD_AXIS,
        SLOT_AXIS_POSITIVE_GAMEPAD_BUTTON,
        SLOT_AXIS_NEGATIVE_GAMEPAD_BUTTON,

        // InputCircleAction / InputSphereAction : clavier
        SLOT_UP_KEY,
        SLOT_DOWN_KEY,
        SLOT_LEFT_KEY,
        SLOT_RIGHT_KEY,
        SLOT_FORWARD_KEY,
        SLOT_BACKWARD_KEY,

        // InputCircleAction / InputSphereAction : manette
        SLOT_DPAD_UP,
        SLOT_DPAD_DOWN,
        SLOT_DPAD_LEFT,
        SLOT_DPAD_RIGHT,
        SLOT_DPAD_FORWARD,
        SLOT_DPAD_BACKWARD,
        SLOT_JOY_AXIS_X,
        SLOT_JOY_AXIS_Y,
        SLOT_JOY_AXIS_Z,
    };

    void _ready() override;
    void _input(const Ref<InputEvent> &p_event) override;

    /// Commence l'écoute pour p_slot de p_action (remplace une écoute en cours).
    void start_rebind(InputAction *p_action, TargetSlot p_slot);
    /// Arrête l'écoute en cours et émet rebind_cancelled.
    void cancel_rebind();

    bool is_listening() const { return listening; }
    InputAction *get_current_action() const;
    TargetSlot get_current_slot() const { return current_slot; }

    void set_joy_motion_threshold(float p_threshold) { joy_motion_threshold = p_threshold; }
    float get_joy_motion_threshold() const { return joy_motion_threshold; }

    void set_cancel_key(Key p_key) { cancel_key = p_key; }
    Key get_cancel_key() const { return cancel_key; }

    void set_listening_timeout(float p_timeout) { listening_timeout = p_timeout; }
    float get_listening_timeout() const { return listening_timeout; }

    /// Écrit l'entrée p_event dans p_slot de p_action si elle convient. Vrai si l'affectation a été faite.
    /// Utilisable sans écoute (ex : tests, affectation par script).
    bool apply_event(InputAction *p_action, TargetSlot p_slot, const Ref<InputEvent> &p_event) const;

  protected:
    static void _bind_methods();

  private:
    void stop_listening();
    void on_timeout(uint64_t p_session);

    float joy_motion_threshold = 0.5f;
    Key cancel_key = KEY_ESCAPE;
    float listening_timeout = 3.0f;

    bool listening = false;
    ObjectID current_action_id;
    TargetSlot current_slot = SLOT_BUTTON_KBM;
    /// Incrémenté à chaque écoute / arrêt : invalide le minuteur d'une écoute précédente.
    uint64_t session = 0;
};
} // namespace godot

VARIANT_ENUM_CAST(InputRebinder::TargetSlot);

#endif // INPUT_REBINDER_HPP
