#ifndef GIZMOS_HPP
#define GIZMOS_HPP

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/text_server_manager.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot
{
/// Propriétaire UNIQUE des ressources RenderingServer partagées par les gizmos 3D : registres (mesh et
// matériaux par instance), shader de texte, matériau par défaut. Remplace les anciennes variables
// statiques, détruites après le moteur au déchargement de la bibliothèque (RID fuités, risque de crash).
// Créé à la première utilisation, détruit au déchargement de CHAQUE module qui l'utilise (destroy() dans son
// registre : tool_editor_register.cpp, movement_physics_register.cpp...) ; ses RID sont libérés par le
// destructeur. Les RID d'instance (gizmos) restent à la charge de leur propriétaire (GizmoHost,
// HandlersManager, DebugGizmos, script).
class GizmoResources
{
  private:
    inline static GizmoResources *singleton = nullptr; // pointeur : destruction triviale au déchargement

  public:
    // Classe C++ ordinaire (pas une classe Godot) : chaque bibliothèque qui inclut ce header a son propre
    // exemplaire sans enregistrer de classe (deux bibliothèques ne peuvent pas enregistrer le même nom).
    ~GizmoResources()
    {
        RenderingServer *rs = RenderingServer::get_singleton();
        if (rs)
        {
            Array meshes = mesh_rids.values();
            for (int i = 0; i < meshes.size(); i++)
            {
                RID mesh = meshes[i];
                if (mesh.is_valid())
                {
                    rs->free_rid(mesh);
                }
            }
            Array glyph_lists = text_materials.values();
            for (int i = 0; i < glyph_lists.size(); i++)
            {
                Array list = glyph_lists[i];
                for (int j = 0; j < list.size(); j++)
                {
                    RID material = list[j];
                    if (material.is_valid())
                    {
                        rs->free_rid(material);
                    }
                }
            }
            if (text_shader.is_valid())
            {
                rs->free_rid(text_shader);
            }
        }
        mesh_rids.clear();
        text_materials.clear();
        materials.clear();
        text_shader = RID();
        default_material.unref();
    }

    Dictionary mesh_rids;      // RID d'instance -> RID de mesh
    Dictionary materials;      // RID d'instance -> Ref<StandardMaterial3D> (draw_3D_icon)
    Dictionary text_materials; // RID d'instance -> Array de RID de matériaux de glyphes (draw_3D_text)
    RID text_shader;
    Ref<StandardMaterial3D> default_material;

    static GizmoResources *get_singleton()
    {
        if (!singleton)
        {
            singleton = memnew(GizmoResources);
        }
        return singleton;
    }
    // À appeler au déchargement du module (niveau SCENE) : libère toutes les ressources partagées.
    static void destroy()
    {
        if (singleton)
        {
            memdelete(singleton);
            singleton = nullptr;
        }
    }
};

/// 🐛 Boîte à outils interne C++ regroupant tous les gizmos 2D/3D et leurs registres.
// Règle commune : chaque draw_* efface d'abord le dessin précédent du gizmo (une forme par gizmo), sauf
// retours anticipés (trop peu de points, flèche de longueur nulle) qui laissent l'ancien dessin en place.
// draw_2D_capsule / draw_2D_origin (transform du canvas_item) et draw_3D_icon / draw_3D_text (transform et
// material_override de l'instance) modifient un état du gizmo que les autres draw_* ne réinitialisent pas.
// Définie dans le header (comme log_it) pour être utilisable depuis tous les modules.
// Les classes Gizmos2D et Gizmos3D exposent ces fonctions à GDScript.
// NOTE : chaque bibliothèque a ses propres registres 3D (GizmoResources) : un gizmo 3D doit être créé et
// libéré depuis le même module.
class Gizmos
{
  public:
    Gizmos() = delete; // Classe purement statique

    //--------------------------------------------------------------------------
    // 2D
    //--------------------------------------------------------------------------

    /// Crée un canvas_item rattaché au canvas du World2D de viewport (coordonnées du monde 2D). RID vide si échec.
    static RID create_2D_gizmo(Viewport *viewport);
    static void clear_2D_gizmo(const RID gizmo_rid); // Efface le visuel 2D d'un Gizmo
    static void free_2D_gizmo(const RID gizmo_rid);  // Libère de la mémoire associée au gizmo, à appeler lorsque le gizmo n'est plus nécessaire (ex:dans un exit_tree d'un Node)

    // Dessine une ligne 2D
    static void draw_2D_line(const RID &gizmo_rid, const Vector2 &from, const Vector2 &to, float width = 2.0, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool antialiasing = false);
    // Dessine une flèche 2D
    static void draw_2D_arrow(
        const RID &gizmo_rid, const Vector2 &from, const Vector2 &to, float arrow_head_size = 12.0f, float width = 1.0f, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool antialiasing = false);
    // Dessine un cercle 2D
    static void
    draw_2D_circle(const RID &gizmo_rid, const Vector2 &center, float radius, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool filled = false, float thickness = 2.0, bool antialiasing = false);
    // Dessine un rectangle 2D (contour ou plein)
    static void draw_2D_rectangle(const RID &gizmo_rid, const Rect2 &rect, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool filled = false, float thickness = 2.0, bool antialiasing = false);

    // Dessine le contour d'une capsule verticale centrée sur position (size.x = diamètre, size.y = hauteur totale).
    // Applique position comme transform du canvas_item (persiste pour les dessins suivants).
    static void draw_2D_capsule(const RID &gizmo_rid, const Vector2 &position, const Vector2 &size, const Color &color = Color(1.0, 1.0, 1.0, 1.0), float thickness = 2.0, bool antialiasing = false);
    // Dessine un polygone 2D fermé (contour) ou plein (polygone simple requis). < 2 points : rien, ancien dessin conservé.
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
    // Dessine une texture centrée sur position (size (0,0) = taille de la texture).
    static void draw_2D_icon(const RID &gizmo_rid,
                             const Vector2 &position,
                             Ref<Texture2D> icon,
                             const Vector2 &size = Vector2(0, 0),
                             RenderingServer::CanvasItemTextureFilter filter = RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
    // Écrit du texte sur une ligne avec la police de repli du thème ; position = début de la ligne de base.
    static void draw_2D_text(const RID &gizmo_rid, const Vector2 &position, const String &text, const Color &color = Color(1.0f, 1.0f, 1.0f, 1.0f), int font_size = 12);

    // ATTENTION : transmet material->_get_shader_rid(), qui renvoie un RID vide pour les matériaux natifs
    // (implémentation par défaut de godot-cpp) : le matériau n'est pas appliqué, il est retiré. Un material
    // nul fait planter le moteur (vérifié en headless).
    static void apply_shader_on_2D_gizmo(const RID &gizmo_rid, const Ref<Material> &material);

    //--------------------------------------------------------------------------
    // 3D
    // NOTE (portage depuis gizmos_manager.gd) : la grille 3D (update_grid_3d) a été explicitement
    // exclue de ce portage à la demande de l'utilisateur.
    //--------------------------------------------------------------------------

    /// Crée une instance dans le scenario du World3D de viewport (repli : viewport 3D n°0 de l'éditeur),
    /// avec le matériau par défaut (non éclairé, couleur des sommets, double face). RID vide si échec.
    static RID create_3D_gizmo(Viewport *viewport);
    static void clear_3D_gizmo(const RID &gizmo_rid);
    static void free_3D_gizmo(const RID &gizmo_rid);

    // Dessine une ligne 3D. width > 1.0 génère un tube plein plutôt qu'une PRIMITIVE_LINES fine.
    static void draw_3D_line(const RID &instance_rid, const Vector3 &from, const Vector3 &to, const Color &color = Color(1.0, 1.0, 1.0, 1.0), float width = 1.0f);

    // Dessine une flèche 3D : ligne se terminant par un cône orienté vers "to".
    static void draw_3D_arrow(const RID &instance_rid, const Vector3 &from, const Vector3 &to, const Color &color = Color(1.0, 1.0, 1.0, 1.0), float head_size = 0.3f, float width = 1.0f);

    // Dessine un cube 3D (parallélépipède) centré sur "center", en fil de fer ou plein.
    static void draw_3D_cube(const RID &instance_rid, const Vector3 &center, const Vector3 &size, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool filled = false, float width = 1.0f);

    // Dessine une sphère 3D : 3 anneaux orthogonaux (contour) ou UV-sphere pleine.
    static void
    draw_3D_sphere(const RID &instance_rid, const Vector3 &center, float radius, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool filled = false, int segments = 16, float width = 1.0f);

    // Dessine un cercle 3D plat centré sur "center", orienté selon "normal".
    static void draw_3D_circle(const RID &instance_rid,
                               const Vector3 &center,
                               float radius,
                               const Vector3 &normal = Vector3(0, 1, 0),
                               const Color &color = Color(1.0, 1.0, 1.0, 1.0),
                               bool filled = false,
                               int segments = 24,
                               float width = 1.0f);

    // Dessine un rectangle 3D plat centré sur "center", orienté selon "normal".
    static void draw_3D_rectangle(const RID &instance_rid,
                                  const Vector3 &center,
                                  const Vector2 &size,
                                  const Vector3 &normal = Vector3(0, 1, 0),
                                  const Color &color = Color(1.0, 1.0, 1.0, 1.0),
                                  bool filled = false,
                                  float width = 1.0f);

    // Dessine une capsule 3D alignée sur l'axe Y, height = hauteur totale. Contour : 2 anneaux + 4 lignes
    // verticales, sans dômes. Pleine (filled) : capsule complète avec ses deux demi-sphères.
    static void draw_3D_capsule(const RID &instance_rid, const Vector3 &center, float radius, float height, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool filled = false, float width = 1.0f);

    // Dessine un polygone 3D coplanaire et convexe : contour fermé, ou plein (triangulation en éventail depuis points[0]).
    static void draw_3D_polygon(const RID &instance_rid, const PackedVector3Array &points, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool filled = false, float width = 1.0f);

    // Dessine un chemin 3D avec les mêmes options que draw_2D_path (à l'exception de
    // l'antialiasing, qui n'a pas de sens ici) : flèches au lieu de lignes, sous-division
    // par segment, espacement, lissage Bézier, affichage de sphères sur les points d'origine.
    static void draw_3D_path(const RID &instance_rid,
                             const PackedVector3Array &points,
                             bool as_arrows = false,
                             int segments_per_line = 1,
                             float spacing = 0.0f,
                             bool use_bezier = false,
                             bool show_points = false,
                             float point_radius = 0.1f,
                             float width = 2.0f,
                             const Color &color = Color(1.0, 1.0, 1.0, 1.0),
                             float arrow_head_size = 12.0f);

    // Dessine un repère d'origine 3D : 3 flèches représentant les axes locaux de "transform"
    // (X rouge, Y vert, Z bleu).
    static void draw_3D_origin(const RID &instance_rid, const Transform3D &transform = Transform3D(), float size = 1.0f, float head_size = 0.2f, float width = 2.0f);

    // Dessine le frustum (tronc de pyramide, 12 arêtes) représentant le champ de vision d'une caméra virtuelle.
    static void draw_3D_camera_frustum(const RID &instance_rid, const Transform3D &transform, float fov, float z_near, float z_far, float aspect, const Color &color = Color(1.0, 0.0, 1.0, 1.0));

    // Dessine une icône 3D en billboard (toujours face à la caméra, sans test de profondeur).
    static void draw_3D_icon(
        const RID &instance_rid, const Vector3 &position, Ref<Texture2D> texture, const Vector2 &size = Vector2(1, 1), BaseMaterial3D::TextureFilter filter = BaseMaterial3D::TEXTURE_FILTER_NEAREST);

    // Remplace le matériau par défaut d'un gizmo 3D par un matériau personnalisé (shader inclus).
    static void apply_shader_on_3D_gizmo(const RID &instance_rid, const Ref<Material> &material);

    // Dessine du texte 3D en billboard, SANS AUCUN nœud/ressource "afficheur" de texte (pas de
    // Label, RichTextLabel, Label3D, TextMesh...) : contrairement à la première version de
    // cette méthode, il n'y a même plus de Viewport/CanvasItem offscreen. À la place, on
    // utilise directement TextServer (le service bas niveau qui gère la mise en forme et les
    // atlas de glyphes, et sur lequel Label3D lui-même s'appuie en interne) : le texte est mis
    // en forme via shaped_text_add_string(), puis chaque glyphe est placé comme un quad dont
    // l'UV pointe directement dans l'atlas de textures de la police (font_get_glyph_uv_rect /
    // font_get_glyph_texture_rid), sans jamais dessiner sur un canvas ni passer par un viewport.
    // Limites assumées : une seule ligne (pas de retour à la ligne automatique), texte de
    // gauche à droite (pas de prise en charge RTL/bidi).
    static void draw_3D_text(const RID &instance_rid, const Vector3 &position, const String &text, const Color &color = Color(1.0, 1.0, 1.0, 1.0), int font_size = 32, float pixel_size = 0.01f);

  private:
    //--------------------------------------------------------------------------
    // Helpers internes 2D
    //--------------------------------------------------------------------------

    // Dessine le contour d'un cercle avec une polyline (sans effacer le canvas_item)
    static void _draw_2D_polyline_circle(
        const RID &gizmo_rid, const Vector2 &center, float radius, const Color color = Color(1.0, 1.0, 1.0, 1.0), float thickness = 1.0, int segments = 24, bool antialiasing = false);
    // Dessine une flèche sans effacer le canvas_item
    static void _add_2D_arrow(const RID &gizmo_rid, const Vector2 &from, const Vector2 &to, float arrow_size, float width, const Color &color, bool antialiasing = false);

    //--------------------------------------------------------------------------
    // Registres internes 3D
    //--------------------------------------------------------------------------

    // Accès aux registres de GizmoResources (propriétaire unique, créé à la première utilisation)
    static Dictionary &_get_mesh_rids();
    static Dictionary &_get_materials();

    // draw_3D_text : liste (Array de RID) des matériaux de glyphes créés pour le dernier appel
    // sur un gizmo donné (un matériau par page d'atlas de police utilisée). Libérés et
    // recréés à chaque appel, car le texte peut changer.
    static Dictionary &_get_text_materials();

    // Shader RID unique et partagé par tous les gizmos de texte (aucune donnée par-instance
    // dans le shader lui-même, seul le paramètre de texture change par matériau).
    static RID &_get_text_shader();

    static Ref<StandardMaterial3D> &_get_default_3D_material();

    //--------------------------------------------------------------------------
    // Helpers internes 3D
    //--------------------------------------------------------------------------

    static RID _get_scenario(Viewport *viewport);

    // Récupère (ou crée) le mesh RID associé à une instance, et le vide s'il existe déjà.
    // Équivalent de _get_or_create_mesh_rid() dans gizmos_manager.gd — appelé au début de
    // chaque draw_3D_*.
    static RID _get_or_create_mesh_rid(const RID &instance_rid);

    // Génère un segment 3D "épais" : simple ligne (PRIMITIVE_LINES) si width <= 1.0,
    // sinon un petit tube en triangles (hexagonal). Équivalent de _add_3d_thick_line().
    static void _add_3D_thick_line(Ref<SurfaceTool> &st, const Vector3 &from, const Vector3 &to, const Color &color, float width);

    // Ajoute une flèche (ligne + cône) directement dans un SurfaceTool déjà ouvert en
    // PRIMITIVE_TRIANGLES — utilisé par draw_3D_origin et draw_3D_path (as_arrows = true).
    // Équivalent de _add_3d_arrow_to_surfacetool(). Force elle-même une largeur de tube
    // minimale (1.5) si width <= 1.0, car le SurfaceTool appelant est toujours en triangles.
    static void _add_3D_arrow_to_surfacetool(Ref<SurfaceTool> &st, const Vector3 &from, const Vector3 &to, const Color &color, float head_size, float width);

    // Ajoute une petite sphère UV pleine (marqueur de point) dans un SurfaceTool déjà ouvert
    // en PRIMITIVE_TRIANGLES — utilisée par draw_3D_path (show_points = true).
    static void _add_3D_point_marker(Ref<SurfaceTool> &st, const Vector3 &center, float radius, const Color &color);
};

//==============================================================================
// 2D
//==============================================================================

inline void Gizmos::_draw_2D_polyline_circle(const RID &gizmo_rid, const Vector2 &center, float radius, const Color color, float thickness, int segments, bool antialiasing)
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
        rs->canvas_item_add_polyline(gizmo_rid, points, colors, thickness, antialiasing);
    }
}

//------------------------------------------------------------------------------
// Permet de créer un gizmo 2D
inline RID Gizmos::create_2D_gizmo(Viewport *viewport)
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
    // Parenter le gizmo au canvas du viewport
    RenderingServer::get_singleton()->canvas_item_set_parent(gizmo_rid, viewport->find_world_2d()->get_canvas());

    return gizmo_rid; // Retourne le RID du gizmo créé
}
//------------------------------------------------------------------------------
// Permet d'effacer tout visuel 2D du gizmo
inline void Gizmos::clear_2D_gizmo(const RID gizmo_rid)
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

inline void Gizmos::free_2D_gizmo(const RID gizmo_rid)
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

inline void Gizmos::draw_2D_line(const RID &gizmo_rid, const Vector2 &from, const Vector2 &to, float width, const Color &color, bool antialiasing)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin "
                        "de la ligne[/color]");
        return;
    }
    RenderingServer::get_singleton()->canvas_item_clear(gizmo_rid); // Efface les dessins précédents du gizmo
    RenderingServer::get_singleton()->canvas_item_add_line(gizmo_rid, from, to, color, width, antialiasing);
}

inline void Gizmos::draw_2D_arrow(const RID &gizmo_rid, const Vector2 &from, const Vector2 &to, float arrow_size, float width, const Color &color, bool antialiasing)
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
    RenderingServer::get_singleton()->canvas_item_add_line(gizmo_rid, from, base_point, color, width, antialiasing);

    // Pointe Isocèle du triangle de la flèche
    Vector2 side = Vector2(-direction.y, direction.x) * (head_length * 0.5); // Crée un vecteur perpendiculaire pour les côtés du triangle
    Vector2 point1 = base_point + side;
    Vector2 point2 = base_point - side;

    RenderingServer::get_singleton()->canvas_item_add_primitive(gizmo_rid, PackedVector2Array{to, point1, point2}, PackedColorArray{color, color, color}, PackedVector2Array(), RID());
}

inline void Gizmos::draw_2D_circle(const RID &gizmo_rid, const Vector2 &center, float radius, const Color &color, bool filled, float thickness, bool antialiasing)
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
        _draw_2D_polyline_circle(gizmo_rid, center, radius, color, thickness, 24, antialiasing);
    }
}
inline void Gizmos::draw_2D_rectangle(const RID &gizmo_rid, const Rect2 &rect, const Color &color, bool filled, float thickness, bool antialiasing)
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
        RenderingServer::get_singleton()->canvas_item_add_polyline(gizmo_rid, points, {color}, thickness, antialiasing);
    }
}

inline void Gizmos::draw_2D_capsule(const RID &gizmo_rid, const Vector2 &position, const Vector2 &size, const Color &color, float thickness, bool antialiasing)
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
    RenderingServer::get_singleton()->canvas_item_add_polyline(gizmo_rid, points, {color}, thickness, antialiasing);
}

inline void Gizmos::draw_2D_polygon(const RID &gizmo_rid, const PackedVector2Array &points, const Color &color, bool filled, float thickness, bool antialiasing)
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

inline void Gizmos::draw_2D_path(const RID &gizmo_rid,
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
            rs->canvas_item_add_circle(gizmo_rid, points[i], point_radius * 1.5, color, antialiasing);
            rs->canvas_item_add_circle(gizmo_rid, points[i], point_radius, Color(1, 1, 1, 1), antialiasing);
        }
    }
}

// Helper privé/interne pour dessiner une flèche sans effacer le canvas_item
inline void Gizmos::_add_2D_arrow(const RID &gizmo_rid, const Vector2 &from, const Vector2 &to, float arrow_size, float width, const Color &color, bool antialiasing)
{
    Vector2 direction = (to - from).normalized();
    float distance = from.distance_to(to);

    if (distance <= 0.001f)
    {
        return;
    }

    float head_length = Math::min(arrow_size, distance);
    Vector2 base_point = to - direction * head_length;

    RenderingServer::get_singleton()->canvas_item_add_line(gizmo_rid, from, base_point, color, width, antialiasing);

    Vector2 side = Vector2(-direction.y, direction.x) * (head_length * 0.5f);
    Vector2 point1 = base_point + side;
    Vector2 point2 = base_point - side;

    RenderingServer::get_singleton()->canvas_item_add_primitive(gizmo_rid, PackedVector2Array{to, point1, point2}, PackedColorArray{color, color, color}, PackedVector2Array(), RID());
}

inline void Gizmos::draw_2D_origin(const RID &gizmo_rid, const Transform2D &transform, float length, float arrow_head_size, float cross_size, float width, const Color &cross_color, bool antialiasing)
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
    _add_2D_arrow(gizmo_rid, Vector2(0, 0), Vector2(length, 0), arrow_head_size, width, Color(1.0f, 0.0f, 0.0f), antialiasing);

    // Axe Y (Vert)
    _add_2D_arrow(gizmo_rid, Vector2(0, 0), Vector2(0, length), arrow_head_size, width, Color(0.0f, 1.0f, 0.0f), antialiasing);

    // Croix au centre (tournée à 45° / Math_PI * 0.25)
    if (cross_size > 0.0f)
    {
        float offset = (cross_size * 0.5f) * 0.70710678f; // cos(45°)

        rs->canvas_item_add_line(gizmo_rid, Vector2(-offset, -offset), Vector2(offset, offset), cross_color, width, antialiasing);
        rs->canvas_item_add_line(gizmo_rid, Vector2(-offset, offset), Vector2(offset, -offset), cross_color, width, antialiasing);
    }
}

inline void Gizmos::draw_2D_icon(const RID &gizmo_rid, const Vector2 &position, Ref<Texture2D> icon, const Vector2 &size, RenderingServer::CanvasItemTextureFilter filter)
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

inline void Gizmos::draw_2D_text(const RID &gizmo_rid, const Vector2 &position, const String &text, const Color &color, int font_size)
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

inline void Gizmos::apply_shader_on_2D_gizmo(const RID &gizmo_rid, const Ref<Material> &material)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour l'application de matériau[/color]");
        return;
    }

    RenderingServer::get_singleton()->canvas_item_set_material(gizmo_rid, material->_get_shader_rid());
}

//==============================================================================
// 3D
//==============================================================================

inline Dictionary &Gizmos::_get_mesh_rids() { return GizmoResources::get_singleton()->mesh_rids; }

inline Dictionary &Gizmos::_get_materials() { return GizmoResources::get_singleton()->materials; }

inline Dictionary &Gizmos::_get_text_materials() { return GizmoResources::get_singleton()->text_materials; }

inline RID &Gizmos::_get_text_shader()
{
    RID &text_shader = GizmoResources::get_singleton()->text_shader;
    if (!text_shader.is_valid())
    {
        text_shader = RenderingServer::get_singleton()->shader_create();
        // Shader minimal : échantillonne directement l'atlas de glyphes de la police (une
        // "page" de police = une texture) et multiplie par la couleur des sommets. "billboard"
        // n'est PAS un render_mode valide en GDShader (contrairement à la propriété du même nom
        // sur StandardMaterial3D) : le billboard complet est reproduit à la main dans vertex()
        // via la matrice de vue inversée, technique standard pour ce cas en shader spatial custom.
        RenderingServer::get_singleton()->shader_set_code(text_shader, "shader_type spatial;\n"
                                                                       "render_mode unshaded, cull_disabled, depth_draw_always, blend_mix;\n"
                                                                       "uniform sampler2D font_texture : source_color, filter_linear;\n"
                                                                       "void vertex() {\n"
                                                                       "    MODELVIEW_MATRIX = VIEW_MATRIX * mat4(INV_VIEW_MATRIX[0], INV_VIEW_MATRIX[1], INV_VIEW_MATRIX[2], MODEL_MATRIX[3]);\n"
                                                                       "}\n"
                                                                       "void fragment() {\n"
                                                                       "    vec4 tex_color = texture(font_texture, UV);\n"
                                                                       "    ALBEDO = tex_color.rgb * COLOR.rgb;\n"
                                                                       "    ALPHA = tex_color.a * COLOR.a;\n"
                                                                       "}\n");
    }
    return text_shader;
}

inline Ref<StandardMaterial3D> &Gizmos::_get_default_3D_material()
{
    Ref<StandardMaterial3D> &default_material = GizmoResources::get_singleton()->default_material;
    if (default_material.is_null())
    {
        default_material.instantiate();
        default_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
        default_material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
        default_material->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
    }
    return default_material;
}

inline RID Gizmos::create_3D_gizmo(Viewport *viewport)
{
    RenderingServer *rs = RenderingServer::get_singleton();
    RID scenario = _get_scenario(viewport);

    if (!scenario.is_valid())
    {
        print_error("Erreur : scenario invalide à cause du viewport invalide !");
        return RID();
    }

    RID instance_rid = rs->instance_create();
    rs->instance_set_scenario(instance_rid, scenario);
    rs->instance_geometry_set_material_override(instance_rid, _get_default_3D_material()->get_rid());

    return instance_rid;
}

inline void Gizmos::clear_3D_gizmo(const RID &gizmo_rid)
{
    if (!gizmo_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID du gizmo invalide pour l'effacement de visuel 3D[/color]");
        return;
    }

    RID mesh_rid = _get_mesh_rids().get(gizmo_rid, RID());
    if (mesh_rid.is_valid() && RenderingServer::get_singleton()->mesh_get_surface_count(mesh_rid) > 0)
    {
        RenderingServer::get_singleton()->mesh_clear(mesh_rid);
    }
}

inline void Gizmos::free_3D_gizmo(const RID &gizmo_rid)
{
    if (gizmo_rid.is_valid())
    {
        _get_materials().erase(gizmo_rid);

        RenderingServer *rs = RenderingServer::get_singleton();

        // draw_3D_text : un matériau par page d'atlas de police utilisée par le dernier texte
        // dessiné sur ce gizmo. Le shader (_get_text_shader) est partagé entre tous les gizmos
        // et n'est jamais libéré ici (il vit pour toute la durée du module).
        if (_get_text_materials().has(gizmo_rid))
        {
            Array glyph_materials = _get_text_materials()[gizmo_rid];
            for (int i = 0; i < glyph_materials.size(); i++)
            {
                RID material_rid = glyph_materials[i];
                if (material_rid.is_valid())
                {
                    rs->free_rid(material_rid);
                }
            }
            _get_text_materials().erase(gizmo_rid);
        }

        RID mesh_rid = _get_mesh_rids().get(gizmo_rid, RID());
        if (mesh_rid.is_valid())
        {
            rs->free_rid(mesh_rid);
            _get_mesh_rids().erase(gizmo_rid);
        }

        rs->free_rid(gizmo_rid);
    }
}

inline RID Gizmos::_get_scenario(Viewport *viewport)
{
    if (viewport)
    {
        Ref<World3D> world = viewport->find_world_3d();
        if (world != nullptr)
        {
            return world->get_scenario();
        }

        if (Engine::get_singleton()->is_editor_hint())
        {
            SubViewport *editor_viewport = EditorInterface::get_singleton()->get_editor_viewport_3d(0);
            if (editor_viewport != nullptr && editor_viewport->find_world_3d() != nullptr)
            {
                return editor_viewport->find_world_3d()->get_scenario();
            }
        }
    }

    print_line_rich("[color=red]Erreur : Impossible de trouver le monde 3D[/color]");
    return RID();
}

//------------------------------------------------------------------------------
// Helpers internes (portage direct de _get_or_create_mesh_rid / _add_3d_thick_line /
// _add_3d_arrow_to_surfacetool depuis gizmos_manager.gd)
//------------------------------------------------------------------------------

inline RID Gizmos::_get_or_create_mesh_rid(const RID &instance_rid)
{
    RenderingServer *rs = RenderingServer::get_singleton();
    RID mesh_rid = _get_mesh_rids().get(instance_rid, RID());
    if (!mesh_rid.is_valid())
    {
        mesh_rid = rs->mesh_create();
        _get_mesh_rids()[instance_rid] = mesh_rid;
        rs->instance_set_base(instance_rid, mesh_rid);
    }
    else
    {
        rs->mesh_clear(mesh_rid);
    }
    return mesh_rid;
}

inline void Gizmos::_add_3D_thick_line(Ref<SurfaceTool> &st, const Vector3 &from, const Vector3 &to, const Color &color, float width)
{
    if (width <= 1.0f)
    {
        st->set_color(color);
        st->add_vertex(from);
        st->set_color(color);
        st->add_vertex(to);
        return;
    }

    float radius = width * 0.005f;
    Vector3 dir = (to - from).normalized();
    if (dir == Vector3(0, 0, 0))
    {
        return;
    }

    Vector3 up = Math::abs(dir.y) < 0.99f ? Vector3(0, 1, 0) : Vector3(1, 0, 0);
    Vector3 side = dir.cross(up).normalized() * radius;
    Vector3 normal = dir.cross(side).normalized() * radius;

    const int sides = 6;
    for (int i = 0; i < sides; i++)
    {
        float a1 = i * 2.0f * Math_PI / sides;
        float a2 = (i + 1) * 2.0f * Math_PI / sides;

        Vector3 offset1 = side * Math::cos(a1) + normal * Math::sin(a1);
        Vector3 offset2 = side * Math::cos(a2) + normal * Math::sin(a2);

        st->set_color(color);
        st->add_vertex(from + offset1);
        st->set_color(color);
        st->add_vertex(to + offset1);
        st->set_color(color);
        st->add_vertex(to + offset2);

        st->set_color(color);
        st->add_vertex(from + offset1);
        st->set_color(color);
        st->add_vertex(to + offset2);
        st->set_color(color);
        st->add_vertex(from + offset2);
    }
}

inline void Gizmos::_add_3D_arrow_to_surfacetool(Ref<SurfaceTool> &st, const Vector3 &from, const Vector3 &to, const Color &color, float head_size, float width)
{
    Vector3 dir = (to - from).normalized();
    float dist = from.distance_to(to);
    if (dist <= 0.001f)
    {
        return;
    }
    float actual_head = Math::min(head_size, dist);
    Vector3 base_pt = to - dir * actual_head;
    // Ce SurfaceTool est TOUJOURS ouvert en PRIMITIVE_TRIANGLES par l'appelant (à cause du
    // cône ci-dessous) : on force donc _add_3D_thick_line à produire un tube (>1.0f) plutôt
    // qu'une ligne à 2 sommets, sinon on casse la règle "multiple de 3" du buffer.
    _add_3D_thick_line(st, from, base_pt, color, width <= 1.0f ? 1.5f : width);

    Vector3 ortho = dir.cross(Vector3(0, 1, 0));
    if (ortho.length_squared() < 0.001f)
    {
        ortho = dir.cross(Vector3(1, 0, 0));
    }
    ortho = ortho.normalized() * (actual_head * 0.3f);
    Vector3 ortho2 = dir.cross(ortho).normalized() * (actual_head * 0.3f);

    for (int j = 0; j < 6; j++)
    {
        float a1 = j * 2.0f * Math_PI / 6.0f;
        float a2 = (j + 1) * 2.0f * Math_PI / 6.0f;
        Vector3 cp1 = base_pt + ortho * Math::cos(a1) + ortho2 * Math::sin(a1);
        Vector3 cp2 = base_pt + ortho * Math::cos(a2) + ortho2 * Math::sin(a2);
        st->set_color(color);
        st->add_vertex(to);
        st->set_color(color);
        st->add_vertex(cp2);
        st->set_color(color);
        st->add_vertex(cp1);
    }
}

inline void Gizmos::_add_3D_point_marker(Ref<SurfaceTool> &st, const Vector3 &center, float radius, const Color &color)
{
    // Sphère UV basse résolution, suffisante pour un simple marqueur de point.
    const int segments = 8;
    const int rings = 4;

    for (int i = 0; i < rings; i++)
    {
        float v1 = static_cast<float>(i) / rings;
        float v2 = static_cast<float>(i + 1) / rings;
        float lat1 = Math_PI * (v1 - 0.5f);
        float lat2 = Math_PI * (v2 - 0.5f);

        for (int j = 0; j < segments; j++)
        {
            float u1 = static_cast<float>(j) / segments;
            float u2 = static_cast<float>(j + 1) / segments;
            float lon1 = 2.0f * Math_PI * u1;
            float lon2 = 2.0f * Math_PI * u2;

            Vector3 p1 = center + Vector3(Math::cos(lat1) * Math::cos(lon1), Math::sin(lat1), Math::cos(lat1) * Math::sin(lon1)) * radius;
            Vector3 p2 = center + Vector3(Math::cos(lat1) * Math::cos(lon2), Math::sin(lat1), Math::cos(lat1) * Math::sin(lon2)) * radius;
            Vector3 p3 = center + Vector3(Math::cos(lat2) * Math::cos(lon2), Math::sin(lat2), Math::cos(lat2) * Math::sin(lon2)) * radius;
            Vector3 p4 = center + Vector3(Math::cos(lat2) * Math::cos(lon1), Math::sin(lat2), Math::cos(lat2) * Math::sin(lon1)) * radius;

            st->set_color(color);
            st->add_vertex(p1);
            st->set_color(color);
            st->add_vertex(p2);
            st->set_color(color);
            st->add_vertex(p3);
            st->set_color(color);
            st->add_vertex(p1);
            st->set_color(color);
            st->add_vertex(p3);
            st->set_color(color);
            st->add_vertex(p4);
        }
    }
}

//------------------------------------------------------------------------------
// Primitives 3D
//------------------------------------------------------------------------------

inline void Gizmos::draw_3D_line(const RID &instance_rid, const Vector3 &from, const Vector3 &to, const Color &color, float width)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin de la ligne 3D[/color]");
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    Ref<SurfaceTool> st;
    st.instantiate();

    bool thin = width <= 1.0f;
    st->begin(thin ? Mesh::PRIMITIVE_LINES : Mesh::PRIMITIVE_TRIANGLES);
    _add_3D_thick_line(st, from, to, color, width);

    // Cas dégénéré : from == to avec width > 1.0f => _add_3D_thick_line ne produit aucun
    // sommet (direction nulle). Éviter d'envoyer une surface vide à RenderingServer.
    Array arr = st->commit_to_arrays();
    PackedVector3Array verts = arr[Mesh::ARRAY_VERTEX];
    if (verts.size() > 0)
    {
        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, thin ? RenderingServer::PRIMITIVE_LINES : RenderingServer::PRIMITIVE_TRIANGLES, arr);
    }
}

inline void Gizmos::draw_3D_arrow(const RID &instance_rid, const Vector3 &from, const Vector3 &to, const Color &color, float head_size, float width)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin de la flèche 3D[/color]");
        return;
    }

    Vector3 dir = (to - from).normalized();
    float dist = from.distance_to(to);
    if (dist <= 0.001f)
    {
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    Ref<SurfaceTool> st;
    st.instantiate();

    float actual_head = Math::min(head_size, dist);
    Vector3 base_pt = to - dir * actual_head;

    // Ligne (peut être fine ou tube, mais le cône est toujours en triangles ; on ouvre
    // donc systématiquement le SurfaceTool en PRIMITIVE_TRIANGLES, comme dans le GDScript d'origine)
    st->begin(Mesh::PRIMITIVE_TRIANGLES);
    _add_3D_thick_line(st, from, base_pt, color, width <= 1.0f ? 1.5f : width); // évite une ligne PRIMITIVE_LINES mélangée à des triangles

    Vector3 ortho = dir.cross(Vector3(0, 1, 0));
    if (ortho.length_squared() < 0.001f)
    {
        ortho = dir.cross(Vector3(1, 0, 0));
    }
    ortho = ortho.normalized() * (actual_head * 0.4f);
    Vector3 ortho2 = dir.cross(ortho).normalized() * (actual_head * 0.4f);

    const int segments = 8;
    for (int i = 0; i < segments; i++)
    {
        float a1 = i * 2.0f * Math_PI / segments;
        float a2 = (i + 1) * 2.0f * Math_PI / segments;
        Vector3 p1 = base_pt + ortho * Math::cos(a1) + ortho2 * Math::sin(a1);
        Vector3 p2 = base_pt + ortho * Math::cos(a2) + ortho2 * Math::sin(a2);

        // Cône
        st->set_color(color);
        st->add_vertex(to);
        st->set_color(color);
        st->add_vertex(p2);
        st->set_color(color);
        st->add_vertex(p1);
        // Base du cône
        st->set_color(color);
        st->add_vertex(base_pt);
        st->set_color(color);
        st->add_vertex(p1);
        st->set_color(color);
        st->add_vertex(p2);
    }

    RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
}

inline void Gizmos::draw_3D_cube(const RID &instance_rid, const Vector3 &center, const Vector3 &size, const Color &color, bool filled, float width)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin du cube 3D[/color]");
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    Ref<SurfaceTool> st;
    st.instantiate();
    Vector3 ext = size * 0.5f;

    if (!filled)
    {
        bool thin = width <= 1.0f;
        st->begin(thin ? Mesh::PRIMITIVE_LINES : Mesh::PRIMITIVE_TRIANGLES);

        Vector3 corners[8] = {center + Vector3(-ext.x, -ext.y, -ext.z), center + Vector3(ext.x, -ext.y, -ext.z), center + Vector3(ext.x, -ext.y, ext.z), center + Vector3(-ext.x, -ext.y, ext.z),
                              center + Vector3(-ext.x, ext.y, -ext.z),  center + Vector3(ext.x, ext.y, -ext.z),  center + Vector3(ext.x, ext.y, ext.z),  center + Vector3(-ext.x, ext.y, ext.z)};
        const int pairs[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

        for (int i = 0; i < 12; i++)
        {
            _add_3D_thick_line(st, corners[pairs[i][0]], corners[pairs[i][1]], color, width);
        }

        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, thin ? RenderingServer::PRIMITIVE_LINES : RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
    }
    else
    {
        st->begin(Mesh::PRIMITIVE_TRIANGLES);
        Vector3 faces[6][4] = {
            {Vector3(-ext.x, -ext.y, ext.z), Vector3(ext.x, -ext.y, ext.z), Vector3(ext.x, ext.y, ext.z), Vector3(-ext.x, ext.y, ext.z)},     // Avant
            {Vector3(ext.x, -ext.y, -ext.z), Vector3(-ext.x, -ext.y, -ext.z), Vector3(-ext.x, ext.y, -ext.z), Vector3(ext.x, ext.y, -ext.z)}, // Arrière
            {Vector3(-ext.x, ext.y, -ext.z), Vector3(-ext.x, ext.y, ext.z), Vector3(ext.x, ext.y, ext.z), Vector3(ext.x, ext.y, -ext.z)},     // Haut
            {Vector3(-ext.x, -ext.y, ext.z), Vector3(-ext.x, -ext.y, -ext.z), Vector3(ext.x, -ext.y, -ext.z), Vector3(ext.x, -ext.y, ext.z)}, // Bas
            {Vector3(ext.x, -ext.y, ext.z), Vector3(ext.x, -ext.y, -ext.z), Vector3(ext.x, ext.y, -ext.z), Vector3(ext.x, ext.y, ext.z)},     // Droite
            {Vector3(-ext.x, -ext.y, -ext.z), Vector3(-ext.x, -ext.y, ext.z), Vector3(-ext.x, ext.y, ext.z), Vector3(-ext.x, ext.y, -ext.z)}  // Gauche
        };

        for (int f = 0; f < 6; f++)
        {
            st->set_color(color);
            st->add_vertex(center + faces[f][0]);
            st->set_color(color);
            st->add_vertex(center + faces[f][1]);
            st->set_color(color);
            st->add_vertex(center + faces[f][2]);
            st->set_color(color);
            st->add_vertex(center + faces[f][0]);
            st->set_color(color);
            st->add_vertex(center + faces[f][2]);
            st->set_color(color);
            st->add_vertex(center + faces[f][3]);
        }

        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
    }
}

inline void Gizmos::draw_3D_sphere(const RID &instance_rid, const Vector3 &center, float radius, const Color &color, bool filled, int segments, float width)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin de la sphère 3D[/color]");
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    Ref<SurfaceTool> st;
    st.instantiate();

    if (!filled)
    {
        bool thin = width <= 1.0f;
        st->begin(thin ? Mesh::PRIMITIVE_LINES : Mesh::PRIMITIVE_TRIANGLES);

        for (int i = 0; i < segments; i++)
        {
            float a1 = i * 2.0f * Math_PI / segments;
            float a2 = (i + 1) * 2.0f * Math_PI / segments;

            _add_3D_thick_line(st, center + Vector3(Math::cos(a1), Math::sin(a1), 0) * radius, center + Vector3(Math::cos(a2), Math::sin(a2), 0) * radius, color, width);
            _add_3D_thick_line(st, center + Vector3(Math::cos(a1), 0, Math::sin(a1)) * radius, center + Vector3(Math::cos(a2), 0, Math::sin(a2)) * radius, color, width);
            _add_3D_thick_line(st, center + Vector3(0, Math::cos(a1), Math::sin(a1)) * radius, center + Vector3(0, Math::cos(a2), Math::sin(a2)) * radius, color, width);
        }

        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, thin ? RenderingServer::PRIMITIVE_LINES : RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
    }
    else
    {
        st->begin(Mesh::PRIMITIVE_TRIANGLES);
        int rings = segments / 2;
        for (int i = 0; i < rings; i++)
        {
            float v1 = static_cast<float>(i) / rings;
            float v2 = static_cast<float>(i + 1) / rings;
            float lat1 = Math_PI * (v1 - 0.5f);
            float lat2 = Math_PI * (v2 - 0.5f);

            for (int j = 0; j < segments; j++)
            {
                float u1 = static_cast<float>(j) / segments;
                float u2 = static_cast<float>(j + 1) / segments;
                float lon1 = 2.0f * Math_PI * u1;
                float lon2 = 2.0f * Math_PI * u2;

                Vector3 p1 = center + Vector3(Math::cos(lat1) * Math::cos(lon1), Math::sin(lat1), Math::cos(lat1) * Math::sin(lon1)) * radius;
                Vector3 p2 = center + Vector3(Math::cos(lat1) * Math::cos(lon2), Math::sin(lat1), Math::cos(lat1) * Math::sin(lon2)) * radius;
                Vector3 p3 = center + Vector3(Math::cos(lat2) * Math::cos(lon2), Math::sin(lat2), Math::cos(lat2) * Math::sin(lon2)) * radius;
                Vector3 p4 = center + Vector3(Math::cos(lat2) * Math::cos(lon1), Math::sin(lat2), Math::cos(lat2) * Math::sin(lon1)) * radius;

                st->set_color(color);
                st->add_vertex(p1);
                st->set_color(color);
                st->add_vertex(p2);
                st->set_color(color);
                st->add_vertex(p3);
                st->set_color(color);
                st->add_vertex(p1);
                st->set_color(color);
                st->add_vertex(p3);
                st->set_color(color);
                st->add_vertex(p4);
            }
        }

        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
    }
}

inline void Gizmos::draw_3D_circle(const RID &instance_rid, const Vector3 &center, float radius, const Vector3 &normal, const Color &color, bool filled, int segments, float width)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin du cercle 3D[/color]");
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    Ref<SurfaceTool> st;
    st.instantiate();

    Vector3 norm = normal.normalized();
    Vector3 up = Math::abs(norm.y) < 0.99f ? Vector3(0, 1, 0) : Vector3(1, 0, 0);
    Vector3 tangent = norm.cross(up).normalized();
    Vector3 bitangent = norm.cross(tangent).normalized();

    if (!filled)
    {
        bool thin = width <= 1.0f;
        st->begin(thin ? Mesh::PRIMITIVE_LINES : Mesh::PRIMITIVE_TRIANGLES);
        for (int i = 0; i < segments; i++)
        {
            float a1 = i * 2.0f * Math_PI / segments;
            float a2 = (i + 1) * 2.0f * Math_PI / segments;
            Vector3 p1 = center + (tangent * Math::cos(a1) + bitangent * Math::sin(a1)) * radius;
            Vector3 p2 = center + (tangent * Math::cos(a2) + bitangent * Math::sin(a2)) * radius;
            _add_3D_thick_line(st, p1, p2, color, width);
        }
        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, thin ? RenderingServer::PRIMITIVE_LINES : RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
    }
    else
    {
        st->begin(Mesh::PRIMITIVE_TRIANGLES);
        for (int i = 0; i < segments; i++)
        {
            float a1 = i * 2.0f * Math_PI / segments;
            float a2 = (i + 1) * 2.0f * Math_PI / segments;
            Vector3 p1 = center + (tangent * Math::cos(a1) + bitangent * Math::sin(a1)) * radius;
            Vector3 p2 = center + (tangent * Math::cos(a2) + bitangent * Math::sin(a2)) * radius;
            st->set_color(color);
            st->add_vertex(center);
            st->set_color(color);
            st->add_vertex(p1);
            st->set_color(color);
            st->add_vertex(p2);
        }
        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
    }
}

inline void Gizmos::draw_3D_rectangle(const RID &instance_rid, const Vector3 &center, const Vector2 &size, const Vector3 &normal, const Color &color, bool filled, float width)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin du rectangle 3D[/color]");
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    Ref<SurfaceTool> st;
    st.instantiate();

    Vector3 norm = normal.normalized();
    Vector3 up = Math::abs(norm.y) < 0.99f ? Vector3(0, 1, 0) : Vector3(1, 0, 0);
    Vector3 tangent = norm.cross(up).normalized() * (size.x * 0.5f);
    Vector3 bitangent = norm.cross(tangent).normalized() * (size.y * 0.5f);

    Vector3 p1 = center - tangent - bitangent;
    Vector3 p2 = center + tangent - bitangent;
    Vector3 p3 = center + tangent + bitangent;
    Vector3 p4 = center - tangent + bitangent;

    if (!filled)
    {
        bool thin = width <= 1.0f;
        st->begin(thin ? Mesh::PRIMITIVE_LINES : Mesh::PRIMITIVE_TRIANGLES);
        _add_3D_thick_line(st, p1, p2, color, width);
        _add_3D_thick_line(st, p2, p3, color, width);
        _add_3D_thick_line(st, p3, p4, color, width);
        _add_3D_thick_line(st, p4, p1, color, width);
        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, thin ? RenderingServer::PRIMITIVE_LINES : RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
    }
    else
    {
        st->begin(Mesh::PRIMITIVE_TRIANGLES);
        st->set_color(color);
        st->add_vertex(p1);
        st->set_color(color);
        st->add_vertex(p2);
        st->set_color(color);
        st->add_vertex(p3);
        st->set_color(color);
        st->add_vertex(p1);
        st->set_color(color);
        st->add_vertex(p3);
        st->set_color(color);
        st->add_vertex(p4);
        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
    }
}

inline void Gizmos::draw_3D_capsule(const RID &instance_rid, const Vector3 &center, float radius, float height, const Color &color, bool filled, float width)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin de la capsule 3D[/color]");
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    Ref<SurfaceTool> st;
    st.instantiate();
    float half_h = Math::max(height - radius * 2.0f, 0.0f) * 0.5f;

    if (filled)
    {
        // Capsule pleine : anneaux de la demi-sphère basse (pôle → équateur), puis de la demi-sphère haute
        // (équateur → pôle) ; le cylindre est la bande entre les deux équateurs. Triangles colorés.
        const int segs = 16;
        const int rings = 6; // anneaux par demi-sphère
        Vector<float> ring_y;
        Vector<float> ring_r;
        for (int j = 0; j <= rings; j++)
        {
            float angle = -Math_PI * 0.5f + j * (Math_PI * 0.5f) / rings;
            ring_y.push_back(center.y - half_h + Math::sin(angle) * radius);
            ring_r.push_back(Math::cos(angle) * radius);
        }
        for (int j = 0; j <= rings; j++)
        {
            float angle = j * (Math_PI * 0.5f) / rings;
            ring_y.push_back(center.y + half_h + Math::sin(angle) * radius);
            ring_r.push_back(Math::cos(angle) * radius);
        }
        st->begin(Mesh::PRIMITIVE_TRIANGLES);
        st->set_color(color);
        auto point = [&](int p_ring, int p_seg)
        {
            float a = p_seg * 2.0f * Math_PI / segs;
            return Vector3(center.x + Math::cos(a) * ring_r[p_ring], ring_y[p_ring], center.z + Math::sin(a) * ring_r[p_ring]);
        };
        for (int j = 0; j + 1 < ring_y.size(); j++)
        {
            for (int i = 0; i < segs; i++)
            {
                Vector3 a = point(j, i), b = point(j, i + 1), c = point(j + 1, i + 1), d = point(j + 1, i);
                st->add_vertex(a);
                st->add_vertex(b);
                st->add_vertex(c);
                st->add_vertex(a);
                st->add_vertex(c);
                st->add_vertex(d);
            }
        }
        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
        return;
    }

    // IMPORTANT : le choix de primitive doit suivre EXACTEMENT la même règle que
    // _add_3D_thick_line() en interne (width <= 1.0f => 2 sommets "ligne"), sinon on
    // obtient un nombre de sommets non multiple de 3 dans un buffer TRIANGLES
    // (ex. "Vertex amount must be a multiple of 3").
    bool as_lines = (width <= 1.0f);
    st->begin(as_lines ? Mesh::PRIMITIVE_LINES : Mesh::PRIMITIVE_TRIANGLES);

    Vector3 top = center + Vector3(0, half_h, 0);
    Vector3 bot = center - Vector3(0, half_h, 0);
    const int segs = 12;

    for (int i = 0; i < segs; i++)
    {
        float a1 = i * 2.0f * Math_PI / segs;
        float a2 = (i + 1) * 2.0f * Math_PI / segs;
        _add_3D_thick_line(st, top + Vector3(Math::cos(a1), 0, Math::sin(a1)) * radius, top + Vector3(Math::cos(a2), 0, Math::sin(a2)) * radius, color, width);
        _add_3D_thick_line(st, bot + Vector3(Math::cos(a1), 0, Math::sin(a1)) * radius, bot + Vector3(Math::cos(a2), 0, Math::sin(a2)) * radius, color, width);
    }

    _add_3D_thick_line(st, top + Vector3(radius, 0, 0), bot + Vector3(radius, 0, 0), color, width);
    _add_3D_thick_line(st, top + Vector3(-radius, 0, 0), bot + Vector3(-radius, 0, 0), color, width);
    _add_3D_thick_line(st, top + Vector3(0, 0, radius), bot + Vector3(0, 0, radius), color, width);
    _add_3D_thick_line(st, top + Vector3(0, 0, -radius), bot + Vector3(0, 0, -radius), color, width);

    RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, as_lines ? RenderingServer::PRIMITIVE_LINES : RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
}

inline void Gizmos::draw_3D_polygon(const RID &instance_rid, const PackedVector3Array &points, const Color &color, bool filled, float width)
{
    if (!instance_rid.is_valid() || points.size() < 3)
    {
        if (instance_rid.is_valid())
        {
            print_line_rich("[color=red]Erreur : au moins 3 points sont nécessaires pour le dessin du polygone 3D[/color]");
        }
        else
        {
            print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin du polygone 3D[/color]");
        }
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    Ref<SurfaceTool> st;
    st.instantiate();

    if (!filled)
    {
        bool thin = width <= 1.0f;
        st->begin(thin ? Mesh::PRIMITIVE_LINES : Mesh::PRIMITIVE_TRIANGLES);
        for (int i = 0; i < points.size(); i++)
        {
            Vector3 p1 = points[i];
            Vector3 p2 = points[(i + 1) % points.size()];
            _add_3D_thick_line(st, p1, p2, color, width);
        }
        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, thin ? RenderingServer::PRIMITIVE_LINES : RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
    }
    else
    {
        st->begin(Mesh::PRIMITIVE_TRIANGLES);
        Vector3 p0 = points[0];
        for (int i = 1; i < points.size() - 1; i++)
        {
            st->set_color(color);
            st->add_vertex(p0);
            st->set_color(color);
            st->add_vertex(points[i]);
            st->set_color(color);
            st->add_vertex(points[i + 1]);
        }
        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
    }
}

inline void Gizmos::draw_3D_path(const RID &instance_rid,
                                 const PackedVector3Array &points,
                                 bool as_arrows,
                                 int segments_per_line,
                                 float spacing,
                                 bool use_bezier,
                                 bool show_points,
                                 float point_radius,
                                 float width,
                                 const Color &color,
                                 float arrow_head_size)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin du chemin 3D[/color]");
        return;
    }

    if (points.size() < 2)
    {
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);

    // Génération du tableau de points (lissé si use_bezier est vrai) — logique identique à draw_2D_path.
    PackedVector3Array path_points;
    int subdivisions = Math::max(1, segments_per_line);

    if (use_bezier && points.size() >= 3)
    {
        path_points.append(points[0]);
        for (int i = 0; i < points.size() - 1; ++i)
        {
            Vector3 p0 = (i == 0) ? points[0] : (points[i] + points[i + 1]) * 0.5f;
            Vector3 p1 = points[i + 1];
            Vector3 p2 = (i + 2 < points.size()) ? (points[i + 1] + points[i + 2]) * 0.5f : points[i + 1];

            for (int s = 1; s <= subdivisions; ++s)
            {
                float t = static_cast<float>(s) / subdivisions;
                float omt = 1.0f - t;
                Vector3 interpolated = omt * omt * p0 + 2.0f * omt * t * p1 + t * t * p2;
                path_points.append(interpolated);
            }
        }
    }
    else
    {
        path_points = points;
    }

    // Géométrie principale (lignes ou flèches). as_arrows force PRIMITIVE_TRIANGLES (cônes) ;
    // sinon le choix suit "width", comme les autres primitives de ce fichier.
    Ref<SurfaceTool> st;
    st.instantiate();
    bool thin = (!as_arrows) && (width <= 1.0f);
    st->begin(thin ? Mesh::PRIMITIVE_LINES : Mesh::PRIMITIVE_TRIANGLES);

    // Traçage des segments avec gestion de l'espacement et des sous-divisions
    for (int i = 0; i < path_points.size() - 1; ++i)
    {
        Vector3 start = path_points[i];
        Vector3 end = path_points[i + 1];

        int steps = use_bezier ? 1 : subdivisions;

        for (int s = 0; s < steps; ++s)
        {
            float t1 = static_cast<float>(s) / steps;
            float t2 = static_cast<float>(s + 1) / steps;

            Vector3 p1 = start.lerp(end, t1);
            Vector3 p2 = start.lerp(end, t2);

            Vector3 dir = (p2 - p1).normalized();
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
                // _add_3D_arrow_to_surfacetool force elle-même une largeur de tube minimale
                // si width <= 1.0f, car ce SurfaceTool est ouvert en triangles.
                _add_3D_arrow_to_surfacetool(st, p1, p2, color, arrow_head_size, width);
            }
            else
            {
                _add_3D_thick_line(st, p1, p2, color, width);
            }
        }
    }

    // Si spacing (ou d'autres paramètres) fait que tous les segments sont ignorés (dist <=
    // 0.001f), le SurfaceTool ne contient aucun sommet : ne PAS appeler
    // mesh_add_surface_from_arrays dans ce cas, sinon RenderingServer refuse la surface
    // ("Mesh created without vertex array").
    Array path_arrays = st->commit_to_arrays();
    PackedVector3Array path_vertices = path_arrays[Mesh::ARRAY_VERTEX];
    if (path_vertices.size() > 0)
    {
        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, thin ? RenderingServer::PRIMITIVE_LINES : RenderingServer::PRIMITIVE_TRIANGLES, path_arrays);
    }
    // sur les points d'ORIGINE du chemin (pas les points interpolés par
    // le Bézier), comme les cercles de draw_2D_path. Ajoutés en tant que 2e surface du même
    // mesh (toujours en triangles) pour ne jamais mélanger deux types de primitives dans un
    // seul buffer.
    if (show_points)
    {
        Ref<SurfaceTool> st_points;
        st_points.instantiate();
        st_points->begin(Mesh::PRIMITIVE_TRIANGLES);

        for (int i = 0; i < points.size(); ++i)
        {
            _add_3D_point_marker(st_points, points[i], point_radius, color);
        }

        RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, st_points->commit_to_arrays());
    }
}

inline void Gizmos::draw_3D_origin(const RID &instance_rid, const Transform3D &transform, float size, float head_size, float width)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin de l'origine 3D[/color]");
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    Vector3 pos = transform.origin;
    Vector3 x_end = pos + transform.basis.get_column(0) * size;
    Vector3 y_end = pos + transform.basis.get_column(1) * size;
    Vector3 z_end = pos + transform.basis.get_column(2) * size;

    _add_3D_arrow_to_surfacetool(st, pos, x_end, Color(1.0, 0.0, 0.0), head_size, width);
    _add_3D_arrow_to_surfacetool(st, pos, y_end, Color(0.0, 1.0, 0.0), head_size, width);
    _add_3D_arrow_to_surfacetool(st, pos, z_end, Color(0.0, 0.0, 1.0), head_size, width);

    RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
}

inline void Gizmos::draw_3D_camera_frustum(const RID &instance_rid, const Transform3D &transform, float fov, float z_near, float z_far, float aspect, const Color &color)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin du frustum de caméra 3D[/color]");
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_LINES);

    float h_near = Math::tan(Math::deg_to_rad(fov) * 0.5f) * z_near;
    float w_near = h_near * aspect;
    float h_far = Math::tan(Math::deg_to_rad(fov) * 0.5f) * z_far;
    float w_far = h_far * aspect;

    Vector3 n_tl = transform.xform(Vector3(-w_near, h_near, -z_near));
    Vector3 n_tr = transform.xform(Vector3(w_near, h_near, -z_near));
    Vector3 n_br = transform.xform(Vector3(w_near, -h_near, -z_near));
    Vector3 n_bl = transform.xform(Vector3(-w_near, -h_near, -z_near));

    Vector3 f_tl = transform.xform(Vector3(-w_far, h_far, -z_far));
    Vector3 f_tr = transform.xform(Vector3(w_far, h_far, -z_far));
    Vector3 f_br = transform.xform(Vector3(w_far, -h_far, -z_far));
    Vector3 f_bl = transform.xform(Vector3(-w_far, -h_far, -z_far));

    Vector3 lines[24] = {n_tl, n_tr, n_tr, n_br, n_br, n_bl, n_bl, n_tl, f_tl, f_tr, f_tr, f_br, f_br, f_bl, f_bl, f_tl, n_tl, f_tl, n_tr, f_tr, n_br, f_br, n_bl, f_bl};

    for (int i = 0; i < 24; i++)
    {
        st->set_color(color);
        st->add_vertex(lines[i]);
    }

    RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_LINES, st->commit_to_arrays());
}

inline void Gizmos::draw_3D_icon(const RID &instance_rid, const Vector3 &position, Ref<Texture2D> texture, const Vector2 &size, BaseMaterial3D::TextureFilter filter)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin de l'icône 3D[/color]");
        return;
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    Vector2 ext = size * 0.5f;

    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    st->set_uv(Vector2(0, 1));
    st->add_vertex(Vector3(-ext.x, -ext.y, 0));
    st->set_uv(Vector2(1, 1));
    st->add_vertex(Vector3(ext.x, -ext.y, 0));
    st->set_uv(Vector2(1, 0));
    st->add_vertex(Vector3(ext.x, ext.y, 0));

    st->set_uv(Vector2(0, 1));
    st->add_vertex(Vector3(-ext.x, -ext.y, 0));
    st->set_uv(Vector2(1, 0));
    st->add_vertex(Vector3(ext.x, ext.y, 0));
    st->set_uv(Vector2(0, 0));
    st->add_vertex(Vector3(-ext.x, ext.y, 0));

    RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays());

    if (texture.is_valid())
    {
        Ref<StandardMaterial3D> mat;
        if (_get_materials().has(instance_rid))
        {
            mat = _get_materials()[instance_rid];
        }
        if (mat.is_null())
        {
            mat.instantiate();
            mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
            mat->set_billboard_mode(BaseMaterial3D::BILLBOARD_ENABLED);
            mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
            mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
            mat->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true); // Rend l'icône toujours visible à travers les maillages
            mat->set_texture_filter(filter);
            _get_materials()[instance_rid] = mat;
        }

        mat->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, texture);
        RenderingServer::get_singleton()->instance_geometry_set_material_override(instance_rid, mat->get_rid());
    }
    else
    {
        print_line_rich("[color=red]Erreur : la texture de l'icône n'est pas valide ou est null[/color]");
    }

    RenderingServer::get_singleton()->instance_set_transform(instance_rid, Transform3D(Basis(), position));
}

inline void Gizmos::draw_3D_text(const RID &instance_rid, const Vector3 &position, const String &text, const Color &color, int font_size, float pixel_size)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour le dessin du texte 3D[/color]");
        return;
    }

    Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
    if (!font.is_valid())
    {
        print_line_rich("[color=red]Erreur : la police par défaut n'est pas valide[/color]");
        return;
    }

    Ref<TextServer> ts = TextServerManager::get_singleton()->get_primary_interface();
    if (!ts.is_valid())
    {
        print_line_rich("[color=red]Erreur : aucune interface TextServer active[/color]");
        return;
    }

    RenderingServer *rs = RenderingServer::get_singleton();

    // Mise en forme du texte (une seule ligne) : shaped_text_add_string fait le travail de
    // shaping (kerning, ligatures, choix de police de repli si un glyphe manque), exactement
    // comme le fait Label3D en interne — mais ici sans jamais créer de nœud.
    RID shaped = ts->create_shaped_text();
    ts->shaped_text_add_string(shaped, text, font->get_rids(), font_size);
    ts->shaped_text_shape(shaped);

    Array glyphs = ts->shaped_text_get_glyphs(shaped);
    float ascent = ts->shaped_text_get_ascent(shaped);
    float total_width = ts->shaped_text_get_width(shaped);

    // Libère les matériaux de glyphes créés lors du DERNIER appel sur ce gizmo (un par page
    // d'atlas utilisée) avant d'en recréer : ces RID de matériau ne sont jamais partagés ni
    // réutilisés d'un texte à l'autre, contrairement au mesh_rid.
    if (_get_text_materials().has(instance_rid))
    {
        Array old_materials = _get_text_materials()[instance_rid];
        for (int i = 0; i < old_materials.size(); i++)
        {
            RID old_mat = old_materials[i];
            if (old_mat.is_valid())
            {
                rs->free_rid(old_mat);
            }
        }
        _get_text_materials().erase(instance_rid);
    }

    RID mesh_rid = _get_or_create_mesh_rid(instance_rid);
    // Un matériau par surface (par page d'atlas) est utilisé ci-dessous : il ne faut donc PAS
    // qu'un material_override (mis en place par create_3D_gizmo, ou par un appel précédent à
    // apply_shader_on_3D_gizmo) masque ces matériaux par-surface.
    rs->instance_geometry_set_material_override(instance_rid, RID());

    // Regroupe les glyphes par page d'atlas de police (RID de texture) : un SurfaceTool et,
    // au final, une surface de mesh + un matériau par page, car un même mesh ne peut pas
    // échantillonner plusieurs textures dans une seule surface.
    Vector<RID> page_textures;
    Vector<Ref<SurfaceTool>> page_tools;

    float pen_x = 0.0f;

    for (int i = 0; i < glyphs.size(); i++)
    {
        Dictionary g = glyphs[i];
        int glyph_index = g["index"];
        RID glyph_font_rid = g["font_rid"];
        int glyph_font_size = g["font_size"];
        float advance = g["advance"];
        int repeat = g.has("repeat") ? static_cast<int>(g["repeat"]) : 1;
        if (repeat < 1)
        {
            repeat = 1;
        }

        // Glyphe non visuel (espace, caractère de contrôle...) : on avance juste le curseur.
        if (glyph_index == 0 || !glyph_font_rid.is_valid())
        {
            pen_x += advance * repeat;
            continue;
        }

        Vector2i ts_size(glyph_font_size, 0);
        RID glyph_tex = ts->font_get_glyph_texture_rid(glyph_font_rid, ts_size, glyph_index);
        if (!glyph_tex.is_valid())
        {
            pen_x += advance * repeat;
            continue;
        }

        Vector2 gl_offset = ts->font_get_glyph_offset(glyph_font_rid, ts_size, glyph_index);
        Vector2 gl_size = ts->font_get_glyph_size(glyph_font_rid, ts_size, glyph_index);
        Rect2 gl_uv = ts->font_get_glyph_uv_rect(glyph_font_rid, ts_size, glyph_index);
        Vector2 atlas_size = ts->font_get_glyph_texture_size(glyph_font_rid, ts_size, glyph_index);

        if (atlas_size.x <= 0.0f || atlas_size.y <= 0.0f || gl_size.x <= 0.0f || gl_size.y <= 0.0f)
        {
            pen_x += advance * repeat;
            continue;
        }

        // Trouve (ou crée) le SurfaceTool associé à cette page d'atlas.
        int page_idx = -1;
        for (int p = 0; p < page_textures.size(); p++)
        {
            if (page_textures[p] == glyph_tex)
            {
                page_idx = p;
                break;
            }
        }
        if (page_idx < 0)
        {
            Ref<SurfaceTool> new_st;
            new_st.instantiate();
            new_st->begin(Mesh::PRIMITIVE_TRIANGLES);
            page_textures.push_back(glyph_tex);
            page_tools.push_back(new_st);
            page_idx = page_textures.size() - 1;
        }
        Ref<SurfaceTool> st = page_tools[page_idx];

        Vector2 uv_tl = Vector2(gl_uv.position.x / atlas_size.x, gl_uv.position.y / atlas_size.y);
        Vector2 uv_br = Vector2((gl_uv.position.x + gl_uv.size.x) / atlas_size.x, (gl_uv.position.y + gl_uv.size.y) / atlas_size.y);

        for (int r = 0; r < repeat; r++)
        {
            // Repère 3D : Y vers le haut. La police exprime ses coordonnées Y vers le bas
            // (comme un canvas_item), d'où l'inversion via "ascent - gl_offset.y - ...".
            float left = (pen_x + gl_offset.x) * pixel_size;
            float right = (pen_x + gl_offset.x + gl_size.x) * pixel_size;
            float top = (ascent - gl_offset.y) * pixel_size;
            float bottom = (ascent - gl_offset.y - gl_size.y) * pixel_size;

            st->set_color(color);
            st->set_uv(Vector2(uv_tl.x, uv_br.y));
            st->add_vertex(Vector3(left, bottom, 0));

            st->set_color(color);
            st->set_uv(Vector2(uv_br.x, uv_br.y));
            st->add_vertex(Vector3(right, bottom, 0));

            st->set_color(color);
            st->set_uv(Vector2(uv_br.x, uv_tl.y));
            st->add_vertex(Vector3(right, top, 0));

            st->set_color(color);
            st->set_uv(Vector2(uv_tl.x, uv_br.y));
            st->add_vertex(Vector3(left, bottom, 0));

            st->set_color(color);
            st->set_uv(Vector2(uv_br.x, uv_tl.y));
            st->add_vertex(Vector3(right, top, 0));

            st->set_color(color);
            st->set_uv(Vector2(uv_tl.x, uv_tl.y));
            st->add_vertex(Vector3(left, top, 0));

            pen_x += advance;
        }
    }

    ts->free_rid(shaped);

    // Centre le texte horizontalement sur "position" plutôt que de le poser à sa droite —
    // cohérent avec le reste des gizmos, qui sont centrés sur leur point d'ancrage.
    float center_offset_x = -(total_width * pixel_size) * 0.5f;

    Array new_materials;
    for (int p = 0; p < page_textures.size(); p++)
    {
        Array arrays = page_tools[p]->commit_to_arrays();
        PackedVector3Array verts = arrays[Mesh::ARRAY_VERTEX];
        if (verts.size() == 0)
        {
            continue;
        }

        for (int v = 0; v < verts.size(); v++)
        {
            verts.set(v, verts[v] + Vector3(center_offset_x, 0, 0));
        }
        arrays[Mesh::ARRAY_VERTEX] = verts;

        int surface_idx = rs->mesh_get_surface_count(mesh_rid);
        rs->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, arrays);

        // Un matériau RID brut par page d'atlas, jamais partagé entre gizmos ni entre pages :
        // seul le shader (RID unique, _get_text_shader) est commun à tous.
        RID material_rid = rs->material_create();
        rs->material_set_shader(material_rid, _get_text_shader());
        rs->material_set_param(material_rid, "font_texture", page_textures[p]);
        rs->mesh_surface_set_material(mesh_rid, surface_idx, material_rid);

        new_materials.push_back(material_rid);
    }

    if (new_materials.size() > 0)
    {
        _get_text_materials()[instance_rid] = new_materials;
    }

    rs->instance_set_transform(instance_rid, Transform3D(Basis(), position));
}
inline void Gizmos::apply_shader_on_3D_gizmo(const RID &instance_rid, const Ref<Material> &material)
{
    if (!instance_rid.is_valid())
    {
        print_line_rich("[color=red]Erreur : RID de l'instance invalide pour l'application de matériau[/color]");
        return;
    }

    RenderingServer::get_singleton()->instance_geometry_set_material_override(instance_rid, material.is_valid() ? material->get_rid() : RID());
}

} // namespace godot

#endif // GIZMOS_HPP
