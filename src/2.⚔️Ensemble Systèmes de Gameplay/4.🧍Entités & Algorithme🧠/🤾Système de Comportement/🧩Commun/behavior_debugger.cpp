#include "behavior_debugger.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

BehaviorDebugger *BehaviorDebugger::singleton = nullptr;

namespace
{
const char *const CAPTURE = "behavior";
constexpr uint64_t SNAPSHOT_PERIOD_MSEC = 100;
constexpr uint64_t RUNNER_LIST_PERIOD_MSEC = 1000;
} // namespace

void BehaviorDebugger::create()
{
    if (singleton)
    {
        return;
    }
    singleton = memnew(BehaviorDebugger);
    // Seulement dans un jeu lancé depuis l'éditeur (le débogueur n'est jamais actif dans l'éditeur lui-même).
    EngineDebugger *debugger = EngineDebugger::get_singleton();
    if (debugger && debugger->is_active() && !Engine::get_singleton()->is_editor_hint())
    {
        debugger->register_message_capture(CAPTURE, callable_mp(singleton, &BehaviorDebugger::_on_message));
    }
}

void BehaviorDebugger::destroy()
{
    if (!singleton)
    {
        return;
    }
    EngineDebugger *debugger = EngineDebugger::get_singleton();
    if (debugger && debugger->has_capture(CAPTURE))
    {
        debugger->unregister_message_capture(CAPTURE);
    }
    memdelete(singleton);
    singleton = nullptr;
}

String BehaviorDebugger::debug_path_of(const Node *p_node)
{
    if (!p_node || !p_node->is_inside_tree())
    {
        return String();
    }
    Node *node = const_cast<Node *>(p_node);
    Node *scene = node->get_tree()->get_current_scene();
    if (scene && (scene == node || scene->is_ancestor_of(node)))
    {
        return String(scene->get_path_to(node));
    }
    return String(p_node->get_path());
}

void BehaviorDebugger::register_runner(Node *p_runner)
{
    ObjectID id(p_runner->get_instance_id());
    if (runners.find(id) < 0)
    {
        runners.push_back(id);
        last_runner_list_msec = 0; // liste renvoyée à la prochaine notification
    }
}

void BehaviorDebugger::unregister_runner(Node *p_runner)
{
    int64_t index = runners.find(ObjectID(p_runner->get_instance_id()));
    if (index >= 0)
    {
        runners.remove_at_unordered(index);
        last_runner_list_msec = 0;
    }
}

void BehaviorDebugger::notify(Node *p_runner)
{
    if (!editor_listening)
    {
        return;
    }
    EngineDebugger *debugger = EngineDebugger::get_singleton();
    if (!debugger || !debugger->is_active())
    {
        return;
    }
    uint64_t now = Time::get_singleton()->get_ticks_msec();
    if (last_runner_list_msec == 0 || now - last_runner_list_msec >= RUNNER_LIST_PERIOD_MSEC)
    {
        last_runner_list_msec = now;
        _send_runner_list();
    }
    if (watched.is_empty())
    {
        return;
    }
    if (now - last_snapshot_msec >= SNAPSHOT_PERIOD_MSEC)
    {
        last_snapshot_msec = now;
        sent_this_window.clear();
    }
    String path = debug_path_of(p_runner);
    if (!watched.has(path) || sent_this_window.has(path))
    {
        return;
    }
    sent_this_window.insert(path);
    Array data;
    data.push_back(path);
    data.push_back(p_runner->call("get_debug_snapshot"));
    debugger->send_message("behavior:snapshot", data);
}

bool BehaviorDebugger::_on_message(const String &p_message, const Array &p_data)
{
    // Selon la version, le message arrive avec ou sans le préfixe de capture.
    String message = p_message.trim_prefix("behavior:");
    editor_listening = true;
    if (message == "watch")
    {
        watched.clear();
        Array paths = p_data.is_empty() ? Array() : Array(p_data[0]);
        for (int i = 0; i < paths.size(); i++)
        {
            watched.insert(String(paths[i]));
        }
        last_snapshot_msec = 0;
        last_runner_list_msec = 0;
        return true;
    }
    return false;
}

void BehaviorDebugger::_send_runner_list()
{
    Array list;
    for (uint32_t i = 0; i < runners.size(); i++)
    {
        Node *runner = Object::cast_to<Node>(ObjectDB::get_instance(runners[i]));
        if (!runner || !runner->is_inside_tree())
        {
            continue;
        }
        Array entry;
        entry.push_back(debug_path_of(runner));
        entry.push_back(runner->get_class());
        entry.push_back(String(runner->get_name()));
        list.push_back(entry);
    }
    Array data;
    data.push_back(list);
    EngineDebugger::get_singleton()->send_message("behavior:runners", data);
}
