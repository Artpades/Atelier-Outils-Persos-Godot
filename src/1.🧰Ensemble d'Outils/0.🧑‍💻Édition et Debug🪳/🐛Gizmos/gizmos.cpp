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

Gizmos::Gizmos() {
  // Initialisation des propriétés et variables ici
}
Gizmos::~Gizmos() {
  // Nettoyage des ressources ici
}

RID Gizmos::create_2D_gizmos(Viewport *viewport) {

  if (!viewport || viewport->find_world_2d() == nullptr) {
    print_line_rich("[color=red]Erreur : Viewport invalide pour la création du "
                    "gizmo 2D[/color]");
    return RID(); // Retourne un RID vide en cas d'erreur
  }

  // TODO : Séparer la création en création editeur et création runtime, pour
  // éviter de parenter le gizmo au GUI de l'éditeur dans le cas d'un gizmo
  // runtime

  // if(!is_editor_hint()) {
  //
  // if(is_editor_hint()) {
  // EditorInterface

  // Implémentation de la création d'un gizmo 2D ici
  // Vous pouvez utiliser les fonctions de Godot pour créer un gizmo 2D
  // et retourner son RID.
  RID gizmo_rid = RenderingServer::get_singleton()
                      ->canvas_item_create(); // Crée un RID vide pour l'instant
  // Parenter le gizmo à la vue (viewport)

  RID viewport_rid = viewport->get_viewport_rid();
  RenderingServer::get_singleton()->canvas_item_set_parent(
      gizmo_rid, viewport->find_world_2d()->get_canvas());

  return gizmo_rid; // Retourne le RID du gizmo créé
}

RID Gizmos::create_3D_gizmos(Viewport *viewport) {
  // Implémentation de la création d'un gizmo 3D ici
  // Vous pouvez utiliser les fonctions de Godot pour créer un gizmo 3D
  // et retourner son RID.
  return RID(); // Retourne un RID vide pour l'instant
}

void Gizmos::draw_2D_point(RID gizmo_rid, const Vector2 &position,
                           const Color &color) {
  // Implémentation du dessin d'un point 2D ici
  // Vous pouvez utiliser les fonctions de dessin de Godot pour dessiner le
  // point sur le gizmo spécifié par gizmo_rid.

  // Si le RID du gizmo est invalide, afficher un message d'erreur
  if (!gizmo_rid.is_valid()) {
    print_line_rich("[color=red]Erreur : RID du gizmo invalide pour le dessin "
                    "du point 2D[/color]");
    return;
  }
  RenderingServer::get_singleton()->canvas_item_clear(
      gizmo_rid); // Efface le contenu précédent du gizmo
                  // Si le RID est valide, continuer avec le dessin
  RenderingServer::get_singleton()->canvas_item_add_circle(
      gizmo_rid, Vector2(0, 0), 5.0, color);
  RenderingServer::get_singleton()->canvas_item_set_transform(
      gizmo_rid, Transform2D(0, position));
}

void Gizmos::clear_gizmos(RID gizmo_rid) {
  // Implémentation de la suppression des gizmos ici
  // Vous pouvez utiliser les fonctions de Godot pour supprimer le gizmo
  // spécifié par gizmo_rid.
  if (!gizmo_rid.is_valid()) {
    print_line_rich("[color=red]Erreur : RID du gizmo invalide pour la "
                    "suppression[/color]");
    return;
  }
  RenderingServer::get_singleton()->canvas_item_clear(gizmo_rid);
}

void Gizmos::free_gizmos(RID gizmo_rid) {
  // Implémentation de la libération des ressources des gizmos ici
  // Vous pouvez utiliser les fonctions de Godot pour libérer les
  // ressources associées au gizmo spécifié par gizmo_rid.
  if (!gizmo_rid.is_valid()) {
    print_line_rich("[color=red]Erreur : RID du gizmo invalide pour la "
                    "libération[/color]");
    return;
  }
  RenderingServer::get_singleton()->free_rid(gizmo_rid);
}

void Gizmos::_bind_methods() {
  // Déclaration des méthodes et propriétés à exposer à Godot ici

  ClassDB::bind_static_method("Gizmos",
                              D_METHOD("create_2D_gizmos", "viewport"),
                              &Gizmos::create_2D_gizmos);
  ClassDB::bind_static_method(
      "Gizmos", D_METHOD("draw_2D_point", "gizmo_rid", "position", "color"),
      &Gizmos::draw_2D_point);
  ClassDB::bind_static_method("Gizmos",
                              D_METHOD("create_3D_gizmos", "viewport"),
                              &Gizmos::create_3D_gizmos);
  ClassDB::bind_static_method("Gizmos", D_METHOD("clear_gizmos", "gizmo_rid"),
                              &Gizmos::clear_gizmos);
  ClassDB::bind_static_method("Gizmos", D_METHOD("free_gizmos", "gizmo_rid"),
                              &Gizmos::free_gizmos);
}