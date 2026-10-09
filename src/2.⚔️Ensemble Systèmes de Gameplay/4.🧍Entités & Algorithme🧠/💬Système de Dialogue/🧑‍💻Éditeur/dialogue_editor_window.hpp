#ifndef DIALOGUE_EDITOR_WINDOW_HPP
#define DIALOGUE_EDITOR_WINDOW_HPP

#include "../dialogue_resource.hpp"

#include "../../🤾Système de Comportement/🗺️Graphe de comportement/behavior_graph_view.hpp"

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
/// 💬 Fenêtre « Éditeur de dialogue » (éditeur uniquement) : le DialogueResource en TechTree / MindMap.
///
/// Clic droit : ajouter un nœud (après celui-ci : il est relié), relier / délier, définir le départ,
/// renommer l'identifiant (les liens suivent), supprimer (les liens entrants sont vidés) ; Maj + glisser
/// d'un nœud à un autre : lien. Toutes les modifications sont annulables. Un clic ouvre le nœud dans
/// l'inspecteur (texte, orateur, conditions...). « Enregistrer » sauve un dialogue en fichier (.tres).
/// Débogage : quand le jeu tourne, affiche en direct le dialogue d'un DialogueSystem choisi dans la liste.
class DialogueEditorWindow : public Window
{
    GDCLASS(DialogueEditorWindow, Window)

  public:
    static void open_for(const Ref<DialogueResource> &p_dialogue);
    static void close_current();
    static DialogueEditorWindow *get_current();

  protected:
    static void _bind_methods() {}
    void _notification(int p_what);

  private:
    enum MenuAction
    {
        ACTION_ADD,
        ACTION_ADD_AFTER,
        ACTION_LINK,
        ACTION_UNLINK,
        ACTION_SET_START,
        ACTION_EDIT,
        ACTION_RENAME,
        ACTION_DELETE,
        ACTION_FIT,
    };

    static ObjectID current_window_id;

    Ref<DialogueResource> dialogue;
    BehaviorGraphView *view = nullptr; // enfants possédés
    Label *title_label = nullptr;
    Label *status_label = nullptr;
    OptionButton *layout_option = nullptr;
    CheckBox *follow_game_check = nullptr;
    OptionButton *remote_option = nullptr;
    PopupMenu *context_menu = nullptr;
    ConfirmationDialog *rename_dialog = nullptr;
    LineEdit *rename_edit = nullptr;

    String context_id;
    /// Identifiant d'élément de menu -> [action, argument].
    Dictionary menu_actions;
    int next_menu_id = 0;
    /// DialogueSystem du jeu affiché ("" : le dialogue édité).
    String remote_path;
    String remote_signature;
    bool showing_remote = false;
    double poll_accumulator = 0.0;
    /// Instantané précédent sérialisé : le graphe n'est redessiné que s'il change.
    String last_signature;

    void _build_ui();
    void _set_dialogue(const Ref<DialogueResource> &p_dialogue);
    void _refresh(bool p_force = false);
    void _poll();
    void _update_remote_list();
    Ref<DialogueNode> _node(const String &p_id) const;

    void _on_close_requested();
    void _on_layout_selected(int p_index);
    void _on_save_pressed();
    void _on_validate_pressed();
    void _on_remote_selected(int p_index);
    void _on_node_selected(const String &p_id);
    void _on_context_menu_requested(const String &p_id, const Vector2 &p_screen_position);
    void _on_menu_id_pressed(int p_id);
    void _on_link_requested(const String &p_from, const String &p_to);
    void _on_delete_requested(const String &p_id);
    void _on_rename_confirmed();

    int _add_action(PopupMenu *p_menu, const String &p_label, MenuAction p_action, const Variant &p_argument = Variant());
    PopupMenu *_types_submenu(MenuAction p_action);

    // ✏️ Édition annulable
    /// Objet et propriété portant le lien n° p_slot d'un nœud (next, next_if_true, option.next...).
    static bool _link_property(const Ref<DialogueNode> &p_node, int p_slot, Object *&r_object, StringName &r_property);
    Ref<DialogueNode> _create_node(const String &p_type) const;
    void _add_node(const String &p_type, const Ref<DialogueNode> &p_after);
    void _link(const Ref<DialogueNode> &p_from, const Ref<DialogueNode> &p_to);
    void _unlink(const Ref<DialogueNode> &p_from, int p_slot);
    void _set_start(const Ref<DialogueNode> &p_node);
    void _rename(const Ref<DialogueNode> &p_node, const StringName &p_id);
    void _delete(const Ref<DialogueNode> &p_node);
    void _after_edit(const String &p_select);
};
} // namespace godot

#endif // DIALOGUE_EDITOR_WINDOW_HPP
