#include "window_editor.hpp"
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/vector2.hpp>

using namespace godot;

WindowEditor::WindowEditor() {
  // Initialisation des propriétés et variables ici
}
WindowEditor::~WindowEditor() {
  // Nettoyage des ressources ici
}
void WindowEditor::add_control_to_window(Control *control) {
  if (window && control) {
    window->add_child(control);
  }
}

void WindowEditor::remove_control_from_window(Control *control) {
  if (window && control) {
    window->remove_child(control);
  }
}

void WindowEditor::open_window() {
  if (!window) {
    window = memnew(Window);
    window->set_title("Fenêtre d'édition");
    window->set_size(Vector2(800, 600));
    window->set_visible(true);

    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    scene_tree->get_root()->add_child(window);
    window->popup_centered();
    window->connect("close_requested", Callable(this, "close_window"));
  } else {
    window->set_visible(true);
  }
  // Implémentation de l'ouverture de la fenêtre ici
  print_line_rich("[color=blue]Ouverture de la fenêtre d'édition[/color]");
}

void WindowEditor::_bind_methods() {
  // Déclaration des méthodes et propriétés à exposer à Godot ici
  ClassDB::bind_method(D_METHOD("open_window"), &WindowEditor::open_window);
  ClassDB::bind_method(D_METHOD("add_control_to_window", "control"), &WindowEditor::add_control_to_window);
  ClassDB::bind_method(D_METHOD("remove_control_from_window", "control"), &WindowEditor::remove_control_from_window);
  ClassDB::bind_method(D_METHOD("close_window"), &WindowEditor::close_window);
}