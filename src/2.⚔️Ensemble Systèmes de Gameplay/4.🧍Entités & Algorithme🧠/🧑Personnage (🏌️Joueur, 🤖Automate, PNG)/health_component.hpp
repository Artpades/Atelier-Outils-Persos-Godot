#ifndef HEALTH_COMPONENT_HPP
#define HEALTH_COMPONENT_HPP

#include "../🌟Système de Capacité/ability_system.hpp"

#include <godot_cpp/classes/node.hpp>

namespace godot
{
/// ❤️ Composant de vie facultatif d'une entité (personnage, caisse, tourelle...).
/// La vie est interne (max_health / health), ou l'attribut health_attribute de l'AbilitySystem du même parent
/// (ability_system_path, sinon un frère) s'il le possède. Invulnérabilité temporaire après des dégâts.
class HealthComponent : public Node
{
    GDCLASS(HealthComponent, Node)

  public:
    void set_max_health(double p_value);
    double get_max_health() const;
    void set_health(double p_value);
    double get_health() const;
    void set_invulnerability_time(double p_time) { invulnerability_time = MAX(0.0, p_time); }
    double get_invulnerability_time() const { return invulnerability_time; }
    void set_health_attribute(const StringName &p_name) { health_attribute = p_name; }
    StringName get_health_attribute() const { return health_attribute; }
    void set_max_health_attribute(const StringName &p_name) { max_health_attribute = p_name; }
    StringName get_max_health_attribute() const { return max_health_attribute; }
    void set_ability_system_path(const NodePath &p_path) { ability_system_path = p_path; }
    NodePath get_ability_system_path() const { return ability_system_path; }

    /// Retire de la vie (ignoré si mort, invulnérable ou p_amount ≤ 0) ; renvoie les dégâts appliqués. Signaux : health_changed, died éventuel, puis damaged.
    double take_damage(double p_amount, Object *p_source = nullptr);
    /// Rend de la vie sans dépasser le maximum ; renvoie la vie réellement rendue.
    double heal(double p_amount);
    /// Tue sans toucher à la vie (émet died une seule fois).
    void kill();
    /// Ressuscite avec p_health PV (< 0 : maximum).
    void revive(double p_health = -1.0);
    bool is_alive() const { return alive; }
    bool is_invulnerable() const { return invulnerable_remaining > 0.0; }
    double get_health_ratio() const;
    AbilitySystem *get_ability_system() const;
    /// Vrai si la vie est l'attribut health_attribute d'un AbilitySystem (mode attribut).
    bool uses_attribute() const;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    double max_health = 100.0;
    double health = 100.0;
    double invulnerability_time = 0.0;
    /// Temps d'invulnérabilité restant, décompté à chaque image physique.
    double invulnerable_remaining = 0.0;
    bool alive = true;
    StringName health_attribute = "health";
    StringName max_health_attribute = "max_health";
    NodePath ability_system_path;

    /// Relaie attribute_changed de l'AbilitySystem (health_changed, mort).
    void _on_attribute_changed(const StringName &p_name, double p_old, double p_new);
};
} // namespace godot

#endif // HEALTH_COMPONENT_HPP
