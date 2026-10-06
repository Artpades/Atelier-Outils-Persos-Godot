#include "input_debugger.hpp"
#include "player_input.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

InputDebugger *InputDebugger::singleton = nullptr;

namespace
{
const char *const CAPTURE = "input";
constexpr uint64_t SNAPSHOT_PERIOD_MSEC = 100;
constexpr uint64_t LIST_PERIOD_MSEC = 1000;
} // namespace

void InputDebugger::create()
{
    if (singleton)
    {
        return;
    }
    singleton = memnew(InputDebugger);
    EngineDebugger *debugger = EngineDebugger::get_singleton();
    if (debugger && debugger->is_active() && !Engine::get_singleton()->is_editor_hint())
    {
        debugger->register_message_capture(CAPTURE, callable_mp(singleton, &InputDebugger::_on_message));
    }
}

void InputDebugger::destroy()
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

String InputDebugger::debug_path_of(const Node *p_node)
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

void InputDebugger::register_player(PlayerInput *p_player)
{
    ObjectID id(p_player->get_instance_id());
    if (players.find(id) < 0)
    {
        players.push_back(id);
        last_list_msec = 0;
    }
}

void InputDebugger::unregister_player(PlayerInput *p_player)
{
    int64_t index = players.find(ObjectID(p_player->get_instance_id()));
    if (index >= 0)
    {
        players.remove_at_unordered(index);
        last_list_msec = 0;
    }
}

void InputDebugger::notify(PlayerInput *p_player)
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
    if (last_list_msec == 0 || now - last_list_msec >= LIST_PERIOD_MSEC)
    {
        last_list_msec = now;
        _send_player_list();
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
    String path = debug_path_of(p_player);
    if (!watched.has(path) || sent_this_window.has(path))
    {
        return;
    }
    sent_this_window.insert(path);
    Array data;
    data.push_back(path);
    data.push_back(p_player->get_debug_snapshot());
    debugger->send_message("input:snapshot", data);
}

PlayerInput *InputDebugger::_find(const String &p_path) const
{
    for (uint32_t i = 0; i < players.size(); i++)
    {
        PlayerInput *player = Object::cast_to<PlayerInput>(ObjectDB::get_instance(players[i]));
        if (player && debug_path_of(player) == p_path)
        {
            return player;
        }
    }
    return nullptr;
}

bool InputDebugger::_on_message(const String &p_message, const Array &p_data)
{
    String message = p_message.trim_prefix("input:");
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
        last_list_msec = 0;
        return true;
    }
    if (message == "simulate" && p_data.size() >= 3)
    {
        if (PlayerInput *player = _find(p_data[0]))
        {
            if (p_data[2].get_type() == Variant::NIL)
            {
                player->stop_simulation(p_data[1]);
            }
            else
            {
                player->simulate(p_data[1], p_data[2]);
            }
        }
        return true;
    }
    if (message == "set_context" && p_data.size() >= 3)
    {
        if (PlayerInput *player = _find(p_data[0]))
        {
            player->set_context_enabled(p_data[1], p_data[2]);
        }
        return true;
    }
    return false;
}

void InputDebugger::_send_player_list()
{
    Array list;
    for (uint32_t i = 0; i < players.size(); i++)
    {
        PlayerInput *player = Object::cast_to<PlayerInput>(ObjectDB::get_instance(players[i]));
        if (!player || !player->is_inside_tree())
        {
            continue;
        }
        Array entry;
        entry.push_back(debug_path_of(player));
        entry.push_back(String(player->get_name()));
        list.push_back(entry);
    }
    Array data;
    data.push_back(list);
    EngineDebugger::get_singleton()->send_message("input:players", data);
}
