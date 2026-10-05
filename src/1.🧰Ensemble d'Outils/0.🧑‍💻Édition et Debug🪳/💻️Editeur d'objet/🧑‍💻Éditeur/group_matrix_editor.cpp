#include "group_matrix_editor.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_flow_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/typed_array.hpp>

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
} // namespace

void GroupMatrixEditor::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("refresh"), &GroupMatrixEditor::refresh);
    ADD_SIGNAL(MethodInfo("registry_changed"));
}

GroupMatrixEditor::GroupMatrixEditor()
{
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_h_size_flags(SIZE_EXPAND_FILL);

    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    add_child(scroll);
    list = memnew(VBoxContainer);
    list->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->add_child(list);

    add_child(memnew(HSeparator));
    HBoxContainer *footer = memnew(HBoxContainer);
    new_group_name = memnew(LineEdit);
    new_group_name->set_placeholder("Nom du nouveau groupe");
    new_group_name->set_h_size_flags(SIZE_EXPAND_FILL);
    footer->add_child(new_group_name);
    Button *create = make_button("+ Nouveau groupe");
    create->connect("pressed", callable_mp(this, &GroupMatrixEditor::_on_new_group));
    footer->add_child(create);
    add_child(footer);
}

void GroupMatrixEditor::set_registries(const Ref<GroupRegistry> &p_groups, const Ref<DataRegistry> &p_data)
{
    groups = p_groups;
    data = p_data;
    refresh();
}

Ref<GroupEntry> GroupMatrixEditor::_group(int64_t p_index) const
{
    if (groups.is_null())
    {
        return Ref<GroupEntry>();
    }
    TypedArray<GroupEntry> list_groups = groups->get_groups();
    return (p_index >= 0 && p_index < list_groups.size()) ? Ref<GroupEntry>(list_groups[p_index]) : Ref<GroupEntry>();
}

bool GroupMatrixEditor::_is_missing(const String &p_data_name) const
{
    if (data.is_null())
    {
        return false;
    }
    Ref<DataEntry> entry = data->find(p_data_name);
    if (entry.is_null())
    {
        entry = data->find(p_data_name, DataEntry::MEMBER_METHOD);
    }
    return entry.is_null() || entry->get_is_missing();
}

void GroupMatrixEditor::_emit_changed()
{
    if (groups.is_valid())
    {
        groups->emit_changed();
    }
    emit_signal("registry_changed");
}

// Reconstruction différée : le contrôle qui émet le signal ne doit pas être libéré pendant son émission.
void GroupMatrixEditor::_refresh_deferred() { callable_mp(this, &GroupMatrixEditor::refresh).call_deferred(); }

void GroupMatrixEditor::refresh()
{
    TypedArray<Node> children = list->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        list->remove_child(child);
        child->queue_free();
    }
    if (groups.is_null())
    {
        return;
    }

    TypedArray<GroupEntry> all = groups->get_groups();
    if (all.is_empty())
    {
        Label *hint = memnew(Label);
        hint->set_text(U"Aucun groupe. Les @export_group de la cible en créent automatiquement pour les nouvelles données.");
        hint->set_modulate(Color(1, 1, 1, 0.7));
        list->add_child(hint);
    }

    for (int i = 0; i < all.size(); i++)
    {
        Ref<GroupEntry> group = all[i];
        if (group.is_null())
        {
            continue;
        }
        PanelContainer *panel = memnew(PanelContainer);
        VBoxContainer *block = memnew(VBoxContainer);
        panel->add_child(block);
        list->add_child(panel);

        // Ligne 1 : ordre, nom, conteneur, colonnes, orientation, suppression.
        HBoxContainer *head = memnew(HBoxContainer);
        block->add_child(head);
        Button *up = make_button(U"▲", "Monter");
        up->set_disabled(i == 0);
        up->connect("pressed", callable_mp(this, &GroupMatrixEditor::_on_move).bind(i, i - 1));
        head->add_child(up);
        Button *down = make_button(U"▼", "Descendre");
        down->set_disabled(i == all.size() - 1);
        down->connect("pressed", callable_mp(this, &GroupMatrixEditor::_on_move).bind(i, i + 1));
        head->add_child(down);

        LineEdit *name = memnew(LineEdit);
        name->set_text(group->get_group_name());
        name->set_h_size_flags(SIZE_EXPAND_FILL);
        name->connect("text_changed", callable_mp(this, &GroupMatrixEditor::_on_name_changed).bind(i));
        head->add_child(name);

        OptionButton *type = memnew(OptionButton);
        type->add_item("VBoxContainer", GroupEntry::CONTAINER_VBOX);
        type->add_item("HBoxContainer", GroupEntry::CONTAINER_HBOX);
        type->add_item("GridContainer", GroupEntry::CONTAINER_GRID);
        type->add_item("PanelContainer", GroupEntry::CONTAINER_PANEL);
        type->select(type->get_item_index(group->get_container_type()));
        type->connect("item_selected", callable_mp(this, &GroupMatrixEditor::_on_type_selected).bind(i));
        head->add_child(type);

        SpinBox *columns = memnew(SpinBox);
        columns->set_prefix("Colonnes");
        columns->set_min(1);
        columns->set_max(12);
        columns->set_value_no_signal(group->get_columns());
        columns->set_editable(group->get_container_type() == GroupEntry::CONTAINER_GRID);
        columns->connect("value_changed", callable_mp(this, &GroupMatrixEditor::_on_columns_changed).bind(i));
        head->add_child(columns);

        CheckBox *vertical = memnew(CheckBox);
        vertical->set_text("Vertical");
        vertical->set_pressed_no_signal(group->get_is_vertical());
        vertical->set_disabled(group->get_container_type() != GroupEntry::CONTAINER_PANEL);
        vertical->connect("toggled", callable_mp(this, &GroupMatrixEditor::_on_vertical_toggled).bind(i));
        head->add_child(vertical);

        Button *remove = make_button(U"✕", "Supprimer le groupe (ses données restent affichées hors groupe)");
        remove->connect("pressed", callable_mp(this, &GroupMatrixEditor::_on_delete).bind(i));
        head->add_child(remove);

        // Ligne 2 : données du groupe (retrait) + ajout.
        HFlowContainer *members = memnew(HFlowContainer);
        block->add_child(members);
        PackedStringArray names = group->get_data_names();
        for (int n = 0; n < names.size(); n++)
        {
            bool missing = _is_missing(names[n]);
            Button *member = make_button((missing ? String(U"⚠ ") : String()) + names[n] + U"  ✕",
                                         missing ? String(U"Cette donnée n'existe pas ou plus sur la cible. Cliquer pour la retirer du groupe.") : String("Retirer du groupe"));
            member->connect("pressed", callable_mp(this, &GroupMatrixEditor::_on_remove_member).bind(names[n], i));
            members->add_child(member);
        }

        OptionButton *add = memnew(OptionButton);
        add->add_item(U"+ Ajouter une donnée…", -1);
        TypedArray<DataEntry> entries = data.is_valid() ? data->get_entries() : TypedArray<DataEntry>();
        for (int e = 0; e < entries.size(); e++)
        {
            Ref<DataEntry> entry = entries[e];
            if (entry.is_valid() && !entry->get_is_missing() && entry->get_member() != DataEntry::MEMBER_SIGNAL && !names.has(entry->get_data_name()))
            {
                Ref<GroupEntry> current = groups->group_of(entry->get_data_name());
                String label = String(entry->get_data_name()) + (current.is_valid() ? U"  (déplacée depuis « " + current->get_group_name() + U" »)" : String());
                add->add_item(label, e);
            }
        }
        add->select(0);
        add->connect("item_selected", callable_mp(this, &GroupMatrixEditor::_on_add_member).bind(i, (uint64_t)add->get_instance_id()));
        members->add_child(add);
    }
}

void GroupMatrixEditor::_on_move(int64_t p_from, int64_t p_to)
{
    TypedArray<GroupEntry> all = groups->get_groups();
    if (p_from < 0 || p_to < 0 || p_from >= all.size() || p_to >= all.size())
    {
        return;
    }
    Variant moved = all[p_from];
    all[p_from] = all[p_to];
    all[p_to] = moved;
    groups->set_groups(all);
    _refresh_deferred();
    _emit_changed();
}

void GroupMatrixEditor::_on_name_changed(const String &p_text, int64_t p_index)
{
    Ref<GroupEntry> group = _group(p_index);
    if (group.is_valid())
    {
        group->set_group_name(p_text);
        _emit_changed();
    }
}

void GroupMatrixEditor::_on_type_selected(int64_t p_item, int64_t p_index)
{
    Ref<GroupEntry> group = _group(p_index);
    if (group.is_valid())
    {
        group->set_container_type((GroupEntry::ContainerType)p_item); // ids = valeurs de l'énumération, dans l'ordre
        _refresh_deferred();
        _emit_changed();
    }
}

void GroupMatrixEditor::_on_columns_changed(double p_value, int64_t p_index)
{
    Ref<GroupEntry> group = _group(p_index);
    if (group.is_valid())
    {
        group->set_columns((int)p_value);
        _emit_changed();
    }
}

void GroupMatrixEditor::_on_vertical_toggled(bool p_on, int64_t p_index)
{
    Ref<GroupEntry> group = _group(p_index);
    if (group.is_valid())
    {
        group->set_is_vertical(p_on);
        _emit_changed();
    }
}

void GroupMatrixEditor::_on_delete(int64_t p_index)
{
    TypedArray<GroupEntry> all = groups->get_groups();
    if (p_index < 0 || p_index >= all.size())
    {
        return;
    }
    all.remove_at(p_index);
    groups->set_groups(all);
    _refresh_deferred();
    _emit_changed();
}

void GroupMatrixEditor::_on_remove_member(const String &p_name, int64_t p_index)
{
    Ref<GroupEntry> group = _group(p_index);
    if (group.is_null())
    {
        return;
    }
    PackedStringArray names = group->get_data_names();
    while (names.has(p_name))
    {
        names.remove_at(names.find(p_name));
    }
    group->set_data_names(names);
    _refresh_deferred();
    _emit_changed();
}

void GroupMatrixEditor::_on_add_member(int64_t p_item, int64_t p_index, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    Ref<GroupEntry> group = _group(p_index);
    if (!option || group.is_null() || data.is_null())
    {
        return;
    }
    int64_t entry_index = option->get_item_id(p_item);
    TypedArray<DataEntry> entries = data->get_entries();
    if (entry_index < 0 || entry_index >= entries.size())
    {
        return;
    }
    Ref<DataEntry> entry = entries[entry_index];
    groups->assign(entry->get_data_name(), group); // retirée de son ancien groupe : une donnée, un groupe
    _refresh_deferred();
    _emit_changed();
}

void GroupMatrixEditor::_on_new_group()
{
    String name = new_group_name->get_text().strip_edges();
    if (name.is_empty() || groups.is_null())
    {
        return;
    }
    groups->ensure(name);
    new_group_name->clear();
    _refresh_deferred();
    _emit_changed();
}
