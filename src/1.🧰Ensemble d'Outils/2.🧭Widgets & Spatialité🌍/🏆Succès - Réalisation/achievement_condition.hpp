#ifndef ACHIEVEMENT_CONDITION_HPP
#define ACHIEVEMENT_CONDITION_HPP

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot
{
/// 🚩 Condition d'un succès (équivalent du CustomFlagCondition de tModLoader) : remplie une fois pour toutes,
/// par code (complete) ou à la réception d'un évènement (AchievementManager.notify) dont le nom vaut event_name
/// et dont l'identifiant figure dans event_ids (liste vide : n'importe quel identifiant).
///
/// Les conditions dérivées redéfinissent la progression : AchievementCounterCondition (compteur / valeur à
/// atteindre) et AchievementCollectionCondition (liste d'identifiants à tous obtenir).
/// Un script peut redéfinir _handle_event() pour une logique personnalisée (il appelle alors complete()).
/// Chaque changement d'état émet `changed` : le succès qui contient la condition se réévalue.
class AchievementCondition : public Resource
{
    GDCLASS(AchievementCondition, Resource)

  public:
    void set_condition_name(const StringName &p_name) { condition_name = p_name; }
    StringName get_condition_name() const { return condition_name; }
    void set_event_name(const StringName &p_event) { event_name = p_event; }
    StringName get_event_name() const { return event_name; }
    void set_event_ids(const PackedStringArray &p_ids) { event_ids = p_ids; }
    PackedStringArray get_event_ids() const { return event_ids; }

    bool is_completed() const { return completed; }
    /// Remplit la condition (un compteur passe à sa cible, une collection est complétée).
    virtual void complete();
    /// Remet la condition à zéro.
    virtual void reset();

    /// Valeur actuelle et cible de la progression (drapeau : 0 ou 1 sur 1).
    virtual double get_value() const { return completed ? 1.0 : 0.0; }
    virtual double get_target() const { return 1.0; }
    /// Progression entre 0 et 1.
    double get_progress() const;
    /// Progression lisible : « 3/10 » (« 0/1 » pour un drapeau).
    String get_progress_text() const;

    /// Vrai si l'évènement concerne cette condition (nom et identifiant).
    virtual bool matches(const StringName &p_event, const String &p_id) const;
    /// Transmet un évènement : vrai si la condition l'a pris en compte.
    bool notify(const StringName &p_event, const String &p_id, double p_amount);

    /// État sauvegardable (et restauré sans émettre de signal).
    virtual Dictionary get_state() const;
    virtual void set_state(const Dictionary &p_state);

  protected:
    static void _bind_methods();

    /// Applique un évènement reconnu par matches().
    virtual void _apply_event(const String &p_id, double p_amount);
    /// Change l'état « rempli » et émet `changed` (et `completed` à la première complétion).
    void _set_completed(bool p_completed);

    GDVIRTUAL3R(bool, _handle_event, StringName, String, double)

    bool completed = false;

  private:
    StringName condition_name;
    StringName event_name;
    PackedStringArray event_ids;
};

/// 🔢 Condition à compteur (équivalent des CustomIntCondition / CustomFloatCondition de tModLoader, et des
/// conditions « tuer N ennemis », « ramasser N objets ») : remplie quand value atteint target.
class AchievementCounterCondition : public AchievementCondition
{
    GDCLASS(AchievementCounterCondition, AchievementCondition)

  public:
    enum ValueMode
    {
        VALUE_MODE_ADD, // chaque évènement ajoute sa quantité (cumul : ennemis tués, pièces ramassées...)
        VALUE_MODE_MAX, // garde la plus grande quantité reçue (record : meilleur score, plus haute altitude...)
        VALUE_MODE_SET, // la dernière quantité reçue remplace la valeur (état : nombre d'objets possédés...)
    };

    void set_target(double p_target);
    double get_target() const override { return target; }
    void set_value(double p_value);
    double get_value() const override { return value; }
    void set_value_mode(ValueMode p_mode) { value_mode = p_mode; }
    ValueMode get_value_mode() const { return value_mode; }

    /// Ajoute p_amount à la valeur.
    void add(double p_amount = 1.0);

    void complete() override;
    void reset() override;
    Dictionary get_state() const override;
    void set_state(const Dictionary &p_state) override;

  protected:
    static void _bind_methods();
    void _apply_event(const String &p_id, double p_amount) override;

  private:
    double target = 10.0;
    double value = 0.0;
    ValueMode value_mode = VALUE_MODE_ADD;

    void _refresh();
};

/// 🗂️ Condition de collection (équivalent d'une liste de ItemPickupCondition de tModLoader) : remplie quand
/// chaque identifiant de required_ids a été reçu au moins une fois avec l'évènement event_name
/// (« ramasser chaque gemme », « visiter chaque biome », « débloquer ces succès »).
class AchievementCollectionCondition : public AchievementCondition
{
    GDCLASS(AchievementCollectionCondition, AchievementCondition)

  public:
    void set_required_ids(const PackedStringArray &p_ids);
    PackedStringArray get_required_ids() const { return required_ids; }
    PackedStringArray get_collected_ids() const { return collected_ids; }
    PackedStringArray get_missing_ids() const;

    /// Marque p_id comme obtenu (ignoré s'il n'est pas dans required_ids) : vrai s'il était manquant.
    bool collect(const String &p_id);
    bool has_collected(const String &p_id) const { return collected_ids.has(p_id); }

    double get_value() const override { return collected_ids.size(); }
    double get_target() const override { return required_ids.size(); }
    bool matches(const StringName &p_event, const String &p_id) const override;
    void complete() override;
    void reset() override;
    Dictionary get_state() const override;
    void set_state(const Dictionary &p_state) override;

  protected:
    static void _bind_methods();
    void _apply_event(const String &p_id, double p_amount) override;

  private:
    PackedStringArray required_ids;
    PackedStringArray collected_ids;

    void _refresh();
};
} // namespace godot

VARIANT_ENUM_CAST(AchievementCounterCondition::ValueMode);

#endif // ACHIEVEMENT_CONDITION_HPP
