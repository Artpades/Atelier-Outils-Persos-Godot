#ifndef GIZMO_CONFIG_WINDOW_HPP
#define GIZMO_CONFIG_WINDOW_HPP

#include "gizmo_list_editor.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🎯Gizmos/gizmo_host.hpp"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// Fenêtre « Configurer les gizmos » (éditeur uniquement) : édite le GizmoRegistry d'un GizmoHost de la
/// scène éditée (créé s'il n'existe pas, enregistré avec la scène). Les dessins se mettent à jour en direct
/// dans la vue 2D / 3D de l'éditeur. Une seule fenêtre ouverte à la fois.
class GizmoConfigWindow : public Window
{
    GDCLASS(GizmoConfigWindow, Window)

  private:
    static ObjectID current_window_id;

    ObjectID host_id;
    Label *target_label = nullptr; // enfants possédés
    GizmoListEditor *list = nullptr;

    void _build_ui();
    void _on_close_requested();

  protected:
    static void _bind_methods() {}

  public:
    static void open_for(GizmoHost *p_host);
    static void close_current();
};
} // namespace godot

#endif // GIZMO_CONFIG_WINDOW_HPP
