#include "gizmos.hpp"
#include "godot_cpp/classes/theme_db.hpp"
#include "godot_cpp/core/math_defs.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/variant/packed_vector2_array.hpp"
#include "godot_cpp/variant/rect2.hpp"
#include <cstdint>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>

#include <godot_cpp/variant/rid.hpp>
using namespace godot;

Gizmos::Gizmos()
{
    // Initialisation des propriétés et variables ici
}
Gizmos::~Gizmos()
{
    // Nettoyage des ressources ici
}
//------------------------------------------------------------------------------

// Permet de créer un gizmo 2D
RID Gizmos::create_2D_gizmos(Viewport *viewport)
{

    if (!viewport || viewport->find_world_2d() == nullptr)
    {
        print_line_rich("[color=red]Erreur : Viewport invalide pour la création du gizmo 2D[/color]");
        return RID(); // Retourne un RID vide en cas d'erreur
    }

    // Implémentation de la création d'un gizmo 2D ici
    // Vous pouvez utiliser les fonctions de Godot pour créer un gizmo 2D
    // et retourner son RID.
    RID gizmo_rid = RenderingServer::get_singleton()->canvas_item_create(); // Crée un RID vide pour l'instant
    // Parenter le gizmo à la vue (viewport)

    RID viewport_rid = viewport->get_viewport_rid();
    RenderingServer::get_singleton()->canvas_item_set_parent(gizmo_rid, viewport->find_world_2d()->get_canvas());

    return gizmo_rid; // Retourne le RID du gizmo créé
}
//------------------------------------------------------------------------------
RID Gizmos::create_2D_gizmos(Node *node_to_apply_to)
{
    if (!node_to_apply_to)
    {
        print_line_rich("[color=red]Erreur : Node invalide pour la création du gizmo 2D[/color]");
        return RID(); // Retourne un RID vide en cas d'erreur
    }

    Viewport *viewport = node_to_apply_to->get_viewport();
    if (!viewport || viewport->find_world_2d() == nullptr)
    {
        print_line_rich("[color=red]Erreur : Viewport invalide pour la création du "
                        "gizmo 2D[/color]");
        return RID(); // Retourne un RID vide en cas d'erreur
    }

    return create_2D_gizmos(viewport);
}

//------------------------------------------------------------------------------
// Permet d'effacer tout visuel 2D du gizmo
void Gizmos::clear_2D_gizmos(const RID gizmo_rid)
{
    // Implémentation de la suppression des gizmos ici
    // Vous pouvez utiliser les fonctions de Godot pour supprimer le gizmo
    // spécifié par gizmo_rid.
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour "
                        "l'effacement de visuel 2D[/color]");
        return;
    }
    RenderingServer::get_singleton()->canvas_item_clear(gizmo_rid);
}

void Gizmos::free_gizmos(const RID gizmo_rid)
{
    // Implémentation de la libération des ressources des gizmos ici
    // Vous pouvez utiliser les fonctions de Godot pour libérer les
    // ressources associées au gizmo spécifié par gizmo_rid.
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour la "
                        "libération[/color]");
        return;
    }
    RenderingServer::get_singleton()->free_rid(gizmo_rid);
}

void Gizmos::draw_2D_line(const RID &gizmo_rid, const Vector2 &from, const Vector2 &to, float width, const Color &color, bool antialiasing)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin "
                        "de la ligne[/color]");
        return;
    }
    RenderingServer::get_singleton()->canvas_item_clear(gizmo_rid); // Efface les dessins précédents du gizmo
    RenderingServer::get_singleton()->canvas_item_add_line(gizmo_rid, from, to, color, width);
}

void Gizmos::draw_2D_arrow(const RID &gizmo_rid, const Vector2 &from, const Vector2 &to, float arrow_size, float width, const Color &color, bool antialiasing)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin de la flèche[/color]");
        return;
    }

    RenderingServer::get_singleton()->canvas_item_clear(gizmo_rid); // Efface les dessins précédents du gizmo

    Vector2 direction = (to - from).normalized();
    float distance = from.distance_to(to);

    if (distance <= 0.001)
    {
        return; // Évite de dessiner une flèche si la distance est trop petite
    }

    float head_length = Math::min(arrow_size, distance);
    Vector2 base_point = to - direction * head_length;

    // Ligne jusqu'à la base du triangle de la flèche
    RenderingServer::get_singleton()->canvas_item_add_line(gizmo_rid, from, base_point, color, width);

    // Pointe Isocèle du triangle de la flèche
    Vector2 side = Vector2(-direction.y, direction.x) * (head_length * 0.5); // Crée un vecteur perpendiculaire pour les côtés du triangle
    Vector2 point1 = base_point + side;
    Vector2 point2 = base_point - side;

    RenderingServer::get_singleton()->canvas_item_add_primitive(gizmo_rid, PackedVector2Array{to, point1, point2}, PackedColorArray{color, color, color}, PackedVector2Array(), RID());
}

void Gizmos::draw_2D_circle(const RID &gizmo_rid, const Vector2 &center, float radius, const Color &color, bool filled, float thickness, bool antialiasing)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin du cercle[/color]");
        return;
    }

    RenderingServer::get_singleton()->canvas_item_clear(gizmo_rid);
    if (filled)
    {
        RenderingServer::get_singleton()->canvas_item_add_circle(gizmo_rid, center, radius, color, antialiasing);
    }
    else
    {
        _draw_2D_polyline_circle_2D(gizmo_rid, center, radius, color, thickness, 24, antialiasing);
    }
}
void Gizmos::draw_2D_rectangle(const RID &gizmo_rid, const Rect2 &rect, const Color &color, bool filled, float thickness, bool antialiasing)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin du rectangle[/color]");
        return;
    }
    RenderingServer::get_singleton()->canvas_item_clear(gizmo_rid);
    if (filled)
    {
        RenderingServer::get_singleton()->canvas_item_add_rect(gizmo_rid, rect, color, antialiasing);
    }
    else
    {
        PackedVector2Array points = {rect.position, Vector2(rect.get_end().x, rect.get_position().y), rect.get_end(), Vector2(rect.get_position().x, rect.get_end().y), rect.position};
        RenderingServer::get_singleton()->canvas_item_add_polyline(gizmo_rid, points, {color}, thickness);
    }
}

void Gizmos::draw_2D_capsule(const RID &gizmo_rid, const Vector2 &position, const Vector2 &size, const Color &color, float thickness, bool antialiasing)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin de capsule[/color]");
        return;
    }

    RenderingServer::get_singleton()->canvas_item_clear(gizmo_rid);

    RenderingServer::get_singleton()->canvas_item_set_transform(gizmo_rid, Transform2D(0, position));

    float radius = size.x * 0.5;
    float height = Math::max(size.y - size.x, 0.0f);
    float half_height = height * 0.5;

    PackedVector2Array points = PackedVector2Array{};
    uint16_t steps = 12;

    for (int i = 0; i < steps + 1; i++)
    {
        float angle = Math_PI + (i * Math_PI / steps);
        points.append(Vector2(cos(angle) * radius, -half_height + sin(angle) * radius));
    }

    for (int i = 0; i < steps + 1; i++)
    {
        float angle = i * Math_PI / steps;
        points.append(Vector2(cos(angle) * radius, half_height + sin(angle) * radius));
    }
    points.append(points[0]);
    RenderingServer::get_singleton()->canvas_item_add_polyline(gizmo_rid, points, {color}, thickness);
}

void Gizmos::draw_2D_polygon(const RID &gizmo_rid, const PackedVector2Array &points, const Color &color, bool filled, float thickness, bool antialiasing)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin de polygone[/color]");
        return;
    }

    if (points.size() < 2)
    {
        return;
    }

    RenderingServer *rs = RenderingServer::get_singleton();
    rs->canvas_item_clear(gizmo_rid);

    if (filled)
    {
        PackedColorArray colors = PackedColorArray();
        colors.resize(points.size());
        colors.fill(color);
        rs->canvas_item_add_polygon(gizmo_rid, points, colors);
    }
    else
    {
        PackedVector2Array closed = points; // Duplication du tableau
        if (points.size() > 0 && points[0] != points[points.size() - 1])
        {
            closed.append(points[0]);
        }

        PackedColorArray colors;
        colors.push_back(color);
        rs->canvas_item_add_polyline(gizmo_rid, closed, colors, thickness, antialiasing);
    }
}

void Gizmos::draw_2D_path(const RID &gizmo_rid,
                          const PackedVector2Array &points,
                          bool as_arrows,
                          int segments_per_line,
                          float spacing,
                          bool use_bezier,
                          bool show_points,
                          float point_radius,
                          float width,
                          const Color &color,
                          float arrow_head_size,
                          bool antialiasing)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin du chemin[/color]");
        return;
    }

    if (points.size() < 2)
    {
        return;
    }

    RenderingServer *rs = RenderingServer::get_singleton();
    rs->canvas_item_clear(gizmo_rid);

    // Génération du tableau de points (lisse si use_bezier est vrai)
    PackedVector2Array path_points;
    int subdivisions = Math::max(1, segments_per_line);

    if (use_bezier && points.size() >= 3)
    {
        path_points.append(points[0]);
        for (int i = 0; i < points.size() - 1; ++i)
        {
            Vector2 p0 = (i == 0) ? points[0] : (points[i] + points[i + 1]) * 0.5f;
            Vector2 p1 = points[i + 1];
            Vector2 p2 = (i + 2 < points.size()) ? (points[i + 1] + points[i + 2]) * 0.5f : points[i + 1];

            for (int s = 1; s <= subdivisions; ++s)
            {
                float t = static_cast<float>(s) / subdivisions;
                float omt = 1.0f - t;
                Vector2 interpolated = omt * omt * p0 + 2.0f * omt * t * p1 + t * t * p2;
                path_points.append(interpolated);
            }
        }
    }
    else
    {
        path_points = points;
    }

    // Traçage des segments avec gestion de l'espacement et des sous-divisions
    for (int i = 0; i < path_points.size() - 1; ++i)
    {
        Vector2 start = path_points[i];
        Vector2 end = path_points[i + 1];

        int steps = use_bezier ? 1 : subdivisions;

        for (int s = 0; s < steps; ++s)
        {
            float t1 = static_cast<float>(s) / steps;
            float t2 = static_cast<float>(s + 1) / steps;

            Vector2 p1 = start.lerp(end, t1);
            Vector2 p2 = start.lerp(end, t2);

            Vector2 dir = (p2 - p1).normalized();
            float dist = p1.distance_to(p2);

            if (spacing > 0.0f)
            {
                dist = Math::max(0.0f, dist - spacing);
                p2 = p1 + dir * dist;
            }

            if (dist <= 0.001f)
            {
                continue;
            }

            if (as_arrows)
            {
                float head_length = Math::min(arrow_head_size, dist);
                Vector2 base_point = p2 - dir * head_length;

                rs->canvas_item_add_line(gizmo_rid, p1, base_point, color, width);

                Vector2 side = Vector2(-dir.y, dir.x) * (head_length * 0.5f);
                Vector2 point1 = base_point + side;
                Vector2 point2 = base_point - side;

                rs->canvas_item_add_primitive(gizmo_rid, PackedVector2Array{p2, point1, point2}, PackedColorArray{color, color, color}, PackedVector2Array(), RID());
            }
            else
            {
                rs->canvas_item_add_line(gizmo_rid, p1, p2, color, width);
            }
        }
    }

    // Dessin des cercles sur les points d'origine du chemin
    if (show_points)
    {
        for (int i = 0; i < points.size(); ++i)
        {
            rs->canvas_item_add_circle(gizmo_rid, points[i], point_radius, color, antialiasing);
        }
    }
}

// Helper privé/interne pour dessiner une flèche sans effacer le canvas_item
static void _draw_arrow_primitive(const RID &gizmo_rid, const Vector2 &from, const Vector2 &to, float arrow_size, float width, const Color &color)
{
    Vector2 direction = (to - from).normalized();
    float distance = from.distance_to(to);

    if (distance <= 0.001f)
    {
        return;
    }

    float head_length = Math::min(arrow_size, distance);
    Vector2 base_point = to - direction * head_length;

    RenderingServer::get_singleton()->canvas_item_add_line(gizmo_rid, from, base_point, color, width);

    Vector2 side = Vector2(-direction.y, direction.x) * (head_length * 0.5f);
    Vector2 point1 = base_point + side;
    Vector2 point2 = base_point - side;

    RenderingServer::get_singleton()->canvas_item_add_primitive(gizmo_rid, PackedVector2Array{to, point1, point2}, PackedColorArray{color, color, color}, PackedVector2Array(), RID());
}

void Gizmos::draw_2D_origin(const RID &gizmo_rid, const Transform2D &transform, float length, float arrow_head_size, float cross_size, float width, const Color &cross_color, bool antialiasing)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin de l'origine[/color]");
        return;
    }

    RenderingServer *rs = RenderingServer::get_singleton();
    rs->canvas_item_clear(gizmo_rid);

    // Applique la transformation 2D au gizmo
    rs->canvas_item_set_transform(gizmo_rid, transform);

    // Axe X (Rouge)
    _draw_arrow_primitive(gizmo_rid, Vector2(0, 0), Vector2(length, 0), arrow_head_size, width, Color(1.0f, 0.0f, 0.0f));

    // Axe Y (Vert)
    _draw_arrow_primitive(gizmo_rid, Vector2(0, 0), Vector2(0, length), arrow_head_size, width, Color(0.0f, 1.0f, 0.0f));

    // Croix au centre (tournée à 45° / Math_PI * 0.25)
    if (cross_size > 0.0f)
    {
        float offset = (cross_size * 0.5f) * 0.70710678f; // cos(45°)

        rs->canvas_item_add_line(gizmo_rid, Vector2(-offset, -offset), Vector2(offset, offset), cross_color, width);
        rs->canvas_item_add_line(gizmo_rid, Vector2(-offset, offset), Vector2(offset, -offset), cross_color, width);
    }
}

void Gizmos::draw_2D_icon(const RID &gizmo_rid, const Vector2 &position, Ref<Texture2D> icon, const Vector2 &size, RenderingServer::CanvasItemTextureFilter filter)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin de l'icône[/color]");
        return;
    }

    RenderingServer::get_singleton()->canvas_item_clear(gizmo_rid);

    if (icon != nullptr)
    {
        Vector2 draw_size;
        if (size != Vector2(0, 0))
        {
            draw_size = size;
        }
        else
        {
            draw_size = icon->get_size();
        }
        Rect2 rect = Rect2(position - (draw_size * 0.5), draw_size);

        RenderingServer::get_singleton()->canvas_item_add_texture_rect(gizmo_rid, rect, icon->get_rid());
        RenderingServer::get_singleton()->canvas_item_set_default_texture_filter(gizmo_rid, filter);
    }
    else
    {
        print_line_rich("[color=red]Erreur : l'icône n'est pas valide ou est null[/color]");
    }
}

void Gizmos::draw_2D_text(const RID &gizmo_rid, const Vector2 &position, const String &text, const Color &color, int font_size)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin du texte[/color]");
        return;
    }
    // Efface les dessins précédents du gizmo
    RenderingServer::get_singleton()->canvas_item_clear(gizmo_rid);

    Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
    if (font.is_valid())
    {
        font->draw_string(gizmo_rid, position, text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color);
    }
    else
    {
        print_line_rich("[color=red]Erreur : la police par défaut n'est pas valide[/color]");
    }
}

void Gizmos::_bind_methods()
{
    // Déclaration des méthodes et propriétés à exposer à Godot ici

    ClassDB::bind_static_method("Gizmos", D_METHOD("create_2D_gizmos", "viewport"), static_cast<RID (*)(Viewport *)>(&Gizmos::create_2D_gizmos));
    ClassDB::bind_static_method("Gizmos", D_METHOD("create_2D_gizmos", "node_to_apply_to"), static_cast<RID (*)(Node *)>(&Gizmos::create_2D_gizmos));
    ClassDB::bind_static_method("Gizmos", D_METHOD("clear_2D_gizmos", "gizmo_rid"), &Gizmos::clear_2D_gizmos);

    ClassDB::bind_static_method("Gizmos", D_METHOD("free_gizmos", "gizmo_rid"), &Gizmos::free_gizmos);

    ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_line", "gizmo_rid", "from", "to", "width", "color", "antialiasing"), &Gizmos::draw_2D_line, DEFVAL(2.0), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)),
                                DEFVAL(false));
    ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_arrow", "gizmo_rid", "from", "to", "arrow_head_size", "width", "color", "antialiasing"), &Gizmos::draw_2D_arrow, DEFVAL(12.0f),
                                DEFVAL(1.0f), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_circle", "gizmo_rid", "center", "radius", "color", "filled", "thickness", "antialiasing"), &Gizmos::draw_2D_circle,
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false), DEFVAL(2.0), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_rectangle", "gizmo_rid", "rect", "color", "filled", "thickness", "antialiasing"), &Gizmos::draw_2D_rectangle,
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false), DEFVAL(2.0), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_capsule", "gizmo_rid", "position", "size", "color", "thickness", "antialiasing"), &Gizmos::draw_2D_capsule,
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(2.0), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_polygon", "gizmo_rid", "points", "color", "filled", "thickness", "antialiasing"), &Gizmos::draw_2D_polygon,
                                DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false), DEFVAL(2.0), DEFVAL(false));
    ClassDB::bind_static_method(
        "Gizmos",
        D_METHOD("draw_2D_path", "gizmo_rid", "points", "as_arrows", "segments_per_line", "spacing", "use_bezier", "show_points", "point_radius", "width", "color", "arrow_head_size", "antialiasing"),
        &Gizmos::draw_2D_path, DEFVAL(false), DEFVAL(1), DEFVAL(0.0f), DEFVAL(false), DEFVAL(false), DEFVAL(4.0f), DEFVAL(2.0f), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(12.0f), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_origin", "gizmo_rid", "transform", "length", "arrow_head_size", "cross_size", "width", "cross_color", "antialiasing"),
                                &Gizmos::draw_2D_origin, DEFVAL(Transform2D()), DEFVAL(32.0f), DEFVAL(8.0f), DEFVAL(8.0f), DEFVAL(2.0f), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false));
    ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_icon", "gizmo_rid", "position", "icon", "size", "filter"), &Gizmos::draw_2D_icon, DEFVAL(Vector2(0, 0)),
                                DEFVAL(RenderingServer::CanvasItemTextureFilter::CANVAS_ITEM_TEXTURE_FILTER_NEAREST));
    ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_text", "gizmo_rid", "position", "text", "color", "font_size"), &Gizmos::draw_2D_text, DEFVAL(Color(1.0f, 1.0f, 1.0f, 1.0f)), DEFVAL(12));
}