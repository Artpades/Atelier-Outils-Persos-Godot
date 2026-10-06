#include "drag_and_drop_system.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event_mouse.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/plane.hpp>

using namespace godot;

void DragAndDropSystem::_ready()
{
    pointer_position = get_viewport()->get_mouse_position();
    initialize_draggables();
}

void DragAndDropSystem::_process(double /*p_delta*/)
{
    // Chaque frame aussi (pas seulement aux mouvements de souris) : la caméra ou les objets peuvent bouger.
    _refresh();
}

void DragAndDropSystem::_refresh()
{
    if (is_free())
    {
        _update_hover();
        return;
    }

    // Nœud saisi libéré ou sorti de l'arbre pendant le glissement : la saisie est abandonnée.
    if (!_resolve_target(grabbed_component))
    {
        grabbed_component.unref();
        return;
    }
    _update_drag();
}

void DragAndDropSystem::_input(const Ref<InputEvent> &p_event)
{
    Ref<InputEventMouse> mouse = p_event;
    if (mouse.is_valid())
    {
        pointer_position = mouse->get_position();
        if (Ref<InputEventMouseMotion>(p_event).is_valid())
        {
            _refresh(); // sans attendre la frame suivante : pas de décalage d'une frame
        }
    }

    // Relâchement capté ici (avant l'interface) : jamais perdu, même au-dessus d'un Control.
    Ref<InputEventMouseButton> button = p_event;
    if (button.is_null() || button->get_button_index() != action_button || is_free())
    {
        return;
    }
    const bool releases = input_mode == INPUT_MODE_HOLD ? !button->is_pressed() : button->is_pressed();
    if (releases)
    {
        release();
        // Consommé : en mode bascule, le même clic ne doit pas ressaisir l'objet dans _unhandled_input.
        get_viewport()->set_input_as_handled();
    }
}

void DragAndDropSystem::_unhandled_input(const Ref<InputEvent> &p_event)
{
    Ref<InputEventMouseButton> button = p_event;
    if (button.is_null() || button->get_button_index() != action_button || !button->is_pressed() || is_occupied())
    {
        return;
    }

    // Survol mis à jour au moment du clic (le dernier _process peut dater d'avant le mouvement).
    _update_hover();
    if (hovered_component.is_valid())
    {
        grab(hovered_component);
        get_viewport()->set_input_as_handled();
    }
}

// 📋 Enregistrement

void DragAndDropSystem::initialize_draggables()
{
    for (int64_t i = 0; i < draggables.size(); i++)
    {
        Ref<DraggableComponent> component = draggables[i];
        if (component.is_null())
        {
            continue;
        }
        if (Node *target = _resolve_target(component))
        {
            component->save_initial_position(target);
        }
    }
}

void DragAndDropSystem::register_draggable(const Ref<DraggableComponent> &p_component)
{
    if (p_component.is_null() || draggables.has(p_component))
    {
        return;
    }
    draggables.push_back(p_component);

    if (!is_inside_tree())
    {
        return; // initialisé au _ready
    }
    if (Node *target = _resolve_target(p_component))
    {
        p_component->save_initial_position(target);
    }
}

void DragAndDropSystem::unregister_draggable(const Ref<DraggableComponent> &p_component)
{
    draggables.erase(p_component);

    if (p_component.is_valid() && hovered_component == p_component)
    {
        hovered_component.unref();
        if (Node *target = p_component->get_target_node())
        {
            emit_signal("depointed", target);
        }
    }
    if (p_component.is_valid() && grabbed_component == p_component)
    {
        grabbed_component.unref(); // abandonné sur place, sans comportement de relâchement
    }
}

Node *DragAndDropSystem::_resolve_target(const Ref<DraggableComponent> &p_component)
{
    Node *target = p_component->get_target_node();
    if (!target && !p_component->get_target_node_path().is_empty() && is_inside_tree())
    {
        target = get_node_or_null(p_component->get_target_node_path());
        if (target && target->is_inside_tree())
        {
            // Résolu après coup (nœud ajouté plus tard) : sa position actuelle devient l'initiale.
            p_component->set_target_node(target);
            p_component->save_initial_position(target);
        }
    }
    return target && target->is_inside_tree() ? target : nullptr;
}

// 👆 Survol

void DragAndDropSystem::_update_hover()
{
    const Ref<DraggableComponent> detected = get_draggable_at(pointer_position);
    if (detected == hovered_component)
    {
        return;
    }

    const Ref<DraggableComponent> previous = hovered_component;
    hovered_component = detected;

    if (previous.is_valid())
    {
        if (Node *target = previous->get_target_node())
        {
            emit_signal("depointed", target);
        }
    }
    if (detected.is_valid())
    {
        emit_signal("pointed", detected->get_target_node());
    }
}

Ref<DraggableComponent> DragAndDropSystem::get_draggable_at(const Vector2 &p_position)
{
    ERR_FAIL_COND_V(!is_inside_tree(), Ref<DraggableComponent>());

    Camera3D *camera = get_viewport()->get_camera_3d();
    Ref<DraggableComponent> closest_3d;
    float closest_depth = INFINITY;

    for (int64_t i = 0; i < draggables.size(); i++)
    {
        Ref<DraggableComponent> component = draggables[i];
        if (component.is_null())
        {
            continue;
        }
        Node *target = _resolve_target(component);

        // 2D : pointeur ramené dans le repère local du nœud (suit position, rotation et échelle).
        if (Control *control = Object::cast_to<Control>(target))
        {
            const Vector2 local = control->get_global_transform_with_canvas().affine_inverse().xform(p_position);
            if (control->is_visible_in_tree() && Rect2(Vector2(), control->get_size()).has_point(local))
            {
                return component;
            }
        }
        else if (Node2D *node_2d = Object::cast_to<Node2D>(target))
        {
            const Vector2 local = node_2d->get_global_transform_with_canvas().affine_inverse().xform(p_position);
            if (node_2d->is_visible_in_tree() && component->get_local_bounds_2d().has_point(local))
            {
                return component;
            }
        }
        // 3D : rayon de la caméra ramené dans le repère local du nœud, testé contre sa boîte.
        else if (Node3D *node_3d = Object::cast_to<Node3D>(target))
        {
            if (!camera || !node_3d->is_visible_in_tree())
            {
                continue;
            }
            const Vector3 ray_origin = camera->project_ray_origin(p_position);
            const Vector3 ray_end = ray_origin + camera->project_ray_normal(p_position) * camera->get_far();
            const Transform3D global = node_3d->get_global_transform();
            const Transform3D to_local = global.affine_inverse();
            Vector3 hit;
            if (component->get_bounds_3d().intersects_segment(to_local.xform(ray_origin), to_local.xform(ray_end), &hit))
            {
                const float depth = ray_origin.distance_to(global.xform(hit));
                if (depth < closest_depth)
                {
                    closest_depth = depth;
                    closest_3d = component;
                }
            }
        }
    }
    return closest_3d;
}

// ✊ Saisie et relâchement

void DragAndDropSystem::grab(const Ref<DraggableComponent> &p_component)
{
    ERR_FAIL_COND_MSG(p_component.is_null(), String(U"[DragAndDropSystem] Composant nul."));
    ERR_FAIL_COND_MSG(is_occupied(), String(U"[DragAndDropSystem] Un objet est déjà saisi : le relâcher d'abord."));
    Node *target = _resolve_target(p_component);
    ERR_FAIL_NULL_MSG(target, String(U"[DragAndDropSystem] Nœud ciblé introuvable ou hors de l'arbre."));

    const bool snap_to_cursor = p_component->get_snap_to_cursor();
    if (Node2D *node_2d = Object::cast_to<Node2D>(target))
    {
        offset_2d = snap_to_cursor ? Vector2() : node_2d->get_global_position() - _pointer_in_canvas(node_2d);
    }
    else if (Control *control = Object::cast_to<Control>(target))
    {
        offset_2d = snap_to_cursor ? Vector2() : control->get_global_position() - _pointer_in_canvas(control);
    }
    else if (Node3D *node_3d = Object::cast_to<Node3D>(target))
    {
        drag_origin_3d = node_3d->get_global_position();
        Vector3 intersection;
        offset_3d = !snap_to_cursor && _intersect_drag_plane(p_component, node_3d, intersection) ? drag_origin_3d - intersection : Vector3();
    }
    else
    {
        ERR_FAIL_MSG(String(U"[DragAndDropSystem] Le nœud ciblé doit être un Control, un Node2D ou un Node3D."));
    }

    grabbed_component = p_component;
    emit_signal("grabbed", target);
}

void DragAndDropSystem::release()
{
    if (grabbed_component.is_null())
    {
        return;
    }
    // Libéré avant les signaux : un récepteur peut saisir un autre objet.
    const Ref<DraggableComponent> component = grabbed_component;
    grabbed_component.unref();

    Node *target = _resolve_target(component);
    if (!target)
    {
        return;
    }

    Node *snap_target = _find_snap_target(component, target, _get_global_position(target));
    switch (component->get_release_behavior())
    {
    case DraggableComponent::RELEASE_RETURN_TO_INITIAL:
        _reset_to_initial_position(component, target);
        break;
    case DraggableComponent::RELEASE_SNAP_TO_VALID_POINT:
        if (snap_target)
        {
            _set_global_position(target, _get_global_position(snap_target));
        }
        else
        {
            _reset_to_initial_position(component, target);
        }
        break;
    case DraggableComponent::RELEASE_LEAVE_IN_PLACE:
        if (snap_target && component->get_snap_behavior() == DraggableComponent::SNAP_WHEN_CLOSE)
        {
            _set_global_position(target, _get_global_position(snap_target));
        }
        break;
    case DraggableComponent::RELEASE_DO_NOTHING:
    default:
        break;
    }

    // Posé sur un point d'aimantage (au relâchement, ou déjà aimanté pendant le glissement)
    if (snap_target && _same_position(_get_global_position(target), _get_global_position(snap_target)))
    {
        emit_signal("snapped", target, snap_target);
    }
    emit_signal("released", target);
}

// 🧭 Déplacement

void DragAndDropSystem::_update_drag()
{
    Node *target = grabbed_component->get_target_node();

    if (Node2D *node_2d = Object::cast_to<Node2D>(target))
    {
        _set_global_position(node_2d, _snap_while_dragging(grabbed_component, node_2d, _pointer_in_canvas(node_2d) + offset_2d));
    }
    else if (Control *control = Object::cast_to<Control>(target))
    {
        _set_global_position(control, _snap_while_dragging(grabbed_component, control, _pointer_in_canvas(control) + offset_2d));
    }
    else if (Node3D *node_3d = Object::cast_to<Node3D>(target))
    {
        Vector3 intersection;
        if (_intersect_drag_plane(grabbed_component, node_3d, intersection))
        {
            _set_global_position(node_3d, _snap_while_dragging(grabbed_component, node_3d, intersection + offset_3d));
        }
    }
}

Vector2 DragAndDropSystem::_pointer_in_canvas(const CanvasItem *p_item) const
{
    return p_item->get_canvas_transform().affine_inverse().xform(pointer_position);
}

bool DragAndDropSystem::_intersect_drag_plane(const Ref<DraggableComponent> &p_component, const Node3D *p_node, Vector3 &r_point) const
{
    Camera3D *camera = get_viewport()->get_camera_3d();
    if (!camera)
    {
        return false;
    }

    const Basis object_basis = p_node->get_global_transform().basis;
    Vector3 normal;
    switch (p_component->get_drag_plane_3d())
    {
    case DraggableComponent::DRAG_PLANE_OBJECT_X:
        normal = object_basis.get_column(0);
        break;
    case DraggableComponent::DRAG_PLANE_OBJECT_Y:
        normal = object_basis.get_column(1);
        break;
    case DraggableComponent::DRAG_PLANE_OBJECT_Z:
        normal = object_basis.get_column(2);
        break;
    case DraggableComponent::DRAG_PLANE_WORLD_X:
        normal = Vector3(1, 0, 0);
        break;
    case DraggableComponent::DRAG_PLANE_WORLD_Y:
        normal = Vector3(0, 1, 0);
        break;
    case DraggableComponent::DRAG_PLANE_WORLD_Z:
        normal = Vector3(0, 0, 1);
        break;
    case DraggableComponent::DRAG_PLANE_CAMERA:
    default:
        normal = -camera->get_global_transform().basis.get_column(2);
        break;
    }

    // Plan fixé à la position de saisie : un aimantage hors du plan ne le déplace pas.
    const Plane plane(normal.normalized(), drag_origin_3d);
    return plane.intersects_ray(camera->project_ray_origin(pointer_position), camera->project_ray_normal(pointer_position), &r_point);
}

// 🧲 Aimantage

Node *DragAndDropSystem::_find_snap_target(const Ref<DraggableComponent> &p_component, Node *p_target, const Variant &p_from) const
{
    Node *closest = nullptr;
    float closest_distance = INFINITY;
    const float threshold = p_component->get_snap_distance_threshold();
    const TypedArray<NodePath> snap_targets = p_component->get_snap_targets();

    for (int64_t i = 0; i < snap_targets.size(); i++)
    {
        Node *snap = get_node_or_null(snap_targets[i]);
        if (!snap || snap == p_target || !snap->is_inside_tree())
        {
            continue;
        }
        float distance;
        if (_distance_to(p_target, p_from, snap, distance) && distance <= threshold && distance < closest_distance)
        {
            closest_distance = distance;
            closest = snap;
        }
    }
    return closest;
}

Variant DragAndDropSystem::_snap_while_dragging(const Ref<DraggableComponent> &p_component, Node *p_target, const Variant &p_desired) const
{
    if (p_component->get_snap_behavior() != DraggableComponent::SNAP_ALWAYS)
    {
        return p_desired;
    }
    // Distance mesurée depuis la position voulue (pas la position actuelle, déjà aimantée) :
    // l'objet se décroche dès que le pointeur s'éloigne du point.
    Node *snap_target = _find_snap_target(p_component, p_target, p_desired);
    return snap_target ? _get_global_position(snap_target) : p_desired;
}

// 🛠️ Utilitaires

void DragAndDropSystem::_reset_to_initial_position(const Ref<DraggableComponent> &p_component, Node *p_target) const
{
    if (Object::cast_to<Node3D>(p_target))
    {
        _set_global_position(p_target, p_component->get_initial_position_3d());
    }
    else
    {
        _set_global_position(p_target, p_component->get_initial_position_2d());
    }
}

Variant DragAndDropSystem::_get_global_position(const Node *p_node)
{
    if (const Node2D *node_2d = Object::cast_to<Node2D>(p_node))
    {
        return node_2d->get_global_position();
    }
    if (const Control *control = Object::cast_to<Control>(p_node))
    {
        return control->get_global_position();
    }
    if (const Node3D *node_3d = Object::cast_to<Node3D>(p_node))
    {
        return node_3d->get_global_position();
    }
    return Variant();
}

void DragAndDropSystem::_set_global_position(Node *p_node, const Variant &p_position)
{
    if (Node2D *node_2d = Object::cast_to<Node2D>(p_node))
    {
        node_2d->set_global_position(p_position);
    }
    else if (Control *control = Object::cast_to<Control>(p_node))
    {
        control->set_global_position(p_position);
    }
    else if (Node3D *node_3d = Object::cast_to<Node3D>(p_node))
    {
        node_3d->set_global_position(p_position);
    }
}

bool DragAndDropSystem::_same_position(const Variant &p_a, const Variant &p_b)
{
    // Approché : set_global_position sous des parents transformés introduit des arrondis.
    if (p_a.get_type() == Variant::VECTOR3)
    {
        return Vector3(p_a).is_equal_approx(p_b);
    }
    return Vector2(p_a).is_equal_approx(p_b);
}

bool DragAndDropSystem::_distance_to(const Node *p_target, const Variant &p_from, const Node *p_snap, float &r_distance)
{
    // Même type exigé : un Control (repère de l'interface) et un Node2D (repère du monde) ne se comparent pas.
    const bool same_kind = (Object::cast_to<Node2D>(p_target) && Object::cast_to<Node2D>(p_snap)) ||
                           (Object::cast_to<Control>(p_target) && Object::cast_to<Control>(p_snap)) ||
                           (Object::cast_to<Node3D>(p_target) && Object::cast_to<Node3D>(p_snap));
    if (!same_kind)
    {
        return false;
    }
    const Variant snap_position = _get_global_position(p_snap);
    if (p_from.get_type() == Variant::VECTOR3)
    {
        r_distance = Vector3(p_from).distance_to(snap_position);
    }
    else
    {
        r_distance = Vector2(p_from).distance_to(snap_position);
    }
    return true;
}

// 🔗 Liaisons

void DragAndDropSystem::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("initialize_draggables"), &DragAndDropSystem::initialize_draggables);
    ClassDB::bind_method(D_METHOD("register_draggable", "component"), &DragAndDropSystem::register_draggable);
    ClassDB::bind_method(D_METHOD("unregister_draggable", "component"), &DragAndDropSystem::unregister_draggable);
    ClassDB::bind_method(D_METHOD("get_draggable_at", "position"), &DragAndDropSystem::get_draggable_at);
    ClassDB::bind_method(D_METHOD("grab", "component"), &DragAndDropSystem::grab);
    ClassDB::bind_method(D_METHOD("release"), &DragAndDropSystem::release);
    ClassDB::bind_method(D_METHOD("is_occupied"), &DragAndDropSystem::is_occupied);
    ClassDB::bind_method(D_METHOD("is_free"), &DragAndDropSystem::is_free);
    ClassDB::bind_method(D_METHOD("get_hovered_component"), &DragAndDropSystem::get_hovered_component);
    ClassDB::bind_method(D_METHOD("get_grabbed_component"), &DragAndDropSystem::get_grabbed_component);
    ClassDB::bind_method(D_METHOD("set_action_button", "button"), &DragAndDropSystem::set_action_button);
    ClassDB::bind_method(D_METHOD("get_action_button"), &DragAndDropSystem::get_action_button);
    ClassDB::bind_method(D_METHOD("set_input_mode", "mode"), &DragAndDropSystem::set_input_mode);
    ClassDB::bind_method(D_METHOD("get_input_mode"), &DragAndDropSystem::get_input_mode);
    ClassDB::bind_method(D_METHOD("set_draggables", "draggables"), &DragAndDropSystem::set_draggables);
    ClassDB::bind_method(D_METHOD("get_draggables"), &DragAndDropSystem::get_draggables);

    BIND_ENUM_CONSTANT(INPUT_MODE_HOLD);
    BIND_ENUM_CONSTANT(INPUT_MODE_TOGGLE);

    ADD_GROUP(U"🖱️ Entrées", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "action_button", PROPERTY_HINT_ENUM, "Left:1,Right:2,Middle:3,Extra 1:8,Extra 2:9"), "set_action_button", "get_action_button");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "input_mode", PROPERTY_HINT_ENUM, "Hold,Toggle"), "set_input_mode", "get_input_mode");
    ADD_GROUP(U"📦 Objets déplaçables", "");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "draggables", PROPERTY_HINT_ARRAY_TYPE, "DraggableComponent"), "set_draggables", "get_draggables");

    ADD_SIGNAL(MethodInfo("pointed", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_NODE_TYPE, "Node")));
    ADD_SIGNAL(MethodInfo("depointed", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_NODE_TYPE, "Node")));
    ADD_SIGNAL(MethodInfo("grabbed", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_NODE_TYPE, "Node")));
    ADD_SIGNAL(MethodInfo("released", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_NODE_TYPE, "Node")));
    ADD_SIGNAL(MethodInfo("snapped", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_NODE_TYPE, "Node"), PropertyInfo(Variant::OBJECT, "snap_target", PROPERTY_HINT_NODE_TYPE, "Node")));
}
