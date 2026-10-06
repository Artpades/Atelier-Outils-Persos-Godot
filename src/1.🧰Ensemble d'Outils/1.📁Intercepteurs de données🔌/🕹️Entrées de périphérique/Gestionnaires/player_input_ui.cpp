#include "player_input_ui.hpp"
#include "input_rebinder.hpp"
#include "player_input.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

PlayerInput *PlayerInputUI::get_player() const { return Object::cast_to<PlayerInput>(get_node_or_null(player_input)); }

InputRebinder *PlayerInputUI::get_active_rebinder() const
{
    if (InputRebinder *rebinder = Object::cast_to<InputRebinder>(get_node_or_null(rebinder_path)))
    {
        return rebinder;
    }
    return own_rebinder;
}

void PlayerInputUI::_build_frame()
{
    tabs = memnew(TabContainer);
    tabs->set_v_size_flags(SIZE_EXPAND_FILL);
    tabs->set_custom_minimum_size(Vector2(420, 240));
    add_child(tabs);

    HBoxContainer *footer = memnew(HBoxContainer);
    add_child(footer);
    status = memnew(Label);
    status->set_h_size_flags(SIZE_EXPAND_FILL);
    status->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
    footer->add_child(status);
    Button *reset = memnew(Button);
    reset->set_text(U"↺ Réinitialiser");
    reset->connect("pressed", callable_mp(this, &PlayerInputUI::_on_reset_pressed));
    footer->add_child(reset);
    Button *save = memnew(Button);
    save->set_text(U"💾 Enregistrer");
    save->connect("pressed", callable_mp(this, &PlayerInputUI::_on_save_pressed));
    footer->add_child(save);

    if (!get_node_or_null(rebinder_path))
    {
        own_rebinder = memnew(InputRebinder);
        own_rebinder->set_name("Rebinder");
        add_child(own_rebinder);
    }
    InputRebinder *rebinder = get_active_rebinder();
    rebinder->connect("rebind_started", callable_mp(this, &PlayerInputUI::_on_rebind_started));
    rebinder->connect("rebind_completed", callable_mp(this, &PlayerInputUI::_on_rebind_completed));
    rebinder->connect("rebind_cancelled", callable_mp(this, &PlayerInputUI::_on_rebind_cancelled));
}

void PlayerInputUI::rebuild()
{
    if (!tabs)
    {
        return;
    }
    while (tabs->get_child_count() > 0)
    {
        Node *child = tabs->get_child(0);
        tabs->remove_child(child);
        child->queue_free();
    }
    slot_buttons.clear();
    PlayerInput *player = get_player();
    if (!player)
    {
        status->set_text(U"Aucun PlayerInput (propriété player_input).");
        return;
    }
    if (!player->is_setup())
    {
        player->setup();
    }
    struct Tab
    {
        bool shown;
        const char32_t *title;
        const char *group;
    };
    Tab tab_list[] = {{show_keyboard_tab, U"⌨️ Clavier / souris", "kbm"}, {show_gamepad_tab, U"🎮 Manette", "gamepad"}};
    for (const Tab &tab : tab_list)
    {
        if (!tab.shown)
        {
            continue;
        }
        ScrollContainer *scroll = memnew(ScrollContainer);
        scroll->set_name(String(tab.title));
        scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
        tabs->add_child(scroll);
        VBoxContainer *list = memnew(VBoxContainer);
        list->set_h_size_flags(SIZE_EXPAND_FILL);
        scroll->add_child(list);
        _fill_tab(list, tab.group);
    }
    refresh();
    status->set_text(U"Cliquez sur une entrée, puis appuyez sur la nouvelle touche (Échap : annuler).");
}

void PlayerInputUI::_fill_tab(VBoxContainer *p_list, const String &p_group)
{
    Array all = get_player()->get_all_actions();
    StringName last_context;
    for (int i = 0; i < all.size(); i++)
    {
        Array pair = all[i];
        Ref<InputContext> context = pair[0];
        Ref<InputAction> action = pair[1];
        if (!context_filter.is_empty() && !context_filter.has(context->get_context_name()))
        {
            continue;
        }
        TypedArray<Dictionary> slots = action->get_rebind_slots();
        Array own;
        for (int s = 0; s < slots.size(); s++)
        {
            if (String(Dictionary(slots[s])["group"]) == p_group)
            {
                own.push_back(slots[s]);
            }
        }
        if (own.is_empty())
        {
            continue;
        }
        if (context->get_context_name() != last_context)
        {
            last_context = context->get_context_name();
            Label *header = memnew(Label);
            header->set_text(String(U"🗂️ ") + String(last_context));
            header->add_theme_color_override("font_color", Color(0.65, 0.8, 1.0));
            p_list->add_child(header);
        }
        HBoxContainer *row = memnew(HBoxContainer);
        p_list->add_child(row);
        Label *name = memnew(Label);
        name->set_text(action->get_action_name());
        name->set_custom_minimum_size(Vector2(130, 0));
        name->set_tooltip_text(action->get_description());
        name->set_mouse_filter(MOUSE_FILTER_PASS);
        row->add_child(name);
        for (int s = 0; s < own.size(); s++)
        {
            Dictionary slot = own[s];
            Button *button = memnew(Button);
            button->set_h_size_flags(SIZE_EXPAND_FILL);
            button->set_clip_text(true);
            button->set_tooltip_text(slot["label"]);
            button->connect("pressed", callable_mp(this, &PlayerInputUI::_on_slot_pressed).bind(action, StringName(slot["slot"])));
            row->add_child(button);
            slot_buttons.push_back(Array::make(button, action, StringName(slot["slot"]), slot["label"]));
        }
    }
}

void PlayerInputUI::refresh()
{
    InputRebinder *rebinder = get_active_rebinder();
    for (int i = 0; i < slot_buttons.size(); i++)
    {
        Array entry = slot_buttons[i];
        Button *button = Object::cast_to<Button>(entry[0]);
        Ref<InputAction> action = entry[1];
        StringName slot = entry[2];
        if (!button)
        {
            continue;
        }
        bool waiting = rebinder && rebinder->is_listening() && rebinder->get_current_action() == action && rebinder->get_current_slot() == slot;
        button->set_text(waiting ? String(U"… appuyez") : String(entry[3]) + " : " + action->get_slot_text(slot));
    }
}

void PlayerInputUI::_on_slot_pressed(const Ref<InputAction> &p_action, const StringName &p_slot)
{
    if (InputRebinder *rebinder = get_active_rebinder())
    {
        rebinder->start_rebind(p_action, p_slot);
    }
}

void PlayerInputUI::_on_rebind_started(const Ref<InputAction> &p_action, const StringName & /*p_slot*/)
{
    refresh();
    status->set_text(String(U"« ") + String(p_action->get_action_name()) + U" » : appuyez sur la nouvelle entrée…");
}

void PlayerInputUI::_on_rebind_completed(const Ref<InputAction> &p_action, const StringName &p_slot)
{
    refresh();
    String text = String(U"✅ « ") + String(p_action->get_action_name()) + U" » → " + p_action->get_slot_text(p_slot);
    if (InputRebinder *rebinder = get_active_rebinder())
    {
        Array conflicts = rebinder->find_conflicts(get_player(), p_action, p_slot);
        if (!conflicts.is_empty())
        {
            Ref<InputAction> other = Array(conflicts[0])[0];
            text += String(U"  ⚠️ déjà utilisé par « ") + String(other->get_action_name()) + U" »";
        }
    }
    status->set_text(text);
}

void PlayerInputUI::_on_rebind_cancelled(const Ref<InputAction> & /*p_action*/, const StringName & /*p_slot*/)
{
    refresh();
    status->set_text(U"Réassignation annulée.");
}

Error PlayerInputUI::save_bindings()
{
    PlayerInput *player = get_player();
    if (!player)
    {
        return ERR_UNCONFIGURED;
    }
    Error error = player->save_bindings(save_path);
    if (status)
    {
        status->set_text(error == OK ? String(U"💾 Contrôles enregistrés.") : String(U"❌ Enregistrement impossible (") + save_path + ")");
    }
    if (error == OK)
    {
        emit_signal("bindings_saved", save_path);
    }
    return error;
}

void PlayerInputUI::reset_bindings()
{
    if (PlayerInput *player = get_player())
    {
        player->reset_bindings();
        refresh();
        if (status)
        {
            status->set_text(U"↺ Contrôles d'origine rétablis (« Enregistrer » pour les garder).");
        }
    }
}

void PlayerInputUI::_on_save_pressed() { save_bindings(); }

void PlayerInputUI::_on_reset_pressed() { reset_bindings(); }

void PlayerInputUI::_notification(int p_what)
{
    if (p_what != NOTIFICATION_READY || Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    _build_frame();
    PlayerInput *player = get_player();
    if (player && auto_load && FileAccess::file_exists(save_path))
    {
        if (!player->is_setup())
        {
            player->setup();
        }
        player->load_bindings(save_path);
    }
    rebuild();
}

void PlayerInputUI::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_player_input", "path"), &PlayerInputUI::set_player_input);
    ClassDB::bind_method(D_METHOD("get_player_input"), &PlayerInputUI::get_player_input);
    ClassDB::bind_method(D_METHOD("set_rebinder", "path"), &PlayerInputUI::set_rebinder);
    ClassDB::bind_method(D_METHOD("get_rebinder"), &PlayerInputUI::get_rebinder);
    ClassDB::bind_method(D_METHOD("set_save_path", "path"), &PlayerInputUI::set_save_path);
    ClassDB::bind_method(D_METHOD("get_save_path"), &PlayerInputUI::get_save_path);
    ClassDB::bind_method(D_METHOD("set_auto_load", "auto_load"), &PlayerInputUI::set_auto_load);
    ClassDB::bind_method(D_METHOD("get_auto_load"), &PlayerInputUI::get_auto_load);
    ClassDB::bind_method(D_METHOD("set_show_keyboard_tab", "show"), &PlayerInputUI::set_show_keyboard_tab);
    ClassDB::bind_method(D_METHOD("get_show_keyboard_tab"), &PlayerInputUI::get_show_keyboard_tab);
    ClassDB::bind_method(D_METHOD("set_show_gamepad_tab", "show"), &PlayerInputUI::set_show_gamepad_tab);
    ClassDB::bind_method(D_METHOD("get_show_gamepad_tab"), &PlayerInputUI::get_show_gamepad_tab);
    ClassDB::bind_method(D_METHOD("set_context_filter", "filter"), &PlayerInputUI::set_context_filter);
    ClassDB::bind_method(D_METHOD("get_context_filter"), &PlayerInputUI::get_context_filter);
    ClassDB::bind_method(D_METHOD("rebuild"), &PlayerInputUI::rebuild);
    ClassDB::bind_method(D_METHOD("refresh"), &PlayerInputUI::refresh);
    ClassDB::bind_method(D_METHOD("save_bindings"), &PlayerInputUI::save_bindings);
    ClassDB::bind_method(D_METHOD("reset_bindings"), &PlayerInputUI::reset_bindings);
    ClassDB::bind_method(D_METHOD("get_active_rebinder"), &PlayerInputUI::get_active_rebinder);

    ADD_SIGNAL(MethodInfo("bindings_saved", PropertyInfo(Variant::STRING, "path")));

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "player_input", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "PlayerInput"), "set_player_input", "get_player_input");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "rebinder", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "InputRebinder"), "set_rebinder", "get_rebinder");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "save_path", PROPERTY_HINT_SAVE_FILE, "*.cfg"), "set_save_path", "get_save_path");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_load"), "set_auto_load", "get_auto_load");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_keyboard_tab"), "set_show_keyboard_tab", "get_show_keyboard_tab");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_gamepad_tab"), "set_show_gamepad_tab", "get_show_gamepad_tab");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "context_filter"), "set_context_filter", "get_context_filter");
}
