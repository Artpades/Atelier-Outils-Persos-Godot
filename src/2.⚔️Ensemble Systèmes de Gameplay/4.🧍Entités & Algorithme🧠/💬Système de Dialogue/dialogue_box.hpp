#ifndef DIALOGUE_BOX_HPP
#define DIALOGUE_BOX_HPP

#include "dialogue_system.hpp"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot
{
/// 🗨️ Boîte de dialogue prête à l'emploi pour un DialogueSystem : orateur, portrait, texte affiché lettre
/// par lettre (BBCode accepté), boutons de choix. advance_action (ui_accept par défaut) termine l'écriture
/// puis passe à la suite. Se masque quand aucun dialogue n'est en cours (hide_when_idle).
/// Ses enfants sont créés par code : la positionner et la thémer comme un PanelContainer.
class DialogueBox : public PanelContainer
{
    GDCLASS(DialogueBox, PanelContainer)

  public:
    DialogueBox();

    void set_dialogue_system_path(const NodePath &p_path);
    NodePath get_dialogue_system_path() const { return dialogue_system_path; }
    void set_dialogue_system(DialogueSystem *p_system);
    DialogueSystem *get_dialogue_system() const;
    void set_characters_per_second(double p_speed) { characters_per_second = MAX(0.0, p_speed); }
    double get_characters_per_second() const { return characters_per_second; }
    void set_advance_action(const StringName &p_action) { advance_action = p_action; }
    StringName get_advance_action() const { return advance_action; }
    void set_hide_when_idle(bool p_hide);
    bool get_hide_when_idle() const { return hide_when_idle; }

    bool is_typing() const { return typing; }
    void skip_typing();
    /// Comme l'action d'avance : finit l'écriture, sinon passe à la réplique suivante.
    void press_advance();
    /// Texte affiché (sans BBCode) et nombre de boutons de choix (pour les tests).
    String get_displayed_text() const;
    String get_speaker_text() const;
    int get_choice_count() const;

    void _unhandled_input(const Ref<InputEvent> &p_event) override;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    NodePath dialogue_system_path;
    ObjectID system_id;
    double characters_per_second = 40.0;
    StringName advance_action = "ui_accept";
    bool hide_when_idle = true;

    TextureRect *portrait = nullptr; // enfants possédés
    Label *speaker_label = nullptr;
    RichTextLabel *text_label = nullptr;
    VBoxContainer *choices_box = nullptr;
    Label *continue_hint = nullptr;

    bool typing = false;
    double typed_characters = 0.0;

    void _connect_system(DialogueSystem *p_system, bool p_connect);
    void _clear_choices();
    void _on_line_shown(const String &p_speaker, const String &p_text, const Ref<DialogueLine> &p_line);
    void _on_choices_shown(const String &p_prompt, const Array &p_options, const Ref<DialogueChoice> &p_choice);
    void _on_dialogue_ended(const Ref<DialogueResource> &p_dialogue);
    void _on_choice_pressed(int p_index);
};
} // namespace godot

#endif // DIALOGUE_BOX_HPP
