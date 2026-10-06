#ifndef ORBITAL_CAMERA_HPP
#define ORBITAL_CAMERA_HPP

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/spring_arm3d.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// 🛰️ Caméra orbitale : fait tourner un SpringArm3D (portant la caméra) autour de son pivot.
/// look_around() peut être appelé directement ou connecté à un signal de type Vector2.
class OrbitalCamera : public Node3D
{
    GDCLASS(OrbitalCamera, Node3D)

  public:
    void _ready() override;
    void _process(double p_delta) override;

    /// Modifie la rotation cible : direction.x tourne autour de l'axe Y (lacet), direction.y autour de X (tangage).
    void look_around(const Vector2 &p_direction);

    void set_arm_3d(SpringArm3D *p_arm);
    SpringArm3D *get_arm_3d() const;
    void set_sensitivity(float p_sensitivity) { sensitivity = p_sensitivity; }
    float get_sensitivity() const { return sensitivity; }
    void set_use_smoothing(bool p_use) { use_smoothing = p_use; }
    bool get_use_smoothing() const { return use_smoothing; }
    void set_smooth_speed(float p_speed) { smooth_speed = p_speed; }
    float get_smooth_speed() const { return smooth_speed; }
    void set_min_pitch(float p_degrees) { min_pitch = p_degrees; }
    float get_min_pitch() const { return min_pitch; }
    void set_max_pitch(float p_degrees) { max_pitch = p_degrees; }
    float get_max_pitch() const { return max_pitch; }

  protected:
    static void _bind_methods();

  private:
    // ObjectID : le SpringArm3D peut être libéré sans prévenir la caméra.
    ObjectID arm_3d_id;
    float sensitivity = 0.005f;
    bool use_smoothing = true;
    float smooth_speed = 15.0f;
    // Limites du tangage (degrés) pour éviter de retourner la caméra
    float min_pitch = -85.0f;
    float max_pitch = 85.0f;

    // Rotation cible vers laquelle on interpole
    Vector3 target_rotation;
};
} // namespace godot

#endif // ORBITAL_CAMERA_HPP
