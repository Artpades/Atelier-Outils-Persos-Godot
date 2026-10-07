#include "interactive_hud_3d.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

static bool is_wheel(MouseButton p_button)
{
    return p_button >= MOUSE_BUTTON_WHEEL_UP && p_button <= MOUSE_BUTTON_WHEEL_RIGHT;
}

static int64_t button_mask(MouseButton p_button)
{
    return int64_t(1) << (int64_t(p_button) - 1);
}

void InteractiveHUD3D::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_ENTER_TREE:
        _build();
        add_to_group(GROUP);
        mouse_position = get_viewport()->get_mouse_position();
        break;
    case NOTIFICATION_EXIT_TREE:
        _release_all();
        _set_hovered(false);
        _set_focused(false);
        break;
    case NOTIFICATION_VISIBILITY_CHANGED:
        if (!is_visible_in_tree())
        {
            _release_all();
            _set_hovered(false);
            _set_focused(false);
        }
        break;
    default:
        break;
    }
}

// 🧱 Construction

void InteractiveHUD3D::_build()
{
    if (sub_viewport)
    {
        return;
    }

    // Enfants internes et sans propriétaire : jamais sauvegardés avec la scène, ni dupliqués.
    sub_viewport = memnew(SubViewport);
    sub_viewport->set_name("HUDViewport");
    sub_viewport->set_size(viewport_size);
    sub_viewport->set_transparent_background(transparent_background);
    sub_viewport->set_update_mode(SubViewport::UPDATE_ALWAYS);
    add_child(sub_viewport, false, INTERNAL_MODE_FRONT);

    quad.instantiate();
    material.instantiate();
    material->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, sub_viewport->get_texture());
    material->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_LINEAR);

    mesh_instance = memnew(MeshInstance3D);
    mesh_instance->set_name("HUDQuad");
    mesh_instance->set_mesh(quad);
    mesh_instance->set_material_override(material);
    add_child(mesh_instance, false, INTERNAL_MODE_FRONT);

    _update_mesh();
    _update_material();
    if (content_scene.is_valid() && !get_content())
    {
        set_content(content_scene->instantiate());
    }
}

void InteractiveHUD3D::_update_mesh()
{
    if (quad.is_valid())
    {
        quad->set_size(get_quad_size());
    }
}

void InteractiveHUD3D::_update_material()
{
    if (material.is_null())
    {
        return;
    }
    material->set_shading_mode(unshaded ? BaseMaterial3D::SHADING_MODE_UNSHADED : BaseMaterial3D::SHADING_MODE_PER_PIXEL);
    material->set_transparency(transparent_background || modulate.a < 1.0f ? BaseMaterial3D::TRANSPARENCY_ALPHA : BaseMaterial3D::TRANSPARENCY_DISABLED);
    material->set_cull_mode(double_sided ? BaseMaterial3D::CULL_DISABLED : BaseMaterial3D::CULL_BACK);
    material->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, always_on_top);
    material->set_render_priority(always_on_top ? Material::RENDER_PRIORITY_MAX : 0);
    material->set_albedo(modulate);
}

void InteractiveHUD3D::_update_billboard()
{
    if (!mesh_instance || !is_inside_tree())
    {
        return;
    }
    if (billboard == BILLBOARD_DISABLED)
    {
        mesh_instance->set_transform(Transform3D());
        return;
    }
    Camera3D *camera = get_viewport()->get_camera_3d();
    if (!camera)
    {
        return;
    }

    const Transform3D global = get_global_transform();
    Basis facing; // +Z (face avant du quad) tourné vers la caméra
    if (billboard == BILLBOARD_ENABLED)
    {
        facing = camera->get_global_transform().basis.orthonormalized();
    }
    else
    {
        Vector3 to_camera = camera->get_global_position() - global.origin;
        to_camera.y = 0;
        if (to_camera.length_squared() > CMP_EPSILON)
        {
            facing = Basis::looking_at(-to_camera.normalized(), Vector3(0, 1, 0));
        }
    }
    mesh_instance->set_global_transform(Transform3D(facing * Basis::from_scale(global.basis.get_scale()), global.origin));
}

// 🧩 Contenu

void InteractiveHUD3D::set_content_scene(const Ref<PackedScene> &p_scene)
{
    content_scene = p_scene;
    if (!sub_viewport)
    {
        return; // instancié à la construction
    }
    set_content(content_scene.is_valid() ? content_scene->instantiate() : nullptr);
}

void InteractiveHUD3D::set_content(Node *p_content)
{
    _build();
    _release_all();
    if (Node *previous = get_content())
    {
        sub_viewport->remove_child(previous);
        previous->queue_free(); // différé : l'ancien contenu peut être l'émetteur du signal en cours
    }
    if (!p_content)
    {
        return;
    }
    sub_viewport->add_child(p_content);
    if (Control *control = Object::cast_to<Control>(p_content))
    {
        control->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    }
}

Node *InteractiveHUD3D::get_content() const
{
    return sub_viewport && sub_viewport->get_child_count() > 0 ? sub_viewport->get_child(0) : nullptr;
}

void InteractiveHUD3D::set_viewport_size(const Vector2i &p_size)
{
    viewport_size = p_size.max(Vector2i(1, 1));
    if (sub_viewport)
    {
        sub_viewport->set_size(viewport_size);
    }
    _update_mesh();
}

void InteractiveHUD3D::set_transparent_background(bool p_enabled)
{
    transparent_background = p_enabled;
    if (sub_viewport)
    {
        sub_viewport->set_transparent_background(p_enabled);
    }
    _update_material();
}

// 🎨 Affichage

void InteractiveHUD3D::set_pixel_size(float p_size)
{
    pixel_size = MAX(p_size, 0.00001f);
    _update_mesh();
}

void InteractiveHUD3D::set_billboard(BillboardMode p_mode)
{
    billboard = p_mode;
    _update_billboard();
}

void InteractiveHUD3D::set_double_sided(bool p_enabled)
{
    double_sided = p_enabled;
    _update_material();
}

void InteractiveHUD3D::set_unshaded(bool p_enabled)
{
    unshaded = p_enabled;
    _update_material();
}

void InteractiveHUD3D::set_always_on_top(bool p_enabled)
{
    always_on_top = p_enabled;
    _update_material();
}

void InteractiveHUD3D::set_modulate(const Color &p_color)
{
    modulate = p_color;
    _update_material();
}

// 🖱️ Interaction

void InteractiveHUD3D::set_interactive(bool p_enabled)
{
    interactive = p_enabled;
    if (!interactive)
    {
        _release_all();
        _set_hovered(false);
        _set_focused(false);
    }
}

void InteractiveHUD3D::set_pointer_source(PointerSource p_source)
{
    if (p_source == pointer_source)
    {
        return;
    }
    _release_all();
    _set_hovered(false);
    pointer_source = p_source;
}

void InteractiveHUD3D::_process(double /*p_delta*/)
{
    _update_billboard();
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    // Chaque frame aussi : la caméra ou le HUD peuvent bouger sous un pointeur immobile.
    Vector3 origin, direction;
    if (!_get_pointer_ray(mouse_position, origin, direction))
    {
        if (!is_captured())
        {
            _set_hovered(false);
        }
        return;
    }
    Vector2 pixel;
    if (_resolve_pointer(origin, direction, pixel) && pixel != last_pixel)
    {
        _send_motion(pixel);
    }
}

void InteractiveHUD3D::_input(const Ref<InputEvent> &p_event)
{
    if (Engine::get_singleton()->is_editor_hint() || !sub_viewport)
    {
        return;
    }

    Ref<InputEventMouse> mouse = p_event;
    if (mouse.is_valid())
    {
        mouse_position = mouse->get_position();
    }

    Ref<InputEventMouseButton> button = p_event;
    if (button.is_valid() && pointer_source != POINTER_SOURCE_MANUAL)
    {
        Vector3 origin, direction;
        const bool has_ray = _get_pointer_ray(button->get_position(), origin, direction);

        // Clic ailleurs que sur ce HUD : il perd le focus clavier.
        if (button->is_pressed() && focused)
        {
            Vector2 pixel;
            float distance;
            if (!has_ray || !_hit(origin, direction, pixel, distance) || _is_behind_another_hud(origin, direction, distance))
            {
                _set_focused(false);
            }
        }

        // Relâchement d'un bouton capturé : capté ici (avant l'interface), jamais perdu.
        if (!button->is_pressed() && (int64_t(captured_buttons) & button_mask(button->get_button_index())))
        {
            _update_billboard();
            Vector2 pixel = last_pixel;
            if (has_ray)
            {
                _resolve_pointer(origin, direction, pixel);
            }
            _on_button(button->get_button_index(), false, pixel);
            _forward_mouse(button, pixel);
            get_viewport()->set_input_as_handled();
        }
        return;
    }

    // Clavier : seulement si un Control du HUD a le focus (sinon les touches restent au jeu).
    Ref<InputEventKey> key = p_event;
    if (key.is_valid() && forward_keyboard && focused && sub_viewport->gui_get_focus_owner())
    {
        sub_viewport->push_input(key, true);
        if (sub_viewport->is_input_handled())
        {
            get_viewport()->set_input_as_handled();
        }
    }
}

void InteractiveHUD3D::_unhandled_input(const Ref<InputEvent> &p_event)
{
    if (Engine::get_singleton()->is_editor_hint() || pointer_source == POINTER_SOURCE_MANUAL)
    {
        return;
    }
    Ref<InputEventMouse> mouse = p_event;
    if (mouse.is_null())
    {
        return;
    }

    _update_billboard();
    Vector3 origin, direction;
    Vector2 pixel;
    if (!_get_pointer_ray(mouse->get_position(), origin, direction) || !_resolve_pointer(origin, direction, pixel))
    {
        return;
    }

    Ref<InputEventMouseButton> button = p_event;
    if (button.is_valid())
    {
        _on_button(button->get_button_index(), button->is_pressed(), pixel);
    }
    _forward_mouse(mouse, pixel);
    get_viewport()->set_input_as_handled();
}

// 🎯 Pointeur manuel

void InteractiveHUD3D::set_pointer_ray(const Vector3 &p_origin, const Vector3 &p_direction)
{
    ERR_FAIL_COND_MSG(p_direction.is_zero_approx(), String(U"[InteractiveHUD3D] Direction de rayon nulle."));
    has_manual_ray = true;
    manual_origin = p_origin;
    manual_direction = p_direction.normalized();

    if (pointer_source != POINTER_SOURCE_MANUAL || !is_inside_tree() || Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    _update_billboard();
    Vector2 pixel;
    if (_resolve_pointer(manual_origin, manual_direction, pixel) && pixel != last_pixel)
    {
        _send_motion(pixel);
    }
}

void InteractiveHUD3D::clear_pointer_ray()
{
    has_manual_ray = false;
    if (pointer_source == POINTER_SOURCE_MANUAL)
    {
        _release_all();
        _set_hovered(false);
    }
}

void InteractiveHUD3D::press_pointer(MouseButton p_button)
{
    ERR_FAIL_COND_MSG(pointer_source != POINTER_SOURCE_MANUAL, String(U"[InteractiveHUD3D] press_pointer() demande pointer_source = POINTER_SOURCE_MANUAL."));
    if (!has_manual_ray || !is_inside_tree())
    {
        return;
    }
    _update_billboard();
    Vector2 pixel;
    if (!_resolve_pointer(manual_origin, manual_direction, pixel))
    {
        _set_focused(false); // appui dans le vide
        return;
    }
    _on_button(p_button, true, pixel);
    _send_button(p_button, true, pixel);
}

void InteractiveHUD3D::release_pointer(MouseButton p_button)
{
    ERR_FAIL_COND_MSG(pointer_source != POINTER_SOURCE_MANUAL, String(U"[InteractiveHUD3D] release_pointer() demande pointer_source = POINTER_SOURCE_MANUAL."));
    const bool captured_button = int64_t(captured_buttons) & button_mask(p_button);
    if (!captured_button && !(is_wheel(p_button) && hovered))
    {
        return;
    }
    Vector2 pixel = last_pixel;
    if (has_manual_ray)
    {
        _resolve_pointer(manual_origin, manual_direction, pixel);
    }
    _on_button(p_button, false, pixel);
    _send_button(p_button, false, pixel);
}

// 🔎 Requêtes

Variant InteractiveHUD3D::ray_to_hud(const Vector3 &p_origin, const Vector3 &p_direction) const
{
    Vector2 pixel;
    float distance;
    return _intersect(p_origin, p_direction.normalized(), true, pixel, distance) ? Variant(pixel) : Variant();
}

Variant InteractiveHUD3D::screen_to_hud(const Vector2 &p_screen_position) const
{
    ERR_FAIL_COND_V(!is_inside_tree(), Variant());
    Camera3D *camera = get_viewport()->get_camera_3d();
    if (!camera)
    {
        return Variant();
    }
    return ray_to_hud(camera->project_ray_origin(p_screen_position), camera->project_ray_normal(p_screen_position));
}

Vector3 InteractiveHUD3D::hud_to_world(const Vector2 &p_pixel) const
{
    ERR_FAIL_NULL_V(mesh_instance, Vector3());
    const Vector2 size = get_quad_size();
    const Vector3 local((p_pixel.x / viewport_size.x - 0.5f) * size.x, (0.5f - p_pixel.y / viewport_size.y) * size.y, 0.0f);
    return mesh_instance->get_global_transform().xform(local);
}

// 📐 Intersection

bool InteractiveHUD3D::_get_pointer_ray(Vector2 p_screen_position, Vector3 &r_origin, Vector3 &r_direction) const
{
    if (!is_inside_tree())
    {
        return false;
    }
    if (pointer_source == POINTER_SOURCE_MANUAL)
    {
        r_origin = manual_origin;
        r_direction = manual_direction;
        return has_manual_ray;
    }
    if (pointer_source == POINTER_SOURCE_SCREEN_CENTER)
    {
        p_screen_position = get_viewport()->get_visible_rect().get_center();
    }
    Camera3D *camera = get_viewport()->get_camera_3d();
    if (!camera)
    {
        return false;
    }
    r_origin = camera->project_ray_origin(p_screen_position);
    r_direction = camera->project_ray_normal(p_screen_position);
    return true;
}

bool InteractiveHUD3D::_intersect(const Vector3 &p_origin, const Vector3 &p_direction, bool p_bounded, Vector2 &r_pixel, float &r_distance) const
{
    if (!mesh_instance || !mesh_instance->is_inside_tree())
    {
        return false;
    }
    // Rayon ramené dans le repère du quad : plan z = 0, face avant vers +Z.
    const Transform3D global = mesh_instance->get_global_transform();
    const Transform3D to_local = global.affine_inverse();
    const Vector3 origin = to_local.xform(p_origin);
    const Vector3 direction = to_local.basis.xform(p_direction);

    if (Math::abs(direction.z) < CMP_EPSILON || (!double_sided && origin.z <= 0.0f))
    {
        return false; // parallèle au plan, ou vu de dos
    }
    const float t = -origin.z / direction.z;
    if (t < 0.0f)
    {
        return false;
    }
    const Vector3 point = origin + direction * t;
    const Vector2 size = get_quad_size();
    if (p_bounded && (Math::abs(point.x) > size.x * 0.5f || Math::abs(point.y) > size.y * 0.5f))
    {
        return false;
    }
    r_pixel = Vector2((point.x / size.x + 0.5f) * viewport_size.x, (0.5f - point.y / size.y) * viewport_size.y);
    r_distance = p_origin.distance_to(global.xform(point));
    return true;
}

bool InteractiveHUD3D::_can_interact() const
{
    return interactive && sub_viewport && is_inside_tree() && is_visible_in_tree() && can_process();
}

bool InteractiveHUD3D::_hit(const Vector3 &p_origin, const Vector3 &p_direction, Vector2 &r_pixel, float &r_distance) const
{
    if (!_can_interact() || !_intersect(p_origin, p_direction, true, r_pixel, r_distance))
    {
        return false;
    }
    if (max_distance > 0.0f && r_distance > max_distance)
    {
        return false;
    }
    return !check_occlusion || !_is_occluded(p_origin, p_origin + p_direction.normalized() * r_distance);
}

bool InteractiveHUD3D::_is_occluded(const Vector3 &p_origin, const Vector3 &p_point) const
{
    Ref<World3D> world = get_world_3d();
    PhysicsDirectSpaceState3D *space = world.is_valid() ? world->get_direct_space_state() : nullptr;
    if (!space)
    {
        return false;
    }
    // Arrêté juste avant le quad : un corps collé au panneau ne l'occulte pas.
    const Vector3 end = p_point - (p_point - p_origin).normalized() * 0.001f;
    Ref<PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(p_origin, end, occlusion_mask);
    return !space->intersect_ray(query).is_empty();
}

bool InteractiveHUD3D::_is_behind_another_hud(const Vector3 &p_origin, const Vector3 &p_direction, float p_distance) const
{
    if (pointer_source == POINTER_SOURCE_MANUAL)
    {
        return false; // chaque HUD manuel a son propre rayon
    }
    const TypedArray<Node> huds = get_tree()->get_nodes_in_group(GROUP);
    for (int64_t i = 0; i < huds.size(); i++)
    {
        const InteractiveHUD3D *other = Object::cast_to<InteractiveHUD3D>(huds[i]);
        if (!other || other == this || other->pointer_source != pointer_source || other->get_viewport() != get_viewport())
        {
            continue;
        }
        if (other->is_captured() && other->_can_interact())
        {
            return true; // le pointeur appartient au HUD qui a capturé un bouton
        }
        Vector2 pixel;
        float distance;
        // À égalité, le premier dans l'arbre l'emporte (ordre de get_nodes_in_group).
        if (other->_hit(p_origin, p_direction, pixel, distance) && (distance < p_distance || (distance == p_distance && !other->is_greater_than(const_cast<InteractiveHUD3D *>(this)))))
        {
            return true;
        }
    }
    return false;
}

// 🖱️ Renvoi des évènements

bool InteractiveHUD3D::_resolve_pointer(const Vector3 &p_origin, const Vector3 &p_direction, Vector2 &r_pixel)
{
    if (is_captured())
    {
        // Bouton capturé : suivi sur le plan, même hors du quad ; le survol est réévalué au relâchement.
        float distance;
        Vector2 pixel;
        if (_intersect(p_origin, p_direction, false, pixel, distance))
        {
            last_pixel = pixel;
        }
        r_pixel = last_pixel;
        return true;
    }

    Vector2 pixel;
    float distance;
    const bool hit = _hit(p_origin, p_direction, pixel, distance) && !_is_behind_another_hud(p_origin, p_direction, distance);
    _set_hovered(hit);
    if (hit)
    {
        r_pixel = pixel;
    }
    return hit;
}

void InteractiveHUD3D::_set_hovered(bool p_hovered)
{
    if (p_hovered == hovered)
    {
        return;
    }
    hovered = p_hovered;
    // À la sortie de l'arbre, le SubViewport (enfant) en est déjà sorti : rien à lui envoyer.
    const bool viewport_ready = sub_viewport && sub_viewport->is_inside_tree();
    if (viewport_ready && hovered)
    {
        // Comme un SubViewportContainer : le SubViewport reçoit le survol.
        sub_viewport->notification(NOTIFICATION_VP_MOUSE_ENTER);
    }
    else if (viewport_ready)
    {
        // VP_MOUSE_EXIT seul n'efface pas le survol des Control : un mouvement hors du SubViewport le fait.
        Ref<InputEventMouseMotion> outside;
        outside.instantiate();
        outside->set_position(Vector2(-1, -1));
        outside->set_global_position(Vector2(-1, -1));
        sub_viewport->push_input(outside, true);
        sub_viewport->notification(NOTIFICATION_VP_MOUSE_EXIT);
    }
    emit_signal(hovered ? "pointer_entered" : "pointer_exited");
}

void InteractiveHUD3D::_set_focused(bool p_focused)
{
    if (p_focused == focused)
    {
        return;
    }
    focused = p_focused;
    if (!focused && sub_viewport && sub_viewport->is_inside_tree())
    {
        sub_viewport->gui_release_focus();
    }
}

void InteractiveHUD3D::_on_button(MouseButton p_button, bool p_pressed, const Vector2 &p_pixel)
{
    if (is_wheel(p_button))
    {
        return; // la molette ne capture rien
    }
    const int64_t mask = button_mask(p_button);
    if (p_pressed)
    {
        captured_buttons = int64_t(captured_buttons) | mask;
        _set_focused(true);
        emit_signal("pointer_pressed", p_pixel, p_button);
    }
    else if (int64_t(captured_buttons) & mask)
    {
        captured_buttons = int64_t(captured_buttons) & ~mask;
        emit_signal("pointer_released", p_pixel, p_button);
    }
}

void InteractiveHUD3D::_forward_mouse(const Ref<InputEventMouse> &p_event, const Vector2 &p_pixel)
{
    Ref<InputEventMouse> event = p_event->duplicate();
    event->set_position(p_pixel);
    event->set_global_position(p_pixel);
    Ref<InputEventMouseMotion> motion = event;
    if (motion.is_valid())
    {
        const Vector2 relative = last_pixel.x >= 0.0f ? p_pixel - last_pixel : Vector2();
        motion->set_relative(relative);
        motion->set_screen_relative(relative);
    }
    last_pixel = p_pixel;
    if (sub_viewport->is_inside_tree())
    {
        sub_viewport->push_input(event, true);
    }
}

void InteractiveHUD3D::_send_motion(const Vector2 &p_pixel)
{
    Ref<InputEventMouseMotion> motion;
    motion.instantiate();
    motion->set_button_mask(captured_buttons);
    _forward_mouse(motion, p_pixel);
}

void InteractiveHUD3D::_send_button(MouseButton p_button, bool p_pressed, const Vector2 &p_pixel)
{
    Ref<InputEventMouseButton> button;
    button.instantiate();
    button->set_button_index(p_button);
    button->set_pressed(p_pressed);
    button->set_button_mask(captured_buttons);
    button->set_factor(1.0f);
    _forward_mouse(button, p_pixel);
}

void InteractiveHUD3D::_release_all()
{
    if (!is_captured() || !sub_viewport)
    {
        captured_buttons = 0;
        return;
    }
    for (int button = MOUSE_BUTTON_LEFT; button <= MOUSE_BUTTON_XBUTTON2; button++)
    {
        if (int64_t(captured_buttons) & button_mask(MouseButton(button)))
        {
            _on_button(MouseButton(button), false, last_pixel);
            _send_button(MouseButton(button), false, last_pixel);
        }
    }
}

// 🔗 Liaisons

void InteractiveHUD3D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_content_scene", "scene"), &InteractiveHUD3D::set_content_scene);
    ClassDB::bind_method(D_METHOD("get_content_scene"), &InteractiveHUD3D::get_content_scene);
    ClassDB::bind_method(D_METHOD("set_content", "content"), &InteractiveHUD3D::set_content);
    ClassDB::bind_method(D_METHOD("get_content"), &InteractiveHUD3D::get_content);
    ClassDB::bind_method(D_METHOD("get_sub_viewport"), &InteractiveHUD3D::get_sub_viewport);
    ClassDB::bind_method(D_METHOD("set_viewport_size", "size"), &InteractiveHUD3D::set_viewport_size);
    ClassDB::bind_method(D_METHOD("get_viewport_size"), &InteractiveHUD3D::get_viewport_size);
    ClassDB::bind_method(D_METHOD("set_transparent_background", "enabled"), &InteractiveHUD3D::set_transparent_background);
    ClassDB::bind_method(D_METHOD("get_transparent_background"), &InteractiveHUD3D::get_transparent_background);

    ClassDB::bind_method(D_METHOD("set_pixel_size", "size"), &InteractiveHUD3D::set_pixel_size);
    ClassDB::bind_method(D_METHOD("get_pixel_size"), &InteractiveHUD3D::get_pixel_size);
    ClassDB::bind_method(D_METHOD("get_quad_size"), &InteractiveHUD3D::get_quad_size);
    ClassDB::bind_method(D_METHOD("set_billboard", "mode"), &InteractiveHUD3D::set_billboard);
    ClassDB::bind_method(D_METHOD("get_billboard"), &InteractiveHUD3D::get_billboard);
    ClassDB::bind_method(D_METHOD("set_double_sided", "enabled"), &InteractiveHUD3D::set_double_sided);
    ClassDB::bind_method(D_METHOD("get_double_sided"), &InteractiveHUD3D::get_double_sided);
    ClassDB::bind_method(D_METHOD("set_unshaded", "enabled"), &InteractiveHUD3D::set_unshaded);
    ClassDB::bind_method(D_METHOD("get_unshaded"), &InteractiveHUD3D::get_unshaded);
    ClassDB::bind_method(D_METHOD("set_always_on_top", "enabled"), &InteractiveHUD3D::set_always_on_top);
    ClassDB::bind_method(D_METHOD("get_always_on_top"), &InteractiveHUD3D::get_always_on_top);
    ClassDB::bind_method(D_METHOD("set_modulate", "color"), &InteractiveHUD3D::set_modulate);
    ClassDB::bind_method(D_METHOD("get_modulate"), &InteractiveHUD3D::get_modulate);

    ClassDB::bind_method(D_METHOD("set_interactive", "enabled"), &InteractiveHUD3D::set_interactive);
    ClassDB::bind_method(D_METHOD("is_interactive"), &InteractiveHUD3D::is_interactive);
    ClassDB::bind_method(D_METHOD("set_pointer_source", "source"), &InteractiveHUD3D::set_pointer_source);
    ClassDB::bind_method(D_METHOD("get_pointer_source"), &InteractiveHUD3D::get_pointer_source);
    ClassDB::bind_method(D_METHOD("set_max_distance", "distance"), &InteractiveHUD3D::set_max_distance);
    ClassDB::bind_method(D_METHOD("get_max_distance"), &InteractiveHUD3D::get_max_distance);
    ClassDB::bind_method(D_METHOD("set_forward_keyboard", "enabled"), &InteractiveHUD3D::set_forward_keyboard);
    ClassDB::bind_method(D_METHOD("get_forward_keyboard"), &InteractiveHUD3D::get_forward_keyboard);
    ClassDB::bind_method(D_METHOD("set_check_occlusion", "enabled"), &InteractiveHUD3D::set_check_occlusion);
    ClassDB::bind_method(D_METHOD("get_check_occlusion"), &InteractiveHUD3D::get_check_occlusion);
    ClassDB::bind_method(D_METHOD("set_occlusion_mask", "mask"), &InteractiveHUD3D::set_occlusion_mask);
    ClassDB::bind_method(D_METHOD("get_occlusion_mask"), &InteractiveHUD3D::get_occlusion_mask);

    ClassDB::bind_method(D_METHOD("set_pointer_ray", "origin", "direction"), &InteractiveHUD3D::set_pointer_ray);
    ClassDB::bind_method(D_METHOD("clear_pointer_ray"), &InteractiveHUD3D::clear_pointer_ray);
    ClassDB::bind_method(D_METHOD("press_pointer", "button"), &InteractiveHUD3D::press_pointer, DEFVAL(MOUSE_BUTTON_LEFT));
    ClassDB::bind_method(D_METHOD("release_pointer", "button"), &InteractiveHUD3D::release_pointer, DEFVAL(MOUSE_BUTTON_LEFT));

    ClassDB::bind_method(D_METHOD("is_hovered"), &InteractiveHUD3D::is_hovered);
    ClassDB::bind_method(D_METHOD("is_captured"), &InteractiveHUD3D::is_captured);
    ClassDB::bind_method(D_METHOD("is_focused"), &InteractiveHUD3D::is_focused);
    ClassDB::bind_method(D_METHOD("get_pointer_position"), &InteractiveHUD3D::get_pointer_position);
    ClassDB::bind_method(D_METHOD("ray_to_hud", "origin", "direction"), &InteractiveHUD3D::ray_to_hud);
    ClassDB::bind_method(D_METHOD("screen_to_hud", "screen_position"), &InteractiveHUD3D::screen_to_hud);
    ClassDB::bind_method(D_METHOD("hud_to_world", "pixel"), &InteractiveHUD3D::hud_to_world);

    BIND_ENUM_CONSTANT(POINTER_SOURCE_MOUSE);
    BIND_ENUM_CONSTANT(POINTER_SOURCE_SCREEN_CENTER);
    BIND_ENUM_CONSTANT(POINTER_SOURCE_MANUAL);
    BIND_ENUM_CONSTANT(BILLBOARD_DISABLED);
    BIND_ENUM_CONSTANT(BILLBOARD_ENABLED);
    BIND_ENUM_CONSTANT(BILLBOARD_Y);

    ADD_GROUP(U"🧩 Contenu", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "content_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_content_scene", "get_content_scene");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "viewport_size", PROPERTY_HINT_NONE, "suffix:px"), "set_viewport_size", "get_viewport_size");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "transparent_background"), "set_transparent_background", "get_transparent_background");
    ADD_GROUP(U"🎨 Affichage", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pixel_size", PROPERTY_HINT_RANGE, "0.0001,0.1,0.0001,or_greater,suffix:m"), "set_pixel_size", "get_pixel_size");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "billboard", PROPERTY_HINT_ENUM, "Disabled,Enabled,Y-Billboard"), "set_billboard", "get_billboard");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "double_sided"), "set_double_sided", "get_double_sided");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "unshaded"), "set_unshaded", "get_unshaded");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "always_on_top"), "set_always_on_top", "get_always_on_top");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "modulate"), "set_modulate", "get_modulate");
    ADD_GROUP(U"🖱️ Interaction", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "interactive"), "set_interactive", "is_interactive");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "pointer_source", PROPERTY_HINT_ENUM, "Mouse,Screen Center,Manual"), "set_pointer_source", "get_pointer_source");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0,100,0.01,or_greater,suffix:m"), "set_max_distance", "get_max_distance");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "forward_keyboard"), "set_forward_keyboard", "get_forward_keyboard");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "check_occlusion"), "set_check_occlusion", "get_check_occlusion");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "occlusion_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_occlusion_mask", "get_occlusion_mask");

    ADD_SIGNAL(MethodInfo("pointer_entered"));
    ADD_SIGNAL(MethodInfo("pointer_exited"));
    ADD_SIGNAL(MethodInfo("pointer_pressed", PropertyInfo(Variant::VECTOR2, "position"), PropertyInfo(Variant::INT, "button_index")));
    ADD_SIGNAL(MethodInfo("pointer_released", PropertyInfo(Variant::VECTOR2, "position"), PropertyInfo(Variant::INT, "button_index")));
}
