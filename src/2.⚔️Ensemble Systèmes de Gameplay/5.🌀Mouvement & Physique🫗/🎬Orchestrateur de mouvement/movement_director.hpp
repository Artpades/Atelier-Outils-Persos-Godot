#ifndef MOVEMENT_DIRECTOR_HPP
#define MOVEMENT_DIRECTOR_HPP

#include "../🧩Commun/movement_component.hpp"

#include <godot_cpp/classes/expression.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// 🎬 Contexte de mouvement : un nom, les composants actifs dans ce contexte (chemins relatifs au
/// MovementDirector), une condition d'activation automatique et une priorité (la plus haute l'emporte).
/// CONDITION_CUSTOM : expression avec body et director (ex. « body.velocity.y > 0 and director.is_in_air() »).
class MovementContext : public Resource
{
    GDCLASS(MovementContext, Resource)

  public:
    enum Condition
    {
        CONDITION_ALWAYS,
        CONDITION_ON_FLOOR,
        CONDITION_IN_AIR,
        CONDITION_IN_WATER,
        CONDITION_ON_WALL,
        CONDITION_ON_ROPE,
        CONDITION_KNOCKED,
        CONDITION_MANUAL, // jamais automatique : seulement par set_context()
        CONDITION_CUSTOM,
    };

    void set_context_name(const StringName &p_name) { context_name = p_name; emit_changed(); }
    StringName get_context_name() const { return context_name; }
    void set_components(const TypedArray<NodePath> &p_components) { components = p_components; emit_changed(); }
    TypedArray<NodePath> get_components() const { return components; }
    void set_condition(Condition p_condition) { condition = p_condition; emit_changed(); }
    Condition get_condition() const { return condition; }
    void set_expression(const String &p_expression);
    String get_expression() const { return expression; }
    void set_priority(int p_priority) { priority = p_priority; emit_changed(); }
    int get_priority() const { return priority; }

    /// Évalue l'expression (CONDITION_CUSTOM).
    bool evaluate_expression(Node *p_body, Node *p_director);

  protected:
    static void _bind_methods();

  private:
    StringName context_name;
    TypedArray<NodePath> components;
    Condition condition = CONDITION_ALWAYS;
    String expression;
    int priority = 0;
    Ref<Expression> parsed;
    bool parse_failed = false;
};

/// 🎬 Orchestrateur des systèmes de mouvement d'un corps (enfant du CharacterBody2D/3D).
///
/// - Transmet l'intention (move, jump, release_jump, set_running, set_vertical_input) à TOUS les
///   MovementComponent du corps : PlayerInput, une IA ou un script ne parlent qu'à lui.
/// - À chaque image physique (auto_switch), choisit le contexte de plus haute priorité dont la condition est
///   remplie (sur une corde, dans l'eau, contre un mur, en l'air, au sol, en recul...) et n'active que ses
///   composants ; set_context(nom) force un contexte, clear_forced_context() rend la main à l'automatique.
/// - Sans contexte configuré, create_default_contexts() en crée selon les composants présents.
class MovementDirector : public Node
{
    GDCLASS(MovementDirector, Node)

  public:
    MovementDirector();

    void set_contexts(const TypedArray<MovementContext> &p_contexts) { contexts = p_contexts; }
    TypedArray<MovementContext> get_contexts() const { return contexts; }
    void set_auto_switch(bool p_auto) { auto_switch = p_auto; }
    bool get_auto_switch() const { return auto_switch; }
    void set_auto_configure(bool p_auto) { auto_configure = p_auto; }
    bool get_auto_configure() const { return auto_configure; }

    // 🎮 Intention transmise aux composants
    void move(const Vector2 &p_input);
    void jump();
    void release_jump();
    void set_running(bool p_running);
    void set_vertical_input(float p_value);
    Vector2 get_move_input() const { return move_input; }
    bool is_jump_held() const { return jump_held; }

    // 🎬 Contextes
    bool set_context(const StringName &p_name);
    void clear_forced_context();
    StringName get_current_context() const { return current; }
    StringName get_forced_context() const { return forced; }
    Ref<MovementContext> get_context(const StringName &p_name) const;
    /// Recalcule et applique le contexte tout de suite (appelé à chaque image physique).
    void update_context();
    /// Crée des contextes à partir des composants du corps : corde, eau, mur, recul, air (plané), sol.
    void create_default_contexts();

    // 🌍 Environnement détecté
    Node *get_body() const { return get_parent(); }
    TypedArray<MovementComponent> get_components() const;
    bool is_on_floor() const;
    bool is_in_air() const { return !is_on_floor(); }
    bool is_in_water() const;
    bool is_on_wall() const;
    bool is_on_rope() const;
    bool is_knocked() const;
    Dictionary get_environment() const;
    bool is_condition_met(const Ref<MovementContext> &p_context);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    TypedArray<MovementContext> contexts;
    bool auto_switch = true;
    bool auto_configure = true;
    StringName current;
    StringName forced;
    Vector2 move_input;
    bool jump_held = false;
    bool running = false;
    float vertical_input = 0.0f;

    void _apply(const Ref<MovementContext> &p_context);

    DEBUG_MODE_DECLARATIONS()
    void _draw_debug();
};
} // namespace godot

VARIANT_ENUM_CAST(MovementContext::Condition);

#endif // MOVEMENT_DIRECTOR_HPP
