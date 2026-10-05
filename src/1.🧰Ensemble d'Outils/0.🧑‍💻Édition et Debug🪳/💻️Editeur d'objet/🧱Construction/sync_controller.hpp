#ifndef SYNC_CONTROLLER_HPP
#define SYNC_CONTROLLER_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/property_widget.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot
{
// UN contrôleur de synchronisation par éditeur (remplace le Node de polling par widget du C#).
// Godot n'a pas de signal « valeur modifiée » : à intervalle régulier, il relit la valeur des widgets
// VISIBLES et met à jour ceux dont la valeur a changé de l'extérieur. Un widget en cours d'édition
// (focus clavier) n'est pas mis à jour, pour ne pas écraser la saisie.
// notify_written (anti-écho, port de NotifyWritten) : après une écriture, relit la valeur ; si le setter
// l'a refusée ou transformée, le widget est corrigé et un avertissement est émis.
class SyncController : public Node
{
    GDCLASS(SyncController, Node)

  private:
    double interval = 0.1;
    double elapsed = 0.0;
    ObjectID target_id;
    bool target_lost = false;
    Vector<ObjectID> widgets;

    void _refresh_widget(PropertyWidget *p_widget, bool p_force);

  protected:
    static void _bind_methods();

  public:
    // Égalité de valeurs : nombres à une tolérance près, sinon même type et même contenu (hash_compare).
    static bool same_value(const Variant &p_a, const Variant &p_b);

    SyncController();
    void _process(double p_delta) override;

    void set_interval(double p_interval);
    double get_interval() const;

    void set_target(Object *p_target);
    void register_widget(PropertyWidget *p_widget);
    void clear();

    // Relit tous les widgets (p_force : même sans changement, et même non visibles ou en cours d'édition).
    void refresh_all(bool p_force);
    void notify_written(PropertyWidget *p_widget, const Variant &p_sent, bool p_committing);
};
} // namespace godot

#endif // SYNC_CONTROLLER_HPP
