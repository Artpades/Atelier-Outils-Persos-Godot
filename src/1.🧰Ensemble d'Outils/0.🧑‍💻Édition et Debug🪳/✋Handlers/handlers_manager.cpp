#include "handlers_manager.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/⚙️Générateur de Gestionnaire/manager_builder_api.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos_api.hpp"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
// ManagerBuilder pour le singleton, Gizmos pour dessiner les handles : sans eux, le manager se désactive.
const modules::Dependency REQUIRED_MODULES[] = {manager_builder_api::DEPENDENCY, gizmos_2d_api::DEPENDENCY, gizmos_3d_api::DEPENDENCY};
} // namespace

HandlersManager::HandlersManager() {}
HandlersManager::~HandlersManager() {}

void HandlersManager::_bind_methods()
{
    ClassDB::bind_static_method("HandlersManager", D_METHOD("get_singleton"), &HandlersManager::get_singleton);

    ClassDB::bind_method(D_METHOD("register_handle_2d", "id", "rect", "viewport", "default_color", "hover_color"), &HandlersManager::register_handle_2d, DEFVAL(Color(1.0f, 1.0f, 1.0f, 1.0f)),
                         DEFVAL(Color(1.0f, 0.9f, 0.1f, 1.0f)));
    ClassDB::bind_method(D_METHOD("unregister_handle_2d", "id"), &HandlersManager::unregister_handle_2d);
    ClassDB::bind_method(D_METHOD("set_handle_2d_rect", "id", "rect"), &HandlersManager::set_handle_2d_rect);
    ClassDB::bind_method(D_METHOD("is_handle_2d_registered", "id"), &HandlersManager::is_handle_2d_registered);
    ClassDB::bind_method(D_METHOD("is_handle_2d_hovered", "id"), &HandlersManager::is_handle_2d_hovered);
    ClassDB::bind_method(D_METHOD("is_handle_2d_dragging", "id"), &HandlersManager::is_handle_2d_dragging);
    ClassDB::bind_method(D_METHOD("get_handle_2d_rect", "id"), &HandlersManager::get_handle_2d_rect);

    ClassDB::bind_method(D_METHOD("register_handle_3d", "id", "aabb_world", "viewport", "camera", "default_color", "hover_color"), &HandlersManager::register_handle_3d, DEFVAL(nullptr),
                         DEFVAL(Color(1.0f, 1.0f, 1.0f, 1.0f)), DEFVAL(Color(1.0f, 0.9f, 0.1f, 1.0f)));
    ClassDB::bind_method(D_METHOD("unregister_handle_3d", "id"), &HandlersManager::unregister_handle_3d);
    ClassDB::bind_method(D_METHOD("set_handle_3d_aabb", "id", "aabb_world"), &HandlersManager::set_handle_3d_aabb);
    ClassDB::bind_method(D_METHOD("is_handle_3d_registered", "id"), &HandlersManager::is_handle_3d_registered);
    ClassDB::bind_method(D_METHOD("is_handle_3d_hovered", "id"), &HandlersManager::is_handle_3d_hovered);
    ClassDB::bind_method(D_METHOD("is_handle_3d_dragging", "id"), &HandlersManager::is_handle_3d_dragging);
    ClassDB::bind_method(D_METHOD("get_handle_3d_aabb", "id"), &HandlersManager::get_handle_3d_aabb);

    ADD_SIGNAL(MethodInfo("handle_2d_hover_entered", PropertyInfo(Variant::STRING_NAME, "id")));
    ADD_SIGNAL(MethodInfo("handle_2d_hover_exited", PropertyInfo(Variant::STRING_NAME, "id")));
    ADD_SIGNAL(MethodInfo("handle_3d_hover_entered", PropertyInfo(Variant::STRING_NAME, "id")));
    ADD_SIGNAL(MethodInfo("handle_3d_hover_exited", PropertyInfo(Variant::STRING_NAME, "id")));
    ADD_SIGNAL(MethodInfo("handle_2d_moved", PropertyInfo(Variant::STRING_NAME, "id"), PropertyInfo(Variant::VECTOR2, "position")));
    ADD_SIGNAL(MethodInfo("handle_3d_moved", PropertyInfo(Variant::STRING_NAME, "id"), PropertyInfo(Variant::VECTOR3, "position")));
}

void HandlersManager::_enter_tree()
{
    if (!modules::require("HandlersManager", REQUIRED_MODULES))
    {
        set_process_mode(PROCESS_MODE_DISABLED);
        return;
    }
    // S'enregistre comme instance canonique du singleton (ou se détruit si un doublon existe déjà).
    manager_builder_api::init_instance("HandlersManager", this);
    set_process(true);
}

void HandlersManager::_exit_tree()
{
    // Libère tous les RID de gizmos encore actifs pour éviter une fuite côté RenderingServer
    // (les handles eux-mêmes ne se libèrent jamais tout seuls, unregister_handle_* n'aura
    // pas été appelé pour chacun si le manager quitte l'arbre avant ses "clients").
    for (KeyValue<StringName, Handle2D> &entry : _handles_2d)
    {
        if (entry.value.gizmo_rid.is_valid())
        {
            gizmos_2d_api::free_gizmos(entry.value.gizmo_rid);
        }
    }
    for (KeyValue<StringName, Handle3D> &entry : _handles_3d)
    {
        if (entry.value.gizmo_rid.is_valid())
        {
            gizmos_3d_api::free_3D_gizmo(entry.value.gizmo_rid);
        }
    }
    _handles_2d.clear();
    _handles_3d.clear();

    manager_builder_api::unregister_instance("HandlersManager", this);
}

HandlersManager *HandlersManager::get_singleton()
{
    if (!modules::require("HandlersManager", REQUIRED_MODULES))
    {
        return nullptr;
    }
    return Object::cast_to<HandlersManager>(manager_builder_api::make_getter("HandlersManager"));
}

PackedStringArray HandlersManager::_get_configuration_warnings() const { return modules::missing_warnings("HandlersManager", REQUIRED_MODULES); }

bool HandlersManager::_is_viewport_effectively_visible(Viewport *p_viewport)
{
    if (!p_viewport)
    {
        return false;
    }

    if (Window *w = Object::cast_to<Window>(p_viewport))
    {
        return w->is_visible();
    }

    // SubViewport : pas de propriété "visible" propre. On remonte l'arbre à la recherche
    // d'un ancêtre Control (typiquement un SubViewportContainer) dont la visibilité
    // effective indique si ce viewport est réellement affiché à l'écran.
    Node *parent = p_viewport->get_parent();
    while (parent)
    {
        if (Control *ctrl = Object::cast_to<Control>(parent))
        {
            return ctrl->is_visible_in_tree();
        }
        parent = parent->get_parent();
    }

    // Aucun ancêtre Control trouvé : impossible de déterminer l'état, on laisse passer
    // plutôt que de bloquer un cas d'usage non prévu (fail-open).
    return true;
}

void HandlersManager::_process(double p_delta)
{
    _process_2d_handles();
    _process_3d_handles();
}

void HandlersManager::_redraw_handle_2d(Handle2D &p_handle)
{
    if (!p_handle.gizmo_rid.is_valid())
    {
        return;
    }
    gizmos_2d_api::draw_2D_rectangle(p_handle.gizmo_rid, p_handle.rect, p_handle.hovered ? p_handle.hover_color : p_handle.default_color, true);
}

void HandlersManager::_redraw_handle_3d(Handle3D &p_handle)
{
    if (!p_handle.gizmo_rid.is_valid())
    {
        return;
    }
    gizmos_3d_api::draw_3D_cube(p_handle.gizmo_rid, p_handle.aabb.get_center(), p_handle.aabb.size, p_handle.hovered ? p_handle.hover_color : p_handle.default_color, false);
}

void HandlersManager::_process_2d_handles()
{
    bool mouse_pressed = Input::get_singleton()->is_mouse_button_pressed(MOUSE_BUTTON_LEFT);

    for (KeyValue<StringName, Handle2D> &entry : _handles_2d)
    {
        Handle2D &handle = entry.value;

        Viewport *vp = Object::cast_to<Viewport>(UtilityFunctions::instance_from_id((int64_t)handle.viewport_id));
        if (!vp || !_is_viewport_effectively_visible(vp))
        {
            // Viewport détruit, ou existant mais pas affiché à l'écran (fenêtre masquée,
            // conteneur caché) : pas de détection tant qu'il n'est pas visible.
            continue;
        }

        // Création paresseuse : au moment de register_handle_2d, le viewport peut être
        // valide mais pas encore inséré dans l'arbre (ex: Window pas encore ajoutée).
        if (!handle.gizmo_rid.is_valid())
        {
            handle.gizmo_rid = gizmos_2d_api::create_2D_gizmos(vp);
            _redraw_handle_2d(handle);
        }

        Vector2 mouse_pos = vp->get_mouse_position();
        bool now_hovered = handle.rect.has_point(mouse_pos);

        if (!handle.dragging && now_hovered && mouse_pressed)
        {
            handle.dragging = true;
            handle.drag_offset = mouse_pos - handle.rect.position;
        }
        else if (handle.dragging && !mouse_pressed)
        {
            handle.dragging = false;
        }

        if (handle.dragging)
        {
            handle.rect.position = mouse_pos - handle.drag_offset;
            _redraw_handle_2d(handle);
            emit_signal("handle_2d_moved", entry.key, handle.rect.position);
        }

        if (now_hovered != handle.hovered)
        {
            handle.hovered = now_hovered;
            _redraw_handle_2d(handle);
            emit_signal(now_hovered ? "handle_2d_hover_entered" : "handle_2d_hover_exited", entry.key);
        }
    }
}

void HandlersManager::_process_3d_handles()
{
    bool mouse_pressed = Input::get_singleton()->is_mouse_button_pressed(MOUSE_BUTTON_LEFT);

    for (KeyValue<StringName, Handle3D> &entry : _handles_3d)
    {
        Handle3D &handle = entry.value;

        Viewport *vp = Object::cast_to<Viewport>(UtilityFunctions::instance_from_id((int64_t)handle.viewport_id));
        if (!vp || !_is_viewport_effectively_visible(vp))
        {
            continue;
        }

        // Caméra explicite si fournie à l'enregistrement, sinon la caméra active du viewport.
        Camera3D *cam = handle.camera_id.is_valid() ? Object::cast_to<Camera3D>(UtilityFunctions::instance_from_id((int64_t)handle.camera_id)) : vp->get_camera_3d();
        if (!cam)
        {
            continue;
        }

        if (!handle.gizmo_rid.is_valid())
        {
            handle.gizmo_rid = gizmos_3d_api::create_3D_gizmo(vp);
            _redraw_handle_3d(handle);
        }

        Vector2 mouse_pos = vp->get_mouse_position();
        Vector3 ray_origin = cam->project_ray_origin(mouse_pos);
        Vector3 ray_dir = cam->project_ray_normal(mouse_pos);

        bool now_hovered = handle.aabb.intersects_ray(ray_origin, ray_dir);

        if (!handle.dragging && now_hovered && mouse_pressed)
        {
            handle.dragging = true;
            // Plan figé au clic : passe par le centre de l'AABB, orienté face à la caméra.
            // La normale n'a pas besoin d'être exactement "vers" la caméra pour que
            // l'intersection soit correcte, juste non-nulle et non-parallèle au rayon.
            Vector3 cam_forward = -cam->get_global_transform().basis.get_column(2);
            handle.drag_plane = Plane(cam_forward, handle.aabb.get_center());
        }
        else if (handle.dragging && !mouse_pressed)
        {
            handle.dragging = false;
        }

        if (handle.dragging)
        {
            Vector3 hit_point;
            bool hit = handle.drag_plane.intersects_ray(ray_origin, ray_dir, &hit_point);
            if (hit)
            {
                handle.aabb.position = hit_point - handle.aabb.size * 0.5f;
                _redraw_handle_3d(handle);
                emit_signal("handle_3d_moved", entry.key, hit_point);
            }
            // Si hit est faux (rayon parallèle au plan), on garde la dernière position :
            // pas de mise à jour ce frame plutôt qu'un saut incohérent.
        }

        if (now_hovered != handle.hovered)
        {
            handle.hovered = now_hovered;
            _redraw_handle_3d(handle);
            emit_signal(now_hovered ? "handle_3d_hover_entered" : "handle_3d_hover_exited", entry.key);
        }
    }
}

void HandlersManager::register_handle_2d(const StringName &p_id, const Rect2 &p_rect, Viewport *p_viewport, const Color &p_default_color, const Color &p_hover_color)
{
    if (!p_viewport)
    {
        print_line_rich("[color=red]HandlersManager::register_handle_2d — Viewport nul pour '" + String(p_id) + "'[/color]");
        return;
    }
    Handle2D h;
    h.rect = p_rect;
    h.default_color = p_default_color;
    h.hover_color = p_hover_color;
    h.viewport_id = p_viewport->get_instance_id();
    // h.gizmo_rid reste invalide : créé au premier _process (cf. _process_2d_handles).
    _handles_2d[p_id] = h;
}

void HandlersManager::unregister_handle_2d(const StringName &p_id)
{
    HashMap<StringName, Handle2D>::Iterator it = _handles_2d.find(p_id);
    if (it)
    {
        if (it->value.gizmo_rid.is_valid())
        {
            gizmos_2d_api::free_gizmos(it->value.gizmo_rid);
        }
        _handles_2d.erase(p_id);
    }
}

void HandlersManager::set_handle_2d_rect(const StringName &p_id, const Rect2 &p_rect)
{
    HashMap<StringName, Handle2D>::Iterator it = _handles_2d.find(p_id);
    if (it)
    {
        it->value.rect = p_rect;
        _redraw_handle_2d(it->value); // no-op si le gizmo n'est pas encore créé
    }
}

bool HandlersManager::is_handle_2d_registered(const StringName &p_id) const { return _handles_2d.has(p_id); }

bool HandlersManager::is_handle_2d_hovered(const StringName &p_id) const
{
    HashMap<StringName, Handle2D>::ConstIterator it = _handles_2d.find(p_id);
    return it ? it->value.hovered : false;
}

bool HandlersManager::is_handle_2d_dragging(const StringName &p_id) const
{
    HashMap<StringName, Handle2D>::ConstIterator it = _handles_2d.find(p_id);
    return it ? it->value.dragging : false;
}

Rect2 HandlersManager::get_handle_2d_rect(const StringName &p_id) const
{
    HashMap<StringName, Handle2D>::ConstIterator it = _handles_2d.find(p_id);
    return it ? it->value.rect : Rect2();
}

void HandlersManager::register_handle_3d(const StringName &p_id, const AABB &p_aabb_world, Viewport *p_viewport, Camera3D *p_camera, const Color &p_default_color, const Color &p_hover_color)
{
    if (!p_viewport)
    {
        print_line_rich("[color=red]HandlersManager::register_handle_3d — Viewport nul pour '" + String(p_id) + "'[/color]");
        return;
    }
    Handle3D h;
    h.aabb = p_aabb_world;
    h.default_color = p_default_color;
    h.hover_color = p_hover_color;
    h.viewport_id = p_viewport->get_instance_id();
    if (p_camera)
    {
        h.camera_id = p_camera->get_instance_id();
    }
    _handles_3d[p_id] = h;
}

void HandlersManager::unregister_handle_3d(const StringName &p_id)
{
    HashMap<StringName, Handle3D>::Iterator it = _handles_3d.find(p_id);
    if (it)
    {
        if (it->value.gizmo_rid.is_valid())
        {
            gizmos_3d_api::free_3D_gizmo(it->value.gizmo_rid);
        }
        _handles_3d.erase(p_id);
    }
}

void HandlersManager::set_handle_3d_aabb(const StringName &p_id, const AABB &p_aabb_world)
{
    HashMap<StringName, Handle3D>::Iterator it = _handles_3d.find(p_id);
    if (it)
    {
        it->value.aabb = p_aabb_world;
        _redraw_handle_3d(it->value);
    }
}

bool HandlersManager::is_handle_3d_registered(const StringName &p_id) const { return _handles_3d.has(p_id); }

bool HandlersManager::is_handle_3d_hovered(const StringName &p_id) const
{
    HashMap<StringName, Handle3D>::ConstIterator it = _handles_3d.find(p_id);
    return it ? it->value.hovered : false;
}

bool HandlersManager::is_handle_3d_dragging(const StringName &p_id) const
{
    HashMap<StringName, Handle3D>::ConstIterator it = _handles_3d.find(p_id);
    return it ? it->value.dragging : false;
}

AABB HandlersManager::get_handle_3d_aabb(const StringName &p_id) const
{
    HashMap<StringName, Handle3D>::ConstIterator it = _handles_3d.find(p_id);
    return it ? it->value.aabb : AABB();
}