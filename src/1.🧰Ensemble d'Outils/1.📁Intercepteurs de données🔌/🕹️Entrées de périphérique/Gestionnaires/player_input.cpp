#include "player_input.hpp"
#include "input_debugger.hpp"

#include "../Actions/Axiales/input_axis_actions.hpp"
#include "../Actions/input_button_action.hpp"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
constexpr int EVENT_LOG_SIZE = 60;
const char *const SAVE_SECTION = "controles";
} // namespace

PlayerInput::PlayerInput() {}

PlayerInput::~PlayerInput() {}

void PlayerInput::set_contexts(const TypedArray<InputContext> &p_contexts)
{
    contexts = p_contexts;
    if (set_up)
    {
        set_up = false; // contextes remplacés en jeu : nouvelles copies à la prochaine mise à jour
        setup();
    }
    update_configuration_warnings();
}

void PlayerInput::set_device_filter(InputAction::DeviceFilter p_filter) { device_filter = p_filter == InputAction::DEVICE_INHERIT ? InputAction::DEVICE_ALL : p_filter; }

void PlayerInput::set_process_in_physics(bool p_physics)
{
    process_in_physics = p_physics;
    if (is_inside_tree() && !Engine::get_singleton()->is_editor_hint())
    {
        set_process(!process_in_physics);
        set_physics_process(process_in_physics);
    }
}

// ⚙️ Mise à jour

void PlayerInput::setup()
{
    // Relâche les actions des anciens contextes de jeu avant de les remplacer.
    for (int i = 0; i < runtime_contexts.size(); i++)
    {
        Ref<InputContext> context = runtime_contexts[i];
        TypedArray<InputAction> actions = context.is_valid() ? context->get_actions() : TypedArray<InputAction>();
        for (int a = 0; a < actions.size(); a++)
        {
            Ref<InputAction> action = actions[a];
            if (action.is_valid())
            {
                action->release(this);
            }
        }
    }
    runtime_contexts.clear();
    for (int i = 0; i < contexts.size(); i++)
    {
        Ref<InputContext> context = contexts[i];
        if (context.is_valid())
        {
            runtime_contexts.push_back(runtime_copies ? context->make_runtime_copy() : context);
        }
    }
    active_states.clear();
    context_stack.clear();
    stack_previous_enabled.clear();
    warned_bindings.clear();
    set_up = true;
    default_bindings = get_bindings_state();
}

Vector<Ref<InputContext>> PlayerInput::_sorted_contexts() const
{
    Vector<Ref<InputContext>> sorted;
    TypedArray<InputContext> source = get_runtime_contexts();
    for (int i = 0; i < source.size(); i++)
    {
        Ref<InputContext> context = source[i];
        if (context.is_valid())
        {
            sorted.push_back(context);
        }
    }
    // Tri stable par priorité effective : les contextes empilés passent devant (le dernier empilé en tête).
    auto effective = [this](const Ref<InputContext> &p_context) -> int64_t {
        int64_t index = context_stack.find(p_context->get_context_name());
        return index >= 0 ? int64_t(1) << 40 | index : int64_t(p_context->get_priority());
    };
    for (int i = 1; i < sorted.size(); i++)
    {
        Ref<InputContext> current = sorted[i];
        int64_t key = effective(current);
        int j = i - 1;
        while (j >= 0 && effective(sorted[j]) < key)
        {
            sorted.set(j + 1, sorted[j]);
            j--;
        }
        sorted.set(j + 1, current);
    }
    return sorted;
}

void PlayerInput::poll(double p_delta)
{
    if (!set_up)
    {
        setup();
    }
    InputReadContext read_context;
    read_context.device = device;
    read_context.keyboard_mouse = device_filter != InputAction::DEVICE_GAMEPAD;
    read_context.gamepad = device_filter != InputAction::DEVICE_KEYBOARD_MOUSE;

    bool blocked = false;
    Vector<Ref<InputContext>> sorted = _sorted_contexts();
    for (const Ref<InputContext> &context : sorted)
    {
        StringName name = context->get_context_name();
        bool stacked = context_stack.has(name);
        bool active = context->is_enabled() && !blocked;
        bool was_active = active_states.get(name, false);
        if (active != was_active || !active_states.has(name))
        {
            active_states[name] = active;
            if (active != was_active)
            {
                emit_signal("context_changed", name, active);
            }
        }
        TypedArray<InputAction> actions = context->get_actions();
        for (int a = 0; a < actions.size(); a++)
        {
            Ref<InputAction> action = actions[a];
            if (action.is_null())
            {
                continue;
            }
            if (active)
            {
                action->update(read_context, p_delta, this);
            }
            else
            {
                action->release(this);
            }
        }
        if (active && (context->is_exclusive() || stacked))
        {
            blocked = true;
        }
    }
    if (InputDebugger *debugger = InputDebugger::get_singleton())
    {
        debugger->notify(this);
    }
}

void PlayerInput::_input(const Ref<InputEvent> &p_event)
{
    if (!Engine::get_singleton()->is_editor_hint())
    {
        handle_input_event(p_event);
    }
}

void PlayerInput::handle_input_event(const Ref<InputEvent> &p_event)
{
    if (p_event.is_null())
    {
        return;
    }
    // 🔎 Type de périphérique utilisé
    InputAction::DeviceFilter type = InputAction::DEVICE_INHERIT;
    if (Object::cast_to<InputEventKey>(p_event.ptr()) || Object::cast_to<InputEventMouseButton>(p_event.ptr()))
    {
        type = InputAction::DEVICE_KEYBOARD_MOUSE;
    }
    else if (InputEventMouseMotion *motion = Object::cast_to<InputEventMouseMotion>(p_event.ptr()))
    {
        type = motion->get_relative().length_squared() > 4.0f ? InputAction::DEVICE_KEYBOARD_MOUSE : InputAction::DEVICE_INHERIT;
    }
    else if (InputEventJoypadButton *button = Object::cast_to<InputEventJoypadButton>(p_event.ptr()))
    {
        type = button->get_device() == device && button->is_pressed() ? InputAction::DEVICE_GAMEPAD : InputAction::DEVICE_INHERIT;
    }
    else if (InputEventJoypadMotion *joy_motion = Object::cast_to<InputEventJoypadMotion>(p_event.ptr()))
    {
        type = joy_motion->get_device() == device && Math::abs(joy_motion->get_axis_value()) >= joy_threshold ? InputAction::DEVICE_GAMEPAD : InputAction::DEVICE_INHERIT;
    }
    if (type != InputAction::DEVICE_INHERIT && type != last_device_type)
    {
        last_device_type = type;
        if (auto_switch_device)
        {
            device_filter = type;
        }
        emit_signal("device_type_changed", int(type));
    }
    // 🖱️ Évènements bruts transmis aux actions des contextes actifs
    if (!set_up)
    {
        return;
    }
    TypedArray<InputContext> source = runtime_contexts;
    for (int i = 0; i < source.size(); i++)
    {
        Ref<InputContext> context = source[i];
        if (context.is_null() || !bool(active_states.get(context->get_context_name(), false)))
        {
            continue;
        }
        TypedArray<InputAction> actions = context->get_actions();
        for (int a = 0; a < actions.size(); a++)
        {
            Ref<InputAction> action = actions[a];
            if (action.is_valid())
            {
                action->handle_event(p_event, this);
            }
        }
    }
}

void PlayerInput::dispatch_event(InputAction *p_action, const StringName &p_event, const Array &p_arguments)
{
    if (!p_action)
    {
        return;
    }
    PackedStringArray calls;
    TypedArray<InputBinding> bindings = p_action->get_bindings();
    for (int i = 0; i < bindings.size(); i++)
    {
        Ref<InputBinding> binding = bindings[i];
        if (binding.is_null() || !binding->is_enabled() || binding->get_event() != p_event)
        {
            continue;
        }
        calls.push_back(binding->get_description());
        if (!dispatch_bindings)
        {
            continue; // essai à blanc (débogage dans l'éditeur) : la liaison est seulement décrite
        }
        Node *target = binding->get_target().is_empty() ? nullptr : get_node_or_null(binding->get_target());
        if (!target)
        {
            _report_binding_failure(p_action, binding, String(U"cible introuvable : ") + String(binding->get_target()));
            continue;
        }
        if (binding->get_method() == StringName() || !target->has_method(binding->get_method()))
        {
            _report_binding_failure(p_action, binding, String(U"méthode introuvable : ") + String(target->get_name()) + "." + String(binding->get_method()));
            continue;
        }
        target->callv(binding->get_method(), binding->build_call_arguments(p_arguments));
    }
    _log_event(p_action, p_event, p_arguments, calls);
    emit_signal("action_triggered", p_action->get_action_name(), p_event, p_arguments);
}

void PlayerInput::_report_binding_failure(InputAction *p_action, const Ref<InputBinding> &p_binding, const String &p_reason)
{
    emit_signal("binding_failed", p_action->get_action_name(), p_binding, p_reason);
    uint64_t id = p_binding->get_instance_id();
    if (!warned_bindings.has(id))
    {
        warned_bindings.insert(id);
        UtilityFunctions::push_warning(String("PlayerInput ") + String(get_name()) + String(U" : liaison de l'action « ") + String(p_action->get_action_name()) + String(U" » ignorée (") + p_reason + ").");
    }
}

void PlayerInput::_log_event(InputAction *p_action, const StringName &p_event, const Array &p_arguments, const PackedStringArray &p_calls)
{
    Dictionary entry;
    entry["msec"] = int64_t(Time::get_singleton()->get_ticks_msec());
    entry["action"] = p_action->get_action_name();
    entry["event"] = p_event;
    PackedStringArray arguments;
    for (int i = 0; i < p_arguments.size(); i++)
    {
        arguments.push_back(p_arguments[i].stringify());
    }
    entry["arguments"] = String(", ").join(arguments);
    entry["calls"] = p_calls;
    event_log.push_back(entry);
    while (event_log.size() > EVENT_LOG_SIZE)
    {
        event_log.pop_front();
    }
}

// 🗂️ Contextes

Ref<InputContext> PlayerInput::get_context(const StringName &p_name) const
{
    TypedArray<InputContext> source = get_runtime_contexts();
    for (int i = 0; i < source.size(); i++)
    {
        Ref<InputContext> context = source[i];
        if (context.is_valid() && context->get_context_name() == p_name)
        {
            return context;
        }
    }
    return Ref<InputContext>();
}

void PlayerInput::set_context_enabled(const StringName &p_name, bool p_enabled)
{
    Ref<InputContext> context = get_context(p_name);
    ERR_FAIL_COND_MSG(context.is_null(), String(U"PlayerInput : contexte inconnu « ") + String(p_name) + U" ».");
    context->set_enabled(p_enabled);
}

bool PlayerInput::is_context_enabled(const StringName &p_name) const
{
    Ref<InputContext> context = get_context(p_name);
    return context.is_valid() && context->is_enabled();
}

bool PlayerInput::is_context_active(const StringName &p_name) const { return active_states.get(p_name, false); }

void PlayerInput::enable_only(const StringName &p_name)
{
    TypedArray<InputContext> source = get_runtime_contexts();
    for (int i = 0; i < source.size(); i++)
    {
        Ref<InputContext> context = source[i];
        if (context.is_valid())
        {
            context->set_enabled(context->get_context_name() == p_name);
        }
    }
}

PackedStringArray PlayerInput::get_active_contexts() const
{
    PackedStringArray names;
    Vector<Ref<InputContext>> sorted = _sorted_contexts();
    for (const Ref<InputContext> &context : sorted)
    {
        if (bool(active_states.get(context->get_context_name(), false)))
        {
            names.push_back(context->get_context_name());
        }
    }
    return names;
}

void PlayerInput::push_context(const StringName &p_name)
{
    Ref<InputContext> context = get_context(p_name);
    ERR_FAIL_COND_MSG(context.is_null(), String(U"PlayerInput : contexte inconnu « ") + String(p_name) + U" ».");
    int64_t index = context_stack.find(p_name);
    if (index >= 0)
    {
        context_stack.remove_at(index); // déjà empilé : remonte en tête
    }
    else
    {
        stack_previous_enabled[p_name] = context->is_enabled();
    }
    context_stack.push_back(p_name);
    context->set_enabled(true);
}

StringName PlayerInput::pop_context()
{
    if (context_stack.is_empty())
    {
        return StringName();
    }
    StringName name = context_stack[context_stack.size() - 1];
    context_stack.remove_at(context_stack.size() - 1);
    Ref<InputContext> context = get_context(name);
    if (context.is_valid())
    {
        context->set_enabled(stack_previous_enabled.get(name, true));
    }
    stack_previous_enabled.erase(name);
    return name;
}

PackedStringArray PlayerInput::get_context_stack() const { return context_stack; }

// 🕹️ Actions

Ref<InputAction> PlayerInput::get_action(const StringName &p_name) const
{
    Ref<InputAction> fallback;
    Vector<Ref<InputContext>> sorted = _sorted_contexts();
    for (const Ref<InputContext> &context : sorted)
    {
        Ref<InputAction> action = context->get_action(p_name);
        if (action.is_null())
        {
            continue;
        }
        if (bool(active_states.get(context->get_context_name(), false)))
        {
            return action;
        }
        if (fallback.is_null())
        {
            fallback = action;
        }
    }
    return fallback;
}

Variant PlayerInput::get_value(const StringName &p_action) const
{
    Ref<InputAction> action = get_action(p_action);
    return action.is_valid() ? action->get_value() : Variant();
}

bool PlayerInput::is_pressed(const StringName &p_action) const
{
    Ref<InputAction> action = get_action(p_action);
    return action.is_valid() && action->is_active();
}

bool PlayerInput::is_just_pressed(const StringName &p_action) const
{
    Ref<InputButtonAction> button = get_action(p_action);
    return button.is_valid() && button->is_just_pressed();
}

bool PlayerInput::is_just_released(const StringName &p_action) const
{
    Ref<InputButtonAction> button = get_action(p_action);
    return button.is_valid() && button->is_just_released();
}

double PlayerInput::get_axis(const StringName &p_action) const
{
    Variant value = get_value(p_action);
    switch (value.get_type())
    {
    case Variant::FLOAT:
    case Variant::INT:
        return value;
    case Variant::BOOL:
        return bool(value) ? 1.0 : 0.0;
    case Variant::VECTOR2:
        return Vector2(value).x;
    default:
        return 0.0;
    }
}

Vector2 PlayerInput::get_vector(const StringName &p_action) const
{
    Variant value = get_value(p_action);
    if (value.get_type() == Variant::VECTOR3)
    {
        Vector3 v = value;
        return Vector2(v.x, v.z);
    }
    return value.get_type() == Variant::VECTOR2 ? Vector2(value) : Vector2();
}

Vector3 PlayerInput::get_vector3(const StringName &p_action) const
{
    Variant value = get_value(p_action);
    if (value.get_type() == Variant::VECTOR2)
    {
        Vector2 v = value;
        return Vector3(v.x, 0.0f, v.y);
    }
    return value.get_type() == Variant::VECTOR3 ? Vector3(value) : Vector3();
}

void PlayerInput::simulate(const StringName &p_action, const Variant &p_value)
{
    if (!set_up)
    {
        setup();
    }
    Ref<InputAction> action = get_action(p_action);
    ERR_FAIL_COND_MSG(action.is_null(), String(U"PlayerInput : action inconnue « ") + String(p_action) + U" ».");
    action->simulate(p_value);
}

void PlayerInput::stop_simulation(const StringName &p_action)
{
    Ref<InputAction> action = get_action(p_action);
    if (action.is_valid())
    {
        action->stop_simulation();
    }
}

void PlayerInput::stop_all_simulations()
{
    Array all = get_all_actions();
    for (int i = 0; i < all.size(); i++)
    {
        Ref<InputAction> action = Array(all[i])[1];
        action->stop_simulation();
    }
}

// 🎮 Périphérique

String PlayerInput::get_joy_name() const { return Input::get_singleton()->get_joy_name(device); }

String PlayerInput::get_joy_guid() const { return Input::get_singleton()->get_joy_guid(device); }

Dictionary PlayerInput::get_joy_info() const { return is_device_connected() ? Input::get_singleton()->get_joy_info(device) : Dictionary(); }

bool PlayerInput::is_device_connected() const { return Input::get_singleton()->get_connected_joypads().has(device); }

void PlayerInput::start_vibration(double p_weak, double p_strong, double p_duration) { Input::get_singleton()->start_joy_vibration(device, p_weak, p_strong, p_duration); }

void PlayerInput::stop_vibration() { Input::get_singleton()->stop_joy_vibration(device); }

// 💾 Contrôles

Array PlayerInput::get_all_actions() const
{
    Array result;
    TypedArray<InputContext> source = get_runtime_contexts();
    for (int i = 0; i < source.size(); i++)
    {
        Ref<InputContext> context = source[i];
        TypedArray<InputAction> actions = context.is_valid() ? context->get_actions() : TypedArray<InputAction>();
        for (int a = 0; a < actions.size(); a++)
        {
            Ref<InputAction> action = actions[a];
            if (action.is_valid())
            {
                result.push_back(Array::make(context, action));
            }
        }
    }
    return result;
}

Dictionary PlayerInput::get_bindings_state() const
{
    Dictionary state;
    Array all = get_all_actions();
    for (int i = 0; i < all.size(); i++)
    {
        Array pair = all[i];
        Ref<InputContext> context = pair[0];
        Ref<InputAction> action = pair[1];
        Dictionary device_state = action->get_device_state();
        if (!device_state.is_empty())
        {
            state[String(context->get_context_name()) + "/" + String(action->get_action_name())] = device_state;
        }
    }
    return state;
}

void PlayerInput::apply_bindings_state(const Dictionary &p_state)
{
    Array all = get_all_actions();
    for (int i = 0; i < all.size(); i++)
    {
        Array pair = all[i];
        Ref<InputContext> context = pair[0];
        Ref<InputAction> action = pair[1];
        String key = String(context->get_context_name()) + "/" + String(action->get_action_name());
        if (p_state.has(key))
        {
            action->set_device_state(p_state[key]);
        }
    }
    emit_signal("bindings_changed");
}

void PlayerInput::reset_bindings()
{
    if (!set_up)
    {
        setup();
    }
    apply_bindings_state(default_bindings);
}

Error PlayerInput::save_bindings(const String &p_path) const
{
    Ref<ConfigFile> file;
    file.instantiate();
    Dictionary state = get_bindings_state();
    Array keys = state.keys();
    for (int i = 0; i < keys.size(); i++)
    {
        file->set_value(SAVE_SECTION, keys[i], state[keys[i]]);
    }
    return file->save(p_path);
}

Error PlayerInput::load_bindings(const String &p_path)
{
    Ref<ConfigFile> file;
    file.instantiate();
    Error error = file->load(p_path);
    if (error != OK)
    {
        return error;
    }
    if (!set_up)
    {
        setup();
    }
    Dictionary state;
    if (file->has_section(SAVE_SECTION))
    {
        PackedStringArray keys = file->get_section_keys(SAVE_SECTION);
        for (const String &key : keys)
        {
            state[key] = file->get_value(SAVE_SECTION, key);
        }
    }
    apply_bindings_state(state);
    return OK;
}

// 🐞 Débogage

Dictionary PlayerInput::get_debug_snapshot() const
{
    Dictionary snapshot;
    snapshot["name"] = get_name();
    snapshot["device"] = device;
    snapshot["device_filter"] = int(device_filter);
    snapshot["last_device_type"] = int(last_device_type);
    snapshot["joy_name"] = get_joy_name();
    snapshot["stack"] = context_stack;
    Array context_list;
    Vector<Ref<InputContext>> sorted = _sorted_contexts();
    for (const Ref<InputContext> &context : sorted)
    {
        Dictionary entry;
        entry["name"] = context->get_context_name();
        entry["enabled"] = context->is_enabled();
        entry["active"] = active_states.get(context->get_context_name(), false);
        entry["priority"] = context->get_priority();
        entry["exclusive"] = context->is_exclusive();
        Array action_list;
        TypedArray<InputAction> actions = context->get_actions();
        for (int a = 0; a < actions.size(); a++)
        {
            Ref<InputAction> action = actions[a];
            if (action.is_null())
            {
                continue;
            }
            Dictionary action_entry;
            action_entry["name"] = action->get_action_name();
            action_entry["class"] = action->get_class();
            action_entry["value"] = action->get_value();
            action_entry["text"] = action->get_value_text();
            action_entry["active"] = action->is_active();
            action_entry["enabled"] = action->is_enabled();
            action_entry["event"] = action->get_last_event();
            action_entry["count"] = action->get_event_count();
            action_entry["simulated"] = action->is_simulated();
            action_entry["bindings"] = action->get_bindings().size();
            action_list.push_back(action_entry);
        }
        entry["actions"] = action_list;
        context_list.push_back(entry);
    }
    snapshot["contexts"] = context_list;
    snapshot["log"] = event_log;
    return snapshot;
}

PackedStringArray PlayerInput::get_configuration_issues() const
{
    PackedStringArray warnings;
    if (contexts.is_empty())
    {
        warnings.push_back(U"Aucun contexte : ajoutez un InputContext (ou ouvrez l'Éditeur d'entrées depuis l'inspecteur).");
    }
    PackedStringArray context_names;
    for (int i = 0; i < contexts.size(); i++)
    {
        Ref<InputContext> context = contexts[i];
        if (context.is_null())
        {
            warnings.push_back(String(U"Contexte n° ") + String::num_int64(i) + U" vide.");
            continue;
        }
        if (context_names.has(context->get_context_name()))
        {
            warnings.push_back(String(U"Deux contextes s'appellent « ") + String(context->get_context_name()) + U" ».");
        }
        context_names.push_back(context->get_context_name());
        PackedStringArray action_names;
        TypedArray<InputAction> actions = context->get_actions();
        for (int a = 0; a < actions.size(); a++)
        {
            Ref<InputAction> action = actions[a];
            if (action.is_null())
            {
                continue;
            }
            if (action_names.has(action->get_action_name()))
            {
                warnings.push_back(String(U"Contexte « ") + String(context->get_context_name()) + U" » : deux actions s'appellent « " + String(action->get_action_name()) + U" ».");
            }
            action_names.push_back(action->get_action_name());
            TypedArray<InputBinding> bindings = action->get_bindings();
            for (int b = 0; b < bindings.size(); b++)
            {
                Ref<InputBinding> binding = bindings[b];
                if (binding.is_valid() && binding->is_enabled() && is_inside_tree() && (binding->get_target().is_empty() || !get_node_or_null(binding->get_target())))
                {
                    warnings.push_back(String(U"Action « ") + String(action->get_action_name()) + U" » : la cible de la liaison « " + binding->get_description() + U" » est introuvable.");
                }
            }
        }
    }
    return warnings;
}

void PlayerInput::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    switch (p_what)
    {
    case NOTIFICATION_ENTER_TREE:
        if (InputDebugger *debugger = InputDebugger::get_singleton())
        {
            debugger->register_player(this);
        }
        break;
    case NOTIFICATION_EXIT_TREE:
        if (InputDebugger *debugger = InputDebugger::get_singleton())
        {
            debugger->unregister_player(this);
        }
        break;
    case NOTIFICATION_READY:
        if (!set_up)
        {
            setup();
        }
        set_process(!process_in_physics);
        set_physics_process(process_in_physics);
        set_process_input(true);
        break;
    case NOTIFICATION_PROCESS:
        poll(get_process_delta_time());
        break;
    case NOTIFICATION_PHYSICS_PROCESS:
        poll(get_physics_process_delta_time());
        break;
    default:
        break;
    }
}

void PlayerInput::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_contexts", "contexts"), &PlayerInput::set_contexts);
    ClassDB::bind_method(D_METHOD("get_contexts"), &PlayerInput::get_contexts);
    ClassDB::bind_method(D_METHOD("set_device", "device"), &PlayerInput::set_device);
    ClassDB::bind_method(D_METHOD("get_device"), &PlayerInput::get_device);
    ClassDB::bind_method(D_METHOD("set_device_filter", "filter"), &PlayerInput::set_device_filter);
    ClassDB::bind_method(D_METHOD("get_device_filter"), &PlayerInput::get_device_filter);
    ClassDB::bind_method(D_METHOD("set_auto_switch_device", "auto_switch"), &PlayerInput::set_auto_switch_device);
    ClassDB::bind_method(D_METHOD("get_auto_switch_device"), &PlayerInput::get_auto_switch_device);
    ClassDB::bind_method(D_METHOD("set_joy_threshold", "threshold"), &PlayerInput::set_joy_threshold);
    ClassDB::bind_method(D_METHOD("get_joy_threshold"), &PlayerInput::get_joy_threshold);
    ClassDB::bind_method(D_METHOD("set_process_in_physics", "physics"), &PlayerInput::set_process_in_physics);
    ClassDB::bind_method(D_METHOD("get_process_in_physics"), &PlayerInput::get_process_in_physics);
    ClassDB::bind_method(D_METHOD("set_dispatch_bindings", "dispatch"), &PlayerInput::set_dispatch_bindings);
    ClassDB::bind_method(D_METHOD("get_dispatch_bindings"), &PlayerInput::get_dispatch_bindings);
    ClassDB::bind_method(D_METHOD("set_runtime_copies", "copies"), &PlayerInput::set_runtime_copies);
    ClassDB::bind_method(D_METHOD("get_runtime_copies"), &PlayerInput::get_runtime_copies);

    ClassDB::bind_method(D_METHOD("setup"), &PlayerInput::setup);
    ClassDB::bind_method(D_METHOD("is_setup"), &PlayerInput::is_setup);
    ClassDB::bind_method(D_METHOD("poll", "delta"), &PlayerInput::poll);
    ClassDB::bind_method(D_METHOD("handle_input_event", "event"), &PlayerInput::handle_input_event);
    ClassDB::bind_method(D_METHOD("get_runtime_contexts"), &PlayerInput::get_runtime_contexts);
    ClassDB::bind_method(D_METHOD("get_context", "name"), &PlayerInput::get_context);
    ClassDB::bind_method(D_METHOD("set_context_enabled", "name", "enabled"), &PlayerInput::set_context_enabled);
    ClassDB::bind_method(D_METHOD("is_context_enabled", "name"), &PlayerInput::is_context_enabled);
    ClassDB::bind_method(D_METHOD("is_context_active", "name"), &PlayerInput::is_context_active);
    ClassDB::bind_method(D_METHOD("enable_only", "name"), &PlayerInput::enable_only);
    ClassDB::bind_method(D_METHOD("get_active_contexts"), &PlayerInput::get_active_contexts);
    ClassDB::bind_method(D_METHOD("push_context", "name"), &PlayerInput::push_context);
    ClassDB::bind_method(D_METHOD("pop_context"), &PlayerInput::pop_context);
    ClassDB::bind_method(D_METHOD("get_context_stack"), &PlayerInput::get_context_stack);

    ClassDB::bind_method(D_METHOD("get_action", "name"), &PlayerInput::get_action);
    ClassDB::bind_method(D_METHOD("get_value", "action"), &PlayerInput::get_value);
    ClassDB::bind_method(D_METHOD("is_pressed", "action"), &PlayerInput::is_pressed);
    ClassDB::bind_method(D_METHOD("is_just_pressed", "action"), &PlayerInput::is_just_pressed);
    ClassDB::bind_method(D_METHOD("is_just_released", "action"), &PlayerInput::is_just_released);
    ClassDB::bind_method(D_METHOD("get_axis", "action"), &PlayerInput::get_axis);
    ClassDB::bind_method(D_METHOD("get_vector", "action"), &PlayerInput::get_vector);
    ClassDB::bind_method(D_METHOD("get_vector3", "action"), &PlayerInput::get_vector3);
    ClassDB::bind_method(D_METHOD("simulate", "action", "value"), &PlayerInput::simulate);
    ClassDB::bind_method(D_METHOD("stop_simulation", "action"), &PlayerInput::stop_simulation);
    ClassDB::bind_method(D_METHOD("stop_all_simulations"), &PlayerInput::stop_all_simulations);

    ClassDB::bind_method(D_METHOD("get_last_device_type"), &PlayerInput::get_last_device_type);
    ClassDB::bind_method(D_METHOD("get_joy_name"), &PlayerInput::get_joy_name);
    ClassDB::bind_method(D_METHOD("get_joy_guid"), &PlayerInput::get_joy_guid);
    ClassDB::bind_method(D_METHOD("get_joy_info"), &PlayerInput::get_joy_info);
    ClassDB::bind_method(D_METHOD("is_device_connected"), &PlayerInput::is_device_connected);
    ClassDB::bind_method(D_METHOD("start_vibration", "weak_magnitude", "strong_magnitude", "duration"), &PlayerInput::start_vibration, DEFVAL(1.0), DEFVAL(1.0), DEFVAL(0.0));
    ClassDB::bind_method(D_METHOD("stop_vibration"), &PlayerInput::stop_vibration);

    ClassDB::bind_method(D_METHOD("get_bindings_state"), &PlayerInput::get_bindings_state);
    ClassDB::bind_method(D_METHOD("apply_bindings_state", "state"), &PlayerInput::apply_bindings_state);
    ClassDB::bind_method(D_METHOD("reset_bindings"), &PlayerInput::reset_bindings);
    ClassDB::bind_method(D_METHOD("save_bindings", "path"), &PlayerInput::save_bindings);
    ClassDB::bind_method(D_METHOD("load_bindings", "path"), &PlayerInput::load_bindings);
    ClassDB::bind_method(D_METHOD("get_all_actions"), &PlayerInput::get_all_actions);
    ClassDB::bind_method(D_METHOD("get_debug_snapshot"), &PlayerInput::get_debug_snapshot);
    ClassDB::bind_method(D_METHOD("get_configuration_issues"), &PlayerInput::get_configuration_issues);
    ClassDB::bind_method(D_METHOD("get_event_log"), &PlayerInput::get_event_log);
    ClassDB::bind_method(D_METHOD("clear_event_log"), &PlayerInput::clear_event_log);

    ADD_SIGNAL(MethodInfo("action_triggered", PropertyInfo(Variant::STRING_NAME, "action"), PropertyInfo(Variant::STRING_NAME, "event"), PropertyInfo(Variant::ARRAY, "arguments")));
    ADD_SIGNAL(MethodInfo("context_changed", PropertyInfo(Variant::STRING_NAME, "context"), PropertyInfo(Variant::BOOL, "active")));
    ADD_SIGNAL(MethodInfo("device_type_changed", PropertyInfo(Variant::INT, "device_type")));
    ADD_SIGNAL(MethodInfo("binding_failed", PropertyInfo(Variant::STRING_NAME, "action"), PropertyInfo(Variant::OBJECT, "binding", PROPERTY_HINT_RESOURCE_TYPE, "InputBinding"), PropertyInfo(Variant::STRING, "reason")));
    ADD_SIGNAL(MethodInfo("bindings_changed"));

    ADD_GROUP(U"🗂️ Contextes", "");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "contexts", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":InputContext"), "set_contexts", "get_contexts");
    ADD_GROUP(U"🎮 Périphérique", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "device", PROPERTY_HINT_RANGE, "0,16,1,or_greater"), "set_device", "get_device");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "device_filter", PROPERTY_HINT_ENUM, U"Tous:1,Clavier et souris:2,Manette:3"), "set_device_filter", "get_device_filter");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_switch_device"), "set_auto_switch_device", "get_auto_switch_device");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "joy_threshold", PROPERTY_HINT_RANGE, "0,1,0.05"), "set_joy_threshold", "get_joy_threshold");
    ADD_GROUP(U"⚙️ Exécution", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "process_in_physics"), "set_process_in_physics", "get_process_in_physics");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dispatch_bindings"), "set_dispatch_bindings", "get_dispatch_bindings");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "runtime_copies"), "set_runtime_copies", "get_runtime_copies");
}
