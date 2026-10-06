#ifndef BEHAVIOR_DEBUGGER_HPP
#define BEHAVIOR_DEBUGGER_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
/// 🐞 Relais de débogage côté jeu (classe interne, une instance créée au chargement du module).
///
/// Quand le jeu est lancé depuis l'éditeur (EngineDebugger actif), les StateMachine et BehaviorTree
/// s'y inscrivent et l'appellent à chaque mise à jour (notify). Le relais envoie à l'éditeur :
///   - "behavior:runners"  [ [chemin, type, nom], ... ]  toutes les secondes (liste des comportements vivants) ;
///   - "behavior:snapshot" [ chemin, instantané ]          10 fois par seconde, pour les chemins observés.
/// L'éditeur choisit les chemins observés avec le message "behavior:watch" [ [chemins] ].
/// Le chemin d'un comportement est relatif à la scène courante (comme dans la scène éditée).
class BehaviorDebugger : public Object
{
    GDCLASS(BehaviorDebugger, Object)

  public:
    static void create();
    static void destroy();
    static BehaviorDebugger *get_singleton() { return singleton; }

    /// Chemin d'identification d'un comportement : relatif à la scène courante si elle le contient.
    static String debug_path_of(const Node *p_node);

    void register_runner(Node *p_runner);
    void unregister_runner(Node *p_runner);
    /// Appelé par un comportement après sa mise à jour : envoie son instantané s'il est observé.
    void notify(Node *p_runner);

  protected:
    static void _bind_methods() {}

  private:
    static BehaviorDebugger *singleton;

    LocalVector<ObjectID> runners;
    HashSet<String> watched;
    bool editor_listening = false; // vrai dès que l'éditeur a envoyé un message
    uint64_t last_runner_list_msec = 0;
    uint64_t last_snapshot_msec = 0;
    HashSet<String> sent_this_window; // chemins déjà envoyés dans la fenêtre des 100 ms en cours

    bool _on_message(const String &p_message, const Array &p_data);
    void _send_runner_list();
};
} // namespace godot

#endif // BEHAVIOR_DEBUGGER_HPP
