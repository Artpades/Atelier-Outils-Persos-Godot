#ifndef GIZMOS_HPP
#define GIZMOS_HPP

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
class Gizmos : public RefCounted {
  GDCLASS(Gizmos, RefCounted)

protected:
  static void _bind_methods();

public:
  Gizmos();
  ~Gizmos();

  // Déclaration des méthodes publiques ici
  static RID create_2D_gizmos(Viewport *viewport);
  // A voir : Déplacer la partie de parentage pour en faire une méthode à part
  // entière, pour plus de flexibilité
  // Ex : sur les sous-vues comme les fenêtres, ou les SubViewport, ou les
  // ViewportContainer, etc...

  static RID create_3D_gizmos(Viewport *viewport);
  static void draw_2D_point(RID gizmo_rid, const Vector2 &position,
                            const Color &color);

  static void clear_gizmos(RID gizmo_rid);
  // Implémentation de la suppression des gizmos ici
  // Vous pouvez utiliser les fonctions de Godot pour supprimer le gizmo
  // spécifié par gizmo_rid.
  // Ne s'utilise que pour réinitialiser le visuel du gizmo, pas pour le
  // supprimer définitivement.
  // Pour la suppression définitive, utilisez free_gizmos(RID gizmo_rid)

  static void free_gizmos(RID gizmo_rid);
  // Implémentation de la libération des ressources des gizmos ici
  // Vous pouvez utiliser les fonctions de Godot pour libérer les
  // ressources associées au gizmo spécifié par gizmo_rid.
};
} // namespace godot

#endif // GIZMOS_HPP
