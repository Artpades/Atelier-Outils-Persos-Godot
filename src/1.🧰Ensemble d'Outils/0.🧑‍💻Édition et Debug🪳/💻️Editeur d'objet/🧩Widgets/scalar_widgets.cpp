#include "scalar_widgets.hpp"

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/check_button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/progress_bar.hpp>
#include <godot_cpp/classes/slider.hpp>
#include <godot_cpp/classes/texture_progress_bar.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/v_slider.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

namespace
{
Variant::Type info_type(const Dictionary &p_info) { return (Variant::Type)(int)p_info.get("type", 0); }
int info_hint(const Dictionary &p_info) { return p_info.get("hint", 0); }
String info_hint_string(const Dictionary &p_info) { return p_info.get("hint_string", ""); }
} // namespace

// =====================================================================================================
// BoolWidget
// =====================================================================================================

Control *BoolWidget::build_editor()
{
    if (get_style() == DataEntry::WIDGET_CHECK_BUTTON)
    {
        button = memnew(CheckButton);
    }
    else
    {
        button = memnew(CheckBox);
    }
    button->set_disabled(is_read_only());
    button->connect("toggled", callable_mp(this, &BoolWidget::_on_toggled));
    return button;
}

void BoolWidget::pull_value(const Variant &p_value) { button->set_pressed_no_signal(bool(p_value)); }

void BoolWidget::_on_toggled(bool p_pressed) { emit_value(p_pressed, true); }

// =====================================================================================================
// NumberWidget
// =====================================================================================================

NumberWidget::RangeSpec NumberWidget::parse_range_hint(const String &p_hint)
{
    RangeSpec spec;
    PackedStringArray parts = p_hint.split(",");
    int numeric = 0;
    for (int i = 0; i < parts.size(); i++)
    {
        String part = parts[i].strip_edges();
        if (part.is_valid_float() && numeric < 3)
        {
            double value = part.to_float();
            if (numeric == 0)
            {
                spec.min = value;
            }
            else if (numeric == 1)
            {
                spec.max = value;
                spec.has_bounds = true;
            }
            else
            {
                spec.step = value;
            }
            numeric++;
        }
        else if (part == "or_greater")
        {
            spec.or_greater = true;
        }
        else if (part == "or_less" || part == "or_lesser")
        {
            spec.or_less = true;
        }
        else if (part.begins_with("suffix:"))
        {
            spec.suffix = part.substr(7);
        }
    }
    return spec;
}

Control *NumberWidget::build_editor()
{
    Dictionary info = get_info();
    is_int = info_type(info) == Variant::INT;

    // Priorité : réglages > indice Range > défauts.
    RangeSpec spec;
    Ref<RangeWidgetSettings> range_settings = get_settings();
    if (range_settings.is_valid())
    {
        spec.has_bounds = true;
        spec.min = range_settings->get_min_value();
        spec.max = range_settings->get_max_value();
        spec.step = range_settings->get_step();
        spec.or_greater = range_settings->get_allow_greater();
        spec.or_less = range_settings->get_allow_lesser();
        spec.suffix = range_settings->get_suffix();
    }
    else if (info_hint(info) == PROPERTY_HINT_RANGE)
    {
        spec = parse_range_hint(info_hint_string(info));
    }

    DataEntry::WidgetChoice style = get_style();
    bool bounded = style != DataEntry::WIDGET_SPIN_BOX;
    double min_value = spec.has_bounds ? spec.min : bounded ? 0.0 : -1e12;
    double max_value = spec.has_bounds ? spec.max : bounded ? 100.0 : 1e12;
    double step = spec.step > 0.0 ? spec.step : is_int ? 1.0 : 0.001;

    Control *editor = nullptr;
    switch (style)
    {
    case DataEntry::WIDGET_H_SLIDER:
    case DataEntry::WIDGET_V_SLIDER:
    {
        Slider *slider = nullptr;
        if (style == DataEntry::WIDGET_H_SLIDER)
        {
            slider = memnew(HSlider);
        }
        else
        {
            slider = memnew(VSlider);
            slider->set_custom_minimum_size(Vector2(0, 100));
        }
        slider->set_editable(!is_read_only());
        slider->set_h_size_flags(SIZE_EXPAND_FILL);
        slider->connect("drag_started", callable_mp(this, &NumberWidget::_on_drag_started));
        slider->connect("drag_ended", callable_mp(this, &NumberWidget::_on_drag_ended));

        value_text = memnew(Label);
        value_text->set_custom_minimum_size(Vector2(48, 0));
        value_text->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);

        HBoxContainer *row = memnew(HBoxContainer);
        row->add_child(slider);
        row->add_child(value_text);
        range = slider;
        editor = row;
        break;
    }
    case DataEntry::WIDGET_PROGRESS_BAR:
    case DataEntry::WIDGET_TEXTURE_PROGRESS_BAR:
    {
        Ref<TextureProgressWidgetSettings> texture_settings = get_settings();
        if (style == DataEntry::WIDGET_TEXTURE_PROGRESS_BAR && texture_settings.is_valid() && texture_settings->get_progress_texture().is_valid())
        {
            TextureProgressBar *bar = memnew(TextureProgressBar);
            bar->set_progress_texture(texture_settings->get_progress_texture());
            bar->set_under_texture(texture_settings->get_under_texture());
            range = bar;
        }
        else
        {
            // Sans texture de progression, une TextureProgressBar n'afficherait rien : ProgressBar simple.
            ProgressBar *bar = memnew(ProgressBar);
            bar->set_show_percentage(false);
            range = bar;
        }
        range->set_custom_minimum_size(Vector2(0, 20));
        editor = range;
        break;
    }
    case DataEntry::WIDGET_SPIN_BOX:
    default:
    {
        SpinBox *spin = memnew(SpinBox);
        spin->set_suffix(spec.suffix);
        spin->set_editable(!is_read_only());
        range = spin;
        editor = spin;
        break;
    }
    }

    range->set_max(max_value);
    range->set_min(min_value);
    range->set_step(step);
    range->set_use_rounded_values(is_int);
    range->set_allow_greater(spec.or_greater || (!spec.has_bounds && !bounded));
    range->set_allow_lesser(spec.or_less || (!spec.has_bounds && !bounded));
    range->connect("value_changed", callable_mp(this, &NumberWidget::_on_value_changed));
    return editor;
}

void NumberWidget::pull_value(const Variant &p_value)
{
    range->set_value_no_signal(double(p_value));
    _update_value_text();
}

void NumberWidget::_update_value_text()
{
    if (value_text)
    {
        value_text->set_text(is_int ? String::num_int64((int64_t)range->get_value()) : String::num(range->get_value(), 3));
    }
}

Variant NumberWidget::_current_value() const
{
    double value = range->get_value();
    return is_int ? Variant((int64_t)Math::round(value)) : Variant(value);
}

void NumberWidget::_on_value_changed(double p_value)
{
    (void)p_value;
    _update_value_text();
    // Pendant un glissement : application directe ; la validation arrive avec drag_ended.
    emit_value(_current_value(), !dragging);
}

void NumberWidget::_on_drag_started() { dragging = true; }

void NumberWidget::_on_drag_ended(bool p_value_changed)
{
    dragging = false;
    if (p_value_changed)
    {
        emit_value(_current_value(), true);
    }
}

// =====================================================================================================
// TextWidget
// =====================================================================================================

Control *TextWidget::build_editor()
{
    if (get_style() == DataEntry::WIDGET_TEXT_EDIT)
    {
        Ref<TextEditWidgetSettings> text_settings = get_settings();
        text_edit = memnew(TextEdit);
        text_edit->set_custom_minimum_size(Vector2(0, text_settings.is_valid() ? text_settings->get_height() : 80.0));
        text_edit->set_editable(!is_read_only());
        text_edit->connect("text_changed", callable_mp(this, &TextWidget::_on_text_edit_changed));
        text_edit->connect("focus_exited", callable_mp(this, &TextWidget::_on_focus_exited));
        return text_edit;
    }

    line_edit = memnew(LineEdit);
    line_edit->set_editable(!is_read_only());
    line_edit->connect("text_changed", callable_mp(this, &TextWidget::_on_text_changed));
    line_edit->connect("text_submitted", callable_mp(this, &TextWidget::_on_text_submitted));
    line_edit->connect("focus_exited", callable_mp(this, &TextWidget::_on_focus_exited));
    return line_edit;
}

void TextWidget::pull_value(const Variant &p_value)
{
    String text = p_value;
    if (line_edit && line_edit->get_text() != text)
    {
        int caret = line_edit->get_caret_column();
        line_edit->set_text(text);
        line_edit->set_caret_column(MIN(caret, text.length()));
    }
    else if (text_edit && text_edit->get_text() != text)
    {
        text_edit->set_text(text); // émet text_changed : ignoré car is_pulling()
    }
}

Variant TextWidget::_wrap(const String &p_text) const
{
    if (info_type(get_info()) == Variant::STRING_NAME)
    {
        return StringName(p_text);
    }
    return p_text;
}

String TextWidget::_current_text() const { return line_edit ? line_edit->get_text() : text_edit ? text_edit->get_text() : String(); }

// text_changed est émis en DIFFÉRÉ par LineEdit/TextEdit : il peut arriver après text_submitted, ou après
// un pull. Un texte identique à la valeur de l'objet n'est donc pas une modification de l'utilisateur.
void TextWidget::_on_text_changed(const String &p_text)
{
    Variant value = _wrap(p_text);
    if (differs_from_object(value))
    {
        emit_value(value, false);
    }
}

void TextWidget::_on_text_edit_changed()
{
    if (is_pulling())
    {
        return;
    }
    _on_text_changed(_current_text());
}

// Entrée : action explicite, validée dès que le texte diffère de l'objet (même si text_changed n'est pas
// encore arrivé). Perte du focus : validée seulement après une vraie frappe (sinon, après un undo, le texte
// affiché serait réécrit et créerait une nouvelle action).
void TextWidget::_on_text_submitted(const String &p_text)
{
    Variant value = _wrap(p_text);
    if (has_uncommitted_edit() || differs_from_object(value))
    {
        emit_value(value, true);
    }
}

void TextWidget::_on_focus_exited() { commit_if_editing(_wrap(_current_text())); }

// =====================================================================================================
// EnumWidget
// =====================================================================================================

void EnumWidget::parse_items(const String &p_hint, bool p_flags, bool p_text, PackedStringArray &r_labels, Array &r_values)
{
    PackedStringArray parts = p_hint.split(",");
    int64_t next = 0;
    int index = 0;
    for (int i = 0; i < parts.size(); i++)
    {
        String part = parts[i].strip_edges();
        if (part.is_empty())
        {
            continue;
        }

        String label = part;
        bool has_explicit = false;
        int64_t explicit_value = 0;
        int colon = part.rfind(":");
        if (colon >= 0 && part.substr(colon + 1).strip_edges().is_valid_int())
        {
            label = part.substr(0, colon).strip_edges();
            explicit_value = part.substr(colon + 1).strip_edges().to_int();
            has_explicit = true;
        }

        Variant value;
        if (p_flags)
        {
            value = has_explicit ? explicit_value : (int64_t(1) << index);
        }
        else if (p_text)
        {
            value = label;
        }
        else
        {
            int64_t v = has_explicit ? explicit_value : next;
            value = v;
            next = v + 1;
        }
        r_labels.push_back(label);
        r_values.push_back(value);
        index++;
    }
}

Control *EnumWidget::build_editor()
{
    Dictionary info = get_info();
    Variant::Type type = info_type(info);
    text = type == Variant::STRING || type == Variant::STRING_NAME;
    flags = info_hint(info) == PROPERTY_HINT_FLAGS;
    parse_items(info_hint_string(info), flags, text, labels, values);

    DataEntry::WidgetChoice style = get_style();
    if (!flags && style == DataEntry::WIDGET_OPTION_BUTTON)
    {
        option = memnew(OptionButton);
        option->set_disabled(is_read_only());
        for (int i = 0; i < labels.size(); i++)
        {
            option->add_item(labels[i], i);
        }
        option->connect("item_selected", callable_mp(this, &EnumWidget::_on_item_selected));
        return option;
    }

    // Liste de cases : exclusives (ButtonGroup) pour un enum, indépendantes pour des flags.
    bool use_switch = style == DataEntry::WIDGET_CHECK_BUTTON_LIST;
    VBoxContainer *box = memnew(VBoxContainer);
    if (!flags)
    {
        group.instantiate();
    }
    for (int i = 0; i < labels.size(); i++)
    {
        BaseButton *check = nullptr;
        if (use_switch)
        {
            CheckButton *b = memnew(CheckButton);
            b->set_text(labels[i]);
            check = b;
        }
        else
        {
            CheckBox *b = memnew(CheckBox);
            b->set_text(labels[i]);
            check = b;
        }
        check->set_disabled(is_read_only());
        if (group.is_valid())
        {
            check->set_button_group(group);
            check->connect("toggled", callable_mp(this, &EnumWidget::_on_item_toggled).bind(i));
        }
        else
        {
            check->connect("toggled", callable_mp(this, &EnumWidget::_on_flag_toggled).bind(i));
        }
        buttons.push_back(check);
        box->add_child(check);
    }
    return box;
}

void EnumWidget::pull_value(const Variant &p_value)
{
    if (option)
    {
        int selected = -1;
        for (int i = 0; i < values.size(); i++)
        {
            if (_same_value(values[i], p_value))
            {
                selected = i;
                break;
            }
        }
        option->select(selected);
        return;
    }

    int64_t mask = p_value;
    for (int i = 0; i < buttons.size(); i++)
    {
        bool pressed = false;
        if (flags)
        {
            int64_t bit = values[i];
            pressed = bit != 0 && (mask & bit) == bit;
        }
        else
        {
            pressed = _same_value(values[i], p_value);
        }
        buttons[i]->set_pressed_no_signal(pressed);
    }
}

bool EnumWidget::_same_value(const Variant &p_a, const Variant &p_b) const
{
    if (text)
    {
        return String(p_a) == String(p_b);
    }
    return int64_t(p_a) == int64_t(p_b);
}

Variant EnumWidget::_wrap(const Variant &p_value) const
{
    if (info_type(get_info()) == Variant::STRING_NAME)
    {
        return StringName(String(p_value));
    }
    return p_value;
}

void EnumWidget::_on_item_selected(int64_t p_index)
{
    if (p_index >= 0 && p_index < values.size())
    {
        emit_value(_wrap(values[p_index]), true);
    }
}

void EnumWidget::_on_item_toggled(bool p_pressed, int64_t p_index)
{
    if (p_pressed && p_index >= 0 && p_index < values.size())
    {
        emit_value(_wrap(values[p_index]), true);
    }
}

void EnumWidget::_on_flag_toggled(bool p_pressed, int64_t p_index)
{
    (void)p_pressed;
    (void)p_index;
    int64_t mask = 0;
    for (int i = 0; i < buttons.size(); i++)
    {
        if (buttons[i]->is_pressed())
        {
            mask |= int64_t(values[i]);
        }
    }
    emit_value(mask, true);
}

// =====================================================================================================
// ColorWidget
// =====================================================================================================

Control *ColorWidget::build_editor()
{
    edit_alpha = info_hint(get_info()) != PROPERTY_HINT_COLOR_NO_ALPHA;

    if (get_style() == DataEntry::WIDGET_COLOR_RECT_CHANNELS)
    {
        HBoxContainer *row = memnew(HBoxContainer);
        preview = memnew(ColorRect);
        preview->set_custom_minimum_size(Vector2(24, 24));
        row->add_child(preview);

        const char *names[] = {"R ", "G ", "B ", "A "};
        int count = edit_alpha ? 4 : 3;
        for (int i = 0; i < count; i++)
        {
            SpinBox *spin = memnew(SpinBox);
            spin->set_prefix(names[i]);
            spin->set_min(0.0);
            spin->set_max(1.0);
            spin->set_step(0.001);
            spin->set_editable(!is_read_only());
            spin->set_h_size_flags(SIZE_EXPAND_FILL);
            spin->connect("value_changed", callable_mp(this, &ColorWidget::_on_channel_changed).bind(i));
            channels.push_back(spin);
            row->add_child(spin);
        }
        return row;
    }

    picker = memnew(ColorPickerButton);
    picker->set_edit_alpha(edit_alpha);
    picker->set_disabled(is_read_only());
    picker->set_custom_minimum_size(Vector2(0, 24));
    picker->connect("color_changed", callable_mp(this, &ColorWidget::_on_color_changed));
    picker->connect("popup_closed", callable_mp(this, &ColorWidget::_on_popup_closed));
    return picker;
}

void ColorWidget::pull_value(const Variant &p_value)
{
    Color color = p_value;
    if (picker)
    {
        picker->set_pick_color(color);
        return;
    }
    const float components[] = {color.r, color.g, color.b, color.a};
    for (int i = 0; i < channels.size(); i++)
    {
        channels[i]->set_value_no_signal(components[i]);
    }
    preview->set_color(color);
}

Color ColorWidget::_channels_color() const
{
    Color color(channels[0]->get_value(), channels[1]->get_value(), channels[2]->get_value(), 1.0);
    if (channels.size() > 3)
    {
        color.a = channels[3]->get_value();
    }
    return color;
}

void ColorWidget::_on_color_changed(const Color &p_color) { emit_value(p_color, false); }

void ColorWidget::_on_popup_closed() { commit_if_editing(picker->get_pick_color()); }

void ColorWidget::_on_channel_changed(double p_value, int64_t p_index)
{
    (void)p_value;
    (void)p_index;
    Color color = _channels_color();
    preview->set_color(color);
    emit_value(color, true);
}
