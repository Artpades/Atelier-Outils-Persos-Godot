#include "behavior_editor_plugin.hpp"
#include "behavior_editor_window.hpp"

#include "../../🌿Arbre de Comportement/behavior_tree.hpp"
#include "../../🤖HFSM/state_machine.hpp"
#include "../../../💬Système de Dialogue/dialogue_system.hpp"
#include "../../../💬Système de Dialogue/🧑‍💻Éditeur/dialogue_editor_window.hpp"
#include "../../../🧑Personnage (🏌️Joueur, 🤖Automate, PNG)/character_controllers.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_debugger_session.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

// --- BehaviorInspectorPlugin ---

namespace
{
/// Dialogue à éditer pour un objet inspecté (DialogueResource, DialogueSystem, NPCController).
Ref<DialogueResource> dialogue_of(Object *p_object)
{
    if (DialogueResource *dialogue = Object::cast_to<DialogueResource>(p_object))
    {
        return Ref<DialogueResource>(dialogue);
    }
    if (DialogueSystem *system = Object::cast_to<DialogueSystem>(p_object))
    {
        return system->get_default_dialogue();
    }
    if (NPCController *npc = Object::cast_to<NPCController>(p_object))
    {
        return npc->get_dialogue();
    }
    return Ref<DialogueResource>();
}
} // namespace

bool BehaviorInspectorPlugin::_can_handle(Object *p_object) const
{
    return Object::cast_to<State>(p_object) || Object::cast_to<BTNode>(p_object) || Object::cast_to<BehaviorTree>(p_object) || Object::cast_to<DialogueResource>(p_object) || Object::cast_to<DialogueSystem>(p_object) || Object::cast_to<NPCController>(p_object);
}

void BehaviorInspectorPlugin::_on_open_dialogue_pressed(uint64_t p_object_id)
{
    Ref<DialogueResource> dialogue = dialogue_of(ObjectDB::get_instance(ObjectID(p_object_id)));
    if (dialogue.is_valid())
    {
        DialogueEditorWindow::open_for(dialogue);
    }
}

void BehaviorInspectorPlugin::_parse_begin(Object *p_object)
{
    if (Object::cast_to<DialogueResource>(p_object) || Object::cast_to<DialogueSystem>(p_object) || Object::cast_to<NPCController>(p_object))
    {
        Button *dialogue_button = memnew(Button);
        dialogue_button->set_text(U"💬 Éditeur de dialogue");
        if (dialogue_of(p_object).is_valid())
        {
            dialogue_button->set_tooltip_text(U"Ouvrir le dialogue en graphe (TechTree / MindMap) pour l'éditer et suivre son déroulement en jeu");
            dialogue_button->connect("pressed", callable_mp(this, &BehaviorInspectorPlugin::_on_open_dialogue_pressed).bind((uint64_t)p_object->get_instance_id()));
        }
        else
        {
            dialogue_button->set_disabled(true);
            dialogue_button->set_tooltip_text(U"Aucun dialogue : renseignez la propriété dialogue (ou default_dialogue) avec un DialogueResource");
        }
        add_custom_control(dialogue_button);
        return;
    }
    Node *node = Object::cast_to<Node>(p_object);
    Button *button = memnew(Button);
    button->set_text(U"🗺️ Éditeur de comportement");
    if (BehaviorGraphView::resolve_runner(node))
    {
        button->set_tooltip_text(U"Ouvrir la machine à états / l'arbre de comportement en TechTree ou MindMap, pour l'éditer et suivre son exécution en jeu");
        button->connect("pressed", callable_mp(this, &BehaviorInspectorPlugin::_on_open_pressed).bind((uint64_t)p_object->get_instance_id()));
    }
    else
    {
        button->set_disabled(true);
        button->set_tooltip_text(U"Ce nœud n'appartient à aucune StateMachine ni aucun BehaviorTree");
    }
    add_custom_control(button);
}

void BehaviorInspectorPlugin::_on_open_pressed(uint64_t p_node_id)
{
    // Le nœud a pu être supprimé depuis l'affichage de l'inspecteur : résolution par ObjectID.
    BehaviorEditorWindow::open_for(Object::cast_to<Node>(ObjectDB::get_instance(ObjectID(p_node_id))));
}

// --- BehaviorDebuggerPlugin ---

BehaviorDebuggerPlugin *BehaviorDebuggerPlugin::singleton = nullptr;

BehaviorDebuggerPlugin::BehaviorDebuggerPlugin() { singleton = this; }

BehaviorDebuggerPlugin::~BehaviorDebuggerPlugin()
{
    if (singleton == this)
    {
        singleton = nullptr;
    }
}

bool BehaviorDebuggerPlugin::_has_capture(const String &p_capture) const { return p_capture == "behavior"; }

bool BehaviorDebuggerPlugin::_capture(const String &p_message, const Array &p_data, int32_t /*p_session_id*/)
{
    if (p_message == "behavior:snapshot" && p_data.size() >= 2)
    {
        snapshots[String(p_data[0])] = p_data[1];
        return true;
    }
    if (p_message == "behavior:runners" && p_data.size() >= 1)
    {
        runners = p_data[0];
        return true;
    }
    return false;
}

void BehaviorDebuggerPlugin::_setup_session(int32_t p_session_id)
{
    Ref<EditorDebuggerSession> session = get_session(p_session_id);
    if (session.is_valid())
    {
        session->connect("started", callable_mp(this, &BehaviorDebuggerPlugin::_on_session_started).bind(p_session_id));
        session->connect("stopped", callable_mp(this, &BehaviorDebuggerPlugin::_on_session_stopped).bind(p_session_id));
    }
}

void BehaviorDebuggerPlugin::_on_session_started(int32_t p_session_id)
{
    if (!active_sessions.has(p_session_id))
    {
        active_sessions.push_back(p_session_id);
    }
    snapshots.clear();
    runners.clear();
    // Premier message au jeu : il commence alors à envoyer la liste de ses comportements.
    _send_watch();
}

void BehaviorDebuggerPlugin::_on_session_stopped(int32_t p_session_id)
{
    int64_t index = active_sessions.find(p_session_id);
    if (index >= 0)
    {
        active_sessions.remove_at(index);
    }
    if (active_sessions.is_empty())
    {
        snapshots.clear();
        runners.clear();
    }
}

void BehaviorDebuggerPlugin::set_watched(const PackedStringArray &p_paths)
{
    if (p_paths == watched)
    {
        return;
    }
    watched = p_paths;
    _send_watch();
}

void BehaviorDebuggerPlugin::_send_watch()
{
    Array data;
    data.push_back(Array(Variant(watched)));
    for (int32_t id : active_sessions)
    {
        Ref<EditorDebuggerSession> session = get_session(id);
        if (session.is_valid())
        {
            session->send_message("behavior:watch", data);
        }
    }
}

Dictionary BehaviorDebuggerPlugin::get_snapshot(const String &p_path) const { return snapshots.get(p_path, Dictionary()); }

// --- BehaviorEditorPlugin ---

void BehaviorEditorPlugin::_enter_tree()
{
    inspector_plugin.instantiate();
    add_inspector_plugin(inspector_plugin);
    debugger_plugin.instantiate();
    add_debugger_plugin(debugger_plugin);
    EditorInterface::get_singleton()->get_selection()->connect("selection_changed", callable_mp(this, &BehaviorEditorPlugin::_on_selection_changed), CONNECT_DEFERRED);
}

void BehaviorEditorPlugin::_exit_tree()
{
    BehaviorDebuggerPlugin::detach(); // avant de fermer la fenêtre, qui ne doit plus rien envoyer au jeu
    if (EditorInterface *editor = EditorInterface::get_singleton())
    {
        Callable selection = callable_mp(this, &BehaviorEditorPlugin::_on_selection_changed);
        if (editor->get_selection()->is_connected("selection_changed", selection))
        {
            editor->get_selection()->disconnect("selection_changed", selection);
        }
    }
    BehaviorEditorWindow::close_current();
    DialogueEditorWindow::close_current();
    if (inspector_plugin.is_valid())
    {
        remove_inspector_plugin(inspector_plugin);
        inspector_plugin.unref();
    }
    if (debugger_plugin.is_valid())
    {
        remove_debugger_plugin(debugger_plugin);
        debugger_plugin.unref();
    }
}

void BehaviorEditorPlugin::_on_selection_changed()
{
    BehaviorEditorWindow *window = BehaviorEditorWindow::get_current();
    if (!window)
    {
        return;
    }
    TypedArray<Node> nodes = EditorInterface::get_singleton()->get_selection()->get_selected_nodes();
    if (nodes.size() == 1)
    {
        window->on_editor_selection(Object::cast_to<Node>(nodes[0]));
    }
}
