#ifndef CAMERA_MANAGER_HPP
#define CAMERA_MANAGER_HPP

#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot
{

// CameraManager
//
// Singleton (via ManagerBuilder) qui change la caméra active d'un Viewport, instantanément
// ou avec une transition interpolée, en 2D comme en 3D.
//
// Principe de la transition : une caméra "transitoire" interne, créée par le manager, est
// ajoutée au Viewport de la caméra cible et rendue active. Elle part de l'état de la caméra
// précédemment active, puis est interpolée chaque frame vers l'état COURANT de la cible
// (relu à chaque frame : une cible qui bouge pendant la transition est suivie, là où un
// Tween figerait la destination au départ). À la fin, la cible devient active et la
// transitoire est libérée.
//
// Différences volontaires avec le CameraManager C# d'origine :
// - Aucune caméra de l'utilisateur n'est désactivée : on passe par make_current(), le
//   Viewport gère lui-même la caméra courante.
// - La transitoire est enfant du Viewport (et non de la caméra cible) : si la cible est
//   libérée en pleine transition, la transitoire n'est pas détruite avec elle.
// - Une transition par Viewport (clé = ObjectID du Viewport) : l'écran partagé fonctionne.
//
// Toutes les références (Viewport, cible, transitoire) sont stockées en ObjectID et résolues
// via UtilityFunctions::instance_from_id à chaque frame : un nœud libéré entre-temps annule
// proprement la transition au lieu de laisser un pointeur périmé.
class CameraManager : public Node
{
    GDCLASS(CameraManager, Node)

  private:
    struct State2D
    {
        Vector2 position;
        Vector2 offset;
        Vector2 zoom = Vector2(1.0f, 1.0f);
        real_t rotation = 0.0f;
    };

    struct State3D
    {
        Transform3D transform;
        real_t fov = 75.0f;
        real_t size = 1.0f;
        // Pas "near"/"far" : ce sont des macros vides dans les en-têtes Windows.
        real_t z_near = 0.05f;
        real_t z_far = 4000.0f;
        real_t h_offset = 0.0f;
        real_t v_offset = 0.0f;
    };

    struct Transition2D
    {
        ObjectID viewport_id;
        ObjectID transitioner_id;
        ObjectID target_id;
        State2D from;    // état de départ, figé au lancement (ou à l'interruption)
        State2D current; // dernier état appliqué, sert de départ si la transition est interrompue
        double elapsed = 0.0;
        double duration = 1.0;
        Tween::TransitionType trans = Tween::TRANS_CUBIC;
        Tween::EaseType ease = Tween::EASE_IN_OUT;
        bool activated = false; // la transitoire a été rendue active au moins une fois
    };

    struct Transition3D
    {
        ObjectID viewport_id;
        ObjectID transitioner_id;
        ObjectID target_id;
        State3D from;
        State3D current;
        double elapsed = 0.0;
        double duration = 1.0;
        Tween::TransitionType trans = Tween::TRANS_CUBIC;
        Tween::EaseType ease = Tween::EASE_IN_OUT;
        bool activated = false;
    };

    // Clé : ObjectID du Viewport.
    HashMap<uint64_t, Transition2D> _transitions_2d;
    HashMap<uint64_t, Transition3D> _transitions_3d;

    double _transition_duration = 1.0;
    Tween::TransitionType _transition_type = Tween::TRANS_CUBIC;
    Tween::EaseType _ease_type = Tween::EASE_IN_OUT;

    static State2D _read_state_2d(Camera2D *p_camera);
    static State3D _read_state_3d(Camera3D *p_camera);
    static State2D _lerp_state_2d(const State2D &p_from, const State2D &p_to, real_t p_weight);
    static State3D _lerp_state_3d(const State3D &p_from, const State3D &p_to, real_t p_weight);
    static void _apply_state_2d(Camera2D *p_camera, const State2D &p_state);
    static void _apply_state_3d(Camera3D *p_camera, const State3D &p_state);

    // Libère la transitoire d'une transition (queue_free : sûr même si l'ajout différé au
    // Viewport n'a pas encore eu lieu).
    static void _free_transitioner(ObjectID p_id);

    void _process_transitions_2d(double p_delta);
    void _process_transitions_3d(double p_delta);
    void _update_processing();

  protected:
    static void _bind_methods();

  public:
    CameraManager();
    ~CameraManager();

    void _enter_tree() override;
    void _exit_tree() override;
    PackedStringArray _get_configuration_warnings() const override;
    void _process(double p_delta) override;

    // Instance singleton (créée via ManagerBuilder au premier appel).
    static CameraManager *get_singleton();

    // --- Paramètres par défaut des transitions ---
    void set_transition_duration(double p_duration);
    double get_transition_duration() const;
    void set_transition_type(Tween::TransitionType p_type);
    Tween::TransitionType get_transition_type() const;
    void set_ease_type(Tween::EaseType p_ease);
    Tween::EaseType get_ease_type() const;

    // --- 2D ---
    // p_duration < 0 : utilise transition_duration. Une durée nulle équivaut à un changement
    // instantané. La caméra doit être dans l'arbre de scène.
    void switch_camera_2d(Camera2D *p_camera, bool p_use_transition = true, double p_duration = -1.0);
    // Caméra "logiquement" active du Viewport : la cible si une transition est en cours,
    // sinon Viewport::get_camera_2d(). Ne renvoie jamais la caméra transitoire interne.
    Camera2D *get_current_camera_2d(Viewport *p_viewport) const;
    bool is_transitioning_2d(Viewport *p_viewport) const;

    // --- 3D ---
    void switch_camera_3d(Camera3D *p_camera, bool p_use_transition = true, double p_duration = -1.0);
    Camera3D *get_current_camera_3d(Viewport *p_viewport) const;
    bool is_transitioning_3d(Viewport *p_viewport) const;
};

} // namespace godot

#endif // CAMERA_MANAGER_HPP
