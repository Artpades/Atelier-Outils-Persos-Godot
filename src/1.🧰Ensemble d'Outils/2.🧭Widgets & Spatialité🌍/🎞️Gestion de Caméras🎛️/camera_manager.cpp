#include "camera_manager.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/⚙️Générateur de Gestionnaire/manager_builder_access.hpp"

#include <godot_cpp/classes/callback_tweener.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

ObjectID CameraManager::instance_id;

namespace
{
// Libère une caméra transitoire jamais rattachée (sinon c'est son parent qui la libère).
void free_if_orphan(const ObjectID &p_id)
{
    Node *node = Object::cast_to<Node>(ObjectDB::get_instance(p_id));
    if (node && !node->get_parent())
    {
        memdelete(node);
    }
}
} // namespace

CameraManager::~CameraManager()
{
    free_if_orphan(transitioner_2d_id);
    free_if_orphan(transitioner_3d_id);
}

// 🛠️ Singleton

CameraManager *CameraManager::get_instance() { return manager_builder::get<CameraManager>(instance_id); }

void CameraManager::_enter_tree()
{
    // Plusieurs CameraManager possibles (Autoload + nœud placé en scène) : seul le premier est conservé.
    manager_builder::init(instance_id, this);
}

void CameraManager::_exit_tree() { manager_builder::release(instance_id, this); }

// 🧩 Outils communs

void CameraManager::_attach_transitioner(Node *p_transitioner, Node *p_target)
{
    if (Node *parent = p_transitioner->get_parent())
    {
        parent->remove_child(p_transitioner);
    }
    // Enfant interne : absent de get_children() et jamais sauvegardé avec la scène de la cible.
    p_target->add_child(p_transitioner, false, INTERNAL_MODE_FRONT);
}

void CameraManager::_kill_tween(Ref<Tween> &r_tween)
{
    if (r_tween.is_valid() && r_tween->is_valid())
    {
        r_tween->kill();
    }
    r_tween.unref();
}

// 🎥 2D

Camera2D *CameraManager::_get_transitioner_2d()
{
    Camera2D *camera = Object::cast_to<Camera2D>(ObjectDB::get_instance(transitioner_2d_id));
    if (!camera)
    {
        camera = memnew(Camera2D);
        camera->set_name("CameraTransitioner2D");
        camera->set_enabled(false);
        // top_level : la transform ne dépend pas de la caméra parente, seulement du Tween.
        camera->set_as_top_level(true);
        transitioner_2d_id = ObjectID(camera->get_instance_id());
    }
    return camera;
}

void CameraManager::switch_2d_camera(Camera2D *p_new_camera, bool p_ease_transition, double p_duration, Tween::TransitionType p_transition_type, Tween::EaseType p_ease_type)
{
    ERR_FAIL_NULL(p_new_camera);
    ERR_FAIL_COND_MSG(!p_new_camera->is_inside_tree(), String(U"CameraManager : la caméra 2D cible n'est pas dans l'arbre de scène."));

    Viewport *viewport = p_new_camera->get_viewport();
    Camera2D *transitioner = _get_transitioner_2d();
    const bool interrupting = transitioning_2d;

    // Une transition en cours est interrompue : la caméra transitoire garde l'état atteint.
    _kill_tween(active_tween_2d);

    Camera2D *previous_camera = interrupting ? nullptr : get_current_camera_2d(viewport);

    // Même caméra que la courante : rien à animer.
    if (!interrupting && previous_camera == p_new_camera)
    {
        emit_signal("current_camera_2d_switched", p_new_camera);
        return;
    }

    // Switch instantané (demandé, ou aucune caméra de départ pour la transition).
    if (!p_ease_transition || (!interrupting && !previous_camera))
    {
        transitioner->set_enabled(false);
        transitioning_2d = false;
        _disable_all_cameras_2d(viewport);
        p_new_camera->set_enabled(true);
        p_new_camera->make_current();
        emit_signal("current_camera_2d_switched", p_new_camera);
        return;
    }

    transitioning_2d = true;
    _disable_all_cameras_2d(viewport);
    _attach_transitioner(transitioner, p_new_camera);

    transitioner->set_ignore_rotation(p_new_camera->is_ignoring_rotation());
    transitioner->set_anchor_mode(p_new_camera->get_anchor_mode());
    if (previous_camera)
    {
        transitioner->set_global_transform(previous_camera->get_global_transform());
        transitioner->set_zoom(previous_camera->get_zoom());
        transitioner->set_offset(previous_camera->get_offset());
    }
    transitioner->set_enabled(true);
    transitioner->make_current();

    const double duration = p_duration < 0.0 ? transition_duration_2d : p_duration;
    active_tween_2d = create_tween();
    active_tween_2d->set_parallel(true);
    // global_transform plutôt que global_rotation : l'interpolation prend le plus court chemin angulaire.
    active_tween_2d->tween_property(transitioner, "global_transform", p_new_camera->get_global_transform(), duration)->set_trans(p_transition_type)->set_ease(p_ease_type);
    active_tween_2d->tween_property(transitioner, "zoom", p_new_camera->get_zoom(), duration)->set_trans(p_transition_type)->set_ease(p_ease_type);
    active_tween_2d->tween_property(transitioner, "offset", p_new_camera->get_offset(), duration)->set_trans(p_transition_type)->set_ease(p_ease_type);
    // ObjectID lié plutôt que le pointeur : la caméra cible peut être libérée pendant la transition.
    active_tween_2d->chain()->tween_callback(callable_mp(this, &CameraManager::_on_transition_2d_complete).bind(p_new_camera->get_instance_id()));
}

void CameraManager::_on_transition_2d_complete(uint64_t p_camera_id)
{
    if (Camera2D *transitioner = Object::cast_to<Camera2D>(ObjectDB::get_instance(transitioner_2d_id)))
    {
        transitioner->set_enabled(false);
    }

    Camera2D *new_camera = Object::cast_to<Camera2D>(ObjectDB::get_instance(ObjectID(p_camera_id)));
    if (new_camera)
    {
        new_camera->set_enabled(true);
        new_camera->make_current();
        emit_signal("current_camera_2d_switched", new_camera);
    }

    transitioning_2d = false;
    active_tween_2d.unref();
}

Camera2D *CameraManager::get_current_camera_2d(Viewport *p_viewport) { return p_viewport ? p_viewport->get_camera_2d() : nullptr; }

void CameraManager::_disable_all_cameras_2d(Viewport *p_viewport)
{
    if (!p_viewport)
    {
        return;
    }
    TypedArray<Node> cameras = p_viewport->find_children("*", "Camera2D", true, false);
    for (int64_t i = 0; i < cameras.size(); i++)
    {
        if (Camera2D *camera = Object::cast_to<Camera2D>(cameras[i]))
        {
            camera->set_enabled(false);
        }
    }
}

// 🎥 3D

Camera3D *CameraManager::_get_transitioner_3d()
{
    Camera3D *camera = Object::cast_to<Camera3D>(ObjectDB::get_instance(transitioner_3d_id));
    if (!camera)
    {
        camera = memnew(Camera3D);
        camera->set_name("CameraTransitioner3D");
        camera->set_current(false);
        camera->set_as_top_level(true);
        transitioner_3d_id = ObjectID(camera->get_instance_id());
    }
    return camera;
}

void CameraManager::switch_3d_camera(Camera3D *p_new_camera, bool p_ease_transition, double p_duration, Tween::TransitionType p_transition_type, Tween::EaseType p_ease_type)
{
    ERR_FAIL_NULL(p_new_camera);
    ERR_FAIL_COND_MSG(!p_new_camera->is_inside_tree(), String(U"CameraManager : la caméra 3D cible n'est pas dans l'arbre de scène."));

    Viewport *viewport = p_new_camera->get_viewport();
    Camera3D *transitioner = _get_transitioner_3d();
    const bool interrupting = transitioning_3d;

    _kill_tween(active_tween_3d);

    Camera3D *previous_camera = interrupting ? nullptr : get_current_camera_3d(viewport);

    if (!interrupting && previous_camera == p_new_camera)
    {
        emit_signal("current_camera_3d_switched", p_new_camera);
        return;
    }

    if (!p_ease_transition || (!interrupting && !previous_camera))
    {
        transitioner->set_current(false);
        transitioning_3d = false;
        _disable_all_cameras_3d(viewport);
        p_new_camera->set_current(true);
        emit_signal("current_camera_3d_switched", p_new_camera);
        return;
    }

    transitioning_3d = true;
    _disable_all_cameras_3d(viewport);
    _attach_transitioner(transitioner, p_new_camera);

    transitioner->set_projection(p_new_camera->get_projection());
    if (previous_camera)
    {
        transitioner->set_global_transform(previous_camera->get_global_transform());
        transitioner->set_fov(previous_camera->get_fov());
        transitioner->set_size(previous_camera->get_size());
        transitioner->set_near(previous_camera->get_near());
        transitioner->set_far(previous_camera->get_far());
    }
    transitioner->set_current(true);

    const double duration = p_duration < 0.0 ? transition_duration_3d : p_duration;
    active_tween_3d = create_tween();
    active_tween_3d->set_parallel(true);
    // global_transform : rotation interpolée par slerp (pas de tour complet entre -179° et 179°).
    active_tween_3d->tween_property(transitioner, "global_transform", p_new_camera->get_global_transform(), duration)->set_trans(p_transition_type)->set_ease(p_ease_type);
    active_tween_3d->tween_property(transitioner, "fov", p_new_camera->get_fov(), duration)->set_trans(p_transition_type)->set_ease(p_ease_type);
    // size : caméra orthogonale
    active_tween_3d->tween_property(transitioner, "size", p_new_camera->get_size(), duration)->set_trans(p_transition_type)->set_ease(p_ease_type);
    active_tween_3d->tween_property(transitioner, "near", p_new_camera->get_near(), duration)->set_trans(p_transition_type)->set_ease(p_ease_type);
    active_tween_3d->tween_property(transitioner, "far", p_new_camera->get_far(), duration)->set_trans(p_transition_type)->set_ease(p_ease_type);
    active_tween_3d->chain()->tween_callback(callable_mp(this, &CameraManager::_on_transition_3d_complete).bind(p_new_camera->get_instance_id()));
}

void CameraManager::_on_transition_3d_complete(uint64_t p_camera_id)
{
    if (Camera3D *transitioner = Object::cast_to<Camera3D>(ObjectDB::get_instance(transitioner_3d_id)))
    {
        transitioner->set_current(false);
    }

    Camera3D *new_camera = Object::cast_to<Camera3D>(ObjectDB::get_instance(ObjectID(p_camera_id)));
    if (new_camera)
    {
        new_camera->set_current(true);
        emit_signal("current_camera_3d_switched", new_camera);
    }

    transitioning_3d = false;
    active_tween_3d.unref();
}

Camera3D *CameraManager::get_current_camera_3d(Viewport *p_viewport) { return p_viewport ? p_viewport->get_camera_3d() : nullptr; }

void CameraManager::_disable_all_cameras_3d(Viewport *p_viewport)
{
    if (!p_viewport)
    {
        return;
    }
    TypedArray<Node> cameras = p_viewport->find_children("*", "Camera3D", true, false);
    for (int64_t i = 0; i < cameras.size(); i++)
    {
        if (Camera3D *camera = Object::cast_to<Camera3D>(cameras[i]))
        {
            camera->set_current(false);
        }
    }
}

// 🔗 Liaisons

void CameraManager::_bind_methods()
{
    ClassDB::bind_static_method("CameraManager", D_METHOD("get_instance"), &CameraManager::get_instance);

    ClassDB::bind_method(D_METHOD("switch_2d_camera", "new_camera", "ease_transition", "duration", "transition_type", "ease_type"), &CameraManager::switch_2d_camera, DEFVAL(true), DEFVAL(-1.0),
                         DEFVAL(Tween::TRANS_CUBIC), DEFVAL(Tween::EASE_IN_OUT));
    ClassDB::bind_static_method("CameraManager", D_METHOD("get_current_camera_2d", "viewport"), &CameraManager::get_current_camera_2d);
    ClassDB::bind_method(D_METHOD("is_transitioning_2d"), &CameraManager::is_transitioning_2d);
    ClassDB::bind_method(D_METHOD("set_transition_duration_2d", "duration"), &CameraManager::set_transition_duration_2d);
    ClassDB::bind_method(D_METHOD("get_transition_duration_2d"), &CameraManager::get_transition_duration_2d);

    ClassDB::bind_method(D_METHOD("switch_3d_camera", "new_camera", "ease_transition", "duration", "transition_type", "ease_type"), &CameraManager::switch_3d_camera, DEFVAL(true), DEFVAL(-1.0),
                         DEFVAL(Tween::TRANS_CUBIC), DEFVAL(Tween::EASE_IN_OUT));
    ClassDB::bind_static_method("CameraManager", D_METHOD("get_current_camera_3d", "viewport"), &CameraManager::get_current_camera_3d);
    ClassDB::bind_method(D_METHOD("is_transitioning_3d"), &CameraManager::is_transitioning_3d);
    ClassDB::bind_method(D_METHOD("set_transition_duration_3d", "duration"), &CameraManager::set_transition_duration_3d);
    ClassDB::bind_method(D_METHOD("get_transition_duration_3d"), &CameraManager::get_transition_duration_3d);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "transition_duration_2d", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater,suffix:s"), "set_transition_duration_2d", "get_transition_duration_2d");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "transition_duration_3d", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater,suffix:s"), "set_transition_duration_3d", "get_transition_duration_3d");

    ADD_SIGNAL(MethodInfo("current_camera_2d_switched", PropertyInfo(Variant::OBJECT, "camera_2d", PROPERTY_HINT_NODE_TYPE, "Camera2D")));
    ADD_SIGNAL(MethodInfo("current_camera_3d_switched", PropertyInfo(Variant::OBJECT, "camera_3d", PROPERTY_HINT_NODE_TYPE, "Camera3D")));
}
