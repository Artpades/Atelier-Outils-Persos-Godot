#ifndef INPUT_ACTION_HPP
#define INPUT_ACTION_HPP

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
class PlayerInput;

/// 🔗 Liaison d'une action (à la manière d'un UnityEvent) : quand l'action émet l'évènement `event`
/// (un de ses signaux : pressed, value_changed...), le PlayerInput appelle `method` sur le nœud `target`
/// (chemin relatif au PlayerInput). Arguments passés : la valeur du signal (si pass_value), puis `arguments`.
/// Exemple : value_changed → ../Joueur/MovementDirector.move(Vector2) ; pressed → ../Joueur.request_jump().
class InputBinding : public Resource
{
    GDCLASS(InputBinding, Resource)

  public:
    void set_enabled(bool p_enabled) { enabled = p_enabled; }
    bool is_enabled() const { return enabled; }
    void set_event(const StringName &p_event);
    StringName get_event() const { return event; }
    void set_target(const NodePath &p_target);
    NodePath get_target() const { return target; }
    void set_method(const StringName &p_method);
    StringName get_method() const { return method; }
    void set_pass_value(bool p_pass) { pass_value = p_pass; }
    bool get_pass_value() const { return pass_value; }
    void set_arguments(const Array &p_arguments) { arguments = p_arguments; }
    Array get_arguments() const { return arguments; }

    /// Arguments de l'appel pour les arguments p_signal_arguments du signal.
    Array build_call_arguments(const Array &p_signal_arguments) const;
    /// Texte lisible : « pressed → ../Joueur.request_jump() ».
    String get_description() const;

  protected:
    static void _bind_methods();

  private:
    bool enabled = true;
    StringName event = "pressed";
    NodePath target;
    StringName method;
    bool pass_value = true;
    Array arguments;
};

/// Paramètres de lecture fournis par le PlayerInput à chaque mise à jour d'une action.
struct InputReadContext
{
    int device = 0;
    bool keyboard_mouse = true;
    bool gamepad = true;
};

/// 🕹️ Action d'entrée (ressource, classe de base) : lit un ou plusieurs périphériques (clavier, souris,
/// manette) et émet des signaux (évènements) quand sa valeur change. Rangée dans un InputContext, elle est
/// mise à jour par le PlayerInput, qui transmet aussi ses évènements à ses liaisons (InputBinding).
///
/// device_filter : périphériques lus (INHERIT = celui du PlayerInput). dead_zone : zone morte des axes de
/// manette. dirty_threshold : variation minimale avant d'émettre un nouvel évènement de valeur.
/// Une action peut être étendue en GDScript : redéfinir _update_action(delta) (appeler emit_event()) et _get_value().
class InputAction : public Resource
{
    GDCLASS(InputAction, Resource)

  public:
    enum DeviceFilter
    {
        DEVICE_INHERIT,
        DEVICE_ALL,
        DEVICE_KEYBOARD_MOUSE,
        DEVICE_GAMEPAD,
    };

    void set_action_name(const StringName &p_name);
    StringName get_action_name() const { return action_name; }
    void set_description(const String &p_description) { description = p_description; }
    String get_description() const { return description; }
    void set_enabled(bool p_enabled);
    bool is_enabled() const { return enabled; }
    void set_device_filter(DeviceFilter p_filter) { device_filter = p_filter; }
    DeviceFilter get_device_filter() const { return device_filter; }
    void set_physical_keys(bool p_physical) { physical_keys = p_physical; }
    bool get_physical_keys() const { return physical_keys; }
    void set_dead_zone(double p_dead_zone) { dead_zone = CLAMP(p_dead_zone, 0.0, 1.0); }
    double get_dead_zone() const { return dead_zone; }
    void set_dirty_threshold(double p_threshold) { dirty_threshold = MAX(0.0, p_threshold); }
    double get_dirty_threshold() const { return dirty_threshold; }
    void set_bindings(const TypedArray<InputBinding> &p_bindings) { bindings = p_bindings; }
    TypedArray<InputBinding> get_bindings() const { return bindings; }

    // ⚙️ Exécution (appelé par le PlayerInput)
    void update(const InputReadContext &p_context, double p_delta, PlayerInput *p_owner);
    /// Évènement brut transmis par le PlayerInput (mouvement de souris, molette...).
    void handle_event(const Ref<InputEvent> &p_event, PlayerInput *p_owner);
    /// Remet l'action au repos (relâchée, valeur nulle), en émettant les évènements correspondants.
    void release(PlayerInput *p_owner);
    /// Émet le signal p_event (avec p_arguments) puis le transmet aux liaisons via le PlayerInput.
    void emit_event(const StringName &p_event, const Array &p_arguments = Array());

    // 📈 État
    /// Valeur courante (actions GDScript : redéfinir _get_value).
    virtual Variant get_value() const;
    /// Vrai si l'action est « en cours » (bouton enfoncé, axe hors du repos...).
    virtual bool is_active() const;
    /// Valeur affichable (débogage).
    String get_value_text() const;
    int get_device() const { return read_context.device; }
    bool is_keyboard_mouse_allowed() const { return read_context.keyboard_mouse; }
    bool is_gamepad_allowed() const { return read_context.gamepad; }
    StringName get_last_event() const { return last_event; }
    int get_event_count() const { return event_count; }

    // 🧪 Simulation (tests, débogage depuis l'éditeur) : la valeur simulée remplace les périphériques.
    void simulate(const Variant &p_value);
    void stop_simulation();
    bool is_simulated() const { return simulated; }

    // 🔗 Évènements et réassignation
    /// Évènements (signaux) que l'action peut émettre, sans ceux de Resource : [{name, args}].
    TypedArray<Dictionary> get_events() const;
    /// Propriétés décrivant les touches / boutons / axes lus (sauvegarde et réinitialisation des contrôles).
    virtual PackedStringArray get_device_properties() const { return PackedStringArray(); }
    /// Emplacements réassignables : [{slot, label, group ("kbm" | "gamepad")}].
    virtual TypedArray<Dictionary> get_rebind_slots() const { return TypedArray<Dictionary>(); }
    /// Assigne l'évènement p_event à l'emplacement p_slot ; faux si l'évènement ne convient pas.
    virtual bool apply_rebind(const StringName & /*p_slot*/, const Ref<InputEvent> & /*p_event*/, double /*p_joy_threshold*/) { return false; }
    /// Texte de l'entrée assignée à un emplacement (« Espace », « Manette A »...).
    virtual String get_slot_text(const StringName &p_slot) const;
    Dictionary get_device_state() const;
    void set_device_state(const Dictionary &p_state);
    /// Copie indépendante (et ses liaisons) pour un PlayerInput en jeu.
    Ref<InputAction> make_runtime_copy() const;

    // 🔤 Noms et hints des entrées (partagés par les actions, le Rebinder et l'éditeur)
    static String key_hint();
    static String mouse_button_hint();
    static String joy_button_hint();
    static String joy_axis_hint();
    static String key_text(int p_key);
    static String mouse_button_text(int p_button);
    static String joy_button_text(int p_button);
    static String joy_axis_text(int p_axis);
    /// Évènement de touche → code (physique si p_physical) ; 0 si ce n'est pas un appui de touche.
    static int event_key(const Ref<InputEvent> &p_event, bool p_physical);
    static int event_mouse_button(const Ref<InputEvent> &p_event);
    static int event_joy_button(const Ref<InputEvent> &p_event);
    /// Axe de manette poussé au-delà de p_threshold (-1 sinon) ; r_sign : sens de la poussée.
    static int event_joy_axis(const Ref<InputEvent> &p_event, double p_threshold, int *r_sign = nullptr);

  protected:
    static void _bind_methods();

    /// Lecture des périphériques et émission des évènements (actions natives).
    virtual void _update(double p_delta);
    virtual void _handle_event(const Ref<InputEvent> & /*p_event*/) {}
    virtual void _release() {}
    /// Application d'une valeur simulée (types natifs).
    virtual void _apply_simulation(const Variant & /*p_value*/, double /*p_delta*/) {}

    // Lecture (respecte le filtre de périphériques)
    bool key_down(int p_key) const;
    bool mouse_down(int p_button) const;
    bool joy_button_down(int p_button) const;
    /// Valeur brute d'un axe de manette, 0 si désactivé ou invalide.
    float joy_axis_value(int p_axis) const;
    /// Moins de dirty_threshold d'écart ?
    bool same_float(double p_a, double p_b) const;
    /// PlayerInput qui met l'action à jour (nul hors mise à jour).
    Node *get_owner_node() const;

    InputReadContext read_context;
    bool simulated = false;
    Variant simulated_value;

    GDVIRTUAL1(_update_action, double)
    GDVIRTUAL0RC(Variant, _get_value)

  private:
    StringName action_name = "action";
    String description;
    bool enabled = true;
    DeviceFilter device_filter = DEVICE_INHERIT;
    bool physical_keys = true;
    double dead_zone = 0.2;
    double dirty_threshold = 0.01;
    TypedArray<InputBinding> bindings;

    ObjectID owner_id;
    StringName last_event;
    int event_count = 0;
};
} // namespace godot

VARIANT_ENUM_CAST(InputAction::DeviceFilter);

#endif // INPUT_ACTION_HPP
