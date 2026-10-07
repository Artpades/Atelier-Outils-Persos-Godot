#include "movement_director.hpp"

#include "../🌊Déplacement Aquatique/water.hpp"
#include "../🌬️Recule/knockback.hpp"
#include "../🦶Déplacement au sol d'objet/ground_movement.hpp"
#include "../🧗Déplacement Mural/wall_movement.hpp"
#include "../🪂Physique Aérodynamique/aerodynamic_movement.hpp"
#include "../🪢Physique de Corde/rope.hpp"

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// =====================================================================================================
// 🎬 MovementContext
// =====================================================================================================

void MovementContext::set_expression(const String &p_expression)
{
    expression = p_expression;
    parsed.unref();
    parse_failed = false;
    emit_changed();
}

bool MovementContext::evaluate_expression(Node *p_body, Node *p_director)
{
    if (expression.strip_edges().is_empty() || parse_failed)
    {
        return false;
    }
    if (parsed.is_null())
    {
        parsed.instantiate();
        PackedStringArray names;
        names.push_back("body");
        names.push_back("director");
        if (parsed->parse(expression, names) != OK)
        {
            parse_failed = true;
            UtilityFunctions::push_warning(String(U"MovementContext « ") + String(context_name) + U" » : expression invalide : " + parsed->get_error_text());
            return false;
        }
    }
    Array inputs;
    inputs.push_back(p_body);
    inputs.push_back(p_director);
    Variant result = parsed->execute(inputs, p_director, false);
    return !parsed->has_execute_failed() && result.booleanize();
}

void MovementContext::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_context_name", "name"), &MovementContext::set_context_name);
    ClassDB::bind_method(D_METHOD("get_context_name"), &MovementContext::get_context_name);
    ClassDB::bind_method(D_METHOD("set_components", "components"), &MovementContext::set_components);
    ClassDB::bind_method(D_METHOD("get_components"), &MovementContext::get_components);
    ClassDB::bind_method(D_METHOD("set_condition", "condition"), &MovementContext::set_condition);
    ClassDB::bind_method(D_METHOD("get_condition"), &MovementContext::get_condition);
    ClassDB::bind_method(D_METHOD("set_expression", "expression"), &MovementContext::set_expression);
    ClassDB::bind_method(D_METHOD("get_expression"), &MovementContext::get_expression);
    ClassDB::bind_method(D_METHOD("set_priority", "priority"), &MovementContext::set_priority);
    ClassDB::bind_method(D_METHOD("get_priority"), &MovementContext::get_priority);
    ClassDB::bind_method(D_METHOD("evaluate_expression", "body", "director"), &MovementContext::evaluate_expression);

    BIND_ENUM_CONSTANT(CONDITION_ALWAYS);
    BIND_ENUM_CONSTANT(CONDITION_ON_FLOOR);
    BIND_ENUM_CONSTANT(CONDITION_IN_AIR);
    BIND_ENUM_CONSTANT(CONDITION_IN_WATER);
    BIND_ENUM_CONSTANT(CONDITION_ON_WALL);
    BIND_ENUM_CONSTANT(CONDITION_ON_ROPE);
    BIND_ENUM_CONSTANT(CONDITION_KNOCKED);
    BIND_ENUM_CONSTANT(CONDITION_MANUAL);
    BIND_ENUM_CONSTANT(CONDITION_CUSTOM);

    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "context_name"), "set_context_name", "get_context_name");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "components", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::NODE_PATH) + "/" + String::num_int64(PROPERTY_HINT_NODE_PATH_VALID_TYPES) + ":MovementComponent"), "set_components", "get_components");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "condition", PROPERTY_HINT_ENUM, "Always,On Floor,In Air,In Water,On Wall,On Rope,Knocked,Manual,Custom"), "set_condition", "get_condition");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "expression", PROPERTY_HINT_EXPRESSION), "set_expression", "get_expression");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "priority", PROPERTY_HINT_RANGE, "-100,100,1,or_greater,or_less"), "set_priority", "get_priority");
}

// =====================================================================================================
// 🎬 MovementDirector
// =====================================================================================================

MovementDirector::MovementDirector()
{
    // Avant les composants : le contexte est choisi avant qu'ils ne bougent le corps.
    set_physics_process_priority(-20);
}

TypedArray<MovementComponent> MovementDirector::get_components() const
{
    TypedArray<MovementComponent> result;
    Node *body = get_body();
    for (int i = 0; body && i < body->get_child_count(); i++)
    {
        if (MovementComponent *component = Object::cast_to<MovementComponent>(body->get_child(i)))
        {
            result.push_back(component);
        }
    }
    return result;
}

// 🎮 Intention

void MovementDirector::move(const Vector2 &p_input)
{
    move_input = p_input.limit_length(1.0f);
    TypedArray<MovementComponent> components = get_components();
    for (int i = 0; i < components.size(); i++)
    {
        Object::cast_to<MovementComponent>(components[i])->move(move_input);
    }
}

void MovementDirector::jump()
{
    // Seulement aux composants actifs : un composant inactif ne doit pas garder un saut en mémoire.
    jump_held = true;
    TypedArray<MovementComponent> components = get_components();
    for (int i = 0; i < components.size(); i++)
    {
        MovementComponent *component = Object::cast_to<MovementComponent>(components[i]);
        if (component->is_active())
        {
            component->jump();
        }
    }
}

void MovementDirector::release_jump()
{
    jump_held = false;
    TypedArray<MovementComponent> components = get_components();
    for (int i = 0; i < components.size(); i++)
    {
        Object::cast_to<MovementComponent>(components[i])->release_jump();
    }
}

void MovementDirector::set_running(bool p_running)
{
    running = p_running;
    TypedArray<MovementComponent> components = get_components();
    for (int i = 0; i < components.size(); i++)
    {
        Object::cast_to<MovementComponent>(components[i])->set_running(p_running);
    }
}

void MovementDirector::set_vertical_input(float p_value)
{
    vertical_input = p_value;
    TypedArray<MovementComponent> components = get_components();
    for (int i = 0; i < components.size(); i++)
    {
        Object::cast_to<MovementComponent>(components[i])->set_vertical_input(p_value);
    }
}

// 🌍 Environnement

bool MovementDirector::is_on_floor() const
{
    Node *body = get_body();
    if (CharacterBody2D *character = Object::cast_to<CharacterBody2D>(body))
    {
        return character->is_on_floor();
    }
    if (CharacterBody3D *character = Object::cast_to<CharacterBody3D>(body))
    {
        return character->is_on_floor();
    }
    if (RigidBody2D *rigid = Object::cast_to<RigidBody2D>(body))
    {
        return rigid->get_contact_count() > 0;
    }
    if (RigidBody3D *rigid = Object::cast_to<RigidBody3D>(body))
    {
        return rigid->get_contact_count() > 0;
    }
    return false;
}

bool MovementDirector::is_in_water() const { return WaterArea2D::find_containing(get_body()) || WaterArea3D::find_containing(get_body()); }

bool MovementDirector::is_on_wall() const
{
    Node *body = get_body();
    if (CharacterBody2D *character = Object::cast_to<CharacterBody2D>(body))
    {
        return character->is_on_wall() && !character->is_on_floor();
    }
    if (CharacterBody3D *character = Object::cast_to<CharacterBody3D>(body))
    {
        return character->is_on_wall() && !character->is_on_floor();
    }
    return false;
}

bool MovementDirector::is_on_rope() const { return Rope2D::find_rope_of(get_body()) || Rope3D::find_rope_of(get_body()); }

bool MovementDirector::is_knocked() const { return Knockback2D::active_on(get_body()) || Knockback3D::active_on(get_body()); }

Dictionary MovementDirector::get_environment() const
{
    Dictionary environment;
    environment["on_floor"] = is_on_floor();
    environment["in_water"] = is_in_water();
    environment["on_wall"] = is_on_wall();
    environment["on_rope"] = is_on_rope();
    environment["knocked"] = is_knocked();
    environment["context"] = current;
    return environment;
}

bool MovementDirector::is_condition_met(const Ref<MovementContext> &p_context)
{
    if (p_context.is_null())
    {
        return false;
    }
    switch (p_context->get_condition())
    {
    case MovementContext::CONDITION_ALWAYS:
        return true;
    case MovementContext::CONDITION_ON_FLOOR:
        return is_on_floor();
    case MovementContext::CONDITION_IN_AIR:
        return !is_on_floor();
    case MovementContext::CONDITION_IN_WATER:
        return is_in_water();
    case MovementContext::CONDITION_ON_WALL:
        return is_on_wall();
    case MovementContext::CONDITION_ON_ROPE:
        return is_on_rope();
    case MovementContext::CONDITION_KNOCKED:
        return is_knocked();
    case MovementContext::CONDITION_MANUAL:
        return false;
    case MovementContext::CONDITION_CUSTOM:
        return p_context->evaluate_expression(get_body(), this);
    }
    return false;
}

// 🎬 Contextes

Ref<MovementContext> MovementDirector::get_context(const StringName &p_name) const
{
    for (int i = 0; i < contexts.size(); i++)
    {
        Ref<MovementContext> context = contexts[i];
        if (context.is_valid() && context->get_context_name() == p_name)
        {
            return context;
        }
    }
    return Ref<MovementContext>();
}

bool MovementDirector::set_context(const StringName &p_name)
{
    if (get_context(p_name).is_null())
    {
        return false;
    }
    forced = p_name;
    update_context();
    return true;
}

void MovementDirector::clear_forced_context()
{
    forced = StringName();
    update_context();
}

void MovementDirector::update_context()
{
    if (contexts.is_empty())
    {
        return;
    }
    Ref<MovementContext> chosen = String(forced).is_empty() ? Ref<MovementContext>() : get_context(forced);
    if (chosen.is_null() && auto_switch)
    {
        for (int i = 0; i < contexts.size(); i++)
        {
            Ref<MovementContext> context = contexts[i];
            if (context.is_valid() && (chosen.is_null() || context->get_priority() > chosen->get_priority()) && is_condition_met(context))
            {
                chosen = context;
            }
        }
    }
    if (chosen.is_null())
    {
        return;
    }
    StringName name = chosen->get_context_name();
    if (name == current)
    {
        return;
    }
    StringName previous = current;
    current = name;
    _apply(chosen);
    emit_signal("context_changed", previous, current);
}

void MovementDirector::_apply(const Ref<MovementContext> &p_context)
{
    // Composants nommés dans au moins un contexte : actifs seulement s'ils sont dans le contexte choisi.
    LocalVector<MovementComponent *> managed;
    for (int i = 0; i < contexts.size(); i++)
    {
        Ref<MovementContext> context = contexts[i];
        TypedArray<NodePath> paths = context.is_valid() ? context->get_components() : TypedArray<NodePath>();
        for (int p = 0; p < paths.size(); p++)
        {
            MovementComponent *component = Object::cast_to<MovementComponent>(get_node_or_null(paths[p]));
            if (component && managed.find(component) < 0)
            {
                managed.push_back(component);
            }
        }
    }
    LocalVector<MovementComponent *> enabled;
    TypedArray<NodePath> paths = p_context->get_components();
    for (int p = 0; p < paths.size(); p++)
    {
        if (MovementComponent *component = Object::cast_to<MovementComponent>(get_node_or_null(paths[p])))
        {
            enabled.push_back(component);
        }
    }
    for (MovementComponent *component : managed)
    {
        bool active = enabled.find(component) >= 0;
        component->set_active(active);
        if (active)
        {
            // L'intention courante suit (saut maintenu compris, pour la hauteur variable).
            component->move(move_input);
            component->set_running(running);
            component->set_vertical_input(vertical_input);
            if (!jump_held)
            {
                component->release_jump();
            }
        }
    }
}

void MovementDirector::create_default_contexts()
{
    contexts.clear();
    auto add = [this](const char32_t *p_name, MovementContext::Condition p_condition, int p_priority, Node *p_component, const String &p_expression = String()) {
        Ref<MovementContext> context;
        context.instantiate();
        context->set_context_name(StringName(String(p_name)));
        context->set_condition(p_condition);
        context->set_priority(p_priority);
        context->set_expression(p_expression);
        TypedArray<NodePath> components;
        components.push_back(get_path_to(p_component));
        context->set_components(components);
        contexts.push_back(context);
    };
    TypedArray<MovementComponent> components = get_components();
    bool is_3d = Object::cast_to<Node3D>(get_body()) != nullptr;
    for (int i = 0; i < components.size(); i++)
    {
        Node *component = Object::cast_to<Node>(components[i]);
        if (Object::cast_to<RopeMovement2D>(component) || Object::cast_to<RopeMovement3D>(component))
        {
            add(U"corde", MovementContext::CONDITION_ON_ROPE, 60, component);
        }
        else if (Object::cast_to<SwimMovement2D>(component) || Object::cast_to<SwimMovement3D>(component))
        {
            add(U"eau", MovementContext::CONDITION_IN_WATER, 50, component);
        }
        else if (Object::cast_to<WallMovement2D>(component) || Object::cast_to<WallMovement3D>(component))
        {
            add(U"mur", MovementContext::CONDITION_ON_WALL, 40, component);
        }
        else if (Object::cast_to<AerodynamicMovement2D>(component) || Object::cast_to<AerodynamicMovement3D>(component))
        {
            if (int(component->get("mode")) == 1)
            {
                add(U"vol", MovementContext::CONDITION_MANUAL, 70, component); // vol libre : set_context("vol")
            }
            else
            {
                // Plané : en l'air, en chute, saut maintenu.
                add(U"plané", MovementContext::CONDITION_CUSTOM, 20, component,
                    is_3d ? String("director.is_in_air() and director.is_jump_held() and body.velocity.y < 0") : String("director.is_in_air() and director.is_jump_held() and body.velocity.y > 0"));
            }
        }
        else if (Object::cast_to<GroundMovement2D>(component) || Object::cast_to<GroundMovement3D>(component))
        {
            add(U"sol", MovementContext::CONDITION_ALWAYS, 0, component);
        }
    }
    current = StringName();
    update_context();
}

void MovementDirector::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_READY)
    {
        if (contexts.is_empty() && auto_configure)
        {
            call_deferred("create_default_contexts"); // les composants frères sont alors tous prêts
        }
        else
        {
            call_deferred("update_context");
        }
        set_physics_process(true);
    }
    else if (p_what == NOTIFICATION_PHYSICS_PROCESS)
    {
        update_context();
        if (debug_mode)
        {
            _draw_debug();
        }
    }
    else if (p_what == NOTIFICATION_EXIT_TREE)
    {
        debug.release();
    }
}

void MovementDirector::_draw_debug()
{
    Node *body = get_body();
    bool body_3d = Object::cast_to<Node3D>(body) != nullptr;
    debug.begin(this, body_3d);
    PackedStringArray flags;
    if (is_on_floor())
    {
        flags.push_back(U"sol");
    }
    if (is_in_water())
    {
        flags.push_back(U"eau");
    }
    if (is_on_wall())
    {
        flags.push_back(U"mur");
    }
    if (is_on_rope())
    {
        flags.push_back(U"corde");
    }
    if (is_knocked())
    {
        flags.push_back(U"recul");
    }
    String text = String(U"🎬 ") + String(current) + (String(forced).is_empty() ? String() : String(U" (forcé)")) + U" · " + (flags.is_empty() ? String(U"en l'air") : String(", ").join(flags));
    if (Node2D *body_2d = Object::cast_to<Node2D>(body))
    {
        debug.text(body_2d->get_global_position() + Vector2(-60, -64), text, Color(1.0f, 0.85f, 0.4f), 12);
    }
    else if (Node3D *body_3d_node = Object::cast_to<Node3D>(body))
    {
        debug.text3(body_3d_node->get_global_position() + Vector3(0, 1.8f, 0), text, Color(1.0f, 0.85f, 0.4f), 36);
    }
    debug.end();
}

void MovementDirector::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_contexts", "contexts"), &MovementDirector::set_contexts);
    ClassDB::bind_method(D_METHOD("get_contexts"), &MovementDirector::get_contexts);
    ClassDB::bind_method(D_METHOD("set_auto_switch", "auto"), &MovementDirector::set_auto_switch);
    ClassDB::bind_method(D_METHOD("get_auto_switch"), &MovementDirector::get_auto_switch);
    ClassDB::bind_method(D_METHOD("set_auto_configure", "auto"), &MovementDirector::set_auto_configure);
    ClassDB::bind_method(D_METHOD("get_auto_configure"), &MovementDirector::get_auto_configure);
    ClassDB::bind_method(D_METHOD("move", "input"), &MovementDirector::move);
    ClassDB::bind_method(D_METHOD("jump"), &MovementDirector::jump);
    ClassDB::bind_method(D_METHOD("release_jump"), &MovementDirector::release_jump);
    ClassDB::bind_method(D_METHOD("set_running", "running"), &MovementDirector::set_running);
    ClassDB::bind_method(D_METHOD("set_vertical_input", "value"), &MovementDirector::set_vertical_input);
    ClassDB::bind_method(D_METHOD("get_move_input"), &MovementDirector::get_move_input);
    ClassDB::bind_method(D_METHOD("is_jump_held"), &MovementDirector::is_jump_held);
    ClassDB::bind_method(D_METHOD("set_context", "name"), &MovementDirector::set_context);
    ClassDB::bind_method(D_METHOD("clear_forced_context"), &MovementDirector::clear_forced_context);
    ClassDB::bind_method(D_METHOD("get_current_context"), &MovementDirector::get_current_context);
    ClassDB::bind_method(D_METHOD("get_forced_context"), &MovementDirector::get_forced_context);
    ClassDB::bind_method(D_METHOD("get_context", "name"), &MovementDirector::get_context);
    ClassDB::bind_method(D_METHOD("update_context"), &MovementDirector::update_context);
    ClassDB::bind_method(D_METHOD("create_default_contexts"), &MovementDirector::create_default_contexts);
    ClassDB::bind_method(D_METHOD("get_body"), &MovementDirector::get_body);
    ClassDB::bind_method(D_METHOD("get_components"), &MovementDirector::get_components);
    ClassDB::bind_method(D_METHOD("is_on_floor"), &MovementDirector::is_on_floor);
    ClassDB::bind_method(D_METHOD("is_in_air"), &MovementDirector::is_in_air);
    ClassDB::bind_method(D_METHOD("is_in_water"), &MovementDirector::is_in_water);
    ClassDB::bind_method(D_METHOD("is_on_wall"), &MovementDirector::is_on_wall);
    ClassDB::bind_method(D_METHOD("is_on_rope"), &MovementDirector::is_on_rope);
    ClassDB::bind_method(D_METHOD("is_knocked"), &MovementDirector::is_knocked);
    ClassDB::bind_method(D_METHOD("get_environment"), &MovementDirector::get_environment);
    ClassDB::bind_method(D_METHOD("is_condition_met", "context"), &MovementDirector::is_condition_met);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "contexts", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":MovementContext"), "set_contexts", "get_contexts");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_switch"), "set_auto_switch", "get_auto_switch");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_configure"), "set_auto_configure", "get_auto_configure");

    ADD_SIGNAL(MethodInfo("context_changed", PropertyInfo(Variant::STRING_NAME, "previous"), PropertyInfo(Variant::STRING_NAME, "current")));
    bind_debug_mode<MovementDirector>();
}
