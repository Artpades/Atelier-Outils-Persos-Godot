#ifndef EDITOR_TEST_HPP
#define EDITOR_TEST_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🪟Fenêtre d'édition/window_editor.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧑‍💻Éditeur/object_interface_panel.hpp"

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
// Plugin d'éditeur de test : ajoute un bouton dans la barre d'outils qui ouvre une WindowEditor contenant
// un ObjectInterfacePanel sur le nœud sélectionné (ou la racine de la scène éditée) : édition de l'objet
// et de l'interface de son type (données, groupes, gizmos), enregistrée dans la base du projet.
// Enregistré au niveau EDITOR via EditorPlugins::add_by_type (voir tool_editor_register.cpp) :
// l'éditeur l'instancie, l'ajoute à l'arbre et le libère lui-même.
class EditorTest : public EditorPlugin
{
    GDCLASS(EditorTest, EditorPlugin)

  private:
    // Bouton créé dans _enter_tree et libéré dans _exit_tree : stocké par ObjectID,
    // car c'est l'éditeur (son conteneur) qui le tient dans l'arbre.
    ObjectID button_id;

    // Window Editor
    Ref<WindowEditor> window;

    // Panneau placé dans la fenêtre (tenu par la fenêtre, donc ObjectID).
    ObjectID interface_panel_id;

  protected:
    static void _bind_methods();

  public:
    void _enter_tree() override;
    void _exit_tree() override;

    // Créer et ouvrir une fenêtre avec des boutons de paramètres
    void creer_fenetre_personnalisation();
    ObjectInterfacePanel *get_interface_panel() const;
};
} // namespace godot

#endif
