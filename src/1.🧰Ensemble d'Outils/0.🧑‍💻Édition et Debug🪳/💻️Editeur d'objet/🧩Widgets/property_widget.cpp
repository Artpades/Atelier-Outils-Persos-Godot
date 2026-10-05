#include "property_widget.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/widget_table.hpp"

#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/base_button.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/slider.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

using namespace godot;

void PropertyWidget::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("pull", "value"), &PropertyWidget::pull);
    ClassDB::bind_method(D_METHOD("emit_value", "value", "committing"), &PropertyWidget::emit_value);
    ClassDB::bind_method(D_METHOD("commit_if_editing", "value"), &PropertyWidget::commit_if_editing);
    ClassDB::bind_method(D_METHOD("has_uncommitted_edit"), &PropertyWidget::has_uncommitted_edit);
    ClassDB::bind_method(D_METHOD("differs_from_object", "value"), &PropertyWidget::differs_from_object);
    ClassDB::bind_method(D_METHOD("get_accessor"), &PropertyWidget::get_accessor);
    ClassDB::bind_method(D_METHOD("get_property_name"), &PropertyWidget::get_property_name);
    ClassDB::bind_method(D_METHOD("get_info"), &PropertyWidget::get_info);
    ClassDB::bind_method(D_METHOD("get_settings"), &PropertyWidget::get_settings);
    ClassDB::bind_method(D_METHOD("get_styled_item"), &PropertyWidget::get_styled_item);
    ClassDB::bind_method(D_METHOD("set_icon_override", "icon"), &PropertyWidget::set_icon_override);
    ClassDB::bind_method(D_METHOD("get_displayed_icon"), &PropertyWidget::get_displayed_icon);
    ClassDB::bind_method(D_METHOD("set_locked", "locked"), &PropertyWidget::set_locked);
    ClassDB::bind_method(D_METHOD("is_locked"), &PropertyWidget::is_locked);
    ClassDB::bind_method(D_METHOD("set_tooltip_texts", "texts"), &PropertyWidget::set_tooltip_texts);
    ClassDB::bind_method(D_METHOD("get_tooltip_texts"), &PropertyWidget::get_tooltip_texts);
    ClassDB::bind_method(D_METHOD("get_style"), &PropertyWidget::get_style);
    ClassDB::bind_method(D_METHOD("is_read_only"), &PropertyWidget::is_read_only);
    ClassDB::bind_method(D_METHOD("has_edit_focus"), &PropertyWidget::has_edit_focus);
    ClassDB::bind_method(D_METHOD("get_last_value"), &PropertyWidget::get_last_value);
    ClassDB::bind_static_method("PropertyWidget", D_METHOD("display_name", "property"), &PropertyWidget::display_name);
    ClassDB::bind_static_method("PropertyWidget", D_METHOD("stringify", "value"), &PropertyWidget::stringify);

    GDVIRTUAL_BIND(_build, "info", "settings");
    GDVIRTUAL_BIND(_pull, "value");

    ADD_SIGNAL(MethodInfo("value_changed", PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT), PropertyInfo(Variant::BOOL, "committing")));
    // Une édition non validée a été remplacée par une valeur venue de l'objet (pull) : elle est abandonnée.
    ADD_SIGNAL(MethodInfo("edit_discarded"));
}

String PropertyWidget::display_name(const StringName &p_property) { return String(p_property).capitalize(); }

String PropertyWidget::stringify(const Variant &p_value)
{
    switch (p_value.get_type())
    {
    case Variant::RID:
    {
        RID rid = p_value;
        return "RID(" + String::num_int64(rid.get_id()) + ")";
    }
    case Variant::OBJECT:
    {
        Object *obj = p_value;
        return obj ? obj->get_class() + " #" + String::num_uint64((uint64_t)obj->get_instance_id()) : String("<null>");
    }
    default:
        return p_value.stringify();
    }
}

void PropertyWidget::setup(const Ref<ValueAccessor> &p_accessor, const Ref<WidgetSettings> &p_settings, DataEntry::WidgetChoice p_style, bool p_vertical)
{
    accessor = p_accessor;
    info = accessor.is_valid() ? accessor->get_property_info() : Dictionary();
    settings = p_settings;
    style = p_style;
    read_only = accessor.is_valid() && accessor->is_read_only();

    set_vertical(p_vertical);
    set_h_size_flags(SIZE_EXPAND_FILL);

    // Un WidgetSettings de base (réglages communs seulement) convient à tous les widgets ; seule une sous-classe
    // destinée à un autre widget est signalée.
    if (settings.is_valid() && settings->get_class() != StringName("WidgetSettings") && !settings->is_class(get_settings_class()))
    {
        log_it(U"[color=orange]« " + String(get_property_name()) + U" » : réglages " + settings->get_class() + U" ignorés (attendu : " + String(get_settings_class()) +
                   U") ; seuls les réglages communs s'appliquent[/color]",
               this);
    }

    bool show_label = (settings.is_null() || settings->get_show_label()) && !has_own_title();
    Label *name_label = nullptr;
    double ratio = settings.is_valid() ? settings->get_label_ratio() : 0.4;

    // Icône des réglages : avant le libellé, ou avant la valeur (ajoutée après le libellé).
    base_icon = settings.is_valid() ? settings->get_icon() : Ref<Texture2D>();
    bool icon_before_value = settings.is_valid() && settings->get_icon_position() == WidgetSettings::ICON_BEFORE_VALUE;
    if (base_icon.is_valid() && !icon_before_value)
    {
        set_icon_override(Ref<Texture2D>());
    }

    if (show_label)
    {
        name_label = memnew(Label);
        name_label->set_text(accessor.is_valid() ? accessor->get_display_name() : display_name(get_property_name()));
        name_label->set_tooltip_text(get_property_name());
        name_label->set_mouse_filter(MOUSE_FILTER_STOP);
        name_label->set_clip_text(true);
        name_label->set_h_size_flags(SIZE_EXPAND_FILL);
        name_label->set_v_size_flags(SIZE_SHRINK_BEGIN);
        name_label->set_stretch_ratio(ratio);
        add_child(name_label);
        name_label_id = ObjectID(name_label->get_instance_id());
    }
    if (base_icon.is_valid() && icon_before_value)
    {
        set_icon_override(Ref<Texture2D>());
    }

    Control *editor = build_editor();
    if (editor)
    {
        editor->set_h_size_flags(SIZE_EXPAND_FILL);
        editor->set_stretch_ratio(show_label ? 1.0 - ratio : 1.0);
        add_child(editor);
    }

    // Style des réglages, sur la partie choisie (le libellé absent : style ignoré pour lui).
    Ref<InterfaceStyle> widget_style = settings.is_valid() ? settings->get_style() : Ref<InterfaceStyle>();
    if (widget_style.is_valid())
    {
        WidgetSettings::StyleTarget target = settings->get_style_target();
        CanvasItem *item = target == WidgetSettings::STYLE_VALUE   ? (CanvasItem *)editor
                           : target == WidgetSettings::STYLE_LABEL ? (CanvasItem *)name_label
                                                                   : (CanvasItem *)this;
        if (item)
        {
            widget_style->apply_to(item);
            styled_item_id = ObjectID(item->get_instance_id());
            if (item == this)
            {
                background = widget_style->get_background();
                queue_redraw();
            }
        }
    }
}

void PropertyWidget::_notification(int p_what)
{
    if (p_what == NOTIFICATION_DRAW && background.is_valid())
    {
        draw_style_box(background, Rect2(Vector2(), get_size()));
    }
}

void PropertyWidget::set_icon_override(const Ref<Texture2D> &p_icon)
{
    Ref<Texture2D> texture = p_icon.is_valid() ? p_icon : base_icon;
    TextureRect *rect = Object::cast_to<TextureRect>(ObjectDB::get_instance(icon_rect_id));
    if (!rect)
    {
        if (texture.is_null())
        {
            return;
        }
        int size = settings.is_valid() ? settings->get_icon_size() : 16;
        rect = memnew(TextureRect);
        rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
        rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        rect->set_custom_minimum_size(Vector2(size, size));
        rect->set_v_size_flags(SIZE_SHRINK_CENTER);
        rect->set_mouse_filter(MOUSE_FILTER_PASS);
        add_child(rect);
        // Avant la valeur (dernier enfant ajouté jusqu'ici = libellé) ou en tête.
        bool before_value = settings.is_valid() && settings->get_icon_position() == WidgetSettings::ICON_BEFORE_VALUE;
        Node *label = Object::cast_to<Node>(ObjectDB::get_instance(name_label_id));
        move_child(rect, (before_value && label) ? label->get_index() + 1 : 0);
        icon_rect_id = ObjectID(rect->get_instance_id());
    }
    rect->set_texture(texture);
    rect->set_visible(texture.is_valid());
}

Ref<Texture2D> PropertyWidget::get_displayed_icon() const
{
    TextureRect *rect = Object::cast_to<TextureRect>(ObjectDB::get_instance(icon_rect_id));
    return (rect && rect->is_visible()) ? rect->get_texture() : Ref<Texture2D>();
}

void PropertyWidget::_set_controls_editable(Node *p_node, bool p_editable)
{
    TypedArray<Node> children = p_node->get_children(true);
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        // État d'origine mémorisé au premier verrouillage, restauré au déverrouillage.
        const StringName meta = "_behavior_editable";
        if (BaseButton *button = Object::cast_to<BaseButton>(child))
        {
            if (!p_editable)
            {
                button->set_meta(meta, !button->is_disabled());
            }
            button->set_disabled(!p_editable || !bool(button->get_meta(meta, true)));
        }
        else if (LineEdit *line = Object::cast_to<LineEdit>(child))
        {
            if (!p_editable)
            {
                line->set_meta(meta, line->is_editable());
            }
            line->set_editable(p_editable && bool(line->get_meta(meta, true)));
        }
        else if (TextEdit *text = Object::cast_to<TextEdit>(child))
        {
            if (!p_editable)
            {
                text->set_meta(meta, text->is_editable());
            }
            text->set_editable(p_editable && bool(text->get_meta(meta, true)));
        }
        else if (SpinBox *spin = Object::cast_to<SpinBox>(child))
        {
            if (!p_editable)
            {
                spin->set_meta(meta, spin->is_editable());
            }
            spin->set_editable(p_editable && bool(spin->get_meta(meta, true)));
        }
        else if (Slider *slider = Object::cast_to<Slider>(child))
        {
            if (!p_editable)
            {
                slider->set_meta(meta, slider->is_editable());
            }
            slider->set_editable(p_editable && bool(slider->get_meta(meta, true)));
        }
        // Les contrôles de saisie gèrent eux-mêmes leurs enfants internes (le LineEdit d'un SpinBox) : y
        // descendre mémoriserait l'état qu'ils viennent de leur imposer.
        if (Object::cast_to<SpinBox>(child) || Object::cast_to<LineEdit>(child) || Object::cast_to<TextEdit>(child) || Object::cast_to<Slider>(child))
        {
            continue;
        }
        _set_controls_editable(child, p_editable);
    }
}

void PropertyWidget::set_locked(bool p_locked)
{
    if (locked == p_locked)
    {
        return;
    }
    locked = p_locked;
    _set_controls_editable(this, !locked);
    // Atténuation par-dessus la teinte éventuelle du style (restaurée au déverrouillage).
    if (locked)
    {
        set_meta("_behavior_modulate", get_modulate());
        set_modulate(get_modulate() * Color(1, 1, 1, 0.55));
    }
    else
    {
        set_modulate(get_meta("_behavior_modulate", Color(1, 1, 1, 1)));
    }
}

bool PropertyWidget::is_locked() const { return locked; }

void PropertyWidget::set_tooltip_texts(const PackedStringArray &p_texts)
{
    tooltip_texts = p_texts;
    // Infobulle enrichie : l'infobulle simple du libellé est retirée (sinon les deux s'afficheraient).
    if (Control *label = Object::cast_to<Control>(ObjectDB::get_instance(name_label_id)))
    {
        label->set_tooltip_text(tooltip_texts.is_empty() ? String(get_property_name()) : String());
    }
}

PackedStringArray PropertyWidget::get_tooltip_texts() const { return tooltip_texts; }

CanvasItem *PropertyWidget::get_styled_item() const { return Object::cast_to<CanvasItem>(ObjectDB::get_instance(styled_item_id)); }

Ref<DataEntry> PropertyWidget::make_default_entry(const Dictionary &p_info, DataEntry::MemberKind p_member)
{
    Ref<DataEntry> entry;
    entry.instantiate();
    entry->set_data_name(p_info.get("name", ""));
    entry->set_member(p_member);
    DataEntry::DataKind kind = WidgetTable::kind_of(p_info, p_member);
    entry->set_kind(kind);
    entry->set_is_vertical(WidgetTable::is_container_kind(kind));
    return entry;
}

void PropertyWidget::set_context(const Ref<WidgetContext> &p_context, int p_depth, const PackedInt64Array &p_ancestors)
{
    context = p_context;
    depth = p_depth;
    ancestors = p_ancestors;
}

PropertyWidget *PropertyWidget::build_child(const Ref<ValueAccessor> &p_accessor, const Ref<DataEntry> &p_entry, const PackedInt64Array &p_ancestors) const
{
    if (context.is_null() || depth + 1 > context->get_max_depth())
    {
        return nullptr;
    }
    return context->build_widget(p_accessor, p_entry, depth + 1, p_ancestors);
}

Control *PropertyWidget::build_editor()
{
    Control *custom = nullptr;
    if (GDVIRTUAL_CALL(_build, info, settings, custom))
    {
        return custom;
    }
    value_label = memnew(Label);
    value_label->set_clip_text(true);
    value_label->set_mouse_filter(MOUSE_FILTER_PASS);
    return value_label;
}

void PropertyWidget::pull_value(const Variant &p_value)
{
    if (GDVIRTUAL_CALL(_pull, p_value))
    {
        return;
    }
    if (value_label)
    {
        value_label->set_text(stringify(p_value));
        value_label->set_tooltip_text(value_label->get_text());
    }
}

void PropertyWidget::pull(const Variant &p_value)
{
    pulling = true;
    pull_value(p_value);
    pulling = false;
    if (editing)
    {
        editing = false;
        emit_signal("edit_discarded");
    }
}

void PropertyWidget::emit_value(const Variant &p_value, bool p_committing)
{
    if (pulling || read_only || locked)
    {
        return;
    }
    editing = !p_committing;
    emit_signal("value_changed", p_value, p_committing);
}

void PropertyWidget::commit_if_editing(const Variant &p_value)
{
    if (editing)
    {
        emit_value(p_value, true);
    }
}

bool PropertyWidget::has_uncommitted_edit() const { return editing; }

Variant PropertyWidget::read_value() const { return accessor.is_valid() ? accessor->read() : Variant(); }

bool PropertyWidget::differs_from_object(const Variant &p_value) const
{
    if (accessor.is_null() || !accessor->is_valid())
    {
        return false;
    }
    Variant current = accessor->read();
    return current.get_type() != p_value.get_type() || !current.hash_compare(p_value);
}

Ref<ValueAccessor> PropertyWidget::get_accessor() const { return accessor; }
StringName PropertyWidget::get_property_name() const { return accessor.is_valid() ? accessor->get_property() : StringName(); }
Dictionary PropertyWidget::get_info() const { return info; }
Ref<WidgetSettings> PropertyWidget::get_settings() const { return settings; }
DataEntry::WidgetChoice PropertyWidget::get_style() const { return style; }
bool PropertyWidget::is_read_only() const { return read_only; }

bool PropertyWidget::has_edit_focus() const
{
    Window *window = get_window();
    if (!window || !window->has_focus())
    {
        return false;
    }
    Control *focus = window->gui_get_focus_owner();
    return focus && is_ancestor_of(focus);
}

bool PropertyWidget::has_last_value() const { return has_last; }
Variant PropertyWidget::get_last_value() const { return last_value; }
void PropertyWidget::set_last_value(const Variant &p_value)
{
    last_value = p_value;
    has_last = true;
}
