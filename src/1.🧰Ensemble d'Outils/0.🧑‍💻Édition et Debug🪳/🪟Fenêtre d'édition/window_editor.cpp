#include "window_editor.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/vector2i.hpp>

using namespace godot;

namespace
{
const char32_t *WINDOW_EDITOR = U"res://🖼️Editor Icons/🧰🧑‍💻/WindowEditor icon.png";
}

WindowEditor::WindowEditor()
{
    // Initialisation des propriétés et variables ici
}
WindowEditor::~WindowEditor()
{
    // Nettoyage fait dans _notification(NOTIFICATION_PREDELETE)
}

void WindowEditor::_notification(int p_what)
{
    if (p_what == NOTIFICATION_PREDELETE)
    {
        // Seul propriétaire de la fenêtre : on la libère si l'arbre ne l'a pas déjà fait.
        if (Window *window = _get_window())
        {
            window->queue_free();
        }
        window_id = ObjectID();
    }
}

Window *WindowEditor::_get_window() const
{
    Window *window = Object::cast_to<Window>(ObjectDB::get_instance(window_id));
    if (window && window->is_queued_for_deletion())
    {
        return nullptr;
    }
    return window;
}

void WindowEditor::add_control_to_window(Control *control)
{
    Window *window = _get_window();
    if (window && control)
    {
        window->add_child(control);
    }
}

void WindowEditor::remove_control_from_window(Control *control)
{
    Window *window = _get_window();
    if (window && control)
    {
        window->remove_child(control);
    }
}

void WindowEditor::open_window()
{
    Window *window = _get_window();
    if (!window)
    {
        // Création (ou recréation si la fenêtre précédente a été libérée ailleurs).
        SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (!scene_tree || !scene_tree->get_root())
        {
            log_it(U"[color=red]Aucun SceneTree actif : fenêtre d'édition non créée[/color]", "WindowEditor", WINDOW_EDITOR);
            return;
        }

        window = memnew(Window);
        window->set_title(U"Fenêtre d'édition");
        window->set_size(Vector2i(800, 600));
        // Une Window est visible par défaut : masquée avant l'ajout, puis affichée par popup_centered().
        window->set_visible(false);
        window->connect("close_requested", Callable(this, "close_window"));

        scene_tree->get_root()->add_child(window);
        window_id = window->get_instance_id();
        window->popup_centered();
    }
    else
    {
        window->set_visible(true);
    }
    // Implémentation de l'ouverture de la fenêtre ici
    log_it(U"[color=green]Ouverture de la fenêtre d'édition[/color]", "WindowEditor", WINDOW_EDITOR);
}

void WindowEditor::close_window()
{
    if (Window *window = _get_window())
    {
        window->set_visible(false);
        log_it(U"[color=red]Fenêtre d'édition fermée[/color]", "WindowEditor", WINDOW_EDITOR);
    }
}

void WindowEditor::_bind_methods()
{
    // Déclaration des méthodes et propriétés à exposer à Godot ici
    ClassDB::bind_method(D_METHOD("open_window"), &WindowEditor::open_window);
    ClassDB::bind_method(D_METHOD("add_control_to_window", "control"), &WindowEditor::add_control_to_window);
    ClassDB::bind_method(D_METHOD("remove_control_from_window", "control"), &WindowEditor::remove_control_from_window);
    ClassDB::bind_method(D_METHOD("close_window"), &WindowEditor::close_window);
}
