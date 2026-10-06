#include "dialogue_box.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

DialogueBox::DialogueBox()
{
    set_custom_minimum_size(Vector2(0, 150));

    MarginContainer *margin = memnew(MarginContainer);
    const char *sides[] = {"margin_left", "margin_top", "margin_right", "margin_bottom"};
    for (const char *side : sides)
    {
        margin->add_theme_constant_override(side, 12);
    }
    add_child(margin, false, INTERNAL_MODE_FRONT);

    HBoxContainer *row = memnew(HBoxContainer);
    row->add_theme_constant_override("separation", 12);
    margin->add_child(row);

    portrait = memnew(TextureRect);
    portrait->set_custom_minimum_size(Vector2(96, 96));
    portrait->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    portrait->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    portrait->set_v_size_flags(SIZE_SHRINK_BEGIN);
    portrait->set_visible(false);
    row->add_child(portrait);

    VBoxContainer *column = memnew(VBoxContainer);
    column->set_h_size_flags(SIZE_EXPAND_FILL);
    row->add_child(column);

    speaker_label = memnew(Label);
    speaker_label->add_theme_font_size_override("font_size", 18);
    speaker_label->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.45f));
    column->add_child(speaker_label);

    text_label = memnew(RichTextLabel);
    text_label->set_use_bbcode(true);
    text_label->set_fit_content(true);
    text_label->set_scroll_active(false);
    text_label->set_v_size_flags(SIZE_EXPAND_FILL);
    column->add_child(text_label);

    choices_box = memnew(VBoxContainer);
    column->add_child(choices_box);

    continue_hint = memnew(Label);
    continue_hint->set_text(U"▼");
    continue_hint->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
    continue_hint->set_modulate(Color(1, 1, 1, 0.6f));
    continue_hint->set_visible(false);
    column->add_child(continue_hint);
}

// 🔌 Système relié

void DialogueBox::set_dialogue_system_path(const NodePath &p_path)
{
    dialogue_system_path = p_path;
    if (is_inside_tree())
    {
        set_dialogue_system(Object::cast_to<DialogueSystem>(get_node_or_null(dialogue_system_path)));
    }
}

DialogueSystem *DialogueBox::get_dialogue_system() const { return Object::cast_to<DialogueSystem>(ObjectDB::get_instance(system_id)); }

void DialogueBox::set_dialogue_system(DialogueSystem *p_system)
{
    if (DialogueSystem *previous = get_dialogue_system())
    {
        _connect_system(previous, false);
    }
    system_id = p_system ? ObjectID(p_system->get_instance_id()) : ObjectID();
    if (p_system)
    {
        _connect_system(p_system, true);
    }
}

void DialogueBox::_connect_system(DialogueSystem *p_system, bool p_connect)
{
    struct Connection
    {
        const char *signal;
        Callable callable;
    };
    Connection connections[] = {
        {"line_shown", callable_mp(this, &DialogueBox::_on_line_shown)},
        {"choices_shown", callable_mp(this, &DialogueBox::_on_choices_shown)},
        {"dialogue_ended", callable_mp(this, &DialogueBox::_on_dialogue_ended)},
    };
    for (const Connection &connection : connections)
    {
        bool connected = p_system->is_connected(connection.signal, connection.callable);
        if (p_connect && !connected)
        {
            p_system->connect(connection.signal, connection.callable);
        }
        else if (!p_connect && connected)
        {
            p_system->disconnect(connection.signal, connection.callable);
        }
    }
}

void DialogueBox::set_hide_when_idle(bool p_hide)
{
    hide_when_idle = p_hide;
    DialogueSystem *system = get_dialogue_system();
    if (hide_when_idle && is_inside_tree() && !Engine::get_singleton()->is_editor_hint() && (!system || !system->is_running()))
    {
        set_visible(false);
    }
}

// 🗨️ Affichage

void DialogueBox::_clear_choices()
{
    for (int i = choices_box->get_child_count() - 1; i >= 0; i--)
    {
        Node *child = choices_box->get_child(i);
        choices_box->remove_child(child);
        child->queue_free();
    }
}

void DialogueBox::_on_line_shown(const String &p_speaker, const String &p_text, const Ref<DialogueLine> &p_line)
{
    set_visible(true);
    _clear_choices();
    speaker_label->set_text(p_speaker);
    speaker_label->set_visible(!p_speaker.is_empty());
    Ref<Texture2D> texture = p_line.is_valid() ? p_line->get_portrait() : Ref<Texture2D>();
    portrait->set_texture(texture);
    portrait->set_visible(texture.is_valid());
    text_label->set_text(p_text);
    typing = characters_per_second > 0.0;
    typed_characters = 0.0;
    text_label->set_visible_characters(typing ? 0 : -1);
    continue_hint->set_visible(!typing);
}

void DialogueBox::_on_choices_shown(const String &p_prompt, const Array &p_options, const Ref<DialogueChoice> & /*p_choice*/)
{
    set_visible(true);
    skip_typing();
    if (!p_prompt.is_empty())
    {
        text_label->set_text(p_prompt);
        text_label->set_visible_characters(-1);
    }
    continue_hint->set_visible(false);
    _clear_choices();
    for (int i = 0; i < p_options.size(); i++)
    {
        Dictionary option = p_options[i];
        Button *button = memnew(Button);
        button->set_text(String(option["text"]));
        button->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
        button->connect("pressed", callable_mp(this, &DialogueBox::_on_choice_pressed).bind(int(option["index"])));
        choices_box->add_child(button);
        if (i == 0)
        {
            button->call_deferred("grab_focus");
        }
    }
}

void DialogueBox::_on_dialogue_ended(const Ref<DialogueResource> & /*p_dialogue*/)
{
    typing = false;
    _clear_choices();
    continue_hint->set_visible(false);
    if (hide_when_idle)
    {
        set_visible(false);
    }
}

void DialogueBox::_on_choice_pressed(int p_index)
{
    if (DialogueSystem *system = get_dialogue_system())
    {
        system->choose(p_index);
    }
}

void DialogueBox::skip_typing()
{
    if (!typing)
    {
        return;
    }
    typing = false;
    text_label->set_visible_characters(-1);
    continue_hint->set_visible(choices_box->get_child_count() == 0);
    emit_signal("typing_finished");
}

void DialogueBox::press_advance()
{
    if (typing)
    {
        skip_typing();
        return;
    }
    DialogueSystem *system = get_dialogue_system();
    if (system && system->get_wait_state() == DialogueSystem::WAIT_LINE)
    {
        system->advance();
    }
}

String DialogueBox::get_displayed_text() const
{
    String text = text_label->get_parsed_text();
    int visible = text_label->get_visible_characters();
    return visible < 0 ? text : text.substr(0, visible);
}

String DialogueBox::get_speaker_text() const { return speaker_label->get_text(); }

int DialogueBox::get_choice_count() const { return choices_box->get_child_count(); }

void DialogueBox::_unhandled_input(const Ref<InputEvent> &p_event)
{
    if (!is_visible_in_tree() || String(advance_action).is_empty() || !p_event->is_action_pressed(advance_action, false, true))
    {
        return;
    }
    if (choices_box->get_child_count() > 0 && !typing)
    {
        return; // les boutons de choix gèrent l'action eux-mêmes
    }
    press_advance();
    get_viewport()->set_input_as_handled();
}

void DialogueBox::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_READY:
        if (Engine::get_singleton()->is_editor_hint())
        {
            break;
        }
        if (!dialogue_system_path.is_empty() && !get_dialogue_system())
        {
            set_dialogue_system(Object::cast_to<DialogueSystem>(get_node_or_null(dialogue_system_path)));
        }
        if (!get_dialogue_system())
        {
            set_dialogue_system(DialogueSystem::get_main());
        }
        set_process(true);
        set_process_unhandled_input(true);
        if (hide_when_idle && (!get_dialogue_system() || !get_dialogue_system()->is_running()))
        {
            set_visible(false);
        }
        break;
    case NOTIFICATION_PROCESS:
        if (typing)
        {
            typed_characters += characters_per_second * get_process_delta_time();
            int total = text_label->get_total_character_count();
            text_label->set_visible_characters(int(typed_characters));
            if (int(typed_characters) >= total)
            {
                skip_typing();
            }
        }
        break;
    case NOTIFICATION_EXIT_TREE:
        if (DialogueSystem *system = get_dialogue_system())
        {
            _connect_system(system, false);
        }
        system_id = ObjectID();
        break;
    default:
        break;
    }
}

// 🔗 Liaisons

void DialogueBox::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_dialogue_system_path", "path"), &DialogueBox::set_dialogue_system_path);
    ClassDB::bind_method(D_METHOD("get_dialogue_system_path"), &DialogueBox::get_dialogue_system_path);
    ClassDB::bind_method(D_METHOD("set_dialogue_system", "system"), &DialogueBox::set_dialogue_system);
    ClassDB::bind_method(D_METHOD("get_dialogue_system"), &DialogueBox::get_dialogue_system);
    ClassDB::bind_method(D_METHOD("set_characters_per_second", "speed"), &DialogueBox::set_characters_per_second);
    ClassDB::bind_method(D_METHOD("get_characters_per_second"), &DialogueBox::get_characters_per_second);
    ClassDB::bind_method(D_METHOD("set_advance_action", "action"), &DialogueBox::set_advance_action);
    ClassDB::bind_method(D_METHOD("get_advance_action"), &DialogueBox::get_advance_action);
    ClassDB::bind_method(D_METHOD("set_hide_when_idle", "hide"), &DialogueBox::set_hide_when_idle);
    ClassDB::bind_method(D_METHOD("get_hide_when_idle"), &DialogueBox::get_hide_when_idle);
    ClassDB::bind_method(D_METHOD("is_typing"), &DialogueBox::is_typing);
    ClassDB::bind_method(D_METHOD("skip_typing"), &DialogueBox::skip_typing);
    ClassDB::bind_method(D_METHOD("press_advance"), &DialogueBox::press_advance);
    ClassDB::bind_method(D_METHOD("get_displayed_text"), &DialogueBox::get_displayed_text);
    ClassDB::bind_method(D_METHOD("get_speaker_text"), &DialogueBox::get_speaker_text);
    ClassDB::bind_method(D_METHOD("get_choice_count"), &DialogueBox::get_choice_count);

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "dialogue_system_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "DialogueSystem"), "set_dialogue_system_path", "get_dialogue_system_path");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "characters_per_second", PROPERTY_HINT_RANGE, "0,200,1,or_greater"), "set_characters_per_second", "get_characters_per_second");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "advance_action"), "set_advance_action", "get_advance_action");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hide_when_idle"), "set_hide_when_idle", "get_hide_when_idle");

    ADD_SIGNAL(MethodInfo("typing_finished"));
}
