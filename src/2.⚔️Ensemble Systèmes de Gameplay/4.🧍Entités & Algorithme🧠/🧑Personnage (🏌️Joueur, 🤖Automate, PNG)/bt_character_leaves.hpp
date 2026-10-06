#ifndef BT_CHARACTER_LEAVES_HPP
#define BT_CHARACTER_LEAVES_HPP

#include "../🤾Système de Comportement/🌿Arbre de Comportement/bt_node.hpp"

namespace godot
{
class AutomateController;

/// 🎯 Cherche le personnage le plus proche de l'agent (CharacterRegister) et le range dans le tableau noir
/// (key). SUCCESS s'il en trouve un, FAILURE sinon (la clé est alors effacée).
class BTFindTarget : public BTNode
{
    GDCLASS(BTFindTarget, BTNode)

  public:
    void set_key(const String &p_key) { key = p_key; }
    String get_key() const { return key; }
    void set_max_distance(double p_distance) { max_distance = p_distance; }
    double get_max_distance() const { return max_distance; }
    void set_hostile_only(bool p_hostile) { hostile_only = p_hostile; }
    bool get_hostile_only() const { return hostile_only; }
    String get_debug_kind() const override { return "condition"; }
    String get_debug_info() const override;

  protected:
    static void _bind_methods();
    Status _native_tick(double p_delta) override;

  private:
    String key = "cible";
    double max_distance = -1.0;
    bool hostile_only = true;
};

/// 🚶 Déplace l'agent (son AutomateController) vers la valeur de target_key : un nœud (suivi tant qu'il
/// bouge), un Vector2 ou un Vector3. RUNNING en chemin, SUCCESS à l'arrivée, FAILURE sans contrôleur ou
/// sans cible. arrive_distance < 0 : celle du contrôleur.
class BTMoveTo : public BTNode
{
    GDCLASS(BTMoveTo, BTNode)

  public:
    void set_target_key(const String &p_key) { target_key = p_key; }
    String get_target_key() const { return target_key; }
    void set_arrive_distance(double p_distance) { arrive_distance = p_distance; }
    double get_arrive_distance() const { return arrive_distance; }
    void set_run(bool p_run) { run = p_run; }
    bool get_run() const { return run; }
    String get_debug_info() const override { return U"→ " + target_key; }

  protected:
    static void _bind_methods();
    Status _native_tick(double p_delta) override;
    void _native_exit() override;

  private:
    String target_key = "cible";
    double arrive_distance = -1.0;
    bool run = false;

    AutomateController *_controller() const;
};

/// 🌟 Active une capacité de l'AbilitySystem de l'agent, sur la cible du tableau noir (target_key, facultatif).
/// SUCCESS si elle s'active (ou, avec wait_until_end, quand elle se termine), FAILURE sinon.
class BTUseAbility : public BTNode
{
    GDCLASS(BTUseAbility, BTNode)

  public:
    void set_ability_id(const StringName &p_id) { ability_id = p_id; }
    StringName get_ability_id() const { return ability_id; }
    void set_target_key(const String &p_key) { target_key = p_key; }
    String get_target_key() const { return target_key; }
    void set_wait_until_end(bool p_wait) { wait_until_end = p_wait; }
    bool get_wait_until_end() const { return wait_until_end; }
    String get_debug_info() const override { return String(U"🌟 ") + String(ability_id); }

  protected:
    static void _bind_methods();
    void _native_enter() override { started = false; }
    Status _native_tick(double p_delta) override;

  private:
    StringName ability_id;
    String target_key;
    bool wait_until_end = false;
    bool started = false;
};
} // namespace godot

#endif // BT_CHARACTER_LEAVES_HPP
