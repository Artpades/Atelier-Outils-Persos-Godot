#ifndef INTERACTABLE_HPP
#define INTERACTABLE_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
/// 🤝 Rend son nœud parent interactif (PNJ, coffre, porte...) : le PlayerController du joueur interagit avec
/// le plus proche à portée (interaction_range, mesurée depuis le parent) et le signal interacted(by) est émis.
class Interactable : public Node
{
    GDCLASS(Interactable, Node)

  public:
    void set_prompt(const String &p_prompt) { prompt = p_prompt; }
    String get_prompt() const { return prompt; }
    void set_interaction_range(double p_range) { interaction_range = MAX(0.0, p_range); }
    double get_interaction_range() const { return interaction_range; }
    void set_enabled(bool p_enabled) { enabled = p_enabled; }
    bool is_enabled() const { return enabled; }
    void set_one_shot(bool p_one_shot) { one_shot = p_one_shot; }
    bool is_one_shot() const { return one_shot; }

    /// Émet interacted(p_by) si activé (désactive l'objet si one_shot). Ne vérifie pas la portée.
    bool interact(Node *p_by);
    Vector3 get_world_position() const;
    /// Vrai si p_node est à portée de la position du parent (et n'est pas ce parent).
    bool is_in_range_of(Node *p_node) const;

    /// Interactable activé le plus proche de p_from, à portée (de lui-même) ; nullptr s'il n'y en a pas.
    static Interactable *find_nearest(Node *p_from);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    String prompt = U"Interagir";
    double interaction_range = 64.0;
    bool enabled = true;
    bool one_shot = false;

    /// Interactable présents dans l'arbre (en jeu), parcourus par find_nearest.
    static LocalVector<ObjectID> instances;
};
} // namespace godot

#endif // INTERACTABLE_HPP
