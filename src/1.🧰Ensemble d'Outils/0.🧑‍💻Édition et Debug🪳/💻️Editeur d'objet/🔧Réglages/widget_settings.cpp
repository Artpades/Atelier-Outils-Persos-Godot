#include "widget_settings.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// --- WidgetSettings ---

void WidgetSettings::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_show_label", "show"), &WidgetSettings::set_show_label);
    ClassDB::bind_method(D_METHOD("get_show_label"), &WidgetSettings::get_show_label);
    ClassDB::bind_method(D_METHOD("set_label_ratio", "ratio"), &WidgetSettings::set_label_ratio);
    ClassDB::bind_method(D_METHOD("get_label_ratio"), &WidgetSettings::get_label_ratio);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_label"), "set_show_label", "get_show_label");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "label_ratio", PROPERTY_HINT_RANGE, "0.1,0.9,0.05"), "set_label_ratio", "get_label_ratio");
}

void WidgetSettings::set_show_label(bool p_show)
{
    show_label = p_show;
    emit_changed();
}
bool WidgetSettings::get_show_label() const { return show_label; }
void WidgetSettings::set_label_ratio(double p_ratio)
{
    label_ratio = CLAMP(p_ratio, 0.1, 0.9);
    emit_changed();
}
double WidgetSettings::get_label_ratio() const { return label_ratio; }

// --- RangeWidgetSettings ---

void RangeWidgetSettings::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_min_value", "value"), &RangeWidgetSettings::set_min_value);
    ClassDB::bind_method(D_METHOD("get_min_value"), &RangeWidgetSettings::get_min_value);
    ClassDB::bind_method(D_METHOD("set_max_value", "value"), &RangeWidgetSettings::set_max_value);
    ClassDB::bind_method(D_METHOD("get_max_value"), &RangeWidgetSettings::get_max_value);
    ClassDB::bind_method(D_METHOD("set_step", "step"), &RangeWidgetSettings::set_step);
    ClassDB::bind_method(D_METHOD("get_step"), &RangeWidgetSettings::get_step);
    ClassDB::bind_method(D_METHOD("set_allow_greater", "allow"), &RangeWidgetSettings::set_allow_greater);
    ClassDB::bind_method(D_METHOD("get_allow_greater"), &RangeWidgetSettings::get_allow_greater);
    ClassDB::bind_method(D_METHOD("set_allow_lesser", "allow"), &RangeWidgetSettings::set_allow_lesser);
    ClassDB::bind_method(D_METHOD("get_allow_lesser"), &RangeWidgetSettings::get_allow_lesser);
    ClassDB::bind_method(D_METHOD("set_suffix", "suffix"), &RangeWidgetSettings::set_suffix);
    ClassDB::bind_method(D_METHOD("get_suffix"), &RangeWidgetSettings::get_suffix);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_value"), "set_min_value", "get_min_value");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_value"), "set_max_value", "get_max_value");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "step", PROPERTY_HINT_RANGE, "0,1000,0.0001,or_greater"), "set_step", "get_step");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_greater"), "set_allow_greater", "get_allow_greater");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_lesser"), "set_allow_lesser", "get_allow_lesser");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "suffix"), "set_suffix", "get_suffix");
}

void RangeWidgetSettings::set_min_value(double p_value)
{
    min_value = p_value;
    emit_changed();
}
double RangeWidgetSettings::get_min_value() const { return min_value; }
void RangeWidgetSettings::set_max_value(double p_value)
{
    max_value = p_value;
    emit_changed();
}
double RangeWidgetSettings::get_max_value() const { return max_value; }
void RangeWidgetSettings::set_step(double p_step)
{
    step = MAX(p_step, 0.0);
    emit_changed();
}
double RangeWidgetSettings::get_step() const { return step; }
void RangeWidgetSettings::set_allow_greater(bool p_allow)
{
    allow_greater = p_allow;
    emit_changed();
}
bool RangeWidgetSettings::get_allow_greater() const { return allow_greater; }
void RangeWidgetSettings::set_allow_lesser(bool p_allow)
{
    allow_lesser = p_allow;
    emit_changed();
}
bool RangeWidgetSettings::get_allow_lesser() const { return allow_lesser; }
void RangeWidgetSettings::set_suffix(const String &p_suffix)
{
    suffix = p_suffix;
    emit_changed();
}
String RangeWidgetSettings::get_suffix() const { return suffix; }

// --- TextureProgressWidgetSettings ---

void TextureProgressWidgetSettings::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_progress_texture", "texture"), &TextureProgressWidgetSettings::set_progress_texture);
    ClassDB::bind_method(D_METHOD("get_progress_texture"), &TextureProgressWidgetSettings::get_progress_texture);
    ClassDB::bind_method(D_METHOD("set_under_texture", "texture"), &TextureProgressWidgetSettings::set_under_texture);
    ClassDB::bind_method(D_METHOD("get_under_texture"), &TextureProgressWidgetSettings::get_under_texture);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "progress_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_progress_texture", "get_progress_texture");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "under_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_under_texture", "get_under_texture");
}

void TextureProgressWidgetSettings::set_progress_texture(const Ref<Texture2D> &p_texture)
{
    progress_texture = p_texture;
    emit_changed();
}
Ref<Texture2D> TextureProgressWidgetSettings::get_progress_texture() const { return progress_texture; }
void TextureProgressWidgetSettings::set_under_texture(const Ref<Texture2D> &p_texture)
{
    under_texture = p_texture;
    emit_changed();
}
Ref<Texture2D> TextureProgressWidgetSettings::get_under_texture() const { return under_texture; }

// --- TextEditWidgetSettings ---

void TextEditWidgetSettings::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_height", "height"), &TextEditWidgetSettings::set_height);
    ClassDB::bind_method(D_METHOD("get_height"), &TextEditWidgetSettings::get_height);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height", PROPERTY_HINT_RANGE, "20,1000,1,or_greater,suffix:px"), "set_height", "get_height");
}

void TextEditWidgetSettings::set_height(double p_height)
{
    height = MAX(p_height, 1.0);
    emit_changed();
}
double TextEditWidgetSettings::get_height() const { return height; }

// --- StructWidgetSettings ---

void StructWidgetSettings::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_step", "step"), &StructWidgetSettings::set_step);
    ClassDB::bind_method(D_METHOD("get_step"), &StructWidgetSettings::get_step);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "step", PROPERTY_HINT_RANGE, "0,1000,0.0001,or_greater"), "set_step", "get_step");
}

void StructWidgetSettings::set_step(double p_step)
{
    step = MAX(p_step, 0.0);
    emit_changed();
}
double StructWidgetSettings::get_step() const { return step; }

// --- ContainerWidgetSettings ---

void ContainerWidgetSettings::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_columns", "columns"), &ContainerWidgetSettings::set_columns);
    ClassDB::bind_method(D_METHOD("get_columns"), &ContainerWidgetSettings::get_columns);
    ClassDB::bind_method(D_METHOD("set_foldable", "foldable"), &ContainerWidgetSettings::set_foldable);
    ClassDB::bind_method(D_METHOD("get_foldable"), &ContainerWidgetSettings::get_foldable);
    ClassDB::bind_method(D_METHOD("set_start_expanded", "expanded"), &ContainerWidgetSettings::set_start_expanded);
    ClassDB::bind_method(D_METHOD("get_start_expanded"), &ContainerWidgetSettings::get_start_expanded);
    ClassDB::bind_method(D_METHOD("set_max_displayed", "max"), &ContainerWidgetSettings::set_max_displayed);
    ClassDB::bind_method(D_METHOD("get_max_displayed"), &ContainerWidgetSettings::get_max_displayed);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "columns", PROPERTY_HINT_RANGE, "1,12,1"), "set_columns", "get_columns");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "foldable"), "set_foldable", "get_foldable");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "start_expanded"), "set_start_expanded", "get_start_expanded");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_displayed", PROPERTY_HINT_RANGE, "1,10000,1,or_greater"), "set_max_displayed", "get_max_displayed");
}

void ContainerWidgetSettings::set_columns(int p_columns)
{
    columns = CLAMP(p_columns, 1, 12);
    emit_changed();
}
int ContainerWidgetSettings::get_columns() const { return columns; }
void ContainerWidgetSettings::set_foldable(bool p_foldable)
{
    foldable = p_foldable;
    emit_changed();
}
bool ContainerWidgetSettings::get_foldable() const { return foldable; }
void ContainerWidgetSettings::set_start_expanded(bool p_expanded)
{
    start_expanded = p_expanded;
    emit_changed();
}
bool ContainerWidgetSettings::get_start_expanded() const { return start_expanded; }
void ContainerWidgetSettings::set_max_displayed(int p_max)
{
    max_displayed = MAX(p_max, 1);
    emit_changed();
}
int ContainerWidgetSettings::get_max_displayed() const { return max_displayed; }
