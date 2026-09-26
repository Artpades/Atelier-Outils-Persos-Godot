#include "input_context.hpp"

#include "input_action.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void InputContext::set_context_enabled(bool p_enabled)
{
    context_enabled = p_enabled;
    // Au chargement de la scène, les enfants n'existent pas encore : la boucle est vide et chaque action
    // garde sa propre valeur enregistrée (qui reflète déjà ce réglage s'il a été fait dans l'éditeur).
    for (int i = 0; i < get_child_count(); i++)
    {
        if (InputAction *action = Object::cast_to<InputAction>(get_child(i)))
        {
            action->set_action_enabled(context_enabled);
        }
    }
}

void InputContext::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_context_name", "name"), &InputContext::set_context_name);
    ClassDB::bind_method(D_METHOD("get_context_name"), &InputContext::get_context_name);
    ClassDB::bind_method(D_METHOD("set_context_enabled", "enabled"), &InputContext::set_context_enabled);
    ClassDB::bind_method(D_METHOD("is_context_enabled"), &InputContext::is_context_enabled);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "context_name"), "set_context_name", "get_context_name");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "context_enabled"), "set_context_enabled", "is_context_enabled");
}
