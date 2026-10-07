#include "rope.hpp"

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/collision_object2d.hpp>
#include <godot_cpp/classes/collision_object3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/physics_direct_space_state2d.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters2d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

LocalVector<ObjectID> Rope2D::ropes;
LocalVector<ObjectID> Rope3D::ropes;

namespace
{
constexpr float POINT_RADIUS_2D = 3.0f;
constexpr float POINT_RADIUS_3D = 0.05f;

template <typename T>
void bind_rope_common(Variant::Type p_vector_type, PropertyHint p_layers_hint)
{
    ClassDB::bind_method(D_METHOD("set_point_count", "count"), &T::set_point_count);
    ClassDB::bind_method(D_METHOD("get_point_count"), &T::get_point_count);
    ClassDB::bind_method(D_METHOD("set_rope_length", "length"), &T::set_rope_length);
    ClassDB::bind_method(D_METHOD("get_rope_length"), &T::get_rope_length);
    ClassDB::bind_method(D_METHOD("set_gravity", "gravity"), &T::set_gravity);
    ClassDB::bind_method(D_METHOD("get_gravity"), &T::get_gravity);
    ClassDB::bind_method(D_METHOD("set_damping", "damping"), &T::set_damping);
    ClassDB::bind_method(D_METHOD("get_damping"), &T::get_damping);
    ClassDB::bind_method(D_METHOD("set_iterations", "iterations"), &T::set_iterations);
    ClassDB::bind_method(D_METHOD("get_iterations"), &T::get_iterations);
    ClassDB::bind_method(D_METHOD("set_start_anchor_path", "path"), &T::set_start_anchor_path);
    ClassDB::bind_method(D_METHOD("get_start_anchor_path"), &T::get_start_anchor_path);
    ClassDB::bind_method(D_METHOD("set_end_anchor_path", "path"), &T::set_end_anchor_path);
    ClassDB::bind_method(D_METHOD("get_end_anchor_path"), &T::get_end_anchor_path);
    ClassDB::bind_method(D_METHOD("set_collide", "collide"), &T::set_collide);
    ClassDB::bind_method(D_METHOD("get_collide"), &T::get_collide);
    ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"), &T::set_collision_mask);
    ClassDB::bind_method(D_METHOD("get_collision_mask"), &T::get_collision_mask);
    ClassDB::bind_method(D_METHOD("set_attached_weight", "weight"), &T::set_attached_weight);
    ClassDB::bind_method(D_METHOD("get_attached_weight"), &T::get_attached_weight);
    ClassDB::bind_method(D_METHOD("set_color", "color"), &T::set_color);
    ClassDB::bind_method(D_METHOD("get_color"), &T::get_color);
    ClassDB::bind_method(D_METHOD("reset_points"), &T::reset_points);
    ClassDB::bind_method(D_METHOD("get_points"), &T::get_points);
    ClassDB::bind_method(D_METHOD("get_segment_length"), &T::get_segment_length);
    ClassDB::bind_method(D_METHOD("get_position_at", "position"), &T::get_position_at);
    ClassDB::bind_method(D_METHOD("get_velocity_at", "position"), &T::get_velocity_at);
    ClassDB::bind_method(D_METHOD("get_closest_position", "point"), &T::get_closest_position);
    ClassDB::bind_method(D_METHOD("apply_impulse_at", "position", "velocity"), &T::apply_impulse_at);
    ClassDB::bind_method(D_METHOD("attach_body", "body", "position"), &T::attach_body);
    ClassDB::bind_method(D_METHOD("detach_body", "body"), &T::detach_body);
    ClassDB::bind_method(D_METHOD("has_body", "body"), &T::has_body);
    ClassDB::bind_method(D_METHOD("get_attached_position", "body"), &T::get_attached_position);
    ClassDB::bind_method(D_METHOD("set_attached_position", "body", "position"), &T::set_attached_position);
    ClassDB::bind_static_method(T::get_class_static(), D_METHOD("find_rope_of", "body"), &T::find_rope_of);

    StringName cls = T::get_class_static();
    ClassDB::add_property_group(cls, U"🪢 Corde", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::INT, "point_count", PROPERTY_HINT_RANGE, "2,128,1"), "set_point_count", "get_point_count");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "rope_length", PROPERTY_HINT_RANGE, "0.01,5000,0.01,or_greater"), "set_rope_length", "get_rope_length");
    ClassDB::add_property(cls, PropertyInfo(p_vector_type, "gravity"), "set_gravity", "get_gravity");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "damping", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_damping", "get_damping");
    ClassDB::add_property(cls, PropertyInfo(Variant::INT, "iterations", PROPERTY_HINT_RANGE, "1,100,1"), "set_iterations", "get_iterations");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "attached_weight", PROPERTY_HINT_RANGE, "0,20,0.01,or_greater"), "set_attached_weight", "get_attached_weight");
    ClassDB::add_property_group(cls, U"⚓ Ancrages", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::NODE_PATH, "start_anchor_path"), "set_start_anchor_path", "get_start_anchor_path");
    ClassDB::add_property(cls, PropertyInfo(Variant::NODE_PATH, "end_anchor_path"), "set_end_anchor_path", "get_end_anchor_path");
    ClassDB::add_property_group(cls, U"💥 Collisions", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::BOOL, "collide"), "set_collide", "get_collide");
    ClassDB::add_property(cls, PropertyInfo(Variant::INT, "collision_mask", p_layers_hint), "set_collision_mask", "get_collision_mask");
    ClassDB::add_property_group(cls, U"🎨 Rendu", "");
    ClassDB::add_property(cls, PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
    ClassDB::add_signal(cls, MethodInfo("body_attached", PropertyInfo(Variant::OBJECT, "body")));
    ClassDB::add_signal(cls, MethodInfo("body_detached", PropertyInfo(Variant::OBJECT, "body")));
}

template <typename T>
void bind_rope_movement_common()
{
    ClassDB::bind_method(D_METHOD("set_grab_distance", "value"), &T::set_grab_distance);
    ClassDB::bind_method(D_METHOD("get_grab_distance"), &T::get_grab_distance);
    ClassDB::bind_method(D_METHOD("set_climb_speed", "value"), &T::set_climb_speed);
    ClassDB::bind_method(D_METHOD("get_climb_speed"), &T::get_climb_speed);
    ClassDB::bind_method(D_METHOD("set_swing_force", "value"), &T::set_swing_force);
    ClassDB::bind_method(D_METHOD("get_swing_force"), &T::get_swing_force);
    ClassDB::bind_method(D_METHOD("set_jump_up_velocity", "value"), &T::set_jump_up_velocity);
    ClassDB::bind_method(D_METHOD("get_jump_up_velocity"), &T::get_jump_up_velocity);
    ClassDB::bind_method(D_METHOD("set_regrab_delay", "value"), &T::set_regrab_delay);
    ClassDB::bind_method(D_METHOD("get_regrab_delay"), &T::get_regrab_delay);
    ClassDB::bind_method(D_METHOD("try_grab"), &T::try_grab);
    ClassDB::bind_method(D_METHOD("release", "jump"), &T::release, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("is_attached"), &T::is_attached);
    ClassDB::bind_method(D_METHOD("get_rope"), &T::get_rope);

    StringName cls = T::get_class_static();
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "grab_distance", PROPERTY_HINT_RANGE, "0,500,0.01,or_greater"), "set_grab_distance", "get_grab_distance");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "climb_speed", PROPERTY_HINT_RANGE, "0,2000,0.01,or_greater"), "set_climb_speed", "get_climb_speed");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "swing_force", PROPERTY_HINT_RANGE, "0,10000,0.01,or_greater"), "set_swing_force", "get_swing_force");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "jump_up_velocity", PROPERTY_HINT_RANGE, "0,3000,0.01,or_greater"), "set_jump_up_velocity", "get_jump_up_velocity");
    ClassDB::add_property(cls, PropertyInfo(Variant::FLOAT, "regrab_delay", PROPERTY_HINT_RANGE, "0,3,0.01,or_greater,suffix:s"), "set_regrab_delay", "get_regrab_delay");
    ClassDB::add_signal(cls, MethodInfo("grabbed", PropertyInfo(Variant::OBJECT, "rope")));
    ClassDB::add_signal(cls, MethodInfo("released"));
}

template <typename V>
V lerp_points(const LocalVector<V> &p_points, double p_position)
{
    if (p_points.is_empty())
    {
        return V();
    }
    double clamped = CLAMP(p_position, 0.0, double(p_points.size() - 1));
    int index = MIN(int(clamped), int(p_points.size()) - 2);
    if (index < 0)
    {
        return p_points[0];
    }
    double fraction = clamped - index;
    return p_points[index] + (p_points[index + 1] - p_points[index]) * fraction;
}

/// Index fractionnaire du point de la polyligne le plus proche, et distance au carré.
template <typename V>
double closest_position(const LocalVector<V> &p_points, const V &p_point, double &r_distance_squared)
{
    double best = 0.0;
    r_distance_squared = 1e30;
    for (int i = 0; i + 1 < int(p_points.size()); i++)
    {
        V segment = p_points[i + 1] - p_points[i];
        double length_squared = segment.length_squared();
        double t = length_squared > 1e-12 ? CLAMP((p_point - p_points[i]).dot(segment) / length_squared, 0.0, 1.0) : 0.0;
        V projected = p_points[i] + segment * t;
        double distance_squared = (p_point - projected).length_squared();
        if (distance_squared < r_distance_squared)
        {
            r_distance_squared = distance_squared;
            best = i + t;
        }
    }
    return best;
}

template <typename V>
void solve_constraints(LocalVector<V> &r_points, double p_segment, int p_iterations, bool p_end_pinned)
{
    int last = int(r_points.size()) - 1;
    for (int iteration = 0; iteration < p_iterations; iteration++)
    {
        for (int i = 0; i < last; i++)
        {
            V delta = r_points[i + 1] - r_points[i];
            double distance = delta.length();
            if (distance < 1e-9)
            {
                continue;
            }
            double weight_a = i == 0 ? 0.0 : 1.0;
            double weight_b = (i + 1 == last && p_end_pinned) ? 0.0 : 1.0;
            double total = weight_a + weight_b;
            if (total <= 0.0)
            {
                continue;
            }
            V correction = delta * ((distance - p_segment) / distance);
            r_points[i] += correction * (weight_a / total);
            r_points[i + 1] -= correction * (weight_b / total);
        }
    }
}
} // namespace

// =====================================================================================================
// 🪢 Rope2D
// =====================================================================================================

void Rope2D::set_point_count(int p_count)
{
    point_count = CLAMP(p_count, 2, 128);
    if (is_inside_tree())
    {
        reset_points();
    }
}

Vector2 Rope2D::_start_position() const
{
    Node2D *anchor = start_anchor_path.is_empty() ? nullptr : Object::cast_to<Node2D>(get_node_or_null(start_anchor_path));
    const Node2D *origin = anchor ? anchor : this;
    return origin->is_inside_tree() ? origin->get_global_position() : origin->get_position();
}

Node2D *Rope2D::_end_anchor() const { return end_anchor_path.is_empty() ? nullptr : Object::cast_to<Node2D>(get_node_or_null(end_anchor_path)); }

void Rope2D::reset_points()
{
    points.resize(point_count);
    previous.resize(point_count);
    Vector2 start = _start_position();
    Node2D *end = _end_anchor();
    Vector2 direction = end ? (end->get_global_position() - start).normalized() : (gravity.length_squared() > 0 ? gravity.normalized() : Vector2(0, 1));
    for (int i = 0; i < point_count; i++)
    {
        points[i] = start + direction * get_segment_length() * i;
        previous[i] = points[i];
    }
    queue_redraw();
}

PackedVector2Array Rope2D::get_points() const
{
    PackedVector2Array result;
    for (const Vector2 &point : points)
    {
        result.push_back(point);
    }
    return result;
}

Vector2 Rope2D::get_position_at(double p_position) const { return lerp_points(points, p_position); }

Vector2 Rope2D::get_velocity_at(double p_position) const
{
    return (lerp_points(points, p_position) - lerp_points(previous, p_position)) / MAX(last_delta, 1e-4);
}

double Rope2D::get_closest_position(const Vector2 &p_point) const
{
    double distance_squared = 0.0;
    return closest_position(points, p_point, distance_squared);
}

void Rope2D::apply_impulse_at(double p_position, const Vector2 &p_velocity)
{
    if (points.size() < 2)
    {
        return;
    }
    double clamped = CLAMP(p_position, 0.0, double(points.size() - 1));
    int index = MIN(int(clamped), int(points.size()) - 2);
    double fraction = clamped - index;
    // Vitesse ajoutée en reculant la position précédente (Verlet).
    previous[index] -= p_velocity * last_delta * (1.0 - fraction);
    previous[index + 1] -= p_velocity * last_delta * fraction;
}

void Rope2D::attach_body(Node *p_body, double p_position)
{
    if (!p_body || has_body(p_body))
    {
        return;
    }
    attachments.push_back({ObjectID(p_body->get_instance_id()), CLAMP(p_position, 0.0, double(point_count - 1))});
    emit_signal("body_attached", p_body);
}

void Rope2D::detach_body(Node *p_body)
{
    for (uint32_t i = 0; i < attachments.size(); i++)
    {
        if (attachments[i].body == ObjectID(p_body->get_instance_id()))
        {
            attachments.remove_at(i);
            emit_signal("body_detached", p_body);
            return;
        }
    }
}

bool Rope2D::has_body(Node *p_body) const { return get_attached_position(p_body) >= 0.0; }

double Rope2D::get_attached_position(Node *p_body) const
{
    for (const Attachment &attachment : attachments)
    {
        if (p_body && attachment.body == ObjectID(p_body->get_instance_id()))
        {
            return attachment.position;
        }
    }
    return -1.0;
}

void Rope2D::set_attached_position(Node *p_body, double p_position)
{
    for (Attachment &attachment : attachments)
    {
        if (p_body && attachment.body == ObjectID(p_body->get_instance_id()))
        {
            attachment.position = CLAMP(p_position, 0.0, double(point_count - 1));
        }
    }
}

Rope2D *Rope2D::find_rope_of(Node *p_body)
{
    for (uint32_t i = 0; p_body && i < ropes.size(); i++)
    {
        Rope2D *rope = Object::cast_to<Rope2D>(ObjectDB::get_instance(ropes[i]));
        if (rope && rope->has_body(p_body))
        {
            return rope;
        }
    }
    return nullptr;
}

Rope2D *Rope2D::find_nearest(const Vector2 &p_point, double p_max_distance, double &r_position)
{
    Rope2D *nearest = nullptr;
    double best = p_max_distance * p_max_distance;
    for (uint32_t i = 0; i < ropes.size(); i++)
    {
        Rope2D *rope = Object::cast_to<Rope2D>(ObjectDB::get_instance(ropes[i]));
        if (!rope)
        {
            continue;
        }
        double distance_squared = 0.0;
        double position = closest_position(rope->points, p_point, distance_squared);
        if (distance_squared <= best)
        {
            best = distance_squared;
            nearest = rope;
            r_position = position;
        }
    }
    return nearest;
}

Variant Rope2D::find_nearest_info(const Vector2 &p_point, double p_max_distance)
{
    double position = 0.0;
    Rope2D *rope = find_nearest(p_point, p_max_distance, position);
    if (!rope)
    {
        return Variant();
    }
    Dictionary info;
    info["rope"] = rope;
    info["position"] = position;
    return info;
}

void Rope2D::_simulate(double p_delta)
{
    if (int(points.size()) != point_count)
    {
        reset_points();
    }
    last_delta = p_delta;
    const int last = point_count - 1;
    Node2D *end = _end_anchor();
    // Poids des corps accrochés : gravité supplémentaire sur les points voisins.
    LocalVector<double> extra;
    extra.resize(point_count);
    for (int i = 0; i < point_count; i++)
    {
        extra[i] = 0.0;
    }
    for (const Attachment &attachment : attachments)
    {
        int index = MIN(int(attachment.position), last - 1);
        double fraction = attachment.position - index;
        extra[index] += attached_weight * (1.0 - fraction);
        extra[index + 1] += attached_weight * fraction;
    }
    for (int i = 1; i <= last; i++)
    {
        if (i == last && end)
        {
            continue;
        }
        Vector2 velocity = (points[i] - previous[i]) * (1.0 - damping);
        previous[i] = points[i];
        points[i] += velocity + gravity * (1.0 + extra[i]) * p_delta * p_delta;
    }
    previous[0] = points[0];
    points[0] = _start_position();
    if (end)
    {
        previous[last] = points[last];
        points[last] = end->get_global_position();
    }
    solve_constraints(points, get_segment_length(), iterations, end != nullptr);

    if (collide && is_inside_tree())
    {
        PhysicsDirectSpaceState2D *space = get_world_2d()->get_direct_space_state();
        TypedArray<RID> exclude;
        for (const Attachment &attachment : attachments)
        {
            if (CollisionObject2D *body = Object::cast_to<CollisionObject2D>(ObjectDB::get_instance(attachment.body)))
            {
                exclude.push_back(body->get_rid());
            }
        }
        for (int i = 1; i <= last; i++)
        {
            Ref<PhysicsRayQueryParameters2D> query = PhysicsRayQueryParameters2D::create(previous[i], points[i], collision_mask, exclude);
            Dictionary hit = space->intersect_ray(query);
            if (!hit.is_empty())
            {
                Vector2 position = hit["position"];
                Vector2 normal = hit["normal"];
                points[i] = position + normal * POINT_RADIUS_2D;
            }
        }
    }

    // Les corps accrochés suivent la corde.
    for (uint32_t a = 0; a < attachments.size(); a++)
    {
        Node2D *body = Object::cast_to<Node2D>(ObjectDB::get_instance(attachments[a].body));
        if (!body || !body->is_inside_tree())
        {
            attachments.remove_at(a--);
            continue;
        }
        body->set_global_position(get_position_at(attachments[a].position));
        if (CharacterBody2D *character = Object::cast_to<CharacterBody2D>(body))
        {
            character->set_velocity(get_velocity_at(attachments[a].position));
        }
    }
    queue_redraw();
}

void Rope2D::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_READY:
        reset_points();
        if (!Engine::get_singleton()->is_editor_hint())
        {
            set_physics_process(true);
        }
        break;
    case NOTIFICATION_ENTER_TREE:
        if (!Engine::get_singleton()->is_editor_hint())
        {
            ropes.push_back(ObjectID(get_instance_id()));
        }
        break;
    case NOTIFICATION_EXIT_TREE:
    {
        int64_t index = ropes.find(ObjectID(get_instance_id()));
        if (index >= 0)
        {
            ropes.remove_at(index);
        }
        debug.release();
        break;
    }
    case NOTIFICATION_PHYSICS_PROCESS:
        _simulate(get_physics_process_delta_time());
        if (debug_mode)
        {
            _draw_debug();
        }
        break;
    case NOTIFICATION_DRAW:
    {
        if (Engine::get_singleton()->is_editor_hint())
        {
            reset_points(); // aperçu dans l'éditeur : corde tendue au repos
        }
        PackedVector2Array local;
        Transform2D inverse = get_global_transform().affine_inverse();
        for (const Vector2 &point : points)
        {
            local.push_back(inverse.xform(point));
        }
        if (local.size() >= 2)
        {
            draw_polyline(local, color, width, true);
        }
        break;
    }
    default:
        break;
    }
}

void Rope2D::_bind_methods()
{
    bind_rope_common<Rope2D>(Variant::VECTOR2, PROPERTY_HINT_LAYERS_2D_PHYSICS);
    ClassDB::bind_method(D_METHOD("set_width", "width"), &Rope2D::set_width);
    ClassDB::bind_method(D_METHOD("get_width"), &Rope2D::get_width);
    ClassDB::bind_static_method("Rope2D", D_METHOD("find_nearest_info", "point", "max_distance"), &Rope2D::find_nearest_info);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "width", PROPERTY_HINT_RANGE, "0,50,0.01,or_greater"), "set_width", "get_width");
    bind_debug_mode<Rope2D>();
}

void Rope2D::_draw_debug()
{
    debug.begin(this, false);
    PackedVector2Array path;
    for (const Vector2 &point : points)
    {
        path.push_back(point);
    }
    debug.path(path, Color(1.0f, 0.8f, 0.3f, 0.8f), true, 1.0f);
    debug.circle(get_global_position(), 6.0f, debug_colors::NORMAL, true);
    if (!points.is_empty())
    {
        debug.text(points[points.size() - 1] + Vector2(8, 0), String(U"🪢 ") + String::num(rope_length, 0) + U" px · " + String::num_int64(points.size()) + U" points", debug_colors::TEXT, 11);
    }
    debug.end();
}

// =====================================================================================================
// 🪢 Rope3D
// =====================================================================================================

Rope3D::Rope3D()
{
    mesh.instantiate();
    material.instantiate();
    material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
    material->set_albedo(color);
    mesh_instance = memnew(MeshInstance3D);
    mesh_instance->set_mesh(mesh);
    mesh_instance->set_as_top_level(true); // points en coordonnées du monde
    add_child(mesh_instance, false, INTERNAL_MODE_FRONT);
}

void Rope3D::set_color(const Color &p_color)
{
    color = p_color;
    material->set_albedo(color);
}

void Rope3D::set_point_count(int p_count)
{
    point_count = CLAMP(p_count, 2, 128);
    if (is_inside_tree())
    {
        reset_points();
    }
}

Vector3 Rope3D::_start_position() const
{
    Node3D *anchor = start_anchor_path.is_empty() ? nullptr : Object::cast_to<Node3D>(get_node_or_null(start_anchor_path));
    const Node3D *origin = anchor ? anchor : this;
    return origin->is_inside_tree() ? origin->get_global_position() : origin->get_position();
}

Node3D *Rope3D::_end_anchor() const { return end_anchor_path.is_empty() ? nullptr : Object::cast_to<Node3D>(get_node_or_null(end_anchor_path)); }

void Rope3D::reset_points()
{
    points.resize(point_count);
    previous.resize(point_count);
    Vector3 start = _start_position();
    Node3D *end = _end_anchor();
    Vector3 direction = end ? (end->get_global_position() - start).normalized() : (gravity.length_squared() > 0 ? gravity.normalized() : Vector3(0, -1, 0));
    for (int i = 0; i < point_count; i++)
    {
        points[i] = start + direction * get_segment_length() * i;
        previous[i] = points[i];
    }
    _redraw();
}

PackedVector3Array Rope3D::get_points() const
{
    PackedVector3Array result;
    for (const Vector3 &point : points)
    {
        result.push_back(point);
    }
    return result;
}

Vector3 Rope3D::get_position_at(double p_position) const { return lerp_points(points, p_position); }

Vector3 Rope3D::get_velocity_at(double p_position) const
{
    return (lerp_points(points, p_position) - lerp_points(previous, p_position)) / MAX(last_delta, 1e-4);
}

double Rope3D::get_closest_position(const Vector3 &p_point) const
{
    double distance_squared = 0.0;
    return closest_position(points, p_point, distance_squared);
}

void Rope3D::apply_impulse_at(double p_position, const Vector3 &p_velocity)
{
    if (points.size() < 2)
    {
        return;
    }
    double clamped = CLAMP(p_position, 0.0, double(points.size() - 1));
    int index = MIN(int(clamped), int(points.size()) - 2);
    double fraction = clamped - index;
    previous[index] -= p_velocity * last_delta * (1.0 - fraction);
    previous[index + 1] -= p_velocity * last_delta * fraction;
}

void Rope3D::attach_body(Node *p_body, double p_position)
{
    if (!p_body || has_body(p_body))
    {
        return;
    }
    attachments.push_back({ObjectID(p_body->get_instance_id()), CLAMP(p_position, 0.0, double(point_count - 1))});
    emit_signal("body_attached", p_body);
}

void Rope3D::detach_body(Node *p_body)
{
    for (uint32_t i = 0; i < attachments.size(); i++)
    {
        if (attachments[i].body == ObjectID(p_body->get_instance_id()))
        {
            attachments.remove_at(i);
            emit_signal("body_detached", p_body);
            return;
        }
    }
}

bool Rope3D::has_body(Node *p_body) const { return get_attached_position(p_body) >= 0.0; }

double Rope3D::get_attached_position(Node *p_body) const
{
    for (const Attachment &attachment : attachments)
    {
        if (p_body && attachment.body == ObjectID(p_body->get_instance_id()))
        {
            return attachment.position;
        }
    }
    return -1.0;
}

void Rope3D::set_attached_position(Node *p_body, double p_position)
{
    for (Attachment &attachment : attachments)
    {
        if (p_body && attachment.body == ObjectID(p_body->get_instance_id()))
        {
            attachment.position = CLAMP(p_position, 0.0, double(point_count - 1));
        }
    }
}

Rope3D *Rope3D::find_rope_of(Node *p_body)
{
    for (uint32_t i = 0; p_body && i < ropes.size(); i++)
    {
        Rope3D *rope = Object::cast_to<Rope3D>(ObjectDB::get_instance(ropes[i]));
        if (rope && rope->has_body(p_body))
        {
            return rope;
        }
    }
    return nullptr;
}

Rope3D *Rope3D::find_nearest(const Vector3 &p_point, double p_max_distance, double &r_position)
{
    Rope3D *nearest = nullptr;
    double best = p_max_distance * p_max_distance;
    for (uint32_t i = 0; i < ropes.size(); i++)
    {
        Rope3D *rope = Object::cast_to<Rope3D>(ObjectDB::get_instance(ropes[i]));
        if (!rope)
        {
            continue;
        }
        double distance_squared = 0.0;
        double position = closest_position(rope->points, p_point, distance_squared);
        if (distance_squared <= best)
        {
            best = distance_squared;
            nearest = rope;
            r_position = position;
        }
    }
    return nearest;
}

void Rope3D::_simulate(double p_delta)
{
    if (int(points.size()) != point_count)
    {
        reset_points();
    }
    last_delta = p_delta;
    const int last = point_count - 1;
    Node3D *end = _end_anchor();
    LocalVector<double> extra;
    extra.resize(point_count);
    for (int i = 0; i < point_count; i++)
    {
        extra[i] = 0.0;
    }
    for (const Attachment &attachment : attachments)
    {
        int index = MIN(int(attachment.position), last - 1);
        double fraction = attachment.position - index;
        extra[index] += attached_weight * (1.0 - fraction);
        extra[index + 1] += attached_weight * fraction;
    }
    for (int i = 1; i <= last; i++)
    {
        if (i == last && end)
        {
            continue;
        }
        Vector3 velocity = (points[i] - previous[i]) * (1.0 - damping);
        previous[i] = points[i];
        points[i] += velocity + gravity * (1.0 + extra[i]) * p_delta * p_delta;
    }
    previous[0] = points[0];
    points[0] = _start_position();
    if (end)
    {
        previous[last] = points[last];
        points[last] = end->get_global_position();
    }
    solve_constraints(points, get_segment_length(), iterations, end != nullptr);

    if (collide && is_inside_tree())
    {
        PhysicsDirectSpaceState3D *space = get_world_3d()->get_direct_space_state();
        TypedArray<RID> exclude;
        for (const Attachment &attachment : attachments)
        {
            if (CollisionObject3D *body = Object::cast_to<CollisionObject3D>(ObjectDB::get_instance(attachment.body)))
            {
                exclude.push_back(body->get_rid());
            }
        }
        for (int i = 1; i <= last; i++)
        {
            Ref<PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(previous[i], points[i], collision_mask, exclude);
            Dictionary hit = space->intersect_ray(query);
            if (!hit.is_empty())
            {
                Vector3 position = hit["position"];
                Vector3 normal = hit["normal"];
                points[i] = position + normal * POINT_RADIUS_3D;
            }
        }
    }

    for (uint32_t a = 0; a < attachments.size(); a++)
    {
        Node3D *body = Object::cast_to<Node3D>(ObjectDB::get_instance(attachments[a].body));
        if (!body || !body->is_inside_tree())
        {
            attachments.remove_at(a--);
            continue;
        }
        body->set_global_position(get_position_at(attachments[a].position));
        if (CharacterBody3D *character = Object::cast_to<CharacterBody3D>(body))
        {
            character->set_velocity(get_velocity_at(attachments[a].position));
        }
    }
    _redraw();
}

void Rope3D::_redraw()
{
    if (mesh.is_null())
    {
        return;
    }
    mesh->clear_surfaces();
    if (points.size() < 2)
    {
        return;
    }
    mesh->surface_begin(Mesh::PRIMITIVE_LINE_STRIP, material);
    for (const Vector3 &point : points)
    {
        mesh->surface_add_vertex(point);
    }
    mesh->surface_end();
}

void Rope3D::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_READY:
        reset_points();
        if (!Engine::get_singleton()->is_editor_hint())
        {
            set_physics_process(true);
        }
        break;
    case NOTIFICATION_ENTER_TREE:
        if (!Engine::get_singleton()->is_editor_hint())
        {
            ropes.push_back(ObjectID(get_instance_id()));
        }
        break;
    case NOTIFICATION_EXIT_TREE:
    {
        int64_t index = ropes.find(ObjectID(get_instance_id()));
        if (index >= 0)
        {
            ropes.remove_at(index);
        }
        debug.release();
        break;
    }
    case NOTIFICATION_PHYSICS_PROCESS:
        _simulate(get_physics_process_delta_time());
        if (debug_mode)
        {
            _draw_debug();
        }
        break;
    case NOTIFICATION_TRANSFORM_CHANGED:
        if (Engine::get_singleton()->is_editor_hint())
        {
            reset_points();
        }
        break;
    default:
        break;
    }
}

void Rope3D::_bind_methods()
{
    bind_rope_common<Rope3D>(Variant::VECTOR3, PROPERTY_HINT_LAYERS_3D_PHYSICS);
    bind_debug_mode<Rope3D>();
}

void Rope3D::_draw_debug()
{
    debug.begin(this, true);
    PackedVector3Array path;
    for (const Vector3 &point : points)
    {
        path.push_back(point);
    }
    debug.path3(path, Color(1.0f, 0.8f, 0.3f), true);
    debug.sphere3(get_global_position(), 0.15f, debug_colors::NORMAL, true);
    if (!points.is_empty())
    {
        debug.text3(points[points.size() - 1] + Vector3(0.3f, 0, 0), String(U"🪢 ") + String::num(rope_length, 1) + U" m · " + String::num_int64(points.size()) + U" points");
    }
    debug.end();
}

// =====================================================================================================
// 🧗‍♂️ RopeMovement2D / 3D
// =====================================================================================================

bool RopeMovement2D::try_grab()
{
    Node2D *body = Object::cast_to<Node2D>(get_body());
    if (!body || is_attached() || Time::get_singleton()->get_ticks_msec() - released_msec < uint64_t(regrab_delay * 1000.0))
    {
        return is_attached();
    }
    double position = 0.0;
    Rope2D *rope = Rope2D::find_nearest(body->get_global_position(), grab_distance, position);
    if (!rope)
    {
        return false;
    }
    rope->attach_body(body, position);
    emit_signal("grabbed", rope);
    return true;
}

void RopeMovement2D::release(bool p_jump)
{
    Rope2D *rope = get_rope();
    if (!rope)
    {
        return;
    }
    Node *body = get_body();
    Vector2 velocity = rope->get_velocity_at(rope->get_attached_position(body));
    rope->detach_body(body);
    if (p_jump)
    {
        velocity.y = MIN(velocity.y, 0.0f) - jump_up_velocity;
    }
    if (CharacterBody2D *character = Object::cast_to<CharacterBody2D>(body))
    {
        character->set_velocity(velocity);
    }
    released_msec = Time::get_singleton()->get_ticks_msec();
    emit_signal("released");
}

void RopeMovement2D::_physics_step(double p_delta)
{
    Rope2D *rope = get_rope();
    if (!rope)
    {
        return;
    }
    Node *body = get_body();
    if (has_jump_request(true))
    {
        release(true);
        return;
    }
    double position = rope->get_attached_position(body) + move_input.y * climb_speed / rope->get_segment_length() * p_delta;
    rope->set_attached_position(body, MAX(position, 1.0)); // pas sur le point d'ancrage
    if (Math::abs(move_input.x) > 0.01f)
    {
        rope->apply_impulse_at(rope->get_attached_position(body), Vector2(move_input.x * swing_force * p_delta, 0));
    }
}

void RopeMovement2D::_bind_methods() { bind_rope_movement_common<RopeMovement2D>(); }

RopeMovement3D::RopeMovement3D() {}

bool RopeMovement3D::try_grab()
{
    Node3D *body = Object::cast_to<Node3D>(get_body());
    if (!body || is_attached() || Time::get_singleton()->get_ticks_msec() - released_msec < uint64_t(regrab_delay * 1000.0))
    {
        return is_attached();
    }
    double position = 0.0;
    Rope3D *rope = Rope3D::find_nearest(body->get_global_position(), grab_distance, position);
    if (!rope)
    {
        return false;
    }
    rope->attach_body(body, position);
    emit_signal("grabbed", rope);
    return true;
}

void RopeMovement3D::release(bool p_jump)
{
    Rope3D *rope = get_rope();
    if (!rope)
    {
        return;
    }
    Node *body = get_body();
    Vector3 velocity = rope->get_velocity_at(rope->get_attached_position(body));
    rope->detach_body(body);
    if (p_jump)
    {
        velocity.y = MAX(velocity.y, 0.0f) + jump_up_velocity;
    }
    if (CharacterBody3D *character = Object::cast_to<CharacterBody3D>(body))
    {
        character->set_velocity(velocity);
    }
    released_msec = Time::get_singleton()->get_ticks_msec();
    emit_signal("released");
}

void RopeMovement3D::_physics_step(double p_delta)
{
    Rope3D *rope = get_rope();
    if (!rope)
    {
        return;
    }
    Node *body = get_body();
    if (has_jump_request(true))
    {
        release(true);
        return;
    }
    float climb = Math::abs(vertical_input) > 0.1f ? vertical_input : -move_input.y;
    double position = rope->get_attached_position(body) - climb * climb_speed / rope->get_segment_length() * p_delta;
    rope->set_attached_position(body, MAX(position, 1.0));
    if (move_input.length_squared() > 0.0001f)
    {
        rope->apply_impulse_at(rope->get_attached_position(body), Vector3(move_input.x, 0, move_input.y) * swing_force * p_delta);
    }
}

void RopeMovement3D::_bind_methods() { bind_rope_movement_common<RopeMovement3D>(); }

// 🐞 Débogage

String RopeMovement2D::_debug_text() const
{
    Rope2D *rope = get_rope();
    return rope ? String(U"accroché · point ") + String::num(rope->get_attached_position(get_body()), 1) : String(U"libre");
}

void RopeMovement2D::_draw_debug(DebugGizmos &p_debug)
{
    Node2D *body = Object::cast_to<Node2D>(get_body());
    if (body && !is_attached())
    {
        p_debug.circle(body->get_global_position(), grab_distance, Color(1.0f, 0.8f, 0.3f, 0.4f), false, 1.0f); // portée d'accroche
    }
    else if (Rope2D *rope = get_rope())
    {
        p_debug.line(body->get_global_position(), rope->get_global_position(), Color(1.0f, 0.8f, 0.3f, 0.6f), 1.0f);
    }
    MovementComponent::_draw_debug(p_debug);
}

String RopeMovement3D::_debug_text() const
{
    Rope3D *rope = get_rope();
    return rope ? String(U"accroché · point ") + String::num(rope->get_attached_position(get_body()), 1) : String(U"libre");
}

void RopeMovement3D::_draw_debug(DebugGizmos &p_debug)
{
    Node3D *body = Object::cast_to<Node3D>(get_body());
    if (body && !is_attached())
    {
        p_debug.sphere3(body->get_global_position(), grab_distance, Color(1.0f, 0.8f, 0.3f, 0.4f));
    }
    else if (Rope3D *rope = get_rope())
    {
        p_debug.line3(body->get_global_position(), rope->get_global_position(), Color(1.0f, 0.8f, 0.3f, 0.6f));
    }
    MovementComponent::_draw_debug(p_debug);
}
