#include "cursor_changer.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

#include <algorithm>
#include <vector>

using namespace godot;

namespace
{
// État partagé par tous les CursorChanger (le curseur de la souris est unique).
// Conteneurs standard : ils sont détruits au déchargement de la bibliothèque, après l'allocateur de Godot.
std::vector<uint64_t> active_stack; // identifiants des CursorChanger actifs, le dernier dessine le curseur

struct AppliedCursor
{
    bool set = false;
    uint64_t texture_id = 0;
    Input::CursorShape shape = Input::CURSOR_ARROW;
    Vector2 hotspot;
} applied;

CursorChanger *current_changer()
{
    // Les CursorChanger libérés sans désactivation (ne devrait pas arriver) sont oubliés au passage
    while (!active_stack.empty())
    {
        if (CursorChanger *changer = Object::cast_to<CursorChanger>(ObjectDB::get_instance(ObjectID(active_stack.back()))))
        {
            return changer;
        }
        active_stack.pop_back();
    }
    return nullptr;
}

void reset_cursor()
{
    if (applied.set)
    {
        Input::get_singleton()->set_custom_mouse_cursor(Ref<Resource>(), applied.shape);
    }
    applied = AppliedCursor();
}
} // namespace

// 🎬 Cycle de vie

void CursorChanger::_ready() { _start(); }

void CursorChanger::_start()
{
    if (activation_mode == ACTIVATION_ALWAYS)
    {
        activate();
    }
    else if (activation_mode == ACTIVATION_HOVER)
    {
        _connect_target();
    }
}

void CursorChanger::_notification(int p_what)
{
    if (p_what == NOTIFICATION_EXIT_TREE)
    {
        _disconnect_target();
        deactivate();
    }
    else if (p_what == NOTIFICATION_ENTER_TREE && is_node_ready() && activation_mode != ACTIVATION_MANUAL)
    {
        _start(); // nœud déplacé ou réinséré dans l'arbre : _ready n'est pas rappelé
    }
}

void CursorChanger::_process(double p_delta)
{
    if (!is_current())
    {
        return;
    }

    // Changement d'état du bouton : l'animation du nouveau style repart de sa première image
    const bool pressed = Input::get_singleton()->is_mouse_button_pressed(pressed_button);
    if (pressed != was_pressed)
    {
        was_pressed = pressed;
        style_time = 0.0;
    }
    style_time += p_delta;
    _apply(false);
}

// 🎯 Activation

void CursorChanger::activate()
{
    const uint64_t id = get_instance_id();
    active_stack.erase(std::remove(active_stack.begin(), active_stack.end(), id), active_stack.end());
    active_stack.push_back(id); // déjà actif : repasse au premier plan

    style_time = 0.0;
    was_pressed = Input::get_singleton()->is_mouse_button_pressed(pressed_button);
    _apply(true);
}

void CursorChanger::deactivate()
{
    const bool was_current = is_current();
    const uint64_t id = get_instance_id();
    active_stack.erase(std::remove(active_stack.begin(), active_stack.end(), id), active_stack.end());
    if (!was_current)
    {
        return;
    }

    // Le précédent reprend la main, sinon retour au curseur du système
    if (CursorChanger *next = current_changer())
    {
        next->_apply(true);
    }
    else
    {
        reset_cursor();
    }
}

bool CursorChanger::is_active() const { return std::find(active_stack.begin(), active_stack.end(), (uint64_t)get_instance_id()) != active_stack.end(); }

bool CursorChanger::is_current() const { return current_changer() == this; }

Ref<CursorStyle> CursorChanger::_get_displayed_style() const { return was_pressed && pressed_style.is_valid() ? pressed_style : style; }

void CursorChanger::_apply(bool p_force)
{
    const Ref<CursorStyle> displayed = _get_displayed_style();
    const Ref<Texture2D> texture = displayed.is_valid() ? displayed->get_frame_at(style_time) : Ref<Texture2D>();
    if (texture.is_null())
    {
        reset_cursor(); // style sans image : curseur du système
        return;
    }

    const Input::CursorShape shape = displayed->get_shape();
    const Vector2 hotspot = displayed->get_hotspot();
    if (!p_force && applied.set && applied.texture_id == texture->get_instance_id() && applied.shape == shape && applied.hotspot == hotspot)
    {
        return; // déjà affiché : set_custom_mouse_cursor est coûteux
    }

    // Autre forme remplacée : l'ancienne retrouve son curseur système
    if (applied.set && applied.shape != shape)
    {
        reset_cursor();
    }
    Input::get_singleton()->set_custom_mouse_cursor(texture, shape, hotspot);
    applied.set = true;
    applied.texture_id = texture->get_instance_id();
    applied.shape = shape;
    applied.hotspot = hotspot;
}

// 🎛️ Réglages

void CursorChanger::set_style(const Ref<CursorStyle> &p_value)
{
    style = p_value;
    if (is_current())
    {
        _apply(true);
    }
}

void CursorChanger::set_pressed_style(const Ref<CursorStyle> &p_value)
{
    pressed_style = p_value;
    if (is_current())
    {
        _apply(true);
    }
}

void CursorChanger::set_activation_mode(ActivationMode p_value)
{
    if (activation_mode == p_value)
    {
        return;
    }
    activation_mode = p_value;
    if (!is_node_ready())
    {
        return;
    }
    _disconnect_target();
    deactivate();
    _start();
}

void CursorChanger::set_target(Node *p_target)
{
    _disconnect_target();
    target_id = p_target ? ObjectID(p_target->get_instance_id()) : ObjectID();
    if (is_node_ready() && activation_mode == ACTIVATION_HOVER)
    {
        _connect_target();
    }
}

Node *CursorChanger::get_target() const { return Object::cast_to<Node>(ObjectDB::get_instance(target_id)); }

// 🖱️ Survol de la cible

void CursorChanger::_connect_target()
{
    Node *target = get_target();
    if (!target)
    {
        target = get_parent();
    }
    if (!target)
    {
        return;
    }
    if (!target->has_signal("mouse_entered") || !target->has_signal("mouse_exited"))
    {
        WARN_PRINT(String(U"[CursorChanger] La cible « ") + target->get_name() + String(U" » n'émet pas mouse_entered / mouse_exited (Control ou CollisionObject attendu)."));
        return;
    }
    const Callable on_entered = callable_mp(this, &CursorChanger::activate);
    if (!target->is_connected("mouse_entered", on_entered))
    {
        target->connect("mouse_entered", on_entered);
        target->connect("mouse_exited", callable_mp(this, &CursorChanger::deactivate));
    }
    connected_target_id = target->get_instance_id();
}

void CursorChanger::_disconnect_target()
{
    Node *target = Object::cast_to<Node>(ObjectDB::get_instance(connected_target_id));
    connected_target_id = ObjectID();
    if (!target)
    {
        return;
    }
    const Callable on_entered = callable_mp(this, &CursorChanger::activate);
    const Callable on_exited = callable_mp(this, &CursorChanger::deactivate);
    if (target->is_connected("mouse_entered", on_entered))
    {
        target->disconnect("mouse_entered", on_entered);
    }
    if (target->is_connected("mouse_exited", on_exited))
    {
        target->disconnect("mouse_exited", on_exited);
    }
}

// 🔗 Liaisons

void CursorChanger::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("activate"), &CursorChanger::activate);
    ClassDB::bind_method(D_METHOD("deactivate"), &CursorChanger::deactivate);
    ClassDB::bind_method(D_METHOD("is_active"), &CursorChanger::is_active);
    ClassDB::bind_method(D_METHOD("is_current"), &CursorChanger::is_current);
    ClassDB::bind_method(D_METHOD("set_style", "value"), &CursorChanger::set_style);
    ClassDB::bind_method(D_METHOD("get_style"), &CursorChanger::get_style);
    ClassDB::bind_method(D_METHOD("set_pressed_style", "value"), &CursorChanger::set_pressed_style);
    ClassDB::bind_method(D_METHOD("get_pressed_style"), &CursorChanger::get_pressed_style);
    ClassDB::bind_method(D_METHOD("set_pressed_button", "value"), &CursorChanger::set_pressed_button);
    ClassDB::bind_method(D_METHOD("get_pressed_button"), &CursorChanger::get_pressed_button);
    ClassDB::bind_method(D_METHOD("set_activation_mode", "value"), &CursorChanger::set_activation_mode);
    ClassDB::bind_method(D_METHOD("get_activation_mode"), &CursorChanger::get_activation_mode);
    ClassDB::bind_method(D_METHOD("set_target", "target"), &CursorChanger::set_target);
    ClassDB::bind_method(D_METHOD("get_target"), &CursorChanger::get_target);

    BIND_ENUM_CONSTANT(ACTIVATION_ALWAYS);
    BIND_ENUM_CONSTANT(ACTIVATION_HOVER);
    BIND_ENUM_CONSTANT(ACTIVATION_MANUAL);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "style", PROPERTY_HINT_RESOURCE_TYPE, "CursorStyle"), "set_style", "get_style");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "pressed_style", PROPERTY_HINT_RESOURCE_TYPE, "CursorStyle"), "set_pressed_style", "get_pressed_style");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "pressed_button", PROPERTY_HINT_ENUM, "Left:1,Right:2,Middle:3"), "set_pressed_button", "get_pressed_button");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "activation_mode", PROPERTY_HINT_ENUM, "Always,Hover,Manual"), "set_activation_mode", "get_activation_mode");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target", PROPERTY_HINT_NODE_TYPE, "Node"), "set_target", "get_target");
}
