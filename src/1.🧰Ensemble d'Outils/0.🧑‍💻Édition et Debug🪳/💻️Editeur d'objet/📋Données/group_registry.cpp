#include "group_registry.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// --- GroupEntry ---

void GroupEntry::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_group_name", "name"), &GroupEntry::set_group_name);
    ClassDB::bind_method(D_METHOD("get_group_name"), &GroupEntry::get_group_name);
    ClassDB::bind_method(D_METHOD("set_container_type", "type"), &GroupEntry::set_container_type);
    ClassDB::bind_method(D_METHOD("get_container_type"), &GroupEntry::get_container_type);
    ClassDB::bind_method(D_METHOD("set_columns", "columns"), &GroupEntry::set_columns);
    ClassDB::bind_method(D_METHOD("get_columns"), &GroupEntry::get_columns);
    ClassDB::bind_method(D_METHOD("set_is_vertical", "vertical"), &GroupEntry::set_is_vertical);
    ClassDB::bind_method(D_METHOD("get_is_vertical"), &GroupEntry::get_is_vertical);
    ClassDB::bind_method(D_METHOD("set_data_names", "names"), &GroupEntry::set_data_names);
    ClassDB::bind_method(D_METHOD("get_data_names"), &GroupEntry::get_data_names);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "group_name"), "set_group_name", "get_group_name");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "container_type", PROPERTY_HINT_ENUM, "VBoxContainer:0,HBoxContainer:1,GridContainer:2,PanelContainer:3"), "set_container_type",
                 "get_container_type");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "columns", PROPERTY_HINT_RANGE, "1,12,1"), "set_columns", "get_columns");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_vertical"), "set_is_vertical", "get_is_vertical");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "data_names"), "set_data_names", "get_data_names");

    BIND_ENUM_CONSTANT(CONTAINER_VBOX);
    BIND_ENUM_CONSTANT(CONTAINER_HBOX);
    BIND_ENUM_CONSTANT(CONTAINER_GRID);
    BIND_ENUM_CONSTANT(CONTAINER_PANEL);
}

void GroupEntry::set_group_name(const String &p_name)
{
    group_name = p_name;
    emit_changed();
}
String GroupEntry::get_group_name() const { return group_name; }
void GroupEntry::set_container_type(ContainerType p_type)
{
    container_type = p_type;
    if (p_type == CONTAINER_VBOX)
    {
        is_vertical = true;
    }
    else if (p_type == CONTAINER_HBOX)
    {
        is_vertical = false;
    }
    emit_changed();
}
GroupEntry::ContainerType GroupEntry::get_container_type() const { return container_type; }
void GroupEntry::set_columns(int p_columns)
{
    columns = CLAMP(p_columns, 1, 12);
    emit_changed();
}
int GroupEntry::get_columns() const { return columns; }
void GroupEntry::set_is_vertical(bool p_vertical)
{
    is_vertical = p_vertical;
    emit_changed();
}
bool GroupEntry::get_is_vertical() const { return is_vertical; }
void GroupEntry::set_data_names(const PackedStringArray &p_names)
{
    data_names = p_names;
    emit_changed();
}
PackedStringArray GroupEntry::get_data_names() const { return data_names; }

// --- GroupRegistry ---

void GroupRegistry::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_groups", "groups"), &GroupRegistry::set_groups);
    ClassDB::bind_method(D_METHOD("get_groups"), &GroupRegistry::get_groups);
    ClassDB::bind_method(D_METHOD("find", "group_name"), &GroupRegistry::find);
    ClassDB::bind_method(D_METHOD("ensure", "group_name"), &GroupRegistry::ensure);
    ClassDB::bind_method(D_METHOD("group_of", "data_name"), &GroupRegistry::group_of);
    ClassDB::bind_method(D_METHOD("remove_data_name", "data_name"), &GroupRegistry::remove_data_name);
    ClassDB::bind_method(D_METHOD("assign", "data_name", "group"), &GroupRegistry::assign);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "groups", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":GroupEntry"),
                 "set_groups", "get_groups");
}

void GroupRegistry::set_groups(const TypedArray<GroupEntry> &p_groups)
{
    groups = p_groups;
    emit_changed();
}
TypedArray<GroupEntry> GroupRegistry::get_groups() const { return groups; }

Ref<GroupEntry> GroupRegistry::find(const String &p_group_name) const
{
    for (int i = 0; i < groups.size(); i++)
    {
        Ref<GroupEntry> group = groups[i];
        if (group.is_valid() && group->get_group_name() == p_group_name)
        {
            return group;
        }
    }
    return Ref<GroupEntry>();
}

Ref<GroupEntry> GroupRegistry::ensure(const String &p_group_name)
{
    Ref<GroupEntry> group = find(p_group_name);
    if (group.is_null())
    {
        group.instantiate();
        group->set_group_name(p_group_name);
        groups.push_back(group);
        emit_changed();
    }
    return group;
}

Ref<GroupEntry> GroupRegistry::group_of(const String &p_data_name) const
{
    for (int i = 0; i < groups.size(); i++)
    {
        Ref<GroupEntry> group = groups[i];
        if (group.is_valid() && group->get_data_names().has(p_data_name))
        {
            return group;
        }
    }
    return Ref<GroupEntry>();
}

void GroupRegistry::remove_data_name(const String &p_data_name)
{
    for (int i = 0; i < groups.size(); i++)
    {
        Ref<GroupEntry> group = groups[i];
        if (group.is_null())
        {
            continue;
        }
        PackedStringArray names = group->get_data_names();
        if (names.has(p_data_name))
        {
            while (names.has(p_data_name))
            {
                names.remove_at(names.find(p_data_name));
            }
            group->set_data_names(names);
        }
    }
}

void GroupRegistry::assign(const String &p_data_name, const Ref<GroupEntry> &p_group)
{
    remove_data_name(p_data_name);
    if (p_group.is_valid())
    {
        PackedStringArray names = p_group->get_data_names();
        names.push_back(p_data_name);
        p_group->set_data_names(names);
    }
    emit_changed();
}
