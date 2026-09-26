#include "input_rebinder.hpp"

#include "input_axis_action.hpp"
#include "input_button_action.hpp"
#include "input_circle_action.hpp"
#include "input_names.hpp"
#include "input_sphere_action.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal/custom_logger.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace
{
/// Ce qu'un emplacement attend, et où l'écrire dans l'action.
struct SlotTarget
{
    enum Kind
    {
        NONE,
        KEY,           // touche
        JOY_BUTTON,    // bouton de manette
        JOY_AXIS,      // axe de manette
        KEY_OR_MOUSE,  // InputButtonAction : touche ou bouton de souris
        BUTTON_OR_AXIS // InputButtonAction : bouton ou axe de manette
    };

    Kind kind = NONE;
    const char *property = "";
    /// Propriétés remises à « aucun » quand l'affectation réussit (entrées concurrentes).
    PackedStringArray cleared;
    /// Préréglage à repasser en « Custom » (InputCircleAction).
    const char *preset = nullptr;
    int preset_custom = 0;
};

SlotTarget resolve_slot(const InputAction *p_action, InputRebinder::TargetSlot p_slot)
{
    SlotTarget t;
    auto set = [&t](SlotTarget::Kind p_kind, const char *p_property) {
        t.kind = p_kind;
        t.property = p_property;
    };

    if (Object::cast_to<InputButtonAction>(p_action))
    {
        if (p_slot == InputRebinder::SLOT_BUTTON_KBM)
        {
            set(SlotTarget::KEY_OR_MOUSE, "");
        }
        else if (p_slot == InputRebinder::SLOT_BUTTON_GAMEPAD)
        {
            set(SlotTarget::BUTTON_OR_AXIS, "");
        }
    }
    else if (Object::cast_to<InputAxisAction>(p_action))
    {
        switch (p_slot)
        {
        case InputRebinder::SLOT_AXIS_POSITIVE_KEY:
            set(SlotTarget::KEY, "positive_key");
            break;
        case InputRebinder::SLOT_AXIS_NEGATIVE_KEY:
            set(SlotTarget::KEY, "negative_key");
            break;
        case InputRebinder::SLOT_AXIS_GAMEPAD_AXIS:
            set(SlotTarget::JOY_AXIS, "gamepad_axis");
            t.cleared = PackedStringArray{"positive_gamepad_button", "negative_gamepad_button"};
            break;
        case InputRebinder::SLOT_AXIS_POSITIVE_GAMEPAD_BUTTON:
            set(SlotTarget::JOY_BUTTON, "positive_gamepad_button");
            t.cleared = PackedStringArray{"gamepad_axis"};
            break;
        case InputRebinder::SLOT_AXIS_NEGATIVE_GAMEPAD_BUTTON:
            set(SlotTarget::JOY_BUTTON, "negative_gamepad_button");
            t.cleared = PackedStringArray{"gamepad_axis"};
            break;
        default:
            break;
        }
    }
    else if (Object::cast_to<InputCircleAction>(p_action) || Object::cast_to<InputSphereAction>(p_action))
    {
        bool sphere = Object::cast_to<InputSphereAction>(p_action) != nullptr;
        switch (p_slot)
        {
        case InputRebinder::SLOT_UP_KEY:
            set(SlotTarget::KEY, "up_key");
            break;
        case InputRebinder::SLOT_DOWN_KEY:
            set(SlotTarget::KEY, "down_key");
            break;
        case InputRebinder::SLOT_LEFT_KEY:
            set(SlotTarget::KEY, "left_key");
            break;
        case InputRebinder::SLOT_RIGHT_KEY:
            set(SlotTarget::KEY, "right_key");
            break;
        case InputRebinder::SLOT_DPAD_UP:
            set(SlotTarget::JOY_BUTTON, "dpad_up");
            break;
        case InputRebinder::SLOT_DPAD_DOWN:
            set(SlotTarget::JOY_BUTTON, "dpad_down");
            break;
        case InputRebinder::SLOT_DPAD_LEFT:
            set(SlotTarget::JOY_BUTTON, "dpad_left");
            break;
        case InputRebinder::SLOT_DPAD_RIGHT:
            set(SlotTarget::JOY_BUTTON, "dpad_right");
            break;
        case InputRebinder::SLOT_JOY_AXIS_X:
            set(SlotTarget::JOY_AXIS, "joy_axis_x");
            break;
        case InputRebinder::SLOT_JOY_AXIS_Y:
            set(SlotTarget::JOY_AXIS, "joy_axis_y");
            break;
        // Emplacements propres à la sphère (axe Z).
        case InputRebinder::SLOT_FORWARD_KEY:
            set(sphere ? SlotTarget::KEY : SlotTarget::NONE, "forward_key");
            break;
        case InputRebinder::SLOT_BACKWARD_KEY:
            set(sphere ? SlotTarget::KEY : SlotTarget::NONE, "backward_key");
            break;
        case InputRebinder::SLOT_DPAD_FORWARD:
            set(sphere ? SlotTarget::JOY_BUTTON : SlotTarget::NONE, "dpad_forward");
            break;
        case InputRebinder::SLOT_DPAD_BACKWARD:
            set(sphere ? SlotTarget::JOY_BUTTON : SlotTarget::NONE, "dpad_backward");
            break;
        case InputRebinder::SLOT_JOY_AXIS_Z:
            set(sphere ? SlotTarget::JOY_AXIS : SlotTarget::NONE, "joy_axis_z");
            break;
        default:
            break;
        }

        // Une affectation manuelle sur un cercle rompt le préréglage correspondant.
        if (!sphere)
        {
            if (t.kind == SlotTarget::KEY)
            {
                t.preset = "key_axis_preset", t.preset_custom = InputCircleAction::KEY_PRESET_CUSTOM;
            }
            else if (t.kind == SlotTarget::JOY_BUTTON)
            {
                t.preset = "dpad_preset", t.preset_custom = InputCircleAction::DPAD_PRESET_CUSTOM;
            }
            else if (t.kind == SlotTarget::JOY_AXIS)
            {
                t.preset = "joy_axis_preset", t.preset_custom = InputCircleAction::JOY_PRESET_CUSTOM;
            }
        }
    }
    return t;
}

String slot_name(InputRebinder::TargetSlot p_slot)
{
    static const char *names[] = {"BUTTON_KBM",   "BUTTON_GAMEPAD", "AXIS_POSITIVE_KEY", "AXIS_NEGATIVE_KEY", "AXIS_GAMEPAD_AXIS", "AXIS_POSITIVE_GAMEPAD_BUTTON",
                                  "AXIS_NEGATIVE_GAMEPAD_BUTTON", "UP_KEY", "DOWN_KEY", "LEFT_KEY", "RIGHT_KEY", "FORWARD_KEY", "BACKWARD_KEY", "DPAD_UP",
                                  "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT", "DPAD_FORWARD", "DPAD_BACKWARD", "JOY_AXIS_X", "JOY_AXIS_Y", "JOY_AXIS_Z"};
    return (int)p_slot >= 0 && (int)p_slot < (int)(sizeof(names) / sizeof(names[0])) ? String(names[p_slot]) : itos(p_slot);
}
} // namespace

void InputRebinder::_ready() { set_process_input(false); }

void InputRebinder::start_rebind(InputAction *p_action, TargetSlot p_slot)
{
    if (p_action == nullptr)
    {
        return;
    }
    if (resolve_slot(p_action, p_slot).kind == SlotTarget::NONE)
    {
        CustomLogger::log(get_name(), InputNames::ICON_REBINDER,
                          "[color=orange]Emplacement " + slot_name(p_slot) + " sans objet pour " + p_action->get_class() + " '" + String(p_action->get_action_name()) + "'[/color]");
        return;
    }

    current_action_id = ObjectID(p_action->get_instance_id());
    current_slot = p_slot;
    listening = true;
    uint64_t this_session = ++session;

    // _input et non _unhandled_input : l'interface consommerait sinon les clics et les touches de navigation
    // (flèches, Entrée, Tab), qui ne pourraient jamais être affectés.
    set_process_input(true);
    emit_signal("rebind_started", p_action, p_slot);

    if (listening_timeout > 0.0f && is_inside_tree())
    {
        Ref<SceneTreeTimer> timer = get_tree()->create_timer(listening_timeout);
        timer->connect("timeout", callable_mp(this, &InputRebinder::on_timeout).bind(this_session));
    }
}

void InputRebinder::on_timeout(uint64_t p_session)
{
    // Session différente : cette écoute s'est déjà terminée (ou une autre a commencé).
    if (listening && p_session == session)
    {
        cancel_rebind();
    }
}

void InputRebinder::cancel_rebind()
{
    if (!listening)
    {
        return;
    }
    stop_listening();
    emit_signal("rebind_cancelled");
}

void InputRebinder::stop_listening()
{
    listening = false;
    current_action_id = ObjectID();
    session++;
    set_process_input(false);
}

InputAction *InputRebinder::get_current_action() const { return Object::cast_to<InputAction>(ObjectDB::get_instance(current_action_id)); }

void InputRebinder::_input(const Ref<InputEvent> &p_event)
{
    if (!listening || Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    InputAction *action = get_current_action();
    if (action == nullptr)
    {
        // Action libérée pendant l'écoute.
        cancel_rebind();
        return;
    }

    Ref<InputEventKey> key_event = p_event;
    if (key_event.is_valid() && key_event->is_pressed() && key_event->get_keycode() == cancel_key)
    {
        get_viewport()->set_input_as_handled();
        cancel_rebind();
        return;
    }

    if (apply_event(action, current_slot, p_event))
    {
        get_viewport()->set_input_as_handled();
        TargetSlot slot = current_slot;
        stop_listening();
        emit_signal("rebind_completed", action, slot);
    }
}

bool InputRebinder::apply_event(InputAction *p_action, TargetSlot p_slot, const Ref<InputEvent> &p_event) const
{
    if (p_action == nullptr || p_event.is_null())
    {
        return false;
    }
    SlotTarget target = resolve_slot(p_action, p_slot);

    Ref<InputEventKey> key_event = p_event;
    Ref<InputEventMouseButton> mouse_event = p_event;
    Ref<InputEventJoypadButton> joy_button_event = p_event;
    Ref<InputEventJoypadMotion> joy_motion_event = p_event;

    // Les répétitions automatiques (touche maintenue) ne comptent pas comme une nouvelle entrée.
    bool key_pressed = key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo();
    bool mouse_pressed = mouse_event.is_valid() && mouse_event->is_pressed();
    bool joy_button_pressed = joy_button_event.is_valid() && joy_button_event->is_pressed();
    bool joy_axis_moved = joy_motion_event.is_valid() && Math::abs(joy_motion_event->get_axis_value()) >= joy_motion_threshold;

    // Certaines dispositions ne fournissent pas de keycode : on se rabat sur la touche physique.
    Key keycode = key_pressed ? (key_event->get_keycode() != KEY_NONE ? key_event->get_keycode() : key_event->get_physical_keycode()) : KEY_NONE;

    Variant value;
    switch (target.kind)
    {
    case SlotTarget::KEY:
        if (key_pressed)
        {
            value = keycode;
        }
        break;
    case SlotTarget::JOY_BUTTON:
        if (joy_button_pressed)
        {
            value = joy_button_event->get_button_index();
        }
        break;
    case SlotTarget::JOY_AXIS:
        if (joy_axis_moved)
        {
            value = joy_motion_event->get_axis();
        }
        break;
    case SlotTarget::KEY_OR_MOUSE:
        if (key_pressed)
        {
            p_action->set("key", keycode);
            p_action->set("mouse_button", MOUSE_BUTTON_NONE);
            return true;
        }
        if (mouse_pressed)
        {
            p_action->set("mouse_button", mouse_event->get_button_index());
            p_action->set("key", KEY_NONE);
            return true;
        }
        return false;
    case SlotTarget::BUTTON_OR_AXIS:
        if (joy_button_pressed)
        {
            p_action->set("gamepad_button", joy_button_event->get_button_index());
            p_action->set("gamepad_axis", JOY_AXIS_INVALID);
            return true;
        }
        if (joy_axis_moved)
        {
            p_action->set("gamepad_axis", joy_motion_event->get_axis());
            p_action->set("gamepad_button", JOY_BUTTON_INVALID);
            return true;
        }
        return false;
    case SlotTarget::NONE:
        return false;
    }

    if (value.get_type() == Variant::NIL)
    {
        return false;
    }

    if (target.preset)
    {
        p_action->set(target.preset, target.preset_custom);
    }
    for (const String &cleared : target.cleared)
    {
        // Axe et boutons de manette : -1 = « aucun » pour les deux énumérations.
        p_action->set(cleared, -1);
    }
    p_action->set(target.property, value);
    return true;
}

void InputRebinder::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("start_rebind", "action", "slot"), &InputRebinder::start_rebind);
    ClassDB::bind_method(D_METHOD("cancel_rebind"), &InputRebinder::cancel_rebind);
    ClassDB::bind_method(D_METHOD("apply_event", "action", "slot", "event"), &InputRebinder::apply_event);
    ClassDB::bind_method(D_METHOD("is_listening"), &InputRebinder::is_listening);
    ClassDB::bind_method(D_METHOD("get_current_action"), &InputRebinder::get_current_action);
    ClassDB::bind_method(D_METHOD("get_current_slot"), &InputRebinder::get_current_slot);
    ClassDB::bind_method(D_METHOD("set_joy_motion_threshold", "threshold"), &InputRebinder::set_joy_motion_threshold);
    ClassDB::bind_method(D_METHOD("get_joy_motion_threshold"), &InputRebinder::get_joy_motion_threshold);
    ClassDB::bind_method(D_METHOD("set_cancel_key", "key"), &InputRebinder::set_cancel_key);
    ClassDB::bind_method(D_METHOD("get_cancel_key"), &InputRebinder::get_cancel_key);
    ClassDB::bind_method(D_METHOD("set_listening_timeout", "timeout"), &InputRebinder::set_listening_timeout);
    ClassDB::bind_method(D_METHOD("get_listening_timeout"), &InputRebinder::get_listening_timeout);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "joy_motion_threshold", PROPERTY_HINT_RANGE, "0.1,0.9,0.05"), "set_joy_motion_threshold", "get_joy_motion_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "cancel_key", PROPERTY_HINT_ENUM, InputNames::key_hint()), "set_cancel_key", "get_cancel_key");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "listening_timeout", PROPERTY_HINT_RANGE, "0,30,0.5,or_greater,suffix:s"), "set_listening_timeout", "get_listening_timeout");

    ADD_SIGNAL(MethodInfo("rebind_started", PropertyInfo(Variant::OBJECT, "action", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, "InputAction"), PropertyInfo(Variant::INT, "slot")));
    ADD_SIGNAL(MethodInfo("rebind_completed", PropertyInfo(Variant::OBJECT, "action", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, "InputAction"), PropertyInfo(Variant::INT, "slot")));
    ADD_SIGNAL(MethodInfo("rebind_cancelled"));

    BIND_ENUM_CONSTANT(SLOT_BUTTON_KBM);
    BIND_ENUM_CONSTANT(SLOT_BUTTON_GAMEPAD);
    BIND_ENUM_CONSTANT(SLOT_AXIS_POSITIVE_KEY);
    BIND_ENUM_CONSTANT(SLOT_AXIS_NEGATIVE_KEY);
    BIND_ENUM_CONSTANT(SLOT_AXIS_GAMEPAD_AXIS);
    BIND_ENUM_CONSTANT(SLOT_AXIS_POSITIVE_GAMEPAD_BUTTON);
    BIND_ENUM_CONSTANT(SLOT_AXIS_NEGATIVE_GAMEPAD_BUTTON);
    BIND_ENUM_CONSTANT(SLOT_UP_KEY);
    BIND_ENUM_CONSTANT(SLOT_DOWN_KEY);
    BIND_ENUM_CONSTANT(SLOT_LEFT_KEY);
    BIND_ENUM_CONSTANT(SLOT_RIGHT_KEY);
    BIND_ENUM_CONSTANT(SLOT_FORWARD_KEY);
    BIND_ENUM_CONSTANT(SLOT_BACKWARD_KEY);
    BIND_ENUM_CONSTANT(SLOT_DPAD_UP);
    BIND_ENUM_CONSTANT(SLOT_DPAD_DOWN);
    BIND_ENUM_CONSTANT(SLOT_DPAD_LEFT);
    BIND_ENUM_CONSTANT(SLOT_DPAD_RIGHT);
    BIND_ENUM_CONSTANT(SLOT_DPAD_FORWARD);
    BIND_ENUM_CONSTANT(SLOT_DPAD_BACKWARD);
    BIND_ENUM_CONSTANT(SLOT_JOY_AXIS_X);
    BIND_ENUM_CONSTANT(SLOT_JOY_AXIS_Y);
    BIND_ENUM_CONSTANT(SLOT_JOY_AXIS_Z);
}
