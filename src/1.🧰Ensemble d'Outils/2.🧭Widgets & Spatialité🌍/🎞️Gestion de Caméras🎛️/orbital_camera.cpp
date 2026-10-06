#include "orbital_camera.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void OrbitalCamera::set_arm_3d(SpringArm3D *p_arm) { arm_3d_id = p_arm ? ObjectID(p_arm->get_instance_id()) : ObjectID(); }

SpringArm3D *OrbitalCamera::get_arm_3d() const { return Object::cast_to<SpringArm3D>(ObjectDB::get_instance(arm_3d_id)); }

void OrbitalCamera::_ready()
{
    SpringArm3D *arm = get_arm_3d();
    if (!arm)
    {
        ERR_PRINT(String(U"[OrbitalCamera] ") + get_name() + String(U" : arm_3d n'est pas assigné !"));
        return;
    }
    // La cible part de la rotation définie dans l'éditeur
    target_rotation = arm->get_rotation();
}

void OrbitalCamera::_process(double p_delta)
{
    SpringArm3D *arm = get_arm_3d();
    if (!arm)
    {
        return;
    }

    if (use_smoothing)
    {
        // Interpolation fluide vers la rotation cible (poids borné à 1)
        arm->set_rotation(arm->get_rotation().lerp(target_rotation, MIN((float)p_delta * smooth_speed, 1.0f)));
    }
    else
    {
        arm->set_rotation(target_rotation);
    }
}

void OrbitalCamera::look_around(const Vector2 &p_direction)
{
    SpringArm3D *arm = get_arm_3d();
    if (!arm)
    {
        return;
    }

    // direction.x (horizontal) -> axe Y (lacet) ; direction.y (vertical) -> axe X (tangage)
    target_rotation += Vector3(-p_direction.y, -p_direction.x, 0.0f) * sensitivity;
    target_rotation.x = CLAMP(target_rotation.x, Math::deg_to_rad(min_pitch), Math::deg_to_rad(max_pitch));

    // Sans lissage, application immédiate pour éviter un retard d'une frame
    if (!use_smoothing)
    {
        arm->set_rotation(target_rotation);
    }
}

void OrbitalCamera::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("look_around", "direction"), &OrbitalCamera::look_around);

    ClassDB::bind_method(D_METHOD("set_arm_3d", "arm"), &OrbitalCamera::set_arm_3d);
    ClassDB::bind_method(D_METHOD("get_arm_3d"), &OrbitalCamera::get_arm_3d);
    ClassDB::bind_method(D_METHOD("set_sensitivity", "sensitivity"), &OrbitalCamera::set_sensitivity);
    ClassDB::bind_method(D_METHOD("get_sensitivity"), &OrbitalCamera::get_sensitivity);
    ClassDB::bind_method(D_METHOD("set_use_smoothing", "use"), &OrbitalCamera::set_use_smoothing);
    ClassDB::bind_method(D_METHOD("get_use_smoothing"), &OrbitalCamera::get_use_smoothing);
    ClassDB::bind_method(D_METHOD("set_smooth_speed", "speed"), &OrbitalCamera::set_smooth_speed);
    ClassDB::bind_method(D_METHOD("get_smooth_speed"), &OrbitalCamera::get_smooth_speed);
    ClassDB::bind_method(D_METHOD("set_min_pitch", "degrees"), &OrbitalCamera::set_min_pitch);
    ClassDB::bind_method(D_METHOD("get_min_pitch"), &OrbitalCamera::get_min_pitch);
    ClassDB::bind_method(D_METHOD("set_max_pitch", "degrees"), &OrbitalCamera::set_max_pitch);
    ClassDB::bind_method(D_METHOD("get_max_pitch"), &OrbitalCamera::get_max_pitch);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "arm_3d", PROPERTY_HINT_NODE_TYPE, "SpringArm3D"), "set_arm_3d", "get_arm_3d");

    ADD_GROUP(U"⚙️ Paramètres de rotation", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sensitivity", PROPERTY_HINT_RANGE, "0,0.1,0.0001,or_greater"), "set_sensitivity", "get_sensitivity");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_smoothing"), "set_use_smoothing", "get_use_smoothing");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "smooth_speed", PROPERTY_HINT_RANGE, "1.0,50.0,0.5"), "set_smooth_speed", "get_smooth_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_pitch", PROPERTY_HINT_RANGE, String(U"-89,0,0.1,suffix:°")), "set_min_pitch", "get_min_pitch");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_pitch", PROPERTY_HINT_RANGE, String(U"0,89,0.1,suffix:°")), "set_max_pitch", "get_max_pitch");
}
