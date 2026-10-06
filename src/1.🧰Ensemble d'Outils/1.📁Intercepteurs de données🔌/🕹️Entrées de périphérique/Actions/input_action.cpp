#include "input_action.hpp"
#include "../Gestionnaires/player_input.hpp"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

// --- InputBinding ---

void InputBinding::set_event(const StringName &p_event)
{
    event = p_event;
    emit_changed();
}

void InputBinding::set_target(const NodePath &p_target)
{
    target = p_target;
    emit_changed();
}

void InputBinding::set_method(const StringName &p_method)
{
    method = p_method;
    emit_changed();
}

Array InputBinding::build_call_arguments(const Array &p_signal_arguments) const
{
    Array result;
    if (pass_value)
    {
        result.append_array(p_signal_arguments);
    }
    result.append_array(arguments);
    return result;
}

String InputBinding::get_description() const
{
    String text = String(event) + U" → " + (target.is_empty() ? String(U"(aucune cible)") : String(target)) + "." + (method == StringName() ? String("?") : String(method)) + "(";
    PackedStringArray parts;
    if (pass_value)
    {
        parts.push_back(U"valeur");
    }
    for (int i = 0; i < arguments.size(); i++)
    {
        parts.push_back(arguments[i].stringify());
    }
    return text + String(", ").join(parts) + ")" + (enabled ? String() : String(U" [désactivée]"));
}

void InputBinding::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &InputBinding::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &InputBinding::is_enabled);
    ClassDB::bind_method(D_METHOD("set_event", "event"), &InputBinding::set_event);
    ClassDB::bind_method(D_METHOD("get_event"), &InputBinding::get_event);
    ClassDB::bind_method(D_METHOD("set_target", "target"), &InputBinding::set_target);
    ClassDB::bind_method(D_METHOD("get_target"), &InputBinding::get_target);
    ClassDB::bind_method(D_METHOD("set_method", "method"), &InputBinding::set_method);
    ClassDB::bind_method(D_METHOD("get_method"), &InputBinding::get_method);
    ClassDB::bind_method(D_METHOD("set_pass_value", "pass_value"), &InputBinding::set_pass_value);
    ClassDB::bind_method(D_METHOD("get_pass_value"), &InputBinding::get_pass_value);
    ClassDB::bind_method(D_METHOD("set_arguments", "arguments"), &InputBinding::set_arguments);
    ClassDB::bind_method(D_METHOD("get_arguments"), &InputBinding::get_arguments);
    ClassDB::bind_method(D_METHOD("build_call_arguments", "signal_arguments"), &InputBinding::build_call_arguments);
    ClassDB::bind_method(D_METHOD("get_description"), &InputBinding::get_description);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "event"), "set_event", "get_event");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target"), "set_target", "get_target");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "method"), "set_method", "get_method");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "pass_value"), "set_pass_value", "get_pass_value");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "arguments"), "set_arguments", "get_arguments");
}

// --- InputAction ---

void InputAction::set_action_name(const StringName &p_name)
{
    action_name = p_name;
    emit_changed();
}

void InputAction::set_enabled(bool p_enabled)
{
    enabled = p_enabled;
    emit_changed();
}

void InputAction::update(const InputReadContext &p_context, double p_delta, PlayerInput *p_owner)
{
    owner_id = p_owner ? ObjectID(p_owner->get_instance_id()) : ObjectID();
    read_context = p_context;
    switch (device_filter)
    {
    case DEVICE_ALL:
        read_context.keyboard_mouse = true;
        read_context.gamepad = true;
        break;
    case DEVICE_KEYBOARD_MOUSE:
        read_context.keyboard_mouse = true;
        read_context.gamepad = false;
        break;
    case DEVICE_GAMEPAD:
        read_context.keyboard_mouse = false;
        read_context.gamepad = true;
        break;
    default:
        break;
    }
    if (!enabled)
    {
        _release(); // n'émet que si l'action n'était pas déjà au repos
        return;
    }
    if (simulated)
    {
        _apply_simulation(simulated_value, p_delta);
        return;
    }
    _update(p_delta);
}

void InputAction::handle_event(const Ref<InputEvent> &p_event, PlayerInput *p_owner)
{
    if (!enabled || p_event.is_null())
    {
        return;
    }
    owner_id = p_owner ? ObjectID(p_owner->get_instance_id()) : ObjectID();
    _handle_event(p_event);
}

void InputAction::release(PlayerInput *p_owner)
{
    owner_id = p_owner ? ObjectID(p_owner->get_instance_id()) : ObjectID();
    _release();
}

void InputAction::_update(double p_delta) { GDVIRTUAL_CALL(_update_action, p_delta); }

Variant InputAction::get_value() const
{
    Variant value;
    GDVIRTUAL_CALL(_get_value, value);
    return value;
}

bool InputAction::is_active() const
{
    Variant value = get_value();
    return value.get_type() != Variant::NIL && value.booleanize();
}

void InputAction::emit_event(const StringName &p_event, const Array &p_arguments)
{
    last_event = p_event;
    event_count++;
    switch (p_arguments.size())
    {
    case 0:
        emit_signal(p_event);
        break;
    case 1:
        emit_signal(p_event, p_arguments[0]);
        break;
    case 2:
        emit_signal(p_event, p_arguments[0], p_arguments[1]);
        break;
    default:
        emit_signal(p_event, p_arguments[0], p_arguments[1], p_arguments[2]);
        break;
    }
    if (PlayerInput *owner = Object::cast_to<PlayerInput>(ObjectDB::get_instance(owner_id)))
    {
        owner->dispatch_event(this, p_event, p_arguments);
    }
}

String InputAction::get_value_text() const
{
    Variant value = get_value();
    switch (value.get_type())
    {
    case Variant::BOOL:
        return bool(value) ? U"● enfoncé" : U"○ relâché";
    case Variant::FLOAT:
        return String::num(double(value), 2);
    case Variant::VECTOR2:
    {
        Vector2 v = value;
        return "(" + String::num(v.x, 2) + ", " + String::num(v.y, 2) + ")";
    }
    case Variant::VECTOR3:
    {
        Vector3 v = value;
        return "(" + String::num(v.x, 2) + ", " + String::num(v.y, 2) + ", " + String::num(v.z, 2) + ")";
    }
    case Variant::NIL:
        return U"—";
    default:
        return value.stringify();
    }
}

void InputAction::simulate(const Variant &p_value)
{
    simulated = true;
    simulated_value = p_value;
}

void InputAction::stop_simulation()
{
    simulated = false;
    simulated_value = Variant();
}

TypedArray<Dictionary> InputAction::get_events() const
{
    static PackedStringArray resource_signals;
    if (resource_signals.is_empty())
    {
        TypedArray<Dictionary> list = ClassDBSingleton::get_singleton()->class_get_signal_list("Resource", false);
        for (int i = 0; i < list.size(); i++)
        {
            resource_signals.push_back(Dictionary(list[i])["name"]);
        }
    }
    TypedArray<Dictionary> result;
    TypedArray<Dictionary> list = get_signal_list();
    for (int i = 0; i < list.size(); i++)
    {
        Dictionary signal = list[i];
        if (resource_signals.has(signal["name"]))
        {
            continue;
        }
        Dictionary entry;
        entry["name"] = signal["name"];
        PackedStringArray arguments;
        Array args = signal["args"];
        for (int a = 0; a < args.size(); a++)
        {
            Dictionary argument = args[a];
            arguments.push_back(String(argument["name"]) + ": " + Variant::get_type_name(Variant::Type(int(argument["type"]))));
        }
        entry["args"] = arguments;
        result.push_back(entry);
    }
    return result;
}

String InputAction::get_slot_text(const StringName &p_slot) const
{
    Variant value = get(p_slot);
    String slot = p_slot;
    if (slot.contains("axis"))
    {
        return joy_axis_text(value);
    }
    if (slot.contains("joy") || slot.contains("dpad"))
    {
        return joy_button_text(value);
    }
    if (slot.contains("mouse"))
    {
        return mouse_button_text(value);
    }
    return key_text(value);
}

Dictionary InputAction::get_device_state() const
{
    Dictionary state;
    PackedStringArray properties = get_device_properties();
    for (const String &property : properties)
    {
        state[property] = get(property);
    }
    return state;
}

void InputAction::set_device_state(const Dictionary &p_state)
{
    PackedStringArray properties = get_device_properties();
    for (const String &property : properties)
    {
        if (p_state.has(property))
        {
            set(property, p_state[property]);
        }
    }
    emit_changed();
}

Ref<InputAction> InputAction::make_runtime_copy() const
{
    Ref<InputAction> copy = duplicate(false);
    TypedArray<InputBinding> copies;
    for (int i = 0; i < bindings.size(); i++)
    {
        Ref<InputBinding> binding = bindings[i];
        if (binding.is_valid())
        {
            Ref<InputBinding> binding_copy = binding->duplicate(false);
            binding_copy->set_arguments(binding->get_arguments().duplicate()); // tableau non partagé
            copies.push_back(binding_copy);
        }
    }
    copy->set_bindings(copies); // nouveau tableau : celui de la ressource d'origine reste intact
    return copy;
}

// Lecture des périphériques

bool InputAction::key_down(int p_key) const
{
    if (p_key == 0 || !read_context.keyboard_mouse)
    {
        return false;
    }
    Input *input = Input::get_singleton();
    return physical_keys ? input->is_physical_key_pressed(Key(p_key)) : input->is_key_pressed(Key(p_key));
}

bool InputAction::mouse_down(int p_button) const { return p_button > 0 && read_context.keyboard_mouse && Input::get_singleton()->is_mouse_button_pressed(MouseButton(p_button)); }

bool InputAction::joy_button_down(int p_button) const { return p_button >= 0 && read_context.gamepad && Input::get_singleton()->is_joy_button_pressed(read_context.device, JoyButton(p_button)); }

float InputAction::joy_axis_value(int p_axis) const
{
    if (p_axis < 0 || !read_context.gamepad)
    {
        return 0.0f;
    }
    return Input::get_singleton()->get_joy_axis(read_context.device, JoyAxis(p_axis));
}

bool InputAction::same_float(double p_a, double p_b) const { return Math::abs(p_a - p_b) < MAX(dirty_threshold, 1e-6); }

Node *InputAction::get_owner_node() const { return Object::cast_to<Node>(ObjectDB::get_instance(owner_id)); }

// 🔤 Noms et hints

String InputAction::key_hint()
{
    static String hint;
    if (!hint.is_empty())
    {
        return hint;
    }
    PackedStringArray entries;
    entries.push_back(String(U"Aucune:0"));
    OS *os = OS::get_singleton();
    auto add = [&](int p_code) {
        String name = os->get_keycode_string(Key(p_code));
        if (!name.is_empty() && os->find_keycode_from_string(name) == p_code && !name.contains(":") && !name.contains(","))
        {
            entries.push_back(name + ":" + String::num_int64(p_code));
        }
    };
    for (int code = 32; code < 127; code++)
    {
        if (code >= 'a' && code <= 'z')
        {
            continue; // les lettres sont en majuscules (A = 65)
        }
        add(code);
    }
    // Touches spéciales : seulement les plages de l'énumération Key (un code inconnu produit une erreur Unicode).
    const int ranges[][2] = {{4194305, 4194366}, {4194370, 4194371}, {4194373, 4194373}, {4194376, 4194382}, {4194388, 4194419}, {4194433, 4194447}};
    for (const auto &range : ranges)
    {
        for (int code = range[0]; code <= range[1]; code++)
        {
            add(code);
        }
    }
    hint = String(",").join(entries);
    return hint;
}

String InputAction::mouse_button_hint() { return String(U"Aucun:0,Gauche:1,Droit:2,Milieu:3,Molette haut:4,Molette bas:5,Molette gauche:6,Molette droite:7,Bouton 4:8,Bouton 5:9"); }

String InputAction::joy_button_hint()
{
    static String hint;
    if (!hint.is_empty())
    {
        return hint;
    }
    PackedStringArray entries;
    for (int button = -1; button <= 20; button++)
    {
        entries.push_back(joy_button_text(button) + ":" + String::num_int64(button));
    }
    hint = String(",").join(entries);
    return hint;
}

String InputAction::joy_axis_hint()
{
    static String hint;
    if (!hint.is_empty())
    {
        return hint;
    }
    PackedStringArray entries;
    for (int axis = -1; axis <= 5; axis++)
    {
        entries.push_back(joy_axis_text(axis) + ":" + String::num_int64(axis));
    }
    hint = String(",").join(entries);
    return hint;
}

String InputAction::key_text(int p_key)
{
    if (p_key == 0)
    {
        return U"—";
    }
    return OS::get_singleton()->get_keycode_string(Key(p_key));
}

String InputAction::mouse_button_text(int p_button)
{
    static const char32_t *names[] = {U"—", U"Clic gauche", U"Clic droit", U"Clic milieu", U"Molette haut", U"Molette bas", U"Molette gauche", U"Molette droite", U"Souris 4", U"Souris 5"};
    return p_button >= 0 && p_button <= 9 ? String(names[p_button]) : String(U"Souris ") + String::num_int64(p_button);
}

String InputAction::joy_button_text(int p_button)
{
    static const char32_t *names[] = {U"A (bas)", U"B (droite)", U"X (gauche)", U"Y (haut)", U"Back", U"Guide", U"Start", U"Stick gauche (clic)", U"Stick droit (clic)", U"LB", U"RB", U"Croix haut", U"Croix bas", U"Croix gauche", U"Croix droite", U"Misc", U"Palette 1", U"Palette 2", U"Palette 3", U"Palette 4", U"Pavé tactile"};
    if (p_button < 0)
    {
        return U"—";
    }
    return p_button <= 20 ? String(names[p_button]) : String(U"Bouton ") + String::num_int64(p_button);
}

String InputAction::joy_axis_text(int p_axis)
{
    static const char32_t *names[] = {U"Stick gauche X", U"Stick gauche Y", U"Stick droit X", U"Stick droit Y", U"Gâchette LT", U"Gâchette RT"};
    if (p_axis < 0)
    {
        return U"—";
    }
    return p_axis <= 5 ? String(names[p_axis]) : String(U"Axe ") + String::num_int64(p_axis);
}

int InputAction::event_key(const Ref<InputEvent> &p_event, bool p_physical)
{
    Ref<InputEventKey> key = p_event;
    if (key.is_null() || !key->is_pressed() || key->is_echo())
    {
        return 0;
    }
    int physical = int(key->get_physical_keycode());
    int logical = int(key->get_keycode());
    return p_physical ? (physical ? physical : logical) : (logical ? logical : physical);
}

int InputAction::event_mouse_button(const Ref<InputEvent> &p_event)
{
    Ref<InputEventMouseButton> mouse = p_event;
    return mouse.is_valid() && mouse->is_pressed() ? int(mouse->get_button_index()) : 0;
}

int InputAction::event_joy_button(const Ref<InputEvent> &p_event)
{
    Ref<InputEventJoypadButton> button = p_event;
    return button.is_valid() && button->is_pressed() ? int(button->get_button_index()) : -1;
}

int InputAction::event_joy_axis(const Ref<InputEvent> &p_event, double p_threshold, int *r_sign)
{
    Ref<InputEventJoypadMotion> motion = p_event;
    if (motion.is_null() || Math::abs(motion->get_axis_value()) < p_threshold)
    {
        return -1;
    }
    if (r_sign)
    {
        *r_sign = motion->get_axis_value() >= 0.0f ? 1 : -1;
    }
    return int(motion->get_axis());
}

void InputAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_action_name", "name"), &InputAction::set_action_name);
    ClassDB::bind_method(D_METHOD("get_action_name"), &InputAction::get_action_name);
    ClassDB::bind_method(D_METHOD("set_description", "description"), &InputAction::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &InputAction::get_description);
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &InputAction::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &InputAction::is_enabled);
    ClassDB::bind_method(D_METHOD("set_device_filter", "filter"), &InputAction::set_device_filter);
    ClassDB::bind_method(D_METHOD("get_device_filter"), &InputAction::get_device_filter);
    ClassDB::bind_method(D_METHOD("set_physical_keys", "physical"), &InputAction::set_physical_keys);
    ClassDB::bind_method(D_METHOD("get_physical_keys"), &InputAction::get_physical_keys);
    ClassDB::bind_method(D_METHOD("set_dead_zone", "dead_zone"), &InputAction::set_dead_zone);
    ClassDB::bind_method(D_METHOD("get_dead_zone"), &InputAction::get_dead_zone);
    ClassDB::bind_method(D_METHOD("set_dirty_threshold", "threshold"), &InputAction::set_dirty_threshold);
    ClassDB::bind_method(D_METHOD("get_dirty_threshold"), &InputAction::get_dirty_threshold);
    ClassDB::bind_method(D_METHOD("set_bindings", "bindings"), &InputAction::set_bindings);
    ClassDB::bind_method(D_METHOD("get_bindings"), &InputAction::get_bindings);

    ClassDB::bind_method(D_METHOD("emit_event", "event", "arguments"), &InputAction::emit_event, DEFVAL(Array()));
    ClassDB::bind_method(D_METHOD("get_value"), &InputAction::get_value);
    ClassDB::bind_method(D_METHOD("is_active"), &InputAction::is_active);
    ClassDB::bind_method(D_METHOD("get_value_text"), &InputAction::get_value_text);
    ClassDB::bind_method(D_METHOD("get_device"), &InputAction::get_device);
    ClassDB::bind_method(D_METHOD("is_keyboard_mouse_allowed"), &InputAction::is_keyboard_mouse_allowed);
    ClassDB::bind_method(D_METHOD("is_gamepad_allowed"), &InputAction::is_gamepad_allowed);
    ClassDB::bind_method(D_METHOD("get_last_event"), &InputAction::get_last_event);
    ClassDB::bind_method(D_METHOD("get_event_count"), &InputAction::get_event_count);
    ClassDB::bind_method(D_METHOD("simulate", "value"), &InputAction::simulate);
    ClassDB::bind_method(D_METHOD("stop_simulation"), &InputAction::stop_simulation);
    ClassDB::bind_method(D_METHOD("is_simulated"), &InputAction::is_simulated);
    ClassDB::bind_method(D_METHOD("get_events"), &InputAction::get_events);
    ClassDB::bind_method(D_METHOD("get_device_properties"), &InputAction::get_device_properties);
    ClassDB::bind_method(D_METHOD("get_rebind_slots"), &InputAction::get_rebind_slots);
    ClassDB::bind_method(D_METHOD("apply_rebind", "slot", "event", "joy_threshold"), &InputAction::apply_rebind, DEFVAL(0.5));
    ClassDB::bind_method(D_METHOD("get_slot_text", "slot"), &InputAction::get_slot_text);
    ClassDB::bind_method(D_METHOD("get_device_state"), &InputAction::get_device_state);
    ClassDB::bind_method(D_METHOD("set_device_state", "state"), &InputAction::set_device_state);
    ClassDB::bind_method(D_METHOD("make_runtime_copy"), &InputAction::make_runtime_copy);

    ClassDB::bind_static_method("InputAction", D_METHOD("key_text", "key"), &InputAction::key_text);
    ClassDB::bind_static_method("InputAction", D_METHOD("mouse_button_text", "button"), &InputAction::mouse_button_text);
    ClassDB::bind_static_method("InputAction", D_METHOD("joy_button_text", "button"), &InputAction::joy_button_text);
    ClassDB::bind_static_method("InputAction", D_METHOD("joy_axis_text", "axis"), &InputAction::joy_axis_text);

    GDVIRTUAL_BIND(_update_action, "delta");
    GDVIRTUAL_BIND(_get_value);

    BIND_ENUM_CONSTANT(DEVICE_INHERIT);
    BIND_ENUM_CONSTANT(DEVICE_ALL);
    BIND_ENUM_CONSTANT(DEVICE_KEYBOARD_MOUSE);
    BIND_ENUM_CONSTANT(DEVICE_GAMEPAD);

    ADD_GROUP(U"🕹️ Action", "");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_name"), "set_action_name", "get_action_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description", "get_description");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "device_filter", PROPERTY_HINT_ENUM, U"Hériter du PlayerInput,Tous,Clavier et souris,Manette"), "set_device_filter", "get_device_filter");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "physical_keys"), "set_physical_keys", "get_physical_keys");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dead_zone", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_dead_zone", "get_dead_zone");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dirty_threshold", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_dirty_threshold", "get_dirty_threshold");
    ADD_GROUP(U"🔗 Liaisons", "");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "bindings", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":InputBinding"), "set_bindings", "get_bindings");
}
