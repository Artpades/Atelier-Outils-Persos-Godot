#ifndef ACHIEVEMENT_MANAGER_HPP
#define ACHIEVEMENT_MANAGER_HPP

#include "achievement.hpp"

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/node.hpp>

namespace godot
{
/// 🏆 Gestionnaire de succès (équivalent de l'AchievementManager / AchievementsHelper de Terraria) : liste des
/// succès, transmission des évènements de jeu à leurs conditions, sauvegarde, et notification à l'écran.
///
/// Le jeu signale ce qui se passe avec notify(évènement, identifiant, quantité) — « enemy_killed », "slime" —
/// sans connaître les succès ; chaque condition décide si l'évènement la concerne. Quand un succès se débloque,
/// le gestionnaire émet achievement_unlocked, affiche une notification, sauvegarde (auto_save), puis émet
/// lui-même l'évènement « achievement_unlocked » (identifiant = id du succès) : des succès peuvent donc
/// dépendre d'autres succès (AchievementCollectionCondition, AchievementCounterCondition).
///
/// À placer de préférence en autoload pour qu'il soit accessible partout.
class AchievementManager : public Node
{
    GDCLASS(AchievementManager, Node)

  public:
    enum NotificationPosition
    {
        NOTIFICATION_POSITION_TOP_LEFT,
        NOTIFICATION_POSITION_TOP_CENTER,
        NOTIFICATION_POSITION_TOP_RIGHT,
        NOTIFICATION_POSITION_BOTTOM_LEFT,
        NOTIFICATION_POSITION_BOTTOM_CENTER,
        NOTIFICATION_POSITION_BOTTOM_RIGHT,
    };

    /// Évènement émis par le gestionnaire à chaque déblocage (identifiant = id du succès).
    static inline const char *UNLOCKED_EVENT = "achievement_unlocked";

    void _notification(int p_what);

    // 📋 Succès
    void set_achievements(const TypedArray<Achievement> &p_achievements);
    TypedArray<Achievement> get_achievements() const { return achievements; }
    void register_achievement(const Ref<Achievement> &p_achievement);
    void unregister_achievement(const Ref<Achievement> &p_achievement);
    Ref<Achievement> get_achievement(const StringName &p_id) const;
    bool has_achievement(const StringName &p_id) const { return get_achievement(p_id).is_valid(); }
    TypedArray<Achievement> get_achievements_in_category(Achievement::Category p_category) const;
    TypedArray<Achievement> get_unlocked_achievements() const;

    // 📣 Évènements et progression
    /// Signale un évènement de jeu à toutes les conditions des succès verrouillés.
    void notify(const StringName &p_event, const Variant &p_id = "", double p_amount = 1.0);
    /// Remplit la condition p_condition du succès p_id (comme Condition.Complete() dans tModLoader).
    void complete_condition(const StringName &p_id, const StringName &p_condition);
    /// Ajoute p_amount à une condition à compteur du succès p_id.
    void add_progress(const StringName &p_id, const StringName &p_condition, double p_amount = 1.0);
    void unlock(const StringName &p_id);
    bool is_unlocked(const StringName &p_id) const;
    void reset_achievement(const StringName &p_id);
    void reset_all();

    int get_achievement_count() const;
    int get_unlocked_count() const;
    /// Part des succès débloqués, entre 0 et 1.
    double get_completion_ratio() const;

    // 💾 Sauvegarde
    void set_save_path(const String &p_path) { save_path = p_path; }
    String get_save_path() const { return save_path; }
    void set_save_password(const String &p_password) { save_password = p_password; }
    String get_save_password() const { return save_password; }
    void set_auto_load(bool p_enabled) { auto_load = p_enabled; }
    bool get_auto_load() const { return auto_load; }
    void set_auto_save(bool p_enabled) { auto_save = p_enabled; }
    bool get_auto_save() const { return auto_save; }
    Error save_achievements() const;
    /// Recharge l'état depuis save_path (fichier absent : OK, rien ne change). N'émet aucun signal de déblocage.
    Error load_achievements();
    /// État de tous les succès { id: état } pour un système de sauvegarde externe ; set_state n'émet aucun signal.
    Dictionary get_state() const;
    void set_state(const Dictionary &p_state);

    // 🔔 Notifications
    void set_show_notifications(bool p_enabled) { show_notifications = p_enabled; }
    bool get_show_notifications() const { return show_notifications; }
    void set_notification_duration(double p_duration) { notification_duration = MAX(p_duration, 0.0); }
    double get_notification_duration() const { return notification_duration; }
    void set_notification_position(NotificationPosition p_position);
    NotificationPosition get_notification_position() const { return notification_position; }
    void set_unlock_sound(const Ref<AudioStream> &p_sound) { unlock_sound = p_sound; }
    Ref<AudioStream> get_unlock_sound() const { return unlock_sound; }
    /// Affiche la notification de déblocage de p_achievement (appelé automatiquement si show_notifications).
    void show_notification(const Ref<Achievement> &p_achievement);
    /// Conteneur des notifications affichées (une PanelContainer par notification).
    BoxContainer *get_notification_container() const { return notification_box; }

  protected:
    static void _bind_methods();

  private:
    TypedArray<Achievement> achievements;
    /// Succès dont les signaux sont écoutés (copie : achievements peut être modifié en place).
    TypedArray<Achievement> connected_achievements;

    String save_path = "user://achievements.cfg";
    String save_password;
    bool auto_load = true;
    bool auto_save = true;

    bool show_notifications = true;
    double notification_duration = 3.0;
    NotificationPosition notification_position = NOTIFICATION_POSITION_TOP_RIGHT;
    Ref<AudioStream> unlock_sound;

    CanvasLayer *notification_layer = nullptr;
    BoxContainer *notification_box = nullptr;
    AudioStreamPlayer *sound_player = nullptr;

    void _connect_achievement(const Ref<Achievement> &p_achievement);
    void _disconnect_achievement(const Ref<Achievement> &p_achievement);
    void _on_achievement_unlocked(const Ref<Achievement> &p_achievement);
    void _on_achievement_progressed(double p_progress, const Ref<Achievement> &p_achievement);
    void _build_notification_layer();
    void _place_notification_box();
};
} // namespace godot

VARIANT_ENUM_CAST(AchievementManager::NotificationPosition);

#endif // ACHIEVEMENT_MANAGER_HPP
