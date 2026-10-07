#ifndef ACHIEVEMENT_HPP
#define ACHIEVEMENT_HPP

#include "achievement_condition.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// 🏆 Succès (équivalent du ModAchievement de tModLoader) : nom, description, icône, catégorie, et liste de
/// conditions. Il se débloque quand ses conditions sont remplies (toutes, ou une seule selon completion_mode),
/// puis émet `unlocked` ; chaque progrès d'une condition émet `progressed`.
///
/// La barre de progression affichée suit progress_tracker (comme UseConditionsCompletedTracker /
/// UseTrackerFromCondition dans tModLoader). L'état de jeu (unlocked, unlock_time, état des conditions) n'est
/// jamais enregistré dans la ressource : il est sauvegardé par l'AchievementManager.
class Achievement : public Resource
{
    GDCLASS(Achievement, Resource)

  public:
    /// Catégories des succès de Terraria (filtre du menu des succès).
    enum Category
    {
        CATEGORY_NONE,
        CATEGORY_SLAYER,     // combat
        CATEGORY_COLLECTOR,  // objets
        CATEGORY_EXPLORER,   // exploration
        CATEGORY_CHALLENGER, // défis difficiles
    };

    enum CompletionMode
    {
        COMPLETION_ALL, // toutes les conditions (comportement de Terraria)
        COMPLETION_ANY, // une seule condition suffit
    };

    enum ProgressTracker
    {
        TRACKER_AUTO,                 // une condition : sa progression ; plusieurs : conditions remplies (ALL) ou meilleure (ANY)
        TRACKER_NONE,                 // pas de barre de progression (débloqué ou non)
        TRACKER_CONDITIONS_COMPLETED, // nombre de conditions remplies sur le total
        TRACKER_FROM_CONDITION,       // progression de la condition nommée tracked_condition
    };

    void set_id(const StringName &p_id) { id = p_id; }
    StringName get_id() const { return id; }
    void set_display_name(const String &p_name) { display_name = p_name; }
    String get_display_name() const { return display_name; }
    void set_description(const String &p_description) { description = p_description; }
    String get_description() const { return description; }
    void set_icon(const Ref<Texture2D> &p_icon) { icon = p_icon; }
    Ref<Texture2D> get_icon() const { return icon; }
    void set_locked_icon(const Ref<Texture2D> &p_icon) { locked_icon = p_icon; }
    Ref<Texture2D> get_locked_icon() const { return locked_icon; }
    /// Icône à afficher selon l'état : locked_icon tant que verrouillé (s'il est défini), sinon icon.
    Ref<Texture2D> get_current_icon() const { return !unlocked && locked_icon.is_valid() ? locked_icon : icon; }
    void set_category(Category p_category) { category = p_category; }
    Category get_category() const { return category; }
    void set_hidden(bool p_hidden) { hidden = p_hidden; }
    bool is_hidden() const { return hidden; }

    void set_conditions(const TypedArray<AchievementCondition> &p_conditions);
    TypedArray<AchievementCondition> get_conditions() const { return conditions; }
    void add_condition(const Ref<AchievementCondition> &p_condition);
    /// Condition nommée p_name, ou null.
    Ref<AchievementCondition> get_condition(const StringName &p_name) const;
    void set_completion_mode(CompletionMode p_mode) { completion_mode = p_mode; }
    CompletionMode get_completion_mode() const { return completion_mode; }
    void set_progress_tracker(ProgressTracker p_tracker) { progress_tracker = p_tracker; }
    ProgressTracker get_progress_tracker() const { return progress_tracker; }
    void set_tracked_condition(const StringName &p_name) { tracked_condition = p_name; }
    StringName get_tracked_condition() const { return tracked_condition; }

    bool is_unlocked() const { return unlocked; }
    /// Date du déblocage (secondes Unix), 0 tant que verrouillé.
    double get_unlock_time() const { return unlock_time; }

    /// Transmet un évènement aux conditions non remplies : vrai si l'une d'elles a progressé.
    bool notify(const StringName &p_event, const String &p_id, double p_amount);
    /// Débloque le succès si ses conditions sont remplies : vrai s'il vient d'être débloqué.
    bool evaluate();
    /// Débloque le succès sans condition (remplit aussi ses conditions).
    void unlock();
    /// Reverrouille le succès et remet ses conditions à zéro.
    void reset();

    int get_completed_condition_count() const;
    /// Vrai si le succès affiche une barre de progression (progress_tracker ≠ TRACKER_NONE et plus d'une étape).
    bool has_progress() const;
    /// Progression entre 0 et 1 (1 une fois débloqué).
    double get_progress() const;
    /// Progression lisible (« 3/10 », « 1/3 conditions »...) ; vide sans barre de progression.
    String get_progress_text() const;

    /// État sauvegardable { unlocked, unlock_time, conditions } ; set_state n'émet aucun signal.
    Dictionary get_state() const;
    void set_state(const Dictionary &p_state);

  protected:
    static void _bind_methods();

  private:
    StringName id;
    String display_name;
    String description;
    Ref<Texture2D> icon;
    Ref<Texture2D> locked_icon;
    Category category = CATEGORY_NONE;
    bool hidden = false;
    TypedArray<AchievementCondition> conditions;
    /// Conditions dont le signal `changed` est écouté (copie : conditions peut être modifié en place).
    TypedArray<AchievementCondition> connected_conditions;
    CompletionMode completion_mode = COMPLETION_ALL;
    ProgressTracker progress_tracker = TRACKER_AUTO;
    StringName tracked_condition;

    bool unlocked = false;
    double unlock_time = 0.0;
    /// Vrai pendant unlock() / reset() : les conditions modifiées en série ne réévaluent pas le succès.
    bool updating = false;

    void _connect_conditions();
    void _on_condition_changed();
    bool _conditions_satisfied() const;
    /// Condition dont la progression est affichée (TRACKER_FROM_CONDITION, ou seule condition en AUTO).
    Ref<AchievementCondition> _tracked() const;
    /// Clé d'une condition dans l'état sauvegardé : son nom, sinon son rang.
    String _condition_key(int64_t p_index) const;
};
} // namespace godot

VARIANT_ENUM_CAST(Achievement::Category);
VARIANT_ENUM_CAST(Achievement::CompletionMode);
VARIANT_ENUM_CAST(Achievement::ProgressTracker);

#endif // ACHIEVEMENT_HPP
