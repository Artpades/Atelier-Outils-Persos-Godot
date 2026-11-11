#ifndef WINDOW_EDITOR_HPP
#define WINDOW_EDITOR_HPP

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot
{
// Une classe permettant de créer une fenêtre dans l'éditeur et de lui ajouter des éléments d'interfaces. Cette classe est un Resource, ce qui permet de la sauvegarder dans un fichier .tres et de la
// réutiliser dans d'autres projets.
class WindowEditor : public Resource
{
    GDCLASS(WindowEditor, Resource)
  private:
    // Fenêtre d'édition qui sera à créer et détruite à chaque ouverture/fermeture de la fenêtre d'édition
    Window *window;

  protected:
    static void _bind_methods();

  public:
    WindowEditor();
    ~WindowEditor();

    Window *get_window() const;
    void set_window(Window *value);

    // Ouvre la fenêtre d'édition
    void open_window();
    // Ferme la fenêtre d'édition
    void close_window();
};
} // namespace godot

#endif // WINDOW_EDITOR_HPP