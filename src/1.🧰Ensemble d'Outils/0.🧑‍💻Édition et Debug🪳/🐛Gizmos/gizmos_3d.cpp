#include "gizmos_3d.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// Les fonctions sont implémentées dans Gizmos (gizmos.hpp) : on les lie directement ici,
// sous les noms historiques utilisés par GDScript.
void Gizmos3D::_bind_methods()
{
    ClassDB::bind_static_method("Gizmos3D", D_METHOD("create_3D_gizmo", "viewport"), &Gizmos::create_3D_gizmo);
    ClassDB::bind_static_method("Gizmos3D", D_METHOD("clear_3D_gizmo", "gizmo_rid"), &Gizmos::clear_3D_gizmo);
    ClassDB::bind_static_method("Gizmos3D", D_METHOD("free_3D_gizmo", "gizmo_rid"), &Gizmos::free_3D_gizmo);

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_line", "instance_rid", "from", "to", "color", "width"), &Gizmos::draw_3D_line, DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(1.0f));

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_arrow", "instance_rid", "from", "to", "color", "head_size", "width"), &Gizmos::draw_3D_arrow, DEFVAL(Color(1.0, 1.0, 1.0, 1.0)),
                                DEFVAL(0.3f), DEFVAL(1.0f));

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_cube", "instance_rid", "center", "size", "color", "filled", "width"), &Gizmos::draw_3D_cube, DEFVAL(Color(1.0, 1.0, 1.0, 1.0)),
                                DEFVAL(false), DEFVAL(1.0f));

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_sphere", "instance_rid", "center", "radius", "color", "filled", "segments", "width"), &Gizmos::draw_3D_sphere,
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false), DEFVAL(16), DEFVAL(1.0f));

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_circle", "instance_rid", "center", "radius", "normal", "color", "filled", "segments", "width"), &Gizmos::draw_3D_circle,
                                DEFVAL(Vector3(0, 1, 0)), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false), DEFVAL(24), DEFVAL(1.0f));

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_rectangle", "instance_rid", "center", "size", "normal", "color", "filled", "width"), &Gizmos::draw_3D_rectangle,
                                DEFVAL(Vector3(0, 1, 0)), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false), DEFVAL(1.0f));

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_capsule", "instance_rid", "center", "radius", "height", "color", "filled", "width"), &Gizmos::draw_3D_capsule,
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false), DEFVAL(1.0f));

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_polygon", "instance_rid", "points", "color", "filled", "width"), &Gizmos::draw_3D_polygon, DEFVAL(Color(1.0, 1.0, 1.0, 1.0)),
                                DEFVAL(false), DEFVAL(1.0f));

    ClassDB::bind_static_method(
        "Gizmos3D", D_METHOD("draw_3D_path", "instance_rid", "points", "as_arrows", "segments_per_line", "spacing", "use_bezier", "show_points", "point_radius", "width", "color", "arrow_head_size"),
        &Gizmos::draw_3D_path, DEFVAL(false), DEFVAL(1), DEFVAL(0.0f), DEFVAL(false), DEFVAL(false), DEFVAL(0.1f), DEFVAL(2.0f), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(12.0f));

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_origin", "instance_rid", "transform", "size", "head_size", "width"), &Gizmos::draw_3D_origin, DEFVAL(Transform3D()), DEFVAL(1.0f),
                                DEFVAL(0.2f), DEFVAL(2.0f));

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_camera_frustum", "instance_rid", "transform", "fov", "z_near", "z_far", "aspect", "color"), &Gizmos::draw_3D_camera_frustum,
                                DEFVAL(Color(1.0, 0.0, 1.0, 1.0)));

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_icon", "instance_rid", "position", "texture", "size", "filter"), &Gizmos::draw_3D_icon, DEFVAL(Vector2(1, 1)),
                                DEFVAL(BaseMaterial3D::TEXTURE_FILTER_NEAREST));

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("apply_shader_on_gizmo", "instance_rid", "material"), &Gizmos::apply_shader_on_3D_gizmo);

    ClassDB::bind_static_method("Gizmos3D", D_METHOD("draw_3D_text", "instance_rid", "position", "text", "color", "font_size", "pixel_size"), &Gizmos::draw_3D_text,
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(32), DEFVAL(0.01f));
}
