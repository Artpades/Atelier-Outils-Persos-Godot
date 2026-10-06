#include "input_editor_window.hpp"
#include "input_editor_plugin.hpp"

#include "../Gestionnaires/player_input.hpp"

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/check_button.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_flow_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
EditorUndoRedoManager *undo_redo() { return EditorInterface::get_singleton()->get_editor_undo_redo(); }

const char *const ACTION_TYPES[] = {"InputButtonAction", "InputAxisAction", "InputCircleAction", "InputSphereAction", "InputMouseAction", "InputGyroAction"};
const char32_t *const ACTION_LABELS[] = {U"🔘 Bouton", U"↔️ Axe", U"🕹️ Cercle (Vector2)", U"🌐 Sphère (Vector3)", U"🖱️ Souris", U"🧭 Gyroscope"};

String action_emoji(const Ref<InputAction> &p_action)
{
    if (p_action.is_null())
    {
        return U"❔";
    }
    for (int i = 0; i < 6; i++)
    {
        if (p_action->is_class(ACTION_TYPES[i]))
        {
            return String(ACTION_LABELS[i]).get_slice(" ", 0);
        }
    }
    return U"🧩";
}

/// Valeurs des propriétés éditables d'un objet (détection des modifications faites ailleurs).
String object_signature(Object *p_object)
{
    if (!p_object)
    {
        return String();
    }
    String signature = p_object->get_class();
    TypedArray<Dictionary> properties = p_object->get_property_list();
    for (int i = 0; i < properties.size(); i++)
    {
        Dictionary property = properties[i];
        int usage = property["usage"];
        int type = property["type"];
        if (!(usage & PROPERTY_USAGE_STORAGE) || type == Variant::OBJECT || type == Variant::ARRAY)
        {
            continue;
        }
        signature += "|" + p_object->get(property["name"]).stringify();
    }
    return signature;
}

struct EnumItem
{
    String label;
    int64_t value;
};

Vector<EnumItem> parse_enum_hint(const String &p_hint)
{
    Vector<EnumItem> items;
    PackedStringArray parts = p_hint.split(",");
    int64_t next = 0;
    for (const String &part : parts)
    {
        int colon = part.rfind(":");
        EnumItem item;
        if (colon >= 0 && part.substr(colon + 1).is_valid_int())
        {
            item.label = part.substr(0, colon);
            item.value = part.substr(colon + 1).to_int();
        }
        else
        {
            item.label = part;
            item.value = next;
        }
        next = item.value + 1;
        items.push_back(item);
    }
    return items;
}

Control *spacer()
{
    Control *control = memnew(Control);
    control->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    return control;
}

Label *make_label(const String &p_text, float p_alpha = 1.0f)
{
    Label *label = memnew(Label);
    label->set_text(p_text);
    if (p_alpha < 1.0f)
    {
        label->set_modulate(Color(1, 1, 1, p_alpha));
    }
    return label;
}

void clear_children(Node *p_node)
{
    while (p_node->get_child_count() > 0)
    {
        Node *child = p_node->get_child(0);
        p_node->remove_child(child);
        child->queue_free();
    }
}
} // namespace

// --- InputActionMeter ---

InputActionMeter::InputActionMeter()
{
    set_custom_minimum_size(Vector2(64, 64));
    set_mouse_filter(MOUSE_FILTER_STOP);
    set_tooltip_text(U"Glisser pour simuler une valeur, relâcher pour rendre la main aux périphériques");
}

void InputActionMeter::set_value(const Variant &p_value)
{
    if (p_value != value)
    {
        value = p_value;
        queue_redraw();
    }
}

void InputActionMeter::_draw()
{
    Vector2 size = get_size();
    Vector2 center = size / 2.0f;
    float radius = MIN(size.x, size.y) / 2.0f - 3.0f;
    Color frame(0.55f, 0.6f, 0.7f, 0.8f);
    Color accent(0.35f, 0.85f, 0.5f);
    draw_rect(Rect2(Vector2(), size), Color(0, 0, 0, 0.25f));
    switch (value.get_type())
    {
    case Variant::BOOL:
        draw_circle(center, radius * 0.6f, bool(value) ? accent : Color(0.25f, 0.27f, 0.3f));
        draw_arc(center, radius * 0.6f, 0, Math_TAU, 32, frame, 1.5f);
        break;
    case Variant::FLOAT:
    {
        double v = CLAMP(double(value), -1.0, 1.0);
        Rect2 bar(Vector2(4, center.y - 7), Vector2(size.x - 8, 14));
        draw_rect(bar, Color(0.2f, 0.22f, 0.25f));
        float half = bar.size.x / 2.0f;
        float x0 = bar.position.x + half;
        draw_rect(Rect2(Vector2(MIN(x0, x0 + float(v) * half), bar.position.y), Vector2(Math::abs(float(v)) * half, bar.size.y)), accent);
        draw_line(Vector2(x0, bar.position.y - 3), Vector2(x0, bar.position.y + bar.size.y + 3), frame, 1.0f);
        break;
    }
    case Variant::VECTOR2:
    case Variant::VECTOR3:
    {
        Vector2 point;
        float height = 0.0f;
        if (value.get_type() == Variant::VECTOR2)
        {
            point = value;
        }
        else
        {
            Vector3 v = value;
            point = Vector2(v.x, v.z);
            height = v.y;
        }
        draw_arc(center, radius, 0, Math_TAU, 48, frame, 1.5f);
        draw_line(center - Vector2(radius, 0), center + Vector2(radius, 0), Color(frame, 0.3f));
        draw_line(center - Vector2(0, radius), center + Vector2(0, radius), Color(frame, 0.3f));
        Vector2 dot = center + point.limit_length(1.0f) * radius;
        draw_line(center, dot, Color(accent, 0.6f), 2.0f);
        draw_circle(dot, 4.0f, accent);
        if (value.get_type() == Variant::VECTOR3)
        {
            Rect2 bar(Vector2(size.x - 6, 4), Vector2(4, size.y - 8));
            draw_rect(bar, Color(0.2f, 0.22f, 0.25f));
            float half = bar.size.y / 2.0f;
            float y0 = bar.position.y + half;
            float h = CLAMP(height, -1.0f, 1.0f) * half;
            draw_rect(Rect2(Vector2(bar.position.x, MIN(y0, y0 - h)), Vector2(bar.size.x, Math::abs(h))), Color(0.95f, 0.75f, 0.3f));
        }
        break;
    }
    default:
    {
        Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
        draw_string(font, Vector2(4, center.y + 4), value.get_type() == Variant::NIL ? String(U"—") : value.stringify(), HORIZONTAL_ALIGNMENT_CENTER, size.x - 8, 11);
        break;
    }
    }
    if (dragging)
    {
        draw_rect(Rect2(Vector2(), size), Color(1.0f, 0.75f, 0.3f), false, 2.0f);
    }
}

Variant InputActionMeter::_value_at(const Vector2 &p_position) const
{
    Vector2 size = get_size();
    Vector2 center = size / 2.0f;
    float radius = MAX(1.0f, MIN(size.x, size.y) / 2.0f - 3.0f);
    switch (value.get_type())
    {
    case Variant::BOOL:
        return true;
    case Variant::FLOAT:
        return CLAMP((p_position.x - center.x) / MAX(1.0f, size.x / 2.0f - 4.0f), -1.0f, 1.0f);
    case Variant::VECTOR2:
        return ((p_position - center) / radius).limit_length(1.0f);
    case Variant::VECTOR3:
    {
        Vector2 flat = ((p_position - center) / radius).limit_length(1.0f);
        return Vector3(flat.x, 0.0f, flat.y);
    }
    default:
        return Variant();
    }
}

void InputActionMeter::_gui_input(const Ref<InputEvent> &p_event)
{
    if (!interactive)
    {
        return;
    }
    Ref<InputEventMouseButton> button = p_event;
    if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT)
    {
        dragging = button->is_pressed();
        if (dragging)
        {
            Variant simulated = _value_at(button->get_position());
            if (simulated.get_type() != Variant::NIL)
            {
                emit_signal("simulate_requested", simulated);
            }
        }
        else
        {
            emit_signal("simulation_released");
        }
        queue_redraw();
        accept_event();
        return;
    }
    Ref<InputEventMouseMotion> motion = p_event;
    if (motion.is_valid() && dragging)
    {
        Variant simulated = _value_at(motion->get_position());
        if (simulated.get_type() != Variant::NIL)
        {
            emit_signal("simulate_requested", simulated);
        }
        accept_event();
    }
}

void InputActionMeter::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("simulate_requested", PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)));
    ADD_SIGNAL(MethodInfo("simulation_released"));
}

// --- InputEditorWindow ---

ObjectID InputEditorWindow::current_window_id;

void InputEditorWindow::open_for(Object *p_object)
{
    EditorInterface *editor = EditorInterface::get_singleton();
    if (!editor || !editor->get_base_control() || !p_object)
    {
        return;
    }
    if (InputEditorWindow *window = get_current())
    {
        window->_set_target(p_object);
        window->grab_focus();
        return;
    }
    InputEditorWindow *window = memnew(InputEditorWindow);
    window->set_size(Vector2i(1180, 760));
    window->set_min_size(Vector2i(760, 460));
    window->set_theme(editor->get_editor_theme());
    window->set_visible(false);
    window->connect("close_requested", callable_mp(window, &InputEditorWindow::_on_close_requested));
    window->connect("window_input", callable_mp(window, &InputEditorWindow::_on_window_input));
    window->_build_ui();
    editor->get_base_control()->add_child(window);
    current_window_id = ObjectID(window->get_instance_id());
    window->_set_target(p_object);
    window->set_process(true);
    window->popup_centered();
}

void InputEditorWindow::close_current()
{
    if (InputEditorWindow *window = get_current())
    {
        window->_stop_local_test();
        window->queue_free();
    }
    current_window_id = ObjectID();
}

InputEditorWindow *InputEditorWindow::get_current() { return Object::cast_to<InputEditorWindow>(ObjectDB::get_instance(current_window_id)); }

InputEditorWindow::~InputEditorWindow()
{
    if (test_player)
    {
        memdelete(test_player);
        test_player = nullptr;
    }
}

void InputEditorWindow::_on_close_requested()
{
    _stop_local_test();
    if (InputDebuggerPlugin *debugger = InputDebuggerPlugin::get_singleton())
    {
        debugger->set_watched(PackedStringArray());
    }
    if (current_window_id == ObjectID(get_instance_id()))
    {
        current_window_id = ObjectID();
    }
    queue_free();
}

// 🧱 Interface

void InputEditorWindow::_build_ui()
{
    Panel *background = memnew(Panel);
    background->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    add_child(background);
    MarginContainer *margin = memnew(MarginContainer);
    margin->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    for (const char *side : {"margin_left", "margin_top", "margin_right", "margin_bottom"})
    {
        margin->add_theme_constant_override(side, 8);
    }
    add_child(margin);
    VBoxContainer *box = memnew(VBoxContainer);
    margin->add_child(box);

    HBoxContainer *toolbar = memnew(HBoxContainer);
    box->add_child(toolbar);
    title_label = memnew(Label);
    title_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    title_label->set_clip_text(true);
    toolbar->add_child(title_label);
    Button *save = memnew(Button);
    save->set_text(U"💾 Enregistrer les ressources");
    save->set_tooltip_text(U"Enregistre les contextes et actions qui ont leur propre fichier (.tres) ; ceux intégrés à la scène s'enregistrent avec la scène");
    save->connect("pressed", callable_mp(this, &InputEditorWindow::_on_save_pressed));
    toolbar->add_child(save);

    HSplitContainer *split = memnew(HSplitContainer);
    split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    split->set_split_offset(300);
    box->add_child(split);

    // 🌳 Contextes et actions
    VBoxContainer *left = memnew(VBoxContainer);
    left->set_custom_minimum_size(Vector2(260, 0));
    split->add_child(left);
    HBoxContainer *tree_toolbar = memnew(HBoxContainer);
    left->add_child(tree_toolbar);
    add_context_button = memnew(Button);
    add_context_button->set_text(U"➕ Contexte");
    add_context_button->connect("pressed", callable_mp(this, &InputEditorWindow::_on_add_context_pressed));
    tree_toolbar->add_child(add_context_button);
    add_action_button = memnew(MenuButton);
    add_action_button->set_text(U"➕ Action ▾");
    add_action_button->set_flat(false);
    for (int i = 0; i < 6; i++)
    {
        add_action_button->get_popup()->add_item(ACTION_LABELS[i], i);
    }
    add_action_button->get_popup()->connect("id_pressed", callable_mp(this, &InputEditorWindow::_on_add_action_type));
    tree_toolbar->add_child(add_action_button);
    tree = memnew(Tree);
    tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    tree->set_hide_root(true);
    tree->connect("item_selected", callable_mp(this, &InputEditorWindow::_on_tree_selected));
    left->add_child(tree);
    HBoxContainer *tree_actions = memnew(HBoxContainer);
    left->add_child(tree_actions);
    struct ToolButton
    {
        const char32_t *text;
        const char32_t *tooltip;
        Callable callable;
    };
    ToolButton buttons[] = {
        {U"⧉", U"Dupliquer", callable_mp(this, &InputEditorWindow::_on_duplicate_pressed)},
        {U"🗑", U"Supprimer", callable_mp(this, &InputEditorWindow::_on_delete_pressed)},
        {U"▲", U"Monter", callable_mp(this, &InputEditorWindow::_on_move_pressed).bind(-1)},
        {U"▼", U"Descendre", callable_mp(this, &InputEditorWindow::_on_move_pressed).bind(1)},
    };
    for (const ToolButton &tool : buttons)
    {
        Button *button = memnew(Button);
        button->set_text(tool.text);
        button->set_tooltip_text(tool.tooltip);
        button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        button->connect("pressed", tool.callable);
        tree_actions->add_child(button);
    }

    // 📑 Onglets
    tabs = memnew(TabContainer);
    tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    split->add_child(tabs);
    ScrollContainer *properties_scroll = memnew(ScrollContainer);
    properties_scroll->set_name(U"⚙️ Propriétés");
    properties_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    tabs->add_child(properties_scroll);
    properties_box = memnew(VBoxContainer);
    properties_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    properties_scroll->add_child(properties_box);

    ScrollContainer *bindings_scroll = memnew(ScrollContainer);
    bindings_scroll->set_name(U"🔗 Liaisons");
    bindings_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    tabs->add_child(bindings_scroll);
    bindings_box = memnew(VBoxContainer);
    bindings_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    bindings_scroll->add_child(bindings_box);

    VBoxContainer *debug_tab = memnew(VBoxContainer);
    debug_tab->set_name(U"🐞 Débogage");
    tabs->add_child(debug_tab);
    HBoxContainer *debug_toolbar = memnew(HBoxContainer);
    debug_tab->add_child(debug_toolbar);
    debug_toolbar->add_child(make_label(U"Source :"));
    source_option = memnew(OptionButton);
    source_option->set_custom_minimum_size(Vector2(280, 0));
    source_option->set_clip_text(true);
    source_option->connect("item_selected", callable_mp(this, &InputEditorWindow::_on_source_selected));
    debug_toolbar->add_child(source_option);
    debug_toolbar->add_child(spacer());
    Button *clear_log = memnew(Button);
    clear_log->set_text(U"🧹 Vider le journal");
    clear_log->connect("pressed", callable_mp(this, &InputEditorWindow::_on_clear_log_pressed));
    debug_toolbar->add_child(clear_log);
    debug_status = make_label(String(), 0.75f);
    debug_status->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    debug_tab->add_child(debug_status);
    ScrollContainer *debug_scroll = memnew(ScrollContainer);
    debug_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    debug_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    debug_tab->add_child(debug_scroll);
    debug_box = memnew(VBoxContainer);
    debug_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    debug_scroll->add_child(debug_box);
    debug_tab->add_child(make_label(U"📜 Évènements (les plus récents en haut) et appels de liaisons", 0.75f));
    log_label = memnew(RichTextLabel);
    log_label->set_use_bbcode(true);
    log_label->set_custom_minimum_size(Vector2(0, 170));
    log_label->set_selection_enabled(true);
    debug_tab->add_child(log_label);

    status_label = make_label(String(), 0.7f);
    status_label->set_clip_text(true);
    box->add_child(status_label);

    _update_source_list();
}

void InputEditorWindow::_set_status(const String &p_text) { status_label->set_text(p_text); }

void InputEditorWindow::_set_target(Object *p_object)
{
    _stop_local_test();
    capture_action.unref();
    player_id = ObjectID();
    lone_context.unref();
    if (PlayerInput *player = Object::cast_to<PlayerInput>(p_object))
    {
        player_id = ObjectID(player->get_instance_id());
        title_label->set_text(String(U"🎮 PlayerInput : ") + String(player->get_name()));
        set_title(String(U"Éditeur d'entrées — ") + String(player->get_name()));
    }
    else if (InputContext *context = Object::cast_to<InputContext>(p_object))
    {
        lone_context = Ref<InputContext>(context);
        String name = context->get_path().is_empty() ? String(context->get_context_name()) : context->get_path().get_file();
        title_label->set_text(String(U"🗂️ Contexte seul : ") + name + U" (liaisons : chemins relatifs au PlayerInput qui l'utilisera)");
        set_title(String(U"Éditeur d'entrées — ") + name);
    }
    add_context_button->set_disabled(lone_context.is_valid());
    selected_context.unref();
    selected_action.unref();
    TypedArray<InputContext> contexts = _contexts();
    if (!contexts.is_empty())
    {
        selected_context = contexts[0];
    }
    source = SOURCE_NONE;
    _update_source_list();
    _refresh_all();
}

PlayerInput *InputEditorWindow::_player() const { return Object::cast_to<PlayerInput>(ObjectDB::get_instance(player_id)); }

TypedArray<InputContext> InputEditorWindow::_contexts() const
{
    if (PlayerInput *player = _player())
    {
        return player->get_contexts();
    }
    TypedArray<InputContext> result;
    if (lone_context.is_valid())
    {
        result.push_back(lone_context);
    }
    return result;
}

String InputEditorWindow::_compute_signature() const
{
    String signature;
    TypedArray<InputContext> contexts = _contexts();
    for (int i = 0; i < contexts.size(); i++)
    {
        Ref<InputContext> context = contexts[i];
        if (context.is_null())
        {
            signature += "#null";
            continue;
        }
        signature += "#" + object_signature(context.ptr());
        TypedArray<InputAction> actions = context->get_actions();
        for (int a = 0; a < actions.size(); a++)
        {
            Ref<InputAction> action = actions[a];
            if (action.is_null())
            {
                signature += "@null";
                continue;
            }
            signature += "@" + object_signature(action.ptr());
            TypedArray<InputBinding> bindings = action->get_bindings();
            for (int b = 0; b < bindings.size(); b++)
            {
                Ref<InputBinding> binding = bindings[b];
                signature += "~" + (binding.is_valid() ? object_signature(binding.ptr()) : String("null"));
            }
        }
    }
    return signature;
}

void InputEditorWindow::_refresh_all()
{
    // Ne pas détruire un champ de texte en cours de saisie : nouvelle tentative au prochain sondage.
    Control *focus = gui_get_focus_owner();
    if (focus && Object::cast_to<LineEdit>(focus) && (properties_box->is_ancestor_of(focus) || bindings_box->is_ancestor_of(focus)))
    {
        refresh_pending = true;
        return;
    }
    refresh_pending = false;
    // La sélection doit encore appartenir au modèle.
    TypedArray<InputContext> contexts = _contexts();
    if (selected_context.is_valid() && !contexts.has(selected_context))
    {
        selected_context.unref();
        selected_action.unref();
    }
    if (selected_action.is_valid() && (selected_context.is_null() || !selected_context->get_actions().has(selected_action)))
    {
        selected_action.unref();
    }
    _rebuild_tree();
    _rebuild_properties();
    _rebuild_bindings();
    model_signature = _compute_signature();
}

void InputEditorWindow::_rebuild_tree()
{
    updating_tree = true;
    tree->clear();
    TreeItem *root = tree->create_item();
    TypedArray<InputContext> contexts = _contexts();
    if (contexts.is_empty())
    {
        TreeItem *empty = tree->create_item(root);
        empty->set_text(0, lone_context.is_null() && !_player() ? String(U"(cible disparue)") : String(U"Aucun contexte : ➕ Contexte"));
        empty->set_selectable(0, false);
    }
    for (int i = 0; i < contexts.size(); i++)
    {
        Ref<InputContext> context = contexts[i];
        if (context.is_null())
        {
            continue;
        }
        TreeItem *context_item = tree->create_item(root);
        String text = String(U"🗂️ ") + String(context->get_context_name()) + U"  ·  priorité " + String::num_int64(context->get_priority());
        if (context->is_exclusive())
        {
            text += U"  🔒";
        }
        if (!context->is_enabled())
        {
            text += U"  ⛔";
        }
        context_item->set_text(0, text);
        context_item->set_metadata(0, Array::make(context, Variant()));
        context_item->set_tooltip_text(0, context->get_description());
        if (context == selected_context && selected_action.is_null())
        {
            context_item->select(0);
        }
        TypedArray<InputAction> actions = context->get_actions();
        for (int a = 0; a < actions.size(); a++)
        {
            Ref<InputAction> action = actions[a];
            if (action.is_null())
            {
                continue;
            }
            TreeItem *action_item = tree->create_item(context_item);
            String action_text = action_emoji(action) + " " + String(action->get_action_name());
            int binding_count = action->get_bindings().size();
            if (binding_count > 0)
            {
                action_text += U"  🔗" + String::num_int64(binding_count);
            }
            if (!action->is_enabled())
            {
                action_text += U"  ⛔";
            }
            action_item->set_text(0, action_text);
            action_item->set_tooltip_text(0, action->get_class() + (action->get_description().is_empty() ? String() : "\n" + action->get_description()));
            action_item->set_metadata(0, Array::make(context, action));
            if (action == selected_action)
            {
                action_item->select(0);
            }
        }
    }
    updating_tree = false;
}

void InputEditorWindow::_on_tree_selected()
{
    if (updating_tree)
    {
        return;
    }
    TreeItem *item = tree->get_selected();
    if (!item)
    {
        return;
    }
    Array data = item->get_metadata(0);
    if (data.size() < 2)
    {
        return;
    }
    selected_context = data[0];
    selected_action = data[1];
    capture_action.unref();
    _rebuild_properties();
    _rebuild_bindings();
    if (selected_action.is_valid())
    {
        EditorInterface::get_singleton()->inspect_object(selected_action.ptr(), String(), true);
    }
}

void InputEditorWindow::_select(const Ref<InputContext> &p_context, const Ref<InputAction> &p_action)
{
    selected_context = p_context;
    selected_action = p_action;
    _refresh_all();
}

// ⚙️ Propriétés

void InputEditorWindow::_rebuild_properties()
{
    clear_children(properties_box);
    Object *object = selected_action.is_valid() ? (Object *)selected_action.ptr() : (Object *)selected_context.ptr();
    if (!object)
    {
        properties_box->add_child(make_label(U"Choisissez un contexte ou une action dans la liste.", 0.7f));
        return;
    }
    HBoxContainer *header = memnew(HBoxContainer);
    properties_box->add_child(header);
    String title = selected_action.is_valid() ? action_emoji(selected_action) + " " + selected_action->get_class() : String(U"🗂️ InputContext");
    Label *title_label_widget = make_label(title);
    title_label_widget->add_theme_font_size_override("font_size", 16);
    header->add_child(title_label_widget);
    header->add_child(spacer());
    Button *inspect = memnew(Button);
    inspect->set_text(U"🔍 Inspecteur");
    inspect->set_tooltip_text(U"Ouvrir dans l'inspecteur de l'éditeur (toutes les propriétés, enregistrement en fichier...)");
    inspect->connect("pressed", callable_mp(this, &InputEditorWindow::_on_inspect_pressed).bind((uint64_t)object->get_instance_id()));
    header->add_child(inspect);
    Resource *resource = Object::cast_to<Resource>(object);
    String path = resource ? resource->get_path() : String();
    properties_box->add_child(make_label(path.is_empty() || path.contains("::") ? String(U"Ressource intégrée à la scène") : String(U"Fichier : ") + path, 0.6f));

    PackedStringArray skip;
    skip.push_back("actions");
    skip.push_back("bindings");
    Array slots = selected_action.is_valid() ? Array(selected_action->get_rebind_slots()) : Array();
    _add_property_editors(properties_box, object, skip, slots);

    if (selected_action.is_valid())
    {
        properties_box->add_child(memnew(HSeparator));
        PackedStringArray names;
        TypedArray<Dictionary> events = selected_action->get_events();
        for (int i = 0; i < events.size(); i++)
        {
            Dictionary event = events[i];
            names.push_back(String(event["name"]) + "(" + String(", ").join(PackedStringArray(event["args"])) + ")");
        }
        Label *events_label = make_label(String(U"📡 Évènements (pour les liaisons) : ") + String(", ").join(names), 0.75f);
        events_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
        properties_box->add_child(events_label);
    }
    else if (selected_context.is_valid())
    {
        Label *help = make_label(U"Les actions d'un contexte inactif sont relâchées. Un contexte « exclusive » bloque, tant qu'il est actif, les contextes de priorité inférieure (menu par-dessus le jeu). PlayerInput.push_context() le fait temporairement.", 0.65f);
        help->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
        properties_box->add_child(help);
    }
}

LineEdit *InputEditorWindow::_add_line_edit(Control *p_parent, Object *p_object, const StringName &p_property, int p_kind, const String &p_placeholder)
{
    LineEdit *line = memnew(LineEdit);
    line->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    line->set_text(p_object->get(p_property).stringify());
    line->set_placeholder(p_placeholder);
    line->set_select_all_on_focus(true);
    uint64_t object_id = p_object->get_instance_id();
    line->connect("text_submitted", callable_mp(this, &InputEditorWindow::_on_text_submitted).bind(object_id, p_property, p_kind));
    line->connect("focus_exited", callable_mp(this, &InputEditorWindow::_on_text_focus_exited).bind((uint64_t)line->get_instance_id(), object_id, p_property, p_kind));
    p_parent->add_child(line);
    return line;
}

void InputEditorWindow::_add_property_editors(VBoxContainer *p_box, Object *p_object, const PackedStringArray &p_skip, const Array &p_slots)
{
    PackedStringArray slot_names;
    for (int i = 0; i < p_slots.size(); i++)
    {
        slot_names.push_back(Dictionary(p_slots[i])["slot"]);
    }
    uint64_t object_id = p_object->get_instance_id();
    TypedArray<Dictionary> properties = p_object->get_property_list();
    for (int i = 0; i < properties.size(); i++)
    {
        Dictionary property = properties[i];
        int usage = property["usage"];
        String name = property["name"];
        if (usage & PROPERTY_USAGE_GROUP)
        {
            if (!name.is_empty())
            {
                Label *group = make_label(name);
                group->add_theme_color_override("font_color", Color(0.6f, 0.78f, 1.0f));
                p_box->add_child(group);
            }
            continue;
        }
        if ((usage & (PROPERTY_USAGE_CATEGORY | PROPERTY_USAGE_SUBGROUP)) || !(usage & PROPERTY_USAGE_EDITOR) || p_skip.has(name) || name.begins_with("resource_") || name == "script" || name.begins_with("metadata/"))
        {
            continue;
        }
        int type = property["type"];
        int hint = property["hint"];
        String hint_string = property["hint_string"];
        Variant value = p_object->get(name);
        HBoxContainer *row = memnew(HBoxContainer);
        p_box->add_child(row);
        Label *label = make_label(name);
        label->set_custom_minimum_size(Vector2(170, 0));
        label->set_clip_text(true);
        label->set_tooltip_text(name);
        label->set_mouse_filter(Control::MOUSE_FILTER_PASS);
        row->add_child(label);
        switch (type)
        {
        case Variant::BOOL:
        {
            CheckBox *check = memnew(CheckBox);
            check->set_pressed_no_signal(value);
            check->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            check->connect("toggled", callable_mp(this, &InputEditorWindow::_on_bool_edited).bind(object_id, StringName(name)));
            row->add_child(check);
            break;
        }
        case Variant::INT:
        case Variant::FLOAT:
        {
            if (type == Variant::INT && hint == PROPERTY_HINT_ENUM)
            {
                OptionButton *option = memnew(OptionButton);
                option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
                option->set_clip_text(true);
                Vector<EnumItem> items = parse_enum_hint(hint_string);
                int64_t current = value;
                bool found = false;
                for (const EnumItem &item : items)
                {
                    option->add_item(item.label, int(item.value));
                    if (item.value == current)
                    {
                        option->select(option->get_item_count() - 1);
                        found = true;
                    }
                }
                if (!found)
                {
                    option->add_item(String::num_int64(current), int(current));
                    option->select(option->get_item_count() - 1);
                }
                option->connect("item_selected", callable_mp(this, &InputEditorWindow::_on_option_edited).bind((uint64_t)option->get_instance_id(), object_id, StringName(name)));
                row->add_child(option);
                break;
            }
            SpinBox *spin = memnew(SpinBox);
            spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            double minimum = -100000.0, maximum = 100000.0, step = type == Variant::INT ? 1.0 : 0.001;
            if (hint == PROPERTY_HINT_RANGE)
            {
                PackedStringArray parts = hint_string.split(",");
                if (parts.size() >= 2)
                {
                    minimum = parts[0].to_float();
                    maximum = parts[1].to_float();
                }
                if (parts.size() >= 3 && parts[2].is_valid_float())
                {
                    step = parts[2].to_float();
                }
                spin->set_allow_greater(hint_string.contains("or_greater"));
                spin->set_allow_lesser(hint_string.contains("or_less"));
                for (const String &part : parts)
                {
                    if (part.begins_with("suffix:"))
                    {
                        spin->set_suffix(part.trim_prefix("suffix:"));
                    }
                }
            }
            else
            {
                spin->set_allow_greater(true);
                spin->set_allow_lesser(true);
            }
            spin->set_min(minimum);
            spin->set_max(maximum);
            spin->set_step(step);
            spin->set_value_no_signal(value);
            spin->connect("value_changed", callable_mp(this, &InputEditorWindow::_on_number_edited).bind(object_id, StringName(name), type == Variant::INT));
            row->add_child(spin);
            break;
        }
        case Variant::STRING:
        case Variant::STRING_NAME:
            _add_line_edit(row, p_object, name, type == Variant::STRING_NAME ? 1 : 0);
            break;
        case Variant::NODE_PATH:
            _add_line_edit(row, p_object, name, 2);
            break;
        default:
        {
            Label *other = make_label(value.stringify(), 0.6f);
            other->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            other->set_clip_text(true);
            other->set_tooltip_text(U"Modifiable dans l'inspecteur (🔍)");
            other->set_mouse_filter(Control::MOUSE_FILTER_PASS);
            row->add_child(other);
            break;
        }
        }
        if (slot_names.has(name))
        {
            bool capturing = capture_action == selected_action && capture_slot == StringName(name);
            Button *capture = memnew(Button);
            capture->set_text(capturing ? String(U"⏺ …") : String(U"🎯"));
            capture->set_focus_mode(Control::FOCUS_NONE);
            capture->set_tooltip_text(U"Capturer : appuyez ensuite sur la touche, le bouton ou l'axe voulu (Échap : annuler)");
            capture->connect("pressed", callable_mp(this, &InputEditorWindow::_on_capture_pressed).bind(StringName(name)));
            row->add_child(capture);
            Button *clear = memnew(Button);
            clear->set_text(U"✖");
            clear->set_focus_mode(Control::FOCUS_NONE);
            clear->set_tooltip_text(U"Effacer cette entrée");
            clear->connect("pressed", callable_mp(this, &InputEditorWindow::_on_clear_slot_pressed).bind(StringName(name)));
            row->add_child(clear);
        }
    }
}

void InputEditorWindow::_commit_property(Object *p_object, const StringName &p_property, const Variant &p_value, bool p_merge)
{
    if (!p_object || p_object->get(p_property) == p_value)
    {
        return;
    }
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(String(U"Entrées : ") + String(p_property), p_merge ? UndoRedo::MERGE_ENDS : UndoRedo::MERGE_DISABLE, _player());
    ur->add_do_property(p_object, p_property, p_value);
    ur->add_undo_property(p_object, p_property, p_object->get(p_property));
    ur->commit_action();
    // L'arbre suit (noms, états) ; les champs restent en place (pas de reconstruction pendant la saisie).
    _rebuild_tree();
    model_signature = _compute_signature();
    if (_player())
    {
        _player()->update_configuration_warnings();
    }
}

void InputEditorWindow::_on_bool_edited(bool p_value, uint64_t p_object_id, const StringName &p_property) { _commit_property(ObjectDB::get_instance(ObjectID(p_object_id)), p_property, p_value); }

void InputEditorWindow::_on_number_edited(double p_value, uint64_t p_object_id, const StringName &p_property, bool p_integer)
{
    Variant value = p_integer ? Variant(int64_t(Math::round(p_value))) : Variant(p_value);
    _commit_property(ObjectDB::get_instance(ObjectID(p_object_id)), p_property, value, true);
}

void InputEditorWindow::_on_option_edited(int p_index, uint64_t p_option_id, uint64_t p_object_id, const StringName &p_property)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(ObjectID(p_option_id)));
    if (!option)
    {
        return;
    }
    Object *object = ObjectDB::get_instance(ObjectID(p_object_id));
    _commit_property(object, p_property, int64_t(option->get_item_id(p_index)));
    // Les préréglages (key_preset...) modifient d'autres propriétés : champs relus.
    if (String(p_property).ends_with("_preset"))
    {
        _rebuild_properties();
    }
}

void InputEditorWindow::_on_text_submitted(const String &p_text, uint64_t p_object_id, const StringName &p_property, int p_kind)
{
    Variant value = p_kind == 1 ? Variant(StringName(p_text)) : (p_kind == 2 ? Variant(NodePath(p_text)) : Variant(p_text));
    _commit_property(ObjectDB::get_instance(ObjectID(p_object_id)), p_property, value);
}

void InputEditorWindow::_on_text_focus_exited(uint64_t p_line_id, uint64_t p_object_id, const StringName &p_property, int p_kind)
{
    if (LineEdit *line = Object::cast_to<LineEdit>(ObjectDB::get_instance(ObjectID(p_line_id))))
    {
        _on_text_submitted(line->get_text(), p_object_id, p_property, p_kind);
    }
}

void InputEditorWindow::_on_inspect_pressed(uint64_t p_object_id)
{
    if (Object *object = ObjectDB::get_instance(ObjectID(p_object_id)))
    {
        EditorInterface::get_singleton()->inspect_object(object);
    }
}

// 🎯 Capture

void InputEditorWindow::_on_capture_pressed(const StringName &p_slot)
{
    if (selected_action.is_null())
    {
        return;
    }
    capture_action = selected_action;
    capture_slot = p_slot;
    _set_status(String(U"🎯 « ") + String(p_slot) + U" » : appuyez sur la touche, le bouton de souris / manette ou poussez l'axe voulu (Échap : annuler).");
    _rebuild_properties();
}

void InputEditorWindow::_on_clear_slot_pressed(const StringName &p_slot)
{
    if (selected_action.is_null())
    {
        return;
    }
    Dictionary before = selected_action->get_device_state();
    String slot = p_slot;
    selected_action->set(p_slot, slot.contains("key") ? 0 : -1);
    if (slot == "key")
    {
        selected_action->set("mouse_button", 0);
    }
    else if (slot == "joy_button")
    {
        selected_action->set("joy_axis", -1);
    }
    Dictionary after = selected_action->get_device_state();
    selected_action->set_device_state(before);
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(U"Entrées : effacer une entrée", UndoRedo::MERGE_DISABLE, _player());
    ur->add_do_method(selected_action.ptr(), "set_device_state", after);
    ur->add_undo_method(selected_action.ptr(), "set_device_state", before);
    ur->commit_action();
    _rebuild_properties();
    model_signature = _compute_signature();
}

void InputEditorWindow::_on_window_input(const Ref<InputEvent> &p_event)
{
    if (capture_action.is_valid())
    {
        Ref<InputEventKey> key = p_event;
        if (key.is_valid() && key->is_pressed() && key->get_keycode() == KEY_ESCAPE)
        {
            capture_action.unref();
            _set_status(U"Capture annulée.");
            _rebuild_properties();
            set_input_as_handled();
            return;
        }
        _finish_capture(p_event);
        return;
    }
    if (source == SOURCE_LOCAL && test_player)
    {
        test_player->handle_input_event(p_event);
    }
}

void InputEditorWindow::_poll_joypad_capture()
{
    if (capture_action.is_null())
    {
        return;
    }
    Input *input = Input::get_singleton();
    TypedArray<int> pads = input->get_connected_joypads();
    for (int i = 0; i < pads.size(); i++)
    {
        int pad = pads[i];
        for (int button = 0; button <= 20; button++)
        {
            if (input->is_joy_button_pressed(pad, JoyButton(button)))
            {
                Ref<InputEventJoypadButton> event;
                event.instantiate();
                event->set_device(pad);
                event->set_button_index(JoyButton(button));
                event->set_pressed(true);
                _finish_capture(event);
                return;
            }
        }
        for (int axis = 0; axis <= 5; axis++)
        {
            float value = input->get_joy_axis(pad, JoyAxis(axis));
            if (Math::abs(value) >= 0.6f)
            {
                Ref<InputEventJoypadMotion> event;
                event.instantiate();
                event->set_device(pad);
                event->set_axis(JoyAxis(axis));
                event->set_axis_value(value);
                _finish_capture(event);
                return;
            }
        }
    }
}

void InputEditorWindow::_finish_capture(const Ref<InputEvent> &p_event)
{
    Ref<InputAction> action = capture_action;
    if (action.is_null())
    {
        return;
    }
    Dictionary before = action->get_device_state();
    if (!action->apply_rebind(capture_slot, p_event, 0.5))
    {
        return; // évènement qui ne convient pas à cet emplacement (mouvement de souris...) : on attend
    }
    set_input_as_handled();
    Dictionary after = action->get_device_state();
    action->set_device_state(before);
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(U"Entrées : assigner une entrée", UndoRedo::MERGE_DISABLE, _player());
    ur->add_do_method(action.ptr(), "set_device_state", after);
    ur->add_undo_method(action.ptr(), "set_device_state", before);
    ur->commit_action();
    _set_status(String(U"✅ ") + String(action->get_action_name()) + " / " + String(capture_slot) + U" → " + action->get_slot_text(capture_slot));
    capture_action.unref();
    _rebuild_properties();
    model_signature = _compute_signature();
}

// 🌳 Structure

void InputEditorWindow::_set_array(Object *p_owner, const StringName &p_property, const Array &p_before, const Array &p_after, const String &p_action_name)
{
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(p_action_name, UndoRedo::MERGE_DISABLE, _player());
    ur->add_do_property(p_owner, p_property, p_after);
    ur->add_undo_property(p_owner, p_property, p_before);
    ur->commit_action();
    if (_player())
    {
        _player()->update_configuration_warnings();
    }
}

void InputEditorWindow::_on_add_context_pressed()
{
    PlayerInput *player = _player();
    if (!player)
    {
        return;
    }
    TypedArray<InputContext> before = player->get_contexts();
    Ref<InputContext> context;
    context.instantiate();
    String name = "contexte";
    int suffix = 2;
    while (true)
    {
        bool taken = false;
        for (int i = 0; i < before.size(); i++)
        {
            Ref<InputContext> other = before[i];
            taken = taken || (other.is_valid() && String(other->get_context_name()) == name);
        }
        if (!taken)
        {
            break;
        }
        name = String("contexte_") + String::num_int64(suffix++);
    }
    context->set_context_name(name);
    TypedArray<InputContext> after = before.duplicate();
    after.push_back(context);
    _set_array(player, "contexts", before, after, U"Entrées : ajouter un contexte");
    _select(context, Ref<InputAction>());
}

void InputEditorWindow::_on_add_action_type(int p_id)
{
    Ref<InputContext> context = selected_context;
    if (context.is_null())
    {
        TypedArray<InputContext> contexts = _contexts();
        context = contexts.is_empty() ? Ref<InputContext>() : Ref<InputContext>(contexts[0]);
    }
    if (context.is_null())
    {
        _set_status(U"Ajoutez d'abord un contexte.");
        return;
    }
    Ref<InputAction> action = ClassDBSingleton::get_singleton()->instantiate(ACTION_TYPES[p_id]);
    if (action.is_null())
    {
        return;
    }
    static const char *const base_names[] = {"bouton", "axe", "deplacement", "deplacement_3d", "souris", "gyroscope"};
    String name = base_names[p_id];
    int suffix = 2;
    while (context->has_action(name))
    {
        name = String(base_names[p_id]) + "_" + String::num_int64(suffix++);
    }
    action->set_action_name(name);
    TypedArray<InputAction> before = context->get_actions();
    TypedArray<InputAction> after = before.duplicate();
    after.push_back(action);
    _set_array(context.ptr(), "actions", before, after, U"Entrées : ajouter une action");
    _select(context, action);
}

void InputEditorWindow::_on_duplicate_pressed()
{
    if (selected_action.is_valid() && selected_context.is_valid())
    {
        Ref<InputAction> copy = selected_action->make_runtime_copy();
        String name = String(selected_action->get_action_name()) + "_copie";
        while (selected_context->has_action(name))
        {
            name += "_";
        }
        copy->set_action_name(name);
        TypedArray<InputAction> before = selected_context->get_actions();
        TypedArray<InputAction> after = before.duplicate();
        after.insert(after.find(selected_action) + 1, copy);
        _set_array(selected_context.ptr(), "actions", before, after, U"Entrées : dupliquer une action");
        _select(selected_context, copy);
        return;
    }
    PlayerInput *player = _player();
    if (selected_context.is_valid() && player)
    {
        Ref<InputContext> copy = selected_context->make_runtime_copy();
        copy->set_context_name(String(selected_context->get_context_name()) + "_copie");
        TypedArray<InputContext> before = player->get_contexts();
        TypedArray<InputContext> after = before.duplicate();
        after.insert(after.find(selected_context) + 1, copy);
        _set_array(player, "contexts", before, after, U"Entrées : dupliquer un contexte");
        _select(copy, Ref<InputAction>());
    }
}

void InputEditorWindow::_on_delete_pressed()
{
    if (selected_action.is_valid() && selected_context.is_valid())
    {
        TypedArray<InputAction> before = selected_context->get_actions();
        TypedArray<InputAction> after = before.duplicate();
        after.erase(selected_action);
        _set_array(selected_context.ptr(), "actions", before, after, U"Entrées : supprimer une action");
        _select(selected_context, Ref<InputAction>());
        return;
    }
    PlayerInput *player = _player();
    if (selected_context.is_valid() && player)
    {
        TypedArray<InputContext> before = player->get_contexts();
        TypedArray<InputContext> after = before.duplicate();
        after.erase(selected_context);
        _set_array(player, "contexts", before, after, U"Entrées : supprimer un contexte");
        _select(after.is_empty() ? Ref<InputContext>() : Ref<InputContext>(after[0]), Ref<InputAction>());
    }
}

void InputEditorWindow::_on_move_pressed(int p_direction)
{
    Object *owner = nullptr;
    StringName property;
    Variant item;
    if (selected_action.is_valid() && selected_context.is_valid())
    {
        owner = selected_context.ptr();
        property = "actions";
        item = selected_action;
    }
    else if (selected_context.is_valid() && _player())
    {
        owner = _player();
        property = "contexts";
        item = selected_context;
    }
    if (!owner)
    {
        return;
    }
    Array before = owner->get(property);
    int index = before.find(item);
    int target = index + p_direction;
    if (index < 0 || target < 0 || target >= before.size())
    {
        return;
    }
    Array after = before.duplicate();
    after[index] = after[target];
    after[target] = item;
    _set_array(owner, property, before, after, U"Entrées : réordonner");
    _refresh_all();
}

void InputEditorWindow::_on_save_pressed()
{
    int saved = 0;
    int embedded = 0;
    TypedArray<InputContext> contexts = _contexts();
    auto save = [&](const Ref<Resource> &p_resource) {
        String path = p_resource->get_path();
        if (path.is_empty() || path.contains("::"))
        {
            embedded++;
            return;
        }
        if (ResourceSaver::get_singleton()->save(p_resource, path) == OK)
        {
            saved++;
        }
    };
    for (int i = 0; i < contexts.size(); i++)
    {
        Ref<InputContext> context = contexts[i];
        if (context.is_null())
        {
            continue;
        }
        TypedArray<InputAction> actions = context->get_actions();
        for (int a = 0; a < actions.size(); a++)
        {
            Ref<InputAction> action = actions[a];
            if (action.is_valid())
            {
                save(action);
            }
        }
        save(context);
    }
    _set_status(String(U"💾 ") + String::num_int64(saved) + U" fichier(s) enregistré(s)" + (embedded > 0 ? String(U", ") + String::num_int64(embedded) + U" ressource(s) intégrée(s) : enregistrez la scène (Ctrl+S)." : String(".")));
}

// 🔗 Liaisons

Node *InputEditorWindow::_binding_target(const Ref<InputBinding> &p_binding) const
{
    PlayerInput *player = _player();
    if (!player || p_binding.is_null() || p_binding->get_target().is_empty() || !player->is_inside_tree())
    {
        return nullptr;
    }
    return player->get_node_or_null(p_binding->get_target());
}

void InputEditorWindow::_rebuild_bindings()
{
    clear_children(bindings_box);
    if (selected_action.is_null())
    {
        bindings_box->add_child(make_label(U"Choisissez une action : ses liaisons appellent une méthode d'un nœud quand elle émet un évènement.", 0.7f));
        return;
    }
    Label *header = make_label(String(U"🔗 Liaisons de « ") + String(selected_action->get_action_name()) + U" »");
    header->add_theme_font_size_override("font_size", 16);
    bindings_box->add_child(header);
    Label *help = make_label(U"Quand l'action émet l'évènement, le PlayerInput appelle cible.méthode(valeur de l'évènement si « valeur », puis les arguments). Cible : chemin relatif au PlayerInput (▾ : nœuds de la scène).", 0.65f);
    help->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    bindings_box->add_child(help);

    TypedArray<Dictionary> events = selected_action->get_events();
    TypedArray<InputBinding> bindings = selected_action->get_bindings();
    for (int i = 0; i < bindings.size(); i++)
    {
        Ref<InputBinding> binding = bindings[i];
        if (binding.is_null())
        {
            continue;
        }
        uint64_t binding_id = binding->get_instance_id();
        bindings_box->add_child(memnew(HSeparator));
        HBoxContainer *row = memnew(HBoxContainer);
        bindings_box->add_child(row);
        CheckBox *enabled = memnew(CheckBox);
        enabled->set_pressed_no_signal(binding->is_enabled());
        enabled->set_tooltip_text(U"Liaison active");
        enabled->connect("toggled", callable_mp(this, &InputEditorWindow::_on_bool_edited).bind(binding_id, StringName("enabled")));
        row->add_child(enabled);
        OptionButton *event = memnew(OptionButton);
        event->set_tooltip_text(U"Évènement (signal) de l'action");
        bool found = false;
        for (int e = 0; e < events.size(); e++)
        {
            Dictionary entry = events[e];
            event->add_item(entry["name"]);
            event->set_item_tooltip(e, String(entry["name"]) + "(" + String(", ").join(PackedStringArray(entry["args"])) + ")");
            if (StringName(entry["name"]) == binding->get_event())
            {
                event->select(e);
                found = true;
            }
        }
        if (!found)
        {
            event->add_item(String(U"⚠️ ") + String(binding->get_event()));
            event->select(event->get_item_count() - 1);
        }
        event->connect("item_selected", callable_mp(this, &InputEditorWindow::_on_binding_event_selected).bind((uint64_t)event->get_instance_id(), binding_id));
        row->add_child(event);
        row->add_child(make_label(U"→"));
        _add_line_edit(row, binding.ptr(), "target", 2, U"../Joueur");
        MenuButton *target_menu = memnew(MenuButton);
        target_menu->set_text(U"▾");
        target_menu->set_tooltip_text(U"Nœuds de la scène éditée");
        target_menu->get_popup()->connect("about_to_popup", callable_mp(this, &InputEditorWindow::_on_target_suggestions).bind((uint64_t)target_menu->get_instance_id(), binding_id));
        target_menu->get_popup()->connect("id_pressed", callable_mp(this, &InputEditorWindow::_on_target_suggestion_chosen).bind(binding_id));
        row->add_child(target_menu);
        row->add_child(make_label("."));
        _add_line_edit(row, binding.ptr(), "method", 1, U"méthode");
        MenuButton *method_menu = memnew(MenuButton);
        method_menu->set_text(U"▾");
        method_menu->set_tooltip_text(U"Méthodes de la cible");
        method_menu->get_popup()->connect("about_to_popup", callable_mp(this, &InputEditorWindow::_on_method_suggestions).bind((uint64_t)method_menu->get_instance_id(), binding_id));
        method_menu->get_popup()->connect("id_pressed", callable_mp(this, &InputEditorWindow::_on_method_suggestion_chosen).bind(binding_id));
        row->add_child(method_menu);
        Button *remove = memnew(Button);
        remove->set_text(U"🗑");
        remove->set_tooltip_text(U"Supprimer la liaison");
        remove->connect("pressed", callable_mp(this, &InputEditorWindow::_on_remove_binding_pressed).bind(binding_id));
        row->add_child(remove);

        HBoxContainer *second = memnew(HBoxContainer);
        bindings_box->add_child(second);
        CheckBox *pass_value = memnew(CheckBox);
        pass_value->set_text(U"valeur");
        pass_value->set_tooltip_text(U"Passer la valeur de l'évènement en premier argument");
        pass_value->set_pressed_no_signal(binding->get_pass_value());
        pass_value->connect("toggled", callable_mp(this, &InputEditorWindow::_on_bool_edited).bind(binding_id, StringName("pass_value")));
        second->add_child(pass_value);
        LineEdit *arguments = memnew(LineEdit);
        arguments->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        arguments->set_placeholder(U"arguments supplémentaires : [1.5, \"texte\", true]");
        arguments->set_text(binding->get_arguments().is_empty() ? String() : UtilityFunctions::var_to_str(binding->get_arguments()));
        arguments->connect("text_submitted", callable_mp(this, &InputEditorWindow::_on_arguments_submitted).bind(binding_id));
        arguments->connect("focus_exited", callable_mp(this, &InputEditorWindow::_on_arguments_focus_exited).bind((uint64_t)arguments->get_instance_id(), binding_id));
        second->add_child(arguments);
        String state;
        if (!_player())
        {
            state = U"ℹ️ cible résolue en jeu";
        }
        else if (Node *target = _binding_target(binding))
        {
            state = target->has_method(binding->get_method()) ? String(U"✅ ") + String(target->get_name()) + " (" + target->get_class() + ")" : String(U"⚠️ méthode introuvable sur ") + String(target->get_name());
        }
        else
        {
            state = U"⚠️ cible introuvable";
        }
        second->add_child(make_label(state, 0.8f));
    }
    bindings_box->add_child(memnew(HSeparator));
    Button *add = memnew(Button);
    add->set_text(U"➕ Ajouter une liaison");
    add->connect("pressed", callable_mp(this, &InputEditorWindow::_on_add_binding_pressed));
    bindings_box->add_child(add);
}

void InputEditorWindow::_on_add_binding_pressed()
{
    if (selected_action.is_null())
    {
        return;
    }
    Ref<InputBinding> binding;
    binding.instantiate();
    TypedArray<Dictionary> events = selected_action->get_events();
    if (!events.is_empty())
    {
        binding->set_event(Dictionary(events[0])["name"]);
    }
    TypedArray<InputBinding> before = selected_action->get_bindings();
    TypedArray<InputBinding> after = before.duplicate();
    after.push_back(binding);
    _set_array(selected_action.ptr(), "bindings", before, after, U"Entrées : ajouter une liaison");
    _refresh_all();
}

void InputEditorWindow::_on_remove_binding_pressed(uint64_t p_binding_id)
{
    Ref<InputBinding> binding = Object::cast_to<InputBinding>(ObjectDB::get_instance(ObjectID(p_binding_id)));
    if (selected_action.is_null() || binding.is_null())
    {
        return;
    }
    TypedArray<InputBinding> before = selected_action->get_bindings();
    TypedArray<InputBinding> after = before.duplicate();
    after.erase(binding);
    _set_array(selected_action.ptr(), "bindings", before, after, U"Entrées : supprimer une liaison");
    _refresh_all();
}

void InputEditorWindow::_on_binding_event_selected(int p_index, uint64_t p_option_id, uint64_t p_binding_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(ObjectID(p_option_id)));
    if (option)
    {
        _commit_property(ObjectDB::get_instance(ObjectID(p_binding_id)), "event", StringName(option->get_item_text(p_index)));
    }
}

void InputEditorWindow::_on_target_suggestions(uint64_t p_menu_id, uint64_t /*p_binding_id*/)
{
    MenuButton *menu = Object::cast_to<MenuButton>(ObjectDB::get_instance(ObjectID(p_menu_id)));
    if (!menu)
    {
        return;
    }
    PopupMenu *popup = menu->get_popup();
    popup->clear();
    suggestions.clear();
    Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
    PlayerInput *player = _player();
    if (!root)
    {
        popup->add_item(U"(aucune scène ouverte)");
        popup->set_item_disabled(0, true);
        return;
    }
    TypedArray<Node> nodes = root->find_children("*", "", true, false);
    nodes.push_front(root);
    for (int i = 0; i < nodes.size() && i < 400; i++)
    {
        Node *node = Object::cast_to<Node>(nodes[i]);
        if (!node || node == player)
        {
            continue;
        }
        String path = player ? String(player->get_path_to(node)) : String(root->get_path_to(node));
        suggestions.push_back(path);
        popup->add_item(path + "   (" + node->get_class() + ")", suggestions.size() - 1);
    }
}

void InputEditorWindow::_on_target_suggestion_chosen(int p_id, uint64_t p_binding_id)
{
    if (p_id >= 0 && p_id < suggestions.size())
    {
        _commit_property(ObjectDB::get_instance(ObjectID(p_binding_id)), "target", NodePath(suggestions[p_id]));
        _rebuild_bindings();
    }
}

void InputEditorWindow::_on_method_suggestions(uint64_t p_menu_id, uint64_t p_binding_id)
{
    MenuButton *menu = Object::cast_to<MenuButton>(ObjectDB::get_instance(ObjectID(p_menu_id)));
    Ref<InputBinding> binding = Object::cast_to<InputBinding>(ObjectDB::get_instance(ObjectID(p_binding_id)));
    if (!menu || binding.is_null())
    {
        return;
    }
    PopupMenu *popup = menu->get_popup();
    popup->clear();
    suggestions.clear();
    Node *target = _binding_target(binding);
    if (!target)
    {
        popup->add_item(U"(cible introuvable : choisissez d'abord la cible)");
        popup->set_item_disabled(0, true);
        return;
    }
    auto add_methods = [&](const TypedArray<Dictionary> &p_methods, const String &p_origin) {
        for (int i = 0; i < p_methods.size(); i++)
        {
            Dictionary method = p_methods[i];
            String name = method["name"];
            if (name.begins_with("_") || name.begins_with("get_") || name.begins_with("is_") || name.begins_with("has_") || name.contains(".") || suggestions.has(name))
            {
                continue;
            }
            PackedStringArray arguments;
            Array args = method["args"];
            for (int a = 0; a < args.size(); a++)
            {
                Dictionary argument = args[a];
                arguments.push_back(String(argument["name"]) + ": " + Variant::get_type_name(Variant::Type(int(argument["type"]))));
            }
            suggestions.push_back(name);
            popup->add_item(name + "(" + String(", ").join(arguments) + ")   " + p_origin, suggestions.size() - 1);
        }
    };
    Ref<Script> script = target->get_script();
    if (script.is_valid())
    {
        add_methods(script->get_script_method_list(), U"· script");
    }
    // Méthodes des classes d'extension (GDExtension) de la cible, sans les milliers de méthodes du moteur.
    ClassDBSingleton *class_db = ClassDBSingleton::get_singleton();
    StringName class_name = target->get_class();
    while (class_name != StringName())
    {
        ClassDBSingleton::APIType api = class_db->class_get_api_type(class_name);
        if (api == ClassDBSingleton::API_EXTENSION || api == ClassDBSingleton::API_EDITOR_EXTENSION)
        {
            add_methods(class_db->class_get_method_list(class_name, true), String(U"· ") + String(class_name));
        }
        class_name = class_db->get_parent_class(class_name);
    }
    if (suggestions.is_empty())
    {
        popup->add_item(U"(aucune méthode proposée : saisissez son nom)");
        popup->set_item_disabled(0, true);
    }
}

void InputEditorWindow::_on_method_suggestion_chosen(int p_id, uint64_t p_binding_id)
{
    if (p_id >= 0 && p_id < suggestions.size())
    {
        _commit_property(ObjectDB::get_instance(ObjectID(p_binding_id)), "method", StringName(suggestions[p_id]));
        _rebuild_bindings();
    }
}

void InputEditorWindow::_commit_arguments(const String &p_text, const Ref<InputBinding> &p_binding)
{
    if (p_binding.is_null())
    {
        return;
    }
    String text = p_text.strip_edges();
    Array arguments;
    if (!text.is_empty())
    {
        Variant parsed = UtilityFunctions::str_to_var(text);
        if (parsed.get_type() == Variant::NIL && text != "null")
        {
            _set_status(String(U"⚠️ Arguments illisibles : ") + text + U" (exemple : [1.5, \"texte\", true])");
            return;
        }
        arguments = parsed.get_type() == Variant::ARRAY ? Array(parsed) : Array::make(parsed);
    }
    if (arguments != p_binding->get_arguments())
    {
        _commit_property(p_binding.ptr(), "arguments", arguments);
    }
}

void InputEditorWindow::_on_arguments_submitted(const String &p_text, uint64_t p_binding_id) { _commit_arguments(p_text, Object::cast_to<InputBinding>(ObjectDB::get_instance(ObjectID(p_binding_id)))); }

void InputEditorWindow::_on_arguments_focus_exited(uint64_t p_line_id, uint64_t p_binding_id)
{
    if (LineEdit *line = Object::cast_to<LineEdit>(ObjectDB::get_instance(ObjectID(p_line_id))))
    {
        _on_arguments_submitted(line->get_text(), p_binding_id);
    }
}

// 🐞 Débogage

void InputEditorWindow::_update_source_list()
{
    InputDebuggerPlugin *debugger = InputDebuggerPlugin::get_singleton();
    Array players = debugger && debugger->has_active_session() ? debugger->get_players() : Array();
    String signature = Variant(players).stringify() + String::num_int64(int(source)) + remote_path;
    if (signature == sources_signature && source_option->get_item_count() > 0)
    {
        return;
    }
    sources_signature = signature;
    source_option->clear();
    source_option->add_item(U"⏸ Aucune", 0);
    source_option->add_item(U"🧪 Test dans l'éditeur (périphériques réels, liaisons non appelées)", 1);
    int selected = source == SOURCE_LOCAL ? 1 : 0;
    for (int i = 0; i < players.size(); i++)
    {
        Array entry = players[i];
        source_option->add_item(String(U"🎮 Jeu : ") + String(entry[0]), 100 + i);
        source_option->set_item_metadata(source_option->get_item_count() - 1, entry[0]);
        if (source == SOURCE_REMOTE && String(entry[0]) == remote_path)
        {
            selected = source_option->get_item_count() - 1;
        }
    }
    source_option->select(selected);
}

void InputEditorWindow::_on_source_selected(int p_index)
{
    int id = source_option->get_item_id(p_index);
    _stop_local_test();
    InputDebuggerPlugin *debugger = InputDebuggerPlugin::get_singleton();
    if (id == 1)
    {
        source = SOURCE_LOCAL;
        _start_local_test();
    }
    else if (id >= 100)
    {
        source = SOURCE_REMOTE;
        remote_path = source_option->get_item_metadata(p_index);
    }
    else
    {
        source = SOURCE_NONE;
    }
    if (debugger)
    {
        PackedStringArray watched;
        if (source == SOURCE_REMOTE)
        {
            watched.push_back(remote_path);
        }
        debugger->set_watched(watched);
    }
    debug_signature = String();
    last_log_signature = String();
    sources_signature = String();
    _update_source_list();
}

void InputEditorWindow::_start_local_test()
{
    _stop_local_test();
    test_player = memnew(PlayerInput);
    test_player->set_contexts(_contexts());
    test_player->set_dispatch_bindings(false);
    if (PlayerInput *player = _player())
    {
        test_player->set_device(player->get_device());
        test_player->set_device_filter(player->get_device_filter());
        test_player->set_auto_switch_device(player->get_auto_switch_device());
    }
    test_player->setup();
}

void InputEditorWindow::_stop_local_test()
{
    if (test_player)
    {
        memdelete(test_player);
        test_player = nullptr;
    }
}

void InputEditorWindow::_update_debug(double p_delta)
{
    Dictionary snapshot;
    switch (source)
    {
    case SOURCE_LOCAL:
        if (test_player)
        {
            test_player->poll(p_delta);
            snapshot = test_player->get_debug_snapshot();
        }
        debug_status->set_text(U"🧪 Test : appuyez sur les touches / boutons (fenêtre au premier plan) ou glissez dans une jauge. Les liaisons sont décrites dans le journal sans être appelées. Les modifications relancent le test.");
        break;
    case SOURCE_REMOTE:
    {
        InputDebuggerPlugin *debugger = InputDebuggerPlugin::get_singleton();
        snapshot = debugger ? debugger->get_snapshot(remote_path) : Dictionary();
        debug_status->set_text(snapshot.is_empty() ? String(U"En attente du jeu…") : String(U"🎮 Jeu en cours : ") + remote_path + U" · périphérique " + String::num_int64(int(snapshot.get("device", 0))) + " " + String(snapshot.get("joy_name", "")) + U" · glisser dans une jauge simule l'action dans le jeu.");
        break;
    }
    default:
        debug_status->set_text(U"Choisissez une source : test dans l'éditeur, ou un PlayerInput du jeu lancé depuis l'éditeur.");
        break;
    }
    // Structure (contextes / actions) : reconstruite seulement si elle change.
    Array contexts = snapshot.get("contexts", Array());
    String structure;
    for (int i = 0; i < contexts.size(); i++)
    {
        Dictionary context = contexts[i];
        structure += "#" + String(context["name"]);
        Array actions = context["actions"];
        for (int a = 0; a < actions.size(); a++)
        {
            Dictionary action = actions[a];
            structure += "@" + String(action["name"]) + ":" + String(action["class"]);
        }
    }
    if (structure != debug_signature)
    {
        debug_signature = structure;
        _rebuild_debug(snapshot);
    }
    for (int i = 0; i < contexts.size(); i++)
    {
        Dictionary context = contexts[i];
        String context_name = context["name"];
        Array context_widgets = debug_widgets.get(String("#") + context_name, Array());
        if (context_widgets.size() == 2)
        {
            if (CheckButton *check = Object::cast_to<CheckButton>(ObjectDB::get_instance(ObjectID(uint64_t(context_widgets[0])))))
            {
                check->set_pressed_no_signal(context["enabled"]);
            }
            if (Label *state = Object::cast_to<Label>(ObjectDB::get_instance(ObjectID(uint64_t(context_widgets[1])))))
            {
                bool active = context["active"];
                state->set_text(active ? String(U"● actif") : (bool(context["enabled"]) ? String(U"◐ bloqué (contexte prioritaire)") : String(U"○ désactivé")));
                state->set_modulate(active ? Color(0.5f, 1.0f, 0.6f) : Color(1, 1, 1, 0.55f));
            }
        }
        Array actions = context["actions"];
        for (int a = 0; a < actions.size(); a++)
        {
            Dictionary action = actions[a];
            Array widgets = debug_widgets.get(context_name + "/" + String(action["name"]), Array());
            if (widgets.size() != 2)
            {
                continue;
            }
            if (InputActionMeter *meter = Object::cast_to<InputActionMeter>(ObjectDB::get_instance(ObjectID(uint64_t(widgets[0])))))
            {
                meter->set_value(action["value"]);
            }
            if (Label *label = Object::cast_to<Label>(ObjectDB::get_instance(ObjectID(uint64_t(widgets[1])))))
            {
                String text = String(action["text"]);
                if (String(action["event"]) != String())
                {
                    text += String(U"\n⚡ ") + String(action["event"]) + U" ×" + String::num_int64(int(action["count"]));
                }
                if (bool(action["simulated"]))
                {
                    text += U"  🧪";
                }
                label->set_text(text);
            }
        }
    }
    // 📜 Journal
    Array log = snapshot.get("log", Array());
    String log_signature = String::num_int64(log.size()) + (log.is_empty() ? String() : Variant(log[log.size() - 1]).stringify());
    if (log_signature != last_log_signature)
    {
        last_log_signature = log_signature;
        String text;
        for (int i = log.size() - 1; i >= 0 && i >= log.size() - 40; i--)
        {
            Dictionary entry = log[i];
            text += String("[color=gray]") + String::num(double(int64_t(entry["msec"])) / 1000.0, 2) + "s[/color] [b]" + String(entry["action"]) + "[/b] · " + String(entry["event"]);
            String arguments = entry["arguments"];
            if (!arguments.is_empty())
            {
                text += " (" + arguments + ")";
            }
            text += "\n";
            PackedStringArray calls = entry["calls"];
            for (const String &call : calls)
            {
                text += String(U"      [color=#8fd18f]→ ") + call + "[/color]\n";
            }
        }
        log_label->set_text(text);
    }
}

void InputEditorWindow::_rebuild_debug(const Dictionary &p_snapshot)
{
    clear_children(debug_box);
    debug_widgets.clear();
    Array contexts = p_snapshot.get("contexts", Array());
    for (int i = 0; i < contexts.size(); i++)
    {
        Dictionary context = contexts[i];
        StringName context_name = context["name"];
        HBoxContainer *header = memnew(HBoxContainer);
        debug_box->add_child(header);
        CheckButton *check = memnew(CheckButton);
        check->set_text(String(U"🗂️ ") + String(context_name) + U"  (priorité " + String::num_int64(int(context["priority"])) + ")");
        check->set_pressed_no_signal(context["enabled"]);
        check->set_tooltip_text(U"Activer / désactiver ce contexte (dans le test ou dans le jeu)");
        check->connect("toggled", callable_mp(this, &InputEditorWindow::_on_context_toggled).bind(context_name));
        header->add_child(check);
        Label *state = make_label(String());
        header->add_child(state);
        debug_widgets[String("#") + String(context_name)] = Array::make((uint64_t)check->get_instance_id(), (uint64_t)state->get_instance_id());

        HFlowContainer *flow = memnew(HFlowContainer);
        flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        debug_box->add_child(flow);
        Array actions = context["actions"];
        for (int a = 0; a < actions.size(); a++)
        {
            Dictionary action = actions[a];
            StringName action_name = action["name"];
            HBoxContainer *cell = memnew(HBoxContainer);
            cell->set_custom_minimum_size(Vector2(230, 0));
            flow->add_child(cell);
            InputActionMeter *meter = memnew(InputActionMeter);
            meter->set_value(action["value"]);
            meter->connect("simulate_requested", callable_mp(this, &InputEditorWindow::_on_meter_simulate).bind(action_name));
            meter->connect("simulation_released", callable_mp(this, &InputEditorWindow::_on_meter_released).bind(action_name));
            cell->add_child(meter);
            VBoxContainer *texts = memnew(VBoxContainer);
            texts->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            cell->add_child(texts);
            Label *name = make_label(String(action_name));
            name->add_theme_color_override("font_color", Color(0.95f, 0.85f, 0.55f));
            name->set_tooltip_text(String(action["class"]) + U" · " + String::num_int64(int(action["bindings"])) + U" liaison(s)");
            name->set_mouse_filter(Control::MOUSE_FILTER_PASS);
            texts->add_child(name);
            Label *value = make_label(String(), 0.85f);
            texts->add_child(value);
            debug_widgets[String(context_name) + "/" + String(action_name)] = Array::make((uint64_t)meter->get_instance_id(), (uint64_t)value->get_instance_id());
        }
        debug_box->add_child(memnew(HSeparator));
    }
}

void InputEditorWindow::_on_meter_simulate(const Variant &p_value, const StringName &p_action)
{
    if (source == SOURCE_LOCAL && test_player)
    {
        test_player->simulate(p_action, p_value);
    }
    else if (source == SOURCE_REMOTE)
    {
        if (InputDebuggerPlugin *debugger = InputDebuggerPlugin::get_singleton())
        {
            debugger->send_simulate(remote_path, p_action, p_value);
        }
    }
}

void InputEditorWindow::_on_meter_released(const StringName &p_action)
{
    if (source == SOURCE_LOCAL && test_player)
    {
        test_player->simulate(p_action, Variant()); // une image « au repos », puis fin de la simulation
        test_player->poll(0.0);
        test_player->stop_simulation(p_action);
    }
    else if (source == SOURCE_REMOTE)
    {
        if (InputDebuggerPlugin *debugger = InputDebuggerPlugin::get_singleton())
        {
            debugger->send_simulate(remote_path, p_action, Variant());
        }
    }
}

void InputEditorWindow::_on_context_toggled(bool p_enabled, const StringName &p_context)
{
    if (source == SOURCE_LOCAL && test_player)
    {
        test_player->set_context_enabled(p_context, p_enabled);
    }
    else if (source == SOURCE_REMOTE)
    {
        if (InputDebuggerPlugin *debugger = InputDebuggerPlugin::get_singleton())
        {
            debugger->send_set_context(remote_path, p_context, p_enabled);
        }
    }
}

void InputEditorWindow::_on_clear_log_pressed()
{
    if (test_player)
    {
        test_player->clear_event_log();
    }
    log_label->clear();
    last_log_signature = String();
}

void InputEditorWindow::_notification(int p_what)
{
    if (p_what != NOTIFICATION_PROCESS)
    {
        return;
    }
    double delta = get_process_delta_time();
    _poll_joypad_capture();
    _update_debug(delta);
    poll_accumulator += delta;
    if (poll_accumulator < 0.3)
    {
        return;
    }
    poll_accumulator = 0.0;
    if (lone_context.is_null() && !_player() && player_id.is_valid())
    {
        player_id = ObjectID(); // PlayerInput supprimé ou scène fermée
        _stop_local_test();
        title_label->set_text(U"🎮 PlayerInput introuvable (supprimé ou scène fermée)");
        _refresh_all();
    }
    String signature = _compute_signature();
    if (signature != model_signature || refresh_pending)
    {
        _refresh_all();
        if (source == SOURCE_LOCAL)
        {
            _start_local_test(); // le test reprend les contextes modifiés
        }
    }
    _update_source_list();
}

void InputEditorWindow::_bind_methods() {}
