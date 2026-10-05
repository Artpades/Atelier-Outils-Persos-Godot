#ifndef LAYOUT_LINKER_HPP
#define LAYOUT_LINKER_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/layout_database.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>

namespace godot
{
// En jeu uniquement : applique les liens de signaux (mode ressource) de l'interface par type à chaque nœud
// qui entre dans l'arbre (SceneTree::node_added), sans ProceduralEditor. Les connexions disparaissent avec
// le nœud. Les Resources n'ont pas d'équivalent de node_added : leurs liens ne s'appliquent que via un
// ProceduralEditor (ou ObjectEditorWindow::open_for).
// Objet unique, créé au chargement du module et détruit au déchargement (tool_editor_register.cpp) ;
// branché sur le SceneTree à la première image (call_deferred), l'arbre déjà chargé est alors parcouru.
class LayoutLinker : public Object
{
    GDCLASS(LayoutLinker, Object)

  private:
    inline static LayoutLinker *singleton = nullptr; // pointeur : destruction triviale au déchargement
    Ref<LayoutDatabase> database;                     // gardée pour la durée du jeu
    bool started = false;
    int64_t applied_count = 0;

    void _apply(Node *p_node);
    void _apply_tree(Node *p_node);
    void _on_node_added(Node *p_node);

  protected:
    static void _bind_methods();

  public:
    static void create();
    static void destroy();
    static LayoutLinker *get_singleton();

    void start();
    bool is_started() const;
    // Nombre de nœuds auxquels des liens ont été appliqués (tests).
    int64_t get_applied_count() const;
};
} // namespace godot

#endif // LAYOUT_LINKER_HPP
