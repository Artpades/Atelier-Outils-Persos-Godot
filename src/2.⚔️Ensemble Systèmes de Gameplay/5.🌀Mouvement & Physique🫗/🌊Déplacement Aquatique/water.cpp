#include "water.hpp"

#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

LocalVector<ObjectID> WaterArea2D::waters;
LocalVector<ObjectID> WaterArea3D::waters;

namespace
{
template <typename T>
void register_instance(LocalVector<ObjectID> &r_list, T *p_node, bool p_enter)
{
    ObjectID id(p_node->get_instance_id());
    int64_t index = r_list.find(id);
    if (p_enter && index < 0)
    {
        r_list.push_back(id);
    }
    else if (!p_enter && index >= 0)
    {
        r_list.remove_at(index);
    }
}

template <typename T>
void bind_water_common(Variant::Type p_vector_type)
{
    ClassDB::bind_method(D_METHOD("set_buoyancy", "value"), &T::set_buoyancy);
    ClassDB::bind_method(D_METHOD("get_buoyancy"), &T::get_buoyancy);
    ClassDB::bind_method(D_METHOD("set_float_depth", "value"), &T::set_float_depth);
    ClassDB::bind_method(D_METHOD("get_float_depth"), &T::get_float_depth);
    ClassDB::bind_method(D_METHOD("set_linear_drag", "value"), &T::set_linear_drag);
    ClassDB::bind_method(D_METHOD("get_linear_drag"), &T::get_linear_drag);
    ClassDB::bind_method(D_METHOD("set_angular_drag", "value"), &T::set_angular_drag);
    ClassDB::bind_method(D_METHOD("get_angular_drag"), &T::get_angular_drag);
    ClassDB::bind_method(D_METHOD("set_current", "current"), &T::set_current);
    ClassDB::bind_method(D_METHOD("get_current"), &T::get_current);
    ClassDB::bind_method(D_METHOD("set_surface_offset", "offset"), &T::set_surface_offset);
    ClassDB::bind_method(D_METHOD("get_surface_offset"), &T::get_surface_offset);
    ClassDB::bind_method(D_METHOD("get_surface_y"), &T::get_surface_y);
    ClassDB::bind_method(D_METHOD("get_depth", "point"), &T::get_depth);
    ClassDB::bind_static_method(T::get_class_static(), D_METHOD("find_containing", "body"), &T::find_containing);

    StringName cls = T::get_class_static();
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "buoyancy", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater"), "set_buoyancy", "get_buoyancy");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "float_depth", PROPERTY_HINT_RANGE, "0.001,500,0.001,or_greater"), "set_float_depth", "get_float_depth");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "linear_drag", PROPERTY_HINT_RANGE, "0,20,0.01,or_greater"), "set_linear_drag", "get_linear_drag");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "angular_drag", PROPERTY_HINT_RANGE, "0,20,0.01,or_greater"), "set_angular_drag", "get_angular_drag");
    ClassDB::add_property(cls, PropertyInfo(p_vector_type, "current"), "set_current", "get_current");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "surface_offset"), "set_surface_offset", "get_surface_offset");
}

template <typename T>
void bind_swim_common()
{
#define SWIM_BIND(m_name, m_arg)                                                       \
    ClassDB::bind_method(D_METHOD("set_" #m_name, m_arg), &T::set_##m_name);           \
    ClassDB::bind_method(D_METHOD("get_" #m_name), &T::get_##m_name);
    SWIM_BIND(style, "style")
    SWIM_BIND(swim_speed, "value")
    SWIM_BIND(acceleration, "value")
    SWIM_BIND(drag, "value")
    SWIM_BIND(buoyancy, "value")
    SWIM_BIND(float_depth, "value")
    SWIM_BIND(surface_tolerance, "value")
    SWIM_BIND(jump_out_velocity, "value")
    SWIM_BIND(stroke_velocity, "value")
    SWIM_BIND(stroke_cooldown, "value")
    SWIM_BIND(turn_speed, "value")
    SWIM_BIND(run_multiplier, "value")
    SWIM_BIND(cruise_ratio, "value")
    SWIM_BIND(align_to_heading, "align")
    SWIM_BIND(undulation_amplitude, "value")
    SWIM_BIND(undulation_frequency, "value")
    SWIM_BIND(visual_path, "path")
#undef SWIM_BIND
    ClassDB::bind_method(D_METHOD("get_speed"), &T::get_speed);
    ClassDB::bind_method(D_METHOD("get_heading"), &T::get_heading);
    ClassDB::bind_method(D_METHOD("get_water"), &T::get_water);

    StringName cls = T::get_class_static();
    ClassDB::bind_integer_constant(cls, "Style", "STYLE_SURFACE", SwimSettings::STYLE_SURFACE);
    ClassDB::bind_integer_constant(cls, "Style", "STYLE_DIRECTIONAL", SwimSettings::STYLE_DIRECTIONAL);
    ClassDB::bind_integer_constant(cls, "Style", "STYLE_FISH", SwimSettings::STYLE_FISH);
    ClassDB::add_property(cls, PropertyInfo(Variant::INT, "style", PROPERTY_HINT_ENUM, String::utf8("Flottaison,Directionnel (Mario Galaxy),Poisson")), "set_style", "get_style");
    ClassDB::add_property_group(cls, String::utf8("🏊 Nage"), "");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "swim_speed", PROPERTY_HINT_RANGE, "0,2000,0.01,or_greater"), "set_swim_speed", "get_swim_speed");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "acceleration", PROPERTY_HINT_RANGE, "0,10000,0.01,or_greater"), "set_acceleration", "get_acceleration");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "drag", PROPERTY_HINT_RANGE, "0,20,0.01,or_greater"), "set_drag", "get_drag");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "turn_speed", PROPERTY_HINT_RANGE, "0,30,0.01,or_greater,suffix:rad/s"), "set_turn_speed", "get_turn_speed");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "run_multiplier", PROPERTY_HINT_RANGE, "0,5,0.01,or_greater"), "set_run_multiplier", "get_run_multiplier");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "cruise_ratio", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_cruise_ratio", "get_cruise_ratio");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "stroke_velocity", PROPERTY_HINT_RANGE, "0,3000,0.01,or_greater"), "set_stroke_velocity", "get_stroke_velocity");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "stroke_cooldown", PROPERTY_HINT_RANGE, "0,3,0.01,or_greater,suffix:s"), "set_stroke_cooldown", "get_stroke_cooldown");
    ClassDB::add_property_group(cls, String::utf8("🌊 Surface"), "");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "buoyancy", PROPERTY_HINT_RANGE, "0,5000,0.01,or_greater"), "set_buoyancy", "get_buoyancy");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "float_depth"), "set_float_depth", "get_float_depth");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "surface_tolerance", PROPERTY_HINT_RANGE, "0,500,0.01,or_greater"), "set_surface_tolerance", "get_surface_tolerance");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "jump_out_velocity", PROPERTY_HINT_RANGE, "0,3000,0.01,or_greater"), "set_jump_out_velocity", "get_jump_out_velocity");
    ClassDB::add_property_group(cls, String::utf8("🐟 Visuel"), "");
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "align_to_heading"), "set_align_to_heading", "get_align_to_heading");
    ClassDB::add_property(cls, PropertyInfo(Variant::NODE_PATH, "visual_path"), "set_visual_path", "get_visual_path");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "undulation_amplitude", PROPERTY_HINT_RANGE, "0,1.5,0.01,suffix:rad"), "set_undulation_amplitude", "get_undulation_amplitude");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "undulation_frequency", PROPERTY_HINT_RANGE, "0,10,0.01,suffix:Hz"), "set_undulation_frequency", "get_undulation_frequency");
    ClassDB::add_signal(cls, MethodInfo("jumped_out"));
    ClassDB::add_signal(cls, MethodInfo("stroked"));
}

/// Tourne p_from vers p_to d'au plus p_max_angle (radians).
Vector2 turn_toward(const Vector2 &p_from, const Vector2 &p_to, double p_max_angle)
{
    double angle = p_from.angle_to(p_to);
    return p_from.rotated(CLAMP(angle, -p_max_angle, p_max_angle)).normalized();
}

Vector3 turn_toward(const Vector3 &p_from, const Vector3 &p_to, double p_max_angle)
{
    double angle = p_from.angle_to(p_to);
    if (angle < 1e-4)
    {
        return p_to.normalized();
    }
    Vector3 axis = p_from.cross(p_to);
    if (axis.length_squared() < 1e-8f)
    {
        axis = Math::abs(p_from.y) < 0.9f ? Vector3(0, 1, 0) : Vector3(1, 0, 0); // directions opposées
    }
    return p_from.rotated(axis.normalized(), MIN(angle, p_max_angle)).normalized();
}

/// Vitesse le long de la direction : monte vers la cible avec acceleration, retombe deux fois moins vite.
double approach_speed(double p_speed, double p_target, double p_acceleration, double p_delta)
{
    return Math::move_toward(p_speed, p_target, p_acceleration * p_delta * (p_target >= p_speed ? 1.0 : 0.5));
}
} // namespace

// =====================================================================================================
// 🌊 WaterArea2D
// =====================================================================================================

double WaterArea2D::get_surface_y() const
{
    for (int i = 0; i < get_child_count(); i++)
    {
        CollisionShape2D *shape = Object::cast_to<CollisionShape2D>(get_child(i));
        Ref<RectangleShape2D> rectangle = shape ? Ref<RectangleShape2D>(shape->get_shape()) : Ref<RectangleShape2D>();
        if (rectangle.is_valid())
        {
            Transform2D transform = shape->is_inside_tree() ? shape->get_global_transform() : get_transform() * shape->get_transform();
            return transform.xform(Vector2(0, -rectangle->get_size().y * 0.5f)).y;
        }
    }
    return (is_inside_tree() ? get_global_position().y : get_position().y) + surface_offset;
}

WaterArea2D *WaterArea2D::find_containing(Node *p_body)
{
    for (uint32_t i = 0; p_body && i < waters.size(); i++)
    {
        WaterArea2D *water = Object::cast_to<WaterArea2D>(ObjectDB::get_instance(waters[i]));
        if (water && water->overlaps_body(p_body))
        {
            return water;
        }
    }
    return nullptr;
}

void WaterArea2D::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_EXIT_TREE)
    {
        register_instance(waters, this, p_what == NOTIFICATION_ENTER_TREE);
        set_physics_process(true);
        if (p_what == NOTIFICATION_EXIT_TREE)
        {
            debug.release();
        }
        return;
    }
    if (p_what != NOTIFICATION_PHYSICS_PROCESS)
    {
        return;
    }
    // Objets flottants : poussée proportionnelle à l'immersion, freinage, courant.
    TypedArray<Node2D> bodies = get_overlapping_bodies();
    for (int i = 0; i < bodies.size(); i++)
    {
        RigidBody2D *rigid = Object::cast_to<RigidBody2D>(bodies[i]);
        if (!rigid)
        {
            continue;
        }
        double submerged = CLAMP(get_depth(rigid->get_global_position()) / float_depth + 0.5, 0.0, 1.0);
        double mass = rigid->get_mass();
        Vector2 gravity = rigid->get_gravity();
        rigid->apply_central_force(-gravity * buoyancy * submerged * mass);
        rigid->apply_central_force(-rigid->get_linear_velocity() * linear_drag * submerged * mass);
        rigid->apply_central_force(current * submerged * mass);
        rigid->apply_torque(-rigid->get_angular_velocity() * angular_drag * submerged * rigid->get_inertia());
    }
    if (debug_mode)
    {
        debug.begin(this, false);
        debug.shape_outline(this, debug_colors::ZONE);
        Vector2 center = get_global_position();
        float surface = get_surface_y();
        debug.line(Vector2(center.x - 4000, surface), Vector2(center.x + 4000, surface), Color(0.4f, 0.7f, 1.0f, 0.5f), 1.0f);
        debug.arrow(center, center + current * 0.3f, debug_colors::FORCE);
        debug.text(Vector2(center.x - 40, surface - 8), String(U"🌊 ") + String(get_name()) + U" · " + String::num_int64(bodies.size()) + U" corps", debug_colors::TEXT, 11);
        debug.end();
    }
}

void WaterArea2D::_bind_methods()
{
    bind_water_common<WaterArea2D>(Variant::VECTOR2);
    bind_debug_mode<WaterArea2D>();
}

// =====================================================================================================
// 🌊 WaterArea3D
// =====================================================================================================

WaterArea3D::WaterArea3D() {}

double WaterArea3D::get_surface_y() const
{
    for (int i = 0; i < get_child_count(); i++)
    {
        CollisionShape3D *shape = Object::cast_to<CollisionShape3D>(get_child(i));
        Ref<BoxShape3D> box = shape ? Ref<BoxShape3D>(shape->get_shape()) : Ref<BoxShape3D>();
        if (box.is_valid())
        {
            Transform3D transform = shape->is_inside_tree() ? shape->get_global_transform() : get_transform() * shape->get_transform();
            return transform.xform(Vector3(0, box->get_size().y * 0.5f, 0)).y;
        }
    }
    return (is_inside_tree() ? get_global_position().y : get_position().y) + surface_offset;
}

WaterArea3D *WaterArea3D::find_containing(Node *p_body)
{
    for (uint32_t i = 0; p_body && i < waters.size(); i++)
    {
        WaterArea3D *water = Object::cast_to<WaterArea3D>(ObjectDB::get_instance(waters[i]));
        if (water && water->overlaps_body(p_body))
        {
            return water;
        }
    }
    return nullptr;
}

void WaterArea3D::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_EXIT_TREE)
    {
        register_instance(waters, this, p_what == NOTIFICATION_ENTER_TREE);
        set_physics_process(true);
        if (p_what == NOTIFICATION_EXIT_TREE)
        {
            debug.release();
        }
        return;
    }
    if (p_what != NOTIFICATION_PHYSICS_PROCESS)
    {
        return;
    }
    TypedArray<Node3D> bodies = get_overlapping_bodies();
    for (int i = 0; i < bodies.size(); i++)
    {
        RigidBody3D *rigid = Object::cast_to<RigidBody3D>(bodies[i]);
        if (!rigid)
        {
            continue;
        }
        double submerged = CLAMP(get_depth(rigid->get_global_position()) / float_depth + 0.5, 0.0, 1.0);
        double mass = rigid->get_mass();
        rigid->apply_central_force(-rigid->get_gravity() * buoyancy * submerged * mass);
        rigid->apply_central_force(-rigid->get_linear_velocity() * linear_drag * submerged * mass);
        rigid->apply_central_force(current * submerged * mass);
        rigid->apply_torque(-rigid->get_angular_velocity() * angular_drag * submerged * mass);
    }
    if (debug_mode)
    {
        debug.begin(this, true);
        debug.shape_outline3(this, debug_colors::ZONE);
        Vector3 center = get_global_position();
        Vector3 surface(center.x, get_surface_y(), center.z);
        debug.circle3(surface, 0.5f, Vector3(0, 1, 0), Color(0.4f, 0.7f, 1.0f));
        debug.arrow3(surface, surface + current * 0.3f, debug_colors::FORCE);
        debug.text3(surface + Vector3(0, 0.4f, 0), String(U"🌊 ") + String(get_name()) + U" · " + String::num_int64(bodies.size()) + U" corps");
        debug.end();
    }
}

void WaterArea3D::_bind_methods()
{
    bind_water_common<WaterArea3D>(Variant::VECTOR3);
    bind_debug_mode<WaterArea3D>();
}

// =====================================================================================================
// 🏊 SwimMovement2D
// =====================================================================================================

void SwimMovement2D::_step_surface(CharacterBody2D *p_character, WaterArea2D *p_water, double p_delta)
{
    Vector2 velocity = p_character->get_velocity();
    double depth = p_water->get_depth(p_character->get_global_position());
    Vector2 target = move_input * swim.swim_speed;
    velocity.x = Math::move_toward(velocity.x, target.x, float(swim.acceleration * p_delta));
    if (Math::abs(move_input.y) > 0.1f)
    {
        velocity.y = Math::move_toward(velocity.y, target.y, float(swim.acceleration * p_delta));
    }
    else
    {
        // Sans commande verticale : retour vers la profondeur d'équilibre (flottaison).
        double offset = CLAMP((depth - swim.float_depth) / MAX(swim.float_depth, 1.0), -1.0, 1.0);
        velocity.y -= offset * swim.buoyancy * p_delta;
    }
    // Freinage de l'eau sur les axes non commandés (la nage commandée atteint swim_speed).
    double resistance = MAX(0.0, 1.0 - swim.drag * p_delta);
    if (Math::abs(move_input.x) < 0.1f)
    {
        velocity.x *= resistance;
    }
    if (Math::abs(move_input.y) < 0.1f)
    {
        velocity.y *= resistance;
    }
    velocity += p_water->get_current() * p_delta + external_acceleration_2d(p_character) * p_delta;
    if (has_jump_request(true))
    {
        if (depth <= swim.surface_tolerance)
        {
            velocity.y = -swim.jump_out_velocity;
            emit_signal("jumped_out");
        }
        else
        {
            velocity.y = MIN(velocity.y, float(-swim.stroke_velocity));
            emit_signal("stroked");
        }
    }
    p_character->set_velocity(velocity);
    p_character->move_and_slide();
    if (Math::abs(velocity.x) > 1.0f)
    {
        heading = Vector2(SIGN(velocity.x), 0);
    }
}

void SwimMovement2D::_physics_step(double p_delta)
{
    CharacterBody2D *character = Object::cast_to<CharacterBody2D>(get_body());
    if (!character)
    {
        return;
    }
    WaterArea2D *water = get_water();
    if (!water)
    {
        swim.initialized = false;
        swim.leaping = false;
        Vector2 velocity = character->get_velocity() + character->get_gravity() * p_delta; // sorti de l'eau sans changer de contexte
        character->set_velocity(velocity);
        character->move_and_slide();
        _orient(p_delta);
        return;
    }
    swim.time += p_delta;
    if (!swim.initialized)
    {
        // Entrée dans l'eau : direction et élan tirés de la vitesse d'arrivée.
        Vector2 arrival = character->get_velocity();
        heading = arrival.length_squared() > 1.0f ? arrival.normalized() : heading;
        swim.speed = MIN(double(arrival.length()), swim.swim_speed);
        drift = Vector2();
        swim.initialized = true;
    }
    if (swim.style == SwimSettings::STYLE_SURFACE)
    {
        _step_surface(character, water, p_delta);
        _orient(p_delta);
        return;
    }
    double depth = water->get_depth(character->get_global_position());
    last_depth = depth;
    if (swim.leaping)
    {
        // Saut hors de l'eau : trajectoire libre tant que le corps monte encore dans l'eau.
        Vector2 velocity = character->get_velocity() + character->get_gravity() * p_delta;
        if (velocity.y >= 0.0f)
        {
            swim.leaping = false;
            swim.initialized = false; // retombée : nouvel élan d'entrée
        }
        character->set_velocity(velocity);
        character->move_and_slide();
        _orient(p_delta);
        return;
    }
    bool at_surface = depth <= swim.float_depth + 2.0;
    double max_speed = swim.swim_speed * (running ? swim.run_multiplier : 1.0);
    desired = move_input;
    if (at_surface && desired.y < 0.0f)
    {
        desired.y = 0.0f; // à la surface, pousser vers le haut ne fait pas nager : on y reste
    }
    double target_speed = desired.length() * max_speed;
    if (desired.length_squared() > 0.01f)
    {
        heading = turn_toward(heading, desired, swim.turn_speed * p_delta);
    }
    if (swim.style == SwimSettings::STYLE_FISH)
    {
        target_speed = MAX(target_speed, swim.cruise_ratio * max_speed); // un poisson ne s'arrête pas
    }
    // À la surface, la nage reste horizontale.
    if (at_surface && heading.y < 0.0f)
    {
        heading = turn_toward(heading, Vector2(heading.x >= 0.0f ? 1.0f : -1.0f, 0.0f), swim.turn_speed * 2.0 * p_delta);
    }
    swim.speed = approach_speed(swim.speed, target_speed, swim.acceleration, p_delta);
    swim.stroke_timer = MAX(0.0, swim.stroke_timer - p_delta);

    drift += (water->get_current() + external_acceleration_2d(character)) * p_delta;
    drift *= MAX(0.0, 1.0 - swim.drag * p_delta);
    Vector2 velocity = heading * swim.speed + drift;
    if (has_jump_request(true))
    {
        if (depth <= swim.surface_tolerance)
        {
            velocity.y = -swim.jump_out_velocity; // saut hors de l'eau, en gardant l'élan horizontal
            swim.leaping = true;
            emit_signal("jumped_out");
            character->set_velocity(velocity);
            character->move_and_slide();
            _orient(p_delta);
            return;
        }
        if (swim.stroke_timer <= 0.0)
        {
            swim.speed = MIN(swim.speed + swim.stroke_velocity, max_speed * 2.0); // coup de nage
            swim.stroke_timer = swim.stroke_cooldown;
            velocity = heading * swim.speed + drift;
            emit_signal("stroked");
        }
    }
    if (depth < 0.0)
    {
        velocity.y = character->get_velocity().y + character->get_gravity().y * p_delta; // au-dessus de la surface : retombe
    }
    else if (depth < swim.float_depth && velocity.y < 0.0f)
    {
        velocity.y = float((swim.float_depth - depth) * 4.0); // ne pas émerger : retour sous la surface
    }
    character->set_velocity(velocity);
    character->move_and_slide();
    // Un obstacle coupe l'élan.
    swim.speed = MIN(swim.speed, MAX(0.0, double((character->get_velocity() - drift).dot(heading))));
    _orient(p_delta);
}

void SwimMovement2D::_orient(double /*p_delta*/)
{
    if (!swim.align_to_heading)
    {
        return;
    }
    Node2D *visual = Object::cast_to<Node2D>(swim.visual_path.is_empty() ? get_body() : get_node_or_null(swim.visual_path));
    if (!visual)
    {
        return;
    }
    // Inclinaison sans retournement (le retournement gauche / droite reste celui du visuel).
    float tilt = heading.x >= 0.0f ? Math::atan2(heading.y, heading.x) : -Math::atan2(heading.y, -heading.x);
    if (swim.style == SwimSettings::STYLE_FISH)
    {
        double intensity = swim.swim_speed > 0.0 ? CLAMP(swim.speed / swim.swim_speed, 0.3, 1.5) : 0.0;
        tilt += float(Math::sin(swim.time * swim.undulation_frequency * Math_TAU) * swim.undulation_amplitude * 0.5 * intensity);
    }
    if (get_water() == nullptr)
    {
        tilt = 0.0f;
    }
    visual->set_rotation(tilt);
}

void SwimMovement2D::_on_deactivated()
{
    swim.initialized = false;
    swim.leaping = false;
    if (swim.align_to_heading)
    {
        if (Node2D *visual = Object::cast_to<Node2D>(swim.visual_path.is_empty() ? get_body() : get_node_or_null(swim.visual_path)))
        {
            visual->set_rotation(0.0f);
        }
    }
}

String SwimMovement2D::_debug_text() const
{
    static const char32_t *styles[] = {U"flottaison", U"directionnel", U"poisson"};
    return String(styles[swim.style]) + U" · prof. " + String::num(last_depth, 0) + U" · " + String::num(Math::abs(swim.speed), 0) + " px/s";
}

void SwimMovement2D::_draw_debug(DebugGizmos &p_debug)
{
    if (Node2D *body = Object::cast_to<Node2D>(get_body()))
    {
        Vector2 position = body->get_global_position();
        p_debug.arrow(position, position + heading * 45.0f, debug_colors::HEADING, 8.0f, 3.0f);
        if (WaterArea2D *water = get_water())
        {
            float surface = water->get_surface_y();
            p_debug.line(Vector2(position.x - 40, surface + swim.float_depth), Vector2(position.x + 40, surface + swim.float_depth), Color(0.4f, 0.7f, 1.0f, 0.7f), 1.0f);
        }
    }
    MovementComponent::_draw_debug(p_debug);
}

void SwimMovement2D::_bind_methods() { bind_swim_common<SwimMovement2D>(); }

// =====================================================================================================
// 🏊 SwimMovement3D
// =====================================================================================================

SwimMovement3D::SwimMovement3D()
{
    swim.swim_speed = 5.0;
    swim.acceleration = 12.0;
    swim.drag = 2.0;
    swim.buoyancy = 6.0;
    swim.float_depth = 0.6;
    swim.surface_tolerance = 0.8;
    swim.jump_out_velocity = 5.0;
    swim.stroke_velocity = 4.0;
    swim.turn_speed = 3.5;
}

void SwimMovement3D::_step_surface(CharacterBody3D *p_character, WaterArea3D *p_water, double p_delta)
{
    Vector3 velocity = p_character->get_velocity();
    Vector2 input = move_input;
    if (camera_relative)
    {
        Camera3D *camera = get_viewport() ? get_viewport()->get_camera_3d() : nullptr;
        if (camera)
        {
            Basis basis = camera->get_global_transform().basis;
            Vector3 forward = -basis.get_column(2);
            Vector3 right = basis.get_column(0);
            forward.y = 0.0f;
            right.y = 0.0f;
            Vector3 direction = right.normalized() * input.x + forward.normalized() * -input.y;
            input = Vector2(direction.x, direction.z).limit_length(1.0f);
        }
    }
    double depth = p_water->get_depth(p_character->get_global_position());
    float vertical = Math::abs(vertical_input) > 0.1f ? vertical_input : (jump_held ? 1.0f : 0.0f);
    Vector2 horizontal(velocity.x, velocity.z);
    horizontal = horizontal.move_toward(input * swim.swim_speed, swim.acceleration * p_delta);
    velocity.x = horizontal.x;
    velocity.z = horizontal.y;
    if (vertical != 0.0f && !(vertical > 0.0f && depth <= swim.float_depth))
    {
        velocity.y = Math::move_toward(velocity.y, float(vertical * swim.swim_speed), float(swim.acceleration * p_delta));
    }
    else
    {
        double offset = CLAMP((depth - swim.float_depth) / MAX(swim.float_depth, 0.01), -1.0, 1.0);
        velocity.y += offset * swim.buoyancy * p_delta;
    }
    double resistance = MAX(0.0, 1.0 - swim.drag * p_delta);
    if (input.length_squared() < 0.01f)
    {
        velocity.x *= resistance;
        velocity.z *= resistance;
    }
    if (vertical == 0.0f)
    {
        velocity.y *= resistance;
    }
    velocity += p_water->get_current() * p_delta + external_acceleration_3d(p_character) * p_delta;
    if (has_jump_request(true) && depth <= swim.surface_tolerance)
    {
        velocity.y = swim.jump_out_velocity;
        emit_signal("jumped_out");
    }
    p_character->set_velocity(velocity);
    p_character->move_and_slide();
    Vector3 flat(velocity.x, 0.0f, velocity.z);
    if (flat.length_squared() > 0.01f)
    {
        heading = flat.normalized();
    }
}

void SwimMovement3D::_physics_step(double p_delta)
{
    CharacterBody3D *character = Object::cast_to<CharacterBody3D>(get_body());
    if (!character)
    {
        return;
    }
    WaterArea3D *water = get_water();
    if (!water)
    {
        swim.initialized = false;
        swim.leaping = false;
        Vector3 velocity = character->get_velocity() + character->get_gravity() * p_delta;
        character->set_velocity(velocity);
        character->move_and_slide();
        _orient(p_delta);
        return;
    }
    swim.time += p_delta;
    if (!swim.initialized)
    {
        Vector3 arrival = character->get_velocity();
        Vector3 facing = -character->get_global_basis().get_column(2);
        heading = arrival.length_squared() > 0.25f ? arrival.normalized() : (facing.length_squared() > 0.01f ? facing.normalized() : heading);
        swim.speed = MIN(double(arrival.length()), swim.swim_speed);
        drift = Vector3();
        swim.initialized = true;
    }
    if (swim.style == SwimSettings::STYLE_SURFACE)
    {
        _step_surface(character, water, p_delta);
        _orient(p_delta);
        return;
    }
    double depth = water->get_depth(character->get_global_position());
    last_depth = depth;
    if (swim.leaping)
    {
        Vector3 velocity = character->get_velocity() + character->get_gravity() * p_delta;
        if (velocity.y <= 0.0f)
        {
            swim.leaping = false;
            swim.initialized = false;
        }
        character->set_velocity(velocity);
        character->move_and_slide();
        _orient(p_delta);
        return;
    }
    bool at_surface = depth <= swim.float_depth + 0.1;
    double max_speed = swim.swim_speed * (running ? swim.run_multiplier : 1.0);
    double target_speed = 0.0;
    double step = swim.turn_speed * p_delta;
    if (swim.style == SwimSettings::STYLE_FISH)
    {
        // Commandes de poisson : lacet (move.x), tangage (vertical), avant / arrière = vitesse.
        Vector3 flat(heading.x, 0.0f, heading.z);
        if (flat.length_squared() < 1e-4f)
        {
            flat = -character->get_global_basis().get_column(2);
            flat.y = 0.0f;
        }
        heading = heading.rotated(Vector3(0, 1, 0), -move_input.x * step).normalized();
        Vector3 right = heading.cross(Vector3(0, 1, 0));
        if (right.length_squared() > 1e-4f && Math::abs(vertical_input) > 0.05f)
        {
            Vector3 pitched = heading.rotated(right.normalized(), vertical_input * step).normalized();
            if (Math::abs(pitched.y) < 0.95f)
            {
                heading = pitched;
            }
        }
        double cruise = swim.cruise_ratio * max_speed;
        target_speed = move_input.y <= 0.0f ? cruise + double(-move_input.y) * (max_speed - cruise) : cruise * (1.0 - double(move_input.y));
        desired = heading * float(target_speed / MAX(max_speed, 0.001));
    }
    else
    {
        // Directionnel : la commande vise une direction (relative à la caméra, inclinaison comprise).
        Vector3 forward(0, 0, -1), right(1, 0, 0);
        if (camera_relative)
        {
            if (Camera3D *camera = get_viewport() ? get_viewport()->get_camera_3d() : nullptr)
            {
                Basis basis = camera->get_global_transform().basis;
                forward = -basis.get_column(2).normalized();
                right = basis.get_column(0).normalized();
            }
        }
        desired = (right * move_input.x + forward * -move_input.y + Vector3(0, vertical_input, 0)).limit_length(1.0f);
        if (at_surface && desired.y > 0.0f)
        {
            desired.y = 0.0f; // à la surface, monter ne fait pas nager : on y reste
        }
        if (desired.length_squared() > 0.01f)
        {
            heading = turn_toward(heading, desired, step);
        }
        target_speed = desired.length() * max_speed;
    }
    if (at_surface && heading.y > 0.0f)
    {
        Vector3 flat(heading.x, 0.0f, heading.z);
        heading = turn_toward(heading, flat.length_squared() > 1e-4f ? flat.normalized() : Vector3(0, 0, -1), step * 2.0);
    }
    swim.speed = approach_speed(swim.speed, target_speed, swim.acceleration, p_delta);
    swim.stroke_timer = MAX(0.0, swim.stroke_timer - p_delta);

    drift += (water->get_current() + external_acceleration_3d(character)) * p_delta;
    drift *= MAX(0.0, 1.0 - swim.drag * p_delta);
    Vector3 velocity = heading * swim.speed + drift;
    if (has_jump_request(true))
    {
        if (depth <= swim.surface_tolerance)
        {
            velocity.y = swim.jump_out_velocity;
            swim.leaping = true;
            emit_signal("jumped_out");
            character->set_velocity(velocity);
            character->move_and_slide();
            _orient(p_delta);
            return;
        }
        if (swim.stroke_timer <= 0.0)
        {
            swim.speed = MIN(swim.speed + swim.stroke_velocity, max_speed * 2.0);
            swim.stroke_timer = swim.stroke_cooldown;
            velocity = heading * swim.speed + drift;
            emit_signal("stroked");
        }
    }
    if (depth < 0.0)
    {
        velocity.y = character->get_velocity().y + character->get_gravity().y * p_delta; // au-dessus de la surface : retombe
    }
    else if (depth < swim.float_depth && velocity.y > 0.0f)
    {
        velocity.y = float((depth - swim.float_depth) * 4.0); // ne pas émerger : retour sous la surface
    }
    character->set_velocity(velocity);
    character->move_and_slide();
    swim.speed = MIN(swim.speed, MAX(0.0, double((character->get_velocity() - drift).dot(heading))));
    _orient(p_delta);
}

void SwimMovement3D::_orient(double p_delta)
{
    if (!swim.align_to_heading)
    {
        return;
    }
    Node3D *visual = Object::cast_to<Node3D>(swim.visual_path.is_empty() ? get_body() : get_node_or_null(swim.visual_path));
    if (!visual)
    {
        return;
    }
    Vector3 forward = heading;
    if (get_water() == nullptr)
    {
        forward.y = 0.0f; // hors de l'eau : redressé
    }
    if (forward.length_squared() < 1e-4f)
    {
        return;
    }
    forward.normalize();
    Vector3 up = Math::abs(forward.y) > 0.98f ? -visual->get_global_basis().get_column(2) : Vector3(0, 1, 0);
    Basis target = Basis::looking_at(forward, up);
    if (swim.style == SwimSettings::STYLE_FISH && get_water())
    {
        double intensity = swim.swim_speed > 0.0 ? CLAMP(swim.speed / swim.swim_speed, 0.3, 1.5) : 0.0;
        target = target * Basis(Vector3(0, 1, 0), float(Math::sin(swim.time * swim.undulation_frequency * Math_TAU) * swim.undulation_amplitude * intensity));
    }
    Basis current = visual->get_global_basis().orthonormalized();
    Vector3 scale = visual->get_global_basis().get_scale();
    Basis blended = current.slerp(target, float(MIN(1.0, 10.0 * p_delta)));
    visual->set_global_basis(blended.scaled_local(scale));
}

void SwimMovement3D::_on_deactivated()
{
    swim.initialized = false;
    swim.leaping = false;
    if (!swim.align_to_heading)
    {
        return;
    }
    // Redressé : on ne garde que le lacet.
    if (Node3D *visual = Object::cast_to<Node3D>(swim.visual_path.is_empty() ? get_body() : get_node_or_null(swim.visual_path)))
    {
        Vector3 forward = -visual->get_global_basis().get_column(2);
        forward.y = 0.0f;
        if (forward.length_squared() > 1e-4f)
        {
            Vector3 scale = visual->get_global_basis().get_scale();
            visual->set_global_basis(Basis::looking_at(forward.normalized(), Vector3(0, 1, 0)).scaled_local(scale));
        }
    }
}

String SwimMovement3D::_debug_text() const
{
    static const char32_t *styles[] = {U"flottaison", U"directionnel", U"poisson"};
    return String(styles[swim.style]) + U" · prof. " + String::num(last_depth, 2) + U" m · " + String::num(Math::abs(swim.speed), 1) + " m/s";
}

void SwimMovement3D::_draw_debug(DebugGizmos &p_debug)
{
    if (Node3D *body = Object::cast_to<Node3D>(get_body()))
    {
        Vector3 position = body->get_global_position();
        p_debug.arrow3(position, position + heading * 1.5f, debug_colors::HEADING, 0.2f, 3.0f);
        p_debug.arrow3(position, position + desired, debug_colors::INPUT);
        if (WaterArea3D *water = get_water())
        {
            Vector3 surface(position.x, water->get_surface_y() - swim.float_depth, position.z);
            p_debug.circle3(surface, 0.6f, Vector3(0, 1, 0), Color(0.4f, 0.7f, 1.0f));
        }
    }
    MovementComponent::_draw_debug(p_debug);
}

void SwimMovement3D::_bind_methods()
{
    bind_swim_common<SwimMovement3D>();
    ClassDB::bind_method(D_METHOD("set_camera_relative", "relative"), &SwimMovement3D::set_camera_relative);
    ClassDB::bind_method(D_METHOD("get_camera_relative"), &SwimMovement3D::get_camera_relative);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "camera_relative"), "set_camera_relative", "get_camera_relative");
}
