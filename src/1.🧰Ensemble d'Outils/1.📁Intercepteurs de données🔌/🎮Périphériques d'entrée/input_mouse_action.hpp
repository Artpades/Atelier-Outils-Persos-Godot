#ifndef INPUT_MOUSE_ACTION_HPP
#define INPUT_MOUSE_ACTION_HPP

#include "input_action.hpp"

#include <godot_cpp/classes/input_event.hpp>

namespace godot
{
/// 🖱️ Position, mouvement relatif et pression (stylet) de la souris dans le viewport de l'action.
class InputMouseAction : public InputAction
{
    GDCLASS(InputMouseAction, InputAction)

  public:
    void _process(double p_delta) override;
    void _input(const Ref<InputEvent> &p_event) override;

    /// Position de la souris dans le viewport (normalisée si normalize_position). Vector2.ZERO si désactivée.
    Vector2 get_mouse_position() const;
    /// Mouvement accumulé depuis la dernière frame, multiplié par motion_sensitivity.
    Vector2 get_accumulated_motion() const;
    /// Pression du stylet (0 à 1).
    float get_pressure() const;

    void set_motion_sensitivity(float p_sensitivity) { motion_sensitivity = p_sensitivity; }
    float get_motion_sensitivity() const { return motion_sensitivity; }

    void set_position_threshold(float p_threshold) { position_threshold = p_threshold; }
    float get_position_threshold() const { return position_threshold; }

    void set_normalize_position(bool p_normalize) { normalize_position = p_normalize; }
    bool get_normalize_position() const { return normalize_position; }

  protected:
    static void _bind_methods();

  private:
    void draw_debug(const Vector2 &p_motion);

    float motion_sensitivity = 1.0f;
    float position_threshold = 0.5f;
    bool normalize_position = false;

    Vector2 accumulated_motion;
    Vector2 last_emitted_position;
    Vector2 last_emitted_motion;
    float current_pressure = 0.0f;
    float last_emitted_pressure = 0.0f;
};
} // namespace godot

#endif // INPUT_MOUSE_ACTION_HPP
