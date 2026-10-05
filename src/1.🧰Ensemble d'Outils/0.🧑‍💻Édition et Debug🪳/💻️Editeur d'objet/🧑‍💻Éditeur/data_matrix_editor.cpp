#include "data_matrix_editor.hpp"
#include "editor_undo_backend.hpp"
#include "icon_picker.hpp"
#include "interface_style_editor.hpp"
#include "preview_source_editor.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/type_utils.hpp"

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
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

namespace
{
const char32_t *const HEADERS[] = {U"", U"Donnée", U"Type", U"Widget", U"Classe Custom", U"Vertical", U"Affiché", U"Infobulle (BBCode)", U"Réglages", U""};
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
    HBoxContainer *top = memnew(HBoxContainer);
    add_child(top);
    // Aperçu personnalisé : fournis (image, modèle 3D, scène 2D) et class_name héritant de PreviewWidget.
    MenuButton *add_preview = memnew(MenuButton);
    add_preview->set_text(U"+ Aperçu");
    add_preview->set_flat(false);
    add_preview->set_tooltip_text(U"Ajouter un aperçu personnalisé (pas une propriété) : il se place, se groupe et se style comme une donnée");
    add_preview->connect("about_to_popup", callable_mp(this, &DataMatrixEditor::_fill_preview_menu).bind((uint64_t)add_preview->get_instance_id()));
    add_preview->get_popup()->connect("id_pressed", callable_mp(this, &DataMatrixEditor::_on_add_preview));
    top->add_child(add_preview);
    Control *spacer = memnew(Control);
    spacer->set_h_size_flags(SIZE_EXPAND_FILL);
    top->add_child(spacer);
    Button *remove_missing = memnew(Button);
    remove_missing->set_text(U"Retirer les entrées manquantes (⚠)");
    remove_missing->connect("pressed", callable_mp(this, &DataMatrixEditor::_on_remove_all_missing));
    top->add_child(remove_missing);

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

void DataMatrixEditor::_set_tooltip(int64_t p_index, const String &p_tooltip)
{
    Ref<DataEntry> entry = _entry(p_index);
    if (entry.is_null() || entry->get_tooltip() == p_tooltip)
    {
        return;
    }
    entry->set_tooltip(p_tooltip);
    _emit_changed();
}

void DataMatrixEditor::_on_tooltip_submitted(const String &p_text, int64_t p_index)
{
    _set_tooltip(p_index, p_text.replace(U"⏎", "\n"));
    callable_mp(this, &DataMatrixEditor::_refresh_settings_panel).call_deferred();
}

void DataMatrixEditor::_on_tooltip_focus_exited(int64_t p_index, uint64_t p_line_id)
{
    if (LineEdit *line = Object::cast_to<LineEdit>(ObjectDB::get_instance(p_line_id)))
    {
        _on_tooltip_submitted(line->get_text(), p_index);
    }
}

void DataMatrixEditor::_on_tooltip_text_changed(uint64_t p_edit_id, uint64_t p_preview_id)
{
    TextEdit *edit = Object::cast_to<TextEdit>(ObjectDB::get_instance(p_edit_id));
    RichTextLabel *preview = Object::cast_to<RichTextLabel>(ObjectDB::get_instance(p_preview_id));
    if (edit && preview)
    {
        preview->set_text(WidgetBehavior::format_text(edit->get_text(), ObjectDB::get_instance(context_id)));
    }
}

void DataMatrixEditor::_on_tooltip_edit_committed(int64_t p_index, uint64_t p_edit_id)
{
    if (TextEdit *edit = Object::cast_to<TextEdit>(ObjectDB::get_instance(p_edit_id)))
    {
        _set_tooltip(p_index, edit->get_text());
        callable_mp(this, &DataMatrixEditor::refresh).call_deferred(); // colonne du tableau à jour
    }
}

PackedStringArray DataMatrixEditor::preview_types()
{
    PackedStringArray types;
    types.push_back("TexturePreview");
    types.push_back("Model3DPreview");
    types.push_back("Scene2DPreview");
    TypedArray<Dictionary> globals = ProjectSettings::get_singleton()->get_global_class_list();
    for (int i = 0; i < globals.size(); i++)
    {
        String name = Dictionary(globals[i]).get("class", "");
        if (TypeUtils::class_inherits(name, "PreviewWidget"))
        {
            types.push_back(name);
        }
    }
    return types;
}

void DataMatrixEditor::_fill_preview_menu(uint64_t p_menu_id)
{
    MenuButton *menu = Object::cast_to<MenuButton>(ObjectDB::get_instance(p_menu_id));
    if (!menu)
    {
        return;
    }
    PopupMenu *popup = menu->get_popup();
    popup->clear();
    PackedStringArray types = preview_types();
    const char32_t *const builtin_names[] = {U"Image (TexturePreview)", U"Modèle 3D (Model3DPreview)", U"Scène 2D (Scene2DPreview)"};
    for (int i = 0; i < types.size(); i++)
    {
        popup->add_item(i < 3 ? String(builtin_names[i]) : types[i] + U" (script)", i);
    }
}

void DataMatrixEditor::_on_add_preview(int64_t p_id)
{
    PackedStringArray types = preview_types();
    if (registry.is_null() || p_id < 0 || p_id >= types.size())
    {
        return;
    }
    // Nom unique : « apercu », « apercu_2 »...
    String name = U"apercu";
    for (int n = 2; registry->find(name, DataEntry::MEMBER_PREVIEW).is_valid() || registry->find(name).is_valid(); n++)
    {
        name = U"apercu_" + String::num_int64(n);
    }
    Ref<DataEntry> entry;
    entry.instantiate();
    entry->set_data_name(name);
    entry->set_member(DataEntry::MEMBER_PREVIEW);
    entry->set_kind(DataEntry::KIND_PREVIEW);
    Ref<PreviewSettings> preview_settings;
    preview_settings.instantiate();
    preview_settings->set_preview_class(types[p_id]);
    preview_settings->get_source(); // objet affiché
    entry->set_settings(preview_settings);
    TypedArray<DataEntry> entries = registry->get_entries();
    entries.push_back(entry);
    registry->set_entries(entries);
    selected = entries.size() - 1;
    callable_mp(this, &DataMatrixEditor::refresh).call_deferred();
    _emit_changed();
}

void DataMatrixEditor::_on_preview_renamed(const String &p_text, int64_t p_index)
{
    Ref<DataEntry> entry = _entry(p_index);
    String name = p_text.strip_edges();
    if (entry.is_null() || name.is_empty() || name == String(entry->get_data_name()) || registry->find(name, DataEntry::MEMBER_PREVIEW).is_valid() ||
        registry->find(name).is_valid())
    {
        return;
    }
    // Les comportements qui visaient l'aperçu suivent le nouveau nom.
    String previous = entry->get_data_name();
    TypedArray<WidgetBehavior> behaviors = registry->get_behaviors();
    for (int i = 0; i < behaviors.size(); i++)
    {
        Ref<WidgetBehavior> behavior = behaviors[i];
        PackedStringArray targets = behavior.is_valid() ? behavior->get_targets() : PackedStringArray();
        int position = targets.find(previous);
        if (position >= 0)
        {
            targets.set(position, name);
            behavior->set_targets(targets);
        }
    }
    entry->set_data_name(name);
    callable_mp(this, &DataMatrixEditor::refresh).call_deferred();
    _emit_changed();
}

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

        // 7. Infobulle (une ligne ; « ⏎ » = saut de ligne ; édition complète sous le tableau)
        LineEdit *tooltip = memnew(LineEdit);
        tooltip->set_text(entry->get_tooltip().replace("\n", U"⏎"));
        tooltip->set_placeholder(U"[b]Texte[/b] {propriété} {fonction()}");
        tooltip->set_custom_minimum_size(Vector2(220, 0));
        tooltip->connect("text_submitted", callable_mp(this, &DataMatrixEditor::_on_tooltip_submitted).bind(i));
        tooltip->connect("focus_exited", callable_mp(this, &DataMatrixEditor::_on_tooltip_focus_exited).bind(i, (uint64_t)tooltip->get_instance_id()));
        grid->add_child(tooltip);

        // 8. Réglages (résumé ; édition sous le tableau)
        Ref<WidgetSettings> settings = entry->get_settings();
        grid->add_child(make_label(settings.is_valid() ? settings->get_class() : String(U"—"), 0.7f));

        // 9. Suppression (entrées manquantes et aperçus : les autres sont recréées à la synchronisation)
        if (entry->get_is_missing() || entry->get_member() == DataEntry::MEMBER_PREVIEW)
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

    // Infobulle : BBCode, modèles {propriété} / {fonction()} résolus sur la cible au survol ; aperçu en direct.
    settings_panel->add_child(make_label(U"Infobulle (BBCode ; {propriété}, {chemin:x}, {fonction()} ; {{ pour une accolade)", 0.8f));
    TextEdit *tooltip_edit = memnew(TextEdit);
    tooltip_edit->set_text(entry->get_tooltip());
    tooltip_edit->set_custom_minimum_size(Vector2(0, 70));
    tooltip_edit->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
    settings_panel->add_child(tooltip_edit);
    RichTextLabel *tooltip_preview = memnew(RichTextLabel);
    tooltip_preview->set_use_bbcode(true);
    tooltip_preview->set_fit_content(true);
    tooltip_preview->set_text(WidgetBehavior::format_text(entry->get_tooltip(), ObjectDB::get_instance(context_id)));
    settings_panel->add_child(tooltip_preview);
    tooltip_edit->connect("text_changed", callable_mp(this, &DataMatrixEditor::_on_tooltip_text_changed).bind((uint64_t)tooltip_edit->get_instance_id(), (uint64_t)tooltip_preview->get_instance_id()));
    tooltip_edit->connect("focus_exited", callable_mp(this, &DataMatrixEditor::_on_tooltip_edit_committed).bind(selected, (uint64_t)tooltip_edit->get_instance_id()));

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
        settings_entry->set_enabled(own.has(String(settings_entry->get_data_name())) && settings_entry->get_data_name() != StringName("style") &&
                                    settings_entry->get_data_name() != StringName("icon") && settings_entry->get_data_name() != StringName("source"));
    }

    ProceduralEditor *settings_editor = memnew(ProceduralEditor);
    settings_editor->set_v_size_flags(SIZE_EXPAND_FILL);
    settings_editor->set_undo_backend(Ref<UndoBackend>(memnew(EditorUndoBackend)));
    settings_editor->set_styles_enabled(false);
    settings_editor->set_use_layouts(false);
    settings_editor->set_registry(settings_registry);
    settings_editor->set_target(settings.ptr());
    settings_panel->add_child(settings_editor);

    // Aperçu : nom (cible des groupes et comportements) et source.
    if (Ref<PreviewSettings> preview_settings = settings; preview_settings.is_valid() && entry->get_member() == DataEntry::MEMBER_PREVIEW)
    {
        HBoxContainer *name_row = memnew(HBoxContainer);
        name_row->add_child(make_label(U"Nom de l'aperçu"));
        LineEdit *name_edit = memnew(LineEdit);
        name_edit->set_text(String(entry->get_data_name()));
        name_edit->set_h_size_flags(SIZE_EXPAND_FILL);
        name_edit->connect("text_submitted", callable_mp(this, &DataMatrixEditor::_on_preview_renamed).bind(selected));
        name_row->add_child(name_edit);
        settings_panel->add_child(name_row);
        PreviewSourceEditor *source_editor = memnew(PreviewSourceEditor);
        source_editor->setup(preview_settings, ObjectDB::get_instance(context_id));
        source_editor->connect("source_changed", callable_mp(this, &DataMatrixEditor::_emit_changed));
        settings_panel->add_child(source_editor);
    }

    // Icône du widget (ressource du projet ou icône de l'éditeur copiée).
    IconPickerRow *icon_row = memnew(IconPickerRow);
    icon_row->setup(settings.ptr(), "icon", U"Icône");
    icon_row->connect("icon_changed", callable_mp(this, &DataMatrixEditor::_emit_changed));
    settings_panel->add_child(icon_row);

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
