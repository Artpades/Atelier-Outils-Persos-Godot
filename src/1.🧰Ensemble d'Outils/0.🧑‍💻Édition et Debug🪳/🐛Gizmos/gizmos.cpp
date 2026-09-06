#include "gizmos.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
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

void Gizmos::draw_2D_line(RID gizmo_rid, const Vector2 &from, const Vector2 &to, float width, const Color &color, bool antialiasing)
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

void Gizmos::draw_2D_arrow(RID gizmo_rid, const Vector2 &from, const Vector2 &to, float arrow_size, float width, const Color &color, bool antialiasing)
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

void Gizmos::draw_2D_circle(RID gizmo_rid, const Vector2 &center, float radius, const Color &color, bool filled, float thickness, bool antialiasing)
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

void Gizmos::_bind_methods()
{
  // Déclaration des méthodes et propriétés à exposer à Godot ici

  ClassDB::bind_static_method("Gizmos", D_METHOD("create_2D_gizmos", "viewport"), static_cast<RID (*)(Viewport *)>(&Gizmos::create_2D_gizmos));
  ClassDB::bind_static_method("Gizmos", D_METHOD("create_2D_gizmos", "node_to_apply_to"), static_cast<RID (*)(Node *)>(&Gizmos::create_2D_gizmos));
  ClassDB::bind_static_method("Gizmos", D_METHOD("clear_2D_gizmos", "gizmo_rid"), &Gizmos::clear_2D_gizmos);

  ClassDB::bind_static_method("Gizmos", D_METHOD("free_gizmos", "gizmo_rid"), &Gizmos::free_gizmos);

  ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_line", "gizmo_rid", "from", "to", "width", "color", "antialiasing"), &Gizmos::draw_2D_line, DEFVAL(2.0), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)),
                              DEFVAL(false));
  ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_arrow", "gizmo_rid", "from", "to", "arrow_head_size", "width", "color", "antialiasing"), &Gizmos::draw_2D_arrow, DEFVAL(12.0f), DEFVAL(1.0f),
                              DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false));
  ClassDB::bind_static_method("Gizmos", D_METHOD("draw_2D_circle", "gizmo_rid", "center", "radius", "color", "filled", "thickness", "antialiasing"), &Gizmos::draw_2D_circle,
                              DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(false), DEFVAL(2.0), DEFVAL(false));
}