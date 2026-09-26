#include "camera_manager.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/⚙️Générateur de Gestionnaire/manager_builder.hpp"

#include <godot_cpp/classes/camera_attributes.hpp>
#include <godot_cpp/classes/compositor.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

CameraManager::CameraManager() {}
CameraManager::~CameraManager() {}

void CameraManager::_bind_methods()
{
    ClassDB::bind_static_method("CameraManager", D_METHOD("get_singleton"), &CameraManager::get_singleton);

    ClassDB::bind_method(D_METHOD("set_transition_duration", "duration"), &CameraManager::set_transition_duration);
    ClassDB::bind_method(D_METHOD("get_transition_duration"), &CameraManager::get_transition_duration);
    ClassDB::bind_method(D_METHOD("set_transition_type", "type"), &CameraManager::set_transition_type);
    ClassDB::bind_method(D_METHOD("get_transition_type"), &CameraManager::get_transition_type);
    ClassDB::bind_method(D_METHOD("set_ease_type", "ease"), &CameraManager::set_ease_type);
    ClassDB::bind_method(D_METHOD("get_ease_type"), &CameraManager::get_ease_type);

    ClassDB::bind_method(D_METHOD("switch_camera_2d", "camera", "use_transition", "duration"), &CameraManager::switch_camera_2d, DEFVAL(true), DEFVAL(-1.0));
    ClassDB::bind_method(D_METHOD("get_current_camera_2d", "viewport"), &CameraManager::get_current_camera_2d);
    ClassDB::bind_method(D_METHOD("is_transitioning_2d", "viewport"), &CameraManager::is_transitioning_2d);

    ClassDB::bind_method(D_METHOD("switch_camera_3d", "camera", "use_transition", "duration"), &CameraManager::switch_camera_3d, DEFVAL(true), DEFVAL(-1.0));
    ClassDB::bind_method(D_METHOD("get_current_camera_3d", "viewport"), &CameraManager::get_current_camera_3d);
    ClassDB::bind_method(D_METHOD("is_transitioning_3d", "viewport"), &CameraManager::is_transitioning_3d);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "transition_duration", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater,suffix:s"), "set_transition_duration", "get_transition_duration");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "transition_type", PROPERTY_HINT_ENUM, "Linear,Sine,Quint,Quart,Quad,Expo,Elastic,Cubic,Circ,Bounce,Back,Spring"), "set_transition_type", "get_transition_type");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ease_type", PROPERTY_HINT_ENUM, "In,Out,In Out,Out In"), "set_ease_type", "get_ease_type");

    ADD_SIGNAL(MethodInfo("camera_2d_switched", PropertyInfo(Variant::OBJECT, "camera", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, "Camera2D")));
    ADD_SIGNAL(MethodInfo("camera_3d_switched", PropertyInfo(Variant::OBJECT, "camera", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, "Camera3D")));
}

void CameraManager::_enter_tree()
{
    // S'enregistre comme instance canonique du singleton (ou se détruit si un doublon existe déjà).
    ManagerBuilder::init_instance("CameraManager", this);
}

void CameraManager::_exit_tree()
{
    // Les transitoires vivent dans les Viewports, pas sous le manager : sans ce nettoyage,
    // elles resteraient actives et figées si le manager quitte l'arbre en pleine transition.
    for (KeyValue<uint64_t, Transition2D> &entry : _transitions_2d)
    {
        _free_transitioner(entry.value.transitioner_id);
    }
    for (KeyValue<uint64_t, Transition3D> &entry : _transitions_3d)
    {
        _free_transitioner(entry.value.transitioner_id);
    }
    _transitions_2d.clear();
    _transitions_3d.clear();

    ManagerBuilder::unregister_instance("CameraManager", this);
}

void CameraManager::_process(double p_delta)
{
    _process_transitions_2d(p_delta);
    _process_transitions_3d(p_delta);
}

CameraManager *CameraManager::get_singleton() { return Object::cast_to<CameraManager>(ManagerBuilder::make_getter("CameraManager")); }

// --- Paramètres par défaut ---

void CameraManager::set_transition_duration(double p_duration) { _transition_duration = MAX(p_duration, 0.0); }
double CameraManager::get_transition_duration() const { return _transition_duration; }
void CameraManager::set_transition_type(Tween::TransitionType p_type) { _transition_type = p_type; }
Tween::TransitionType CameraManager::get_transition_type() const { return _transition_type; }
void CameraManager::set_ease_type(Tween::EaseType p_ease) { _ease_type = p_ease; }
Tween::EaseType CameraManager::get_ease_type() const { return _ease_type; }

// --- Outils internes ---

CameraManager::State2D CameraManager::_read_state_2d(Camera2D *p_camera)
{
    State2D state;
    state.position = p_camera->get_global_position();
    state.offset = p_camera->get_offset();
    state.zoom = p_camera->get_zoom();
    state.rotation = p_camera->get_global_rotation();
    return state;
}

CameraManager::State3D CameraManager::_read_state_3d(Camera3D *p_camera)
{
    State3D state;
    // Orthonormalisé : la rotation est interpolée en quaternion, une échelle parasite fausserait l'extraction.
    state.transform = p_camera->get_global_transform().orthonormalized();
    state.fov = p_camera->get_fov();
    state.size = p_camera->get_size();
    state.z_near = p_camera->get_near();
    state.z_far = p_camera->get_far();
    state.h_offset = p_camera->get_h_offset();
    state.v_offset = p_camera->get_v_offset();
    return state;
}

CameraManager::State2D CameraManager::_lerp_state_2d(const State2D &p_from, const State2D &p_to, real_t p_weight)
{
    State2D state;
    state.position = p_from.position.lerp(p_to.position, p_weight);
    state.offset = p_from.offset.lerp(p_to.offset, p_weight);
    state.zoom = p_from.zoom.lerp(p_to.zoom, p_weight);
    // lerp_angle : prend le chemin le plus court (pas de tour complet entre -PI et PI).
    state.rotation = Math::lerp_angle(p_from.rotation, p_to.rotation, p_weight);
    return state;
}

CameraManager::State3D CameraManager::_lerp_state_3d(const State3D &p_from, const State3D &p_to, real_t p_weight)
{
    State3D state;
    Quaternion from_rotation = p_from.transform.basis.get_rotation_quaternion();
    Quaternion to_rotation = p_to.transform.basis.get_rotation_quaternion();
    state.transform = Transform3D(Basis(from_rotation.slerp(to_rotation, p_weight)), p_from.transform.origin.lerp(p_to.transform.origin, p_weight));
    state.fov = Math::lerp(p_from.fov, p_to.fov, p_weight);
    state.size = Math::lerp(p_from.size, p_to.size, p_weight);
    state.z_near = Math::lerp(p_from.z_near, p_to.z_near, p_weight);
    state.z_far = Math::lerp(p_from.z_far, p_to.z_far, p_weight);
    state.h_offset = Math::lerp(p_from.h_offset, p_to.h_offset, p_weight);
    state.v_offset = Math::lerp(p_from.v_offset, p_to.v_offset, p_weight);
    return state;
}

void CameraManager::_apply_state_2d(Camera2D *p_camera, const State2D &p_state)
{
    p_camera->set_global_position(p_state.position);
    p_camera->set_global_rotation(p_state.rotation);
    p_camera->set_offset(p_state.offset);
    p_camera->set_zoom(p_state.zoom);
}

void CameraManager::_apply_state_3d(Camera3D *p_camera, const State3D &p_state)
{
    p_camera->set_global_transform(p_state.transform);
    p_camera->set_fov(p_state.fov);
    p_camera->set_size(p_state.size);
    p_camera->set_near(p_state.z_near);
    p_camera->set_far(p_state.z_far);
    p_camera->set_h_offset(p_state.h_offset);
    p_camera->set_v_offset(p_state.v_offset);
}

void CameraManager::_free_transitioner(ObjectID p_id)
{
    Node *node = Object::cast_to<Node>(UtilityFunctions::instance_from_id((int64_t)p_id));
    if (node)
    {
        node->queue_free();
    }
}

// --- 2D ---

void CameraManager::switch_camera_2d(Camera2D *p_camera, bool p_use_transition, double p_duration)
{
    ERR_FAIL_NULL(p_camera);
    ERR_FAIL_COND_MSG(!p_camera->is_inside_tree(), "CameraManager::switch_camera_2d — la caméra doit être dans l'arbre de scène.");
    Viewport *viewport = p_camera->get_viewport();
    ERR_FAIL_NULL(viewport);

    const uint64_t key = (uint64_t)viewport->get_instance_id();
    const double duration = p_duration < 0.0 ? _transition_duration : p_duration;
    Transition2D *running = _transitions_2d.getptr(key);

    // Changement instantané (demandé, ou durée nulle) : annule une éventuelle transition en cours.
    if (!p_use_transition || duration <= 0.0)
    {
        p_camera->set_enabled(true);
        p_camera->make_current();
        if (running)
        {
            _free_transitioner(running->transitioner_id);
            _transitions_2d.erase(key);
        }
        emit_signal("camera_2d_switched", p_camera);
        return;
    }

    // Interruption : on repart de l'état actuel de la transitoire vers la nouvelle cible.
    if (running)
    {
        if ((uint64_t)running->target_id == p_camera->get_instance_id())
        {
            return; // Déjà en route vers cette caméra : la transition continue telle quelle.
        }
        running->target_id = p_camera->get_instance_id();
        running->from = running->current;
        running->elapsed = 0.0;
        running->duration = duration;
        running->trans = _transition_type;
        running->ease = _ease_type;
        if (Camera2D *transitioner = Object::cast_to<Camera2D>(UtilityFunctions::instance_from_id((int64_t)running->transitioner_id)))
        {
            transitioner->set_anchor_mode(p_camera->get_anchor_mode());
            transitioner->set_ignore_rotation(p_camera->is_ignoring_rotation());
        }
        return;
    }

    Camera2D *source = viewport->get_camera_2d();
    if (!source || source == p_camera)
    {
        // Aucune caméra de départ (ou déjà la bonne) : rien à interpoler.
        p_camera->set_enabled(true);
        p_camera->make_current();
        emit_signal("camera_2d_switched", p_camera);
        return;
    }

    Camera2D *transitioner = memnew(Camera2D);
    transitioner->set_name("CameraManager Transition 2D");
    // Réglages repris de la cible : c'est avec eux que la vue doit coïncider à l'arrivée.
    transitioner->set_anchor_mode(p_camera->get_anchor_mode());
    transitioner->set_ignore_rotation(p_camera->is_ignoring_rotation());

    Transition2D transition;
    transition.viewport_id = viewport->get_instance_id();
    transition.transitioner_id = transitioner->get_instance_id();
    transition.target_id = p_camera->get_instance_id();
    transition.from = _read_state_2d(source);
    transition.current = transition.from;
    transition.duration = duration;
    transition.trans = _transition_type;
    transition.ease = _ease_type;
    _transitions_2d.insert(key, transition);

    // Ajout différé : le Viewport peut être occupé à installer ses enfants (appel depuis un
    // _ready). La transitoire ne devient active qu'une fois dans l'arbre, dans _process ;
    // d'ici là la caméra source reste affichée, sans saut visuel.
    viewport->call_deferred("add_child", transitioner);
}

Camera2D *CameraManager::get_current_camera_2d(Viewport *p_viewport) const
{
    ERR_FAIL_NULL_V(p_viewport, nullptr);
    const Transition2D *running = _transitions_2d.getptr((uint64_t)p_viewport->get_instance_id());
    if (running)
    {
        return Object::cast_to<Camera2D>(UtilityFunctions::instance_from_id((int64_t)running->target_id));
    }
    return p_viewport->get_camera_2d();
}

bool CameraManager::is_transitioning_2d(Viewport *p_viewport) const
{
    ERR_FAIL_NULL_V(p_viewport, false);
    return _transitions_2d.has((uint64_t)p_viewport->get_instance_id());
}

void CameraManager::_process_transitions_2d(double p_delta)
{
    LocalVector<uint64_t> to_erase;
    LocalVector<ObjectID> finished;

    for (KeyValue<uint64_t, Transition2D> &entry : _transitions_2d)
    {
        Transition2D &transition = entry.value;
        Viewport *viewport = Object::cast_to<Viewport>(UtilityFunctions::instance_from_id((int64_t)transition.viewport_id));
        Camera2D *transitioner = Object::cast_to<Camera2D>(UtilityFunctions::instance_from_id((int64_t)transition.transitioner_id));
        Camera2D *target = Object::cast_to<Camera2D>(UtilityFunctions::instance_from_id((int64_t)transition.target_id));

        // Viewport, transitoire ou cible disparus (ou cible déplacée hors du Viewport) : on abandonne.
        if (!viewport || !transitioner || !target || !target->is_inside_tree() || target->get_viewport() != viewport)
        {
            _free_transitioner(transition.transitioner_id);
            to_erase.push_back(entry.key);
            continue;
        }

        if (!transitioner->is_inside_tree())
        {
            continue; // Ajout différé pas encore effectué.
        }

        // Une autre caméra a été rendue active par ailleurs : on lui laisse la main.
        if (transition.activated && !transitioner->is_current())
        {
            _free_transitioner(transition.transitioner_id);
            to_erase.push_back(entry.key);
            continue;
        }

        transition.elapsed += p_delta;
        const bool done = transition.elapsed >= transition.duration;
        const real_t weight = done ? 1.0f : (real_t)(double)Tween::interpolate_value(0.0, 1.0, transition.elapsed, transition.duration, transition.trans, transition.ease);

        // La cible est relue chaque frame : si elle bouge, la transition la suit.
        transition.current = _lerp_state_2d(transition.from, _read_state_2d(target), weight);
        _apply_state_2d(transitioner, transition.current);

        if (!transition.activated)
        {
            transitioner->make_current();
            transition.activated = true;
        }

        if (done)
        {
            target->set_enabled(true);
            target->make_current();
            _free_transitioner(transition.transitioner_id);
            to_erase.push_back(entry.key);
            finished.push_back(transition.target_id);
        }
    }

    for (const uint64_t &key : to_erase)
    {
        _transitions_2d.erase(key);
    }
    // Émission après la boucle : un écouteur peut relancer switch_camera_2d sans invalider l'itération.
    for (const ObjectID &id : finished)
    {
        if (Camera2D *camera = Object::cast_to<Camera2D>(UtilityFunctions::instance_from_id((int64_t)id)))
        {
            emit_signal("camera_2d_switched", camera);
        }
    }
}

// --- 3D ---

void CameraManager::switch_camera_3d(Camera3D *p_camera, bool p_use_transition, double p_duration)
{
    ERR_FAIL_NULL(p_camera);
    ERR_FAIL_COND_MSG(!p_camera->is_inside_tree(), "CameraManager::switch_camera_3d — la caméra doit être dans l'arbre de scène.");
    Viewport *viewport = p_camera->get_viewport();
    ERR_FAIL_NULL(viewport);

    const uint64_t key = (uint64_t)viewport->get_instance_id();
    const double duration = p_duration < 0.0 ? _transition_duration : p_duration;
    Transition3D *running = _transitions_3d.getptr(key);

    if (!p_use_transition || duration <= 0.0)
    {
        p_camera->make_current();
        if (running)
        {
            _free_transitioner(running->transitioner_id);
            _transitions_3d.erase(key);
        }
        emit_signal("camera_3d_switched", p_camera);
        return;
    }

    if (running)
    {
        if ((uint64_t)running->target_id == p_camera->get_instance_id())
        {
            return;
        }
        running->target_id = p_camera->get_instance_id();
        running->from = running->current;
        running->elapsed = 0.0;
        running->duration = duration;
        running->trans = _transition_type;
        running->ease = _ease_type;
        return;
    }

    Camera3D *source = viewport->get_camera_3d();
    if (!source || source == p_camera)
    {
        p_camera->make_current();
        emit_signal("camera_3d_switched", p_camera);
        return;
    }

    Camera3D *transitioner = memnew(Camera3D);
    transitioner->set_name("CameraManager Transition 3D");
    // Réglages non interpolables repris de la source : le rendu ne change pas au premier
    // frame. Ils basculent sur ceux de la cible à l'arrivée, quand elle devient active.
    transitioner->set_projection(source->get_projection());
    transitioner->set_keep_aspect_mode(source->get_keep_aspect_mode());
    transitioner->set_cull_mask(source->get_cull_mask());
    transitioner->set_environment(source->get_environment());
    transitioner->set_attributes(source->get_attributes());
    transitioner->set_compositor(source->get_compositor());
    transitioner->set_frustum_offset(source->get_frustum_offset());
    transitioner->set_doppler_tracking(source->get_doppler_tracking());

    Transition3D transition;
    transition.viewport_id = viewport->get_instance_id();
    transition.transitioner_id = transitioner->get_instance_id();
    transition.target_id = p_camera->get_instance_id();
    transition.from = _read_state_3d(source);
    transition.current = transition.from;
    transition.duration = duration;
    transition.trans = _transition_type;
    transition.ease = _ease_type;
    _transitions_3d.insert(key, transition);

    viewport->call_deferred("add_child", transitioner);
}

Camera3D *CameraManager::get_current_camera_3d(Viewport *p_viewport) const
{
    ERR_FAIL_NULL_V(p_viewport, nullptr);
    const Transition3D *running = _transitions_3d.getptr((uint64_t)p_viewport->get_instance_id());
    if (running)
    {
        return Object::cast_to<Camera3D>(UtilityFunctions::instance_from_id((int64_t)running->target_id));
    }
    return p_viewport->get_camera_3d();
}

bool CameraManager::is_transitioning_3d(Viewport *p_viewport) const
{
    ERR_FAIL_NULL_V(p_viewport, false);
    return _transitions_3d.has((uint64_t)p_viewport->get_instance_id());
}

void CameraManager::_process_transitions_3d(double p_delta)
{
    LocalVector<uint64_t> to_erase;
    LocalVector<ObjectID> finished;

    for (KeyValue<uint64_t, Transition3D> &entry : _transitions_3d)
    {
        Transition3D &transition = entry.value;
        Viewport *viewport = Object::cast_to<Viewport>(UtilityFunctions::instance_from_id((int64_t)transition.viewport_id));
        Camera3D *transitioner = Object::cast_to<Camera3D>(UtilityFunctions::instance_from_id((int64_t)transition.transitioner_id));
        Camera3D *target = Object::cast_to<Camera3D>(UtilityFunctions::instance_from_id((int64_t)transition.target_id));

        if (!viewport || !transitioner || !target || !target->is_inside_tree() || target->get_viewport() != viewport)
        {
            _free_transitioner(transition.transitioner_id);
            to_erase.push_back(entry.key);
            continue;
        }

        if (!transitioner->is_inside_tree())
        {
            continue;
        }

        if (transition.activated && !transitioner->is_current())
        {
            _free_transitioner(transition.transitioner_id);
            to_erase.push_back(entry.key);
            continue;
        }

        transition.elapsed += p_delta;
        const bool done = transition.elapsed >= transition.duration;
        const real_t weight = done ? 1.0f : (real_t)(double)Tween::interpolate_value(0.0, 1.0, transition.elapsed, transition.duration, transition.trans, transition.ease);

        transition.current = _lerp_state_3d(transition.from, _read_state_3d(target), weight);
        _apply_state_3d(transitioner, transition.current);

        if (!transition.activated)
        {
            transitioner->make_current();
            transition.activated = true;
        }

        if (done)
        {
            target->make_current();
            _free_transitioner(transition.transitioner_id);
            to_erase.push_back(entry.key);
            finished.push_back(transition.target_id);
        }
    }

    for (const uint64_t &key : to_erase)
    {
        _transitions_3d.erase(key);
    }
    for (const ObjectID &id : finished)
    {
        if (Camera3D *camera = Object::cast_to<Camera3D>(UtilityFunctions::instance_from_id((int64_t)id)))
        {
            emit_signal("camera_3d_switched", camera);
        }
    }
}
