#ifndef BEHAVIOR_EDITOR_WINDOW_HPP
#define BEHAVIOR_EDITOR_WINDOW_HPP

#include "../behavior_graph_view.hpp"

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
class State;

/// 🗺️ Fenêtre « Éditeur de comportement » (éditeur uniquement), ouverte par le bouton de l'inspecteur.
///
/// Affiche la StateMachine ou le BehaviorTree de la scène éditée en TechTree / arbre vertical / MindMap :
///   - édition annulable (EditorUndoRedoManager) par clic droit : ajouter un sous-état / un enfant,
///     envelopper dans un décorateur, transitions, état initial, renommer, monter/descendre, supprimer ;
///     glisser un nœud sur un autre pour le déplacer, Maj + glisser pour créer une transition ;
///   - la sélection est synchronisée avec l'inspecteur et l'arbre de scène ;
///   - débogage : quand le jeu tourne (lancé depuis l'éditeur), la vue montre l'état en direct du même
///     comportement (ou d'un autre, choisi dans la liste) grâce au BehaviorDebuggerPlugin.
/// Une seule fenêtre à la fois ; sauf si elle est épinglée, elle suit la sélection de l'éditeur.
class BehaviorEditorWindow : public Window
{
    GDCLASS(BehaviorEditorWindow, Window)

  public:
    static void open_for(Node *p_node);
    static void close_current();
    static BehaviorEditorWindow *get_current();

    /// La sélection de l'éditeur a changé (appelé par BehaviorEditorPlugin).
    void on_editor_selection(Node *p_node);

  protected:
    static void _bind_methods() {}
    void _notification(int p_what);

  private:
    enum MenuAction
    {
        ACTION_ADD_CHILD,
        ACTION_WRAP,
        ACTION_RENAME,
        ACTION_DELETE,
        ACTION_MOVE_UP,
        ACTION_MOVE_DOWN,
        ACTION_SET_INITIAL,
        ACTION_ADD_TRANSITION,
        ACTION_EDIT_TRANSITION,
        ACTION_REMOVE_TRANSITION,
        ACTION_OPEN_SCRIPT,
        ACTION_FIT,
    };

    static ObjectID current_window_id;

    ObjectID target_id; // StateMachine ou BehaviorTree de la scène éditée
    BehaviorGraphView *view = nullptr; // enfants possédés
    Label *target_label = nullptr;
    OptionButton *layout_option = nullptr;
    CheckBox *follow_game_check = nullptr;
    CheckBox *pin_check = nullptr;
    OptionButton *remote_option = nullptr;
    Label *status_label = nullptr;
    PopupMenu *context_menu = nullptr;
    ConfirmationDialog *rename_dialog = nullptr;
    LineEdit *rename_edit = nullptr;

    String context_id;
    Dictionary menu_actions; // identifiant d'élément de menu -> [action, argument]
    int next_menu_id = 0;
    String remote_path;      // comportement du jeu affiché ("" : celui de la scène éditée)
    String remote_signature; // pour ne reconstruire la liste des comportements du jeu que si elle change
    bool showing_remote = false;
    double poll_accumulator = 0.0;

    void _build_ui();
    void _set_target(Node *p_runner);
    Node *_target() const;
    Node *_node_from_id(const String &p_id) const;
    String _id_of(Node *p_node) const;
    String _edited_path() const;

    void _poll();
    void _update_remote_list();
    void _heal_paths();

    void _on_close_requested();
    void _on_layout_selected(int p_index);
    void _on_labels_toggled(bool p_pressed);
    void _on_follow_toggled(bool p_pressed);
    void _on_remote_selected(int p_index);
    void _on_node_selected(const String &p_id);
    void _on_node_activated(const String &p_id);
    void _on_context_menu_requested(const String &p_id, const Vector2 &p_screen_position);
    void _on_menu_id_pressed(int p_id);
    void _on_node_dropped(const String &p_id, const String &p_target_id);
    void _on_link_requested(const String &p_from_id, const String &p_to_id);
    void _on_delete_requested(const String &p_id);
    void _on_rename_confirmed();

    // 📋 Menus
    int _add_action(PopupMenu *p_menu, const String &p_label, MenuAction p_action, const Variant &p_argument = Variant());
    PopupMenu *_types_submenu(const StringName &p_base, MenuAction p_action, bool p_containers_only = false);
    Array _creatable_types(const StringName &p_base) const;
    Node *_instantiate(const Dictionary &p_type) const;
    bool _accepts_children(Node *p_node) const;

    // ✏️ Édition annulable
    void _add_child(Node *p_parent, const Dictionary &p_type);
    void _wrap(Node *p_node, const Dictionary &p_type);
    void _delete(Node *p_node);
    void _move(Node *p_node, int p_offset);
    void _reparent(Node *p_node, Node *p_new_parent);
    void _set_initial(State *p_state);
    void _add_transition(State *p_from, State *p_to);
    void _remove_transition(State *p_from, int p_index);
    void _after_edit(Node *p_select);
};
} // namespace godot

#endif // BEHAVIOR_EDITOR_WINDOW_HPP
