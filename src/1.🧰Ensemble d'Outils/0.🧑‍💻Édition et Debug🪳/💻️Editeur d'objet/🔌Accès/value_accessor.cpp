#include "value_accessor.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

void ValueAccessor::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("setup", "target", "property"), &ValueAccessor::setup);
    ClassDB::bind_method(D_METHOD("child", "key", "info", "display_name"), &ValueAccessor::child);
    ClassDB::bind_method(D_METHOD("with_info", "info"), &ValueAccessor::with_info);
    ClassDB::bind_method(D_METHOD("get_target"), &ValueAccessor::get_target);
    ClassDB::bind_method(D_METHOD("is_valid"), &ValueAccessor::is_valid);
    ClassDB::bind_method(D_METHOD("get_property"), &ValueAccessor::get_property);
    ClassDB::bind_method(D_METHOD("get_path"), &ValueAccessor::get_path);
    ClassDB::bind_method(D_METHOD("is_root"), &ValueAccessor::is_root);
    ClassDB::bind_method(D_METHOD("get_property_info"), &ValueAccessor::get_property_info);
    ClassDB::bind_method(D_METHOD("get_display_name"), &ValueAccessor::get_display_name);
    ClassDB::bind_method(D_METHOD("is_read_only"), &ValueAccessor::is_read_only);
    ClassDB::bind_method(D_METHOD("is_undoable"), &ValueAccessor::is_undoable);
    ClassDB::bind_method(D_METHOD("read_root"), &ValueAccessor::read_root);
    ClassDB::bind_method(D_METHOD("read"), &ValueAccessor::read);
    ClassDB::bind_method(D_METHOD("write", "value"), &ValueAccessor::write);
    ClassDB::bind_static_method("ValueAccessor", D_METHOD("find_property_info", "target", "property"), &ValueAccessor::find_property_info);
}

Dictionary ValueAccessor::find_property_info(Object *p_target, const StringName &p_property)
{
    if (!p_target)
    {
        return Dictionary();
    }
    TypedArray<Dictionary> list = p_target->get_property_list();
    for (int i = 0; i < list.size(); i++)
    {
        Dictionary info = list[i];
        if (StringName(info.get("name", "")) == p_property)
        {
            return info;
        }
    }
    return Dictionary();
}

Variant ValueAccessor::copy_collection(const Variant &p_value)
{
    Variant::Type type = p_value.get_type();
    if (type == Variant::ARRAY || type == Variant::DICTIONARY || (type >= Variant::PACKED_BYTE_ARRAY && type <= Variant::PACKED_VECTOR4_ARRAY))
    {
        return p_value.duplicate(false); // garde le typage d'un Array / Dictionary typé
    }
    return p_value;
}

Variant ValueAccessor::_with_value(const Variant &p_container, const Array &p_path, int p_index, const Variant &p_value)
{
    if (p_index >= p_path.size())
    {
        return p_value;
    }
    Variant copy = copy_collection(p_container);
    bool valid = false;
    Variant child_value = copy.get(p_path[p_index], &valid);
    copy.set(p_path[p_index], _with_value(child_value, p_path, p_index + 1, p_value), &valid);
    return copy;
}

void ValueAccessor::setup(Object *p_target, const StringName &p_property)
{
    target_id = p_target ? ObjectID(p_target->get_instance_id()) : ObjectID();
    property = p_property;
    path = Array();
    property_info = find_property_info(p_target, p_property);
    display_name = String();
}

Ref<ValueAccessor> ValueAccessor::child(const Variant &p_key, const Dictionary &p_info, const String &p_display_name) const
{
    Ref<ValueAccessor> accessor;
    accessor.instantiate();
    accessor->target_id = target_id;
    accessor->property = property;
    accessor->path = path.duplicate();
    accessor->path.push_back(p_key);
    accessor->property_info = p_info;
    accessor->display_name = p_display_name;
    accessor->undoable = undoable;
    // Une valeur contenue dans une valeur en lecture seule l'est aussi.
    if (is_read_only())
    {
        accessor->property_info["usage"] = int(p_info.get("usage", 0)) | PROPERTY_USAGE_READ_ONLY;
    }
    return accessor;
}

Ref<ValueAccessor> ValueAccessor::with_info(const Dictionary &p_info) const
{
    Ref<ValueAccessor> accessor;
    accessor.instantiate();
    accessor->target_id = target_id;
    accessor->property = property;
    accessor->path = path.duplicate();
    accessor->property_info = p_info;
    accessor->display_name = display_name;
    accessor->undoable = undoable;
    return accessor;
}

Object *ValueAccessor::get_target() const { return ObjectDB::get_instance(target_id); }
bool ValueAccessor::is_valid() const { return get_target() != nullptr; }
StringName ValueAccessor::get_property() const { return property; }
Array ValueAccessor::get_path() const { return path; }
bool ValueAccessor::is_root() const { return path.is_empty(); }
Dictionary ValueAccessor::get_property_info() const { return property_info; }
void ValueAccessor::set_property_info(const Dictionary &p_info) { property_info = p_info; }
String ValueAccessor::get_display_name() const { return display_name.is_empty() ? String(property).capitalize() : display_name; }
void ValueAccessor::set_display_name(const String &p_name) { display_name = p_name; }
bool ValueAccessor::is_read_only() const { return (int(property_info.get("usage", 0)) & PROPERTY_USAGE_READ_ONLY) != 0; }
bool ValueAccessor::is_undoable() const { return undoable; }
void ValueAccessor::set_undoable(bool p_undoable) { undoable = p_undoable; }

Variant ValueAccessor::read_root() const
{
    Object *target = get_target();
    return target ? target->get(property) : Variant();
}

Variant ValueAccessor::read() const
{
    Variant value = read_root();
    for (int i = 0; i < path.size(); i++)
    {
        bool valid = false;
        value = value.get(path[i], &valid);
        if (!valid)
        {
            return Variant();
        }
    }
    return value;
}

void ValueAccessor::write(const Variant &p_value)
{
    Object *target = get_target();
    if (!target)
    {
        return;
    }
    target->set(property, path.is_empty() ? p_value : _with_value(read_root(), path, 0, p_value));
    if (Resource *resource = Object::cast_to<Resource>(target))
    {
        resource->emit_changed();
    }
}
