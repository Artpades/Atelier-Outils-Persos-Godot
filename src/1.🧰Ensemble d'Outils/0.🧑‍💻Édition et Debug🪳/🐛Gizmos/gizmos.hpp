#ifndef GIZMOS_HPP
#define GIZMOS_HPP

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot
{

class Gizmos : public RefCounted
{
  GDCLASS(Gizmos, RefCounted)
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

protected:
  static void _bind_methods();

public:
  Gizmos();
  ~Gizmos();

  static RID create_2D_gizmos(Node *node_to_apply_to);
  static RID create_2D_gizmos(Viewport *viewport);
  // Déclaration des méthodes publiques ici
  // A voir : Déplacer la partie de parentage pour en faire une méthode à part
  // entière, pour plus de flexibilité
  // Ex : sur les sous-vues comme les fenêtres, ou les SubViewport, ou les
  // ViewportContainer, etc...

  // static void set_material(RID gizmo_rid, RID material_rid);
  static void clear_2D_gizmos(const RID gizmo_rid);
  // Implémentation de la suppression des gizmos ici
  // Vous pouvez utiliser les fonctions de Godot pour supprimer le gizmo
  // spécifié par gizmo_rid.
  // Ne s'utilise que pour réinitialiser le visuel du gizmo, pas pour le
  // supprimer définitivement.
  // Pour la suppression définitive, utilisez free_gizmos(RID gizmo_rid)
  static void free_gizmos(const RID gizmo_rid);
  // Implémentation de la libération des ressources des gizmos ici
  // Vous pouvez utiliser les fonctions de Godot pour libérer les
  // ressources associées au gizmo spécifié par gizmo_rid.
  // static void draw_2D_handler(RID gizmo_rid, const Vector2 &position, const
  // Color &color);
  // static void draw_2D_point(RID gizmo_rid, const Vector2 &position, const
  // Color &color);

  static void draw_2D_line(RID gizmo_rid, const Vector2 &from, const Vector2 &to, float width = 2.0, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool antialiasing = false);
  static void
  draw_2D_arrow(RID gizmo_rid, const Vector2 &from, const Vector2 &to, float arrow_head_size = 12.0f, float width = 1.0f, const Color &color = Color(1.0, 1.0, 1.0, 1.0), bool antialiasing = false);
};

} // namespace godot

#endif // GIZMOS_HPP
