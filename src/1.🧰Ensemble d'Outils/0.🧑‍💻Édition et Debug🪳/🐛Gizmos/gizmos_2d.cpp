#include "gizmos_2d.hpp"
#include "gizmos.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// Les fonctions sont implémentées dans Gizmos (gizmos.hpp) : on les lie directement ici,
// sous les noms historiques utilisés par GDScript.
void Gizmos2D::_bind_methods()
{
    ClassDB::bind_static_method("Gizmos2D", D_METHOD("create_2D_gizmo", "viewport"), &Gizmos::create_2D_gizmo);

    ClassDB::bind_static_method("Gizmos2D", D_METHOD("clear_2D_gizmo", "gizmo_rid"), &Gizmos::clear_2D_gizmo);

    ClassDB::bind_static_method("Gizmos2D", D_METHOD("free_2D_gizmo", "gizmo_rid"), &Gizmos::free_2D_gizmo);

    ClassDB::bind_static_method("Gizmos2D", D_METHOD("draw_2D_line", "gizmo_rid", "from", "to", "width", "color", "antialiasing"), &Gizmos::draw_2D_line, DEFVAL(2.0),
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos2D", D_METHOD("draw_2D_arrow", "gizmo_rid", "from", "to", "arrow_head_size", "width", "color", "antialiasing"), &Gizmos::draw_2D_arrow, DEFVAL(12.0f),
                                DEFVAL(1.0f), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos2D", D_METHOD("draw_2D_circle", "gizmo_rid", "center", "radius", "color", "filled", "thickness", "antialiasing"), &Gizmos::draw_2D_circle,
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false), DEFVAL(2.0), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos2D", D_METHOD("draw_2D_rectangle", "gizmo_rid", "rect", "color", "filled", "thickness", "antialiasing"), &Gizmos::draw_2D_rectangle,
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false), DEFVAL(2.0), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos2D", D_METHOD("draw_2D_capsule", "gizmo_rid", "position", "size", "color", "thickness", "antialiasing"), &Gizmos::draw_2D_capsule,
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(2.0), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos2D", D_METHOD("draw_2D_polygon", "gizmo_rid", "points", "color", "filled", "thickness", "antialiasing"), &Gizmos::draw_2D_polygon,
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false), DEFVAL(2.0), DEFVAL(false));
    ClassDB::bind_static_method(
        "Gizmos2D",
        D_METHOD("draw_2D_path", "gizmo_rid", "points", "as_arrows", "segments_per_line", "spacing", "use_bezier", "show_points", "point_radius", "width", "color", "arrow_head_size", "antialiasing"),
        &Gizmos::draw_2D_path, DEFVAL(false), DEFVAL(1), DEFVAL(0.0f), DEFVAL(false), DEFVAL(false), DEFVAL(4.0f), DEFVAL(2.0f), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(12.0f), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos2D", D_METHOD("draw_2D_origin", "gizmo_rid", "transform", "length", "arrow_head_size", "cross_size", "width", "cross_color", "antialiasing"),
                                &Gizmos::draw_2D_origin, DEFVAL(Transform2D()), DEFVAL(32.0f), DEFVAL(8.0f), DEFVAL(8.0f), DEFVAL(2.0f), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos2D", D_METHOD("draw_2D_icon", "gizmo_rid", "position", "icon", "size", "filter"), &Gizmos::draw_2D_icon, DEFVAL(Vector2(0, 0)),
                                DEFVAL(RenderingServer::CanvasItemTextureFilter::CANVAS_ITEM_TEXTURE_FILTER_NEAREST));
    ClassDB::bind_static_method("Gizmos2D", D_METHOD("draw_2D_text", "gizmo_rid", "position", "text", "color", "font_size"), &Gizmos::draw_2D_text, DEFVAL(Color(1.0f, 1.0f, 1.0f, 1.0f)),
                                DEFVAL(12));
    ClassDB::bind_static_method("Gizmos2D", D_METHOD("apply_shader_on_gizmo", "gizmo_rid", "material"), &Gizmos::apply_shader_on_2D_gizmo);
}
