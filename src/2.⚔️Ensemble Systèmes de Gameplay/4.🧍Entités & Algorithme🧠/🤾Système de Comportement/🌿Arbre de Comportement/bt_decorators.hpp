#ifndef BT_DECORATORS_HPP
#define BT_DECORATORS_HPP

#include "bt_node.hpp"

namespace godot
{
/// ◇ Base des nœuds à un seul enfant, qui modifient son résultat ou son exécution
/// (classe de base, non instanciable seule). Sans script, transmet le statut de l'enfant.
class BTDecorator : public BTNode
{
    GDCLASS(BTDecorator, BTNode)

  public:
    /// Premier BTNode enfant (nullptr s'il n'y en a pas).
    BTNode *get_child_node() const { return get_bt_child(0); }
    String get_debug_kind() const override { return "decorator"; }
    PackedStringArray _get_configuration_warnings() const override;

  protected:
    static void _bind_methods();
    Status _native_tick(double p_delta) override;
};

/// 🔁 Inverseur : SUCCESS ↔ FAILURE (RUNNING inchangé).
class BTInverter : public BTDecorator
{
    GDCLASS(BTInverter, BTDecorator)

  protected:
    static void _bind_methods() {}
    Status _native_tick(double p_delta) override;
};

/// ✅ Résultat forcé : renvoie toujours result une fois l'enfant terminé (RUNNING inchangé).
class BTForceResult : public BTDecorator
{
    GDCLASS(BTForceResult, BTDecorator)

  public:
    void set_result(Status p_result) { result = p_result == RUNNING ? SUCCESS : p_result; }
    Status get_result() const { return result; }
    String get_debug_info() const override { return result == SUCCESS ? "SUCCESS" : "FAILURE"; }

  protected:
    static void _bind_methods();
    Status _native_tick(double p_delta) override;

  private:
    Status result = SUCCESS;
};

/// 🔂 Répéteur : relance l'enfant à chaque fin d'exécution (une fois par tick au plus).
/// count = 0 : sans fin. mode : toujours, jusqu'à une réussite, ou jusqu'à un échec.
class BTRepeater : public BTDecorator
{
    GDCLASS(BTRepeater, BTDecorator)

  public:
    enum RepeatMode
    {
        REPEAT_ALWAYS,        // compte les exécutions, SUCCESS une fois count atteint
        REPEAT_UNTIL_SUCCESS, // SUCCESS dès que l'enfant réussit ; FAILURE si count est atteint avant
        REPEAT_UNTIL_FAILURE, // SUCCESS dès que l'enfant échoue ; FAILURE si count est atteint avant
    };

    void set_count(int p_count) { count = MAX(0, p_count); }
    int get_count() const { return count; }
    void set_mode(RepeatMode p_mode) { mode = p_mode; }
    RepeatMode get_mode() const { return mode; }
    int get_iteration() const { return iteration; }
    String get_debug_info() const override;

  protected:
    static void _bind_methods();
    void _native_enter() override { iteration = 0; }
    Status _native_tick(double p_delta) override;

  private:
    int count = 0;
    RepeatMode mode = REPEAT_ALWAYS;
    int iteration = 0;
};

/// ⏳ Temps de recharge : après une réussite de l'enfant (ou un échec si include_failures), renvoie
/// FAILURE sans exécuter l'enfant pendant duration secondes (temps de l'arbre).
class BTCooldown : public BTDecorator
{
    GDCLASS(BTCooldown, BTDecorator)

  public:
    void set_duration(double p_duration) { duration = MAX(0.0, p_duration); }
    double get_duration() const { return duration; }
    void set_include_failures(bool p_include) { include_failures = p_include; }
    bool get_include_failures() const { return include_failures; }
    /// Temps de recharge restant (0 si disponible).
    double get_remaining() const;
    void reset_cooldown() { ready_time = -1.0; }
    String get_debug_info() const override;

  protected:
    static void _bind_methods();
    Status _native_tick(double p_delta) override;

  private:
    double duration = 1.0;
    bool include_failures = false;
    double ready_time = -1.0; // temps de l'arbre à partir duquel l'enfant peut de nouveau s'exécuter
};

/// ⏱️ Limite de temps : interrompt l'enfant et renvoie FAILURE s'il tourne plus de duration secondes.
class BTTimeLimit : public BTDecorator
{
    GDCLASS(BTTimeLimit, BTDecorator)

  public:
    void set_duration(double p_duration) { duration = MAX(0.0, p_duration); }
    double get_duration() const { return duration; }
    String get_debug_info() const override;

  protected:
    static void _bind_methods();
    Status _native_tick(double p_delta) override;

  private:
    double duration = 5.0;
};
} // namespace godot

VARIANT_ENUM_CAST(BTRepeater::RepeatMode);

#endif // BT_DECORATORS_HPP
