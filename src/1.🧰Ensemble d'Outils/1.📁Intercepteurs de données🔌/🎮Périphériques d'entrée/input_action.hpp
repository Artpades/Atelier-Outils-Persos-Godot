#ifndef INPUT_ACTION_HPP
#define INPUT_ACTION_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot
{
/// 🎮 Base abstraite d'une action d'entrée : une valeur lue sur un périphérique (bouton, axe, stick, capteur...)
/// et émise par signal quand elle change. Le périphérique (device_id) et le filtre (detector_type) sont
/// normalement fournis par le PlayerInput parent.
///
/// Outils de debug communs à toutes les actions :
/// - debug_log : trace les événements dans le 📰Journal (CustomLogger).
/// - debug_draw : dessine l'état de l'action avec les 🐛Gizmos2D, dans le canvas du viewport.
class InputAction : public Node
{
    GDCLASS(InputAction, Node)

  public:
    enum DeviceDetectorType
    {
        DETECTOR_ALL_DEVICES = 1,
        DETECTOR_KEYBOARD_MOUSE = 2,
        DETECTOR_GAMEPAD = 3,
    };

    void _exit_tree() override;

    void set_detector_type(DeviceDetectorType p_type) { detector_type = p_type; }
    DeviceDetectorType get_detector_type() const { return detector_type; }

    void set_action_name(const StringName &p_name) { action_name = p_name; }
    StringName get_action_name() const { return action_name; }

    void set_action_enabled(bool p_enabled) { action_enabled = p_enabled; }
    bool is_action_enabled() const { return action_enabled; }

    void set_dead_zone(float p_dead_zone) { dead_zone = p_dead_zone; }
    float get_dead_zone() const { return dead_zone; }

    void set_dirty_threshold(float p_threshold) { dirty_threshold = p_threshold; }
    float get_dirty_threshold() const { return dirty_threshold; }

    void set_device_id(int p_id) { device_id = p_id; }
    int get_device_id() const { return device_id; }

    void set_debug_log(bool p_enabled) { debug_log = p_enabled; }
    bool get_debug_log() const { return debug_log; }

    void set_debug_draw(bool p_enabled);
    bool get_debug_draw() const { return debug_draw; }

    void set_debug_draw_position(const Vector2 &p_position) { debug_draw_position = p_position; }
    Vector2 get_debug_draw_position() const { return debug_draw_position; }

    void set_debug_draw_size(float p_size) { debug_draw_size = p_size; }
    float get_debug_draw_size() const { return debug_draw_size; }

    bool is_keyboard_mouse_allowed() const { return detector_type == DETECTOR_ALL_DEVICES || detector_type == DETECTOR_KEYBOARD_MOUSE; }
    bool is_gamepad_allowed() const { return detector_type == DETECTOR_ALL_DEVICES || detector_type == DETECTOR_GAMEPAD; }

    /// Propriétés qui décrivent l'affectation (touches, boutons, axes) : c'est ce que PlayerInputUI
    /// sauvegarde, restaure et réinitialise. Les préréglages viennent en premier : les rétablir réécrit les
    /// touches, qui sont ensuite rétablies à leur tour.
    virtual PackedStringArray get_binding_properties() const { return PackedStringArray(); }

  protected:
    static void _bind_methods();

    /// device_id si p_device < 0 (valeur par défaut des méthodes de lecture exposées).
    int resolve_device(int p_device) const { return p_device < 0 ? device_id : p_device; }

    /// Trace un message dans le 📰Journal si debug_log est actif.
    void log_debug(const String &p_message) const;

    /// Faux si debug_draw est inactif (les gizmos sont alors libérés).
    bool can_debug_draw();
    /// Gizmo 2D n° p_index : chaque appel à Gizmos2D::draw_2D_* efface son canvas item, une forme = un gizmo.
    RID debug_gizmo(int p_index);
    /// Dessine le nom de l'action au-dessus de la zone de debug.
    void draw_debug_label();

    DeviceDetectorType detector_type = DETECTOR_ALL_DEVICES;
    StringName action_name = StringName(String(U"[Insérer le nom de l'action ici]"));
    bool action_enabled = true;
    float dead_zone = 0.2f;
    float dirty_threshold = 0.01f;
    int device_id = 0;

    bool debug_log = false;
    bool debug_draw = false;
    Vector2 debug_draw_position = Vector2(64, 64);
    float debug_draw_size = 48.0f;

  private:
    void free_debug_gizmos();

    Vector<RID> debug_gizmos;
};
} // namespace godot

VARIANT_ENUM_CAST(InputAction::DeviceDetectorType);

#endif // INPUT_ACTION_HPP
