#ifndef MOVEMENT_DEBUG_GIZMOS_HPP
#define MOVEMENT_DEBUG_GIZMOS_HPP

// Méthodes inline de Gizmos (module Édition et Debug) : header seul, compilé dans cette bibliothèque.
#include "../../../1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos.hpp"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/circle_shape2d.hpp>
#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/sphere_shape3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
/// 🐞 Gizmos de débogage d'un nœud (classe C++ interne du module Mouvement & Physique).
///
/// Chaque méthode draw_* de Gizmos REMPLACE le contenu d'un gizmo : DebugGizmos tient donc un réservoir de
/// gizmos, un par forme. À chaque image : begin(nœud, 3d), une forme par appel (line, arrow, circle,
/// text...), puis end() qui vide les gizmos inutilisés. release() libère tout (debug_mode désactivé, sortie
/// de l'arbre). Les coordonnées sont globales (monde 2D ou 3D du viewport du nœud).
class DebugGizmos
{
  public:
    ~DebugGizmos() { release(); }

    void begin(Node *p_owner, bool p_3d)
    {
        Viewport *viewport = p_owner && p_owner->is_inside_tree() ? p_owner->get_viewport() : nullptr;
        if (p_3d != three_d || !viewport || ObjectID(viewport->get_instance_id()) != viewport_id)
        {
            release(); // viewport ou dimension changés : nouveaux gizmos
        }
        three_d = p_3d;
        viewport_id = viewport ? ObjectID(viewport->get_instance_id()) : ObjectID();
        used = 0;
    }

    void end()
    {
        for (uint32_t i = used; i < rids.size(); i++)
        {
            if (three_d)
            {
                Gizmos::clear_3D_gizmo(rids[i]);
            }
            else
            {
                RenderingServer::get_singleton()->canvas_item_clear(rids[i]);
            }
        }
    }

    void release()
    {
        if (RenderingServer::get_singleton())
        {
            for (uint32_t i = 0; i < rids.size(); i++)
            {
                if (three_d)
                {
                    Gizmos::free_3D_gizmo(rids[i]);
                }
                else
                {
                    Gizmos::free_2D_gizmo(rids[i]);
                }
            }
        }
        rids.clear();
        used = 0;
    }

    bool is_empty() const { return rids.is_empty(); }

    // 🖍️ 2D
    void line(const Vector2 &p_from, const Vector2 &p_to, const Color &p_color, float p_width = 2.0f) { _draw2(&Gizmos::draw_2D_line, p_from, p_to, p_width, p_color); }
    void arrow(const Vector2 &p_from, const Vector2 &p_to, const Color &p_color, float p_head = 8.0f, float p_width = 2.0f)
    {
        if (p_from.distance_squared_to(p_to) > 0.25f)
        {
            RID rid = _next();
            if (rid.is_valid())
            {
                Gizmos::draw_2D_arrow(rid, p_from, p_to, p_head, p_width, p_color, true);
            }
        }
    }
    void circle(const Vector2 &p_center, float p_radius, const Color &p_color, bool p_filled = false, float p_width = 2.0f)
    {
        RID rid = _next();
        if (rid.is_valid())
        {
            Gizmos::draw_2D_circle(rid, p_center, p_radius, p_color, p_filled, p_width, true);
        }
    }
    void polygon(const PackedVector2Array &p_points, const Color &p_color, bool p_filled = false, float p_width = 2.0f)
    {
        RID rid = p_points.size() >= 2 ? _next() : RID();
        if (rid.is_valid())
        {
            Gizmos::draw_2D_polygon(rid, p_points, p_color, p_filled, p_width, true);
        }
    }
    void path(const PackedVector2Array &p_points, const Color &p_color, bool p_show_points = false, float p_width = 2.0f)
    {
        RID rid = p_points.size() >= 2 ? _next() : RID();
        if (rid.is_valid())
        {
            Gizmos::draw_2D_path(rid, p_points, false, 1, 0.0f, false, p_show_points, 3.0f, p_width, p_color);
        }
    }
    void text(const Vector2 &p_position, const String &p_text, const Color &p_color = Color(1, 1, 1), int p_size = 12)
    {
        RID rid = _next();
        if (rid.is_valid())
        {
            Gizmos::draw_2D_text(rid, p_position, p_text, p_color, p_size);
        }
    }
    /// Contour de la première forme (rectangle, cercle) d'un nœud 2D.
    void shape_outline(Node *p_node, const Color &p_color)
    {
        for (int i = 0; p_node && i < p_node->get_child_count(); i++)
        {
            CollisionShape2D *shape = Object::cast_to<CollisionShape2D>(p_node->get_child(i));
            if (!shape || shape->get_shape().is_null())
            {
                continue;
            }
            Transform2D transform = shape->get_global_transform();
            Ref<RectangleShape2D> rectangle = shape->get_shape();
            Ref<CircleShape2D> round = shape->get_shape();
            if (rectangle.is_valid())
            {
                Vector2 half = rectangle->get_size() * 0.5f;
                PackedVector2Array corners;
                corners.push_back(transform.xform(Vector2(-half.x, -half.y)));
                corners.push_back(transform.xform(Vector2(half.x, -half.y)));
                corners.push_back(transform.xform(Vector2(half.x, half.y)));
                corners.push_back(transform.xform(Vector2(-half.x, half.y)));
                polygon(corners, p_color);
            }
            else if (round.is_valid())
            {
                circle(transform.get_origin(), round->get_radius() * transform.get_scale().x, p_color);
            }
            return;
        }
    }

    // 🧊 3D
    void line3(const Vector3 &p_from, const Vector3 &p_to, const Color &p_color, float p_width = 1.0f)
    {
        RID rid = _next();
        if (rid.is_valid())
        {
            Gizmos::draw_3D_line(rid, p_from, p_to, p_color, p_width);
        }
    }
    void arrow3(const Vector3 &p_from, const Vector3 &p_to, const Color &p_color, float p_head = 0.15f, float p_width = 2.0f)
    {
        if (p_from.distance_squared_to(p_to) > 0.0004f)
        {
            RID rid = _next();
            if (rid.is_valid())
            {
                Gizmos::draw_3D_arrow(rid, p_from, p_to, p_color, MIN(p_head, p_from.distance_to(p_to) * 0.5f), p_width);
            }
        }
    }
    void sphere3(const Vector3 &p_center, float p_radius, const Color &p_color, bool p_filled = false)
    {
        RID rid = _next();
        if (rid.is_valid())
        {
            Gizmos::draw_3D_sphere(rid, p_center, p_radius, p_color, p_filled, 16, 1.0f);
        }
    }
    void circle3(const Vector3 &p_center, float p_radius, const Vector3 &p_normal, const Color &p_color)
    {
        RID rid = _next();
        if (rid.is_valid())
        {
            Gizmos::draw_3D_circle(rid, p_center, p_radius, p_normal, p_color, false, 32, 1.0f);
        }
    }
    void rectangle3(const Vector3 &p_center, const Vector2 &p_size, const Vector3 &p_normal, const Color &p_color, bool p_filled = false)
    {
        RID rid = _next();
        if (rid.is_valid())
        {
            Gizmos::draw_3D_rectangle(rid, p_center, p_size, p_normal, p_color, p_filled, 1.0f);
        }
    }
    void cube3(const Vector3 &p_center, const Vector3 &p_size, const Color &p_color)
    {
        RID rid = _next();
        if (rid.is_valid())
        {
            Gizmos::draw_3D_cube(rid, p_center, p_size, p_color, false, 1.0f);
        }
    }
    void path3(const PackedVector3Array &p_points, const Color &p_color, bool p_show_points = false)
    {
        RID rid = p_points.size() >= 2 ? _next() : RID();
        if (rid.is_valid())
        {
            Gizmos::draw_3D_path(rid, p_points, false, 1, 0.0f, false, p_show_points, 0.05f, 1.0f, p_color);
        }
    }
    void origin3(const Transform3D &p_transform, float p_size = 0.6f)
    {
        RID rid = _next();
        if (rid.is_valid())
        {
            Gizmos::draw_3D_origin(rid, p_transform, p_size, 0.1f, 2.0f);
        }
    }
    void text3(const Vector3 &p_position, const String &p_text, const Color &p_color = Color(1, 1, 1), int p_size = 32)
    {
        RID rid = _next();
        if (rid.is_valid())
        {
            Gizmos::draw_3D_text(rid, p_position, p_text, p_color, p_size, 0.008f);
        }
    }
    /// Contour de la première forme (boîte, sphère) d'un nœud 3D (boîte : sans rotation).
    void shape_outline3(Node *p_node, const Color &p_color)
    {
        for (int i = 0; p_node && i < p_node->get_child_count(); i++)
        {
            CollisionShape3D *shape = Object::cast_to<CollisionShape3D>(p_node->get_child(i));
            if (!shape || shape->get_shape().is_null())
            {
                continue;
            }
            Transform3D transform = shape->get_global_transform();
            Ref<BoxShape3D> box = shape->get_shape();
            Ref<SphereShape3D> ball = shape->get_shape();
            if (box.is_valid())
            {
                cube3(transform.origin, box->get_size() * transform.basis.get_scale(), p_color);
            }
            else if (ball.is_valid())
            {
                sphere3(transform.origin, ball->get_radius() * transform.basis.get_scale().x, p_color);
            }
            return;
        }
    }

  private:
    LocalVector<RID> rids;
    uint32_t used = 0;
    bool three_d = false;
    ObjectID viewport_id;

    RID _next()
    {
        if (used < rids.size())
        {
            return rids[used++];
        }
        Viewport *viewport = Object::cast_to<Viewport>(ObjectDB::get_instance(viewport_id));
        if (!viewport)
        {
            return RID();
        }
        RID rid = three_d ? Gizmos::create_3D_gizmo(viewport) : Gizmos::create_2D_gizmo(viewport);
        if (!rid.is_valid())
        {
            return RID();
        }
        if (!three_d)
        {
            RenderingServer::get_singleton()->canvas_item_set_z_index(rid, 4000); // par-dessus la scène
        }
        rids.push_back(rid);
        used++;
        return rid;
    }

    template <typename F>
    void _draw2(F p_function, const Vector2 &p_from, const Vector2 &p_to, float p_width, const Color &p_color)
    {
        RID rid = _next();
        if (rid.is_valid())
        {
            p_function(rid, p_from, p_to, p_width, p_color, true);
        }
    }
};

/// Propriété debug_mode d'une classe qui n'est pas un MovementComponent (à placer à la FIN de la classe :
/// la macro se termine en section private). Dessin : debug.begin(this, 3d) ... debug.end() quand debug_mode.
#define DEBUG_MODE_DECLARATIONS()                     \
  public:                                             \
    void set_debug_mode(bool p_debug)                 \
    {                                                 \
        debug_mode = p_debug;                         \
        if (!debug_mode)                              \
        {                                             \
            debug.release();                          \
        }                                             \
    }                                                 \
    bool get_debug_mode() const { return debug_mode; } \
                                                      \
  private:                                            \
    bool debug_mode = false;                          \
    DebugGizmos debug;

/// Liaison de debug_mode (groupe « 🐞 Débogage ») pour une classe utilisant DEBUG_MODE_DECLARATIONS.
template <typename T>
void bind_debug_mode()
{
    ClassDB::bind_method(D_METHOD("set_debug_mode", "debug"), &T::set_debug_mode);
    ClassDB::bind_method(D_METHOD("get_debug_mode"), &T::get_debug_mode);
    ClassDB::add_property_group(T::get_class_static(), String::utf8("🐞 Débogage"), "");
    ClassDB::add_property(T::get_class_static(), PropertyInfo(Variant::BOOL, "debug_mode"), "set_debug_mode", "get_debug_mode");
}

/// Couleurs communes des gizmos de débogage du module.
namespace debug_colors
{
const Color VELOCITY(0.3f, 1.0f, 0.4f);
const Color INPUT(1.0f, 0.85f, 0.25f);
const Color FORCE(0.35f, 0.8f, 1.0f);
const Color NORMAL(1.0f, 0.35f, 0.35f);
const Color ZONE(0.6f, 0.75f, 1.0f, 0.8f);
const Color TEXT(1.0f, 1.0f, 1.0f);
const Color HEADING(1.0f, 0.5f, 0.9f);
} // namespace debug_colors
} // namespace godot

#endif // MOVEMENT_DEBUG_GIZMOS_HPP
