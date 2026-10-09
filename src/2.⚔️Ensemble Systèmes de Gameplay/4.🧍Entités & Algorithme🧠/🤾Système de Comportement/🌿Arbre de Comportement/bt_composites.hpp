#ifndef BT_COMPOSITES_HPP
#define BT_COMPOSITES_HPP

#include "bt_node.hpp"

#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
/// 🔀 Base des nœuds à plusieurs enfants (classe de base, non instanciable seule).
class BTComposite : public BTNode
{
    GDCLASS(BTComposite, BTNode)

  public:
    String get_debug_kind() const override { return "composite"; }
    PackedStringArray _get_configuration_warnings() const override;

  protected:
    static void _bind_methods() {}
};

/// Partie commune de la séquence et du sélecteur : parcours des enfants dans l'ordre (ou mélangé),
/// en reprenant à l'enfant en cours (ou depuis le début à chaque tick si reactive).
class BTOrderedComposite : public BTComposite
{
    GDCLASS(BTOrderedComposite, BTComposite)

  public:
    void set_reactive(bool p_reactive) { reactive = p_reactive; }
    bool is_reactive() const { return reactive; }
    void set_random_order(bool p_random) { random_order = p_random; }
    bool is_random_order() const { return random_order; }

    String get_debug_info() const override;

  protected:
    static void _bind_methods();
    void _native_enter() override;
    Status _native_tick(double p_delta) override;

    /// Statut qui arrête le parcours (FAILURE pour une séquence, SUCCESS pour un sélecteur).
    virtual Status _stop_status() const = 0;

  private:
    bool reactive = false;
    bool random_order = false;
    /// Ordre de parcours des enfants (mélangé si random_order), fixé à chaque _native_enter.
    LocalVector<int> order;
    /// Rang (dans order) de l'enfant où reprendre au prochain tick (hors mode reactive).
    int current = 0;
};

/// ➡️ Séquence : exécute ses enfants dans l'ordre ; échoue dès qu'un enfant échoue, réussit si tous réussissent.
class BTSequence : public BTOrderedComposite
{
    GDCLASS(BTSequence, BTOrderedComposite)

  protected:
    static void _bind_methods() {}
    Status _stop_status() const override { return FAILURE; }
};

/// ❓ Sélecteur (fallback) : essaie ses enfants dans l'ordre ; réussit dès qu'un enfant réussit, échoue si tous échouent.
class BTSelector : public BTOrderedComposite
{
    GDCLASS(BTSelector, BTOrderedComposite)

  protected:
    static void _bind_methods() {}
    Status _stop_status() const override { return SUCCESS; }
};

/// ⇉ Parallèle : exécute tous ses enfants à chaque tick. Réussit quand success_threshold enfants ont réussi
/// (0 = tous), échoue quand failure_threshold enfants ont échoué (0 = tous). Les enfants encore en cours
/// sont alors interrompus.
class BTParallel : public BTComposite
{
    GDCLASS(BTParallel, BTComposite)

  public:
    void set_success_threshold(int p_threshold) { success_threshold = MAX(0, p_threshold); }
    int get_success_threshold() const { return success_threshold; }
    void set_failure_threshold(int p_threshold) { failure_threshold = MAX(0, p_threshold); }
    int get_failure_threshold() const { return failure_threshold; }

    String get_debug_info() const override;

  protected:
    static void _bind_methods();
    void _native_enter() override;
    Status _native_tick(double p_delta) override;

  private:
    int success_threshold = 0;
    int failure_threshold = 1;
    LocalVector<int> results; // statut de chaque enfant pendant l'exécution en cours (-1 : pas terminé)
    int successes = 0;
    int failures = 0;
};
} // namespace godot

#endif // BT_COMPOSITES_HPP
