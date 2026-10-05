#ifndef WINDOW_EDITOR_HPP
#define WINDOW_EDITOR_HPP

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
// Fenêtre d'édition utilisable en jeu comme dans l'éditeur : la Window est ajoutée sous la racine
// du SceneTree courant (racine du jeu, ou de l'éditeur).
class WindowEditor : public RefCounted
{
    GDCLASS(WindowEditor, RefCounted)
  private:
    // Fenêtre créée par cet objet, mais tenue par l'arbre de scène qui peut la libérer de son côté
    // (ex: fermeture de l'éditeur) : stockée par ObjectID et résolue à chaque accès.
    ObjectID window_id;

    // Fenêtre valide, ou nullptr si elle n'existe plus (ou est déjà en cours de suppression).
    Window *_get_window() const;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  public:
    WindowEditor();
    ~WindowEditor();

    // Déclaration des méthodes publiques ici
    void open_window();
    void close_window();
    void add_control_to_window(Control *control);
    void remove_control_from_window(Control *control);
};
} // namespace godot

#endif
