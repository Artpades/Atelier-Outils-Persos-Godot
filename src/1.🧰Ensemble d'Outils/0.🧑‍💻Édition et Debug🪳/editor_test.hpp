#ifndef EDITOR_TEST_HPP
#define EDITOR_TEST_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🪟Fenêtre d'édition/window_editor.hpp"

#include "godot_cpp/classes/editor_plugin.hpp"
#include "godot_cpp/core/print_string.hpp"
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/ref.hpp>

namespace godot
{
class EditorTest : public EditorPlugin
{
    GDCLASS(EditorTest, EditorPlugin)

  private:
    // Window Editor
    Ref<WindowEditor> window;

  public:
    void _enable_plugin() override { print_line("Enable"); }
    void _enter_tree() override { print_line("Enter Tree"); }
    void creer_bouton_editeur()
    {
        EditorInterface *editor = EditorInterface::get_singleton();
        if (!editor)
            return;

        Control *base_control = editor->get_base_control();
        if (!base_control)
            return;

        Button *btn = memnew(Button);
        btn->set_text("Ouvrir fenêtre de personnalisation");

        Node *target_container = base_control->find_child("MainScreen", true, false);
        if (!target_container)
        {
            target_container = base_control;
        }

        target_container->add_child(btn);
        btn->connect("pressed", Callable(this, "creer_fenetre_personnalisation"));
    }

    // Créer et ouvrir une fenêtre avec des boutons de paramètres
    void creer_fenetre_personnalisation()
    {
        if (window.is_null())
            window.instantiate();
        window->open_window();
    }

  protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("creer_bouton_editeur"), &EditorTest::creer_bouton_editeur);
        ClassDB::bind_method(D_METHOD("creer_fenetre_personnalisation"), &EditorTest::creer_fenetre_personnalisation);
    }
};
} // namespace godot

#endif