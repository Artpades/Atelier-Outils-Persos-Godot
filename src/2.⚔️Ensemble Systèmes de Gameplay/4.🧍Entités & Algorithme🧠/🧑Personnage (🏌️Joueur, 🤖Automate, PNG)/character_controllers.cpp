#include "character_controllers.hpp"
#include "character_registry.hpp"

#include "../💬Système de Dialogue/dialogue_system.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// =====================================================================================================
// 🏌️ PlayerController
// =====================================================================================================

PlayerController::PlayerController()
{
    // Avant le personnage (son parent) : l'intention est fixée avant son déplacement de la même image.
    set_physics_process_priority(-10);
}

Node *PlayerController::get_character() const { return get_parent(); }

bool PlayerController::_action_exists(const StringName &p_action) const { return !String(p_action).is_empty() && InputMap::get_singleton()->has_action(p_action); }

void PlayerController::create_default_actions()
{
    struct Binding
    {
        StringName action;
        Key keys[3];
        int joy_axis;   // -1 : aucun
        float axis_sign;
        int joy_button; // -1 : aucun
    };
    const Binding bindings[] = {
        {action_left, {KEY_A, KEY_Q, KEY_LEFT}, JOY_AXIS_LEFT_X, -1.0f, JOY_BUTTON_DPAD_LEFT},
        {action_right, {KEY_D, KEY_RIGHT, KEY_NONE}, JOY_AXIS_LEFT_X, 1.0f, JOY_BUTTON_DPAD_RIGHT},
        {action_up, {KEY_W, KEY_Z, KEY_UP}, JOY_AXIS_LEFT_Y, -1.0f, JOY_BUTTON_DPAD_UP},
        {action_down, {KEY_S, KEY_DOWN, KEY_NONE}, JOY_AXIS_LEFT_Y, 1.0f, JOY_BUTTON_DPAD_DOWN},
        {action_jump, {KEY_SPACE, KEY_NONE, KEY_NONE}, -1, 0.0f, JOY_BUTTON_A},
        {action_run, {KEY_SHIFT, KEY_NONE, KEY_NONE}, -1, 0.0f, JOY_BUTTON_LEFT_SHOULDER},
        {action_interact, {KEY_E, KEY_NONE, KEY_NONE}, -1, 0.0f, JOY_BUTTON_X},
    };
    InputMap *map = InputMap::get_singleton();
    for (const Binding &binding : bindings)
    {
        if (String(binding.action).is_empty() || map->has_action(binding.action))
        {
            continue;
        }
        map->add_action(binding.action, 0.2f);
        for (Key key : binding.keys)
        {
            if (key == KEY_NONE)
            {
                continue;
            }
            Ref<InputEventKey> event;
            event.instantiate();
            event->set_physical_keycode(key);
            map->action_add_event(binding.action, event);
        }
        if (binding.joy_axis >= 0)
        {
            Ref<InputEventJoypadMotion> motion;
            motion.instantiate();
            motion->set_axis(JoyAxis(binding.joy_axis));
            motion->set_axis_value(binding.axis_sign);
            map->action_add_event(binding.action, motion);
        }
        if (binding.joy_button >= 0)
        {
            Ref<InputEventJoypadButton> button;
            button.instantiate();
            button->set_button_index(JoyButton(binding.joy_button));
            map->action_add_event(binding.action, button);
        }
    }
}

Interactable *PlayerController::get_focused_interactable() const { return Object::cast_to<Interactable>(ObjectDB::get_instance(focused_id)); }

bool PlayerController::interact()
{
    Node *character = get_character();
    Interactable *target = character ? Interactable::find_nearest(character) : nullptr;
    return target && target->interact(character);
}

bool PlayerController::use_ability(const StringName &p_id)
{
    CharacterAPI *character = character_api_of(get_character());
    AbilitySystem *system = character ? character->get_ability_system() : nullptr;
    if (!system)
    {
        return false;
    }
    Node *target = auto_target_range > 0.0 ? CharacterRegister::get_nearest(get_character(), auto_target_range, true) : nullptr;
    return system->activate(p_id, target);
}

void PlayerController::_physics_update()
{
    CharacterAPI *character = character_api_of(get_character());
    if (!character)
    {
        return;
    }
    DialogueSystem *dialogue = DialogueSystem::get_main();
    bool talking = block_during_dialogue && dialogue && dialogue->is_running();
    if (!enabled || talking)
    {
        character->set_move_input(Vector2());
        character->release_jump();
        character->set_running(false);
        return;
    }
    Input *input = Input::get_singleton();

    // 🕹️ Déplacement
    Vector2 raw;
    if (_action_exists(action_left) && _action_exists(action_right) && _action_exists(action_up) && _action_exists(action_down))
    {
        raw = input->get_vector(action_left, action_right, action_up, action_down);
    }
    Vector2 move = raw;
    if (character->is_3d() && camera_relative)
    {
        Viewport *viewport = get_viewport();
        Camera3D *camera = viewport ? viewport->get_camera_3d() : nullptr;
        if (camera)
        {
            Basis basis = camera->get_global_transform().basis;
            Vector3 forward = -basis.get_column(2);
            Vector3 right = basis.get_column(0);
            forward.y = 0.0f;
            right.y = 0.0f;
            Vector3 direction = right.normalized() * raw.x + forward.normalized() * -raw.y;
            move = Vector2(direction.x, direction.z).limit_length(1.0f);
        }
    }
    character->set_move_input(move);

    // 🦘 Saut et course
    if (_action_exists(action_jump))
    {
        if (input->is_action_just_pressed(action_jump))
        {
            character->request_jump();
        }
        if (!input->is_action_pressed(action_jump))
        {
            character->release_jump();
        }
    }
    character->set_running(_action_exists(action_run) && input->is_action_pressed(action_run));

    // 🤝 Interaction
    Interactable *nearest = Interactable::find_nearest(get_character());
    ObjectID nearest_id = nearest ? ObjectID(nearest->get_instance_id()) : ObjectID();
    if (nearest_id != focused_id)
    {
        focused_id = nearest_id;
        emit_signal("focus_changed", nearest);
    }
    if (nearest && _action_exists(action_interact) && input->is_action_just_pressed(action_interact))
    {
        nearest->interact(get_character());
    }

    // 🌟 Capacités
    Array actions = ability_actions.keys();
    for (int i = 0; i < actions.size(); i++)
    {
        StringName action = actions[i];
        if (_action_exists(action) && input->is_action_just_pressed(action))
        {
            use_ability(ability_actions[actions[i]]);
        }
    }
}

void PlayerController::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    if (p_what == NOTIFICATION_READY)
    {
        if (create_missing_actions)
        {
            create_default_actions();
        }
        set_physics_process(true);
    }
    else if (p_what == NOTIFICATION_PHYSICS_PROCESS)
    {
        _physics_update();
    }
}

void PlayerController::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &PlayerController::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &PlayerController::is_enabled);
    ClassDB::bind_method(D_METHOD("set_action_left", "action"), &PlayerController::set_action_left);
    ClassDB::bind_method(D_METHOD("get_action_left"), &PlayerController::get_action_left);
    ClassDB::bind_method(D_METHOD("set_action_right", "action"), &PlayerController::set_action_right);
    ClassDB::bind_method(D_METHOD("get_action_right"), &PlayerController::get_action_right);
    ClassDB::bind_method(D_METHOD("set_action_up", "action"), &PlayerController::set_action_up);
    ClassDB::bind_method(D_METHOD("get_action_up"), &PlayerController::get_action_up);
    ClassDB::bind_method(D_METHOD("set_action_down", "action"), &PlayerController::set_action_down);
    ClassDB::bind_method(D_METHOD("get_action_down"), &PlayerController::get_action_down);
    ClassDB::bind_method(D_METHOD("set_action_jump", "action"), &PlayerController::set_action_jump);
    ClassDB::bind_method(D_METHOD("get_action_jump"), &PlayerController::get_action_jump);
    ClassDB::bind_method(D_METHOD("set_action_run", "action"), &PlayerController::set_action_run);
    ClassDB::bind_method(D_METHOD("get_action_run"), &PlayerController::get_action_run);
    ClassDB::bind_method(D_METHOD("set_action_interact", "action"), &PlayerController::set_action_interact);
    ClassDB::bind_method(D_METHOD("get_action_interact"), &PlayerController::get_action_interact);
    ClassDB::bind_method(D_METHOD("set_ability_actions", "actions"), &PlayerController::set_ability_actions);
    ClassDB::bind_method(D_METHOD("get_ability_actions"), &PlayerController::get_ability_actions);
    ClassDB::bind_method(D_METHOD("set_auto_target_range", "range"), &PlayerController::set_auto_target_range);
    ClassDB::bind_method(D_METHOD("get_auto_target_range"), &PlayerController::get_auto_target_range);
    ClassDB::bind_method(D_METHOD("set_camera_relative", "relative"), &PlayerController::set_camera_relative);
    ClassDB::bind_method(D_METHOD("get_camera_relative"), &PlayerController::get_camera_relative);
    ClassDB::bind_method(D_METHOD("set_block_during_dialogue", "block"), &PlayerController::set_block_during_dialogue);
    ClassDB::bind_method(D_METHOD("get_block_during_dialogue"), &PlayerController::get_block_during_dialogue);
    ClassDB::bind_method(D_METHOD("set_create_missing_actions", "create"), &PlayerController::set_create_missing_actions);
    ClassDB::bind_method(D_METHOD("get_create_missing_actions"), &PlayerController::get_create_missing_actions);
    ClassDB::bind_method(D_METHOD("get_character"), &PlayerController::get_character);
    ClassDB::bind_method(D_METHOD("get_focused_interactable"), &PlayerController::get_focused_interactable);
    ClassDB::bind_method(D_METHOD("interact"), &PlayerController::interact);
    ClassDB::bind_method(D_METHOD("use_ability", "id"), &PlayerController::use_ability);
    ClassDB::bind_method(D_METHOD("create_default_actions"), &PlayerController::create_default_actions);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
    ADD_GROUP(U"🕹️ Actions", "action_");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_left", PROPERTY_HINT_INPUT_NAME, "show_builtin"), "set_action_left", "get_action_left");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_right", PROPERTY_HINT_INPUT_NAME, "show_builtin"), "set_action_right", "get_action_right");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_up", PROPERTY_HINT_INPUT_NAME, "show_builtin"), "set_action_up", "get_action_up");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_down", PROPERTY_HINT_INPUT_NAME, "show_builtin"), "set_action_down", "get_action_down");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_jump", PROPERTY_HINT_INPUT_NAME, "show_builtin"), "set_action_jump", "get_action_jump");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_run", PROPERTY_HINT_INPUT_NAME, "show_builtin"), "set_action_run", "get_action_run");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_interact", PROPERTY_HINT_INPUT_NAME, "show_builtin"), "set_action_interact", "get_action_interact");
    ADD_GROUP(U"🌟 Capacités", "");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "ability_actions", PROPERTY_HINT_DICTIONARY_TYPE, "StringName;StringName"), "set_ability_actions", "get_ability_actions");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "auto_target_range", PROPERTY_HINT_RANGE, "0,2000,0.1,or_greater"), "set_auto_target_range", "get_auto_target_range");
    ADD_GROUP(U"⚙️ Options", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "camera_relative"), "set_camera_relative", "get_camera_relative");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "block_during_dialogue"), "set_block_during_dialogue", "get_block_during_dialogue");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "create_missing_actions"), "set_create_missing_actions", "get_create_missing_actions");

    ADD_SIGNAL(MethodInfo("focus_changed", PropertyInfo(Variant::OBJECT, "interactable", PROPERTY_HINT_NODE_TYPE, "Interactable")));
}

// =====================================================================================================
// 🤖 AutomateController
// =====================================================================================================

AutomateController::AutomateController() { set_physics_process_priority(-10); }

Node *AutomateController::get_character() const { return get_parent(); }

Node *AutomateController::get_follow_target() const { return Object::cast_to<Node>(ObjectDB::get_instance(follow_id)); }

double AutomateController::get_effective_arrive_distance() const
{
    if (arrive_distance >= 0.0)
    {
        return arrive_distance;
    }
    CharacterAPI *character = character_api_of(get_character());
    return character && character->is_3d() ? 0.3 : 8.0;
}

Vector3 AutomateController::_to_point(const Variant &p_point)
{
    switch (p_point.get_type())
    {
    case Variant::VECTOR2:
    {
        Vector2 point = p_point;
        return Vector3(point.x, point.y, 0.0f);
    }
    case Variant::VECTOR2I:
    {
        Vector2i point = p_point;
        return Vector3(point.x, point.y, 0.0f);
    }
    case Variant::VECTOR3:
        return p_point;
    case Variant::OBJECT:
        return node_world_position(Object::cast_to<Node>(p_point));
    default:
        return Vector3();
    }
}

void AutomateController::move_to(const Variant &p_point)
{
    destination = _to_point(p_point);
    mode = MODE_MOVE_TO;
    wait_remaining = 0.0;
}

void AutomateController::follow(Node *p_target, double p_distance)
{
    if (!p_target)
    {
        stop();
        return;
    }
    follow_id = ObjectID(p_target->get_instance_id());
    if (p_distance >= 0.0)
    {
        follow_distance = p_distance;
    }
    mode = MODE_FOLLOW;
    wait_remaining = 0.0;
}

void AutomateController::patrol(const Array &p_points)
{
    if (!p_points.is_empty())
    {
        patrol_points = p_points;
    }
    if (patrol_points.is_empty())
    {
        stop();
        return;
    }
    patrol_index = 0;
    patrol_direction = 1;
    destination = _to_point(patrol_points[0]);
    mode = MODE_PATROL;
    wait_remaining = 0.0;
}

void AutomateController::wander(double p_radius)
{
    if (p_radius >= 0.0)
    {
        wander_radius = p_radius;
    }
    mode = MODE_WANDER;
    wait_remaining = 0.0;
    _pick_wander_point();
}

void AutomateController::stop()
{
    mode = MODE_IDLE;
    follow_id = ObjectID();
    if (CharacterAPI *character = character_api_of(get_character()))
    {
        character->set_move_input(Vector2());
        character->set_running(false);
    }
}

bool AutomateController::is_moving() const
{
    CharacterAPI *character = character_api_of(get_character());
    return character && character->get_move_input() != Vector2();
}

void AutomateController::_pick_wander_point()
{
    CharacterAPI *character = character_api_of(get_character());
    double angle = UtilityFunctions::randf_range(0.0, Math_TAU);
    double distance = wander_radius * Math::sqrt(UtilityFunctions::randf());
    Vector3 offset = character && character->is_3d() ? Vector3(Math::cos(angle), 0.0, Math::sin(angle)) : Vector3(Math::cos(angle), Math::sin(angle), 0.0);
    destination = home + offset * distance;
}

Vector3 AutomateController::_next_path_point(CharacterAPI *p_character, const Vector3 &p_goal)
{
    if (!use_navigation)
    {
        return p_goal;
    }
    Node *body = p_character->as_node();
    Node *agent = nullptr;
    for (int i = 0; i < body->get_child_count() && !agent; i++)
    {
        Node *child = body->get_child(i);
        if (child->is_class("NavigationAgent2D") || child->is_class("NavigationAgent3D"))
        {
            agent = child;
        }
    }
    if (!agent)
    {
        return p_goal;
    }
    bool is_3d = agent->is_class("NavigationAgent3D");
    agent->set("target_position", is_3d ? Variant(p_goal) : Variant(Vector2(p_goal.x, p_goal.y)));
    if (bool(agent->call("is_navigation_finished")))
    {
        return p_goal;
    }
    Variant next = agent->call("get_next_path_position");
    Vector3 point = _to_point(next);
    // Carte de navigation pas encore prête : le chemin se réduit à la position actuelle.
    return p_character->planar_distance_to(point) < 0.001 ? p_goal : point;
}

void AutomateController::_physics_update(double p_delta)
{
    CharacterAPI *character = character_api_of(get_character());
    if (!character)
    {
        return;
    }
    if (!home_set)
    {
        home = character->get_world_position();
        home_set = true;
    }
    if (!enabled || _is_paused() || mode == MODE_IDLE)
    {
        character->set_move_input(Vector2());
        return;
    }
    if (wait_remaining > 0.0)
    {
        wait_remaining -= p_delta;
        character->set_move_input(Vector2());
        return;
    }

    Vector3 goal = destination;
    double stop_distance = get_effective_arrive_distance();
    if (mode == MODE_FOLLOW)
    {
        Node *target = get_follow_target();
        if (!target || !target->is_inside_tree())
        {
            stop();
            emit_signal("target_lost");
            return;
        }
        goal = node_world_position(target);
        destination = goal;
        stop_distance = MAX(stop_distance, follow_distance);
    }

    double distance = character->planar_distance_to(goal);
    if (distance <= stop_distance)
    {
        character->set_move_input(Vector2());
        character->set_running(false);
        switch (mode)
        {
        case MODE_MOVE_TO:
            mode = MODE_IDLE;
            emit_signal("destination_reached");
            break;
        case MODE_PATROL:
        {
            emit_signal("patrol_point_reached", patrol_index);
            int count = patrol_points.size();
            if (count > 1)
            {
                if (patrol_loop)
                {
                    patrol_index = (patrol_index + 1) % count;
                }
                else
                {
                    if (patrol_index + patrol_direction < 0 || patrol_index + patrol_direction >= count)
                    {
                        patrol_direction = -patrol_direction;
                    }
                    patrol_index += patrol_direction;
                }
            }
            destination = count > 0 ? _to_point(patrol_points[patrol_index]) : destination;
            wait_remaining = wait_time;
            break;
        }
        case MODE_WANDER:
            emit_signal("destination_reached");
            _pick_wander_point();
            wait_remaining = wait_time;
            break;
        default:
            break; // suivi : reste à distance
        }
        return;
    }

    Vector3 next = _next_path_point(character, goal);
    bool run = run_distance > 0.0 && distance > run_distance;
    character->set_running(run);
    // Freinage à l'approche (« arrive ») : vitesse voulue √(2·a·d) sur la distance restante d, avec a la plus
    // faible de l'accélération et de la décélération du déplacement actif (elles limitent sa réponse) ;
    // sinon le personnage dépasse le point d'arrivée. Sans ces réglages (autre déplacement) : pas de freinage.
    double scale = 1.0;
    Node *movement = character->get_active_movement();
    Variant max_speed = movement ? movement->get("max_speed") : Variant();
    if (max_speed.get_type() == Variant::FLOAT)
    {
        double speed = double(max_speed) * (run ? double(movement->get("run_multiplier")) : 1.0);
        double response = 0.9 * MIN(double(movement->get("acceleration")), double(movement->get("deceleration")));
        double wanted = Math::sqrt(2.0 * MAX(0.0, response) * MAX(0.0, distance - stop_distance));
        scale = speed > 0.0 ? CLAMP(wanted / speed, 0.05, 1.0) : 1.0;
    }
    character->set_move_input(character->direction_input_to(next) * float(scale));
}

void AutomateController::_notification(int p_what)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    switch (p_what)
    {
    case NOTIFICATION_READY:
        set_physics_process(true);
        switch (start_mode)
        {
        case MODE_PATROL:
            patrol();
            break;
        case MODE_WANDER:
            if (CharacterAPI *character = character_api_of(get_character()))
            {
                home = character->get_world_position();
                home_set = true;
            }
            wander();
            break;
        default:
            break;
        }
        break;
    case NOTIFICATION_PHYSICS_PROCESS:
        _physics_update(get_physics_process_delta_time());
        break;
    default:
        break;
    }
}

void AutomateController::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &AutomateController::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &AutomateController::is_enabled);
    ClassDB::bind_method(D_METHOD("set_start_mode", "mode"), &AutomateController::set_start_mode);
    ClassDB::bind_method(D_METHOD("get_start_mode"), &AutomateController::get_start_mode);
    ClassDB::bind_method(D_METHOD("set_arrive_distance", "distance"), &AutomateController::set_arrive_distance);
    ClassDB::bind_method(D_METHOD("get_arrive_distance"), &AutomateController::get_arrive_distance);
    ClassDB::bind_method(D_METHOD("get_effective_arrive_distance"), &AutomateController::get_effective_arrive_distance);
    ClassDB::bind_method(D_METHOD("set_patrol_points", "points"), &AutomateController::set_patrol_points);
    ClassDB::bind_method(D_METHOD("get_patrol_points"), &AutomateController::get_patrol_points);
    ClassDB::bind_method(D_METHOD("set_patrol_loop", "loop"), &AutomateController::set_patrol_loop);
    ClassDB::bind_method(D_METHOD("get_patrol_loop"), &AutomateController::get_patrol_loop);
    ClassDB::bind_method(D_METHOD("set_wait_time", "time"), &AutomateController::set_wait_time);
    ClassDB::bind_method(D_METHOD("get_wait_time"), &AutomateController::get_wait_time);
    ClassDB::bind_method(D_METHOD("set_wander_radius", "radius"), &AutomateController::set_wander_radius);
    ClassDB::bind_method(D_METHOD("get_wander_radius"), &AutomateController::get_wander_radius);
    ClassDB::bind_method(D_METHOD("set_follow_distance", "distance"), &AutomateController::set_follow_distance);
    ClassDB::bind_method(D_METHOD("get_follow_distance"), &AutomateController::get_follow_distance);
    ClassDB::bind_method(D_METHOD("set_run_distance", "distance"), &AutomateController::set_run_distance);
    ClassDB::bind_method(D_METHOD("get_run_distance"), &AutomateController::get_run_distance);
    ClassDB::bind_method(D_METHOD("set_use_navigation", "use"), &AutomateController::set_use_navigation);
    ClassDB::bind_method(D_METHOD("get_use_navigation"), &AutomateController::get_use_navigation);
    ClassDB::bind_method(D_METHOD("move_to", "point"), &AutomateController::move_to);
    ClassDB::bind_method(D_METHOD("follow", "target", "distance"), &AutomateController::follow, DEFVAL(-1.0));
    ClassDB::bind_method(D_METHOD("patrol", "points"), &AutomateController::patrol, DEFVAL(Array()));
    ClassDB::bind_method(D_METHOD("wander", "radius"), &AutomateController::wander, DEFVAL(-1.0));
    ClassDB::bind_method(D_METHOD("stop"), &AutomateController::stop);
    ClassDB::bind_method(D_METHOD("get_mode"), &AutomateController::get_mode);
    ClassDB::bind_method(D_METHOD("is_moving"), &AutomateController::is_moving);
    ClassDB::bind_method(D_METHOD("get_destination"), &AutomateController::get_destination);
    ClassDB::bind_method(D_METHOD("get_follow_target"), &AutomateController::get_follow_target);
    ClassDB::bind_method(D_METHOD("get_character"), &AutomateController::get_character);
    ClassDB::bind_method(D_METHOD("set_home", "home"), &AutomateController::set_home);
    ClassDB::bind_method(D_METHOD("get_home"), &AutomateController::get_home);

    BIND_ENUM_CONSTANT(MODE_IDLE);
    BIND_ENUM_CONSTANT(MODE_MOVE_TO);
    BIND_ENUM_CONSTANT(MODE_FOLLOW);
    BIND_ENUM_CONSTANT(MODE_PATROL);
    BIND_ENUM_CONSTANT(MODE_WANDER);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "start_mode", PROPERTY_HINT_ENUM, "Idle,Move To,Follow,Patrol,Wander"), "set_start_mode", "get_start_mode");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "arrive_distance", PROPERTY_HINT_RANGE, "-1,100,0.01,or_greater"), "set_arrive_distance", "get_arrive_distance");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_navigation"), "set_use_navigation", "get_use_navigation");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "run_distance", PROPERTY_HINT_RANGE, "0,2000,0.1,or_greater"), "set_run_distance", "get_run_distance");
    ADD_GROUP(U"🚩 Patrouille", "");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "patrol_points"), "set_patrol_points", "get_patrol_points");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "patrol_loop"), "set_patrol_loop", "get_patrol_loop");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wait_time", PROPERTY_HINT_RANGE, "0,30,0.01,or_greater,suffix:s"), "set_wait_time", "get_wait_time");
    ADD_GROUP(U"🌀 Errance et suivi", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wander_radius", PROPERTY_HINT_RANGE, "0,2000,0.1,or_greater"), "set_wander_radius", "get_wander_radius");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "follow_distance", PROPERTY_HINT_RANGE, "0,2000,0.1,or_greater"), "set_follow_distance", "get_follow_distance");

    ADD_SIGNAL(MethodInfo("destination_reached"));
    ADD_SIGNAL(MethodInfo("patrol_point_reached", PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(MethodInfo("target_lost"));
}

// =====================================================================================================
// 🧑‍🌾 NPCController
// =====================================================================================================

Interactable *NPCController::get_interactable() const
{
    Node *character = get_character();
    for (int i = 0; character && i < character->get_child_count(); i++)
    {
        if (Interactable *interactable = Object::cast_to<Interactable>(character->get_child(i)))
        {
            return interactable;
        }
    }
    return nullptr;
}

bool NPCController::talk(Node *p_interlocutor)
{
    DialogueSystem *system = dialogue_system_path.is_empty() ? DialogueSystem::get_main() : Object::cast_to<DialogueSystem>(get_node_or_null(dialogue_system_path));
    if (!system || dialogue.is_null() || talking)
    {
        return false;
    }
    if (!system->start(dialogue, dialogue_start_id, get_character()))
    {
        return false;
    }
    talking = system->is_running();
    if (!talking)
    {
        return true; // dialogue sans réplique ni choix : déjà terminé
    }
    system_id = ObjectID(system->get_instance_id());
    Callable ended = callable_mp(this, &NPCController::_on_dialogue_ended);
    if (!system->is_connected("dialogue_ended", ended))
    {
        system->connect("dialogue_ended", ended);
    }
    CharacterAPI *character = character_api_of(get_character());
    if (character && face_interlocutor && p_interlocutor)
    {
        character->face_toward(node_world_position(p_interlocutor));
    }
    if (character && stop_while_talking)
    {
        character->set_move_input(Vector2());
    }
    emit_signal("talk_started", p_interlocutor);
    return true;
}

void NPCController::_on_interacted(Node *p_by) { talk(p_by); }

void NPCController::_on_dialogue_ended(const Ref<DialogueResource> & /*p_dialogue*/)
{
    if (DialogueSystem *system = Object::cast_to<DialogueSystem>(ObjectDB::get_instance(system_id)))
    {
        Callable ended = callable_mp(this, &NPCController::_on_dialogue_ended);
        if (system->is_connected("dialogue_ended", ended))
        {
            system->disconnect("dialogue_ended", ended);
        }
    }
    talking = false;
    emit_signal("talk_ended");
}

void NPCController::_notification(int p_what)
{
    if (p_what != NOTIFICATION_READY || Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    Interactable *interactable = get_interactable();
    if (!interactable && get_character())
    {
        CharacterAPI *character = character_api_of(get_character());
        interactable = memnew(Interactable);
        interactable->set_name("Interactable");
        interactable->set_prompt(interaction_prompt);
        interactable->set_interaction_range(character && character->is_3d() ? 2.0 : 64.0);
        get_character()->call_deferred("add_child", interactable);
    }
    if (interactable)
    {
        interactable->connect("interacted", callable_mp(this, &NPCController::_on_interacted));
    }
}

void NPCController::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_dialogue", "dialogue"), &NPCController::set_dialogue);
    ClassDB::bind_method(D_METHOD("get_dialogue"), &NPCController::get_dialogue);
    ClassDB::bind_method(D_METHOD("set_dialogue_start_id", "id"), &NPCController::set_dialogue_start_id);
    ClassDB::bind_method(D_METHOD("get_dialogue_start_id"), &NPCController::get_dialogue_start_id);
    ClassDB::bind_method(D_METHOD("set_dialogue_system_path", "path"), &NPCController::set_dialogue_system_path);
    ClassDB::bind_method(D_METHOD("get_dialogue_system_path"), &NPCController::get_dialogue_system_path);
    ClassDB::bind_method(D_METHOD("set_interaction_prompt", "prompt"), &NPCController::set_interaction_prompt);
    ClassDB::bind_method(D_METHOD("get_interaction_prompt"), &NPCController::get_interaction_prompt);
    ClassDB::bind_method(D_METHOD("set_face_interlocutor", "face"), &NPCController::set_face_interlocutor);
    ClassDB::bind_method(D_METHOD("get_face_interlocutor"), &NPCController::get_face_interlocutor);
    ClassDB::bind_method(D_METHOD("set_stop_while_talking", "stop"), &NPCController::set_stop_while_talking);
    ClassDB::bind_method(D_METHOD("get_stop_while_talking"), &NPCController::get_stop_while_talking);
    ClassDB::bind_method(D_METHOD("talk", "interlocutor"), &NPCController::talk);
    ClassDB::bind_method(D_METHOD("is_talking"), &NPCController::is_talking);
    ClassDB::bind_method(D_METHOD("get_interactable"), &NPCController::get_interactable);

    ADD_GROUP(U"💬 Dialogue", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "dialogue", PROPERTY_HINT_RESOURCE_TYPE, "DialogueResource"), "set_dialogue", "get_dialogue");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "dialogue_start_id"), "set_dialogue_start_id", "get_dialogue_start_id");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "dialogue_system_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "DialogueSystem"), "set_dialogue_system_path", "get_dialogue_system_path");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "interaction_prompt"), "set_interaction_prompt", "get_interaction_prompt");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "face_interlocutor"), "set_face_interlocutor", "get_face_interlocutor");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "stop_while_talking"), "set_stop_while_talking", "get_stop_while_talking");

    ADD_SIGNAL(MethodInfo("talk_started", PropertyInfo(Variant::OBJECT, "interlocutor", PROPERTY_HINT_NODE_TYPE, "Node")));
    ADD_SIGNAL(MethodInfo("talk_ended"));
}
