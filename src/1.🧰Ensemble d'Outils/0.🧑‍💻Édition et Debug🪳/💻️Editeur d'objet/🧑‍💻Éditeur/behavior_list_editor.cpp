#include "behavior_list_editor.hpp"
#include "editor_undo_backend.hpp"
#include "icon_picker.hpp"
#include "interface_style_editor.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/type_utils.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/container_widgets.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧱Construction/procedural_editor.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_flow_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

namespace
{
Button *make_button(const String &p_text, const String &p_tooltip = String())
{
    Button *button = memnew(Button);
    button->set_text(p_text);
    button->set_tooltip_text(p_tooltip);
    return button;
}

Label *make_label(const String &p_text, float p_alpha = 1.0f)
{
    Label *label = memnew(Label);
    label->set_text(p_text);
    label->set_modulate(Color(1, 1, 1, p_alpha));
    return label;
}

// Types proposés par « + Comportement » : fournis puis class_name héritant de WidgetBehavior (dans l'ordre des ids).
PackedStringArray addable_types()
{
    PackedStringArray types;
    Dictionary builtins = BehaviorListEditor::builtin_types();
    Array keys = builtins.keys();
    for (int i = 0; i < keys.size(); i++)
    {
        types.push_back(keys[i]);
    }
    TypedArray<Dictionary> globals = ProjectSettings::get_singleton()->get_global_class_list();
    for (int i = 0; i < globals.size(); i++)
    {
        String name = Dictionary(globals[i]).get("class", "");
        if (TypeUtils::class_inherits(name, "WidgetBehavior"))
        {
            types.push_back(name);
        }
    }
    return types;
}
} // namespace

void BehaviorListEditor::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("refresh"), &BehaviorListEditor::refresh);
    ADD_SIGNAL(MethodInfo("registry_changed"));
}

Dictionary BehaviorListEditor::builtin_types()
{
    Dictionary types;
    types["ShowIfBehavior"] = "Montrer si";
    types["HideIfBehavior"] = "Cacher si";
    types["EnableIfBehavior"] = "Activer si";
    types["StyleIfBehavior"] = "Style si";
    types["TooltipIfBehavior"] = "Infobulle si";
    types["IconIfBehavior"] = String(U"Icône si");
    return types;
}

BehaviorListEditor::BehaviorListEditor()
{
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_h_size_flags(SIZE_EXPAND_FILL);
    backend = Ref<UndoBackend>(memnew(EditorUndoBackend));

    MenuButton *add = memnew(MenuButton);
    add->set_text("+ Comportement");
    add->set_flat(false);
    add->set_h_size_flags(SIZE_SHRINK_BEGIN);
    add->connect("about_to_popup", callable_mp(this, &BehaviorListEditor::_on_child_changed)); // types de script à jour
    add->get_popup()->connect("id_pressed", callable_mp(this, &BehaviorListEditor::_on_add_id).bind((uint64_t)add->get_instance_id()));
    add_child(add);

    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    add_child(scroll);
    list = memnew(VBoxContainer);
    list->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->add_child(list);
}

void BehaviorListEditor::set_registries(const Ref<DataRegistry> &p_registry, const Ref<GroupRegistry> &p_groups)
{
    registry = p_registry;
    groups = p_groups;
    refresh();
}

void BehaviorListEditor::set_context_object(Object *p_object)
{
    context_id = p_object ? ObjectID(p_object->get_instance_id()) : ObjectID();
    _queue_refresh();
}

void BehaviorListEditor::set_context_type(const String &p_type)
{
    context_type = p_type;
    _queue_refresh();
}

Ref<WidgetBehavior> BehaviorListEditor::_behavior(int64_t p_index) const
{
    TypedArray<WidgetBehavior> behaviors = registry.is_valid() ? registry->get_behaviors() : TypedArray<WidgetBehavior>();
    return (p_index >= 0 && p_index < behaviors.size()) ? Ref<WidgetBehavior>(behaviors[p_index]) : Ref<WidgetBehavior>();
}

void BehaviorListEditor::_queue_refresh()
{
    if (!refresh_queued)
    {
        refresh_queued = true;
        callable_mp(this, &BehaviorListEditor::refresh).call_deferred();
    }
}

void BehaviorListEditor::_commit(const String &p_name, const Array &p_do, const Array &p_undo)
{
    backend->commit_calls(p_name, p_do, p_undo);
    emit_signal("registry_changed");
    _queue_refresh();
}

void BehaviorListEditor::_commit_set(Object *p_object, const StringName &p_property, const Variant &p_value)
{
    _commit(U"Comportements : " + String(p_property), Array::make(Array::make(p_object, "set", p_property, p_value)),
            Array::make(Array::make(p_object, "set", p_property, p_object->get(p_property))));
}

void BehaviorListEditor::_set_behaviors(const TypedArray<WidgetBehavior> &p_behaviors, const String &p_name)
{
    _commit(p_name, Array::make(Array::make(registry, "set", "behaviors", p_behaviors)), Array::make(Array::make(registry, "set", "behaviors", registry->get_behaviors())));
}

Object *BehaviorListEditor::_resolve_context(Variant &r_sample) const
{
    if (Object *context = ObjectDB::get_instance(context_id))
    {
        return context;
    }
    if (!context_type.is_empty())
    {
        r_sample = ObjectWidget::instantiate_class(context_type);
    }
    return r_sample.get_validated_object();
}

void BehaviorListEditor::_on_child_changed()
{
    // Menu « + Comportement » reconstruit à l'ouverture (nouveaux class_name de script pris en compte).
    TypedArray<Node> children = get_children();
    for (int i = 0; i < children.size(); i++)
    {
        MenuButton *menu = Object::cast_to<MenuButton>(children[i]);
        if (!menu)
        {
            continue;
        }
        PopupMenu *popup = menu->get_popup();
        popup->clear();
        Dictionary builtins = builtin_types();
        PackedStringArray types = addable_types();
        for (int t = 0; t < types.size(); t++)
        {
            popup->add_item(builtins.has(types[t]) ? String(builtins[types[t]]) : types[t] + U" (script)", t);
        }
    }
}

void BehaviorListEditor::_on_add_id(int64_t p_id, uint64_t p_menu_id)
{
    (void)p_menu_id;
    PackedStringArray types = addable_types();
    if (registry.is_null() || p_id < 0 || p_id >= types.size())
    {
        return;
    }
    Variant created = ObjectWidget::instantiate_class(types[p_id]);
    Ref<WidgetBehavior> behavior = created;
    if (behavior.is_null())
    {
        return;
    }
    TypedArray<WidgetBehavior> behaviors = registry->get_behaviors().duplicate();
    behaviors.push_back(behavior);
    _set_behaviors(behaviors, U"Comportements : ajout");
}

void BehaviorListEditor::_on_move(int64_t p_from, int64_t p_to)
{
    TypedArray<WidgetBehavior> behaviors = registry->get_behaviors().duplicate();
    if (p_from < 0 || p_to < 0 || p_from >= behaviors.size() || p_to >= behaviors.size())
    {
        return;
    }
    Variant moved = behaviors[p_from];
    behaviors[p_from] = behaviors[p_to];
    behaviors[p_to] = moved;
    _set_behaviors(behaviors, U"Comportements : ordre");
}

void BehaviorListEditor::_on_remove(int64_t p_index)
{
    TypedArray<WidgetBehavior> behaviors = registry->get_behaviors().duplicate();
    if (p_index < 0 || p_index >= behaviors.size())
    {
        return;
    }
    behaviors.remove_at(p_index);
    _set_behaviors(behaviors, U"Comportements : suppression");
}

void BehaviorListEditor::_on_toggle(bool p_on, int64_t p_index, const String &p_property)
{
    if (Ref<WidgetBehavior> behavior = _behavior(p_index); behavior.is_valid())
    {
        _commit_set(behavior.ptr(), p_property, p_on);
    }
}

void BehaviorListEditor::_on_source_selected(int64_t p_item, int64_t p_index, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    Ref<WidgetBehavior> behavior = _behavior(p_index);
    if (option && behavior.is_valid())
    {
        _commit_set(behavior->get_condition().ptr(), "source", option->get_item_id(p_item));
    }
}

void BehaviorListEditor::_on_member_selected(int64_t p_item, int64_t p_index, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    Ref<WidgetBehavior> behavior = _behavior(p_index);
    if (!option || behavior.is_null())
    {
        return;
    }
    String name = option->get_item_metadata(p_item);
    if (name.is_empty())
    {
        return;
    }
    Ref<ArgumentBinding> condition = behavior->get_condition();
    if (condition->get_source() == ArgumentBinding::SOURCE_METHOD)
    {
        _commit_set(condition.ptr(), "method", StringName(name));
    }
    else
    {
        _commit_set(condition.ptr(), "property_path", NodePath(name));
    }
}

void BehaviorListEditor::_on_target_added(int64_t p_item, int64_t p_index, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    Ref<WidgetBehavior> behavior = _behavior(p_index);
    if (!option || behavior.is_null())
    {
        return;
    }
    String name = option->get_item_metadata(p_item);
    PackedStringArray targets = behavior->get_targets();
    if (name.is_empty() || targets.has(name))
    {
        return;
    }
    targets.push_back(name);
    _commit_set(behavior.ptr(), "targets", targets);
}

void BehaviorListEditor::_on_target_removed(const String &p_name, int64_t p_index)
{
    Ref<WidgetBehavior> behavior = _behavior(p_index);
    if (behavior.is_null())
    {
        return;
    }
    PackedStringArray targets = behavior->get_targets();
    targets.erase(p_name);
    _commit_set(behavior.ptr(), "targets", targets);
}

void BehaviorListEditor::_on_tooltip_committed(int64_t p_index, uint64_t p_edit_id)
{
    TextEdit *edit = Object::cast_to<TextEdit>(ObjectDB::get_instance(p_edit_id));
    Ref<TooltipIfBehavior> behavior = _behavior(p_index);
    if (edit && behavior.is_valid() && behavior->get_tooltip() != edit->get_text())
    {
        _commit_set(behavior.ptr(), "tooltip", edit->get_text());
    }
}

void BehaviorListEditor::refresh()
{
    refresh_queued = false;
    TypedArray<Node> children = list->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        list->remove_child(child);
        child->queue_free();
    }
    _on_child_changed();
    if (registry.is_null())
    {
        return;
    }
    TypedArray<WidgetBehavior> behaviors = registry->get_behaviors();
    if (behaviors.is_empty())
    {
        list->add_child(make_label(U"Aucun comportement : « + Comportement » (Montrer si, Cacher si, Activer si, Style si, Infobulle si, Icône si...).", 0.6f));
        return;
    }
    Variant sample;
    Object *context = _resolve_context(sample);
    Dictionary bool_info;
    bool_info["type"] = Variant::BOOL;

    for (int i = 0; i < behaviors.size(); i++)
    {
        Ref<WidgetBehavior> behavior = behaviors[i];
        if (behavior.is_null())
        {
            continue;
        }
        PanelContainer *panel = memnew(PanelContainer);
        VBoxContainer *box = memnew(VBoxContainer);
        panel->add_child(box);
        list->add_child(panel);

        // Ligne 1 : activé, type, inversion, ordre, suppression.
        HBoxContainer *head = memnew(HBoxContainer);
        box->add_child(head);
        CheckBox *enabled = memnew(CheckBox);
        enabled->set_pressed_no_signal(behavior->get_enabled());
        enabled->set_tooltip_text(U"Comportement actif");
        enabled->connect("toggled", callable_mp(this, &BehaviorListEditor::_on_toggle).bind(i, String("enabled")));
        head->add_child(enabled);
        Label *type = make_label(behavior->get_display_name());
        type->set_h_size_flags(SIZE_EXPAND_FILL);
        head->add_child(type);
        CheckBox *invert = memnew(CheckBox);
        invert->set_text("Inverser");
        invert->set_tooltip_text(U"La condition est considérée vraie quand la valeur est fausse");
        invert->set_pressed_no_signal(behavior->get_invert());
        invert->connect("toggled", callable_mp(this, &BehaviorListEditor::_on_toggle).bind(i, String("invert")));
        head->add_child(invert);
        Button *up = make_button(U"▲", "Monter");
        up->set_disabled(i == 0);
        up->connect("pressed", callable_mp(this, &BehaviorListEditor::_on_move).bind(i, i - 1));
        head->add_child(up);
        Button *down = make_button(U"▼", "Descendre");
        down->set_disabled(i == behaviors.size() - 1);
        down->connect("pressed", callable_mp(this, &BehaviorListEditor::_on_move).bind(i, i + 1));
        head->add_child(down);
        Button *remove = make_button(U"✕", "Supprimer le comportement");
        remove->connect("pressed", callable_mp(this, &BehaviorListEditor::_on_remove).bind(i));
        head->add_child(remove);

        // Ligne 2 : condition (propriété ou fonction booléenne).
        Ref<ArgumentBinding> condition = behavior->get_condition();
        bool method = condition->get_source() == ArgumentBinding::SOURCE_METHOD;
        HBoxContainer *condition_row = memnew(HBoxContainer);
        box->add_child(condition_row);
        condition_row->add_child(make_label("Condition"));
        OptionButton *source = memnew(OptionButton);
        source->add_item(U"Propriété", ArgumentBinding::SOURCE_PROPERTY);
        source->add_item("Fonction", ArgumentBinding::SOURCE_METHOD);
        source->select(method ? 1 : 0);
        source->connect("item_selected", callable_mp(this, &BehaviorListEditor::_on_source_selected).bind(i, (uint64_t)source->get_instance_id()));
        condition_row->add_child(source);
        Node *context_node = Object::cast_to<Node>(context);
        Object *source_object = condition->get_object_resource().is_valid()             ? (Object *)condition->get_object_resource().ptr()
                                : condition->get_object_path().is_empty()               ? context
                                : (context_node && context_node->is_inside_tree()) ? (Object *)context_node->get_node_or_null(condition->get_object_path())
                                                                                   : nullptr;
        OptionButton *member = memnew(OptionButton);
        member->set_fit_to_longest_item(false);
        member->set_custom_minimum_size(Vector2(200, 0));
        member->add_item(method ? U"‹ fonction booléenne ›" : U"‹ propriété booléenne ›");
        member->set_item_metadata(0, "");
        String selected_name = method ? String(condition->get_method()) : String(condition->get_property_path());
        TypedArray<Dictionary> members = ArgumentBinding::compatible_members(source_object, method ? ArgumentBinding::SOURCE_METHOD : ArgumentBinding::SOURCE_PROPERTY, bool_info);
        int added = 0;
        for (int m = 0; m < members.size(); m++)
        {
            Dictionary info = members[m];
            if ((int)info.get("type", 0) != Variant::BOOL) // booléens uniquement (pas les Variant)
            {
                continue;
            }
            String member_name = info.get("name", "");
            String suffix = (method && !bool(info.get("const", false))) ? String(U"  ⚠") : String();
            member->add_item(member_name + (method ? String("()") : String()) + suffix);
            member->set_item_metadata(++added, member_name);
            if (member_name == selected_name)
            {
                member->select(added);
            }
        }
        member->set_disabled(added == 0);
        member->set_tooltip_text(!source_object ? String(U"Objet introuvable : saisir le nom à la main") : U"Membres booléens de " + source_object->get_class());
        member->connect("item_selected", callable_mp(this, &BehaviorListEditor::_on_member_selected).bind(i, (uint64_t)member->get_instance_id()));
        condition_row->add_child(member);
        // Saisie libre : objet (chemin) + nom.
        Ref<DataRegistry> condition_registry;
        condition_registry.instantiate();
        condition_registry->set_scope(DataEntry::SCOPE_CLASS_ONLY);
        condition_registry->sync(condition.ptr());
        TypedArray<DataEntry> condition_entries = condition_registry->get_entries();
        for (int e = 0; e < condition_entries.size(); e++)
        {
            Ref<DataEntry> entry = condition_entries[e];
            String field = entry->get_data_name();
            entry->set_enabled(field == "object_path" || field == (method ? "method" : "property_path"));
        }
        ProceduralEditor *condition_editor = memnew(ProceduralEditor);
        condition_editor->set_scroll_enabled(false);
        condition_editor->set_styles_enabled(false);
        condition_editor->set_use_layouts(false);
        condition_editor->set_h_size_flags(SIZE_EXPAND_FILL);
        condition_editor->set_undo_backend(backend);
        condition_editor->set_registry(condition_registry);
        condition_editor->set_target(condition.ptr());
        condition_row->add_child(condition_editor);
        if (method && condition->get_method() != StringName() && (!source_object || !ArgumentBinding::is_method_const(source_object, condition->get_method())))
        {
            Label *warning = make_label(U"⚠ « " + String(condition->get_method()) +
                                        U"() » n'est pas déclarée const (ou c'est une méthode de script) : elle est appelée à chaque relecture, attention aux effets de bord.");
            warning->set_modulate(Color(1, 0.75, 0.3));
            warning->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
            box->add_child(warning);
        }

        // Ligne 3 : cibles (données, groupes).
        HFlowContainer *targets_row = memnew(HFlowContainer);
        box->add_child(targets_row);
        targets_row->add_child(make_label("Cibles"));
        PackedStringArray targets = behavior->get_targets();
        for (const String &target_name : targets)
        {
            bool known = registry->find(target_name).is_valid() || registry->find(target_name, DataEntry::MEMBER_METHOD).is_valid() ||
                         registry->find(target_name, DataEntry::MEMBER_SIGNAL).is_valid() || registry->find(target_name, DataEntry::MEMBER_PREVIEW).is_valid() ||
                         (groups.is_valid() && groups->find(target_name).is_valid());
            Button *chip = make_button((known ? String() : String(U"⚠ ")) + target_name + U"  ✕", known ? String("Retirer la cible") : String(U"Aucune donnée ni aucun groupe de ce nom"));
            chip->connect("pressed", callable_mp(this, &BehaviorListEditor::_on_target_removed).bind(target_name, i));
            targets_row->add_child(chip);
        }
        OptionButton *add_target = memnew(OptionButton);
        add_target->add_item("+ Cible");
        add_target->set_item_metadata(0, "");
        int target_items = 0;
        TypedArray<DataEntry> entries = registry->get_entries();
        for (int e = 0; e < entries.size(); e++)
        {
            Ref<DataEntry> entry = entries[e];
            if (entry.is_valid() && !entry->get_is_missing() && !targets.has(String(entry->get_data_name())))
            {
                add_target->add_item(String(entry->get_data_name()));
                add_target->set_item_metadata(++target_items, String(entry->get_data_name()));
            }
        }
        TypedArray<GroupEntry> all_groups = groups.is_valid() ? groups->get_groups() : TypedArray<GroupEntry>();
        for (int g = 0; g < all_groups.size(); g++)
        {
            Ref<GroupEntry> group = all_groups[g];
            if (group.is_valid() && !targets.has(group->get_group_name()))
            {
                add_target->add_item("Groupe : " + group->get_group_name());
                add_target->set_item_metadata(++target_items, group->get_group_name());
            }
        }
        add_target->connect("item_selected", callable_mp(this, &BehaviorListEditor::_on_target_added).bind(i, (uint64_t)add_target->get_instance_id()));
        targets_row->add_child(add_target);

        // Réglages propres au type de comportement.
        if (Ref<StyleIfBehavior> style_if = behavior; style_if.is_valid())
        {
            InterfaceStyleEditor *style_editor = memnew(InterfaceStyleEditor);
            style_editor->set_context_object(context_id.is_valid() ? ObjectDB::get_instance(context_id) : nullptr);
            style_editor->set_context_type(context_type);
            style_editor->set_owner(style_if.ptr(), "style");
            style_editor->connect("style_changed", Callable(this, "emit_signal").bind("registry_changed"));
            box->add_child(style_editor);
        }
        else if (Ref<TooltipIfBehavior> tooltip_if = behavior; tooltip_if.is_valid())
        {
            box->add_child(make_label(U"Infobulle (BBCode ; {propriété}, {fonction()}) — ajoutée à celle de la donnée", 0.8f));
            TextEdit *edit = memnew(TextEdit);
            edit->set_text(tooltip_if->get_tooltip());
            edit->set_custom_minimum_size(Vector2(0, 60));
            edit->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
            edit->connect("focus_exited", callable_mp(this, &BehaviorListEditor::_on_tooltip_committed).bind(i, (uint64_t)edit->get_instance_id()));
            box->add_child(edit);
            RichTextLabel *preview = memnew(RichTextLabel);
            preview->set_use_bbcode(true);
            preview->set_fit_content(true);
            preview->set_text(WidgetBehavior::format_text(tooltip_if->get_tooltip(), context));
            box->add_child(preview);
        }
        else if (Ref<IconIfBehavior> icon_if = behavior; icon_if.is_valid())
        {
            IconPickerRow *icon_row = memnew(IconPickerRow);
            icon_row->setup(icon_if.ptr(), "icon", U"Icône");
            icon_row->connect("icon_changed", Callable(this, "emit_signal").bind("registry_changed"));
            box->add_child(icon_row);
        }
        else if (behavior->get_script().get_type() != Variant::NIL)
        {
            // Comportement de script : ses propriétés exportées.
            Ref<DataRegistry> script_registry;
            script_registry.instantiate();
            script_registry->set_scope(DataEntry::SCOPE_CLASS_ONLY);
            script_registry->sync(behavior.ptr());
            ProceduralEditor *script_editor = memnew(ProceduralEditor);
            script_editor->set_scroll_enabled(false);
            script_editor->set_styles_enabled(false);
            script_editor->set_use_layouts(false);
            script_editor->set_undo_backend(backend);
            script_editor->set_registry(script_registry);
            script_editor->set_target(behavior.ptr());
            box->add_child(script_editor);
        }
    }
    if (Object *sample_object = sample.get_validated_object(); sample_object && !Object::cast_to<RefCounted>(sample_object))
    {
        memdelete(sample_object);
    }
}
