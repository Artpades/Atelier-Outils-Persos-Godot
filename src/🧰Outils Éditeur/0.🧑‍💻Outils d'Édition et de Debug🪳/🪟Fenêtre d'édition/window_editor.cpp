#include "window_editor.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/variant/callable.hpp"
#include <godot_cpp/classes/callback_tweener.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/variant/vector2.hpp>
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

Vector2i WindowEditor::get_window_size() const { return window_size; }
void WindowEditor::set_window_size(const Vector2i &value) { window_size = value; }

Callable WindowEditor::get_open_window_button() { return Callable(this, "open_window"); }
void WindowEditor::set_open_window_button(const Callable &value) { open_window_button = value; }

WindowOpenningAnimationMode WindowEditor::get_anim_open_mode() const { return open_anim_mode; }
void WindowEditor::set_anim_open_mode(WindowOpenningAnimationMode value) { open_anim_mode = value; }

WindowClosingAnimationMode WindowEditor::get_anim_close_mode() const { return close_anim_mode; }
void WindowEditor::set_anim_close_mode(WindowClosingAnimationMode value) { close_anim_mode = value; }
#pragma endregion

// Créer ou affiche une fenêtre, et l'ouvre
void WindowEditor::open_window()
{
    if (window != nullptr && window->is_visible())
        return;
    // print_line(U"Ouverture de la fenêtre d'édition : " + window_title);

    // Ajout dans l'arborescence de l'éditeur pour qu'elle soit visible
    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (scene_tree == nullptr || scene_tree->get_root() == nullptr)
    {
        print_line_rich(U"[color=red]Aucun SceneTree actif : fenêtre d'édition non créée[/color]", "WindowEditor", WINDOW_EDITOR);
        return;
    }

    if (window == nullptr)
    {

        // Création de la fenêtre avec ses paramètres
        window = memnew(Window());
        window->set_title(window_title);
        window->set_size(Vector2(10, 10));
        window->set_visible(true);

        // Connecter le signal close_requested à la méthode close_window pour fermer la fenêtre lorsque l'utilisateur clique sur le bouton de fermeture
        window->connect("close_requested", Callable(this, "close_window"));
        scene_tree->get_root()->add_child(window);
        window->popup_centered();
    }
    else if (window != nullptr)
    {
        window->set_visible(true);
        print_line(U"Fenêtre d'édition déjà ouverte : " + window_title);
    }

    // Pour chaque cas d'effet à l'ouverture
    switch (open_anim_mode)
    {
    case WindowOpenningAnimationMode::O_NONE:
    {
        window->set_size(Vector2(window_size));
        break;
    }
    case WindowOpenningAnimationMode::O_POPUP:
    {
        // Créer un twenning pour la taille et un pour la position
        Ref<Tween> window_popup_tween = scene_tree->create_tween();
        window_popup_tween->parallel()->tween_property(window, "position", window->get_position() - window_size / 2, 0.5)->set_ease(Tween::EASE_OUT)->set_trans(Tween::TRANS_BACK);
        window_popup_tween->parallel()->tween_property(window, "size", window_size, 0.5)->set_ease(Tween::EASE_OUT)->set_trans(Tween::TRANS_BACK);
        break;
    }
    case WindowOpenningAnimationMode::O_NUMERIC_LIKE:
    {
        // Créer un twenning pour la taille et un pour la position
        Ref<Tween> window_popup_tween = scene_tree->create_tween();

        window_popup_tween->tween_property(window, "position:x", window->get_position().x - window_size.x / 2, 0.5)->set_ease(Tween::EASE_OUT)->set_trans(Tween::TRANS_CUBIC);
        window_popup_tween->parallel()->tween_property(window, "size:x", window_size.x, 0.5)->set_ease(Tween::EASE_OUT)->set_trans(Tween::TRANS_CUBIC);
        window_popup_tween->tween_property(window, "position:y", window->get_position().y - window_size.y / 2, 0.5)->set_ease(Tween::EASE_OUT)->set_trans(Tween::TRANS_CUBIC);
        window_popup_tween->parallel()->tween_property(window, "size:y", window_size.y, 0.5)->set_ease(Tween::EASE_OUT)->set_trans(Tween::TRANS_CUBIC);

        break;
    }
    default:
        break;
    }
    emit_signal("window_opened");
}

void WindowEditor::close_window()
{
    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (scene_tree == nullptr || scene_tree->get_root() == nullptr)
    {
        print_line_rich(U"[color=red]Aucun SceneTree actif : fenêtre d'édition non créée[/color]", "WindowEditor", WINDOW_EDITOR);
        return;
    }

    if (window != nullptr)
    {
        if (close_tween.is_valid() && close_tween->is_running())
        {
            return;
        }

        switch (close_anim_mode)
        {
        case WindowClosingAnimationMode::C_NONE:
        {
            free_window();
            break;
        }
        case WindowClosingAnimationMode::C_POPUP:
        {
            close_tween = scene_tree->create_tween();
            close_tween->bind_node(window);

            Vector2i target_pos = window->get_position() + window->get_size() / 2;
            Vector2i target_size = Vector2i(0, 0);

            // Utilisation de Vector2i complets (pas de :x / :y)
            close_tween->parallel()->tween_property(window, "position", target_pos, 0.5)->set_ease(Tween::EASE_IN)->set_trans(Tween::TRANS_BACK);
            close_tween->parallel()->tween_property(window, "size", target_size, 0.5)->set_ease(Tween::EASE_IN)->set_trans(Tween::TRANS_BACK);

            // Le callback s'exécutera exactement à la fin des 0.5 secondes
            close_tween->tween_callback(Callable(this, "free_window"));
            break;
        }
        case WindowClosingAnimationMode::C_NUMERIC_LIKE:
        {
            close_tween = scene_tree->create_tween();
            close_tween->bind_node(window);

            close_tween->tween_property(window, "position:x", window->get_position().x + window->get_size().x / 2, 0.5)->set_ease(Tween::EASE_OUT)->set_trans(Tween::TRANS_CUBIC);
            close_tween->parallel()->tween_property(window, "size:x", 0, 0.5)->set_ease(Tween::EASE_OUT)->set_trans(Tween::TRANS_CUBIC);
            close_tween->tween_property(window, "position:y", window->get_position().y + window->get_size().y / 2, 0.5)->set_ease(Tween::EASE_OUT)->set_trans(Tween::TRANS_CUBIC);
            close_tween->parallel()->tween_property(window, "size:y", 0, 0.5)->set_ease(Tween::EASE_OUT)->set_trans(Tween::TRANS_CUBIC);

            close_tween->tween_callback(Callable(this, "free_window"));
            break;
        }
        default:
            free_window();
            break;
        }
    }
}

void WindowEditor::free_window()
{
    if (window != nullptr)
    {
        window->queue_free();
        window = nullptr;
    }
}

void WindowEditor::_bind_methods()
{

    BIND_ENUM_CONSTANT(WindowOpenningAnimationMode::O_NONE);
    BIND_ENUM_CONSTANT(WindowOpenningAnimationMode::O_POPUP);
    BIND_ENUM_CONSTANT(WindowOpenningAnimationMode::O_NUMERIC_LIKE);

    BIND_ENUM_CONSTANT(WindowClosingAnimationMode::C_NONE);
    BIND_ENUM_CONSTANT(WindowClosingAnimationMode::C_POPUP);
    BIND_ENUM_CONSTANT(WindowClosingAnimationMode::C_NUMERIC_LIKE);

#pragma region Getter/Setter
    ClassDB::bind_method(D_METHOD("get_window"), &WindowEditor::get_window);
    ClassDB::bind_method(D_METHOD("set_window", "value"), &WindowEditor::set_window);

    ClassDB::bind_method(D_METHOD("get_window_title"), &WindowEditor::get_window_title);
    ClassDB::bind_method(D_METHOD("set_window_title", "value"), &WindowEditor::set_window_title);

    ClassDB::bind_method(D_METHOD("get_window_size"), &WindowEditor::get_window_size);
    ClassDB::bind_method(D_METHOD("set_window_size", "value"), &WindowEditor::set_window_size);

    ClassDB::bind_method(D_METHOD("get_open_window_button"), &WindowEditor::get_open_window_button);
    ClassDB::bind_method(D_METHOD("set_open_window_button", "value"), &WindowEditor::set_open_window_button);

    ClassDB::bind_method(D_METHOD("get_anim_open_mode"), &WindowEditor::get_anim_open_mode);
    ClassDB::bind_method(D_METHOD("set_anim_open_mode", "value"), &WindowEditor::set_anim_open_mode);
    ClassDB::bind_method(D_METHOD("get_anim_close_mode"), &WindowEditor::get_anim_close_mode);
    ClassDB::bind_method(D_METHOD("set_anim_close_mode", "value"), &WindowEditor::set_anim_close_mode);
#pragma endregion

#pragma region Méthodes principales
    ClassDB::bind_method(D_METHOD("open_window"), &WindowEditor::open_window);
    ClassDB::bind_method(D_METHOD("close_window"), &WindowEditor::close_window);
    ClassDB::bind_method(D_METHOD("free_window"), &WindowEditor::free_window);
#pragma endregion

#pragma region Propriétés
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "window_title"), "set_window_title", "get_window_title");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "window_size"), "set_window_size", "get_window_size");

    ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "open_window_button", PROPERTY_HINT_TOOL_BUTTON, U"Ouvrir la fenêtre", PROPERTY_USAGE_EDITOR), "", "get_open_window_button");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "open_anim_mode", PROPERTY_HINT_ENUM, "O_NONE,O_POPUP,O_NUMERIC_LIKE"), "set_anim_open_mode", "get_anim_open_mode");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "close_anim_mode", PROPERTY_HINT_ENUM, "C_NONE,C_POPUP,C_NUMERIC_LIKE"), "set_anim_close_mode", "get_anim_close_mode");
#pragma endregion

    ADD_SIGNAL(MethodInfo("window_opened"));
}