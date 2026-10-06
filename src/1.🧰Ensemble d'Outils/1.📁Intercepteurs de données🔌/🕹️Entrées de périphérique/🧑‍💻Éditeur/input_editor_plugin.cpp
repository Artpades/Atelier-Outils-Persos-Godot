#include "input_editor_plugin.hpp"
#include "input_editor_window.hpp"

#include "../Gestionnaires/player_input.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_debugger_session.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

// --- InputInspectorPlugin ---

bool InputInspectorPlugin::_can_handle(Object *p_object) const { return Object::cast_to<PlayerInput>(p_object) || Object::cast_to<InputContext>(p_object); }

void InputInspectorPlugin::_parse_begin(Object *p_object)
{
    Button *button = memnew(Button);
    button->set_text(U"🕹️ Éditeur d'entrées");
    button->set_tooltip_text(U"Éditer les contextes, actions et liaisons, et suivre les entrées en direct (test dans l'éditeur ou jeu en cours)");
    button->connect("pressed", callable_mp(this, &InputInspectorPlugin::_on_open_pressed).bind((uint64_t)p_object->get_instance_id()));
    add_custom_control(button);
}

void InputInspectorPlugin::_on_open_pressed(uint64_t p_object_id) { InputEditorWindow::open_for(ObjectDB::get_instance(ObjectID(p_object_id))); }

// --- InputDebuggerPlugin ---

InputDebuggerPlugin *InputDebuggerPlugin::singleton = nullptr;

InputDebuggerPlugin::InputDebuggerPlugin() { singleton = this; }

InputDebuggerPlugin::~InputDebuggerPlugin()
{
    if (singleton == this)
    {
        singleton = nullptr;
    }
}

bool InputDebuggerPlugin::_has_capture(const String &p_capture) const { return p_capture == "input"; }

bool InputDebuggerPlugin::_capture(const String &p_message, const Array &p_data, int32_t /*p_session_id*/)
{
    if (p_message == "input:snapshot" && p_data.size() >= 2)
    {
        snapshots[String(p_data[0])] = p_data[1];
        return true;
    }
    if (p_message == "input:players" && p_data.size() >= 1)
    {
        players = p_data[0];
        return true;
    }
    return false;
}

void InputDebuggerPlugin::_setup_session(int32_t p_session_id)
{
    Ref<EditorDebuggerSession> session = get_session(p_session_id);
    if (session.is_valid())
    {
        session->connect("started", callable_mp(this, &InputDebuggerPlugin::_on_session_started).bind(p_session_id));
        session->connect("stopped", callable_mp(this, &InputDebuggerPlugin::_on_session_stopped).bind(p_session_id));
    }
}

void InputDebuggerPlugin::_on_session_started(int32_t p_session_id)
{
    if (!active_sessions.has(p_session_id))
    {
        active_sessions.push_back(p_session_id);
    }
    snapshots.clear();
    players.clear();
    Array data;
    data.push_back(Array(Variant(watched)));
    _send("input:watch", data); // premier message : le jeu commence à envoyer la liste de ses PlayerInput
}

void InputDebuggerPlugin::_on_session_stopped(int32_t p_session_id)
{
    int64_t index = active_sessions.find(p_session_id);
    if (index >= 0)
    {
        active_sessions.remove_at(index);
    }
    if (active_sessions.is_empty())
    {
        snapshots.clear();
        players.clear();
    }
}

void InputDebuggerPlugin::_send(const String &p_message, const Array &p_data)
{
    for (int32_t id : active_sessions)
    {
        Ref<EditorDebuggerSession> session = get_session(id);
        if (session.is_valid())
        {
            session->send_message(p_message, p_data);
        }
    }
}

void InputDebuggerPlugin::set_watched(const PackedStringArray &p_paths)
{
    if (p_paths == watched)
    {
        return;
    }
    watched = p_paths;
    Array data;
    data.push_back(Array(Variant(watched)));
    _send("input:watch", data);
}

void InputDebuggerPlugin::send_simulate(const String &p_path, const StringName &p_action, const Variant &p_value) { _send("input:simulate", Array::make(p_path, p_action, p_value)); }

void InputDebuggerPlugin::send_set_context(const String &p_path, const StringName &p_context, bool p_enabled) { _send("input:set_context", Array::make(p_path, p_context, p_enabled)); }

// --- InputEditorPlugin ---

void InputEditorPlugin::_enter_tree()
{
    inspector_plugin.instantiate();
    add_inspector_plugin(inspector_plugin);
    debugger_plugin.instantiate();
    add_debugger_plugin(debugger_plugin);
}

void InputEditorPlugin::_exit_tree()
{
    InputDebuggerPlugin::detach();
    InputEditorWindow::close_current();
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
