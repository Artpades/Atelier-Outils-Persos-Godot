#include "behavior_editor_window.hpp"
#include "behavior_editor_plugin.hpp"

#include "../../🌿Arbre de Comportement/behavior_tree.hpp"
#include "../../🌿Arbre de Comportement/bt_composites.hpp"
#include "../../🌿Arbre de Comportement/bt_decorators.hpp"
#include "../../🌿Arbre de Comportement/bt_leaves.hpp"
#include "../../🤖HFSM/state_machine.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

ObjectID BehaviorEditorWindow::current_window_id;

namespace
{
constexpr double POLL_INTERVAL = 0.1;

EditorUndoRedoManager *undo_redo() { return EditorInterface::get_singleton()->get_editor_undo_redo(); }

Node *edited_root() { return EditorInterface::get_singleton()->get_edited_scene_root(); }

/// Catégorie d'un type pour le classement des menus.
String category_of(const StringName &p_native)
{
    if (ClassDB::is_parent_class(p_native, "BTComposite"))
    {
        return U"Composites";
    }
    if (ClassDB::is_parent_class(p_native, "BTDecorator"))
    {
        return U"Décorateurs";
    }
    if (ClassDB::is_parent_class(p_native, "BTCondition"))
    {
        return U"Conditions";
    }
    if (ClassDB::is_parent_class(p_native, "BTNode"))
    {
        return U"Actions";
    }
    return U"États";
}

/// Tri des types proposés : par catégorie, puis par nom.
bool type_less(const Variant &p_a, const Variant &p_b)
{
    Dictionary a = p_a;
    Dictionary b = p_b;
    if (String(a["category"]) != String(b["category"]))
    {
        return String(a["category"]) < String(b["category"]);
    }
    return String(a["label"]).naturalnocasecmp_to(String(b["label"])) < 0;
}

/// Classes de base non instanciables seules (« virtuelles ») : jamais proposées.
bool is_hidden_type(const String &p_name)
{
    return p_name == "BTNode" || p_name == "BTComposite" || p_name == "BTOrderedComposite" || p_name == "BTDecorator";
}
} // namespace

// 🪟 Ouverture / fermeture

void BehaviorEditorWindow::open_for(Node *p_node)
{
    EditorInterface *editor = EditorInterface::get_singleton();
    Node *runner = BehaviorGraphView::resolve_runner(p_node);
    if (!editor || !editor->get_base_control() || !runner)
    {
        return;
    }
    if (BehaviorEditorWindow *window = get_current())
    {
        window->_set_target(runner);
        window->view->set_selected_id(window->_id_of(p_node));
        window->grab_focus();
        return;
    }

    BehaviorEditorWindow *window = memnew(BehaviorEditorWindow);
    window->set_title(U"Éditeur de comportement");
    window->set_size(Vector2i(1200, 760));
    window->set_min_size(Vector2i(640, 400));
    window->set_theme(editor->get_editor_theme());
    window->set_visible(false);
    window->connect("close_requested", callable_mp(window, &BehaviorEditorWindow::_on_close_requested));
    window->_build_ui();
    editor->get_base_control()->add_child(window);
    current_window_id = ObjectID(window->get_instance_id());
    window->_set_target(runner);
    window->view->set_selected_id(window->_id_of(p_node));
    window->popup_centered();
}

void BehaviorEditorWindow::close_current()
{
    if (BehaviorEditorWindow *window = get_current())
    {
        window->queue_free();
    }
    current_window_id = ObjectID();
    if (BehaviorDebuggerPlugin *debugger = BehaviorDebuggerPlugin::get_singleton())
    {
        debugger->set_watched(PackedStringArray());
    }
}

BehaviorEditorWindow *BehaviorEditorWindow::get_current() { return Object::cast_to<BehaviorEditorWindow>(ObjectDB::get_instance(current_window_id)); }

void BehaviorEditorWindow::_on_close_requested()
{
    if (current_window_id == ObjectID(get_instance_id()))
    {
        close_current();
    }
    else
    {
        queue_free();
    }
}

// 🧱 Interface

void BehaviorEditorWindow::_build_ui()
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
    target_label = memnew(Label);
    target_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    target_label->set_clip_text(true);
    toolbar->add_child(target_label);

    layout_option = memnew(OptionButton);
    layout_option->add_item(U"TechTree →");
    layout_option->add_item(U"Arbre vertical ↓");
    layout_option->add_item(U"MindMap ↔");
    layout_option->set_tooltip_text(U"Disposition du graphe");
    layout_option->connect("item_selected", callable_mp(this, &BehaviorEditorWindow::_on_layout_selected));
    toolbar->add_child(layout_option);

    Button *fit = memnew(Button);
    fit->set_text(U"Tout afficher");
    fit->set_tooltip_text(U"Cadrer tout le graphe (touche F)");
    fit->connect("pressed", callable_mp(this, &BehaviorEditorWindow::_on_menu_id_pressed).bind(-1));
    toolbar->add_child(fit);

    CheckBox *labels = memnew(CheckBox);
    labels->set_text(U"Libellés");
    labels->set_tooltip_text(U"Afficher l'événement et la condition de chaque transition");
    labels->set_pressed(true);
    labels->connect("toggled", callable_mp(this, &BehaviorEditorWindow::_on_labels_toggled));
    toolbar->add_child(labels);

    toolbar->add_child(memnew(VSeparator));
    follow_game_check = memnew(CheckBox);
    follow_game_check->set_text(U"🐞 Suivre le jeu");
    follow_game_check->set_tooltip_text(U"Quand le jeu tourne (lancé depuis l'éditeur), afficher l'état en direct du comportement choisi à droite");
    follow_game_check->set_pressed(true);
    follow_game_check->connect("toggled", callable_mp(this, &BehaviorEditorWindow::_on_follow_toggled));
    toolbar->add_child(follow_game_check);
    remote_option = memnew(OptionButton);
    remote_option->set_custom_minimum_size(Vector2(240, 0));
    remote_option->set_clip_text(true);
    remote_option->set_tooltip_text(U"Comportement du jeu à afficher (les instances créées en jeu y apparaissent aussi)");
    remote_option->connect("item_selected", callable_mp(this, &BehaviorEditorWindow::_on_remote_selected));
    toolbar->add_child(remote_option);
    pin_check = memnew(CheckBox);
    pin_check->set_text(U"📌 Épingler");
    pin_check->set_tooltip_text(U"Ne pas suivre la sélection de l'éditeur");
    toolbar->add_child(pin_check);

    view = memnew(BehaviorGraphView);
    view->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    view->set_editable(true);
    view->connect("node_selected", callable_mp(this, &BehaviorEditorWindow::_on_node_selected));
    view->connect("node_activated", callable_mp(this, &BehaviorEditorWindow::_on_node_activated));
    view->connect("node_context_menu_requested", callable_mp(this, &BehaviorEditorWindow::_on_context_menu_requested));
    view->connect("node_dropped", callable_mp(this, &BehaviorEditorWindow::_on_node_dropped));
    view->connect("link_requested", callable_mp(this, &BehaviorEditorWindow::_on_link_requested));
    view->connect("delete_requested", callable_mp(this, &BehaviorEditorWindow::_on_delete_requested));
    box->add_child(view);

    status_label = memnew(Label);
    status_label->set_modulate(Color(1, 1, 1, 0.7f));
    status_label->set_clip_text(true);
    box->add_child(status_label);

    context_menu = memnew(PopupMenu);
    context_menu->connect("id_pressed", callable_mp(this, &BehaviorEditorWindow::_on_menu_id_pressed));
    add_child(context_menu);

    rename_dialog = memnew(ConfirmationDialog);
    rename_dialog->set_title(U"Renommer");
    rename_edit = memnew(LineEdit);
    rename_dialog->add_child(rename_edit);
    rename_dialog->register_text_enter(rename_edit);
    rename_dialog->connect("confirmed", callable_mp(this, &BehaviorEditorWindow::_on_rename_confirmed));
    add_child(rename_dialog);

    _update_remote_list();
}

// 🎯 Cible

void BehaviorEditorWindow::_set_target(Node *p_runner)
{
    target_id = p_runner ? ObjectID(p_runner->get_instance_id()) : ObjectID();
    remote_path = String();
    remote_signature = String(); // la liste est reconstruite pour resélectionner la bonne entrée
    view->set_target(p_runner);
    if (showing_remote)
    {
        showing_remote = false;
        view->set_live_update(true);
        view->set_editable(true);
    }
    view->refresh();
    if (!p_runner)
    {
        target_label->set_text(U"Aucune cible");
        return;
    }
    bool hfsm = Object::cast_to<StateMachine>(p_runner) != nullptr;
    target_label->set_text(String(hfsm ? U"🤖 Machine à états : " : U"🌿 Arbre de comportement : ") + String(p_runner->get_name()) + U"   (" + _edited_path() + ")");
    set_title(U"Éditeur de comportement — " + String(p_runner->get_name()));
    _poll();
}

Node *BehaviorEditorWindow::_target() const { return Object::cast_to<Node>(ObjectDB::get_instance(target_id)); }

Node *BehaviorEditorWindow::_node_from_id(const String &p_id) const
{
    Node *target = _target();
    if (!target || p_id.is_empty())
    {
        return nullptr;
    }
    return p_id == "." ? target : target->get_node_or_null(NodePath(p_id));
}

String BehaviorEditorWindow::_id_of(Node *p_node) const
{
    Node *target = _target();
    if (!target || !p_node || (p_node != target && !target->is_ancestor_of(p_node)))
    {
        return String();
    }
    return String(target->get_path_to(p_node));
}

String BehaviorEditorWindow::_edited_path() const
{
    Node *target = _target();
    Node *root = edited_root();
    if (!target || !root || (root != target && !root->is_ancestor_of(target)))
    {
        return String();
    }
    return String(root->get_path_to(target));
}

void BehaviorEditorWindow::on_editor_selection(Node *p_node)
{
    Node *runner = BehaviorGraphView::resolve_runner(p_node);
    // Un DialogueSystem s'ouvre dans l'éditeur de dialogue, pas ici.
    if (!runner || (!Object::cast_to<StateMachine>(runner) && !Object::cast_to<BehaviorTree>(runner)))
    {
        return;
    }
    if (runner != _target())
    {
        if (pin_check->is_pressed())
        {
            return;
        }
        _set_target(runner);
    }
    view->set_selected_id(_id_of(p_node));
}

// 🔄 Mise à jour (édition / débogage du jeu)

void BehaviorEditorWindow::_notification(int p_what)
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

void BehaviorEditorWindow::_poll()
{
    BehaviorDebuggerPlugin *debugger = BehaviorDebuggerPlugin::get_singleton();
    bool session = debugger && debugger->has_active_session();
    String path = remote_path.is_empty() ? _edited_path() : remote_path;
    if (debugger)
    {
        PackedStringArray watched;
        if (session && follow_game_check->is_pressed() && !path.is_empty())
        {
            watched.push_back(path);
        }
        debugger->set_watched(watched);
    }
    _update_remote_list();

    Dictionary snapshot = session && debugger && follow_game_check->is_pressed() && !path.is_empty() ? debugger->get_snapshot(path) : Dictionary();
    if (!snapshot.is_empty())
    {
        if (!showing_remote)
        {
            showing_remote = true;
            view->set_live_update(false);
            view->set_editable(false);
        }
        view->set_snapshot(snapshot);
        status_label->set_text(U"🐞 Jeu en cours — « " + path + U" » en direct (lecture seule). Décochez « Suivre le jeu » pour éditer.");
        return;
    }

    if (showing_remote)
    {
        showing_remote = false;
        view->set_live_update(true);
        view->set_editable(true);
        view->refresh();
    }
    _heal_paths();
    if (!_target())
    {
        status_label->set_text(U"Aucune cible : sélectionnez une StateMachine ou un BehaviorTree de la scène et cliquez sur « Éditeur de comportement » dans l'inspecteur.");
    }
    else if (session && follow_game_check->is_pressed())
    {
        status_label->set_text(U"🐞 Jeu en cours : en attente des données de « " + path + U" » (ce comportement est-il dans la scène lancée ?)");
    }
    else
    {
        status_label->set_text(U"✏️ Clic droit : actions · Glisser un nœud sur un autre : le déplacer · Maj + glisser : transition · Suppr : supprimer · Double-clic : script");
    }
}

void BehaviorEditorWindow::_update_remote_list()
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
    remote_option->add_item(runners.is_empty() ? String(U"(jeu non lancé)") : String(U"Même chemin que la scène éditée"));
    remote_option->set_item_metadata(0, String());
    int selected = 0;
    for (int i = 0; i < runners.size(); i++)
    {
        Array entry = runners[i];
        if (entry.size() < 3)
        {
            continue;
        }
        String path = entry[0];
        remote_option->add_item(path + "  (" + String(entry[1]) + ")");
        int index = remote_option->get_item_count() - 1;
        remote_option->set_item_metadata(index, path);
        if (path == remote_path)
        {
            selected = index;
        }
    }
    remote_option->select(selected);
}

void BehaviorEditorWindow::_heal_paths()
{
    // Après un renommage ou un déplacement (ici ou dans l'arbre de scène), les chemins des transitions et
    // des états initiaux ne désignent plus leur cible : ils sont recalculés vers la dernière cible connue.
    StateMachine *machine = Object::cast_to<StateMachine>(_target());
    if (!machine)
    {
        return;
    }
    TypedArray<Node> states = machine->find_children("*", "State", true, false);
    states.push_back(machine);
    for (int i = 0; i < states.size(); i++)
    {
        State *state = Object::cast_to<State>(states[i]);
        TypedArray<StateTransition> transitions = state->get_transitions();
        for (int t = 0; t < transitions.size(); t++)
        {
            Ref<StateTransition> transition = transitions[t];
            if (transition.is_null() || transition->get_target_state().is_empty() || transition->resolve_target(state))
            {
                continue;
            }
            State *last = transition->get_last_resolved_target();
            if (last && (last == machine || machine->is_ancestor_of(last)))
            {
                transition->set_target_state(state->get_path_to(last));
            }
        }
        if (!state->get_initial_state().is_empty())
        {
            State *initial = Object::cast_to<State>(state->get_node_or_null(state->get_initial_state()));
            State *last = state->get_last_resolved_initial_state();
            if ((!initial || initial->get_parent() != state) && last && last->get_parent() == state)
            {
                state->set_initial_state(state->get_path_to(last));
            }
        }
    }
}

// 🎛️ Barre d'outils

void BehaviorEditorWindow::_on_layout_selected(int p_index) { view->set_layout_mode(BehaviorGraphView::LayoutMode(p_index)); }

void BehaviorEditorWindow::_on_labels_toggled(bool p_pressed) { view->set_show_transition_labels(p_pressed); }

void BehaviorEditorWindow::_on_follow_toggled(bool /*p_pressed*/) { _poll(); }

void BehaviorEditorWindow::_on_remote_selected(int p_index)
{
    remote_path = remote_option->get_item_metadata(p_index);
    _poll();
}

// 🖱️ Vue

void BehaviorEditorWindow::_on_node_selected(const String &p_id)
{
    // En débogage, le même chemin désigne le nœud correspondant de la scène éditée.
    Node *node = _node_from_id(p_id);
    if (!node)
    {
        return;
    }
    EditorInterface *editor = EditorInterface::get_singleton();
    editor->get_selection()->clear();
    editor->get_selection()->add_node(node);
    editor->edit_node(node);
}

void BehaviorEditorWindow::_on_node_activated(const String &p_id)
{
    Node *node = _node_from_id(p_id);
    Ref<Script> script = node ? Ref<Script>(node->get_script()) : Ref<Script>();
    if (script.is_valid())
    {
        EditorInterface::get_singleton()->edit_script(script);
    }
    else
    {
        view->center_on(p_id);
    }
}

int BehaviorEditorWindow::_add_action(PopupMenu *p_menu, const String &p_label, MenuAction p_action, const Variant &p_argument)
{
    int id = next_menu_id++;
    p_menu->add_item(p_label, id);
    Array entry;
    entry.push_back(p_action);
    entry.push_back(p_argument);
    menu_actions[id] = entry;
    return p_menu->get_item_count() - 1;
}

Array BehaviorEditorWindow::_creatable_types(const StringName &p_base) const
{
    Array types;
    PackedStringArray natives = ClassDB::get_inheriters_from_class(p_base);
    natives.push_back(p_base);
    for (const String &name : natives)
    {
        if (is_hidden_type(name) || !ClassDB::can_instantiate(name))
        {
            continue;
        }
        Dictionary type;
        type["label"] = name;
        type["name"] = name;
        type["native"] = name;
        type["script"] = String();
        type["category"] = category_of(name);
        types.push_back(type);
    }

    // Classes de script (class_name) qui héritent de la base
    TypedArray<Dictionary> globals = ProjectSettings::get_singleton()->get_global_class_list();
    Dictionary base_of;
    for (int i = 0; i < globals.size(); i++)
    {
        Dictionary entry = globals[i];
        base_of[entry["class"]] = entry["base"];
    }
    for (int i = 0; i < globals.size(); i++)
    {
        Dictionary entry = globals[i];
        StringName native = entry["base"];
        for (int guard = 0; guard < 32 && !ClassDB::class_exists(native) && base_of.has(native); guard++)
        {
            native = base_of[native];
        }
        if (!ClassDB::class_exists(native) || !ClassDB::is_parent_class(native, p_base))
        {
            continue;
        }
        Dictionary type;
        type["label"] = String(entry["class"]) + U" 📜";
        type["name"] = entry["class"];
        type["native"] = native;
        type["script"] = entry["path"];
        type["category"] = category_of(native);
        types.push_back(type);
    }
    types.sort_custom(callable_mp_static(&type_less));
    return types;
}

PopupMenu *BehaviorEditorWindow::_types_submenu(const StringName &p_base, MenuAction p_action, bool p_containers_only)
{
    PopupMenu *submenu = memnew(PopupMenu);
    submenu->connect("id_pressed", callable_mp(this, &BehaviorEditorWindow::_on_menu_id_pressed));
    Array types = _creatable_types(p_base);
    String category;
    for (int i = 0; i < types.size(); i++)
    {
        Dictionary type = types[i];
        String type_category = type["category"];
        if (p_containers_only && type_category != U"Composites" && type_category != U"Décorateurs")
        {
            continue;
        }
        if (type_category != category)
        {
            category = type_category;
            submenu->add_separator(category);
        }
        _add_action(submenu, type["label"], p_action, type);
    }
    return submenu;
}

bool BehaviorEditorWindow::_accepts_children(Node *p_node) const
{
    if (BehaviorTree *tree = Object::cast_to<BehaviorTree>(p_node))
    {
        return tree->get_root_bt_node() == nullptr;
    }
    if (BTDecorator *decorator = Object::cast_to<BTDecorator>(p_node))
    {
        return decorator->get_bt_child_count() == 0;
    }
    return Object::cast_to<BTComposite>(p_node) || Object::cast_to<State>(p_node);
}

void BehaviorEditorWindow::_on_context_menu_requested(const String &p_id, const Vector2 &p_screen_position)
{
    context_menu->clear(true);
    menu_actions.clear();
    next_menu_id = 0;
    context_id = p_id;

    Node *node = showing_remote ? nullptr : _node_from_id(p_id);
    Node *target = _target();
    StateMachine *machine = Object::cast_to<StateMachine>(target);
    if (node && machine)
    {
        State *state = Object::cast_to<State>(node);
        context_menu->add_submenu_node_item(U"➕ Ajouter un sous-état", _types_submenu("State", ACTION_ADD_CHILD));
        if (state && state != machine)
        {
            int index = _add_action(context_menu, U"⭐ Définir comme état initial", ACTION_SET_INITIAL);
            State *parent = state->get_parent_state();
            context_menu->set_item_disabled(index, parent && parent->get_initial_substate() == state && !parent->get_remember_last_substate());
        }
        // Transitions vers chaque état de la machine (y compris lui-même)
        PopupMenu *targets = memnew(PopupMenu);
        targets->connect("id_pressed", callable_mp(this, &BehaviorEditorWindow::_on_menu_id_pressed));
        TypedArray<Node> states = machine->find_children("*", "State", true, false);
        for (int i = 0; i < states.size(); i++)
        {
            State *candidate = Object::cast_to<State>(states[i]);
            _add_action(targets, String(machine->get_path_to(candidate)), ACTION_ADD_TRANSITION, Variant(candidate));
        }
        context_menu->add_submenu_node_item(U"➡️ Ajouter une transition vers", targets);
        TypedArray<StateTransition> transitions = state ? state->get_transitions() : TypedArray<StateTransition>();
        if (!transitions.is_empty())
        {
            PopupMenu *edit = memnew(PopupMenu);
            edit->connect("id_pressed", callable_mp(this, &BehaviorEditorWindow::_on_menu_id_pressed));
            PopupMenu *remove = memnew(PopupMenu);
            remove->connect("id_pressed", callable_mp(this, &BehaviorEditorWindow::_on_menu_id_pressed));
            for (int i = 0; i < transitions.size(); i++)
            {
                Ref<StateTransition> transition = transitions[i];
                String label = transition.is_valid() ? U"→ " + String(transition->get_target_state()) + "  " + transition->get_label() : String(U"(vide)");
                _add_action(edit, label, ACTION_EDIT_TRANSITION, i);
                _add_action(remove, label, ACTION_REMOVE_TRANSITION, i);
            }
            context_menu->add_submenu_node_item(U"✏️ Modifier une transition", edit);
            context_menu->add_submenu_node_item(U"✖ Supprimer une transition", remove);
        }
    }
    else if (node && Object::cast_to<BehaviorTree>(target))
    {
        if (_accepts_children(node))
        {
            context_menu->add_submenu_node_item(Object::cast_to<BehaviorTree>(node) ? U"➕ Ajouter la racine" : U"➕ Ajouter un enfant", _types_submenu("BTNode", ACTION_ADD_CHILD));
        }
        if (node != target)
        {
            context_menu->add_submenu_node_item(U"◇ Envelopper dans", _types_submenu("BTNode", ACTION_WRAP, true));
        }
    }
    if (node && node != target)
    {
        context_menu->add_separator();
        _add_action(context_menu, U"Renommer…", ACTION_RENAME);
        _add_action(context_menu, U"⬆ Monter", ACTION_MOVE_UP);
        _add_action(context_menu, U"⬇ Descendre", ACTION_MOVE_DOWN);
        _add_action(context_menu, U"🗑 Supprimer", ACTION_DELETE);
    }
    if (node && Ref<Script>(node->get_script()).is_valid())
    {
        _add_action(context_menu, U"📜 Ouvrir le script", ACTION_OPEN_SCRIPT);
    }
    context_menu->add_separator();
    _add_action(context_menu, U"Tout afficher (F)", ACTION_FIT);

    context_menu->set_position(Vector2i(p_screen_position));
    context_menu->reset_size();
    context_menu->popup();
}

void BehaviorEditorWindow::_on_menu_id_pressed(int p_id)
{
    if (p_id < 0)
    {
        view->fit_to_view(); // bouton « Tout afficher »
        return;
    }
    if (!menu_actions.has(p_id))
    {
        return;
    }
    Array entry = menu_actions[p_id];
    MenuAction action = MenuAction(int(entry[0]));
    Variant argument = entry[1];
    Node *node = _node_from_id(context_id);
    State *state = Object::cast_to<State>(node);

    switch (action)
    {
    case ACTION_ADD_CHILD:
        if (node)
        {
            _add_child(node, argument);
        }
        break;
    case ACTION_WRAP:
        if (node)
        {
            _wrap(node, argument);
        }
        break;
    case ACTION_RENAME:
        if (node)
        {
            rename_edit->set_text(node->get_name());
            rename_dialog->popup_centered(Vector2i(320, 0));
            rename_edit->grab_focus();
            rename_edit->select_all();
        }
        break;
    case ACTION_DELETE:
        _delete(node);
        break;
    case ACTION_MOVE_UP:
        _move(node, -1);
        break;
    case ACTION_MOVE_DOWN:
        _move(node, 1);
        break;
    case ACTION_SET_INITIAL:
        _set_initial(state);
        break;
    case ACTION_ADD_TRANSITION:
        _add_transition(state, Object::cast_to<State>(argument));
        break;
    case ACTION_EDIT_TRANSITION:
        if (state && int(argument) < state->get_transitions().size())
        {
            Ref<StateTransition> transition = state->get_transitions()[int(argument)];
            if (transition.is_valid())
            {
                EditorInterface::get_singleton()->inspect_object(transition.ptr(), String(), true);
            }
        }
        break;
    case ACTION_REMOVE_TRANSITION:
        _remove_transition(state, argument);
        break;
    case ACTION_OPEN_SCRIPT:
        if (node)
        {
            EditorInterface::get_singleton()->edit_script(node->get_script());
        }
        break;
    case ACTION_FIT:
        view->fit_to_view();
        break;
    }
}

void BehaviorEditorWindow::_on_node_dropped(const String &p_id, const String &p_target_id)
{
    if (showing_remote)
    {
        return;
    }
    Node *node = _node_from_id(p_id);
    Node *new_parent = _node_from_id(p_target_id);
    Node *target = _target();
    if (!node || !new_parent || node == target || node == new_parent || node->is_ancestor_of(new_parent) || node->get_parent() == new_parent)
    {
        return;
    }
    bool valid = Object::cast_to<StateMachine>(target) ? Object::cast_to<State>(new_parent) != nullptr : _accepts_children(new_parent);
    if (!valid)
    {
        status_label->set_text(U"⚠ « " + String(new_parent->get_name()) + U" » ne peut pas recevoir d'enfant.");
        return;
    }
    _reparent(node, new_parent);
}

void BehaviorEditorWindow::_on_link_requested(const String &p_from_id, const String &p_to_id)
{
    if (showing_remote)
    {
        return;
    }
    State *from = Object::cast_to<State>(_node_from_id(p_from_id));
    State *to = Object::cast_to<State>(_node_from_id(p_to_id));
    if (from && to && to != _target())
    {
        _add_transition(from, to);
    }
}

void BehaviorEditorWindow::_on_delete_requested(const String &p_id)
{
    if (!showing_remote)
    {
        _delete(_node_from_id(p_id));
    }
}

void BehaviorEditorWindow::_on_rename_confirmed()
{
    Node *node = _node_from_id(context_id);
    String name = rename_edit->get_text().strip_edges().validate_node_name();
    if (!node || name.is_empty() || name == String(node->get_name()))
    {
        return;
    }
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(U"Comportement : renommer " + String(node->get_name()));
    ur->add_do_method(node, "set_name", name);
    ur->add_undo_method(node, "set_name", node->get_name());
    ur->commit_action();
    _after_edit(node);
}

// ✏️ Édition annulable

Node *BehaviorEditorWindow::_instantiate(const Dictionary &p_type) const
{
    Node *node = Object::cast_to<Node>(ClassDB::instantiate(StringName(p_type.get("native", ""))));
    if (!node)
    {
        return nullptr;
    }
    String script_path = p_type.get("script", "");
    if (!script_path.is_empty())
    {
        Ref<Script> script = ResourceLoader::get_singleton()->load(script_path);
        if (script.is_valid())
        {
            node->set_script(script);
        }
    }
    node->set_name(String(p_type.get("name", node->get_class())));
    return node;
}

void BehaviorEditorWindow::_add_child(Node *p_parent, const Dictionary &p_type)
{
    Node *root = edited_root();
    Node *child = _instantiate(p_type);
    if (!child || !root)
    {
        if (child)
        {
            memdelete(child);
        }
        return;
    }
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(U"Comportement : ajouter " + String(child->get_name()));
    ur->add_do_method(p_parent, "add_child", child, true);
    ur->add_do_method(child, "set_owner", root);
    ur->add_do_reference(child);
    ur->add_undo_method(p_parent, "remove_child", child);
    ur->commit_action();
    _after_edit(child);
}

void BehaviorEditorWindow::_wrap(Node *p_node, const Dictionary &p_type)
{
    Node *root = edited_root();
    Node *parent = p_node->get_parent();
    Node *wrapper = _instantiate(p_type);
    if (!wrapper || !root || !parent)
    {
        if (wrapper)
        {
            memdelete(wrapper);
        }
        return;
    }
    int index = p_node->get_index();
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(U"Comportement : envelopper " + String(p_node->get_name()));
    ur->add_do_method(parent, "add_child", wrapper, true);
    ur->add_do_method(parent, "move_child", wrapper, index);
    ur->add_do_method(wrapper, "set_owner", root);
    ur->add_do_method(p_node, "reparent", wrapper, false);
    ur->add_do_reference(wrapper);
    ur->add_undo_method(p_node, "reparent", parent, false);
    ur->add_undo_method(parent, "move_child", p_node, index);
    ur->add_undo_method(parent, "remove_child", wrapper);
    ur->commit_action();
    _after_edit(wrapper);
}

void BehaviorEditorWindow::_delete(Node *p_node)
{
    Node *root = edited_root();
    if (!p_node || p_node == _target() || !p_node->get_parent() || !root)
    {
        return;
    }
    Node *parent = p_node->get_parent();
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(U"Comportement : supprimer " + String(p_node->get_name()));
    ur->add_do_method(parent, "remove_child", p_node);
    ur->add_undo_method(parent, "add_child", p_node, true);
    ur->add_undo_method(parent, "move_child", p_node, p_node->get_index());
    // Le retrait efface le propriétaire de toute la branche : il est rétabli à l'annulation.
    ur->add_undo_method(p_node, "set_owner", root);
    TypedArray<Node> descendants = p_node->find_children("*", "", true, false);
    for (int i = 0; i < descendants.size(); i++)
    {
        Node *descendant = Object::cast_to<Node>(descendants[i]);
        if (descendant->get_owner() == root)
        {
            ur->add_undo_method(descendant, "set_owner", root);
        }
    }
    ur->add_undo_reference(p_node);
    ur->commit_action();
    EditorInterface::get_singleton()->get_selection()->clear();
    _after_edit(parent);
}

void BehaviorEditorWindow::_move(Node *p_node, int p_offset)
{
    if (!p_node || p_node == _target() || !p_node->get_parent())
    {
        return;
    }
    int index = p_node->get_index();
    int destination = CLAMP(index + p_offset, 0, p_node->get_parent()->get_child_count() - 1);
    if (destination == index)
    {
        return;
    }
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(U"Comportement : déplacer " + String(p_node->get_name()));
    ur->add_do_method(p_node->get_parent(), "move_child", p_node, destination);
    ur->add_undo_method(p_node->get_parent(), "move_child", p_node, index);
    ur->commit_action();
    _after_edit(p_node);
}

void BehaviorEditorWindow::_reparent(Node *p_node, Node *p_new_parent)
{
    Node *old_parent = p_node->get_parent();
    int index = p_node->get_index();
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(U"Comportement : déplacer " + String(p_node->get_name()) + U" sous " + String(p_new_parent->get_name()));
    // reparent conserve le propriétaire (la racine de la scène) du nœud et de sa branche.
    ur->add_do_method(p_node, "reparent", p_new_parent, false);
    ur->add_undo_method(p_node, "reparent", old_parent, false);
    ur->add_undo_method(old_parent, "move_child", p_node, index);
    ur->commit_action();
    _after_edit(p_node);
}

void BehaviorEditorWindow::_set_initial(State *p_state)
{
    State *parent = p_state ? p_state->get_parent_state() : nullptr;
    if (!parent)
    {
        return;
    }
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(U"Comportement : état initial " + String(p_state->get_name()));
    ur->add_do_method(parent, "set_initial_state", parent->get_path_to(p_state));
    ur->add_undo_method(parent, "set_initial_state", parent->get_initial_state());
    ur->commit_action();
    _after_edit(p_state);
}

void BehaviorEditorWindow::_add_transition(State *p_from, State *p_to)
{
    if (!p_from || !p_to)
    {
        return;
    }
    Ref<StateTransition> transition;
    transition.instantiate();
    transition->set_target_state(p_from->get_path_to(p_to));
    // Copies : le tableau rendu par le getter est partagé avec l'état.
    Array before = p_from->get_transitions().duplicate();
    Array after = before.duplicate();
    after.push_back(transition);
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(U"Comportement : transition " + String(p_from->get_name()) + U" → " + String(p_to->get_name()));
    ur->add_do_method(p_from, "set_transitions", after);
    ur->add_undo_method(p_from, "set_transitions", before);
    ur->commit_action();
    _after_edit(p_from);
    // La transition s'ouvre dans l'inspecteur pour régler son événement / sa condition.
    EditorInterface::get_singleton()->inspect_object(transition.ptr(), String(), true);
}

void BehaviorEditorWindow::_remove_transition(State *p_from, int p_index)
{
    if (!p_from || p_index < 0 || p_index >= p_from->get_transitions().size())
    {
        return;
    }
    Array before = p_from->get_transitions().duplicate();
    Array after = before.duplicate();
    after.remove_at(p_index);
    EditorUndoRedoManager *ur = undo_redo();
    ur->create_action(U"Comportement : supprimer une transition de " + String(p_from->get_name()));
    ur->add_do_method(p_from, "set_transitions", after);
    ur->add_undo_method(p_from, "set_transitions", before);
    ur->commit_action();
    _after_edit(p_from);
}

void BehaviorEditorWindow::_after_edit(Node *p_select)
{
    view->refresh();
    if (p_select && p_select->is_inside_tree())
    {
        view->set_selected_id(_id_of(p_select));
        EditorInterface::get_singleton()->edit_node(p_select);
    }
}
