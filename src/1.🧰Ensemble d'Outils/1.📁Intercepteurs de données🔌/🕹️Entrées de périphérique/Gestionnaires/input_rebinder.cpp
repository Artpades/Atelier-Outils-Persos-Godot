#include "input_rebinder.hpp"
#include "player_input.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void InputRebinder::start_rebind(const Ref<InputAction> &p_action, const StringName &p_slot)
{
    ERR_FAIL_COND_MSG(p_action.is_null(), U"InputRebinder.start_rebind : action nulle.");
    if (listening)
    {
        cancel_rebind();
    }
    listening = true;
    current_action = p_action;
    current_slot = p_slot;
    remaining = listening_timeout;
    set_process(listening_timeout > 0.0);
    emit_signal("rebind_started", p_action, p_slot);
}

void InputRebinder::cancel_rebind()
{
    if (!listening)
    {
        return;
    }
    Ref<InputAction> action = current_action;
    StringName slot = current_slot;
    _stop();
    emit_signal("rebind_cancelled", action, slot);
}

void InputRebinder::_stop()
{
    listening = false;
    current_action.unref();
    current_slot = StringName();
    remaining = 0.0;
    set_process(false);
}

bool InputRebinder::submit_event(const Ref<InputEvent> &p_event)
{
    if (!listening || current_action.is_null() || p_event.is_null())
    {
        return false;
    }
    Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed() && !key->is_echo() && (int(key->get_keycode()) == cancel_key || int(key->get_physical_keycode()) == cancel_key))
    {
        cancel_rebind();
        return false;
    }
    if (!current_action->apply_rebind(current_slot, p_event, joy_threshold))
    {
        return false;
    }
    Ref<InputAction> action = current_action;
    StringName slot = current_slot;
    _stop();
    emit_signal("rebind_completed", action, slot);
    return true;
}

void InputRebinder::_input(const Ref<InputEvent> &p_event)
{
    if (!listening || Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    bool consumable = Object::cast_to<InputEventKey>(p_event.ptr()) || Object::cast_to<InputEventMouseButton>(p_event.ptr()) || Object::cast_to<InputEventJoypadButton>(p_event.ptr()) || Object::cast_to<InputEventJoypadMotion>(p_event.ptr());
    submit_event(p_event);
    if (consumable && get_viewport())
    {
        get_viewport()->set_input_as_handled(); // la touche choisie ne déclenche rien d'autre (bouton du menu...)
    }
}

Array InputRebinder::find_conflicts(Node *p_player, const Ref<InputAction> &p_action, const StringName &p_slot) const
{
    Array conflicts;
    PlayerInput *player = Object::cast_to<PlayerInput>(p_player);
    if (!player || p_action.is_null())
    {
        return conflicts;
    }
    String text = p_action->get_slot_text(p_slot);
    if (text == String(U"—"))
    {
        return conflicts;
    }
    String group;
    TypedArray<Dictionary> own_slots = p_action->get_rebind_slots();
    for (int i = 0; i < own_slots.size(); i++)
    {
        Dictionary slot = own_slots[i];
        if (StringName(slot["slot"]) == p_slot)
        {
            group = slot["group"];
        }
    }
    Array all = player->get_all_actions();
    for (int i = 0; i < all.size(); i++)
    {
        Ref<InputAction> action = Array(all[i])[1];
        TypedArray<Dictionary> slots = action->get_rebind_slots();
        for (int s = 0; s < slots.size(); s++)
        {
            Dictionary slot = slots[s];
            StringName name = slot["slot"];
            if ((action == p_action && name == p_slot) || String(slot["group"]) != group)
            {
                continue;
            }
            if (action->get_slot_text(name) == text)
            {
                conflicts.push_back(Array::make(action, name));
            }
        }
    }
    return conflicts;
}

void InputRebinder::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY && !Engine::get_singleton()->is_editor_hint())
    {
        set_process_input(true);
        set_process(false);
    }
    else if (p_what == NOTIFICATION_PROCESS && listening && listening_timeout > 0.0)
    {
        remaining -= get_process_delta_time();
        if (remaining <= 0.0)
        {
            cancel_rebind();
        }
    }
}

void InputRebinder::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_joy_threshold", "threshold"), &InputRebinder::set_joy_threshold);
    ClassDB::bind_method(D_METHOD("get_joy_threshold"), &InputRebinder::get_joy_threshold);
    ClassDB::bind_method(D_METHOD("set_cancel_key", "key"), &InputRebinder::set_cancel_key);
    ClassDB::bind_method(D_METHOD("get_cancel_key"), &InputRebinder::get_cancel_key);
    ClassDB::bind_method(D_METHOD("set_listening_timeout", "timeout"), &InputRebinder::set_listening_timeout);
    ClassDB::bind_method(D_METHOD("get_listening_timeout"), &InputRebinder::get_listening_timeout);
    ClassDB::bind_method(D_METHOD("start_rebind", "action", "slot"), &InputRebinder::start_rebind);
    ClassDB::bind_method(D_METHOD("cancel_rebind"), &InputRebinder::cancel_rebind);
    ClassDB::bind_method(D_METHOD("is_listening"), &InputRebinder::is_listening);
    ClassDB::bind_method(D_METHOD("get_current_action"), &InputRebinder::get_current_action);
    ClassDB::bind_method(D_METHOD("get_current_slot"), &InputRebinder::get_current_slot);
    ClassDB::bind_method(D_METHOD("get_remaining_time"), &InputRebinder::get_remaining_time);
    ClassDB::bind_method(D_METHOD("submit_event", "event"), &InputRebinder::submit_event);
    ClassDB::bind_method(D_METHOD("find_conflicts", "player", "action", "slot"), &InputRebinder::find_conflicts);

    ADD_SIGNAL(MethodInfo("rebind_started", PropertyInfo(Variant::OBJECT, "action", PROPERTY_HINT_RESOURCE_TYPE, "InputAction"), PropertyInfo(Variant::STRING_NAME, "slot")));
    ADD_SIGNAL(MethodInfo("rebind_completed", PropertyInfo(Variant::OBJECT, "action", PROPERTY_HINT_RESOURCE_TYPE, "InputAction"), PropertyInfo(Variant::STRING_NAME, "slot")));
    ADD_SIGNAL(MethodInfo("rebind_cancelled", PropertyInfo(Variant::OBJECT, "action", PROPERTY_HINT_RESOURCE_TYPE, "InputAction"), PropertyInfo(Variant::STRING_NAME, "slot")));

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "joy_threshold", PROPERTY_HINT_RANGE, "0.05,1,0.05"), "set_joy_threshold", "get_joy_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "cancel_key", PROPERTY_HINT_ENUM, InputAction::key_hint()), "set_cancel_key", "get_cancel_key");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "listening_timeout", PROPERTY_HINT_RANGE, "0,30,0.5,or_greater,suffix:s"), "set_listening_timeout", "get_listening_timeout");
}
