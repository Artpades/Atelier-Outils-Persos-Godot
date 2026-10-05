#ifndef SCALAR_WIDGETS_HPP
#define SCALAR_WIDGETS_HPP

#include "property_widget.hpp"

#include <godot_cpp/classes/base_button.hpp>
#include <godot_cpp/classes/button_group.hpp>
#include <godot_cpp/classes/color_picker_button.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/range.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

// Widgets des types scalaires (Phase 1). Les contrôles enfants sont créés et possédés par le widget
// (libérés avec lui) : les pointeurs bruts vers eux restent donc valides tant que le widget existe.

namespace godot
{
// bool : CheckBox / CheckButton.
class BoolWidget : public PropertyWidget
{
    GDCLASS(BoolWidget, PropertyWidget)

  private:
    BaseButton *button = nullptr;
    void _on_toggled(bool p_pressed);

  protected:
    static void _bind_methods() {}
    Control *build_editor() override;
    void pull_value(const Variant &p_value) override;
};

// int / float : SpinBox, HSlider, VSlider (éditables) ; ProgressBar, TextureProgressBar (lecture seule).
// Bornes : RangeWidgetSettings > indice PROPERTY_HINT_RANGE > défauts (0-100 pour sliders et barres, sans borne pour SpinBox).
class NumberWidget : public PropertyWidget
{
    GDCLASS(NumberWidget, PropertyWidget)

  public:
    // Contenu d'un indice "min,max[,step][,or_greater][,or_less][,suffix:x]".
    struct RangeSpec
    {
        bool has_bounds = false;
        double min = 0.0;
        double max = 100.0;
        double step = 0.0;
        bool or_greater = false;
        bool or_less = false;
        String suffix;
    };
    static RangeSpec parse_range_hint(const String &p_hint);

  private:
    Range *range = nullptr;
    Label *value_text = nullptr; // valeur affichée à côté d'un slider
    bool is_int = false;
    bool dragging = false;

    void _update_value_text();
    Variant _current_value() const;
    void _on_value_changed(double p_value);
    void _on_drag_started();
    void _on_drag_ended(bool p_value_changed);

  protected:
    static void _bind_methods() {}
    Control *build_editor() override;
    void pull_value(const Variant &p_value) override;
    StringName get_settings_class() const override { return "RangeWidgetSettings"; }
};

// String / StringName : LineEdit, TextEdit.
class TextWidget : public PropertyWidget
{
    GDCLASS(TextWidget, PropertyWidget)

  private:
    LineEdit *line_edit = nullptr;
    TextEdit *text_edit = nullptr;

    Variant _wrap(const String &p_text) const; // StringName si la propriété en est une
    String _current_text() const;
    void _on_text_changed(const String &p_text);
    void _on_text_edit_changed();
    void _on_text_submitted(const String &p_text);
    void _on_focus_exited();

  protected:
    static void _bind_methods() {}
    Control *build_editor() override;
    void pull_value(const Variant &p_value) override;
    StringName get_settings_class() const override { return "TextEditWidgetSettings"; }
};

// PROPERTY_HINT_ENUM (int ou String, format "Nom:valeur") : OptionButton, ou liste de CheckButton/CheckBox
// exclusifs (ButtonGroup). PROPERTY_HINT_FLAGS : liste de CheckBox/CheckButton sans groupe (masque de bits).
class EnumWidget : public PropertyWidget
{
    GDCLASS(EnumWidget, PropertyWidget)

  public:
    // Analyse un hint_string d'enum ou de flags. Valeurs implicites : suivante (enum), 1 << index (flags),
    // le libellé lui-même (enum texte).
    static void parse_items(const String &p_hint, bool p_flags, bool p_text, PackedStringArray &r_labels, Array &r_values);

  private:
    PackedStringArray labels;
    Array values;
    bool flags = false;
    bool text = false;
    OptionButton *option = nullptr;
    Vector<BaseButton *> buttons;
    Ref<ButtonGroup> group;

    bool _same_value(const Variant &p_a, const Variant &p_b) const;
    Variant _wrap(const Variant &p_value) const;
    void _on_item_selected(int64_t p_index);
    void _on_item_toggled(bool p_pressed, int64_t p_index);
    void _on_flag_toggled(bool p_pressed, int64_t p_index);

  protected:
    static void _bind_methods() {}
    Control *build_editor() override;
    void pull_value(const Variant &p_value) override;
};

// Color : ColorPickerButton, ou un SpinBox par canal + ColorRect d'aperçu.
class ColorWidget : public PropertyWidget
{
    GDCLASS(ColorWidget, PropertyWidget)

  private:
    ColorPickerButton *picker = nullptr;
    ColorRect *preview = nullptr;
    Vector<SpinBox *> channels;
    bool edit_alpha = true;

    Color _channels_color() const;
    void _on_color_changed(const Color &p_color);
    void _on_popup_closed();
    void _on_channel_changed(double p_value, int64_t p_index);

  protected:
    static void _bind_methods() {}
    Control *build_editor() override;
    void pull_value(const Variant &p_value) override;
};
} // namespace godot

#endif // SCALAR_WIDGETS_HPP
