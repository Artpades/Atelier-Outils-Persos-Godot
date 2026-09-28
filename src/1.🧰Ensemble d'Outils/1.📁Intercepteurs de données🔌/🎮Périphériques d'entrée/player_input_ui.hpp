#ifndef PLAYER_INPUT_UI_HPP
#define PLAYER_INPUT_UI_HPP

#include "input_rebinder.hpp"
#include "player_input.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot
{
/// ⚙️ Écran de configuration des contrôles, généré à partir des InputAction d'un PlayerInput : un onglet
/// clavier/souris, un onglet manette, un bouton par emplacement réaffectable, sauvegarde via SaveManager.
///
/// Chaque bouton est relié à sa valeur par un 🔗 InspectorBinding (outil de l'inspecteur procédural) : son
/// texte suit la valeur réelle de l'action, quelle que soit la source du changement (rebinder, réinitialisation,
/// chargement, script).
///
/// Modules requis : 🧩 Générateur de Widget d'éditions (liaisons) et 💾 Manipulateur de données (sauvegarde).
/// S'il en manque un, l'interface n'est pas générée. Le SaveData est manipulé comme une Resource : sa classe
/// appartient au module de sauvegarde.
class PlayerInputUI : public Control
{
    GDCLASS(PlayerInputUI, Control)

  public:
    void _ready() override;
    PackedStringArray _get_configuration_warnings() const override;

    /// Régénère les lignes (à appeler si des actions sont ajoutées ou retirées du PlayerInput).
    void rebuild();
    /// Écrit les affectations dans save_data puis sur disque.
    void apply_and_save();
    /// Rétablit les affectations d'origine de toutes les actions.
    void reset_all();
    /// Rétablit les affectations d'origine d'une action.
    void reset_action(InputAction *p_action);
    /// Relit la sauvegarde et l'applique aux actions. Faux si aucune sauvegarde n'existe.
    bool load_saved_bindings();

    /// Texte affiché pour un emplacement, ex. « Haut : Z ».
    static String describe_slot(const InputAction *p_action, InputRebinder::TargetSlot p_slot);

    void set_target_player_input(PlayerInput *p_player);
    PlayerInput *get_target_player_input() const;

    void set_rebinder(InputRebinder *p_rebinder);
    InputRebinder *get_rebinder() const;

    void set_save_data(const Ref<Resource> &p_save_data) { save_data = p_save_data; }
    Ref<Resource> get_save_data() const { return save_data; }

    void set_minimum_list_size(const Vector2 &p_size) { minimum_list_size = p_size; }
    Vector2 get_minimum_list_size() const { return minimum_list_size; }

    void set_show_kbm_tab(bool p_show) { show_kbm_tab = p_show; }
    bool get_show_kbm_tab() const { return show_kbm_tab; }

    void set_show_gamepad_tab(bool p_show) { show_gamepad_tab = p_show; }
    bool get_show_gamepad_tab() const { return show_gamepad_tab; }

  protected:
    static void _bind_methods();

  private:
    void build_structure();
    void capture_defaults();
    void add_action_row(VBoxContainer *p_list, InputAction *p_action, bool p_kbm);
    void add_slot_button(Control *p_parent, InputAction *p_action, InputRebinder::TargetSlot p_slot);
    void add_circle_preset_selector(Control *p_parent, InputAction *p_action);
    void set_controls_enabled(bool p_enabled);
    void set_status(const String &p_text);

    void on_rebind_started(Object *p_action, int p_slot);
    void on_rebind_completed(Object *p_action, int p_slot);
    void on_rebind_cancelled();
    /// Fin d'écoute (réussie ou non) : réactive l'interface et rafraîchit les liaisons.
    void end_rebind(const String &p_status);

    ObjectID target_player_input_id;
    ObjectID rebinder_id;
    Ref<Resource> save_data;
    Vector2 minimum_list_size = Vector2(600, 350);
    bool show_kbm_tab = true;
    bool show_gamepad_tab = true;

    TabContainer *tab_container = nullptr;
    VBoxContainer *kbm_list = nullptr;
    VBoxContainer *gamepad_list = nullptr;
    Label *status_label = nullptr;
    Button *apply_button = nullptr;
    Button *reset_button = nullptr;

    /// Contrôles désactivés pendant une écoute du rebinder.
    Vector<ObjectID> row_controls;
    /// Bouton de l'emplacement en cours de réaffectation (reprend le focus à la fin).
    ObjectID rebinding_button_id;
    /// Affectations d'origine, par instance d'action : { propriété: valeur } dans l'ordre de get_binding_properties().
    HashMap<uint64_t, Dictionary> defaults;
};
} // namespace godot

#endif // PLAYER_INPUT_UI_HPP
