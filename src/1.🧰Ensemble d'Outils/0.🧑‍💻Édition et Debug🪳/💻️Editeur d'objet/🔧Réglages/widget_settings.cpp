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
    ClassDB::bind_method(D_METHOD("set_style", "style"), &WidgetSettings::set_style);
    ClassDB::bind_method(D_METHOD("get_style"), &WidgetSettings::get_style);
    ClassDB::bind_method(D_METHOD("set_style_target", "target"), &WidgetSettings::set_style_target);
    ClassDB::bind_method(D_METHOD("get_style_target"), &WidgetSettings::get_style_target);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "style", PROPERTY_HINT_RESOURCE_TYPE, "InterfaceStyle"), "set_style", "get_style");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "style_target", PROPERTY_HINT_ENUM, U"Widget entier:0,Valeur seulement:1,Libellé seulement:2"), "set_style_target", "get_style_target");
    BIND_ENUM_CONSTANT(STYLE_WIDGET);
    BIND_ENUM_CONSTANT(STYLE_VALUE);
    BIND_ENUM_CONSTANT(STYLE_LABEL);
    ClassDB::bind_method(D_METHOD("set_icon", "icon"), &WidgetSettings::set_icon);
    ClassDB::bind_method(D_METHOD("get_icon"), &WidgetSettings::get_icon);
    ClassDB::bind_method(D_METHOD("set_icon_position", "position"), &WidgetSettings::set_icon_position);
    ClassDB::bind_method(D_METHOD("get_icon_position"), &WidgetSettings::get_icon_position);
    ClassDB::bind_method(D_METHOD("set_icon_size", "size"), &WidgetSettings::set_icon_size);
    ClassDB::bind_method(D_METHOD("get_icon_size"), &WidgetSettings::get_icon_size);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "icon", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_icon", "get_icon");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "icon_position", PROPERTY_HINT_ENUM, U"Avant le libellé:0,Avant la valeur:1"), "set_icon_position", "get_icon_position");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "icon_size", PROPERTY_HINT_RANGE, "8,128,1,suffix:px"), "set_icon_size", "get_icon_size");
    BIND_ENUM_CONSTANT(ICON_BEFORE_LABEL);
    BIND_ENUM_CONSTANT(ICON_BEFORE_VALUE);
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
void WidgetSettings::set_style(const Ref<InterfaceStyle> &p_style)
{
    style = p_style;
    emit_changed();
}
Ref<InterfaceStyle> WidgetSettings::get_style() const { return style; }
void WidgetSettings::set_style_target(StyleTarget p_target)
{
    style_target = p_target;
    emit_changed();
}
WidgetSettings::StyleTarget WidgetSettings::get_style_target() const { return style_target; }
void WidgetSettings::set_icon(const Ref<Texture2D> &p_icon)
{
    icon = p_icon;
    emit_changed();
}
Ref<Texture2D> WidgetSettings::get_icon() const { return icon; }
void WidgetSettings::set_icon_position(IconPosition p_position)
{
    icon_position = p_position;
    emit_changed();
}
WidgetSettings::IconPosition WidgetSettings::get_icon_position() const { return icon_position; }
void WidgetSettings::set_icon_size(int p_size)
{
    icon_size = CLAMP(p_size, 8, 128);
    emit_changed();
}
int WidgetSettings::get_icon_size() const { return icon_size; }

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

// --- TableWidgetSettings ---

void TableWidgetSettings::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_transpose", "transpose"), &TableWidgetSettings::set_transpose);
    ClassDB::bind_method(D_METHOD("get_transpose"), &TableWidgetSettings::get_transpose);
    ClassDB::bind_method(D_METHOD("set_table_columns", "columns"), &TableWidgetSettings::set_table_columns);
    ClassDB::bind_method(D_METHOD("get_table_columns"), &TableWidgetSettings::get_table_columns);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "transpose"), "set_transpose", "get_transpose");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "table_columns"), "set_table_columns", "get_table_columns");
}

void TableWidgetSettings::set_transpose(bool p_transpose)
{
    transpose = p_transpose;
    emit_changed();
}
bool TableWidgetSettings::get_transpose() const { return transpose; }
void TableWidgetSettings::set_table_columns(const PackedStringArray &p_columns)
{
    table_columns = p_columns;
    emit_changed();
}
PackedStringArray TableWidgetSettings::get_table_columns() const { return table_columns; }

// --- PreviewSettings ---

void PreviewSettings::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_preview_class", "class_name"), &PreviewSettings::set_preview_class);
    ClassDB::bind_method(D_METHOD("get_preview_class"), &PreviewSettings::get_preview_class);
    ClassDB::bind_method(D_METHOD("set_source", "source"), &PreviewSettings::set_source);
    ClassDB::bind_method(D_METHOD("get_source"), &PreviewSettings::get_source);
    ClassDB::bind_method(D_METHOD("get_stored_source"), &PreviewSettings::get_stored_source);
    ClassDB::bind_method(D_METHOD("set_height", "height"), &PreviewSettings::set_height);
    ClassDB::bind_method(D_METHOD("get_height"), &PreviewSettings::get_height);
    ClassDB::bind_method(D_METHOD("set_update_mode", "mode"), &PreviewSettings::set_update_mode);
    ClassDB::bind_method(D_METHOD("get_update_mode"), &PreviewSettings::get_update_mode);
    ClassDB::bind_method(D_METHOD("set_duplicate_scripts", "duplicate"), &PreviewSettings::set_duplicate_scripts);
    ClassDB::bind_method(D_METHOD("get_duplicate_scripts"), &PreviewSettings::get_duplicate_scripts);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "preview_class"), "set_preview_class", "get_preview_class");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source", PROPERTY_HINT_RESOURCE_TYPE, "ArgumentBinding"), "set_source", "get_stored_source");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "height", PROPERTY_HINT_RANGE, "32,1024,1,suffix:px"), "set_height", "get_height");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "update_mode", PROPERTY_HINT_ENUM, U"Au changement de la source:0,Chaque image:1"), "set_update_mode", "get_update_mode");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "duplicate_scripts"), "set_duplicate_scripts", "get_duplicate_scripts");
    BIND_ENUM_CONSTANT(UPDATE_ON_CHANGE);
    BIND_ENUM_CONSTANT(UPDATE_EVERY_FRAME);
}

void PreviewSettings::set_preview_class(const StringName &p_class)
{
    preview_class = p_class;
    emit_changed();
}
StringName PreviewSettings::get_preview_class() const { return preview_class; }
void PreviewSettings::set_source(const Ref<ArgumentBinding> &p_source)
{
    source = p_source;
    emit_changed();
}
Ref<ArgumentBinding> PreviewSettings::get_source()
{
    if (source.is_null())
    {
        source.instantiate();
        source->set_source(ArgumentBinding::SOURCE_PROPERTY); // sans chemin : l'objet affiché
    }
    return source;
}
// Getter de la propriété : sans création (Godot l'appelle aussi pour calculer la valeur par défaut).
Ref<ArgumentBinding> PreviewSettings::get_stored_source() const { return source; }
void PreviewSettings::set_height(int p_height)
{
    height = CLAMP(p_height, 32, 1024);
    emit_changed();
}
int PreviewSettings::get_height() const { return height; }
void PreviewSettings::set_update_mode(UpdateMode p_mode)
{
    update_mode = p_mode;
    emit_changed();
}
PreviewSettings::UpdateMode PreviewSettings::get_update_mode() const { return update_mode; }
void PreviewSettings::set_duplicate_scripts(bool p_duplicate)
{
    duplicate_scripts = p_duplicate;
    emit_changed();
}
bool PreviewSettings::get_duplicate_scripts() const { return duplicate_scripts; }
