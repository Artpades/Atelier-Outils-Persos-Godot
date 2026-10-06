#ifndef INPUT_EDITOR_WINDOW_HPP
#define INPUT_EDITOR_WINDOW_HPP

#include "../Gestionnaires/input_context.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
class PlayerInput;

/// 📈 Jauge d'une action (débogage) : voyant (bouton), barre (axe), disque (Vector2), disque + barre
/// (Vector3). Glisser dans la jauge simule une valeur (signal simulate_requested), relâcher l'arrête.
class InputActionMeter : public Control
{
    GDCLASS(InputActionMeter, Control)

  public:
    InputActionMeter();
    void set_value(const Variant &p_value);
    Variant get_value() const { return value; }
    void set_interactive(bool p_interactive) { interactive = p_interactive; }

    void _draw() override;
    void _gui_input(const Ref<InputEvent> &p_event) override;

  protected:
    static void _bind_methods();

  private:
    Variant value;
    bool interactive = true;
    bool dragging = false;

    Variant _value_at(const Vector2 &p_position) const;
};

/// 🕹️ Fenêtre « Éditeur d'entrées » (éditeur uniquement), ouverte depuis l'inspecteur d'un PlayerInput (ou
/// d'un InputContext seul).
///
/// - Arbre des contextes et de leurs actions : ajouter (contexte, action de chaque type), dupliquer,
///   supprimer, réordonner.
/// - ⚙️ Propriétés : réglages de l'élément choisi ; 🎯 capture la prochaine touche / bouton / axe pour une
///   entrée (clavier, souris, manette).
/// - 🔗 Liaisons : évènement de l'action → nœud cible → méthode (suggestions tirées de la scène éditée).
/// - 🐞 Débogage : valeurs en direct, journal des évènements et des appels de liaisons, activation des
///   contextes, simulation (glisser dans une jauge). Source : test dans l'éditeur (périphériques lus
///   directement, liaisons décrites sans être appelées) ou un PlayerInput du jeu en cours.
/// Toutes les modifications passent par l'historique d'annulation de l'éditeur.
class InputEditorWindow : public Window
{
    GDCLASS(InputEditorWindow, Window)

  public:
    static void open_for(Object *p_object);
    static void close_current();
    static InputEditorWindow *get_current();

    ~InputEditorWindow();

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    enum Source
    {
        SOURCE_NONE,
        SOURCE_LOCAL,
        SOURCE_REMOTE,
    };

    static ObjectID current_window_id;

    ObjectID player_id;
    Ref<InputContext> lone_context;
    Ref<InputContext> selected_context;
    Ref<InputAction> selected_action;

    Label *title_label = nullptr;
    Label *status_label = nullptr;
    Tree *tree = nullptr;
    Button *add_context_button = nullptr;
    MenuButton *add_action_button = nullptr;
    TabContainer *tabs = nullptr;
    VBoxContainer *properties_box = nullptr;
    VBoxContainer *bindings_box = nullptr;
    OptionButton *source_option = nullptr;
    VBoxContainer *debug_box = nullptr;
    RichTextLabel *log_label = nullptr;
    Label *debug_status = nullptr;

    String model_signature;
    bool refresh_pending = false;
    bool updating_tree = false;
    String sources_signature;
    double poll_accumulator = 0.0;

    // 🎯 Capture d'une entrée
    Ref<InputAction> capture_action;
    StringName capture_slot;

    // 🔗 Suggestions des liaisons
    PackedStringArray suggestions;

    // 🐞 Débogage
    Source source = SOURCE_NONE;
    PlayerInput *test_player = nullptr; // hors de l'arbre, possédé par la fenêtre
    String remote_path;
    String debug_signature;
    Dictionary debug_widgets; // "contexte/action" → [jauge, étiquette]
    String last_log_signature;

    // 🧱 Interface
    void _build_ui();
    void _set_target(Object *p_object);
    PlayerInput *_player() const;
    TypedArray<InputContext> _contexts() const;
    String _compute_signature() const;
    void _refresh_all();
    void _rebuild_tree();
    void _rebuild_properties();
    void _rebuild_bindings();
    void _set_status(const String &p_text);

    // ⚙️ Éditeurs de propriétés
    void _add_property_editors(VBoxContainer *p_box, Object *p_object, const PackedStringArray &p_skip, const Array &p_slots);
    void _on_bool_edited(bool p_value, uint64_t p_object_id, const StringName &p_property);
    void _on_number_edited(double p_value, uint64_t p_object_id, const StringName &p_property, bool p_integer);
    void _on_option_edited(int p_index, uint64_t p_option_id, uint64_t p_object_id, const StringName &p_property);
    /// p_kind : 0 = String, 1 = StringName, 2 = NodePath.
    void _on_text_submitted(const String &p_text, uint64_t p_object_id, const StringName &p_property, int p_kind);
    void _on_text_focus_exited(uint64_t p_line_id, uint64_t p_object_id, const StringName &p_property, int p_kind);
    LineEdit *_add_line_edit(Control *p_parent, Object *p_object, const StringName &p_property, int p_kind, const String &p_placeholder = String());
    void _commit_property(Object *p_object, const StringName &p_property, const Variant &p_value, bool p_merge = false);
    void _on_inspect_pressed(uint64_t p_object_id);

    // 🎯 Capture
    void _on_capture_pressed(const StringName &p_slot);
    void _on_clear_slot_pressed(const StringName &p_slot);
    void _on_window_input(const Ref<InputEvent> &p_event);
    void _poll_joypad_capture();
    void _finish_capture(const Ref<InputEvent> &p_event);

    // 🌳 Arbre et structure
    void _on_tree_selected();
    void _on_add_context_pressed();
    void _on_add_action_type(int p_id);
    void _on_duplicate_pressed();
    void _on_delete_pressed();
    void _on_move_pressed(int p_direction);
    void _on_save_pressed();
    void _set_array(Object *p_owner, const StringName &p_property, const Array &p_before, const Array &p_after, const String &p_action_name);
    void _select(const Ref<InputContext> &p_context, const Ref<InputAction> &p_action);

    // 🔗 Liaisons
    void _on_add_binding_pressed();
    void _on_remove_binding_pressed(uint64_t p_binding_id);
    void _on_binding_event_selected(int p_index, uint64_t p_option_id, uint64_t p_binding_id);
    void _on_target_suggestions(uint64_t p_menu_id, uint64_t p_binding_id);
    void _on_target_suggestion_chosen(int p_id, uint64_t p_binding_id);
    void _on_method_suggestions(uint64_t p_menu_id, uint64_t p_binding_id);
    void _on_method_suggestion_chosen(int p_id, uint64_t p_binding_id);
    void _on_arguments_submitted(const String &p_text, uint64_t p_binding_id);
    void _on_arguments_focus_exited(uint64_t p_line_id, uint64_t p_binding_id);
    void _commit_arguments(const String &p_text, const Ref<InputBinding> &p_binding);
    Node *_binding_target(const Ref<InputBinding> &p_binding) const;

    // 🐞 Débogage
    void _update_source_list();
    void _on_source_selected(int p_index);
    void _start_local_test();
    void _stop_local_test();
    void _update_debug(double p_delta);
    void _rebuild_debug(const Dictionary &p_snapshot);
    void _on_meter_simulate(const Variant &p_value, const StringName &p_action);
    void _on_meter_released(const StringName &p_action);
    void _on_context_toggled(bool p_enabled, const StringName &p_context);
    void _on_clear_log_pressed();
    void _on_close_requested();
};
} // namespace godot

#endif // INPUT_EDITOR_WINDOW_HPP
