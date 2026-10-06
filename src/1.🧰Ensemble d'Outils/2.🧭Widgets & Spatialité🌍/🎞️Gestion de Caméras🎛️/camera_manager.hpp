#ifndef CAMERA_MANAGER_HPP
#define CAMERA_MANAGER_HPP

#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// 🎞️ Gestionnaire de caméras (singleton via ManagerBuilder).
///
/// Bascule la caméra courante d'un Viewport, instantanément ou par une transition fluide : une caméra
/// transitoire interne part de la caméra courante et rejoint la nouvelle (transform, zoom/offset en 2D,
/// fov/size/near/far en 3D), puis la nouvelle caméra devient courante.
/// Une transition peut être interrompue par un nouvel appel : elle repart alors de l'état atteint.
class CameraManager : public Node
{
    GDCLASS(CameraManager, Node)

  public:
    ~CameraManager();

    /// Singleton : instance existante, ou créée sous la racine du SceneTree.
    static CameraManager *get_instance();

    void _enter_tree() override;
    void _exit_tree() override;

    // 🎥 2D

    /// Rend p_new_camera courante. p_duration < 0 : utilise transition_duration_2d.
    void switch_2d_camera(Camera2D *p_new_camera, bool p_ease_transition = true, double p_duration = -1.0, Tween::TransitionType p_transition_type = Tween::TRANS_CUBIC,
                          Tween::EaseType p_ease_type = Tween::EASE_IN_OUT);
    /// Caméra 2D courante de p_viewport (la caméra transitoire pendant une transition).
    static Camera2D *get_current_camera_2d(Viewport *p_viewport);
    bool is_transitioning_2d() const { return transitioning_2d; }

    void set_transition_duration_2d(double p_duration) { transition_duration_2d = p_duration; }
    double get_transition_duration_2d() const { return transition_duration_2d; }

    // 🎥 3D

    /// Rend p_new_camera courante. p_duration < 0 : utilise transition_duration_3d.
    void switch_3d_camera(Camera3D *p_new_camera, bool p_ease_transition = true, double p_duration = -1.0, Tween::TransitionType p_transition_type = Tween::TRANS_CUBIC,
                          Tween::EaseType p_ease_type = Tween::EASE_IN_OUT);
    /// Caméra 3D courante de p_viewport (la caméra transitoire pendant une transition).
    static Camera3D *get_current_camera_3d(Viewport *p_viewport);
    bool is_transitioning_3d() const { return transitioning_3d; }

    void set_transition_duration_3d(double p_duration) { transition_duration_3d = p_duration; }
    double get_transition_duration_3d() const { return transition_duration_3d; }

  protected:
    static void _bind_methods();

  private:
    static ObjectID instance_id;

    // Caméras transitoires : ObjectID car elles sont rattachées à la caméra cible, et libérées avec elle.
    ObjectID transitioner_2d_id;
    ObjectID transitioner_3d_id;
    Ref<Tween> active_tween_2d;
    Ref<Tween> active_tween_3d;
    bool transitioning_2d = false;
    bool transitioning_3d = false;
    double transition_duration_2d = 1.0;
    double transition_duration_3d = 1.0;

    /// Caméra transitoire existante, ou recréée si elle a été libérée.
    Camera2D *_get_transitioner_2d();
    Camera3D *_get_transitioner_3d();
    /// Rattache la caméra transitoire sous p_target (elle doit être dans le même Viewport que la cible).
    static void _attach_transitioner(Node *p_transitioner, Node *p_target);
    static void _kill_tween(Ref<Tween> &r_tween);

    void _on_transition_2d_complete(uint64_t p_camera_id);
    void _on_transition_3d_complete(uint64_t p_camera_id);

    static void _disable_all_cameras_2d(Viewport *p_viewport);
    static void _disable_all_cameras_3d(Viewport *p_viewport);
};
} // namespace godot

#endif // CAMERA_MANAGER_HPP
