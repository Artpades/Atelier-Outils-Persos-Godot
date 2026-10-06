#include "dialogue_resource.hpp"

#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/local_vector.hpp>

using namespace godot;

namespace
{
Dictionary make_link(const StringName &p_to, const String &p_label, int p_slot)
{
    Dictionary link;
    link["to"] = String(p_to);
    link["label"] = p_label;
    link["slot"] = p_slot;
    return link;
}

String shorten(const String &p_text, int p_length = 48)
{
    String text = p_text.replace("\n", " ").strip_edges();
    return text.length() > p_length ? text.substr(0, p_length - 1) + String(U"…") : text;
}
} // namespace

// 💬 DialogueNode

void DialogueNode::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_node_id", "id"), &DialogueNode::set_node_id);
    ClassDB::bind_method(D_METHOD("get_node_id"), &DialogueNode::get_node_id);
    ClassDB::bind_method(D_METHOD("set_comment", "comment"), &DialogueNode::set_comment);
    ClassDB::bind_method(D_METHOD("get_comment"), &DialogueNode::get_comment);
    ClassDB::bind_method(D_METHOD("get_links"), &DialogueNode::get_links);
    ClassDB::bind_method(D_METHOD("set_link", "slot", "to"), &DialogueNode::set_link);
    ClassDB::bind_method(D_METHOD("get_graph_kind"), &DialogueNode::get_graph_kind);
    ClassDB::bind_method(D_METHOD("get_summary"), &DialogueNode::get_summary);

    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "node_id"), "set_node_id", "get_node_id");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "comment", PROPERTY_HINT_MULTILINE_TEXT), "set_comment", "get_comment");
}

// 🗨️ DialogueLine

Array DialogueLine::get_links() const
{
    Array links;
    if (!String(next).is_empty())
    {
        links.push_back(make_link(next, String(), 0));
    }
    return links;
}

void DialogueLine::set_link(int /*p_slot*/, const StringName &p_to) { set_next(p_to); }

String DialogueLine::get_summary() const { return (speaker.is_empty() ? String() : speaker + String(" : ")) + shorten(text); }

void DialogueLine::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_speaker", "speaker"), &DialogueLine::set_speaker);
    ClassDB::bind_method(D_METHOD("get_speaker"), &DialogueLine::get_speaker);
    ClassDB::bind_method(D_METHOD("set_text", "text"), &DialogueLine::set_text);
    ClassDB::bind_method(D_METHOD("get_text"), &DialogueLine::get_text);
    ClassDB::bind_method(D_METHOD("set_portrait", "portrait"), &DialogueLine::set_portrait);
    ClassDB::bind_method(D_METHOD("get_portrait"), &DialogueLine::get_portrait);
    ClassDB::bind_method(D_METHOD("set_voice", "voice"), &DialogueLine::set_voice);
    ClassDB::bind_method(D_METHOD("get_voice"), &DialogueLine::get_voice);
    ClassDB::bind_method(D_METHOD("set_auto_advance_time", "time"), &DialogueLine::set_auto_advance_time);
    ClassDB::bind_method(D_METHOD("get_auto_advance_time"), &DialogueLine::get_auto_advance_time);
    ClassDB::bind_method(D_METHOD("set_next", "next"), &DialogueLine::set_next);
    ClassDB::bind_method(D_METHOD("get_next"), &DialogueLine::get_next);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "speaker"), "set_speaker", "get_speaker");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "text", PROPERTY_HINT_MULTILINE_TEXT), "set_text", "get_text");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "portrait", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_portrait", "get_portrait");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "voice", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"), "set_voice", "get_voice");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "auto_advance_time", PROPERTY_HINT_RANGE, "0,30,0.01,or_greater,suffix:s"), "set_auto_advance_time", "get_auto_advance_time");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "next"), "set_next", "get_next");
}

// ☑️ DialogueOption

void DialogueOption::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_text", "text"), &DialogueOption::set_text);
    ClassDB::bind_method(D_METHOD("get_text"), &DialogueOption::get_text);
    ClassDB::bind_method(D_METHOD("set_next", "next"), &DialogueOption::set_next);
    ClassDB::bind_method(D_METHOD("get_next"), &DialogueOption::get_next);
    ClassDB::bind_method(D_METHOD("set_condition", "condition"), &DialogueOption::set_condition);
    ClassDB::bind_method(D_METHOD("get_condition"), &DialogueOption::get_condition);
    ClassDB::bind_method(D_METHOD("set_once_only", "once"), &DialogueOption::set_once_only);
    ClassDB::bind_method(D_METHOD("get_once_only"), &DialogueOption::get_once_only);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "next"), "set_next", "get_next");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition", PROPERTY_HINT_EXPRESSION), "set_condition", "get_condition");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "once_only"), "set_once_only", "get_once_only");
}

// 🔀 DialogueChoice

Array DialogueChoice::get_links() const
{
    Array links;
    for (int i = 0; i < options.size(); i++)
    {
        Ref<DialogueOption> option = options[i];
        if (option.is_valid() && !String(option->get_next()).is_empty())
        {
            String label = shorten(option->get_text(), 28);
            if (!option->get_condition().strip_edges().is_empty())
            {
                label += " [" + option->get_condition().strip_edges() + "]";
            }
            links.push_back(make_link(option->get_next(), label, i));
        }
    }
    if (!String(fallback_next).is_empty())
    {
        links.push_back(make_link(fallback_next, U"(aucune option)", -1));
    }
    return links;
}

void DialogueChoice::set_link(int p_slot, const StringName &p_to)
{
    if (p_slot < 0)
    {
        set_fallback_next(p_to);
        return;
    }
    if (p_slot < options.size())
    {
        Ref<DialogueOption> option = options[p_slot];
        if (option.is_valid())
        {
            option->set_next(p_to);
            emit_changed();
        }
    }
}

String DialogueChoice::get_summary() const
{
    String summary = prompt.is_empty() ? String(U"Choix") : shorten(prompt);
    return summary + " (" + String::num_int64(options.size()) + ")";
}

void DialogueChoice::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_prompt", "prompt"), &DialogueChoice::set_prompt);
    ClassDB::bind_method(D_METHOD("get_prompt"), &DialogueChoice::get_prompt);
    ClassDB::bind_method(D_METHOD("set_options", "options"), &DialogueChoice::set_options);
    ClassDB::bind_method(D_METHOD("get_options"), &DialogueChoice::get_options);
    ClassDB::bind_method(D_METHOD("set_fallback_next", "next"), &DialogueChoice::set_fallback_next);
    ClassDB::bind_method(D_METHOD("get_fallback_next"), &DialogueChoice::get_fallback_next);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "prompt", PROPERTY_HINT_MULTILINE_TEXT), "set_prompt", "get_prompt");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "options", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":DialogueOption"), "set_options", "get_options");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "fallback_next"), "set_fallback_next", "get_fallback_next");
}

// ❔ DialogueBranch

Array DialogueBranch::get_links() const
{
    Array links;
    if (!String(next_if_true).is_empty())
    {
        links.push_back(make_link(next_if_true, U"vrai", 0));
    }
    if (!String(next_if_false).is_empty())
    {
        links.push_back(make_link(next_if_false, U"faux", 1));
    }
    return links;
}

void DialogueBranch::set_link(int p_slot, const StringName &p_to)
{
    if (p_slot == 1)
    {
        set_next_if_false(p_to);
    }
    else
    {
        set_next_if_true(p_to);
    }
}

void DialogueBranch::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_condition", "condition"), &DialogueBranch::set_condition);
    ClassDB::bind_method(D_METHOD("get_condition"), &DialogueBranch::get_condition);
    ClassDB::bind_method(D_METHOD("set_next_if_true", "next"), &DialogueBranch::set_next_if_true);
    ClassDB::bind_method(D_METHOD("get_next_if_true"), &DialogueBranch::get_next_if_true);
    ClassDB::bind_method(D_METHOD("set_next_if_false", "next"), &DialogueBranch::set_next_if_false);
    ClassDB::bind_method(D_METHOD("get_next_if_false"), &DialogueBranch::get_next_if_false);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition", PROPERTY_HINT_EXPRESSION), "set_condition", "get_condition");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "next_if_true"), "set_next_if_true", "get_next_if_true");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "next_if_false"), "set_next_if_false", "get_next_if_false");
}

// ⚙️ DialogueAction

Array DialogueAction::get_links() const
{
    Array links;
    if (!String(next).is_empty())
    {
        links.push_back(make_link(next, String(), 0));
    }
    return links;
}

void DialogueAction::set_link(int /*p_slot*/, const StringName &p_to) { set_next(p_to); }

String DialogueAction::get_summary() const
{
    PackedStringArray parts;
    for (const String &assignment : assignments)
    {
        parts.push_back(assignment.strip_edges());
    }
    if (!String(event).is_empty())
    {
        parts.push_back(String(U"⚡") + String(event));
    }
    return shorten(String("; ").join(parts));
}

void DialogueAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_assignments", "assignments"), &DialogueAction::set_assignments);
    ClassDB::bind_method(D_METHOD("get_assignments"), &DialogueAction::get_assignments);
    ClassDB::bind_method(D_METHOD("set_event", "event"), &DialogueAction::set_event);
    ClassDB::bind_method(D_METHOD("get_event"), &DialogueAction::get_event);
    ClassDB::bind_method(D_METHOD("set_event_arguments", "arguments"), &DialogueAction::set_event_arguments);
    ClassDB::bind_method(D_METHOD("get_event_arguments"), &DialogueAction::get_event_arguments);
    ClassDB::bind_method(D_METHOD("set_next", "next"), &DialogueAction::set_next);
    ClassDB::bind_method(D_METHOD("get_next"), &DialogueAction::get_next);

    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "assignments"), "set_assignments", "get_assignments");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "event"), "set_event", "get_event");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "event_arguments"), "set_event_arguments", "get_event_arguments");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "next"), "set_next", "get_next");
}

// 📜 DialogueResource

Ref<DialogueNode> DialogueResource::get_dialogue_node(const StringName &p_id) const
{
    if (String(p_id).is_empty())
    {
        return Ref<DialogueNode>();
    }
    for (int i = 0; i < nodes.size(); i++)
    {
        Ref<DialogueNode> node = nodes[i];
        if (node.is_valid() && node->get_node_id() == p_id)
        {
            return node;
        }
    }
    return Ref<DialogueNode>();
}

Ref<DialogueNode> DialogueResource::get_start_node() const
{
    if (!String(start_id).is_empty())
    {
        return get_dialogue_node(start_id);
    }
    return nodes.is_empty() ? Ref<DialogueNode>() : Ref<DialogueNode>(nodes[0]);
}

StringName DialogueResource::generate_id(const String &p_prefix) const
{
    for (int i = 1; i < 100000; i++)
    {
        StringName id = p_prefix + String("_") + String::num_int64(i);
        if (!has_dialogue_node(id))
        {
            return id;
        }
    }
    return StringName(p_prefix);
}

PackedStringArray DialogueResource::validate() const
{
    PackedStringArray issues;
    HashSet<StringName> ids;
    for (int i = 0; i < nodes.size(); i++)
    {
        Ref<DialogueNode> node = nodes[i];
        if (node.is_null())
        {
            issues.push_back(U"Nœud n°" + String::num_int64(i) + U" vide.");
            continue;
        }
        if (String(node->get_node_id()).is_empty())
        {
            issues.push_back(U"Nœud n°" + String::num_int64(i) + U" sans identifiant.");
        }
        else if (ids.has(node->get_node_id()))
        {
            issues.push_back(U"Identifiant en double : « " + String(node->get_node_id()) + U" ».");
        }
        ids.insert(node->get_node_id());
    }
    if (get_start_node().is_null())
    {
        issues.push_back(U"Nœud de départ introuvable.");
    }
    for (int i = 0; i < nodes.size(); i++)
    {
        Ref<DialogueNode> node = nodes[i];
        if (node.is_null())
        {
            continue;
        }
        Array links = node->get_links();
        for (int l = 0; l < links.size(); l++)
        {
            StringName to = String(Dictionary(links[l])["to"]);
            if (!ids.has(to))
            {
                issues.push_back(U"« " + String(node->get_node_id()) + U" » → « " + String(to) + U" » : nœud introuvable.");
            }
        }
    }
    return issues;
}

Dictionary DialogueResource::get_graph_snapshot(const StringName &p_current, const Dictionary &p_visited) const
{
    // Parcours en largeur depuis le départ : le premier lien qui atteint un nœud devient son lien « parent »
    // (dessiné comme une branche), les autres liens sont dessinés en arcs.
    struct Entry
    {
        Ref<DialogueNode> node;
        String parent;
        String edge_label;
        int edge_slot = -2;
        Array arcs;
        bool reached = false;
    };
    LocalVector<Entry> entries;
    HashMap<String, int> index_of;
    for (int i = 0; i < nodes.size(); i++)
    {
        Ref<DialogueNode> node = nodes[i];
        if (node.is_valid() && !String(node->get_node_id()).is_empty() && !index_of.has(node->get_node_id()))
        {
            index_of[node->get_node_id()] = int(entries.size());
            Entry entry;
            entry.node = node;
            entries.push_back(entry);
        }
    }
    LocalVector<int> order;
    Ref<DialogueNode> start = get_start_node();
    if (start.is_valid() && index_of.has(start->get_node_id()))
    {
        int first = index_of[start->get_node_id()];
        entries[first].reached = true;
        entries[first].parent = ".";
        order.push_back(first);
    }
    for (uint32_t cursor = 0; cursor < order.size(); cursor++)
    {
        int current = order[cursor];
        Array links = entries[current].node->get_links();
        for (int l = 0; l < links.size(); l++)
        {
            Dictionary link = links[l];
            String to = link["to"];
            if (!index_of.has(to))
            {
                continue;
            }
            int target = index_of[to];
            if (!entries[target].reached)
            {
                entries[target].reached = true;
                entries[target].parent = entries[current].node->get_node_id();
                entries[target].edge_label = link["label"];
                entries[target].edge_slot = link["slot"];
                order.push_back(target);
            }
            else
            {
                Dictionary arc;
                arc["to"] = to;
                arc["label"] = link["label"];
                arc["event"] = String();
                arc["slot"] = link["slot"];
                entries[current].arcs.push_back(arc);
            }
        }
    }
    for (uint32_t i = 0; i < entries.size(); i++)
    {
        if (!entries[i].reached)
        {
            entries[i].parent = "."; // inaccessible depuis le départ : rattaché à la racine
            order.push_back(i);
        }
    }

    Array result;
    Dictionary root;
    String name = get_name().is_empty() ? get_path().get_file().get_basename() : get_name();
    root["id"] = ".";
    root["parent"] = String();
    root["name"] = name.is_empty() ? String(U"Dialogue") : name;
    root["class"] = "DialogueResource";
    root["kind"] = "tree";
    root["status"] = -1;
    root["active"] = false;
    root["info"] = String::num_int64(entries.size()) + U" nœuds";
    root["links"] = Array();
    result.push_back(root);
    for (int index : order)
    {
        const Entry &entry = entries[index];
        Ref<DialogueNode> node = entry.node;
        String id = node->get_node_id();
        Dictionary data;
        data["id"] = id;
        data["parent"] = entry.parent;
        data["name"] = node->get_summary().is_empty() ? id : node->get_summary();
        Ref<Script> script = node->get_script();
        data["class"] = id + "  ·  " + (script.is_valid() && !String(script->get_global_name()).is_empty() ? String(script->get_global_name()) : node->get_class());
        data["kind"] = node->get_graph_kind();
        data["active"] = id == String(p_current);
        data["status"] = p_visited.has(id) ? 0 : -1;
        data["fresh"] = true;
        data["initial"] = start.is_valid() && start == node;
        data["edge_label"] = entry.edge_label;
        data["edge_slot"] = entry.edge_slot;
        String info = entry.reached ? node->get_comment() : String(U"⚠ inaccessible depuis le départ");
        Array links = node->get_links();
        for (int l = 0; l < links.size(); l++)
        {
            String to = Dictionary(links[l])["to"];
            if (!index_of.has(to))
            {
                info = U"⚠ lien cassé → " + to;
            }
        }
        data["info"] = info;
        data["links"] = entry.arcs;
        result.push_back(data);
    }

    Dictionary snapshot;
    snapshot["type"] = "dialogue";
    snapshot["name"] = root["name"];
    snapshot["running"] = !String(p_current).is_empty();
    snapshot["time"] = 0.0;
    snapshot["nodes"] = result;
    snapshot["history"] = Array();
    return snapshot;
}

void DialogueResource::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_nodes", "nodes"), &DialogueResource::set_nodes);
    ClassDB::bind_method(D_METHOD("get_nodes"), &DialogueResource::get_nodes);
    ClassDB::bind_method(D_METHOD("set_start_id", "id"), &DialogueResource::set_start_id);
    ClassDB::bind_method(D_METHOD("get_start_id"), &DialogueResource::get_start_id);
    ClassDB::bind_method(D_METHOD("get_dialogue_node", "id"), &DialogueResource::get_dialogue_node);
    ClassDB::bind_method(D_METHOD("has_dialogue_node", "id"), &DialogueResource::has_dialogue_node);
    ClassDB::bind_method(D_METHOD("get_start_node"), &DialogueResource::get_start_node);
    ClassDB::bind_method(D_METHOD("generate_id", "prefix"), &DialogueResource::generate_id);
    ClassDB::bind_method(D_METHOD("validate"), &DialogueResource::validate);
    ClassDB::bind_method(D_METHOD("get_graph_snapshot", "current", "visited"), &DialogueResource::get_graph_snapshot, DEFVAL(StringName()), DEFVAL(Dictionary()));

    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "start_id"), "set_start_id", "get_start_id");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "nodes", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":DialogueNode"), "set_nodes", "get_nodes");
}
