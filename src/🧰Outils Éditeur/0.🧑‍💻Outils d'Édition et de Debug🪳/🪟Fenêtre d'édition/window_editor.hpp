#ifndef WINDOW_EDITOR_HPP
#define WINDOW_EDITOR_HPP

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>

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
    String window_title;
    Vector2 window_position;
    Vector2 window_size;

    Callable open_window_button;

  protected:
    static void _bind_methods();

  public:
    WindowEditor();
    ~WindowEditor();

    Window *get_window() const;
    void set_window(Window *value);

    String get_window_title() const;
    void set_window_title(const String &value);

    Vector2 get_window_position() const;
    void set_window_position(const Vector2 &value);

    Vector2 get_window_size() const;
    void set_window_size(const Vector2 &value);

    Callable get_open_window_button();
    void set_open_window_button(const Callable &value);

    // Ouvre la fenêtre d'édition
    void open_window();
    // Ferme la fenêtre d'édition
    void close_window();
};
} // namespace godot

#endif // WINDOW_EDITOR_HPP