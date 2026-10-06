#ifndef INPUT_CONTEXT_HPP
#define INPUT_CONTEXT_HPP

#include "../Actions/input_action.hpp"

namespace godot
{
/// 🗂️ Contexte d'entrée (ressource) : un groupe d'actions actives ensemble (« Exploration », « Menu »,
/// « Véhicule »...). Le PlayerInput met à jour les actions des contextes actifs, du plus prioritaire au moins
/// prioritaire ; un contexte exclusive bloque ceux de priorité inférieure tant qu'il est actif (un menu par
/// dessus le jeu). Les actions d'un contexte inactif sont relâchées (released, valeur nulle).
class InputContext : public Resource
{
    GDCLASS(InputContext, Resource)

  public:
    void set_context_name(const StringName &p_name);
    StringName get_context_name() const { return context_name; }
    void set_description(const String &p_description) { description = p_description; }
    String get_description() const { return description; }
    void set_enabled(bool p_enabled);
    bool is_enabled() const { return enabled; }
    void set_priority(int p_priority);
    int get_priority() const { return priority; }
    void set_exclusive(bool p_exclusive);
    bool is_exclusive() const { return exclusive; }
    void set_actions(const TypedArray<InputAction> &p_actions);
    TypedArray<InputAction> get_actions() const { return actions; }

    Ref<InputAction> get_action(const StringName &p_name) const;
    bool has_action(const StringName &p_name) const { return get_action(p_name).is_valid(); }
    PackedStringArray get_action_names() const;
    void add_action(const Ref<InputAction> &p_action);
    void remove_action(const Ref<InputAction> &p_action);
    /// Copie indépendante (actions et liaisons comprises) pour un PlayerInput en jeu.
    Ref<InputContext> make_runtime_copy() const;

  protected:
    static void _bind_methods();

  private:
    StringName context_name = "contexte";
    String description;
    bool enabled = true;
    int priority = 0;
    bool exclusive = false;
    TypedArray<InputAction> actions;
};
} // namespace godot

#endif // INPUT_CONTEXT_HPP
