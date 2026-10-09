#ifndef CAMERA_PANNER_HPP
#define CAMERA_PANNER_HPP

#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot
{
/// 🖐️ Caméra 2D déplaçable et zoomable : souris (bouton maintenu + molette), trackpad / tactile
/// (glissement à deux doigts, pincement) et manette (actions d'entrée configurables).
/// Les mouvements sont lissés vers une position et un zoom cibles.
class CameraPanner : public Camera2D
{
    GDCLASS(CameraPanner, Camera2D)

  public:
    void _ready() override;
    void _process(double p_delta) override;
    void _input(const Ref<InputEvent> &p_event) override;

    void set_pan_mouse_button(MouseButton p_button) { pan_mouse_button = p_button; }
    MouseButton get_pan_mouse_button() const { return pan_mouse_button; }
    void set_zoom_factor(float p_factor) { zoom_factor = p_factor; }
    float get_zoom_factor() const { return zoom_factor; }
    void set_min_zoom(float p_zoom) { min_zoom = p_zoom; }
    float get_min_zoom() const { return min_zoom; }
    void set_max_zoom(float p_zoom) { max_zoom = p_zoom; }
    float get_max_zoom() const { return max_zoom; }
    void set_lerp_speed(float p_speed) { lerp_speed = p_speed; }
    float get_lerp_speed() const { return lerp_speed; }

    void set_joypad_pan_speed(float p_speed) { joypad_pan_speed = p_speed; }
    float get_joypad_pan_speed() const { return joypad_pan_speed; }
    void set_joypad_zoom_speed(float p_speed) { joypad_zoom_speed = p_speed; }
    float get_joypad_zoom_speed() const { return joypad_zoom_speed; }
    void set_touch_pan_sensitivity(float p_sensitivity) { touch_pan_sensitivity = p_sensitivity; }
    float get_touch_pan_sensitivity() const { return touch_pan_sensitivity; }

    void set_action_pan_left(const StringName &p_action) { action_pan_left = p_action; }
    StringName get_action_pan_left() const { return action_pan_left; }
    void set_action_pan_right(const StringName &p_action) { action_pan_right = p_action; }
    StringName get_action_pan_right() const { return action_pan_right; }
    void set_action_pan_up(const StringName &p_action) { action_pan_up = p_action; }
    StringName get_action_pan_up() const { return action_pan_up; }
    void set_action_pan_down(const StringName &p_action) { action_pan_down = p_action; }
    StringName get_action_pan_down() const { return action_pan_down; }
    void set_action_zoom_in(const StringName &p_action) { action_zoom_in = p_action; }
    StringName get_action_zoom_in() const { return action_zoom_in; }
    void set_action_zoom_out(const StringName &p_action) { action_zoom_out = p_action; }
    StringName get_action_zoom_out() const { return action_zoom_out; }

    /// Place immédiatement la caméra (position et zoom), sans lissage.
    void teleport(const Vector2 &p_position, float p_zoom);

  protected:
    static void _bind_methods();

  private:
    MouseButton pan_mouse_button = MOUSE_BUTTON_MIDDLE;
    float zoom_factor = 1.15f;
    float min_zoom = 0.1f;
    float max_zoom = 10.0f;
    float lerp_speed = 20.0f;

    // Sensibilité pour la manette et les gestes tactiles
    float joypad_pan_speed = 500.0f;
    float joypad_zoom_speed = 2.0f;
    float touch_pan_sensitivity = 1.0f;

    // Actions de l'Input Map utilisées par la manette (et le clavier)
    StringName action_pan_left = "ui_left";
    StringName action_pan_right = "ui_right";
    StringName action_pan_up = "ui_up";
    StringName action_pan_down = "ui_down";
    StringName action_zoom_in = "ui_page_up";
    StringName action_zoom_out = "ui_page_down";

    /// Position et zoom cibles modifiés par les entrées ; _process en rapproche la caméra (lerp_speed).
    /// Initialisés au _ready depuis position et zoom.x, remplacés par teleport().
    Vector2 target_position;
    float target_zoom = 1.0f;

    /// Actions de l'Input Map (déplacement et zoom vers le centre de l'écran), lues à chaque image.
    void _handle_joypad_input(float p_delta);
    /// Zoome d'un facteur en gardant fixe le point du monde situé sous p_screen_position.
    void _apply_zoom(float p_factor, const Vector2 &p_screen_position);
};
} // namespace godot

#endif // CAMERA_PANNER_HPP
