#include "window_editor.hpp"

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

#pragma endregion

void WindowEditor::open_window()
{
    if (window == nullptr)
    {
        window = memnew(Window());
        window->set_title("Fenêtre d'édition");
        window->set_size(Vector2(400, 300));
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
#pragma endregion

#pragma region Méthodes principales
    ClassDB::bind_method(D_METHOD("open_window"), &WindowEditor::open_window);
    ClassDB::bind_method(D_METHOD("close_window"), &WindowEditor::close_window);
#pragma endregion
}