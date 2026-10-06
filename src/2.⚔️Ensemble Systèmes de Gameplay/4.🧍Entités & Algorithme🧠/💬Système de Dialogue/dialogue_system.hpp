#ifndef DIALOGUE_SYSTEM_HPP
#define DIALOGUE_SYSTEM_HPP

#include "dialogue_resource.hpp"

#include "../🤾Système de Comportement/🧩Commun/behavior_expression.hpp"
#include "../🤾Système de Comportement/🧩Commun/blackboard.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
/// 💬 Exécuteur de dialogues : parcourt un DialogueResource et émet ce qu'il faut afficher.
///
///   start(dialogue) → line_shown(...) → advance() → ... → choices_shown(...) → choose(i) → ... → dialogue_ended
/// Les embranchements et actions sont traités aussitôt (sans attendre le joueur). Les variables (conditions,
/// « {nom} » dans les textes, affectations) sont dans le tableau noir variables, conservé entre les dialogues ;
/// l'interlocuteur passé à start() est l'« agent » des expressions.
/// Une DialogueBox (ou votre propre interface) écoute les signaux et appelle advance() / choose().
class DialogueSystem : public Node
{
    GDCLASS(DialogueSystem, Node)

  public:
    enum WaitState
    {
        WAIT_NONE,
        WAIT_LINE,   // une réplique est affichée : advance()
        WAIT_CHOICE, // des options sont proposées : choose(i)
    };

    void set_variables(const Ref<Blackboard> &p_variables) { variables = p_variables; }
    Ref<Blackboard> get_variables();
    void set_default_dialogue(const Ref<DialogueResource> &p_dialogue) { default_dialogue = p_dialogue; }
    Ref<DialogueResource> get_default_dialogue() const { return default_dialogue; }
    void set_history_size(int p_size) { history_size = MAX(0, p_size); }
    int get_history_size() const { return history_size; }

    bool start(const Ref<DialogueResource> &p_dialogue = Ref<DialogueResource>(), const StringName &p_start_id = StringName(), Node *p_interlocutor = nullptr);
    void advance();
    void choose(int p_index);
    void stop();

    bool is_running() const { return running; }
    WaitState get_wait_state() const { return wait_state; }
    Ref<DialogueResource> get_dialogue() const { return dialogue; }
    Ref<DialogueNode> get_current_node() const;
    StringName get_current_id() const { return current_id; }
    Node *get_interlocutor() const;
    /// Options proposées : [{ "index", "text", "option" }].
    Array get_available_options() const { return available_options; }
    /// Remplace « {nom} » par la valeur de la variable nom.
    String interpolate(const String &p_text);
    /// Exécute une affectation « nom = expr », « nom += expr » ou « nom -= expr ».
    bool execute_assignment(const String &p_assignment);
    /// Évalue une condition (vide = vraie).
    bool evaluate_condition(const String &p_condition);
    /// Dernières répliques et choix : [« Orateur : texte », « → option »...]
    Array get_history() const { return history; }
    void clear_seen_options() { seen_options.clear(); }

    /// Premier DialogueSystem dans l'arbre de scène (pour les PNJ qui lancent un dialogue).
    static DialogueSystem *get_main();

    Dictionary get_debug_snapshot() const;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    Ref<Blackboard> variables;
    Ref<DialogueResource> default_dialogue;
    int history_size = 50;

    Ref<DialogueResource> dialogue;
    bool running = false;
    WaitState wait_state = WAIT_NONE;
    StringName current_id;
    ObjectID interlocutor_id;
    Array available_options;
    Dictionary visited;
    Dictionary seen_options; // « chemin|id|n° » des options once_only déjà prises
    Array history;
    double auto_advance_remaining = 0.0;
    BehaviorExpression evaluator;

    static LocalVector<ObjectID> instances;

    void _goto(const StringName &p_id);
    void _log(const String &p_entry);
    String _option_key(int p_index) const;
};
} // namespace godot

VARIANT_ENUM_CAST(DialogueSystem::WaitState);

#endif // DIALOGUE_SYSTEM_HPP
