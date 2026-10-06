#ifndef PLAYER_INPUT_UI_HPP
#define PLAYER_INPUT_UI_HPP

#include "../Actions/input_action.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot
{
class PlayerInput;
class InputRebinder;

/// ⚙️ Menu de réassignation des contrôles, en jeu : un onglet clavier / souris et un onglet manette listent les
/// actions du PlayerInput (player_input) et leurs emplacements (un bouton chacun : clic, puis appuyer sur la
/// nouvelle touche). « Enregistrer » écrit les contrôles dans save_path, relus au démarrage (auto_load) ;
/// « Réinitialiser » rétablit les contrôles d'origine. Un InputRebinder est créé si `rebinder` est vide.
class PlayerInputUI : public VBoxContainer
{
    GDCLASS(PlayerInputUI, VBoxContainer)

  public:
    void set_player_input(const NodePath &p_path) { player_input = p_path; }
    NodePath get_player_input() const { return player_input; }
    void set_rebinder(const NodePath &p_path) { rebinder_path = p_path; }
    NodePath get_rebinder() const { return rebinder_path; }
    void set_save_path(const String &p_path) { save_path = p_path; }
    String get_save_path() const { return save_path; }
    void set_auto_load(bool p_auto) { auto_load = p_auto; }
    bool get_auto_load() const { return auto_load; }
    void set_show_keyboard_tab(bool p_show) { show_keyboard_tab = p_show; }
    bool get_show_keyboard_tab() const { return show_keyboard_tab; }
    void set_show_gamepad_tab(bool p_show) { show_gamepad_tab = p_show; }
    bool get_show_gamepad_tab() const { return show_gamepad_tab; }
    void set_context_filter(const PackedStringArray &p_filter) { context_filter = p_filter; }
    PackedStringArray get_context_filter() const { return context_filter; }

    /// Reconstruit les listes (après un changement de contextes).
    void rebuild();
    /// Met à jour le texte des boutons.
    void refresh();
    Error save_bindings();
    void reset_bindings();
    PlayerInput *get_player() const;
    InputRebinder *get_active_rebinder() const;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    NodePath player_input;
    NodePath rebinder_path;
    String save_path = "user://controles.cfg";
    bool auto_load = true;
    bool show_keyboard_tab = true;
    bool show_gamepad_tab = true;
    PackedStringArray context_filter;

    TabContainer *tabs = nullptr;
    Label *status = nullptr;
    InputRebinder *own_rebinder = nullptr;
    Array slot_buttons; // [ [bouton, action, emplacement], ... ]

    void _build_frame();
    void _fill_tab(VBoxContainer *p_list, const String &p_group);
    void _on_slot_pressed(const Ref<InputAction> &p_action, const StringName &p_slot);
    void _on_rebind_started(const Ref<InputAction> &p_action, const StringName &p_slot);
    void _on_rebind_completed(const Ref<InputAction> &p_action, const StringName &p_slot);
    void _on_rebind_cancelled(const Ref<InputAction> &p_action, const StringName &p_slot);
    void _on_save_pressed();
    void _on_reset_pressed();
};
} // namespace godot

#endif // PLAYER_INPUT_UI_HPP
