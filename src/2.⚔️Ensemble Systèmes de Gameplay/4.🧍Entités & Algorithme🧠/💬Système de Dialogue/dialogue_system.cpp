#include "dialogue_system.hpp"

#include "../🤾Système de Comportement/🧩Commun/behavior_debugger.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

LocalVector<ObjectID> DialogueSystem::instances;

namespace
{
constexpr int MAX_STEPS = 256; // garde-fou : boucle d'embranchements / d'actions sans réplique
}

Ref<Blackboard> DialogueSystem::get_variables()
{
    if (variables.is_null())
    {
        variables.instantiate();
    }
    return variables;
}

// ▶️ Déroulement

bool DialogueSystem::start(const Ref<DialogueResource> &p_dialogue, const StringName &p_start_id, Node *p_interlocutor)
{
    Ref<DialogueResource> chosen = p_dialogue.is_valid() ? p_dialogue : default_dialogue;
    if (chosen.is_null())
    {
        ERR_PRINT("DialogueSystem.start : aucun dialogue (ni argument ni default_dialogue).");
        return false;
    }
    if (running)
    {
        stop();
    }
    StringName start_id = p_start_id;
    if (String(start_id).is_empty())
    {
        Ref<DialogueNode> start_node = chosen->get_start_node();
        start_id = start_node.is_valid() ? start_node->get_node_id() : StringName();
    }
    dialogue = chosen;
    running = true;
    wait_state = WAIT_NONE;
    visited.clear();
    available_options.clear();
    interlocutor_id = p_interlocutor ? ObjectID(p_interlocutor->get_instance_id()) : ObjectID();
    emit_signal("dialogue_started", dialogue);
    _goto(start_id);
    return true;
}

void DialogueSystem::_goto(const StringName &p_id)
{
    StringName id = p_id;
    for (int step = 0; step < MAX_STEPS && running; step++)
    {
        if (String(id).is_empty())
        {
            stop();
            return;
        }
        Ref<DialogueNode> node = dialogue->get_dialogue_node(id);
        if (node.is_null())
        {
            UtilityFunctions::push_warning(U"DialogueSystem : nœud « " + String(id) + U" » introuvable, fin du dialogue.");
            stop();
            return;
        }
        current_id = id;
        visited[String(id)] = true;

        if (DialogueLine *line = Object::cast_to<DialogueLine>(node.ptr()))
        {
            wait_state = WAIT_LINE;
            auto_advance_remaining = line->get_auto_advance_time();
            String text = interpolate(line->get_text());
            _log(line->get_speaker().is_empty() ? text : line->get_speaker() + String(" : ") + text);
            emit_signal("line_shown", line->get_speaker(), text, node);
            return;
        }
        if (DialogueChoice *choice = Object::cast_to<DialogueChoice>(node.ptr()))
        {
            available_options.clear();
            TypedArray<DialogueOption> options = choice->get_options();
            for (int i = 0; i < options.size(); i++)
            {
                Ref<DialogueOption> option = options[i];
                if (option.is_null() || (option->get_once_only() && seen_options.has(_option_key(i))) || !evaluate_condition(option->get_condition()))
                {
                    continue;
                }
                Dictionary entry;
                entry["index"] = available_options.size();
                entry["option_index"] = i;
                entry["text"] = interpolate(option->get_text());
                entry["option"] = option;
                available_options.push_back(entry);
            }
            if (available_options.is_empty())
            {
                id = choice->get_fallback_next();
                continue;
            }
            wait_state = WAIT_CHOICE;
            emit_signal("choices_shown", interpolate(choice->get_prompt()), available_options, node);
            return;
        }
        if (DialogueBranch *branch = Object::cast_to<DialogueBranch>(node.ptr()))
        {
            id = evaluate_condition(branch->get_condition()) ? branch->get_next_if_true() : branch->get_next_if_false();
            continue;
        }
        if (DialogueAction *action = Object::cast_to<DialogueAction>(node.ptr()))
        {
            for (const String &assignment : action->get_assignments())
            {
                execute_assignment(assignment);
            }
            if (!String(action->get_event()).is_empty())
            {
                emit_signal("dialogue_event", action->get_event(), action->get_event_arguments());
            }
            id = action->get_next();
            continue;
        }
        stop(); // DialogueEnd, ou type inconnu
        return;
    }
    if (running)
    {
        ERR_PRINT("DialogueSystem : trop d'étapes sans réplique ni choix (boucle ?), fin du dialogue.");
        stop();
    }
}

void DialogueSystem::advance()
{
    if (!running || wait_state != WAIT_LINE)
    {
        return;
    }
    Ref<DialogueLine> line = get_current_node();
    wait_state = WAIT_NONE;
    _goto(line.is_valid() ? line->get_next() : StringName());
}

void DialogueSystem::choose(int p_index)
{
    if (!running || wait_state != WAIT_CHOICE || p_index < 0 || p_index >= available_options.size())
    {
        return;
    }
    Dictionary entry = available_options[p_index];
    Ref<DialogueOption> option = entry["option"];
    if (option->get_once_only())
    {
        seen_options[_option_key(entry["option_index"])] = true;
    }
    _log(String(U"→ ") + String(entry["text"]));
    wait_state = WAIT_NONE;
    available_options.clear();
    emit_signal("option_chosen", p_index, entry["text"]);
    _goto(option->get_next());
}

void DialogueSystem::stop()
{
    if (!running)
    {
        return;
    }
    running = false;
    wait_state = WAIT_NONE;
    available_options.clear();
    Ref<DialogueResource> ended = dialogue;
    current_id = StringName();
    emit_signal("dialogue_ended", ended);
}

// 🔤 Variables et expressions

String DialogueSystem::interpolate(const String &p_text)
{
    if (!p_text.contains("{"))
    {
        return p_text;
    }
    return p_text.format(get_variables()->get_values());
}

bool DialogueSystem::evaluate_condition(const String &p_condition)
{
    if (p_condition.strip_edges().is_empty())
    {
        return true;
    }
    bool ok = false;
    Variant result = evaluator.evaluate(p_condition, get_variables(), get_interlocutor(), ok);
    return ok && result.booleanize();
}

bool DialogueSystem::execute_assignment(const String &p_assignment)
{
    String text = p_assignment.strip_edges();
    String key;
    String expression;
    String operation;
    int compound = MAX(text.find("+="), text.find("-="));
    if (compound > 0)
    {
        key = text.substr(0, compound).strip_edges();
        operation = text.substr(compound, 1);
        expression = text.substr(compound + 2).strip_edges();
    }
    else
    {
        // Premier « = » qui n'appartient pas à ==, !=, <=, >=
        for (int i = 0; i < text.length(); i++)
        {
            if (text[i] != '=')
            {
                continue;
            }
            char32_t before = i > 0 ? text[i - 1] : ' ';
            char32_t after = i + 1 < text.length() ? text[i + 1] : ' ';
            if (after == '=' || before == '=' || before == '!' || before == '<' || before == '>')
            {
                continue;
            }
            key = text.substr(0, i).strip_edges();
            expression = text.substr(i + 1).strip_edges();
            break;
        }
    }
    if (BehaviorExpression::is_reserved_word(key))
    {
        UtilityFunctions::push_warning(U"DialogueSystem : « " + key + U" » est un mot réservé des expressions, choisissez un autre nom de variable.");
        return false;
    }
    if (!key.is_valid_ascii_identifier() || expression.is_empty())
    {
        UtilityFunctions::push_warning(U"DialogueSystem : affectation invalide « " + p_assignment + U" » (attendu : nom = expression).");
        return false;
    }
    Ref<Blackboard> board = get_variables();
    if (!operation.is_empty())
    {
        if (!board->has_value(key))
        {
            board->set_value(key, 0);
        }
        expression = key + " " + operation + " (" + expression + ")";
    }
    bool ok = false;
    Variant value = evaluator.evaluate(expression, board, get_interlocutor(), ok);
    if (ok)
    {
        board->set_value(key, value);
    }
    return ok;
}

// 🔍 Requêtes

Ref<DialogueNode> DialogueSystem::get_current_node() const { return dialogue.is_valid() ? dialogue->get_dialogue_node(current_id) : Ref<DialogueNode>(); }

Node *DialogueSystem::get_interlocutor() const { return Object::cast_to<Node>(ObjectDB::get_instance(interlocutor_id)); }

String DialogueSystem::_option_key(int p_index) const
{
    String path = dialogue.is_valid() ? dialogue->get_path() : String();
    if (path.is_empty() && dialogue.is_valid())
    {
        path = String::num_uint64(dialogue->get_instance_id());
    }
    return path + "|" + String(current_id) + "|" + String::num_int64(p_index);
}

void DialogueSystem::_log(const String &p_entry)
{
    if (history_size <= 0)
    {
        return;
    }
    history.push_back(p_entry);
    while (history.size() > history_size)
    {
        history.pop_front();
    }
}

DialogueSystem *DialogueSystem::get_main()
{
    for (uint32_t i = 0; i < instances.size(); i++)
    {
        DialogueSystem *system = Object::cast_to<DialogueSystem>(ObjectDB::get_instance(instances[i]));
        if (system && system->is_inside_tree())
        {
            return system;
        }
    }
    return nullptr;
}

Dictionary DialogueSystem::get_debug_snapshot() const
{
    Dictionary snapshot;
    if (dialogue.is_valid())
    {
        snapshot = dialogue->get_graph_snapshot(current_id, visited);
    }
    else
    {
        snapshot["type"] = "dialogue";
        snapshot["nodes"] = Array();
    }
    snapshot["name"] = String(get_name()) + (dialogue.is_valid() ? String(U" — ") + String(snapshot.get("name", "")) : String());
    snapshot["running"] = running;
    Array recent;
    for (int i = MAX(0, int(history.size()) - 8); i < history.size(); i++)
    {
        recent.push_back(history[i]);
    }
    snapshot["history"] = recent;
    return snapshot;
}

void DialogueSystem::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_ENTER_TREE:
        instances.push_back(ObjectID(get_instance_id()));
        if (!Engine::get_singleton()->is_editor_hint())
        {
            if (BehaviorDebugger *debugger = BehaviorDebugger::get_singleton())
            {
                debugger->register_runner(this);
            }
            set_process(true);
        }
        break;
    case NOTIFICATION_EXIT_TREE:
    {
        int64_t index = instances.find(ObjectID(get_instance_id()));
        if (index >= 0)
        {
            instances.remove_at(index);
        }
        if (BehaviorDebugger *debugger = BehaviorDebugger::get_singleton())
        {
            debugger->unregister_runner(this);
        }
        break;
    }
    case NOTIFICATION_PROCESS:
        if (running && wait_state == WAIT_LINE && auto_advance_remaining > 0.0)
        {
            auto_advance_remaining -= get_process_delta_time();
            if (auto_advance_remaining <= 0.0)
            {
                advance();
            }
        }
        if (BehaviorDebugger *debugger = BehaviorDebugger::get_singleton())
        {
            debugger->notify(this);
        }
        break;
    default:
        break;
    }
}

// 🔗 Liaisons

void DialogueSystem::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_variables", "variables"), &DialogueSystem::set_variables);
    ClassDB::bind_method(D_METHOD("get_variables"), &DialogueSystem::get_variables);
    ClassDB::bind_method(D_METHOD("set_default_dialogue", "dialogue"), &DialogueSystem::set_default_dialogue);
    ClassDB::bind_method(D_METHOD("get_default_dialogue"), &DialogueSystem::get_default_dialogue);
    ClassDB::bind_method(D_METHOD("set_history_size", "size"), &DialogueSystem::set_history_size);
    ClassDB::bind_method(D_METHOD("get_history_size"), &DialogueSystem::get_history_size);

    ClassDB::bind_method(D_METHOD("start", "dialogue", "start_id", "interlocutor"), &DialogueSystem::start, DEFVAL(Variant()), DEFVAL(StringName()), DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("advance"), &DialogueSystem::advance);
    ClassDB::bind_method(D_METHOD("choose", "index"), &DialogueSystem::choose);
    ClassDB::bind_method(D_METHOD("stop"), &DialogueSystem::stop);
    ClassDB::bind_method(D_METHOD("is_running"), &DialogueSystem::is_running);
    ClassDB::bind_method(D_METHOD("get_wait_state"), &DialogueSystem::get_wait_state);
    ClassDB::bind_method(D_METHOD("get_dialogue"), &DialogueSystem::get_dialogue);
    ClassDB::bind_method(D_METHOD("get_current_node"), &DialogueSystem::get_current_node);
    ClassDB::bind_method(D_METHOD("get_current_id"), &DialogueSystem::get_current_id);
    ClassDB::bind_method(D_METHOD("get_interlocutor"), &DialogueSystem::get_interlocutor);
    ClassDB::bind_method(D_METHOD("get_available_options"), &DialogueSystem::get_available_options);
    ClassDB::bind_method(D_METHOD("interpolate", "text"), &DialogueSystem::interpolate);
    ClassDB::bind_method(D_METHOD("execute_assignment", "assignment"), &DialogueSystem::execute_assignment);
    ClassDB::bind_method(D_METHOD("evaluate_condition", "condition"), &DialogueSystem::evaluate_condition);
    ClassDB::bind_method(D_METHOD("get_history"), &DialogueSystem::get_history);
    ClassDB::bind_method(D_METHOD("clear_seen_options"), &DialogueSystem::clear_seen_options);
    ClassDB::bind_method(D_METHOD("get_debug_snapshot"), &DialogueSystem::get_debug_snapshot);
    ClassDB::bind_static_method("DialogueSystem", D_METHOD("get_main"), &DialogueSystem::get_main);

    BIND_ENUM_CONSTANT(WAIT_NONE);
    BIND_ENUM_CONSTANT(WAIT_LINE);
    BIND_ENUM_CONSTANT(WAIT_CHOICE);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_dialogue", PROPERTY_HINT_RESOURCE_TYPE, "DialogueResource"), "set_default_dialogue", "get_default_dialogue");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "variables", PROPERTY_HINT_RESOURCE_TYPE, "Blackboard"), "set_variables", "get_variables");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "history_size", PROPERTY_HINT_RANGE, "0,500,1"), "set_history_size", "get_history_size");

    ADD_SIGNAL(MethodInfo("dialogue_started", PropertyInfo(Variant::OBJECT, "dialogue", PROPERTY_HINT_RESOURCE_TYPE, "DialogueResource")));
    ADD_SIGNAL(MethodInfo("line_shown", PropertyInfo(Variant::STRING, "speaker"), PropertyInfo(Variant::STRING, "text"), PropertyInfo(Variant::OBJECT, "line", PROPERTY_HINT_RESOURCE_TYPE, "DialogueLine")));
    ADD_SIGNAL(MethodInfo("choices_shown", PropertyInfo(Variant::STRING, "prompt"), PropertyInfo(Variant::ARRAY, "options"), PropertyInfo(Variant::OBJECT, "choice", PROPERTY_HINT_RESOURCE_TYPE, "DialogueChoice")));
    ADD_SIGNAL(MethodInfo("option_chosen", PropertyInfo(Variant::INT, "index"), PropertyInfo(Variant::STRING, "text")));
    ADD_SIGNAL(MethodInfo("dialogue_event", PropertyInfo(Variant::STRING_NAME, "event"), PropertyInfo(Variant::ARRAY, "arguments")));
    ADD_SIGNAL(MethodInfo("dialogue_ended", PropertyInfo(Variant::OBJECT, "dialogue", PROPERTY_HINT_RESOURCE_TYPE, "DialogueResource")));
}
