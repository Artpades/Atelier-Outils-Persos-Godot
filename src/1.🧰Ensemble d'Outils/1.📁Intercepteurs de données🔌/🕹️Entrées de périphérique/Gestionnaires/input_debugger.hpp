#ifndef INPUT_DEBUGGER_HPP
#define INPUT_DEBUGGER_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
class PlayerInput;

/// 🐞 Relais de débogage des entrées, côté jeu (classe interne, créée au chargement du module).
///
/// Dans un jeu lancé depuis l'éditeur, les PlayerInput s'y inscrivent. Messages envoyés à l'éditeur :
///   - "input:players"  [ [chemin, nom], ... ]       toutes les secondes ;
///   - "input:snapshot" [ chemin, instantané ]       10 fois par seconde pour les chemins observés
///                                                   (PlayerInput.get_debug_snapshot).
/// Messages reçus de l'éditeur :
///   - "input:watch"       [ [chemins] ]
///   - "input:simulate"    [ chemin, action, valeur ]  (valeur nulle : fin de la simulation)
///   - "input:set_context" [ chemin, contexte, actif ]
class InputDebugger : public Object
{
    GDCLASS(InputDebugger, Object)

  public:
    static void create();
    static void destroy();
    static InputDebugger *get_singleton() { return singleton; }

    /// Chemin relatif à la scène courante (comme dans la scène éditée).
    static String debug_path_of(const Node *p_node);

    void register_player(PlayerInput *p_player);
    void unregister_player(PlayerInput *p_player);
    void notify(PlayerInput *p_player);

  protected:
    static void _bind_methods() {}

  private:
    static InputDebugger *singleton;

    LocalVector<ObjectID> players;
    HashSet<String> watched;
    bool editor_listening = false;
    uint64_t last_list_msec = 0;
    uint64_t last_snapshot_msec = 0;
    HashSet<String> sent_this_window;

    bool _on_message(const String &p_message, const Array &p_data);
    PlayerInput *_find(const String &p_path) const;
    void _send_player_list();
};
} // namespace godot

#endif // INPUT_DEBUGGER_HPP
