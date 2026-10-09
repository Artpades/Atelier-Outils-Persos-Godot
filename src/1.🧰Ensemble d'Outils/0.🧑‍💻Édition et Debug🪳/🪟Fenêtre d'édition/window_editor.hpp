#ifndef WINDOW_EDITOR_HPP
#define WINDOW_EDITOR_HPP

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// 🪟 Fenêtre d'édition utilisable en jeu comme dans l'éditeur : la Window (800 × 600, centrée) est ajoutée
/// sous la racine du SceneTree courant (racine du jeu, ou de l'éditeur). La croix la masque sans la détruire ;
/// elle est libérée avec le WindowEditor (NOTIFICATION_PREDELETE), donc quand plus rien ne le référence.
class WindowEditor : public RefCounted
{
    GDCLASS(WindowEditor, RefCounted)
  private:
    /// Fenêtre créée par cet objet, mais tenue par l'arbre de scène qui peut la libérer de son côté
    /// (ex: fermeture de l'éditeur) : stockée par ObjectID et résolue à chaque accès.
    ObjectID window_id;

    /// Fenêtre valide, ou nullptr si elle n'existe plus (ou est déjà en cours de suppression).
    Window *_get_window() const;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  public:
    WindowEditor();
    ~WindowEditor();

    /// Crée la fenêtre au premier appel (ou si elle a été libérée), sinon la réaffiche. Erreur en console sans SceneTree.
    void open_window();
    /// Masque la fenêtre sans la détruire (connectée à close_requested).
    void close_window();
    /// Ajoute control comme enfant de la fenêtre ; sans effet si elle n'existe pas encore (appeler open_window d'abord).
    void add_control_to_window(Control *control);
    /// Retire control de la fenêtre sans le libérer (à la charge de l'appelant).
    void remove_control_from_window(Control *control);
};
} // namespace godot

#endif
