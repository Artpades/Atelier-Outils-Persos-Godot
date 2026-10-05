#include "type_utils.hpp"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

bool TypeUtils::class_inherits(const String &p_class, const String &p_base)
{
    if (p_class.is_empty() || p_base.is_empty())
    {
        return false;
    }
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    if (cdb->class_exists(p_class))
    {
        return p_class == p_base || (cdb->class_exists(p_base) && cdb->is_parent_class(p_class, p_base));
    }

    // class_name : on remonte les "base" de la liste des classes globales jusqu'à une classe native.
    TypedArray<Dictionary> globals = ProjectSettings::get_singleton()->get_global_class_list();
    Dictionary base_of;
    for (int i = 0; i < globals.size(); i++)
    {
        Dictionary global = globals[i];
        base_of[global.get("class", "")] = global.get("base", "");
    }
    String current = p_class;
    for (int guard = 0; guard < 64 && !current.is_empty(); guard++)
    {
        if (current == p_base)
        {
            return true;
        }
        if (!base_of.has(current))
        {
            return cdb->class_exists(current) && cdb->class_exists(p_base) && cdb->is_parent_class(current, p_base);
        }
        current = String(base_of[current]);
    }
    return false;
}

bool TypeUtils::is_compatible(const Dictionary &p_arg, const Dictionary &p_param)
{
    int arg_type = p_arg.get("type", 0);
    int param_type = p_param.get("type", 0);
    if (arg_type == Variant::NIL || param_type == Variant::NIL)
    {
        return true;
    }
    if (arg_type == Variant::INT && param_type == Variant::FLOAT)
    {
        return true;
    }
    if (arg_type != param_type)
    {
        return false;
    }
    if (arg_type == Variant::OBJECT)
    {
        String arg_class = p_arg.get("class_name", "");
        String param_class = p_param.get("class_name", "");
        return param_class.is_empty() || arg_class.is_empty() || class_inherits(arg_class, param_class);
    }
    return true;
}
