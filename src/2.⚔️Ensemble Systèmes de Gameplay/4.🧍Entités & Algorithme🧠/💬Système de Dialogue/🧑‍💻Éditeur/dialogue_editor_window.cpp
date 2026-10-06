#include "dialogue_editor_window.hpp"

#include "../../🤾Système de Comportement/🗺️Graphe de comportement/🧑‍💻Éditeur/behavior_editor_plugin.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

ObjectID DialogueEditorWindow::current_window_id;

namespace
{
constexpr double POLL_INTERVAL = 0.2;

struct NodeType
{
    const char *type;
    const char32_t *label;
    const char *prefix;
};
const NodeType NODE_TYPES[] = {
    {"DialogueLine", U"🗨️ Réplique", "ligne"},
    {"DialogueChoice", U"🔀 Choix", "choix"},
    {"DialogueBranch", U"❔ Embranchement", "test"},
    {"DialogueAction", U"⚙️ Action", "action"},
    {"DialogueEnd", U"🏁 Fin", "fin"},
};

EditorUndoRedoManager *undo_redo() { return EditorInterface::get_singleton()->get_editor_undo_redo(); }
} // namespace

// 🪟 Ouverture / fermeture

void DialogueEditorWindow::open_for(const Ref<DialogueResource> &p_dialogue)
{
    EditorInterface *editor = EditorInterface::get_singleton();
    if (!editor || !editor->get_base_control() || p_dialogue.is_null())
    {
        return;
    }
    if (DialogueEditorWindow *window = get_current())
    {
        window->_set_dialogue(p_dialogue);
        window->grab_focus();
        return;
    }
    DialogueEditorWindow *window = memnew(DialogueEditorWindow);
    window->set_size(Vector2i(1200, 760));
    window->set_min_size(Vector2i(640, 400));
    window->set_theme(editor->get_editor_theme());
    window->set_visible(false);
    window->connect("close_requested", callable_mp(window, &DialogueEditorWindow::_on_close_requested));
    window->_build_ui();
    editor->get_base_control()->add_child(window);
    current_window_id = ObjectID(window->get_instance_id());
    window->_set_dialogue(p_dialogue);
    window->popup_centered();
}

void DialogueEditorWindow::close_current()
{
    if (DialogueEditorWindow *window = get_current())
    {
        window->queue_free();
    }
    current_window_id = ObjectID();
}

DialogueEditorWindow *DialogueEditorWindow::get_current() { return Object::cast_to<DialogueEditorWindow>(ObjectDB::get_instance(current_window_id)); }

void DialogueEditorWindow::_on_close_requested()
{
    if (current_window_id == ObjectID(get_instance_id()))
    {
        current_window_id = ObjectID();
    }
    queue_free();
}

// 🧱 Interface

void DialogueEditorWindow::_build_ui()
{
    Panel *background = memnew(Panel);
    background->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    add_child(background);
    MarginContainer *margin = memnew(MarginContainer);
    margin->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    const char *sides[] = {"margin_left", "margin_top", "margin_right", "margin_bottom"};
    for (const char *side : sides)
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
    layout_option = memnew(OptionButton);
    layout_option->add_item(U"TechTree →");
    layout_option->add_item(U"Arbre vertical ↓");
    layout_option->add_item(U"MindMap ↔");
    layout_option->connect("item_selected", callable_mp(this, &DialogueEditorWindow::_on_layout_selected));
    toolbar->add_child(layout_option);
    Button *fit = memnew(Button);
    fit->set_text(U"Tout afficher");
    fit->connect("pressed", callable_mp(this, &DialogueEditorWindow::_on_menu_id_pressed).bind(-1));
    toolbar->add_child(fit);
    Button *check = memnew(Button);
    check->set_text(U"Vérifier");
    check->set_tooltip_text(U"Identifiants en double, liens cassés, départ introuvable");
    check->connect("pressed", callable_mp(this, &DialogueEditorWindow::_on_validate_pressed));
    toolbar->add_child(check);
    Button *save = memnew(Button);
    save->set_text(U"💾 Enregistrer");
    save->set_tooltip_text(U"Enregistre le dialogue dans son fichier (.tres) ; un dialogue intégré à une scène s'enregistre avec la scène");
    save->connect("pressed", callable_mp(this, &DialogueEditorWindow::_on_save_pressed));
    toolbar->add_child(save);
    toolbar->add_child(memnew(VSeparator));
    follow_game_check = memnew(CheckBox);
    follow_game_check->set_text(U"🐞 Suivre le jeu");
    follow_game_check->set_pressed(true);
    toolbar->add_child(follow_game_check);
    remote_option = memnew(OptionButton);
    remote_option->set_custom_minimum_size(Vector2(220, 0));
    remote_option->set_clip_text(true);
    remote_option->connect("item_selected", callable_mp(this, &DialogueEditorWindow::_on_remote_selected));
    toolbar->add_child(remote_option);

    view = memnew(BehaviorGraphView);
    view->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    view->set_editable(true);
    view->set_show_history(true);
    view->set_placeholder_text(U"Dialogue vide : clic droit → Ajouter un nœud.");
    view->connect("node_selected", callable_mp(this, &DialogueEditorWindow::_on_node_selected));
    view->connect("node_context_menu_requested", callable_mp(this, &DialogueEditorWindow::_on_context_menu_requested));
    view->connect("link_requested", callable_mp(this, &DialogueEditorWindow::_on_link_requested));
    view->connect("delete_requested", callable_mp(this, &DialogueEditorWindow::_on_delete_requested));
    box->add_child(view);

    status_label = memnew(Label);
    status_label->set_modulate(Color(1, 1, 1, 0.7f));
    status_label->set_clip_text(true);
    box->add_child(status_label);

    context_menu = memnew(PopupMenu);
    context_menu->connect("id_pressed", callable_mp(this, &DialogueEditorWindow::_on_menu_id_pressed));
    add_child(context_menu);
    rename_dialog = memnew(ConfirmationDialog);
    rename_dialog->set_title(U"Renommer l'identifiant");
    rename_edit = memnew(LineEdit);
    rename_dialog->add_child(rename_edit);
    rename_dialog->register_text_enter(rename_edit);
    rename_dialog->connect("confirmed", callable_mp(this, &DialogueEditorWindow::_on_rename_confirmed));
    add_child(rename_dialog);
}

void DialogueEditorWindow::_set_dialogue(const Ref<DialogueResource> &p_dialogue)
{
    dialogue = p_dialogue;
    String name = dialogue->get_path().is_empty() ? String(U"(intégré)") : dialogue->get_path();
    title_label->set_text(String(U"💬 Dialogue : ") + name);
    set_title(String(U"Éditeur de dialogue — ") + name.get_file());
    last_signature = String();
    showing_remote = false;
    view->set_editable(true);
    _refresh(true);
    view->fit_to_view();
}

Ref<DialogueNode> DialogueEditorWindow::_node(const String &p_id) const { return dialogue.is_valid() ? dialogue->get_dialogue_node(p_id) : Ref<DialogueNode>(); }

// 🔄 Mise à jour

void DialogueEditorWindow::_refresh(bool p_force)
{
    if (dialogue.is_null() || showing_remote)
    {
        return;
    }
    Dictionary snapshot = dialogue->get_graph_snapshot();
    String signature = Variant(snapshot).stringify();
    if (p_force || signature != last_signature)
    {
        last_signature = signature;
        view->set_snapshot(snapshot);
    }
}

void DialogueEditorWindow::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        set_process(true);
    }
    else if (p_what == NOTIFICATION_PROCESS)
    {
        poll_accumulator += get_process_delta_time();
        if (poll_accumulator >= POLL_INTERVAL)
        {
            poll_accumulator = 0.0;
            _poll();
        }
    }
}

void DialogueEditorWindow::_poll()
{
    BehaviorDebuggerPlugin *debugger = BehaviorDebuggerPlugin::get_singleton();
    bool session = debugger && debugger->has_active_session();
    _update_remote_list();
    if (debugger && session && follow_game_check->is_pressed() && !remote_path.is_empty())
    {
        PackedStringArray watched;
        watched.push_back(remote_path);
        debugger->set_watched(watched);
        Dictionary snapshot = debugger->get_snapshot(remote_path);
        if (!snapshot.is_empty())
        {
            showing_remote = true;
            view->set_editable(false);
            view->set_snapshot(snapshot);
            status_label->set_text(String(U"🐞 Jeu en cours — « ") + remote_path + U" » en direct (lecture seule).");
            return;
        }
    }
    if (showing_remote)
    {
        showing_remote = false;
        view->set_editable(true);
        _refresh(true);
    }
    _refresh();
    status_label->set_text(U"✏️ Clic droit : ajouter, relier, départ, renommer, supprimer · Maj + glisser : relier deux nœuds · Clic : modifier dans l'inspecteur · Suppr : supprimer");
}

void DialogueEditorWindow::_update_remote_list()
{
    BehaviorDebuggerPlugin *debugger = BehaviorDebuggerPlugin::get_singleton();
    Array runners = debugger && debugger->has_active_session() ? debugger->get_runners() : Array();
    String signature = Variant(runners).stringify() + "|" + remote_path;
    if (signature == remote_signature)
    {
        return;
    }
    remote_signature = signature;
    remote_option->clear();
    remote_option->add_item(runners.is_empty() ? String(U"(jeu non lancé)") : String(U"— aucun DialogueSystem suivi —"));
    remote_option->set_item_metadata(0, String());
    int selected = 0;
    for (int i = 0; i < runners.size(); i++)
    {
        Array entry = runners[i];
        if (entry.size() < 3 || String(entry[1]) != "DialogueSystem")
        {
            continue;
        }
        String path = entry[0];
        remote_option->add_item(path);
        remote_option->set_item_metadata(remote_option->get_item_count() - 1, path);
        if (path == remote_path || (remote_path.is_empty() && selected == 0))
        {
            selected = remote_option->get_item_count() - 1;
            remote_path = path; // premier DialogueSystem du jeu choisi par défaut
        }
    }
    remote_option->select(selected);
}

void DialogueEditorWindow::_on_remote_selected(int p_index) { remote_path = remote_option->get_item_metadata(p_index); }

void DialogueEditorWindow::_on_layout_selected(int p_index) { view->set_layout_mode(BehaviorGraphView::LayoutMode(p_index)); }

void DialogueEditorWindow::_on_save_pressed()
{
    String path = dialogue.is_valid() ? dialogue->get_path() : String();
    if (path.is_empty() || path.contains("::"))
    {
        status_label->set_text(U"Ce dialogue est intégré à une scène ou une ressource : enregistrez-la (Ctrl+S).");
        return;
    }
    Error error = ResourceSaver::get_singleton()->save(dialogue, path);
    status_label->set_text(error == OK ? String(U"💾 Enregistré : ") + path : String(U"⚠ Échec de l'enregistrement de ") + path);
}

void DialogueEditorWindow::_on_validate_pressed()
{
    PackedStringArray issues = dialogue.is_valid() ? dialogue->validate() : PackedStringArray();
    status_label->set_text(issues.is_empty() ? String(U"✅ Aucun problème détecté.") : String(U"⚠ ") + String("  ·  ").join(issues));
}

// 🖱️ Vue et menus

void DialogueEditorWindow::_on_node_selected(const String &p_id)
{
    if (showing_remote || dialogue.is_null())
    {
        return;
    }
    Ref<DialogueNode> node = _node(p_id);
    EditorInterface::get_singleton()->inspect_object(node.is_valid() ? static_cast<Object *>(node.ptr()) : static_cast<Object *>(dialogue.ptr()), String(), true);
}

int DialogueEditorWindow::_add_action(PopupMenu *p_menu, const String &p_label, MenuAction p_action, const Variant &p_argument)
{
    int id = next_menu_id++;
    p_menu->add_item(p_label, id);
    Array entry;
    entry.push_back(p_action);
    entry.push_back(p_argument);
    menu_actions[id] = entry;
    return p_menu->get_item_count() - 1;
}

PopupMenu *DialogueEditorWindow::_types_submenu(MenuAction p_action)
{
    PopupMenu *submenu = memnew(PopupMenu);
    submenu->connect("id_pressed", callable_mp(this, &DialogueEditorWindow::_on_menu_id_pressed));
    for (const NodeType &type : NODE_TYPES)
    {
        _add_action(submenu, type.label, p_action, String(type.type));
    }
    return submenu;
}

void DialogueEditorWindow::_on_context_menu_requested(const String &p_id, const Vector2 &p_screen_position)
{
    if (showing_remote || dialogue.is_null())
    {
        return;
    }
    context_menu->clear(true);
    menu_actions.clear();
    next_menu_id = 0;
    context_id = p_id;
    Ref<DialogueNode> node = _node(p_id);

    context_menu->add_submenu_node_item(U"➕ Ajouter un nœud (non relié)", _types_submenu(ACTION_ADD));
    if (node.is_valid() && !Object::cast_to<DialogueEnd>(node.ptr()))
    {
        context_menu->add_submenu_node_item(U"➕ Ajouter après ce nœud", _types_submenu(ACTION_ADD_AFTER));
        PopupMenu *targets = memnew(PopupMenu);
        targets->connect("id_pressed", callable_mp(this, &DialogueEditorWindow::_on_menu_id_pressed));
        TypedArray<DialogueNode> nodes = dialogue->get_nodes();
        for (int i = 0; i < nodes.size(); i++)
        {
            Ref<DialogueNode> target = nodes[i];
            if (target.is_valid())
            {
                _add_action(targets, String(target->get_node_id()) + "  " + target->get_summary().left(40), ACTION_LINK, String(target->get_node_id()));
            }
        }
        context_menu->add_submenu_node_item(U"🔗 Relier à", targets);
        Array links = node->get_links();
        if (!links.is_empty())
        {
            PopupMenu *unlink = memnew(PopupMenu);
            unlink->connect("id_pressed", callable_mp(this, &DialogueEditorWindow::_on_menu_id_pressed));
            for (int i = 0; i < links.size(); i++)
            {
                Dictionary link = links[i];
                _add_action(unlink, String(U"→ ") + String(link["to"]) + "  " + String(link["label"]), ACTION_UNLINK, link["slot"]);
            }
            context_menu->add_submenu_node_item(U"✂️ Retirer un lien", unlink);
        }
    }
    if (node.is_valid())
    {
        context_menu->add_separator();
        int index = _add_action(context_menu, U"⭐ Définir comme départ", ACTION_SET_START);
        context_menu->set_item_disabled(index, dialogue->get_start_node() == node);
        _add_action(context_menu, U"✏️ Modifier (inspecteur)", ACTION_EDIT);
        _add_action(context_menu, U"Renommer l'identifiant…", ACTION_RENAME);
        _add_action(context_menu, U"🗑 Supprimer", ACTION_DELETE);
    }
    context_menu->add_separator();
    _add_action(context_menu, U"Tout afficher (F)", ACTION_FIT);
    context_menu->set_position(Vector2i(p_screen_position));
    context_menu->reset_size();
    context_menu->popup();
}

void DialogueEditorWindow::_on_menu_id_pressed(int p_id)
{
    if (p_id < 0)
    {
        view->fit_to_view();
        return;
    }
    if (!menu_actions.has(p_id))
    {
        return;
    }
    Array entry = menu_actions[p_id];
    Variant argument = entry[1];
    Ref<DialogueNode> node = _node(context_id);
    switch (MenuAction(int(entry[0])))
    {
    case ACTION_ADD:
        _add_node(argument, Ref<DialogueNode>());
        break;
    case ACTION_ADD_AFTER:
        _add_node(argument, node);
        break;
    case ACTION_LINK:
        _link(node, _node(argument));
        break;
    case ACTION_UNLINK:
        _unlink(node, argument);
        break;
    case ACTION_SET_START:
        _set_start(node);
        break;
    case ACTION_EDIT:
        _on_node_selected(context_id);
        break;
    case ACTION_RENAME:
        if (node.is_valid())
        {
            rename_edit->set_text(node->get_node_id());
            rename_dialog->popup_centered(Vector2i(320, 0));
            rename_edit->grab_focus();
            rename_edit->select_all();
        }
        break;
    case ACTION_DELETE:
        _delete(node);
        break;
    case ACTION_FIT:
        view->fit_to_view();
        break;
    }
}

void DialogueEditorWindow::_on_link_requested(const String &p_from, const String &p_to)
{
    if (!showing_remote)
    {
        _link(_node(p_from), _node(p_to));
    }
}

void DialogueEditorWindow::_on_delete_requested(const String &p_id)
{
    if (!showing_remote)
    {
        _delete(_node(p_id));
    }
}

void DialogueEditorWindow::_on_rename_confirmed() { _rename(_node(context_id), rename_edit->get_text().strip_edges()); }

// ✏️ Édition annulable

bool DialogueEditorWindow::_link_property(const Ref<DialogueNode> &p_node, int p_slot, Object *&r_object, StringName &r_property)
{
    if (Object::cast_to<DialogueLine>(p_node.ptr()) || Object::cast_to<DialogueAction>(p_node.ptr()))
    {
        r_object = p_node.ptr();
        r_property = "next";
        return true;
    }
    if (Object::cast_to<DialogueBranch>(p_node.ptr()))
    {
        r_object = p_node.ptr();
        r_property = p_slot == 1 ? "next_if_false" : "next_if_true";
        return true;
    }
    if (DialogueChoice *choice = Object::cast_to<DialogueChoice>(p_node.ptr()))
    {
        if (p_slot < 0)
        {
            r_object = choice;
            r_property = "fallback_next";
            return true;
        }
        if (p_slot < choice->get_options().size())
        {
            Ref<DialogueOption> option = choice->get_options()[p_slot];
            r_object = option.ptr();
            r_property = "next";
            return option.is_valid();
        }
    }
    return false;
}

Ref<DialogueNode> DialogueEditorWindow::_create_node(const String &p_type) const
{
    for (const NodeType &type : NODE_TYPES)
    {
        if (p_type == type.type)
        {
            Ref<DialogueNode> node = ClassDB::instantiate(type.type);
            if (node.is_valid())
            {
                node->set_node_id(dialogue->generate_id(type.prefix));
                if (DialogueLine *line = Object::cast_to<DialogueLine>(node.ptr()))
                {
                    line->set_text(U"…");
                }
            }
            return node;
        }
    }
    return Ref<DialogueNode>();
}

void DialogueEditorWindow::_add_node(const String &p_type, const Ref<DialogueNode> &p_after)
{
    Ref<DialogueNode> node = _create_node(p_type);
    if (node.is_null())
    {
        return;
    }
    Array before = dialogue->get_nodes().duplicate();
    Array after = before.duplicate();
    after.push_back(node);
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(String(U"Dialogue : ajouter ") + String(node->get_node_id()));
    ur->add_do_property(dialogue.ptr(), "nodes", after);
    ur->add_undo_property(dialogue.ptr(), "nodes", before);
    if (p_after.is_valid())
    {
        if (DialogueChoice *choice = Object::cast_to<DialogueChoice>(p_after.ptr()))
        {
            Ref<DialogueOption> option;
            option.instantiate();
            option->set_text(U"Nouvelle option");
            option->set_next(node->get_node_id());
            Array options_before = choice->get_options().duplicate();
            Array options_after = options_before.duplicate();
            options_after.push_back(option);
            ur->add_do_property(choice, "options", options_after);
            ur->add_undo_property(choice, "options", options_before);
        }
        else
        {
            Object *object = nullptr;
            StringName property;
            int slot = 0;
            if (DialogueBranch *branch = Object::cast_to<DialogueBranch>(p_after.ptr()))
            {
                slot = String(branch->get_next_if_true()).is_empty() || !String(branch->get_next_if_false()).is_empty() ? 0 : 1;
            }
            if (_link_property(p_after, slot, object, property))
            {
                StringName previous = object->get(property);
                // Insertion : le nouveau nœud reprend la suite de celui qu'on prolonge.
                if (Object::cast_to<DialogueLine>(node.ptr()) || Object::cast_to<DialogueAction>(node.ptr()))
                {
                    node->set("next", previous);
                }
                ur->add_do_property(object, property, node->get_node_id());
                ur->add_undo_property(object, property, previous);
            }
        }
    }
    ur->commit_action();
    _after_edit(node->get_node_id());
}

void DialogueEditorWindow::_link(const Ref<DialogueNode> &p_from, const Ref<DialogueNode> &p_to)
{
    if (p_from.is_null() || p_to.is_null() || Object::cast_to<DialogueEnd>(p_from.ptr()))
    {
        return;
    }
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(String(U"Dialogue : relier ") + String(p_from->get_node_id()) + U" → " + String(p_to->get_node_id()));
    if (DialogueChoice *choice = Object::cast_to<DialogueChoice>(p_from.ptr()))
    {
        Ref<DialogueOption> option;
        option.instantiate();
        option->set_text(p_to->get_summary().is_empty() ? String(p_to->get_node_id()) : p_to->get_summary().left(30));
        option->set_next(p_to->get_node_id());
        Array before = choice->get_options().duplicate();
        Array after = before.duplicate();
        after.push_back(option);
        ur->add_do_property(choice, "options", after);
        ur->add_undo_property(choice, "options", before);
    }
    else
    {
        int slot = 0;
        if (DialogueBranch *branch = Object::cast_to<DialogueBranch>(p_from.ptr()))
        {
            slot = String(branch->get_next_if_true()).is_empty() || !String(branch->get_next_if_false()).is_empty() ? 0 : 1;
        }
        Object *object = nullptr;
        StringName property;
        if (_link_property(p_from, slot, object, property))
        {
            ur->add_do_property(object, property, p_to->get_node_id());
            ur->add_undo_property(object, property, object->get(property));
        }
    }
    ur->commit_action();
    _after_edit(p_from->get_node_id());
}

void DialogueEditorWindow::_unlink(const Ref<DialogueNode> &p_from, int p_slot)
{
    if (p_from.is_null())
    {
        return;
    }
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(String(U"Dialogue : retirer un lien de ") + String(p_from->get_node_id()));
    DialogueChoice *choice = Object::cast_to<DialogueChoice>(p_from.ptr());
    if (choice && p_slot >= 0)
    {
        Array before = choice->get_options().duplicate();
        Array after = before.duplicate();
        after.remove_at(p_slot);
        ur->add_do_property(choice, "options", after);
        ur->add_undo_property(choice, "options", before);
    }
    else
    {
        Object *object = nullptr;
        StringName property;
        if (_link_property(p_from, p_slot, object, property))
        {
            ur->add_do_property(object, property, StringName());
            ur->add_undo_property(object, property, object->get(property));
        }
    }
    ur->commit_action();
    _after_edit(p_from->get_node_id());
}

void DialogueEditorWindow::_set_start(const Ref<DialogueNode> &p_node)
{
    if (p_node.is_null())
    {
        return;
    }
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(String(U"Dialogue : départ ") + String(p_node->get_node_id()));
    ur->add_do_property(dialogue.ptr(), "start_id", p_node->get_node_id());
    ur->add_undo_property(dialogue.ptr(), "start_id", dialogue->get_start_id());
    ur->commit_action();
    _after_edit(p_node->get_node_id());
}

void DialogueEditorWindow::_rename(const Ref<DialogueNode> &p_node, const StringName &p_id)
{
    if (p_node.is_null() || String(p_id).is_empty() || p_id == p_node->get_node_id())
    {
        return;
    }
    if (dialogue->has_dialogue_node(p_id))
    {
        status_label->set_text(String(U"⚠ L'identifiant « ") + String(p_id) + U" » existe déjà.");
        return;
    }
    StringName old_id = p_node->get_node_id();
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(String(U"Dialogue : renommer ") + String(old_id) + U" → " + String(p_id));
    ur->add_do_property(p_node.ptr(), "node_id", p_id);
    ur->add_undo_property(p_node.ptr(), "node_id", old_id);
    // Les liens entrants et le départ suivent le nouvel identifiant.
    TypedArray<DialogueNode> nodes = dialogue->get_nodes();
    for (int i = 0; i < nodes.size(); i++)
    {
        Ref<DialogueNode> source = nodes[i];
        Array links = source.is_valid() ? source->get_links() : Array();
        for (int l = 0; l < links.size(); l++)
        {
            Dictionary link = links[l];
            Object *object = nullptr;
            StringName property;
            if (StringName(String(link["to"])) == old_id && _link_property(source, link["slot"], object, property))
            {
                ur->add_do_property(object, property, p_id);
                ur->add_undo_property(object, property, old_id);
            }
        }
    }
    if (dialogue->get_start_id() == old_id)
    {
        ur->add_do_property(dialogue.ptr(), "start_id", p_id);
        ur->add_undo_property(dialogue.ptr(), "start_id", old_id);
    }
    ur->commit_action();
    _after_edit(p_id);
}

void DialogueEditorWindow::_delete(const Ref<DialogueNode> &p_node)
{
    if (p_node.is_null())
    {
        return;
    }
    StringName id = p_node->get_node_id();
    Array before = dialogue->get_nodes().duplicate();
    Array after = before.duplicate();
    after.erase(p_node);
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(String(U"Dialogue : supprimer ") + String(id));
    ur->add_do_property(dialogue.ptr(), "nodes", after);
    ur->add_undo_property(dialogue.ptr(), "nodes", before);
    // Liens entrants vidés (options de choix comprises).
    for (int i = 0; i < after.size(); i++)
    {
        Ref<DialogueNode> source = after[i];
        Array links = source.is_valid() ? source->get_links() : Array();
        for (int l = 0; l < links.size(); l++)
        {
            Dictionary link = links[l];
            Object *object = nullptr;
            StringName property;
            if (StringName(String(link["to"])) == id && _link_property(source, link["slot"], object, property))
            {
                ur->add_do_property(object, property, StringName());
                ur->add_undo_property(object, property, id);
            }
        }
    }
    if (dialogue->get_start_id() == id)
    {
        ur->add_do_property(dialogue.ptr(), "start_id", StringName());
        ur->add_undo_property(dialogue.ptr(), "start_id", id);
    }
    ur->commit_action();
    _after_edit(String());
}

void DialogueEditorWindow::_after_edit(const String &p_select)
{
    _refresh(true);
    if (!p_select.is_empty())
    {
        view->set_selected_id(p_select);
        _on_node_selected(p_select);
    }
}
