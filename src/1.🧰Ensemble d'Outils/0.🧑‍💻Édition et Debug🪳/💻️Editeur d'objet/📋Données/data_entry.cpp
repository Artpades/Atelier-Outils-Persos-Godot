#include "data_entry.hpp"
#include "widget_table.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void DataEntry::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_data_name", "name"), &DataEntry::set_data_name);
    ClassDB::bind_method(D_METHOD("get_data_name"), &DataEntry::get_data_name);
    ClassDB::bind_method(D_METHOD("set_member", "member"), &DataEntry::set_member);
    ClassDB::bind_method(D_METHOD("get_member"), &DataEntry::get_member);
    ClassDB::bind_method(D_METHOD("set_kind", "kind"), &DataEntry::set_kind);
    ClassDB::bind_method(D_METHOD("get_kind"), &DataEntry::get_kind);
    ClassDB::bind_method(D_METHOD("set_widget_choice", "choice"), &DataEntry::set_widget_choice);
    ClassDB::bind_method(D_METHOD("get_widget_choice"), &DataEntry::get_widget_choice);
    ClassDB::bind_method(D_METHOD("set_custom_widget_class", "class_name"), &DataEntry::set_custom_widget_class);
    ClassDB::bind_method(D_METHOD("get_custom_widget_class"), &DataEntry::get_custom_widget_class);
    ClassDB::bind_method(D_METHOD("set_settings", "settings"), &DataEntry::set_settings);
    ClassDB::bind_method(D_METHOD("get_settings"), &DataEntry::get_settings);
    ClassDB::bind_method(D_METHOD("set_is_vertical", "vertical"), &DataEntry::set_is_vertical);
    ClassDB::bind_method(D_METHOD("get_is_vertical"), &DataEntry::get_is_vertical);
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &DataEntry::set_enabled);
    ClassDB::bind_method(D_METHOD("get_enabled"), &DataEntry::get_enabled);
    ClassDB::bind_method(D_METHOD("set_is_missing", "missing"), &DataEntry::set_is_missing);
    ClassDB::bind_method(D_METHOD("get_is_missing"), &DataEntry::get_is_missing);
    ClassDB::bind_method(D_METHOD("set_links", "links"), &DataEntry::set_links);
    ClassDB::bind_method(D_METHOD("get_links"), &DataEntry::get_links);
    ClassDB::bind_method(D_METHOD("resolved_choice"), &DataEntry::resolved_choice);

    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "data_name"), "set_data_name", "get_data_name");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "member", PROPERTY_HINT_ENUM, U"Propriété:0,Méthode:1,Signal:2,Aperçu:3"), "set_member", "get_member");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "kind", PROPERTY_HINT_ENUM, WidgetTable::kind_hint_string()), "set_kind", "get_kind");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "widget_choice", PROPERTY_HINT_ENUM, WidgetTable::choice_hint_string()), "set_widget_choice", "get_widget_choice");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "custom_widget_class"), "set_custom_widget_class", "get_custom_widget_class");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "settings", PROPERTY_HINT_RESOURCE_TYPE, "WidgetSettings"), "set_settings", "get_settings");
    ClassDB::bind_method(D_METHOD("set_tooltip", "tooltip"), &DataEntry::set_tooltip);
    ClassDB::bind_method(D_METHOD("get_tooltip"), &DataEntry::get_tooltip);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "tooltip", PROPERTY_HINT_MULTILINE_TEXT), "set_tooltip", "get_tooltip");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_vertical"), "set_is_vertical", "get_is_vertical");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "get_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_missing"), "set_is_missing", "get_is_missing");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "links", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":SignalLink"), "set_links",
                 "get_links");

    BIND_ENUM_CONSTANT(MEMBER_PROPERTY);
    BIND_ENUM_CONSTANT(MEMBER_METHOD);
    BIND_ENUM_CONSTANT(MEMBER_SIGNAL);
    BIND_ENUM_CONSTANT(MEMBER_PREVIEW);

    BIND_ENUM_CONSTANT(KIND_UNSUPPORTED);
    BIND_ENUM_CONSTANT(KIND_BOOL);
    BIND_ENUM_CONSTANT(KIND_NUMBER);
    BIND_ENUM_CONSTANT(KIND_TEXT);
    BIND_ENUM_CONSTANT(KIND_ENUM);
    BIND_ENUM_CONSTANT(KIND_FLAGS);
    BIND_ENUM_CONSTANT(KIND_COLOR);
    BIND_ENUM_CONSTANT(KIND_METHOD);
    BIND_ENUM_CONSTANT(KIND_SIGNAL);
    BIND_ENUM_CONSTANT(KIND_OBJECT);
    BIND_ENUM_CONSTANT(KIND_NODE_PATH);
    BIND_ENUM_CONSTANT(KIND_ARRAY);
    BIND_ENUM_CONSTANT(KIND_DICTIONARY);
    BIND_ENUM_CONSTANT(KIND_STRUCT);
    BIND_ENUM_CONSTANT(KIND_VARIANT);
    BIND_ENUM_CONSTANT(KIND_RID);
    BIND_ENUM_CONSTANT(KIND_PACKED_ARRAY);
    BIND_ENUM_CONSTANT(KIND_PREVIEW);

    BIND_ENUM_CONSTANT(WIDGET_AUTO);
    BIND_ENUM_CONSTANT(WIDGET_CHECK_BOX);
    BIND_ENUM_CONSTANT(WIDGET_CHECK_BUTTON);
    BIND_ENUM_CONSTANT(WIDGET_LABEL);
    BIND_ENUM_CONSTANT(WIDGET_SPIN_BOX);
    BIND_ENUM_CONSTANT(WIDGET_H_SLIDER);
    BIND_ENUM_CONSTANT(WIDGET_V_SLIDER);
    BIND_ENUM_CONSTANT(WIDGET_PROGRESS_BAR);
    BIND_ENUM_CONSTANT(WIDGET_TEXTURE_PROGRESS_BAR);
    BIND_ENUM_CONSTANT(WIDGET_LINE_EDIT);
    BIND_ENUM_CONSTANT(WIDGET_TEXT_EDIT);
    BIND_ENUM_CONSTANT(WIDGET_OPTION_BUTTON);
    BIND_ENUM_CONSTANT(WIDGET_CHECK_BUTTON_LIST);
    BIND_ENUM_CONSTANT(WIDGET_COLOR_PICKER_BUTTON);
    BIND_ENUM_CONSTANT(WIDGET_COLOR_RECT_CHANNELS);
    BIND_ENUM_CONSTANT(WIDGET_BUTTON);
    BIND_ENUM_CONSTANT(WIDGET_BOX_CONTAINER);
    BIND_ENUM_CONSTANT(WIDGET_GRID_CONTAINER);
    BIND_ENUM_CONSTANT(WIDGET_PATH_BUTTON);
    BIND_ENUM_CONSTANT(WIDGET_STRUCT_SPIN_BOXES);
    BIND_ENUM_CONSTANT(WIDGET_CUSTOM);
    BIND_ENUM_CONSTANT(WIDGET_VARIANT_SELECTOR);
    BIND_ENUM_CONSTANT(WIDGET_CHECK_BOX_LIST);
    BIND_ENUM_CONSTANT(WIDGET_FLOW_CONTAINER);
    BIND_ENUM_CONSTANT(WIDGET_TAB_CONTAINER);
    BIND_ENUM_CONSTANT(WIDGET_TABLE);
    BIND_ENUM_CONSTANT(WIDGET_PREVIEW);

    BIND_ENUM_CONSTANT(SCOPE_AUTO);
    BIND_ENUM_CONSTANT(SCOPE_SCRIPT_ONLY);
    BIND_ENUM_CONSTANT(SCOPE_ALL);
    BIND_ENUM_CONSTANT(SCOPE_CLASS_ONLY);
}

void DataEntry::_validate_property(PropertyInfo &p_property) const
{
    if (p_property.name == StringName("widget_choice"))
    {
        p_property.hint_string = WidgetTable::choice_hint_string_for(kind);
    }
}

void DataEntry::set_data_name(const StringName &p_name) { data_name = p_name; }
StringName DataEntry::get_data_name() const { return data_name; }
void DataEntry::set_member(MemberKind p_member) { member = p_member; }
DataEntry::MemberKind DataEntry::get_member() const { return member; }
void DataEntry::set_kind(DataKind p_kind)
{
    if (kind != p_kind)
    {
        kind = p_kind;
        notify_property_list_changed(); // les choix de widget proposés changent
    }
}
DataEntry::DataKind DataEntry::get_kind() const { return kind; }
void DataEntry::set_widget_choice(WidgetChoice p_choice) { widget_choice = p_choice; }
DataEntry::WidgetChoice DataEntry::get_widget_choice() const { return widget_choice; }
void DataEntry::set_custom_widget_class(const StringName &p_class) { custom_widget_class = p_class; }
StringName DataEntry::get_custom_widget_class() const { return custom_widget_class; }
void DataEntry::set_settings(const Ref<WidgetSettings> &p_settings) { settings = p_settings; }
Ref<WidgetSettings> DataEntry::get_settings() const { return settings; }
void DataEntry::set_tooltip(const String &p_tooltip)
{
    tooltip = p_tooltip;
    emit_changed();
}
String DataEntry::get_tooltip() const { return tooltip; }
void DataEntry::set_is_vertical(bool p_vertical) { is_vertical = p_vertical; }
bool DataEntry::get_is_vertical() const { return is_vertical; }
void DataEntry::set_enabled(bool p_enabled) { enabled = p_enabled; }
bool DataEntry::get_enabled() const { return enabled; }
void DataEntry::set_is_missing(bool p_missing) { is_missing = p_missing; }
bool DataEntry::get_is_missing() const { return is_missing; }
void DataEntry::set_links(const TypedArray<SignalLink> &p_links)
{
    links = p_links;
    emit_changed();
}
TypedArray<SignalLink> DataEntry::get_links() const { return links; }

DataEntry::WidgetChoice DataEntry::resolved_choice() const
{
    if (widget_choice == WIDGET_AUTO || !WidgetTable::is_allowed(kind, widget_choice))
    {
        return WidgetTable::default_for(kind);
    }
    return widget_choice;
}
