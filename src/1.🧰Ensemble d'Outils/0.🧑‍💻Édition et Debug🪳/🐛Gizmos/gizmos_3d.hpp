#ifndef GIZMOS_3D_HPP
#define GIZMOS_3D_HPP

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot
{

// NOTE (portage depuis gizmos_manager.gd) : la grille 3D (update_grid_3d) et le texte 3D
// (update_text_3d, qui nécessite un SubViewport offscreen par gizmo) ont été explicitement
// exclus de ce portage à la demande de l'utilisateur. _get_text_viewports() reste présent
// (et vidé dans free_3D_gizmo) pour ne pas casser l'API existante, mais n'est alimenté par
// aucune méthode dans cette version.
class Gizmos3D : public RefCounted
{
    GDCLASS(Gizmos3D, RefCounted)

  private:
    // Getters statiques : les objets seront créés uniquement lors de leur premier appel
    static Dictionary &_get_mesh_rids();
    static Dictionary &_get_materials();
    static Dictionary &_get_text_viewports();
    static Ref<StandardMaterial3D> &_get_default_3D_material();

    static RID _get_scenario(Viewport *viewport);

    // Récupère (ou crée) le mesh RID associé à une instance, et le vide s'il existe déjà.
    // Équivalent de _get_or_create_mesh_rid() dans gizmos_manager.gd — appelé au début de
    // chaque draw_3D_*.
    static RID _get_or_create_mesh_rid(const RID &instance_rid);

    // Génère un segment 3D "épais" : simple ligne (PRIMITIVE_LINES) si width <= 1.0,
    // sinon un petit tube en triangles (hexagonal). Équivalent de _add_3d_thick_line().
    static void _add_3D_thick_line(Ref<SurfaceTool> &st, const Vector3 &from, const Vector3 &to, const Color &color, float width);

    // Ajoute une flèche (ligne + cône) directement dans un SurfaceTool déjà ouvert en
    // PRIMITIVE_TRIANGLES — utilisé par draw_3D_origin. Équivalent de
    // _add_3d_arrow_to_surfacetool().
    static void _add_3D_arrow_to_surfacetool(Ref<SurfaceTool> &st, const Vector3 &from, const Vector3 &to, const Color &color, float head_size, float width);

  protected:
    static void _bind_methods();

  public:
    Gizmos3D();
    ~Gizmos3D();

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

    // Dessine une capsule 3D (contour uniquement : 2 anneaux + 4 lignes verticales, sans dômes), alignée sur l'axe Y.
    // NOTE : comme dans le GDScript d'origine, "filled" ne change que le choix triangles/lignes, il n'existe
    // aucun mode plein réel (pas de dômes ni de faces générées).
    static void draw_3D_capsule(const RID &instance_rid, const Vector3 &center, float radius, float height, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool filled = false, float width = 1.0f);

    // Dessine un polygone 3D coplanaire et convexe : contour fermé, ou plein (triangulation en éventail depuis points[0]).
    static void draw_3D_polygon(const RID &instance_rid, const PackedVector3Array &points, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool filled = false, float width = 1.0f);

    // Dessine un chemin 3D : polyligne reliant les points, segment par segment.
    static void draw_3D_path(const RID &instance_rid, const PackedVector3Array &points, const Color &color = Color(0.0, 1.0, 0.0, 1.0), float width = 1.0f);

    // Variante de draw_3D_path où chaque segment se termine par un cône indiquant le sens de parcours.
    static void draw_3D_arrow_path(const RID &instance_rid, const PackedVector3Array &points, const Color &color = Color(0.0, 1.0, 0.0, 1.0), float head_size = 0.3f, float width = 1.0f);

    // Dessine un repère d'origine 3D : 3 flèches représentant les axes locaux de "transform"
    // (X rouge, Y vert, Z bleu).
    static void draw_3D_origin(const RID &instance_rid, const Transform3D &transform = Transform3D(), float size = 1.0f, float head_size = 0.2f, float width = 2.0f);

    // Dessine le frustum (tronc de pyramide, 12 arêtes) représentant le champ de vision d'une caméra virtuelle.
    static void draw_3D_camera_frustum(const RID &instance_rid, const Transform3D &transform, float fov, float z_near, float z_far, float aspect, const Color &color = Color(1.0, 0.0, 1.0, 1.0));

    // Dessine une icône 3D en billboard (toujours face à la caméra, sans test de profondeur).
    static void draw_3D_icon(
        const RID &instance_rid, const Vector3 &position, Ref<Texture2D> texture, const Vector2 &size = Vector2(1, 1), BaseMaterial3D::TextureFilter filter = BaseMaterial3D::TEXTURE_FILTER_NEAREST);

    // Remplace le matériau par défaut d'un gizmo 3D par un matériau personnalisé (shader inclus).
    static void apply_shader_on_gizmo(const RID &instance_rid, const Ref<Material> &material);
};

} // namespace godot

#endif // GIZMOS_3D_HPP