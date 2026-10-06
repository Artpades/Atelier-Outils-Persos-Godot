#ifndef DRAGGABLE_COMPONENT_HPP
#define DRAGGABLE_COMPONENT_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// ✊📦 Configuration d'un objet déplaçable par un DragAndDropSystem : nœud ciblé (Control, Node2D ou
/// Node3D), zone de détection, plan de glissement 3D, comportement au relâchement et points d'aimantage.
/// Les chemins (target_node_path, snap_targets) sont relatifs au DragAndDropSystem.
class DraggableComponent : public Resource
{
    GDCLASS(DraggableComponent, Resource)

  public:
    /// Plan sur lequel glisse un Node3D (passant par sa position au moment de la saisie).
    enum DragPlane
    {
        DRAG_PLANE_CAMERA,   // face à la caméra
        DRAG_PLANE_OBJECT_X, // normale = axe X local de l'objet
        DRAG_PLANE_OBJECT_Y,
        DRAG_PLANE_OBJECT_Z,
        DRAG_PLANE_WORLD_X, // normale = axe X du monde
        DRAG_PLANE_WORLD_Y, // sol horizontal
        DRAG_PLANE_WORLD_Z,
    };

    enum ReleaseBehavior
    {
        RELEASE_RETURN_TO_INITIAL,   // revient toujours à sa position initiale
        RELEASE_SNAP_TO_VALID_POINT, // va au point d'aimantage le plus proche, sinon revient
        RELEASE_LEAVE_IN_PLACE,      // reste là où il est lâché (aimanté si SNAP_WHEN_CLOSE et assez proche)
        RELEASE_DO_NOTHING,          // reste là où il est lâché, sans aimantage
    };

    enum SnapMode
    {
        SNAP_NONE,
        SNAP_WHEN_CLOSE, // aimanté au relâchement s'il est assez proche d'un point
        SNAP_ALWAYS,     // aimanté pendant le glissement dès qu'il passe près d'un point
    };

    void set_target_node_path(const NodePath &p_path) { target_node_path = p_path; }
    NodePath get_target_node_path() const { return target_node_path; }
    /// Nœud déplacé : résolu depuis target_node_path par le DragAndDropSystem, ou défini par code.
    void set_target_node(Node *p_node) { target_node_id = p_node ? ObjectID(p_node->get_instance_id()) : ObjectID(); }
    Node *get_target_node() const { return Object::cast_to<Node>(ObjectDB::get_instance(target_node_id)); }

    void set_local_bounds_2d(const Rect2 &p_bounds) { local_bounds_2d = p_bounds; }
    Rect2 get_local_bounds_2d() const { return local_bounds_2d; }
    void set_bounds_3d(const AABB &p_bounds) { bounds_3d = p_bounds; }
    AABB get_bounds_3d() const { return bounds_3d; }

    void set_snap_to_cursor(bool p_enabled) { snap_to_cursor = p_enabled; }
    bool get_snap_to_cursor() const { return snap_to_cursor; }
    void set_drag_plane_3d(DragPlane p_plane) { drag_plane_3d = p_plane; }
    DragPlane get_drag_plane_3d() const { return drag_plane_3d; }
    void set_release_behavior(ReleaseBehavior p_behavior) { release_behavior = p_behavior; }
    ReleaseBehavior get_release_behavior() const { return release_behavior; }

    void set_snap_behavior(SnapMode p_mode) { snap_behavior = p_mode; }
    SnapMode get_snap_behavior() const { return snap_behavior; }
    void set_snap_distance_threshold(float p_distance) { snap_distance_threshold = p_distance; }
    float get_snap_distance_threshold() const { return snap_distance_threshold; }
    void set_snap_targets(const TypedArray<NodePath> &p_targets) { snap_targets = p_targets; }
    TypedArray<NodePath> get_snap_targets() const { return snap_targets; }

    void set_initial_position_2d(const Vector2 &p_position) { initial_position_2d = p_position; }
    Vector2 get_initial_position_2d() const { return initial_position_2d; }
    void set_initial_position_3d(const Vector3 &p_position) { initial_position_3d = p_position; }
    Vector3 get_initial_position_3d() const { return initial_position_3d; }

    /// Mémorise la position globale de p_node (par défaut : le nœud ciblé) comme position initiale.
    void save_initial_position(Node *p_node = nullptr);

  protected:
    static void _bind_methods();

  private:
    NodePath target_node_path;
    // ObjectID : le nœud peut être libéré sans prévenir la ressource.
    ObjectID target_node_id;

    /// Zone de saisie d'un Node2D, dans son repère local (suit rotation et échelle).
    Rect2 local_bounds_2d = Rect2(-32, -32, 64, 64);
    /// Zone de saisie d'un Node3D, dans son repère local.
    AABB bounds_3d = AABB(Vector3(-0.5f, -0.5f, -0.5f), Vector3(1, 1, 1));

    bool snap_to_cursor = false;
    DragPlane drag_plane_3d = DRAG_PLANE_CAMERA;
    ReleaseBehavior release_behavior = RELEASE_SNAP_TO_VALID_POINT;

    SnapMode snap_behavior = SNAP_WHEN_CLOSE;
    float snap_distance_threshold = 50.0f; // pixels en 2D, unités en 3D
    TypedArray<NodePath> snap_targets;

    Vector2 initial_position_2d;
    Vector3 initial_position_3d;
};
} // namespace godot

VARIANT_ENUM_CAST(DraggableComponent::DragPlane);
VARIANT_ENUM_CAST(DraggableComponent::ReleaseBehavior);
VARIANT_ENUM_CAST(DraggableComponent::SnapMode);

#endif // DRAGGABLE_COMPONENT_HPP
