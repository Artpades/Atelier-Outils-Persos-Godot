#ifndef DRAG_AND_DROP_SYSTEM_HPP
#define DRAG_AND_DROP_SYSTEM_HPP

#include "draggable_component.hpp"

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>

namespace godot
{
/// ✊📦🤚 Glisser-déposer à la souris des Control, Node2D et Node3D décrits par ses DraggableComponent :
/// survol, saisie (bouton maintenu ou bascule), déplacement (plan de glissement en 3D), puis relâchement
/// selon le comportement du composant (retour, aimantage vers un point, laissé sur place).
///
/// La saisie passe par _unhandled_input : une interface (Control en MOUSE_FILTER_STOP) placée au-dessus
/// d'un objet bloque donc sa saisie ; un Control déplaçable doit être en MOUSE_FILTER_IGNORE ou PASS.
/// Le relâchement passe par _input : il n'est jamais perdu, même au-dessus d'une interface.
class DragAndDropSystem : public Node
{
    GDCLASS(DragAndDropSystem, Node)

  public:
    enum InputMode
    {
        INPUT_MODE_HOLD,   // saisi tant que le bouton est maintenu
        INPUT_MODE_TOGGLE, // un clic saisit, le clic suivant relâche
    };

    void _ready() override;
    void _process(double p_delta) override;
    void _input(const Ref<InputEvent> &p_event) override;
    void _unhandled_input(const Ref<InputEvent> &p_event) override;

    /// Résout les nœuds ciblés et mémorise leurs positions initiales (appelé au _ready).
    void initialize_draggables();
    void register_draggable(const Ref<DraggableComponent> &p_component);
    void unregister_draggable(const Ref<DraggableComponent> &p_component);

    /// Composant dont le nœud se trouve sous p_position (coordonnées du Viewport), ou null.
    /// Un Control ou Node2D l'emporte sur un Node3D ; entre Node3D, le plus proche de la caméra.
    Ref<DraggableComponent> get_draggable_at(const Vector2 &p_position);
    /// Saisit p_component par code, comme un clic à la position actuelle du pointeur.
    void grab(const Ref<DraggableComponent> &p_component);
    /// Relâche l'objet saisi en appliquant son comportement de relâchement.
    void release();

    bool is_occupied() const { return grabbed_component.is_valid(); }
    bool is_free() const { return !is_occupied(); }
    Ref<DraggableComponent> get_hovered_component() const { return hovered_component; }
    Ref<DraggableComponent> get_grabbed_component() const { return grabbed_component; }

    void set_action_button(MouseButton p_button) { action_button = p_button; }
    MouseButton get_action_button() const { return action_button; }
    void set_input_mode(InputMode p_mode) { input_mode = p_mode; }
    InputMode get_input_mode() const { return input_mode; }
    void set_draggables(const TypedArray<DraggableComponent> &p_draggables) { draggables = p_draggables; }
    TypedArray<DraggableComponent> get_draggables() const { return draggables; }

  protected:
    static void _bind_methods();

  private:
    MouseButton action_button = MOUSE_BUTTON_LEFT;
    InputMode input_mode = INPUT_MODE_HOLD;
    TypedArray<DraggableComponent> draggables;

    Ref<DraggableComponent> hovered_component;
    Ref<DraggableComponent> grabbed_component;

    /// Dernière position connue du pointeur, en coordonnées du Viewport (suivie dans _input).
    Vector2 pointer_position;
    /// Décalage entre l'objet saisi et le pointeur au moment de la saisie (nul avec snap_to_cursor).
    Vector2 offset_2d;
    Vector3 offset_3d;
    /// Point par lequel passe le plan de glissement 3D (position de l'objet à la saisie).
    Vector3 drag_origin_3d;

    /// Nœud ciblé (résolu et mis en cache au premier accès), ou nullptr s'il est absent / hors de l'arbre.
    Node *_resolve_target(const Ref<DraggableComponent> &p_component);
    /// Met à jour le survol (libre) ou la position de l'objet saisi (occupé).
    void _refresh();
    void _update_hover();
    void _update_drag();

    /// Position du pointeur dans le repère global (canvas) de p_item.
    Vector2 _pointer_in_canvas(const CanvasItem *p_item) const;
    /// Intersection du rayon du pointeur avec le plan de glissement du composant.
    bool _intersect_drag_plane(const Ref<DraggableComponent> &p_component, const Node3D *p_node, Vector3 &r_point) const;

    /// Point d'aimantage le plus proche de p_from (même type de nœud que p_target), dans le seuil.
    Node *_find_snap_target(const Ref<DraggableComponent> &p_component, Node *p_target, const Variant &p_from) const;
    /// p_desired, ou la position du point d'aimantage le plus proche si le composant est en SNAP_ALWAYS.
    Variant _snap_while_dragging(const Ref<DraggableComponent> &p_component, Node *p_target, const Variant &p_desired) const;
    void _reset_to_initial_position(const Ref<DraggableComponent> &p_component, Node *p_target) const;

    /// Position globale (Vector2 ou Vector3) d'un Node2D, Control ou Node3D ; Variant nul sinon.
    static Variant _get_global_position(const Node *p_node);
    static void _set_global_position(Node *p_node, const Variant &p_position);
    static bool _same_position(const Variant &p_a, const Variant &p_b);
    /// Distance entre p_from et p_snap, si p_target et p_snap sont du même type (Node2D, Control ou Node3D).
    static bool _distance_to(const Node *p_target, const Variant &p_from, const Node *p_snap, float &r_distance);
};
} // namespace godot

VARIANT_ENUM_CAST(DragAndDropSystem::InputMode);

#endif // DRAG_AND_DROP_SYSTEM_HPP
