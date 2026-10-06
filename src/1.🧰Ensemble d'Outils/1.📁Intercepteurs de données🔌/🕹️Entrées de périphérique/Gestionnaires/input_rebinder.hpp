#ifndef INPUT_REBINDER_HPP
#define INPUT_REBINDER_HPP

#include "../Actions/input_action.hpp"

#include <godot_cpp/classes/node.hpp>

namespace godot
{
/// 🔄 Réassignation d'une entrée : start_rebind(action, emplacement) écoute la prochaine touche / bouton /
/// axe poussé (au-delà de joy_threshold) et l'assigne à l'emplacement (voir InputAction.get_rebind_slots :
/// "key", "joy_button", "up_key", "joy_axis_x"...). cancel_key (Échap) annule ; listening_timeout (0 = jamais)
/// annule aussi. Pendant l'écoute, les évènements de touches et boutons sont consommés.
/// find_conflicts liste les autres emplacements d'un PlayerInput qui utilisent déjà la même entrée.
class InputRebinder : public Node
{
    GDCLASS(InputRebinder, Node)

  public:
    void set_joy_threshold(double p_threshold) { joy_threshold = CLAMP(p_threshold, 0.05, 1.0); }
    double get_joy_threshold() const { return joy_threshold; }
    void set_cancel_key(int p_key) { cancel_key = p_key; }
    int get_cancel_key() const { return cancel_key; }
    void set_listening_timeout(double p_timeout) { listening_timeout = MAX(0.0, p_timeout); }
    double get_listening_timeout() const { return listening_timeout; }

    void start_rebind(const Ref<InputAction> &p_action, const StringName &p_slot);
    void cancel_rebind();
    bool is_listening() const { return listening; }
    Ref<InputAction> get_current_action() const { return current_action; }
    StringName get_current_slot() const { return current_slot; }
    double get_remaining_time() const { return remaining; }
    /// Évènement proposé (pour les tests ou une interface personnalisée) ; vrai s'il a été assigné.
    bool submit_event(const Ref<InputEvent> &p_event);
    /// Autres emplacements de p_player qui affichent la même entrée que p_slot : [ [action, emplacement], ... ].
    Array find_conflicts(Node *p_player, const Ref<InputAction> &p_action, const StringName &p_slot) const;

    void _input(const Ref<InputEvent> &p_event) override;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    double joy_threshold = 0.5;
    int cancel_key = 4194305; // KEY_ESCAPE
    double listening_timeout = 5.0;

    bool listening = false;
    Ref<InputAction> current_action;
    StringName current_slot;
    double remaining = 0.0;

    void _stop();
};
} // namespace godot

#endif // INPUT_REBINDER_HPP
