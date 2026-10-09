#ifndef GIZMO_HOST_HPP
#define GIZMO_HOST_HPP

#include "gizmo_registry.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// Hôte des gizmos automatiques : dessine chaque GizmoEntry de son registre, dans l'éditeur comme en jeu.
///  - UN RID par entrée (chaque draw_* efface son gizmo : 1 RID = 1 forme), créé paresseusement dans le
///    viewport de l'hôte (dans l'éditeur, c'est le viewport de l'éditeur 2D / le World3D de l'éditeur 3D) ;
///  - appel par ClassDB::class_call_static, le RID en premier argument, puis les ArgumentBinding résolus
///    (propriétés lues sur la cible) ;
///  - SEUL propriétaire de ces RID : libérés dans NOTIFICATION_PREDELETE et à la sortie de l'arbre, recréés
///    à l'entrée, recréés si le viewport change ; une entrée retirée / désactivée / changée libère le sien.
class GizmoHost : public Node
{
    GDCLASS(GizmoHost, Node)

  private:
    Ref<GizmoRegistry> registry;
    NodePath target_path;   // contexte des sources « propriété » (vide : le parent, à défaut l'hôte)
    NodePath viewport_path; // viewport de dessin (vide : celui de l'hôte)
    /// Cible désignée directement (ex: objet affiché par une fenêtre d'édition) : prioritaire sur target_path ;
    /// son viewport est alors utilisé (rien n'est dessiné tant qu'elle n'est pas dans l'arbre).
    ObjectID target_object_id;

    /// ObjectID (int) de la GizmoEntry -> { "rid", "function", "args" (derniers arguments dessinés), "drawn" }
    Dictionary live;
    ObjectID current_viewport_id;
    Dictionary known_functions; // "Classe.méthode" -> bool (existe dans le catalogue) : évite de relire ClassDB à chaque image
    int64_t draw_count = 0;
    Dictionary warned; // "entrée:paramètre" déjà signalés (un avertissement par problème, pas un par image)

    /// Valeur résolue adaptée au type du paramètre : null -> valeur par défaut, autre type -> conversion.
    Variant _checked_argument(const Ref<GizmoEntry> &p_entry, int p_index, const Variant &p_value);

    void _update();
    void _free_slot(const Variant &p_key);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  public:
    GizmoHost();
    void _process(double p_delta) override;
    void _exit_tree() override;

    void set_registry(const Ref<GizmoRegistry> &p_registry);
    Ref<GizmoRegistry> get_registry() const;
    void set_target_path(const NodePath &p_path);
    NodePath get_target_path() const;
    void set_viewport_path(const NodePath &p_path);
    NodePath get_viewport_path() const;

    void set_target_object(Object *p_object);
    Object *get_target_object() const;
    /// Contexte des sources : cible directe, sinon target_path, sinon le parent (nullptr hors de l'arbre).
    Object *resolve_target() const;
    /// Viewport de dessin : viewport_path, sinon celui de la cible directe, sinon celui de l'hôte.
    Viewport *resolve_viewport() const;

    /// Libère tous les RID de l'hôte (recréés au prochain dessin).
    void free_all();
    /// Nombre de RID de gizmo actuellement détenus (0 après free_all / sortie de l'arbre).
    int get_live_rid_count() const;
    /// Nombre total d'appels de dessin (tests de la politique de redessin).
    int64_t get_draw_count() const;
};
} // namespace godot

#endif // GIZMO_HOST_HPP
