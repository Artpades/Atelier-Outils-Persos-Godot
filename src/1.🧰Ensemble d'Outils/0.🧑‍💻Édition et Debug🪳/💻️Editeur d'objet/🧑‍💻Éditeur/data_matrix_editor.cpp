#include "data_matrix_editor.hpp"
#include "editor_undo_backend.hpp"
#include "interface_style_editor.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/widget_table.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧱Construction/procedural_editor.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/editor_resource_picker.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

namespace
{
const char32_t *const HEADERS[] = {U"U", U"Donnée", U"Type", U"Widget", U"Classe Custom", U"Vertical", U"AffichéU", U"Réglages", U"U"};
const int COLUMNS = sizeof(HEADERS) / sizeof(HEADERS[0]);

Label *make_label(const String &p_text, float p_alpha = 1.0f)
{
    Label *label = memnew(Label);
    label->set_text(p_text);
    label->set_modulate(Color(1, 1, 1, p_alpha));
    return label;
}
} // namespace

void DataMatrixEditor::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_registry", "registry"), &DataMatrixEditor::set_registry);
    ClassDB::bind_method(D_METHOD("get_registry"), &DataMatrixEditor::get_registry);
    ClassDB::bind_method(D_METHOD("refresh"), &DataMatrixEditor::refresh);

    ADD_SIGNAL(MethodInfo("registry_changed"));
}

DataMatrixEditor::DataMatrixEditor()
{
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_h_size_flags(SIZE_EXPAND_FILL);

    // Les entrées manquantes ne sont jamais supprimées automatiquement (leur configuration est conservée) :
    // ce bouton les retire toutes, par exemple après avoir réduit le périmètre du scan.
    Button *remove_missing = memnew(Button);
    remove_missing->set_text(U"Retirer les entrées manquantes (⚠)");
    remove_missing->set_h_size_flags(SIZE_SHRINK_END);
    remove_missing->connect("pressed", callable_mp(this, &DataMatrixEditor::_on_remove_all_missing));
    add_child(remove_missing);

    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    scroll->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(scroll);

    grid = memnew(GridContainer);
    grid->set_columns(COLUMNS);
    grid->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->add_child(grid);

    add_child(memnew(HSeparator));

    settings_panel = memnew(VBoxContainer);
    settings_panel->set_custom_minimum_size(Vector2(0, 220));
    add_child(settings_panel);
}

void DataMatrixEditor::_notification(int p_what)
{
    if (p_what == NOTIFICATION_PREDELETE)
    {
        _watch_settings(Ref<WidgetSettings>());
    }
}

void DataMatrixEditor::set_registry(const Ref<DataRegistry> &p_registry)
{
    registry = p_registry;
    selected = -1;
    refresh();
}

Ref<DataRegistry> DataMatrixEditor::get_registry() const { return registry; }

void DataMatrixEditor::set_context_object(Object *p_object) { context_id = p_object ? ObjectID(p_object->get_instance_id()) : ObjectID(); }
void DataMatrixEditor::set_context_type(const String &p_type) { context_type = p_type; }

Ref<DataEntry> DataMatrixEditor::_entry(int64_t p_index) const
{
    if (registry.is_null())
    {
        return Ref<DataEntry>();
    }
    TypedArray<DataEntry> entries = registry->get_entries();
    return (p_index >= 0 && p_index < entries.size()) ? Ref<DataEntry>(entries[p_index]) : Ref<DataEntry>();
}

void DataMatrixEditor::_emit_changed()
{
    if (registry.is_valid())
    {
        registry->emit_changed();
    }
    emit_signal("registry_changed");
}

void DataMatrixEditor::_clear(Node *p_container)
{
    TypedArray<Node> children = p_container->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        if (child)
        {
            p_container->remove_child(child);
            child->queue_free();
        }
    }
}

void DataMatrixEditor::refresh()
{
    _clear(grid);
    for (int c = 0; c < COLUMNS; c++)
    {
        grid->add_child(make_label(HEADERS[c], 0.7f));
    }

    TypedArray<DataEntry> entries = registry.is_valid() ? registry->get_entries() : TypedArray<DataEntry>();
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<DataEntry> entry = entries[i];
        if (entry.is_null())
        {
            continue;
        }
        DataEntry::WidgetChoice choice = entry->get_widget_choice();

        // 0. Ordre d'affichage
        HBoxContainer *order = memnew(HBoxContainer);
        Button *up = memnew(Button);
        up->set_text(U"▲");
        up->set_disabled(i == 0);
        up->connect("pressed", callable_mp(this, &DataMatrixEditor::_on_move).bind(i, i - 1));
        Button *down = memnew(Button);
        down->set_text(U"▼");
        down->set_disabled(i == entries.size() - 1);
        down->connect("pressed", callable_mp(this, &DataMatrixEditor::_on_move).bind(i, i + 1));
        order->add_child(up);
        order->add_child(down);
        grid->add_child(order);

        // 1. Nom (sélectionne la ligne pour éditer ses réglages)
        Button *name = memnew(Button);
        name->set_text((entry->get_is_missing() ? String(U"⚠ ") : String()) + String(entry->get_data_name()));
        name->set_tooltip_text(entry->get_is_missing() ? String("Ce membre n'existe plus sur la cible.") : String(entry->get_data_name()));
        name->set_flat(i != selected);
        name->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
        name->set_custom_minimum_size(Vector2(160, 0));
        name->connect("pressed", callable_mp(this, &DataMatrixEditor::_on_select).bind(i));
        grid->add_child(name);

        // 2. Membre · type
        grid->add_child(make_label(WidgetTable::member_name(entry->get_member()) + U" · " + WidgetTable::kind_name(entry->get_kind()), 0.7f));

        // 3. Widget : AUTO + choix autorisés pour le type
        OptionButton *option = memnew(OptionButton);
        option->set_custom_minimum_size(Vector2(190, 0));
        option->add_item("Auto (" + WidgetTable::choice_name(WidgetTable::default_for(entry->get_kind())) + ")", DataEntry::WIDGET_AUTO);
        WidgetTable::Choices choices = WidgetTable::allowed_for(entry->get_kind());
        for (int c = 0; c < choices.size; c++)
        {
            option->add_item(WidgetTable::choice_name(choices.data[c]), choices.data[c]);
        }
        int item = option->get_item_index(choice);
        option->select(item >= 0 ? item : 0);
        option->connect("item_selected", callable_mp(this, &DataMatrixEditor::_on_choice_selected).bind(i, (uint64_t)option->get_instance_id()));
        grid->add_child(option);

        // 4. Classe du widget Custom (native ou class_name héritant de PropertyWidget)
        LineEdit *custom = memnew(LineEdit);
        custom->set_text(entry->get_custom_widget_class());
        custom->set_placeholder(U"Classe héritant de PropertyWidget");
        custom->set_editable(choice == DataEntry::WIDGET_CUSTOM);
        custom->set_custom_minimum_size(Vector2(180, 0));
        custom->connect("text_changed", callable_mp(this, &DataMatrixEditor::_on_custom_class_changed).bind(i));
        grid->add_child(custom);

        // 5-6. Vertical / Affiché
        CheckBox *vertical = memnew(CheckBox);
        vertical->set_pressed_no_signal(entry->get_is_vertical());
        vertical->connect("toggled", callable_mp(this, &DataMatrixEditor::_on_vertical_toggled).bind(i));
        grid->add_child(vertical);
        CheckBox *enabled = memnew(CheckBox);
        enabled->set_pressed_no_signal(entry->get_enabled());
        enabled->connect("toggled", callable_mp(this, &DataMatrixEditor::_on_enabled_toggled).bind(i));
        grid->add_child(enabled);

        // 7. Réglages (résumé ; édition sous le tableau)
        Ref<WidgetSettings> settings = entry->get_settings();
        grid->add_child(make_label(settings.is_valid() ? settings->get_class() : String(U"—"), 0.7f));

        // 8. Suppression (entrées manquantes uniquement : les autres sont recréées à la synchronisation)
        if (entry->get_is_missing())
        {
            Button *remove = memnew(Button);
            remove->set_text(U"✕");
            remove->set_tooltip_text(U"Supprimer cette entrée (membre disparu)");
            remove->connect("pressed", callable_mp(this, &DataMatrixEditor::_on_remove).bind(i));
            grid->add_child(remove);
        }
        else
        {
            grid->add_child(memnew(Control));
        }
    }

    _refresh_settings_panel();
}

void DataMatrixEditor::_watch_settings(const Ref<WidgetSettings> &p_settings)
{
    Callable callback = callable_mp(this, &DataMatrixEditor::_on_settings_edited);
    if (Object *previous = ObjectDB::get_instance(watched_settings_id))
    {
        if (previous->is_connected("changed", callback))
        {
            previous->disconnect("changed", callback);
        }
    }
    watched_settings_id = ObjectID();
    if (p_settings.is_valid())
    {
        p_settings->connect("changed", callback);
        watched_settings_id = ObjectID(p_settings->get_instance_id());
    }
}

void DataMatrixEditor::_refresh_settings_panel()
{
    _clear(settings_panel);
    Ref<DataEntry> entry = _entry(selected);
    if (entry.is_null())
    {
        _watch_settings(Ref<WidgetSettings>());
        settings_panel->add_child(make_label(U"Cliquer sur le nom d'une donnée pour éditer ses réglages.", 0.7f));
        return;
    }

    DataEntry::WidgetChoice resolved = entry->resolved_choice();
    StringName expected = WidgetTable::settings_class_for(resolved);
    settings_panel->add_child(make_label(U"Réglages de « " + String(entry->get_data_name()) + U" » — widget " + WidgetTable::choice_name(resolved) + U", réglages lus : " + String(expected)));

    Ref<WidgetSettings> settings = entry->get_settings();
    EditorResourcePicker *picker = memnew(EditorResourcePicker);
    picker->set_base_type(expected);
    picker->set_edited_resource(settings);
    picker->connect("resource_changed", callable_mp(this, &DataMatrixEditor::_on_settings_resource_changed).bind(selected));
    settings_panel->add_child(picker);

    _watch_settings(settings);
    if (settings.is_null())
    {
        return;
    }
    if (!settings->is_class(expected) && settings->get_class() != StringName("WidgetSettings"))
    {
        settings_panel->add_child(make_label(U"⚠ " + settings->get_class() + U" n'est pas lue par ce widget : seuls les réglages communs s'appliquent."));
    }

    // Édition des réglages avec le ProceduralEditor lui-même : seules les propriétés déclarées par
    // WidgetSettings et ses sous-classes (pas celles de Resource).
    Dictionary own;
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    for (StringName cls = settings->get_class(); !String(cls).is_empty(); cls = cdb->get_parent_class(cls))
    {
        TypedArray<Dictionary> list = cdb->class_get_property_list(cls, true);
        for (int i = 0; i < list.size(); i++)
        {
            own[Dictionary(list[i]).get("name", "")] = true;
        }
        if (cls == StringName("WidgetSettings"))
        {
            break;
        }
    }
    Ref<DataRegistry> settings_registry;
    settings_registry.instantiate();
    settings_registry->set_scope(DataEntry::SCOPE_ALL);
    settings_registry->sync(settings.ptr());
    TypedArray<DataEntry> settings_entries = settings_registry->get_entries();
    for (int i = 0; i < settings_entries.size(); i++)
    {
        Ref<DataEntry> settings_entry = settings_entries[i];
        // Le style a son propre éditeur (ci-dessous).
        settings_entry->set_enabled(own.has(String(settings_entry->get_data_name())) && settings_entry->get_data_name() != StringName("style"));
    }

    ProceduralEditor *settings_editor = memnew(ProceduralEditor);
    settings_editor->set_v_size_flags(SIZE_EXPAND_FILL);
    settings_editor->set_undo_backend(Ref<UndoBackend>(memnew(EditorUndoBackend)));
    settings_editor->set_styles_enabled(false);
    settings_editor->set_use_layouts(false);
    settings_editor->set_registry(settings_registry);
    settings_editor->set_target(settings.ptr());
    settings_panel->add_child(settings_editor);

    // Style du widget (thème, fond, shader et uniforms liés), sur la partie choisie par style_target.
    InterfaceStyleEditor *style_editor = memnew(InterfaceStyleEditor);
    style_editor->set_context_object(ObjectDB::get_instance(context_id));
    style_editor->set_context_type(context_type);
    style_editor->set_owner(settings.ptr(), "style");
    style_editor->connect("style_changed", callable_mp(this, &DataMatrixEditor::_emit_changed));
    settings_panel->add_child(style_editor);
}

void DataMatrixEditor::_on_move(int64_t p_from, int64_t p_to)
{
    TypedArray<DataEntry> entries = registry->get_entries();
    if (p_from < 0 || p_to < 0 || p_from >= entries.size() || p_to >= entries.size())
    {
        return;
    }
    Variant moved = entries[p_from];
    entries[p_from] = entries[p_to];
    entries[p_to] = moved;
    registry->set_entries(entries);
    if (selected == p_from)
    {
        selected = p_to;
    }
    else if (selected == p_to)
    {
        selected = p_from;
    }
    callable_mp(this, &DataMatrixEditor::refresh).call_deferred();
    _emit_changed();
}

void DataMatrixEditor::_on_select(int64_t p_index)
{
    selected = p_index;
    callable_mp(this, &DataMatrixEditor::refresh).call_deferred();
}

void DataMatrixEditor::_on_choice_selected(int64_t p_item, int64_t p_index, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    Ref<DataEntry> entry = _entry(p_index);
    if (!option || entry.is_null())
    {
        return;
    }
    entry->set_widget_choice((DataEntry::WidgetChoice)option->get_item_id(p_item));
    selected = p_index; // affiche les réglages correspondant au nouveau widget
    // Reconstruction différée : le bouton qui émet ce signal ne doit pas être libéré pendant son émission.
    callable_mp(this, &DataMatrixEditor::refresh).call_deferred();
    _emit_changed();
}

void DataMatrixEditor::_on_custom_class_changed(const String &p_text, int64_t p_index)
{
    Ref<DataEntry> entry = _entry(p_index);
    if (entry.is_valid())
    {
        entry->set_custom_widget_class(p_text.strip_edges());
        _emit_changed();
    }
}

void DataMatrixEditor::_on_vertical_toggled(bool p_on, int64_t p_index)
{
    Ref<DataEntry> entry = _entry(p_index);
    if (entry.is_valid())
    {
        entry->set_is_vertical(p_on);
        _emit_changed();
    }
}

void DataMatrixEditor::_on_enabled_toggled(bool p_on, int64_t p_index)
{
    Ref<DataEntry> entry = _entry(p_index);
    if (entry.is_valid())
    {
        entry->set_enabled(p_on);
        _emit_changed();
    }
}

void DataMatrixEditor::_on_remove(int64_t p_index)
{
    TypedArray<DataEntry> entries = registry->get_entries();
    if (p_index < 0 || p_index >= entries.size())
    {
        return;
    }
    entries.remove_at(p_index);
    registry->set_entries(entries);
    selected = -1;
    callable_mp(this, &DataMatrixEditor::refresh).call_deferred();
    _emit_changed();
}

void DataMatrixEditor::_on_remove_all_missing()
{
    if (registry.is_null())
    {
        return;
    }
    TypedArray<DataEntry> entries = registry->get_entries();
    TypedArray<DataEntry> kept;
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<DataEntry> entry = entries[i];
        if (entry.is_valid() && !entry->get_is_missing())
        {
            kept.push_back(entry);
        }
    }
    if (kept.size() == entries.size())
    {
        return;
    }
    registry->set_entries(kept);
    selected = -1;
    callable_mp(this, &DataMatrixEditor::refresh).call_deferred();
    _emit_changed();
}

void DataMatrixEditor::_on_settings_resource_changed(const Ref<Resource> &p_resource, int64_t p_index)
{
    Ref<DataEntry> entry = _entry(p_index);
    if (entry.is_null())
    {
        return;
    }
    entry->set_settings(p_resource);
    callable_mp(this, &DataMatrixEditor::refresh).call_deferred();
    _emit_changed();
}

void DataMatrixEditor::_on_settings_edited() { _emit_changed(); }
