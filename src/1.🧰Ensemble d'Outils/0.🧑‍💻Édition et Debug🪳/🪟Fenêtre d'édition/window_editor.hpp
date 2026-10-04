#ifndef WINDOW_EDITOR_HPP
#define WINDOW_EDITOR_HPP

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/window.hpp>

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"
namespace godot
{
namespace
{
const char32_t *WINDOW_EDITOR = U"res://🖼️Editor Icons/🧰🧑‍💻/WindowEditor icon.png";
}
class WindowEditor : public RefCounted
{
    GDCLASS(WindowEditor, RefCounted)
  private:
    // Déclaration des propriétés et variables privées ici
    Window *window;
    void on_window_close()
    {
        if (window)
        {
            window->set_visible(false);

            log_it("[color=red]Fenêtre d'édition fermée[/color]", "WindowEditor", WINDOW_EDITOR);
        }
    }

  protected:
    static void _bind_methods();

  public:
    WindowEditor();
    ~WindowEditor();

    // Déclaration des méthodes publiques ici
    void open_window();
    void close_window()
    {
        if (window)
        {
            window->set_visible(false);
            log_it("[color=red]Fenêtre d'édition fermée[/color]", "WindowEditor", WINDOW_EDITOR);
        }
    }
    void add_control_to_window(Control *control);
    void remove_control_from_window(Control *control);
};
} // namespace godot

#endif