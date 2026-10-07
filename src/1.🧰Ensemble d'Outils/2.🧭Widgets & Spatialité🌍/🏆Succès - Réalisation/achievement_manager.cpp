#include "achievement_manager.hpp"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/classes/callback_tweener.hpp>
#include <godot_cpp/classes/interval_tweener.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

void AchievementManager::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return; // ni notification affichée, ni sauvegarde dans l'éditeur
    }
    switch (p_what)
    {
    case NOTIFICATION_READY:
        _build_notification_layer();
        if (auto_load)
        {
            load_achievements();
        }
        break;
    case NOTIFICATION_EXIT_TREE:
        if (auto_save)
        {
            save_achievements();
        }
        break;
    default:
        break;
    }
}

// 📋 Succès

void AchievementManager::set_achievements(const TypedArray<Achievement> &p_achievements)
{
    for (int64_t i = 0; i < connected_achievements.size(); i++)
    {
        _disconnect_achievement(connected_achievements[i]);
    }
    achievements = p_achievements;
    // Nouveau tableau (pas une copie partagée de achievements, cf. Array partagé en godot-cpp).
    connected_achievements = TypedArray<Achievement>();
    for (int64_t i = 0; i < achievements.size(); i++)
    {
        _connect_achievement(achievements[i]);
    }
}

void AchievementManager::register_achievement(const Ref<Achievement> &p_achievement)
{
    ERR_FAIL_COND(p_achievement.is_null());
    if (achievements.has(p_achievement))
    {
        return;
    }
    ERR_FAIL_COND_MSG(has_achievement(p_achievement->get_id()), String(U"[AchievementManager] Un succès d'id « ") + String(p_achievement->get_id()) + U" » existe déjà.");
    achievements.push_back(p_achievement);
    _connect_achievement(p_achievement);
}

void AchievementManager::unregister_achievement(const Ref<Achievement> &p_achievement)
{
    achievements.erase(p_achievement);
    _disconnect_achievement(p_achievement);
}

Ref<Achievement> AchievementManager::get_achievement(const StringName &p_id) const
{
    for (int64_t i = 0; i < achievements.size(); i++)
    {
        Ref<Achievement> achievement = achievements[i];
        if (achievement.is_valid() && achievement->get_id() == p_id)
        {
            return achievement;
        }
    }
    return Ref<Achievement>();
}

TypedArray<Achievement> AchievementManager::get_achievements_in_category(Achievement::Category p_category) const
{
    TypedArray<Achievement> result;
    for (int64_t i = 0; i < achievements.size(); i++)
    {
        Ref<Achievement> achievement = achievements[i];
        if (achievement.is_valid() && achievement->get_category() == p_category)
        {
            result.push_back(achievement);
        }
    }
    return result;
}

TypedArray<Achievement> AchievementManager::get_unlocked_achievements() const
{
    TypedArray<Achievement> result;
    for (int64_t i = 0; i < achievements.size(); i++)
    {
        Ref<Achievement> achievement = achievements[i];
        if (achievement.is_valid() && achievement->is_unlocked())
        {
            result.push_back(achievement);
        }
    }
    return result;
}

void AchievementManager::_connect_achievement(const Ref<Achievement> &p_achievement)
{
    if (p_achievement.is_null() || connected_achievements.has(p_achievement))
    {
        return;
    }
    p_achievement->connect("unlocked", callable_mp(this, &AchievementManager::_on_achievement_unlocked).bind(p_achievement));
    p_achievement->connect("progressed", callable_mp(this, &AchievementManager::_on_achievement_progressed).bind(p_achievement));
    connected_achievements.push_back(p_achievement);
}

void AchievementManager::_disconnect_achievement(const Ref<Achievement> &p_achievement)
{
    if (p_achievement.is_null() || !connected_achievements.has(p_achievement))
    {
        return;
    }
    p_achievement->disconnect("unlocked", callable_mp(this, &AchievementManager::_on_achievement_unlocked).bind(p_achievement));
    p_achievement->disconnect("progressed", callable_mp(this, &AchievementManager::_on_achievement_progressed).bind(p_achievement));
    connected_achievements.erase(p_achievement);
}

// 📣 Évènements et progression

void AchievementManager::notify(const StringName &p_event, const Variant &p_id, double p_amount)
{
    // Identifiant quelconque (texte, nombre, StringName...) comparé sous forme de texte.
    const String id = p_id;
    // Émis avant la transmission : les déblocages qu'il provoque sont signalés après lui.
    emit_signal("event_notified", p_event, id, p_amount);
    for (int64_t i = 0; i < achievements.size(); i++)
    {
        Ref<Achievement> achievement = achievements[i];
        if (achievement.is_valid())
        {
            achievement->notify(p_event, id, p_amount);
        }
    }
}

void AchievementManager::complete_condition(const StringName &p_id, const StringName &p_condition)
{
    Ref<Achievement> achievement = get_achievement(p_id);
    ERR_FAIL_COND_MSG(achievement.is_null(), String(U"[AchievementManager] Succès introuvable : ") + String(p_id));
    Ref<AchievementCondition> condition = achievement->get_condition(p_condition);
    ERR_FAIL_COND_MSG(condition.is_null(), String(U"[AchievementManager] Condition introuvable : ") + String(p_id) + "/" + String(p_condition));
    condition->complete();
}

void AchievementManager::add_progress(const StringName &p_id, const StringName &p_condition, double p_amount)
{
    Ref<Achievement> achievement = get_achievement(p_id);
    ERR_FAIL_COND_MSG(achievement.is_null(), String(U"[AchievementManager] Succès introuvable : ") + String(p_id));
    Ref<AchievementCounterCondition> counter = achievement->get_condition(p_condition);
    ERR_FAIL_COND_MSG(counter.is_null(), String(U"[AchievementManager] Condition à compteur introuvable : ") + String(p_id) + "/" + String(p_condition));
    if (!achievement->is_unlocked())
    {
        counter->add(p_amount);
    }
}

void AchievementManager::unlock(const StringName &p_id)
{
    Ref<Achievement> achievement = get_achievement(p_id);
    ERR_FAIL_COND_MSG(achievement.is_null(), String(U"[AchievementManager] Succès introuvable : ") + String(p_id));
    achievement->unlock();
}

bool AchievementManager::is_unlocked(const StringName &p_id) const
{
    Ref<Achievement> achievement = get_achievement(p_id);
    return achievement.is_valid() && achievement->is_unlocked();
}

void AchievementManager::reset_achievement(const StringName &p_id)
{
    Ref<Achievement> achievement = get_achievement(p_id);
    ERR_FAIL_COND_MSG(achievement.is_null(), String(U"[AchievementManager] Succès introuvable : ") + String(p_id));
    achievement->reset();
}

void AchievementManager::reset_all()
{
    for (int64_t i = 0; i < achievements.size(); i++)
    {
        Ref<Achievement> achievement = achievements[i];
        if (achievement.is_valid())
        {
            achievement->reset();
        }
    }
}

int AchievementManager::get_achievement_count() const
{
    int count = 0;
    for (int64_t i = 0; i < achievements.size(); i++)
    {
        count += Ref<Achievement>(achievements[i]).is_valid() ? 1 : 0;
    }
    return count;
}

int AchievementManager::get_unlocked_count() const
{
    return get_unlocked_achievements().size();
}

double AchievementManager::get_completion_ratio() const
{
    const int total = get_achievement_count();
    return total == 0 ? 0.0 : double(get_unlocked_count()) / double(total);
}

void AchievementManager::_on_achievement_unlocked(const Ref<Achievement> &p_achievement)
{
    emit_signal("achievement_unlocked", p_achievement);
    if (show_notifications)
    {
        show_notification(p_achievement);
    }
    if (sound_player && unlock_sound.is_valid())
    {
        sound_player->set_stream(unlock_sound);
        sound_player->play();
    }
    if (auto_save && is_inside_tree() && !Engine::get_singleton()->is_editor_hint())
    {
        save_achievements();
    }
    // Succès qui dépendent d'autres succès
    notify(UNLOCKED_EVENT, String(p_achievement->get_id()), 1.0);
}

void AchievementManager::_on_achievement_progressed(double p_progress, const Ref<Achievement> &p_achievement)
{
    emit_signal("achievement_progressed", p_achievement, p_progress);
}

// 💾 Sauvegarde

Dictionary AchievementManager::get_state() const
{
    Dictionary state;
    for (int64_t i = 0; i < achievements.size(); i++)
    {
        Ref<Achievement> achievement = achievements[i];
        if (achievement.is_valid())
        {
            state[achievement->get_id()] = achievement->get_state();
        }
    }
    return state;
}

void AchievementManager::set_state(const Dictionary &p_state)
{
    for (int64_t i = 0; i < achievements.size(); i++)
    {
        Ref<Achievement> achievement = achievements[i];
        if (achievement.is_null())
        {
            continue;
        }
        // Clés StringName (get_state) ou String (fichier, JSON...) ; succès absent : verrouillé.
        const StringName id = achievement->get_id();
        Variant entry = p_state.has(id) ? p_state[id] : p_state.get(String(id), Dictionary());
        achievement->set_state(entry.get_type() == Variant::DICTIONARY ? Dictionary(entry) : Dictionary());
    }
}

Error AchievementManager::save_achievements() const
{
    ERR_FAIL_COND_V_MSG(save_path.is_empty(), ERR_FILE_BAD_PATH, String(U"[AchievementManager] save_path est vide."));
    Ref<ConfigFile> file;
    file.instantiate();
    const Dictionary state = get_state();
    const Array ids = state.keys();
    for (int64_t i = 0; i < ids.size(); i++)
    {
        // Une section par succès, une clé par champ : fichier lisible (sauf mot de passe).
        const Dictionary achievement_state = state[ids[i]];
        const Array keys = achievement_state.keys();
        for (int64_t k = 0; k < keys.size(); k++)
        {
            file->set_value(String(ids[i]), keys[k], achievement_state[keys[k]]);
        }
    }
    const Error error = save_password.is_empty() ? file->save(save_path) : file->save_encrypted_pass(save_path, save_password);
    ERR_FAIL_COND_V_MSG(error != OK, error, String(U"[AchievementManager] Échec de la sauvegarde : ") + save_path);
    return OK;
}

Error AchievementManager::load_achievements()
{
    if (save_path.is_empty() || !FileAccess::file_exists(save_path))
    {
        return OK; // première partie : rien à charger
    }
    Ref<ConfigFile> file;
    file.instantiate();
    const Error error = save_password.is_empty() ? file->load(save_path) : file->load_encrypted_pass(save_path, save_password);
    ERR_FAIL_COND_V_MSG(error != OK, error, String(U"[AchievementManager] Échec du chargement : ") + save_path);

    Dictionary state;
    const PackedStringArray sections = file->get_sections();
    for (const String &section : sections)
    {
        Dictionary achievement_state;
        const PackedStringArray keys = file->get_section_keys(section);
        for (const String &key : keys)
        {
            achievement_state[key] = file->get_value(section, key);
        }
        state[section] = achievement_state;
    }
    set_state(state);
    return OK;
}

// 🔔 Notifications

void AchievementManager::_build_notification_layer()
{
    if (notification_layer)
    {
        return;
    }
    notification_layer = memnew(CanvasLayer);
    notification_layer->set_name("AchievementNotifications");
    notification_layer->set_layer(100);
    add_child(notification_layer, false, INTERNAL_MODE_BACK);

    notification_box = memnew(VBoxContainer);
    notification_box->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    notification_box->add_theme_constant_override("separation", 8);
    notification_layer->add_child(notification_box);
    _place_notification_box();

    sound_player = memnew(AudioStreamPlayer);
    add_child(sound_player, false, INTERNAL_MODE_BACK);
}

void AchievementManager::set_notification_position(NotificationPosition p_position)
{
    notification_position = p_position;
    _place_notification_box();
}

void AchievementManager::_place_notification_box()
{
    if (!notification_box)
    {
        return;
    }
    static const Control::LayoutPreset presets[] = {
        Control::PRESET_TOP_LEFT, Control::PRESET_CENTER_TOP, Control::PRESET_TOP_RIGHT,
        Control::PRESET_BOTTOM_LEFT, Control::PRESET_CENTER_BOTTOM, Control::PRESET_BOTTOM_RIGHT,
    };
    // Taille minimale : le conteneur grandit vers l'intérieur de l'écran à chaque notification.
    notification_box->set_anchors_and_offsets_preset(presets[notification_position], Control::PRESET_MODE_MINSIZE, 16);
}

void AchievementManager::show_notification(const Ref<Achievement> &p_achievement)
{
    ERR_FAIL_COND(p_achievement.is_null());
    if (!notification_box)
    {
        return; // hors de l'arbre ou dans l'éditeur
    }

    PanelContainer *toast = memnew(PanelContainer);
    toast->set_name(String("Notification_") + String(p_achievement->get_id()));
    toast->set_meta("achievement_id", p_achievement->get_id());
    toast->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    toast->set_custom_minimum_size(Vector2(280, 0));
    Ref<StyleBoxFlat> style;
    style.instantiate();
    style->set_bg_color(Color(0.1f, 0.1f, 0.13f, 0.92f));
    style->set_border_color(Color(0.95f, 0.75f, 0.25f));
    style->set_border_width_all(2);
    style->set_corner_radius_all(8);
    style->set_content_margin_all(10);
    toast->add_theme_stylebox_override("panel", style);

    HBoxContainer *row = memnew(HBoxContainer);
    row->add_theme_constant_override("separation", 10);
    row->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    toast->add_child(row);

    if (p_achievement->get_icon().is_valid())
    {
        TextureRect *icon = memnew(TextureRect);
        icon->set_texture(p_achievement->get_icon());
        icon->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
        icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        icon->set_custom_minimum_size(Vector2(48, 48));
        icon->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        row->add_child(icon);
    }

    VBoxContainer *texts = memnew(VBoxContainer);
    texts->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    row->add_child(texts);
    Label *title = memnew(Label);
    title->set_text(U"🏆 Succès débloqué !");
    title->add_theme_font_size_override("font_size", 13);
    title->add_theme_color_override("font_color", Color(0.95f, 0.75f, 0.25f));
    texts->add_child(title);
    Label *name = memnew(Label);
    name->set_name("Name");
    name->set_text(p_achievement->get_display_name().is_empty() ? String(p_achievement->get_id()) : p_achievement->get_display_name());
    name->add_theme_font_size_override("font_size", 18);
    texts->add_child(name);

    toast->set_modulate(Color(1, 1, 1, 0));
    notification_box->add_child(toast);

    // Apparition, attente, disparition puis libération (le tween meurt avec la notification).
    Ref<Tween> tween = toast->create_tween();
    tween->tween_property(toast, "modulate:a", 1.0, 0.25);
    tween->tween_interval(notification_duration);
    tween->tween_property(toast, "modulate:a", 0.0, 0.4);
    tween->tween_callback(Callable(toast, "queue_free"));
}

// 🔗 Liaisons

void AchievementManager::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_achievements", "achievements"), &AchievementManager::set_achievements);
    ClassDB::bind_method(D_METHOD("get_achievements"), &AchievementManager::get_achievements);
    ClassDB::bind_method(D_METHOD("register_achievement", "achievement"), &AchievementManager::register_achievement);
    ClassDB::bind_method(D_METHOD("unregister_achievement", "achievement"), &AchievementManager::unregister_achievement);
    ClassDB::bind_method(D_METHOD("get_achievement", "id"), &AchievementManager::get_achievement);
    ClassDB::bind_method(D_METHOD("has_achievement", "id"), &AchievementManager::has_achievement);
    ClassDB::bind_method(D_METHOD("get_achievements_in_category", "category"), &AchievementManager::get_achievements_in_category);
    ClassDB::bind_method(D_METHOD("get_unlocked_achievements"), &AchievementManager::get_unlocked_achievements);

    ClassDB::bind_method(D_METHOD("notify", "event", "id", "amount"), &AchievementManager::notify, DEFVAL(""), DEFVAL(1.0));
    ClassDB::bind_method(D_METHOD("complete_condition", "id", "condition"), &AchievementManager::complete_condition);
    ClassDB::bind_method(D_METHOD("add_progress", "id", "condition", "amount"), &AchievementManager::add_progress, DEFVAL(1.0));
    ClassDB::bind_method(D_METHOD("unlock", "id"), &AchievementManager::unlock);
    ClassDB::bind_method(D_METHOD("is_unlocked", "id"), &AchievementManager::is_unlocked);
    ClassDB::bind_method(D_METHOD("reset_achievement", "id"), &AchievementManager::reset_achievement);
    ClassDB::bind_method(D_METHOD("reset_all"), &AchievementManager::reset_all);
    ClassDB::bind_method(D_METHOD("get_achievement_count"), &AchievementManager::get_achievement_count);
    ClassDB::bind_method(D_METHOD("get_unlocked_count"), &AchievementManager::get_unlocked_count);
    ClassDB::bind_method(D_METHOD("get_completion_ratio"), &AchievementManager::get_completion_ratio);

    ClassDB::bind_method(D_METHOD("set_save_path", "path"), &AchievementManager::set_save_path);
    ClassDB::bind_method(D_METHOD("get_save_path"), &AchievementManager::get_save_path);
    ClassDB::bind_method(D_METHOD("set_save_password", "password"), &AchievementManager::set_save_password);
    ClassDB::bind_method(D_METHOD("get_save_password"), &AchievementManager::get_save_password);
    ClassDB::bind_method(D_METHOD("set_auto_load", "enabled"), &AchievementManager::set_auto_load);
    ClassDB::bind_method(D_METHOD("get_auto_load"), &AchievementManager::get_auto_load);
    ClassDB::bind_method(D_METHOD("set_auto_save", "enabled"), &AchievementManager::set_auto_save);
    ClassDB::bind_method(D_METHOD("get_auto_save"), &AchievementManager::get_auto_save);
    ClassDB::bind_method(D_METHOD("save_achievements"), &AchievementManager::save_achievements);
    ClassDB::bind_method(D_METHOD("load_achievements"), &AchievementManager::load_achievements);
    ClassDB::bind_method(D_METHOD("get_state"), &AchievementManager::get_state);
    ClassDB::bind_method(D_METHOD("set_state", "state"), &AchievementManager::set_state);

    ClassDB::bind_method(D_METHOD("set_show_notifications", "enabled"), &AchievementManager::set_show_notifications);
    ClassDB::bind_method(D_METHOD("get_show_notifications"), &AchievementManager::get_show_notifications);
    ClassDB::bind_method(D_METHOD("set_notification_duration", "duration"), &AchievementManager::set_notification_duration);
    ClassDB::bind_method(D_METHOD("get_notification_duration"), &AchievementManager::get_notification_duration);
    ClassDB::bind_method(D_METHOD("set_notification_position", "position"), &AchievementManager::set_notification_position);
    ClassDB::bind_method(D_METHOD("get_notification_position"), &AchievementManager::get_notification_position);
    ClassDB::bind_method(D_METHOD("set_unlock_sound", "sound"), &AchievementManager::set_unlock_sound);
    ClassDB::bind_method(D_METHOD("get_unlock_sound"), &AchievementManager::get_unlock_sound);
    ClassDB::bind_method(D_METHOD("show_notification", "achievement"), &AchievementManager::show_notification);
    ClassDB::bind_method(D_METHOD("get_notification_container"), &AchievementManager::get_notification_container);

    BIND_ENUM_CONSTANT(NOTIFICATION_POSITION_TOP_LEFT);
    BIND_ENUM_CONSTANT(NOTIFICATION_POSITION_TOP_CENTER);
    BIND_ENUM_CONSTANT(NOTIFICATION_POSITION_TOP_RIGHT);
    BIND_ENUM_CONSTANT(NOTIFICATION_POSITION_BOTTOM_LEFT);
    BIND_ENUM_CONSTANT(NOTIFICATION_POSITION_BOTTOM_CENTER);
    BIND_ENUM_CONSTANT(NOTIFICATION_POSITION_BOTTOM_RIGHT);

    ADD_GROUP(U"🏆 Succès", "");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "achievements", PROPERTY_HINT_ARRAY_TYPE, "Achievement"), "set_achievements", "get_achievements");
    ADD_GROUP(U"💾 Sauvegarde", "");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "save_path", PROPERTY_HINT_SAVE_FILE, "*.cfg"), "set_save_path", "get_save_path");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "save_password", PROPERTY_HINT_PASSWORD), "set_save_password", "get_save_password");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_load"), "set_auto_load", "get_auto_load");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_save"), "set_auto_save", "get_auto_save");
    ADD_GROUP(U"🔔 Notifications", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_notifications"), "set_show_notifications", "get_show_notifications");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "notification_duration", PROPERTY_HINT_RANGE, "0,30,0.1,or_greater,suffix:s"), "set_notification_duration", "get_notification_duration");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "notification_position", PROPERTY_HINT_ENUM, "Top Left,Top Center,Top Right,Bottom Left,Bottom Center,Bottom Right"), "set_notification_position", "get_notification_position");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "unlock_sound", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"), "set_unlock_sound", "get_unlock_sound");

    ADD_SIGNAL(MethodInfo("achievement_unlocked", PropertyInfo(Variant::OBJECT, "achievement", PROPERTY_HINT_RESOURCE_TYPE, "Achievement")));
    ADD_SIGNAL(MethodInfo("achievement_progressed", PropertyInfo(Variant::OBJECT, "achievement", PROPERTY_HINT_RESOURCE_TYPE, "Achievement"), PropertyInfo(Variant::FLOAT, "progress")));
    ADD_SIGNAL(MethodInfo("event_notified", PropertyInfo(Variant::STRING_NAME, "event"), PropertyInfo(Variant::STRING, "id"), PropertyInfo(Variant::FLOAT, "amount")));
}
