#include "character_2d.hpp"
#include "character_registry.hpp"
#include "health_component.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

Character2D::Character2D()
{
    // Avant le MovementDirector (-20) et les composants : l'intention bloquée (mort, étourdi) est appliquée d'abord.
    set_physics_process_priority(-30);
}

bool Character2D::is_side_view() const
{
    if (view_mode != VIEW_AUTO)
    {
        return view_mode == VIEW_SIDE;
    }
    Node *movement = core.find_active_movement(this);
    return movement && movement->is_class("GroundMovement2D") && int(movement->get("mode")) == 0;
}

Vector2 Character2D::get_facing() const { return core.facing; }

// ❤️ Vie (raccourcis vers le HealthComponent, s'il existe)

double Character2D::take_damage(double p_amount, Object *p_source)
{
    HealthComponent *health = core.find_health(this);
    return health ? health->take_damage(p_amount, p_source) : 0.0;
}

double Character2D::heal(double p_amount)
{
    HealthComponent *health = core.find_health(this);
    return health ? health->heal(p_amount) : 0.0;
}

double Character2D::get_health() const
{
    HealthComponent *health = core.find_health(this);
    return health ? health->get_health() : 0.0;
}

double Character2D::get_max_health() const
{
    HealthComponent *health = core.find_health(this);
    return health ? health->get_max_health() : 0.0;
}

// 📍 Position et orientation

Vector3 Character2D::get_world_position() const { return node_world_position(this); }

Vector2 Character2D::direction_input_to(const Vector3 &p_point) const
{
    Vector3 position = get_world_position();
    Vector2 difference(p_point.x - position.x, p_point.y - position.y);
    if (is_side_view())
    {
        return Math::abs(difference.x) > 0.5f ? Vector2(SIGN(difference.x), 0) : Vector2();
    }
    return difference.length_squared() > 0.0001f ? difference.normalized() : Vector2();
}

double Character2D::planar_distance_to(const Vector3 &p_point) const
{
    Vector3 position = get_world_position();
    if (is_side_view())
    {
        return Math::abs(p_point.x - position.x);
    }
    return Vector2(p_point.x - position.x, p_point.y - position.y).length();
}

void Character2D::face_toward(const Vector3 &p_point)
{
    Vector3 position = get_world_position();
    Vector2 difference(p_point.x - position.x, p_point.y - position.y);
    if (is_side_view())
    {
        core.facing = Vector2(difference.x >= 0.0f ? 1.0f : -1.0f, 0);
    }
    else if (difference.length_squared() > 0.0001f)
    {
        core.facing = difference.normalized();
    }
}

void Character2D::_notification(int p_what)
{
    if (p_what == NOTIFICATION_CHILD_ORDER_CHANGED)
    {
        notify_property_list_changed(); // composants ajoutés / retirés : propriétés exposées à jour
        return;
    }
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    switch (p_what)
    {
    case NOTIFICATION_ENTER_TREE:
        CharacterRegister::register_character(this);
        break;
    case NOTIFICATION_EXIT_TREE:
        CharacterRegister::unregister_character(this);
        break;
    case NOTIFICATION_READY:
        set_physics_process(true);
        break;
    case NOTIFICATION_PHYSICS_PROCESS:
    {
        // Intention réappliquée à chaque image : un blocage (mort, étourdi) la coupe aussitôt.
        core.forward_movement(this);
        Node *movement = core.find_active_movement(this);
        if (movement && core.move_input.length_squared() > 0.01f && movement->has_method("get_facing"))
        {
            core.facing = movement->call("get_facing");
        }
        else if (!movement && core.move_input.length_squared() > 0.01f)
        {
            core.facing = core.move_input.normalized();
        }
        break;
    }
    default:
        break;
    }
}

void Character2D::_bind_methods()
{
    bind_character_common<Character2D>();
    ClassDB::bind_method(D_METHOD("set_view_mode", "mode"), &Character2D::set_view_mode);
    ClassDB::bind_method(D_METHOD("get_view_mode"), &Character2D::get_view_mode);
    ClassDB::bind_method(D_METHOD("is_side_view"), &Character2D::is_side_view);
    BIND_ENUM_CONSTANT(VIEW_AUTO);
    BIND_ENUM_CONSTANT(VIEW_SIDE);
    BIND_ENUM_CONSTANT(VIEW_TOP_DOWN);
    ADD_GROUP(U"🪪 Identité", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "view_mode", PROPERTY_HINT_ENUM, "Auto,Side,Top Down"), "set_view_mode", "get_view_mode");
}
