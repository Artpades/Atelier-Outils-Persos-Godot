#include "input_axis_actions.hpp"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace
{
/// Valeur d'axe hors zone morte, remise à l'échelle sur [0, 1] (pas de saut au bord de la zone morte).
double rescale(double p_value, double p_dead_zone)
{
    double magnitude = Math::abs(p_value);
    if (magnitude <= p_dead_zone)
    {
        return 0.0;
    }
    return SIGN(p_value) * MIN(1.0, (magnitude - p_dead_zone) / MAX(1e-6, 1.0 - p_dead_zone));
}

template <typename V>
V rescale_radial(const V &p_value, double p_dead_zone)
{
    double length = p_value.length();
    if (length <= p_dead_zone)
    {
        return V();
    }
    return p_value / length * MIN(1.0, (length - p_dead_zone) / MAX(1e-6, 1.0 - p_dead_zone));
}

Dictionary make_slot(const char *p_slot, const String &p_label, const char *p_group)
{
    Dictionary slot;
    slot["slot"] = p_slot;
    slot["label"] = p_label;
    slot["group"] = p_group;
    return slot;
}

/// Réassignation selon le nom de l'emplacement : *_key ← touche, *axis* ← axe de manette, sinon ← bouton.
bool assign_slot(InputAction *p_action, const StringName &p_slot, const Ref<InputEvent> &p_event, double p_threshold)
{
    String slot = p_slot;
    int value = -1;
    if (slot.ends_with("_key"))
    {
        value = InputAction::event_key(p_event, p_action->get_physical_keys());
        if (value == 0)
        {
            return false;
        }
    }
    else if (slot.contains("axis"))
    {
        value = InputAction::event_joy_axis(p_event, p_threshold);
    }
    else
    {
        value = InputAction::event_joy_button(p_event);
    }
    if (value < 0)
    {
        return false;
    }
    p_action->set(p_slot, value);
    p_action->emit_changed();
    return true;
}

/// Émission « propre » d'une valeur : variation d'au moins p_threshold, ou retour exact au repos.
template <typename V>
bool should_emit(const V &p_value, const V &p_last, double p_threshold)
{
    return p_value != p_last && ((p_value - p_last).length() >= p_threshold || p_value == V());
}

bool should_emit_float(double p_value, double p_last, double p_threshold)
{
    return p_value != p_last && (Math::abs(p_value - p_last) >= p_threshold || p_value == 0.0);
}
} // namespace

// --- InputAxisAction ---

void InputAxisAction::_update(double p_delta)
{
    double result = rescale(joy_axis_value(joy_axis), get_dead_zone());
    if (result == 0.0)
    {
        result += key_down(positive_key) ? 1.0 : 0.0;
        result -= key_down(negative_key) ? 1.0 : 0.0;
        result += joy_button_down(positive_joy_button) ? 1.0 : 0.0;
        result -= joy_button_down(negative_joy_button) ? 1.0 : 0.0;
        result = CLAMP(result, -1.0, 1.0);
    }
    _process(invert ? -result : result, p_delta);
}

void InputAxisAction::_apply_simulation(const Variant &p_value, double p_delta) { _process(CLAMP(double(p_value), -1.0, 1.0), p_delta); }

void InputAxisAction::_release()
{
    if (value != 0.0 || velocity != 0.0)
    {
        _process(0.0, 0.0);
    }
}

void InputAxisAction::_process(double p_value, double p_delta)
{
    double new_velocity = p_delta > 0.0 ? (p_value - value) / p_delta : 0.0;
    value = p_value;
    velocity = new_velocity;
    if (should_emit_float(value, last_emitted, get_dirty_threshold()))
    {
        last_emitted = value;
        emit_event("value_changed", Array::make(value));
    }
    if (should_emit_float(velocity, last_emitted_velocity, get_dirty_threshold()))
    {
        last_emitted_velocity = velocity;
        emit_event("velocity_changed", Array::make(velocity));
    }
}

PackedStringArray InputAxisAction::get_device_properties() const
{
    PackedStringArray properties;
    for (const char *name : {"positive_key", "negative_key", "joy_axis", "positive_joy_button", "negative_joy_button", "invert"})
    {
        properties.push_back(name);
    }
    return properties;
}

TypedArray<Dictionary> InputAxisAction::get_rebind_slots() const
{
    TypedArray<Dictionary> slots;
    slots.push_back(make_slot("positive_key", U"Touche +", "kbm"));
    slots.push_back(make_slot("negative_key", U"Touche −", "kbm"));
    slots.push_back(make_slot("joy_axis", U"Axe", "gamepad"));
    slots.push_back(make_slot("positive_joy_button", U"Bouton +", "gamepad"));
    slots.push_back(make_slot("negative_joy_button", U"Bouton −", "gamepad"));
    return slots;
}

bool InputAxisAction::apply_rebind(const StringName &p_slot, const Ref<InputEvent> &p_event, double p_joy_threshold)
{
    if (!assign_slot(this, p_slot, p_event, p_joy_threshold))
    {
        return false;
    }
    // Un axe remplace les deux boutons, un bouton remplace l'axe.
    if (p_slot == StringName("joy_axis"))
    {
        positive_joy_button = -1;
        negative_joy_button = -1;
    }
    else if (String(p_slot).ends_with("joy_button"))
    {
        joy_axis = -1;
    }
    return true;
}

void InputAxisAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_positive_key", "key"), &InputAxisAction::set_positive_key);
    ClassDB::bind_method(D_METHOD("get_positive_key"), &InputAxisAction::get_positive_key);
    ClassDB::bind_method(D_METHOD("set_negative_key", "key"), &InputAxisAction::set_negative_key);
    ClassDB::bind_method(D_METHOD("get_negative_key"), &InputAxisAction::get_negative_key);
    ClassDB::bind_method(D_METHOD("set_joy_axis", "axis"), &InputAxisAction::set_joy_axis);
    ClassDB::bind_method(D_METHOD("get_joy_axis"), &InputAxisAction::get_joy_axis);
    ClassDB::bind_method(D_METHOD("set_positive_joy_button", "button"), &InputAxisAction::set_positive_joy_button);
    ClassDB::bind_method(D_METHOD("get_positive_joy_button"), &InputAxisAction::get_positive_joy_button);
    ClassDB::bind_method(D_METHOD("set_negative_joy_button", "button"), &InputAxisAction::set_negative_joy_button);
    ClassDB::bind_method(D_METHOD("get_negative_joy_button"), &InputAxisAction::get_negative_joy_button);
    ClassDB::bind_method(D_METHOD("set_invert", "invert"), &InputAxisAction::set_invert);
    ClassDB::bind_method(D_METHOD("get_invert"), &InputAxisAction::get_invert);
    ClassDB::bind_method(D_METHOD("get_axis"), &InputAxisAction::get_axis);
    ClassDB::bind_method(D_METHOD("get_velocity"), &InputAxisAction::get_velocity);

    ADD_SIGNAL(MethodInfo("value_changed", PropertyInfo(Variant::FLOAT, "value")));
    ADD_SIGNAL(MethodInfo("velocity_changed", PropertyInfo(Variant::FLOAT, "velocity")));

    ADD_GROUP(U"⌨️ Clavier", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "positive_key", PROPERTY_HINT_ENUM, key_hint()), "set_positive_key", "get_positive_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "negative_key", PROPERTY_HINT_ENUM, key_hint()), "set_negative_key", "get_negative_key");
    ADD_GROUP(U"🎮 Manette", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_axis", PROPERTY_HINT_ENUM, joy_axis_hint()), "set_joy_axis", "get_joy_axis");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "positive_joy_button", PROPERTY_HINT_ENUM, joy_button_hint()), "set_positive_joy_button", "get_positive_joy_button");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "negative_joy_button", PROPERTY_HINT_ENUM, joy_button_hint()), "set_negative_joy_button", "get_negative_joy_button");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "invert"), "set_invert", "get_invert");
}

// --- InputCircleAction ---

InputCircleAction::InputCircleAction()
{
    set_key_preset(KEYS_WASD);
    set_joy_preset(JOY_LEFT_STICK);
    set_dpad_preset(DPAD_CROSS);
}

void InputCircleAction::set_key_preset(KeyPreset p_preset)
{
    switch (p_preset)
    {
    case KEYS_WASD:
        up_key = KEY_W, down_key = KEY_S, left_key = KEY_A, right_key = KEY_D;
        break;
    case KEYS_ZQSD:
        up_key = KEY_Z, down_key = KEY_S, left_key = KEY_Q, right_key = KEY_D;
        break;
    case KEYS_ARROWS:
        up_key = KEY_UP, down_key = KEY_DOWN, left_key = KEY_LEFT, right_key = KEY_RIGHT;
        break;
    default:
        return;
    }
    notify_property_list_changed();
}

InputCircleAction::KeyPreset InputCircleAction::get_key_preset() const
{
    if (up_key == KEY_W && down_key == KEY_S && left_key == KEY_A && right_key == KEY_D)
    {
        return KEYS_WASD;
    }
    if (up_key == KEY_Z && down_key == KEY_S && left_key == KEY_Q && right_key == KEY_D)
    {
        return KEYS_ZQSD;
    }
    if (up_key == KEY_UP && down_key == KEY_DOWN && left_key == KEY_LEFT && right_key == KEY_RIGHT)
    {
        return KEYS_ARROWS;
    }
    return KEYS_CUSTOM;
}

void InputCircleAction::set_joy_preset(JoyPreset p_preset)
{
    switch (p_preset)
    {
    case JOY_LEFT_STICK:
        joy_axis_x = JOY_AXIS_LEFT_X, joy_axis_y = JOY_AXIS_LEFT_Y;
        break;
    case JOY_RIGHT_STICK:
        joy_axis_x = JOY_AXIS_RIGHT_X, joy_axis_y = JOY_AXIS_RIGHT_Y;
        break;
    default:
        return;
    }
    notify_property_list_changed();
}

InputCircleAction::JoyPreset InputCircleAction::get_joy_preset() const
{
    if (joy_axis_x == JOY_AXIS_LEFT_X && joy_axis_y == JOY_AXIS_LEFT_Y)
    {
        return JOY_LEFT_STICK;
    }
    if (joy_axis_x == JOY_AXIS_RIGHT_X && joy_axis_y == JOY_AXIS_RIGHT_Y)
    {
        return JOY_RIGHT_STICK;
    }
    return JOY_CUSTOM;
}

void InputCircleAction::set_dpad_preset(DpadPreset p_preset)
{
    switch (p_preset)
    {
    case DPAD_CROSS:
        dpad_up = JOY_BUTTON_DPAD_UP, dpad_down = JOY_BUTTON_DPAD_DOWN, dpad_left = JOY_BUTTON_DPAD_LEFT, dpad_right = JOY_BUTTON_DPAD_RIGHT;
        break;
    case DPAD_FACE_BUTTONS:
        dpad_up = JOY_BUTTON_Y, dpad_down = JOY_BUTTON_A, dpad_left = JOY_BUTTON_X, dpad_right = JOY_BUTTON_B;
        break;
    default:
        return;
    }
    notify_property_list_changed();
}

InputCircleAction::DpadPreset InputCircleAction::get_dpad_preset() const
{
    if (dpad_up == JOY_BUTTON_DPAD_UP && dpad_down == JOY_BUTTON_DPAD_DOWN && dpad_left == JOY_BUTTON_DPAD_LEFT && dpad_right == JOY_BUTTON_DPAD_RIGHT)
    {
        return DPAD_CROSS;
    }
    if (dpad_up == JOY_BUTTON_Y && dpad_down == JOY_BUTTON_A && dpad_left == JOY_BUTTON_X && dpad_right == JOY_BUTTON_B)
    {
        return DPAD_FACE_BUTTONS;
    }
    return DPAD_CUSTOM;
}

void InputCircleAction::_update(double p_delta)
{
    Vector2 joy;
    if (joy_axis_x >= 0 || joy_axis_y >= 0)
    {
        joy = rescale_radial(Vector2(joy_axis_value(joy_axis_x), joy_axis_value(joy_axis_y)), get_dead_zone());
    }
    Vector2 result = joy;
    if (joy == Vector2())
    {
        result.x += (key_down(right_key) || joy_button_down(dpad_right)) ? 1.0f : 0.0f;
        result.x -= (key_down(left_key) || joy_button_down(dpad_left)) ? 1.0f : 0.0f;
        result.y += (key_down(down_key) || joy_button_down(dpad_down)) ? 1.0f : 0.0f;
        result.y -= (key_down(up_key) || joy_button_down(dpad_up)) ? 1.0f : 0.0f;
        if (result.length_squared() > 1.0f)
        {
            result = result.normalized();
        }
    }
    if (invert_y)
    {
        result.y = -result.y;
    }
    _process(result, p_delta);
}

void InputCircleAction::_apply_simulation(const Variant &p_value, double p_delta)
{
    Vector2 simulated_vector = p_value;
    _process(simulated_vector.limit_length(1.0f), p_delta);
}

void InputCircleAction::_release()
{
    if (value != Vector2() || velocity != Vector2())
    {
        _process(Vector2(), 0.0);
    }
}

void InputCircleAction::_process(const Vector2 &p_value, double p_delta)
{
    velocity = p_delta > 0.0 ? (p_value - value) / p_delta : Vector2();
    value = p_value;
    double threshold = get_dirty_threshold();
    if (should_emit(value, last_emitted, threshold))
    {
        last_emitted = value;
        emit_event("value_changed", Array::make(value));
    }
    if (should_emit(velocity, last_emitted_velocity, threshold))
    {
        last_emitted_velocity = velocity;
        emit_event("velocity_changed", Array::make(velocity));
    }
    if (value != Vector2())
    {
        orientation = value.angle();
        if (Math::abs(Math::wrapf(orientation - last_emitted_orientation, -Math_PI, Math_PI)) >= MAX(threshold, 1e-4))
        {
            last_emitted_orientation = orientation;
            emit_event("orientation_changed", Array::make(orientation));
        }
    }
}

PackedStringArray InputCircleAction::get_device_properties() const
{
    PackedStringArray properties;
    for (const char *name : {"up_key", "down_key", "left_key", "right_key", "joy_axis_x", "joy_axis_y", "dpad_up", "dpad_down", "dpad_left", "dpad_right", "invert_y"})
    {
        properties.push_back(name);
    }
    return properties;
}

TypedArray<Dictionary> InputCircleAction::get_rebind_slots() const
{
    TypedArray<Dictionary> slots;
    slots.push_back(make_slot("up_key", U"Haut", "kbm"));
    slots.push_back(make_slot("down_key", U"Bas", "kbm"));
    slots.push_back(make_slot("left_key", U"Gauche", "kbm"));
    slots.push_back(make_slot("right_key", U"Droite", "kbm"));
    slots.push_back(make_slot("joy_axis_x", U"Axe X", "gamepad"));
    slots.push_back(make_slot("joy_axis_y", U"Axe Y", "gamepad"));
    slots.push_back(make_slot("dpad_up", U"Bouton haut", "gamepad"));
    slots.push_back(make_slot("dpad_down", U"Bouton bas", "gamepad"));
    slots.push_back(make_slot("dpad_left", U"Bouton gauche", "gamepad"));
    slots.push_back(make_slot("dpad_right", U"Bouton droite", "gamepad"));
    return slots;
}

bool InputCircleAction::apply_rebind(const StringName &p_slot, const Ref<InputEvent> &p_event, double p_joy_threshold)
{
    bool assigned = assign_slot(this, p_slot, p_event, p_joy_threshold);
    if (assigned)
    {
        notify_property_list_changed(); // préréglages recalculés
    }
    return assigned;
}

void InputCircleAction::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_key_preset", "preset"), &InputCircleAction::set_key_preset);
    ClassDB::bind_method(D_METHOD("get_key_preset"), &InputCircleAction::get_key_preset);
    ClassDB::bind_method(D_METHOD("set_joy_preset", "preset"), &InputCircleAction::set_joy_preset);
    ClassDB::bind_method(D_METHOD("get_joy_preset"), &InputCircleAction::get_joy_preset);
    ClassDB::bind_method(D_METHOD("set_dpad_preset", "preset"), &InputCircleAction::set_dpad_preset);
    ClassDB::bind_method(D_METHOD("get_dpad_preset"), &InputCircleAction::get_dpad_preset);
    ClassDB::bind_method(D_METHOD("set_up_key", "key"), &InputCircleAction::set_up_key);
    ClassDB::bind_method(D_METHOD("get_up_key"), &InputCircleAction::get_up_key);
    ClassDB::bind_method(D_METHOD("set_down_key", "key"), &InputCircleAction::set_down_key);
    ClassDB::bind_method(D_METHOD("get_down_key"), &InputCircleAction::get_down_key);
    ClassDB::bind_method(D_METHOD("set_left_key", "key"), &InputCircleAction::set_left_key);
    ClassDB::bind_method(D_METHOD("get_left_key"), &InputCircleAction::get_left_key);
    ClassDB::bind_method(D_METHOD("set_right_key", "key"), &InputCircleAction::set_right_key);
    ClassDB::bind_method(D_METHOD("get_right_key"), &InputCircleAction::get_right_key);
    ClassDB::bind_method(D_METHOD("set_joy_axis_x", "axis"), &InputCircleAction::set_joy_axis_x);
    ClassDB::bind_method(D_METHOD("get_joy_axis_x"), &InputCircleAction::get_joy_axis_x);
    ClassDB::bind_method(D_METHOD("set_joy_axis_y", "axis"), &InputCircleAction::set_joy_axis_y);
    ClassDB::bind_method(D_METHOD("get_joy_axis_y"), &InputCircleAction::get_joy_axis_y);
    ClassDB::bind_method(D_METHOD("set_dpad_up", "button"), &InputCircleAction::set_dpad_up);
    ClassDB::bind_method(D_METHOD("get_dpad_up"), &InputCircleAction::get_dpad_up);
    ClassDB::bind_method(D_METHOD("set_dpad_down", "button"), &InputCircleAction::set_dpad_down);
    ClassDB::bind_method(D_METHOD("get_dpad_down"), &InputCircleAction::get_dpad_down);
    ClassDB::bind_method(D_METHOD("set_dpad_left", "button"), &InputCircleAction::set_dpad_left);
    ClassDB::bind_method(D_METHOD("get_dpad_left"), &InputCircleAction::get_dpad_left);
    ClassDB::bind_method(D_METHOD("set_dpad_right", "button"), &InputCircleAction::set_dpad_right);
    ClassDB::bind_method(D_METHOD("get_dpad_right"), &InputCircleAction::get_dpad_right);
    ClassDB::bind_method(D_METHOD("set_invert_y", "invert"), &InputCircleAction::set_invert_y);
    ClassDB::bind_method(D_METHOD("get_invert_y"), &InputCircleAction::get_invert_y);
    ClassDB::bind_method(D_METHOD("get_vector"), &InputCircleAction::get_vector);
    ClassDB::bind_method(D_METHOD("get_velocity"), &InputCircleAction::get_velocity);
    ClassDB::bind_method(D_METHOD("get_orientation"), &InputCircleAction::get_orientation);

    BIND_ENUM_CONSTANT(KEYS_CUSTOM);
    BIND_ENUM_CONSTANT(KEYS_WASD);
    BIND_ENUM_CONSTANT(KEYS_ZQSD);
    BIND_ENUM_CONSTANT(KEYS_ARROWS);
    BIND_ENUM_CONSTANT(JOY_CUSTOM);
    BIND_ENUM_CONSTANT(JOY_LEFT_STICK);
    BIND_ENUM_CONSTANT(JOY_RIGHT_STICK);
    BIND_ENUM_CONSTANT(DPAD_CUSTOM);
    BIND_ENUM_CONSTANT(DPAD_CROSS);
    BIND_ENUM_CONSTANT(DPAD_FACE_BUTTONS);

    ADD_SIGNAL(MethodInfo("value_changed", PropertyInfo(Variant::VECTOR2, "value")));
    ADD_SIGNAL(MethodInfo("velocity_changed", PropertyInfo(Variant::VECTOR2, "velocity")));
    ADD_SIGNAL(MethodInfo("orientation_changed", PropertyInfo(Variant::FLOAT, "angle")));

    // Préréglages : calculés d'après les entrées, donc non enregistrés (seules les entrées le sont).
    const int preset_usage = PROPERTY_USAGE_EDITOR;
    ADD_GROUP(U"⌨️ Clavier", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "key_preset", PROPERTY_HINT_ENUM, U"Personnalisé,WASD,ZQSD,Flèches", preset_usage), "set_key_preset", "get_key_preset");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "up_key", PROPERTY_HINT_ENUM, key_hint()), "set_up_key", "get_up_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "down_key", PROPERTY_HINT_ENUM, key_hint()), "set_down_key", "get_down_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "left_key", PROPERTY_HINT_ENUM, key_hint()), "set_left_key", "get_left_key");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "right_key", PROPERTY_HINT_ENUM, key_hint()), "set_right_key", "get_right_key");
    ADD_GROUP(U"🎮 Stick", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_preset", PROPERTY_HINT_ENUM, U"Personnalisé,Stick gauche,Stick droit", preset_usage), "set_joy_preset", "get_joy_preset");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_axis_x", PROPERTY_HINT_ENUM, joy_axis_hint()), "set_joy_axis_x", "get_joy_axis_x");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "joy_axis_y", PROPERTY_HINT_ENUM, joy_axis_hint()), "set_joy_axis_y", "get_joy_axis_y");
    ADD_GROUP(U"✖️ Boutons directionnels", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_preset", PROPERTY_HINT_ENUM, U"Personnalisé,Croix directionnelle,Boutons ABXY", preset_usage), "set_dpad_preset", "get_dpad_preset");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_up", PROPERTY_HINT_ENUM, joy_button_hint()), "set_dpad_up", "get_dpad_up");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_down", PROPERTY_HINT_ENUM, joy_button_hint()), "set_dpad_down", "get_dpad_down");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_left", PROPERTY_HINT_ENUM, joy_button_hint()), "set_dpad_left", "get_dpad_left");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dpad_right", PROPERTY_HINT_ENUM, joy_button_hint()), "set_dpad_right", "get_dpad_right");
    ADD_GROUP(U"⚙️ Réglages", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "invert_y"), "set_invert_y", "get_invert_y");
}

// --- InputSphereAction ---

InputSphereAction::InputSphereAction()
{
    forward_key = KEY_W;
    backward_key = KEY_S;
    left_key = KEY_A;
    right_key = KEY_D;
    up_key = KEY_E;
    down_key = KEY_Q;
    joy_axis_x = JOY_AXIS_LEFT_X;
    joy_axis_z = JOY_AXIS_LEFT_Y; // stick vers le bas = arrière (+Z)
}

void InputSphereAction::_update(double p_delta)
{
    Vector3 joy(joy_axis_value(joy_axis_x), joy_axis_value(joy_axis_y), joy_axis_value(joy_axis_z));
    joy = rescale_radial(joy, get_dead_zone());
    Vector3 result = joy;
    if (joy == Vector3())
    {
        result.x += (key_down(right_key) || joy_button_down(dpad_right)) ? 1.0f : 0.0f;
        result.x -= (key_down(left_key) || joy_button_down(dpad_left)) ? 1.0f : 0.0f;
        result.y += (key_down(up_key) || joy_button_down(dpad_up)) ? 1.0f : 0.0f;
        result.y -= (key_down(down_key) || joy_button_down(dpad_down)) ? 1.0f : 0.0f;
        result.z -= (key_down(forward_key) || joy_button_down(dpad_forward)) ? 1.0f : 0.0f;
        result.z += (key_down(backward_key) || joy_button_down(dpad_backward)) ? 1.0f : 0.0f;
        if (result.length_squared() > 1.0f)
        {
            result = result.normalized();
        }
    }
    if (invert_y)
    {
        result.y = -result.y;
    }
    _process(result, p_delta);
}

void InputSphereAction::_apply_simulation(const Variant &p_value, double p_delta)
{
    Vector3 simulated_vector = p_value;
    _process(simulated_vector.limit_length(1.0f), p_delta);
}

void InputSphereAction::_release()
{
    if (value != Vector3() || velocity != Vector3())
    {
        _process(Vector3(), 0.0);
    }
}

void InputSphereAction::_process(const Vector3 &p_value, double p_delta)
{
    velocity = p_delta > 0.0 ? (p_value - value) / p_delta : Vector3();
    value = p_value;
    double threshold = get_dirty_threshold();
    if (should_emit(value, last_emitted, threshold))
    {
        last_emitted = value;
        emit_event("value_changed", Array::make(value));
    }
    if (should_emit(velocity, last_emitted_velocity, threshold))
    {
        last_emitted_velocity = velocity;
        emit_event("velocity_changed", Array::make(velocity));
    }
    if (value != Vector3())
    {
        // Lacet : angle qui oriente -Z vers la direction ; tangage : élévation.
        float yaw = Math::atan2(-value.x, -value.z);
        float pitch = Math::atan2(value.y, Vector2(value.x, value.z).length());
        orientation = Vector3(pitch, yaw, 0.0f);
        if (orientation.distance_to(last_emitted_orientation) >= MAX(threshold, 1e-4))
        {
            last_emitted_orientation = orientation;
            emit_event("orientation_changed", Array::make(orientation));
        }
    }
}

PackedStringArray InputSphereAction::get_device_properties() const
{
    PackedStringArray properties;
    for (const char *name : {"left_key", "right_key", "up_key", "down_key", "forward_key", "backward_key", "joy_axis_x", "joy_axis_y", "joy_axis_z", "dpad_left", "dpad_right", "dpad_up", "dpad_down", "dpad_forward", "dpad_backward", "invert_y"})
    {
        properties.push_back(name);
    }
    return properties;
}

TypedArray<Dictionary> InputSphereAction::get_rebind_slots() const
{
    TypedArray<Dictionary> slots;
    slots.push_back(make_slot("forward_key", U"Avant", "kbm"));
    slots.push_back(make_slot("backward_key", U"Arrière", "kbm"));
    slots.push_back(make_slot("left_key", U"Gauche", "kbm"));
    slots.push_back(make_slot("right_key", U"Droite", "kbm"));
    slots.push_back(make_slot("up_key", U"Haut", "kbm"));
    slots.push_back(make_slot("down_key", U"Bas", "kbm"));
    slots.push_back(make_slot("joy_axis_x", U"Axe X", "gamepad"));
    slots.push_back(make_slot("joy_axis_y", U"Axe Y", "gamepad"));
    slots.push_back(make_slot("joy_axis_z", U"Axe Z", "gamepad"));
    slots.push_back(make_slot("dpad_forward", U"Bouton avant", "gamepad"));
    slots.push_back(make_slot("dpad_backward", U"Bouton arrière", "gamepad"));
    slots.push_back(make_slot("dpad_left", U"Bouton gauche", "gamepad"));
    slots.push_back(make_slot("dpad_right", U"Bouton droite", "gamepad"));
    slots.push_back(make_slot("dpad_up", U"Bouton haut", "gamepad"));
    slots.push_back(make_slot("dpad_down", U"Bouton bas", "gamepad"));
    return slots;
}

bool InputSphereAction::apply_rebind(const StringName &p_slot, const Ref<InputEvent> &p_event, double p_joy_threshold) { return assign_slot(this, p_slot, p_event, p_joy_threshold); }

void InputSphereAction::_bind_methods()
{
#define BIND_SPHERE_INPUT(m_name, m_hint)                                                                                      \
    ClassDB::bind_method(D_METHOD("set_" #m_name, "value"), &InputSphereAction::set_##m_name);                               \
    ClassDB::bind_method(D_METHOD("get_" #m_name), &InputSphereAction::get_##m_name);                                         \
    ADD_PROPERTY(PropertyInfo(Variant::INT, #m_name, PROPERTY_HINT_ENUM, m_hint), "set_" #m_name, "get_" #m_name);

    ClassDB::bind_method(D_METHOD("set_invert_y", "invert"), &InputSphereAction::set_invert_y);
    ClassDB::bind_method(D_METHOD("get_invert_y"), &InputSphereAction::get_invert_y);
    ClassDB::bind_method(D_METHOD("get_vector"), &InputSphereAction::get_vector);
    ClassDB::bind_method(D_METHOD("get_velocity"), &InputSphereAction::get_velocity);
    ClassDB::bind_method(D_METHOD("get_orientation"), &InputSphereAction::get_orientation);

    ADD_SIGNAL(MethodInfo("value_changed", PropertyInfo(Variant::VECTOR3, "value")));
    ADD_SIGNAL(MethodInfo("velocity_changed", PropertyInfo(Variant::VECTOR3, "velocity")));
    ADD_SIGNAL(MethodInfo("orientation_changed", PropertyInfo(Variant::VECTOR3, "orientation")));

    ADD_GROUP(U"⌨️ Clavier", "");
    BIND_SPHERE_INPUT(forward_key, key_hint())
    BIND_SPHERE_INPUT(backward_key, key_hint())
    BIND_SPHERE_INPUT(left_key, key_hint())
    BIND_SPHERE_INPUT(right_key, key_hint())
    BIND_SPHERE_INPUT(up_key, key_hint())
    BIND_SPHERE_INPUT(down_key, key_hint())
    ADD_GROUP(U"🎮 Axes", "");
    BIND_SPHERE_INPUT(joy_axis_x, joy_axis_hint())
    BIND_SPHERE_INPUT(joy_axis_y, joy_axis_hint())
    BIND_SPHERE_INPUT(joy_axis_z, joy_axis_hint())
    ADD_GROUP(U"✖️ Boutons", "");
    BIND_SPHERE_INPUT(dpad_forward, joy_button_hint())
    BIND_SPHERE_INPUT(dpad_backward, joy_button_hint())
    BIND_SPHERE_INPUT(dpad_left, joy_button_hint())
    BIND_SPHERE_INPUT(dpad_right, joy_button_hint())
    BIND_SPHERE_INPUT(dpad_up, joy_button_hint())
    BIND_SPHERE_INPUT(dpad_down, joy_button_hint())
    ADD_GROUP(U"⚙️ Réglages", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "invert_y"), "set_invert_y", "get_invert_y");
#undef BIND_SPHERE_INPUT
}
