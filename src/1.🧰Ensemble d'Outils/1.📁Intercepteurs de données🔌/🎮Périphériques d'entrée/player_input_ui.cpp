#include "player_input_ui.hpp"

#include "input_axis_action.hpp"
#include "input_button_action.hpp"
#include "input_circle_action.hpp"
#include "input_context.hpp"
#include "input_names.hpp"
#include "input_sphere_action.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal/custom_logger_api.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🧩Générateur de Widget d'éditions/🔍Inspecteur procédural/procedural_inspector_api.hpp"
#include "1.🧰Ensemble d'Outils/1.📁Intercepteurs de données🔌/💾Manipulateur de données🔐/save_manager_api.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace
{
const modules::Dependency REQUIRED_MODULES[] = {procedural_inspector_api::DEPENDENCY, save_manager_api::DEPENDENCY};

/// Emplacements affichés pour une action, par onglet.
Vector<InputRebinder::TargetSlot> slots_for(const InputAction *p_action, bool p_kbm)
{
    using S = InputRebinder;
    if (Object::cast_to<InputButtonAction>(p_action))
    {
        return p_kbm ? Vector<S::TargetSlot>{S::SLOT_BUTTON_KBM} : Vector<S::TargetSlot>{S::SLOT_BUTTON_GAMEPAD};
    }
    if (Object::cast_to<InputAxisAction>(p_action))
    {
        return p_kbm ? Vector<S::TargetSlot>{S::SLOT_AXIS_POSITIVE_KEY, S::SLOT_AXIS_NEGATIVE_KEY}
                     : Vector<S::TargetSlot>{S::SLOT_AXIS_GAMEPAD_AXIS, S::SLOT_AXIS_POSITIVE_GAMEPAD_BUTTON, S::SLOT_AXIS_NEGATIVE_GAMEPAD_BUTTON};
    }
    if (Object::cast_to<InputCircleAction>(p_action))
    {
        // Côté manette, le cercle se règle par préréglage (sélecteur dédié), pas par emplacement.
        return p_kbm ? Vector<S::TargetSlot>{S::SLOT_UP_KEY, S::SLOT_DOWN_KEY, S::SLOT_LEFT_KEY, S::SLOT_RIGHT_KEY} : Vector<S::TargetSlot>();
    }
    if (Object::cast_to<InputSphereAction>(p_action))
    {
        return p_kbm ? Vector<S::TargetSlot>{S::SLOT_UP_KEY, S::SLOT_DOWN_KEY, S::SLOT_LEFT_KEY, S::SLOT_RIGHT_KEY, S::SLOT_FORWARD_KEY, S::SLOT_BACKWARD_KEY}
                     : Vector<S::TargetSlot>{S::SLOT_JOY_AXIS_X, S::SLOT_JOY_AXIS_Y, S::SLOT_JOY_AXIS_Z};
    }
    return Vector<S::TargetSlot>();
}

/// Index du sélecteur de préréglage manette d'un cercle (-1 : réglage personnalisé).
int circle_preset_index(const InputCircleAction *p_circle)
{
    if (p_circle->get_joy_axis_preset() == InputCircleAction::JOY_PRESET_LEFT_STICK)
    {
        return 0;
    }
    if (p_circle->get_joy_axis_preset() == InputCircleAction::JOY_PRESET_RIGHT_STICK)
    {
        return 1;
    }
    if (p_circle->get_dpad_preset() == InputCircleAction::DPAD_PRESET_TRADITIONAL)
    {
        return 2;
    }
    if (p_circle->get_dpad_preset() == InputCircleAction::DPAD_PRESET_ABXY)
    {
        return 3;
    }
    return -1;
}

/// Un stick OU une croix : le préréglage choisi efface l'autre source.
void apply_circle_preset(InputCircleAction *p_circle, int p_index)
{
    if (p_index == 0 || p_index == 1)
    {
        p_circle->set_joy_axis_preset(p_index == 0 ? InputCircleAction::JOY_PRESET_LEFT_STICK : InputCircleAction::JOY_PRESET_RIGHT_STICK);
        p_circle->set_dpad_preset(InputCircleAction::DPAD_PRESET_CUSTOM);
        p_circle->set_dpad_up(JOY_BUTTON_INVALID);
        p_circle->set_dpad_down(JOY_BUTTON_INVALID);
        p_circle->set_dpad_left(JOY_BUTTON_INVALID);
        p_circle->set_dpad_right(JOY_BUTTON_INVALID);
    }
    else if (p_index == 2 || p_index == 3)
    {
        p_circle->set_dpad_preset(p_index == 2 ? InputCircleAction::DPAD_PRESET_TRADITIONAL : InputCircleAction::DPAD_PRESET_ABXY);
        p_circle->set_joy_axis_preset(InputCircleAction::JOY_PRESET_CUSTOM);
        p_circle->set_joy_axis_x(JOY_AXIS_INVALID);
        p_circle->set_joy_axis_y(JOY_AXIS_INVALID);
    }
}

/// Lie `p_widget` à une valeur calculée sur une action (InspectorBinding, via le module de l'inspecteur).
/// La liaison s'arrête dès que l'action est libérée.
Node *bind_to_action(Control *p_widget, InputAction *p_action, std::function<Variant(InputAction *)> p_get, std::function<void(const Variant &)> p_pull,
                     std::function<void(InputAction *, const Variant &)> p_set = nullptr)
{
    ObjectID action_id = ObjectID(p_action->get_instance_id());
    auto resolve = [action_id]() { return Object::cast_to<InputAction>(ObjectDB::get_instance(action_id)); };

    Callable setter;
    if (p_set)
    {
        setter = lambda_callable_1(p_action, [resolve, p_set](const Variant &p_value) { p_set(resolve(), p_value); });
    }
    return procedural_inspector_api::bind_value(p_widget, p_action, [resolve, p_get]() { return p_get(resolve()); }, p_pull, setter);
}
} // namespace

// ─── Cycle de vie ───────────────────────────────────────────────────────────────────────────────────────

void PlayerInputUI::_ready()
{
    if (!modules::require("PlayerInputUI", REQUIRED_MODULES))
    {
        return;
    }
    build_structure();

    bool editor = Engine::get_singleton()->is_editor_hint();
    if (InputRebinder *rebinder = get_rebinder(); rebinder && !editor)
    {
        rebinder->connect("rebind_started", callable_mp(this, &PlayerInputUI::on_rebind_started));
        rebinder->connect("rebind_completed", callable_mp(this, &PlayerInputUI::on_rebind_completed));
        rebinder->connect("rebind_cancelled", callable_mp(this, &PlayerInputUI::on_rebind_cancelled));
    }

    if (get_target_player_input() == nullptr)
    {
        if (!editor)
        {
            custom_logger_api::log(get_name(), InputNames::ICON_UI, "[color=orange]Aucun PlayerInput cible : l'interface reste vide.[/color]");
        }
        return;
    }

    capture_defaults();
    // Dans l'éditeur, on se contente d'un aperçu : charger la sauvegarde modifierait la scène éditée.
    if (!editor)
    {
        load_saved_bindings();
    }
    rebuild();
}

void PlayerInputUI::build_structure()
{
    // Nœuds internes : absents du dock Scène et jamais enregistrés avec la scène.
    VBoxContainer *layout = memnew(VBoxContainer);
    layout->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
    add_child(layout, false, INTERNAL_MODE_FRONT);

    Label *title = memnew(Label);
    title->set_text(U"Configuration des Contrôles");
    title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    layout->add_child(title);

    tab_container = memnew(TabContainer);
    tab_container->set_v_size_flags(SIZE_EXPAND_FILL);
    tab_container->set_h_size_flags(SIZE_EXPAND_FILL);
    layout->add_child(tab_container);

    auto add_tab = [this](const String &p_title) {
        ScrollContainer *scroll = memnew(ScrollContainer);
        scroll->set_custom_minimum_size(minimum_list_size);
        scroll->set_h_size_flags(SIZE_EXPAND_FILL);
        scroll->set_v_size_flags(SIZE_EXPAND_FILL);
        VBoxContainer *list = memnew(VBoxContainer);
        list->set_h_size_flags(SIZE_EXPAND_FILL);
        scroll->add_child(list);
        tab_container->add_child(scroll);
        tab_container->set_tab_title(tab_container->get_tab_count() - 1, p_title);
        return list;
    };
    kbm_list = show_kbm_tab ? add_tab(U"Clavier & Souris") : nullptr;
    gamepad_list = show_gamepad_tab ? add_tab(U"Manette") : nullptr;
    // Un seul onglet : la barre d'onglets n'apporte rien.
    tab_container->set_tabs_visible(show_kbm_tab && show_gamepad_tab);

    status_label = memnew(Label);
    status_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    layout->add_child(status_label);

    HBoxContainer *bar = memnew(HBoxContainer);
    bar->set_alignment(BoxContainer::ALIGNMENT_CENTER);
    layout->add_child(bar);

    apply_button = memnew(Button);
    apply_button->set_text("Appliquer & Sauvegarder");
    apply_button->connect("pressed", callable_mp(this, &PlayerInputUI::apply_and_save));
    bar->add_child(apply_button);

    reset_button = memnew(Button);
    reset_button->set_text(U"Tout réinitialiser");
    reset_button->connect("pressed", callable_mp(this, &PlayerInputUI::reset_all));
    bar->add_child(reset_button);
}

void PlayerInputUI::capture_defaults()
{
    defaults.clear();
    PlayerInput *player = get_target_player_input();
    TypedArray<InputAction> actions = player->get_all_actions();
    for (int i = 0; i < actions.size(); i++)
    {
        InputAction *action = Object::cast_to<InputAction>(actions[i]);
        Dictionary snapshot;
        for (const String &property : action->get_binding_properties())
        {
            snapshot[property] = action->get(property);
        }
        defaults[action->get_instance_id()] = snapshot;
    }
}

// ─── Génération des lignes ──────────────────────────────────────────────────────────────────────────────

void PlayerInputUI::rebuild()
{
    row_controls.clear();
    for (VBoxContainer *list : {kbm_list, gamepad_list})
    {
        if (list == nullptr)
        {
            continue;
        }
        for (int i = list->get_child_count() - 1; i >= 0; i--)
        {
            Node *child = list->get_child(i);
            list->remove_child(child);
            child->queue_free();
        }
    }

    PlayerInput *player = get_target_player_input();
    if (player == nullptr)
    {
        return;
    }

    TypedArray<InputAction> actions = player->get_all_actions();
    for (VBoxContainer *list : {kbm_list, gamepad_list})
    {
        if (list == nullptr)
        {
            continue;
        }
        bool kbm = list == kbm_list;
        String current_context;
        for (int i = 0; i < actions.size(); i++)
        {
            InputAction *action = Object::cast_to<InputAction>(actions[i]);
            bool has_content = !slots_for(action, kbm).is_empty() || (!kbm && Object::cast_to<InputCircleAction>(action));
            if (!has_content)
            {
                continue; // souris, gyroscope... : rien à réaffecter dans cet onglet
            }

            InputContext *context = Object::cast_to<InputContext>(action->get_parent());
            String context_name = context ? context->get_context_name() : String();
            if (!context_name.is_empty() && context_name != current_context)
            {
                current_context = context_name;
                Label *header = memnew(Label);
                header->set_text("--- " + context_name + " ---");
                header->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
                list->add_child(header);
            }
            add_action_row(list, action, kbm);
        }
    }
}

void PlayerInputUI::add_action_row(VBoxContainer *p_list, InputAction *p_action, bool p_kbm)
{
    HBoxContainer *row = memnew(HBoxContainer);
    row->set_h_size_flags(SIZE_EXPAND_FILL);
    p_list->add_child(row);

    Label *label = memnew(Label);
    label->set_custom_minimum_size(Vector2(180, 0));
    row->add_child(label);
    // Le nom aussi est lié : renommer l'action (script, inspecteur en jeu) met la ligne à jour.
    bind_to_action(label, p_action, [](InputAction *a) { return String(a->get_action_name()); }, [label](const Variant &v) { label->set_text(v); });

    HBoxContainer *slots = memnew(HBoxContainer);
    slots->set_h_size_flags(SIZE_EXPAND_FILL);
    row->add_child(slots);

    for (InputRebinder::TargetSlot slot : slots_for(p_action, p_kbm))
    {
        add_slot_button(slots, p_action, slot);
    }
    if (!p_kbm && Object::cast_to<InputCircleAction>(p_action))
    {
        add_circle_preset_selector(slots, p_action);
    }

    Button *reset = memnew(Button);
    reset->set_text(U"↺");
    reset->set_tooltip_text(U"Réinitialiser cette entrée par défaut");
    ObjectID action_id = ObjectID(p_action->get_instance_id());
    reset->connect("pressed", lambda_callable(this, [this, action_id]() { reset_action(Object::cast_to<InputAction>(ObjectDB::get_instance(action_id))); }));
    row->add_child(reset);
    row_controls.push_back(ObjectID(reset->get_instance_id()));
}

void PlayerInputUI::add_slot_button(Control *p_parent, InputAction *p_action, InputRebinder::TargetSlot p_slot)
{
    Button *button = memnew(Button);
    button->set_h_size_flags(SIZE_EXPAND_FILL);
    p_parent->add_child(button);
    row_controls.push_back(ObjectID(button->get_instance_id()));

    bind_to_action(button, p_action, [p_slot](InputAction *a) { return describe_slot(a, p_slot); }, [button](const Variant &v) { button->set_text(v); });

    ObjectID action_id = ObjectID(p_action->get_instance_id());
    button->connect("pressed", lambda_callable(button, [this, button, action_id, p_slot]() {
                        InputRebinder *rebinder = get_rebinder();
                        InputAction *action = Object::cast_to<InputAction>(ObjectDB::get_instance(action_id));
                        if (rebinder == nullptr || action == nullptr)
                        {
                            set_status(U"Aucun InputRebinder configuré.");
                            return;
                        }
                        rebinding_button_id = ObjectID(button->get_instance_id());
                        button->release_focus();
                        rebinder->start_rebind(action, p_slot);
                    }));
}

void PlayerInputUI::add_circle_preset_selector(Control *p_parent, InputAction *p_action)
{
    OptionButton *selector = memnew(OptionButton);
    selector->set_h_size_flags(SIZE_EXPAND_FILL);
    selector->add_item("Stick Gauche (Left Stick)", 0);
    selector->add_item("Stick Droit (Right Stick)", 1);
    selector->add_item("Croix Directionnelle (D-Pad)", 2);
    selector->add_item("Boutons d'action (ABXY)", 3);
    p_parent->add_child(selector);
    row_controls.push_back(ObjectID(selector->get_instance_id()));

    // Liaison bidirectionnelle : lecture = préréglage courant, écriture = application d'un préréglage.
    Node *binding = bind_to_action(
        selector, p_action, [](InputAction *a) { return circle_preset_index(Object::cast_to<InputCircleAction>(a)); }, [selector](const Variant &v) { selector->select(v); },
        [](InputAction *a, const Variant &v) { apply_circle_preset(Object::cast_to<InputCircleAction>(a), v); });

    selector->connect("item_selected", lambda_callable_1(selector, [binding](const Variant &v) { binding->call("commit", v); }));
}

String PlayerInputUI::describe_slot(const InputAction *p_action, InputRebinder::TargetSlot p_slot)
{
    if (p_action == nullptr)
    {
        return String();
    }
    const String unassigned = U"[Non assigné]";
    auto key = [p_action](const char *p_property) { return InputNames::key((Key)(int)p_action->get(p_property)); };
    auto button = [p_action](const char *p_property) { return InputNames::joy_button((JoyButton)(int)p_action->get(p_property)); };
    auto axis = [p_action](const char *p_property) { return InputNames::joy_axis((JoyAxis)(int)p_action->get(p_property)); };

    switch (p_slot)
    {
    case InputRebinder::SLOT_BUTTON_KBM:
        if ((int)p_action->get("key") != KEY_NONE)
        {
            return "Touche : " + key("key");
        }
        if ((int)p_action->get("mouse_button") != MOUSE_BUTTON_NONE)
        {
            return "Souris : " + InputNames::mouse_button((MouseButton)(int)p_action->get("mouse_button"));
        }
        return unassigned;
    case InputRebinder::SLOT_BUTTON_GAMEPAD:
        if ((int)p_action->get("gamepad_button") != JOY_BUTTON_INVALID)
        {
            return "Bouton : " + button("gamepad_button");
        }
        if ((int)p_action->get("gamepad_axis") != JOY_AXIS_INVALID)
        {
            return "Axe : " + axis("gamepad_axis");
        }
        return unassigned;
    case InputRebinder::SLOT_AXIS_POSITIVE_KEY:
        return "+Touche : " + key("positive_key");
    case InputRebinder::SLOT_AXIS_NEGATIVE_KEY:
        return "-Touche : " + key("negative_key");
    case InputRebinder::SLOT_AXIS_GAMEPAD_AXIS:
        return "Axe Stick : " + axis("gamepad_axis");
    case InputRebinder::SLOT_AXIS_POSITIVE_GAMEPAD_BUTTON:
        return "+Bouton : " + button("positive_gamepad_button");
    case InputRebinder::SLOT_AXIS_NEGATIVE_GAMEPAD_BUTTON:
        return "-Bouton : " + button("negative_gamepad_button");
    case InputRebinder::SLOT_UP_KEY:
        return "Haut : " + key("up_key");
    case InputRebinder::SLOT_DOWN_KEY:
        return "Bas : " + key("down_key");
    case InputRebinder::SLOT_LEFT_KEY:
        return "Gauche : " + key("left_key");
    case InputRebinder::SLOT_RIGHT_KEY:
        return "Droite : " + key("right_key");
    case InputRebinder::SLOT_FORWARD_KEY:
        return "Avant : " + key("forward_key");
    case InputRebinder::SLOT_BACKWARD_KEY:
        return String(U"Arrière : ") + key("backward_key");
    case InputRebinder::SLOT_DPAD_UP:
        return "Croix haut : " + button("dpad_up");
    case InputRebinder::SLOT_DPAD_DOWN:
        return "Croix bas : " + button("dpad_down");
    case InputRebinder::SLOT_DPAD_LEFT:
        return "Croix gauche : " + button("dpad_left");
    case InputRebinder::SLOT_DPAD_RIGHT:
        return "Croix droite : " + button("dpad_right");
    case InputRebinder::SLOT_DPAD_FORWARD:
        return "Croix avant : " + button("dpad_forward");
    case InputRebinder::SLOT_DPAD_BACKWARD:
        return String(U"Croix arrière : ") + button("dpad_backward");
    case InputRebinder::SLOT_JOY_AXIS_X:
        return "Axe X : " + axis("joy_axis_x");
    case InputRebinder::SLOT_JOY_AXIS_Y:
        return "Axe Y : " + axis("joy_axis_y");
    case InputRebinder::SLOT_JOY_AXIS_Z:
        return "Axe Z : " + axis("joy_axis_z");
    }
    return unassigned;
}

// ─── Réaffectation ──────────────────────────────────────────────────────────────────────────────────────

void PlayerInputUI::on_rebind_started(Object *p_action, int p_slot)
{
    if (Button *button = Object::cast_to<Button>(ObjectDB::get_instance(rebinding_button_id)))
    {
        // Pas écrasé par la liaison : la valeur ne change pas pendant l'écoute.
        button->set_text("[ Appuyez... ]");
    }
    InputAction *action = Object::cast_to<InputAction>(p_action);
    set_status(String(U"Appuyez sur une touche pour '") + String(action ? action->get_action_name() : StringName()) + String(U"' — [Échap] pour annuler"));
    set_controls_enabled(false);
}

void PlayerInputUI::on_rebind_completed(Object *p_action, int p_slot)
{
    InputAction *action = Object::cast_to<InputAction>(p_action);
    end_rebind(String(U"Réaffectation effectuée pour ") + String(action ? action->get_action_name() : StringName()));
}

void PlayerInputUI::on_rebind_cancelled() { end_rebind(U"Réaffectation annulée."); }

void PlayerInputUI::end_rebind(const String &p_status)
{
    set_status(p_status);
    set_controls_enabled(true);
    // Forcé : après une annulation la valeur n'a pas changé, et le bouton afficherait encore « Appuyez... ».
    procedural_inspector_api::refresh_bindings(this);
    if (Button *button = Object::cast_to<Button>(ObjectDB::get_instance(rebinding_button_id)))
    {
        button->grab_focus();
    }
    rebinding_button_id = ObjectID();
}

void PlayerInputUI::set_controls_enabled(bool p_enabled)
{
    apply_button->set_disabled(!p_enabled);
    reset_button->set_disabled(!p_enabled);
    for (const ObjectID &id : row_controls)
    {
        if (BaseButton *control = Object::cast_to<BaseButton>(ObjectDB::get_instance(id)))
        {
            control->set_disabled(!p_enabled);
        }
    }
}

void PlayerInputUI::set_status(const String &p_text)
{
    if (status_label)
    {
        status_label->set_text(p_text);
    }
}

// ─── Sauvegarde & réinitialisation ──────────────────────────────────────────────────────────────────────

void PlayerInputUI::apply_and_save()
{
    PlayerInput *player = get_target_player_input();
    if (save_data.is_null() || player == nullptr)
    {
        set_status(save_data.is_null() ? U"Aucun SaveData configuré." : U"Aucun PlayerInput cible.");
        return;
    }

    // Clé « nom de l'action _ propriété » : lisible dans le fichier, et stable si l'arbre est réorganisé.
    Dictionary data = save_data->get("data");
    TypedArray<InputAction> actions = player->get_all_actions();
    for (int i = 0; i < actions.size(); i++)
    {
        InputAction *action = Object::cast_to<InputAction>(actions[i]);
        String prefix = String(action->get_action_name()) + "_";
        for (const String &property : action->get_binding_properties())
        {
            data[prefix + property] = action->get(property);
        }
    }
    save_data->set("data", data);

    save_manager_api::save(save_data, Ref<Resource>(save_data->get("file_config")));
    set_status(U"Configuration enregistrée avec succès !");
    custom_logger_api::log(get_name(), InputNames::ICON_UI, String(U"Affectations sauvegardées (") + itos(actions.size()) + " actions)");
}

bool PlayerInputUI::load_saved_bindings()
{
    PlayerInput *player = get_target_player_input();
    if (save_data.is_null() || player == nullptr)
    {
        return false;
    }

    Ref<Resource> loaded = save_manager_api::load(save_data, Ref<Resource>(save_data->get("file_config")));
    if (loaded.is_null())
    {
        return false;
    }
    save_data->set("data", loaded->get("data"));
    Dictionary data = save_data->get("data");

    TypedArray<InputAction> actions = player->get_all_actions();
    for (int i = 0; i < actions.size(); i++)
    {
        InputAction *action = Object::cast_to<InputAction>(actions[i]);
        String prefix = String(action->get_action_name()) + "_";
        for (const String &property : action->get_binding_properties())
        {
            Variant value = data.get(prefix + property, Variant());
            // Le JSON ne distingue pas entiers et flottants : toutes les affectations sont des énumérations entières.
            if (value.get_type() == Variant::INT || value.get_type() == Variant::FLOAT)
            {
                action->set(property, (int64_t)value);
            }
        }
    }
    procedural_inspector_api::refresh_bindings(this);
    return true;
}

void PlayerInputUI::reset_all()
{
    PlayerInput *player = get_target_player_input();
    if (player == nullptr)
    {
        return;
    }
    TypedArray<InputAction> actions = player->get_all_actions();
    for (int i = 0; i < actions.size(); i++)
    {
        reset_action(Object::cast_to<InputAction>(actions[i]));
    }
    set_status(U"Contrôles réinitialisés aux valeurs par défaut.");
}

void PlayerInputUI::reset_action(InputAction *p_action)
{
    if (p_action == nullptr)
    {
        return;
    }
    HashMap<uint64_t, Dictionary>::ConstIterator it = defaults.find(p_action->get_instance_id());
    if (!it)
    {
        return;
    }
    // Ordre d'insertion conservé : préréglages d'abord, valeurs ensuite (cf. get_binding_properties()).
    Array properties = it->value.keys();
    for (int i = 0; i < properties.size(); i++)
    {
        p_action->set(properties[i], it->value[properties[i]]);
    }
    procedural_inspector_api::refresh_bindings(this);
}

// ─── Propriétés ─────────────────────────────────────────────────────────────────────────────────────────

void PlayerInputUI::set_target_player_input(PlayerInput *p_player) { target_player_input_id = p_player ? ObjectID(p_player->get_instance_id()) : ObjectID(); }

PackedStringArray PlayerInputUI::_get_configuration_warnings() const { return modules::missing_warnings("PlayerInputUI", REQUIRED_MODULES); }

PlayerInput *PlayerInputUI::get_target_player_input() const { return Object::cast_to<PlayerInput>(ObjectDB::get_instance(target_player_input_id)); }

void PlayerInputUI::set_rebinder(InputRebinder *p_rebinder) { rebinder_id = p_rebinder ? ObjectID(p_rebinder->get_instance_id()) : ObjectID(); }

InputRebinder *PlayerInputUI::get_rebinder() const { return Object::cast_to<InputRebinder>(ObjectDB::get_instance(rebinder_id)); }

void PlayerInputUI::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("rebuild"), &PlayerInputUI::rebuild);
    ClassDB::bind_method(D_METHOD("apply_and_save"), &PlayerInputUI::apply_and_save);
    ClassDB::bind_method(D_METHOD("reset_all"), &PlayerInputUI::reset_all);
    ClassDB::bind_method(D_METHOD("reset_action", "action"), &PlayerInputUI::reset_action);
    ClassDB::bind_method(D_METHOD("load_saved_bindings"), &PlayerInputUI::load_saved_bindings);
    ClassDB::bind_static_method("PlayerInputUI", D_METHOD("describe_slot", "action", "slot"), &PlayerInputUI::describe_slot);

    ClassDB::bind_method(D_METHOD("set_target_player_input", "player_input"), &PlayerInputUI::set_target_player_input);
    ClassDB::bind_method(D_METHOD("get_target_player_input"), &PlayerInputUI::get_target_player_input);
    ClassDB::bind_method(D_METHOD("set_rebinder", "rebinder"), &PlayerInputUI::set_rebinder);
    ClassDB::bind_method(D_METHOD("get_rebinder"), &PlayerInputUI::get_rebinder);
    ClassDB::bind_method(D_METHOD("set_save_data", "save_data"), &PlayerInputUI::set_save_data);
    ClassDB::bind_method(D_METHOD("get_save_data"), &PlayerInputUI::get_save_data);
    ClassDB::bind_method(D_METHOD("set_minimum_list_size", "size"), &PlayerInputUI::set_minimum_list_size);
    ClassDB::bind_method(D_METHOD("get_minimum_list_size"), &PlayerInputUI::get_minimum_list_size);
    ClassDB::bind_method(D_METHOD("set_show_kbm_tab", "show"), &PlayerInputUI::set_show_kbm_tab);
    ClassDB::bind_method(D_METHOD("get_show_kbm_tab"), &PlayerInputUI::get_show_kbm_tab);
    ClassDB::bind_method(D_METHOD("set_show_gamepad_tab", "show"), &PlayerInputUI::set_show_gamepad_tab);
    ClassDB::bind_method(D_METHOD("get_show_gamepad_tab"), &PlayerInputUI::get_show_gamepad_tab);

    ADD_GROUP(U"🔗 Références", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target_player_input", PROPERTY_HINT_NODE_TYPE, "PlayerInput"), "set_target_player_input", "get_target_player_input");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "rebinder", PROPERTY_HINT_NODE_TYPE, "InputRebinder"), "set_rebinder", "get_rebinder");

    ADD_GROUP(U"💾 Sauvegarde", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "save_data", PROPERTY_HINT_RESOURCE_TYPE, "SaveData"), "set_save_data", "get_save_data");

    ADD_GROUP(U"🎨 Thème & Visibilité", "");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "minimum_list_size", PROPERTY_HINT_NONE, "suffix:px"), "set_minimum_list_size", "get_minimum_list_size");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_kbm_tab"), "set_show_kbm_tab", "get_show_kbm_tab");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_gamepad_tab"), "set_show_gamepad_tab", "get_show_gamepad_tab");
}
