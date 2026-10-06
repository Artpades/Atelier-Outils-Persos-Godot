#ifndef PLAYER_INPUT_HPP
#define PLAYER_INPUT_HPP

#include "input_context.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/hash_set.hpp>

namespace godot
{
/// 🎮 Orchestrateur des entrées d'un joueur : met à jour, à chaque image, les actions (InputAction) de ses
/// contextes (InputContext) actifs, lues sur le périphérique `device`, et exécute leurs liaisons
/// (InputBinding : « quand l'action émet tel évènement, appeler telle méthode de tel nœud »).
///
/// - Contextes : activés / désactivés (set_context_enabled, enable_only), empilés (push_context : le
///   contexte passe au premier plan et bloque les autres jusqu'à pop_context), ou exclusifs par priorité.
/// - Périphériques : device_filter (tous, clavier-souris, manette) ; auto_switch_device bascule sur le dernier
///   type de périphérique utilisé (signal device_type_changed).
/// - Ressources : en jeu, chaque PlayerInput travaille sur des copies de ses contextes (runtime_copies), pour
///   que deux joueurs partageant les mêmes ressources ne se mélangent pas. get_context / get_action
///   renvoient ces copies.
/// - Contrôles : get_bindings_state / apply_bindings_state, save_bindings / load_bindings (ConfigFile),
///   reset_bindings (contrôles d'origine) ; voir InputRebinder et PlayerInputUI pour la réassignation.
/// - Vibration : start_vibration / stop_vibration.
/// - Débogage : get_debug_snapshot (et la fenêtre « Éditeur d'entrées », bouton de l'inspecteur).
class PlayerInput : public Node
{
    GDCLASS(PlayerInput, Node)

  public:
    PlayerInput();
    ~PlayerInput();

    void set_contexts(const TypedArray<InputContext> &p_contexts);
    TypedArray<InputContext> get_contexts() const { return contexts; }
    void set_device(int p_device) { device = MAX(0, p_device); }
    int get_device() const { return device; }
    void set_device_filter(InputAction::DeviceFilter p_filter);
    InputAction::DeviceFilter get_device_filter() const { return device_filter; }
    void set_auto_switch_device(bool p_auto) { auto_switch_device = p_auto; }
    bool get_auto_switch_device() const { return auto_switch_device; }
    void set_joy_threshold(double p_threshold) { joy_threshold = CLAMP(p_threshold, 0.0, 1.0); }
    double get_joy_threshold() const { return joy_threshold; }
    void set_process_in_physics(bool p_physics);
    bool get_process_in_physics() const { return process_in_physics; }
    void set_dispatch_bindings(bool p_dispatch) { dispatch_bindings = p_dispatch; }
    bool get_dispatch_bindings() const { return dispatch_bindings; }
    void set_runtime_copies(bool p_copies) { runtime_copies = p_copies; }
    bool get_runtime_copies() const { return runtime_copies; }

    // ⚙️ Mise à jour
    /// Prépare les contextes de jeu (copies) ; fait automatiquement à _ready.
    void setup();
    bool is_setup() const { return set_up; }
    /// Met à jour toutes les actions (appelé à chaque image ; appelable à la main hors de l'arbre).
    void poll(double p_delta);
    /// Évènement brut (souris, détection du type de périphérique) ; appelé par _input.
    void handle_input_event(const Ref<InputEvent> &p_event);
    void _input(const Ref<InputEvent> &p_event) override;
    /// Appelé par une action qui émet un évènement : signal action_triggered, puis liaisons.
    void dispatch_event(InputAction *p_action, const StringName &p_event, const Array &p_arguments);

    // 🗂️ Contextes
    TypedArray<InputContext> get_runtime_contexts() const { return set_up ? runtime_contexts : contexts; }
    Ref<InputContext> get_context(const StringName &p_name) const;
    void set_context_enabled(const StringName &p_name, bool p_enabled);
    bool is_context_enabled(const StringName &p_name) const;
    /// Activé et non bloqué par un contexte exclusif / empilé plus prioritaire (dernière mise à jour).
    bool is_context_active(const StringName &p_name) const;
    /// Active ce contexte et désactive tous les autres.
    void enable_only(const StringName &p_name);
    PackedStringArray get_active_contexts() const;
    void push_context(const StringName &p_name);
    StringName pop_context();
    PackedStringArray get_context_stack() const;

    // 🕹️ Actions (contexte actif le plus prioritaire d'abord)
    Ref<InputAction> get_action(const StringName &p_name) const;
    Variant get_value(const StringName &p_action) const;
    bool is_pressed(const StringName &p_action) const;
    bool is_just_pressed(const StringName &p_action) const;
    bool is_just_released(const StringName &p_action) const;
    double get_axis(const StringName &p_action) const;
    Vector2 get_vector(const StringName &p_action) const;
    Vector3 get_vector3(const StringName &p_action) const;
    void simulate(const StringName &p_action, const Variant &p_value);
    void stop_simulation(const StringName &p_action);
    void stop_all_simulations();

    // 🎮 Périphérique
    /// Dernier type de périphérique utilisé (DEVICE_KEYBOARD_MOUSE ou DEVICE_GAMEPAD).
    InputAction::DeviceFilter get_last_device_type() const { return last_device_type; }
    String get_joy_name() const;
    String get_joy_guid() const;
    Dictionary get_joy_info() const;
    bool is_device_connected() const;
    void start_vibration(double p_weak = 1.0, double p_strong = 1.0, double p_duration = 0.0);
    void stop_vibration();

    // 💾 Contrôles
    Dictionary get_bindings_state() const;
    void apply_bindings_state(const Dictionary &p_state);
    void reset_bindings();
    Error save_bindings(const String &p_path) const;
    Error load_bindings(const String &p_path);
    /// Toutes les actions de jeu : [ [contexte, action], ... ].
    Array get_all_actions() const;

    // 🐞 Débogage
    Dictionary get_debug_snapshot() const;
    Array get_event_log() const { return event_log; }
    void clear_event_log() { event_log.clear(); }

    /// Problèmes de configuration (contextes ou actions en double, cibles de liaisons introuvables...).
    PackedStringArray get_configuration_issues() const;
    PackedStringArray _get_configuration_warnings() const override { return get_configuration_issues(); }

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    TypedArray<InputContext> contexts;
    int device = 0;
    InputAction::DeviceFilter device_filter = InputAction::DEVICE_ALL;
    bool auto_switch_device = false;
    double joy_threshold = 0.3;
    bool process_in_physics = false;
    bool dispatch_bindings = true;
    bool runtime_copies = true;

    bool set_up = false;
    TypedArray<InputContext> runtime_contexts;
    Dictionary active_states; // nom de contexte → actif à la dernière mise à jour
    PackedStringArray context_stack;
    Dictionary stack_previous_enabled; // nom → état « enabled » avant push_context
    InputAction::DeviceFilter last_device_type = InputAction::DEVICE_KEYBOARD_MOUSE;
    Dictionary default_bindings;
    Array event_log;
    HashSet<uint64_t> warned_bindings;

    /// Contextes de jeu, triés par priorité effective décroissante (empilés en tête).
    Vector<Ref<InputContext>> _sorted_contexts() const;
    void _report_binding_failure(InputAction *p_action, const Ref<InputBinding> &p_binding, const String &p_reason);
    void _log_event(InputAction *p_action, const StringName &p_event, const Array &p_arguments, const PackedStringArray &p_calls);
};
} // namespace godot

#endif // PLAYER_INPUT_HPP
