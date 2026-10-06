#include "behavior_expression.hpp"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

bool BehaviorExpression::is_reserved_word(const String &p_name)
{
    static const char *const words[] = {"and", "or", "not", "in", "true", "false", "null", "self", "PI", "TAU", "INF", "NAN"};
    for (const char *word : words)
    {
        if (p_name == word)
        {
            return true;
        }
    }
    return false;
}

Variant BehaviorExpression::evaluate(const String &p_source, const Ref<Blackboard> &p_blackboard, Node *p_agent, bool &r_ok)
{
    r_ok = false;
    if (p_source.strip_edges().is_empty())
    {
        error = U"expression vide";
        return Variant();
    }

    // Entrées : clés du tableau noir utilisables comme identifiants, puis agent et blackboard.
    PackedStringArray current_names;
    Array inputs;
    if (p_blackboard.is_valid())
    {
        Dictionary values = p_blackboard->get_values();
        Array keys = values.keys();
        for (int i = 0; i < keys.size(); i++)
        {
            String key = keys[i];
            if (key.is_valid_ascii_identifier() && key != "agent" && key != "blackboard" && !is_reserved_word(key))
            {
                current_names.push_back(key);
                inputs.push_back(values[keys[i]]);
            }
        }
    }
    current_names.push_back("agent");
    inputs.push_back(p_agent);
    current_names.push_back("blackboard");
    inputs.push_back(p_blackboard);

    if (!parsed || p_source != source || current_names != names)
    {
        source = p_source;
        names = current_names;
        expression.instantiate();
        parsed = true;
        if (expression->parse(source, names) != OK)
        {
            error = expression->get_error_text();
            expression.unref();
        }
        else
        {
            error = String();
        }
    }
    if (expression.is_null())
    {
        if (reported_error != error)
        {
            reported_error = error;
            UtilityFunctions::push_warning(U"Expression de comportement invalide « " + source + U" » : " + error);
        }
        return Variant();
    }

    Variant result = expression->execute(inputs, p_agent, false);
    if (expression->has_execute_failed())
    {
        error = expression->get_error_text();
        if (reported_error != error)
        {
            reported_error = error;
            UtilityFunctions::push_warning(U"Échec de l'expression de comportement « " + source + U" » : " + error);
        }
        return Variant();
    }
    error = String();
    r_ok = true;
    return result;
}
