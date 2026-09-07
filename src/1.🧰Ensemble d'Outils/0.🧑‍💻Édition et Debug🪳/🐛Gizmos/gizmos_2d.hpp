#ifndef GIZMOS_HPP
#define GIZMOS_HPP

#include "godot_cpp/classes/canvas_item.hpp"
#include "godot_cpp/variant/packed_vector2_array.hpp"
#include "godot_cpp/variant/rect2.hpp"
#include "godot_cpp/variant/transform2d.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot
{
// Gizmos2DServer
class Gizmos2D : public RefCounted
{
    GDCLASS(Gizmos2D, RefCounted)
  private:
    static Ref<StandardMaterial3D> _default_material;
    static Ref<StandardMaterial3D> get_default_material()
    {
        if (!_default_material.is_valid())
        {
            _default_material.instantiate();
            _default_material->set_albedo(Color(1, 0, 0)); // Couleur rouge par défaut
        }
        return _default_material;
    }
    static RID get_scenario(Viewport *viewport)
    {
        if (viewport)
        {
            Ref<World3D> world_3d = viewport->find_world_3d();
            if (world_3d.is_valid())
            {
                return world_3d->get_scenario();
            }

            if (Engine::get_singleton()->is_editor_hint())
            {
                EditorInterface *editor_interface = EditorInterface::get_singleton();
                SubViewport *editor_viewport = editor_interface ? editor_interface->get_editor_viewport_3d() : nullptr;
                if (editor_viewport && editor_viewport->find_world_3d() != nullptr)
                {
                    return editor_viewport->find_world_3d()->get_scenario();
                }
            }
        }
        return RID();
    }

    static void _draw_2D_polyline_circle_2D(
        const RID &gizmo_rid, const Vector2 &center, float radius, const Color color = Color(1.0, 1.0, 1.0, 1.0), float thickness = 1.0, int segments = 24, bool antialiasing = false)
    {
        PackedVector2Array points = PackedVector2Array{};
        points.resize(segments + 1);
        for (int i = 0; i <= segments; i++)
        {
            float angle = i * 2.0 * Math_PI / segments;
            points.set(i, center + Vector2(Math::cos(angle), Math::sin(angle)) * radius);
        }
        PackedColorArray colors;
        colors.push_back(color);

        RenderingServer *rs = RenderingServer::get_singleton();
        if (rs)
        {
            rs->canvas_item_add_polyline(gizmo_rid, points, colors, thickness);
        }
    }

  protected:
    static void _bind_methods();

  public:
    Gizmos2D();
    ~Gizmos2D();

    static RID create_2D_gizmos(Node *node_to_apply_to);
    static RID create_2D_gizmos(Viewport *viewport);
    static void clear_2D_gizmos(const RID gizmo_rid); // Efface le visuel 2D d'un Gizmo
    static void free_gizmos(const RID gizmo_rid);     // Libère de la mémoire associée au gizmo, à appeler lorsque le gizmo n'est plus nécessaire (ex:dans un exit_tree d'un Node)

    // Dessine une ligne 2D
    static void draw_2D_line(const RID &gizmo_rid, const Vector2 &from, const Vector2 &to, float width = 2.0, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool antialiasing = false);
    // Dessine une flèche 2D
    static void draw_2D_arrow(
        const RID &gizmo_rid, const Vector2 &from, const Vector2 &to, float arrow_head_size = 12.0f, float width = 1.0f, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool antialiasing = false);
    // Dessine un cercle 2D
    static void
    draw_2D_circle(const RID &gizmo_rid, const Vector2 &center, float radius, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool filled = false, float thickness = 2.0, bool antialiasing = false);
    // static void draw_2D_rectangle(RID gizmo_rid, const Vector2 &top_left, const Vector2 &bottom_right, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool antialiasing = false);
    static void draw_2D_rectangle(const RID &gizmo_rid, const Rect2 &rect, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool filled = false, float thickness = 2.0, bool antialiasing = false);

    static void draw_2D_capsule(const RID &gizmo_rid, const Vector2 &position, const Vector2 &size, const Color &color = Color(1.0, 1.0, 1.0, 1.0), float thickness = 2.0, bool antialiasing = false);
    static void draw_2D_polygon(const RID &gizmo_rid, const PackedVector2Array &points, const Color &color = Color(1, 1, 1, 1), bool filled = false, float width = 2.0f, bool antialiasing = false);

    // Dessine un chemin 2D avec options d'espacement, Bézier et affichage des points
    static void draw_2D_path(const RID &gizmo_rid,
                             const PackedVector2Array &points,
                             bool as_arrows = false,
                             int segments_per_line = 1,
                             float spacing = 0.0f,
                             bool use_bezier = false,
                             bool show_points = false,
                             float point_radius = 4.0f,
                             float width = 2.0f,
                             const Color &color = Color(1.0, 1.0, 1.0, 1.0),
                             float arrow_head_size = 12.0f,
                             bool antialiasing = false);

    // Dessine un repère d'origine 2D (Axe X rouge, Axe Y vert + croix au centre à 45°)
    static void draw_2D_origin(const RID &gizmo_rid,
                               const Transform2D &transform = Transform2D(),
                               float length = 32.0f,
                               float arrow_head_size = 8.0f,
                               float cross_size = 8.0f,
                               float width = 2.0f,
                               const Color &cross_color = Color(1.0, 1.0, 1.0, 1.0),
                               bool antialiasing = false);
    static void draw_2D_icon(const RID &gizmo_rid,
                             const Vector2 &position,
                             Ref<Texture2D> icon,
                             const Vector2 &size = Vector2(0, 0),
                             RenderingServer::CanvasItemTextureFilter filter = RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
    static void draw_2D_text(const RID &gizmo_rid, const Vector2 &position, const String &text, const Color &color = Color(1.0f, 1.0f, 1.0f, 1.0f), int font_size = 12);

    static void apply_shader_on_gizmo(const RID &gizmo_rid, const Ref<Material> &material);
};

} // namespace godot

#endif // GIZMOS_HPP
