#include "window_editor.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/variant/callable.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

using namespace godot;

namespace
{
const char32_t *WINDOW_EDITOR = U"res://🖼️Editor Icons/🧰🧑‍💻/WindowEditor icon.png";
}
#pragma region Constructeur/Destructeur
WindowEditor::WindowEditor()
{
    // Initialisation des propriétés et variables ici
}
WindowEditor::~WindowEditor()
{
    // Nettoyage fait dans _notification(NOTIFICATION_PREDELETE)
}
#pragma endregion

#pragma region Getter/Setter

Window *WindowEditor::get_window() const { return window; }
void WindowEditor::set_window(Window *value) { window = value; }

String WindowEditor::get_window_title() const { return window_title; }
void WindowEditor::set_window_title(const String &value) { window_title = value; }

Vector2 WindowEditor::get_window_position() const { return window_position; }
void WindowEditor::set_window_position(const Vector2 &value) { window_position = value; }

Vector2 WindowEditor::get_window_size() const { return window_size; }
void WindowEditor::set_window_size(const Vector2 &value) { window_size = value; }

Callable WindowEditor::get_open_window_button() { return Callable(this, "open_window"); }
void WindowEditor::set_open_window_button(const Callable &value) { open_window_button = value; }

#pragma endregion

void WindowEditor::open_window()
{
    if (window == nullptr)
    {
        // Création de la fenêtre avec ses paramètres
        window = memnew(Window());
        window->set_title(window_title);
        window->set_position(window_position);
        window->set_size(window_size);
        window->set_visible(true);
        print_line(U"Fenêtre d'édition ouverte : " + window_title);
        // Ajout dans l'arborescence de l'éditeur pour qu'elle soit visible
        SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());

        if (scene_tree == nullptr || scene_tree->get_root() == nullptr)
        {
            print_line_rich(U"[color=red]Aucun SceneTree actif : fenêtre d'édition non créée[/color]", "WindowEditor", WINDOW_EDITOR);
            return;
        }

        print_line(U"Ajout de la fenêtre d'édition à l'arborescence de l'éditeur");
        // Connecter le signal close_requested à la méthode close_window pour fermer la fenêtre lorsque l'utilisateur clique sur le bouton de fermeture
        window->connect("close_requested", Callable(this, "close_window"));
        scene_tree->get_root()->add_child(window);
        window->popup_centered();
    }
    else
    {
        window->set_visible(true);
    }
}

void WindowEditor::close_window()
{
    if (window != nullptr)
    {
        window->queue_free();
        window = nullptr;
    }
}

void WindowEditor::_bind_methods()
{
#pragma region Getter/Setter
    ClassDB::bind_method(D_METHOD("get_window"), &WindowEditor::get_window);
    ClassDB::bind_method(D_METHOD("set_window", "value"), &WindowEditor::set_window);

    ClassDB::bind_method(D_METHOD("get_window_title"), &WindowEditor::get_window_title);
    ClassDB::bind_method(D_METHOD("set_window_title", "value"), &WindowEditor::set_window_title);

    ClassDB::bind_method(D_METHOD("get_window_position"), &WindowEditor::get_window_position);
    ClassDB::bind_method(D_METHOD("set_window_position", "value"), &WindowEditor::set_window_position);

    ClassDB::bind_method(D_METHOD("get_window_size"), &WindowEditor::get_window_size);
    ClassDB::bind_method(D_METHOD("set_window_size", "value"), &WindowEditor::set_window_size);

    ClassDB::bind_method(D_METHOD("get_open_window_button"), &WindowEditor::get_open_window_button);
    ClassDB::bind_method(D_METHOD("set_open_window_button", "value"), &WindowEditor::set_open_window_button);
#pragma endregion

#pragma region Méthodes principales
    ClassDB::bind_method(D_METHOD("open_window"), &WindowEditor::open_window);
    ClassDB::bind_method(D_METHOD("close_window"), &WindowEditor::close_window);
#pragma endregion

#pragma region Propriétés
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "window_title"), "set_window_title", "get_window_title");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "window_position"), "set_window_position", "get_window_position");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "window_size"), "set_window_size", "get_window_size");

    ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "open_window_button", PROPERTY_HINT_TOOL_BUTTON, U"Ouvrir la fenêtre", PROPERTY_USAGE_EDITOR), "", "get_open_window_button");
#pragma endregion
}