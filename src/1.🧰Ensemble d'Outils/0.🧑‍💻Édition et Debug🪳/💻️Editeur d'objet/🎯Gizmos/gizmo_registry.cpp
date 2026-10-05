#include "gizmo_registry.hpp"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// =====================================================================================================
// GizmoCatalog
// =====================================================================================================

namespace
{
const char *const GIZMO_CLASSES[] = {"Gizmos2D", "Gizmos3D"};
}

PackedStringArray GizmoCatalog::functions()
{
    PackedStringArray result;
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    for (const char *cls : GIZMO_CLASSES)
    {
        if (!cdb->class_exists(cls))
        {
            continue;
        }
        TypedArray<Dictionary> methods = cdb->class_get_method_list(cls, true);
        for (int i = 0; i < methods.size(); i++)
        {
            Dictionary info = methods[i];
            String name = info.get("name", "");
            Array args = info.get("args", Array());
            bool is_static = (int(info.get("flags", 0)) & METHOD_FLAG_STATIC) != 0;
            // Fonction de dessin : statique, draw_*, premier paramètre = RID du gizmo.
            if (is_static && name.begins_with("draw_") && args.size() > 0 && int(Dictionary(args[0]).get("type", 0)) == Variant::RID)
            {
                result.push_back(String(cls) + "." + name);
            }
        }
    }
    result.sort();
    return result;
}

String GizmoCatalog::class_of(const String &p_function) { return p_function.get_slice(".", 0); }
String GizmoCatalog::method_of(const String &p_function) { return p_function.get_slice(".", 1); }
bool GizmoCatalog::is_3d(const String &p_function) { return class_of(p_function) == "Gizmos3D"; }

Dictionary GizmoCatalog::method_info(const String &p_function)
{
    String cls = class_of(p_function);
    String method = method_of(p_function);
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    if (cls.is_empty() || method.is_empty() || !cdb->class_exists(cls))
    {
        return Dictionary();
    }
    TypedArray<Dictionary> methods = cdb->class_get_method_list(cls, true);
    for (int i = 0; i < methods.size(); i++)
    {
        Dictionary info = methods[i];
        if (String(info.get("name", "")) == method)
        {
            return info;
        }
    }
    return Dictionary();
}

int GizmoCatalog::user_param_count(const String &p_function)
{
    Array args = method_info(p_function).get("args", Array());
    return MAX(0, (int)args.size() - 1);
}

Dictionary GizmoCatalog::user_param(const String &p_function, int p_index)
{
    Array args = method_info(p_function).get("args", Array());
    return (p_index + 1 < args.size()) ? Dictionary(args[p_index + 1]) : Dictionary();
}

Variant GizmoCatalog::user_default(const String &p_function, int p_index)
{
    Dictionary info = method_info(p_function);
    Array args = info.get("args", Array());
    Array defaults = info.get("default_args", Array());
    int full_index = p_index + 1;
    int first_default = args.size() - defaults.size();
    if (full_index >= first_default && full_index - first_default < defaults.size())
    {
        return defaults[full_index - first_default];
    }
    int type = user_param(p_function, p_index).get("type", 0);
    return (type == Variant::NIL || type == Variant::OBJECT) ? Variant() : UtilityFunctions::type_convert(Variant(), type);
}

// =====================================================================================================
// GizmoEntry
// =====================================================================================================

void GizmoEntry::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &GizmoEntry::set_enabled);
    ClassDB::bind_method(D_METHOD("get_enabled"), &GizmoEntry::get_enabled);
    ClassDB::bind_method(D_METHOD("set_function", "function"), &GizmoEntry::set_function);
    ClassDB::bind_method(D_METHOD("get_function"), &GizmoEntry::get_function);
    ClassDB::bind_method(D_METHOD("restore", "function", "arguments"), &GizmoEntry::restore);
    ClassDB::bind_method(D_METHOD("set_arguments", "arguments"), &GizmoEntry::set_arguments);
    ClassDB::bind_method(D_METHOD("get_arguments"), &GizmoEntry::get_arguments);
    ClassDB::bind_method(D_METHOD("set_visibility", "visibility"), &GizmoEntry::set_visibility);
    ClassDB::bind_method(D_METHOD("get_visibility"), &GizmoEntry::get_visibility);
    ClassDB::bind_method(D_METHOD("set_redraw", "redraw"), &GizmoEntry::set_redraw);
    ClassDB::bind_method(D_METHOD("get_redraw"), &GizmoEntry::get_redraw);
    ClassDB::bind_method(D_METHOD("is_shown_now"), &GizmoEntry::is_shown_now);
    ClassDB::bind_static_method("GizmoEntry", D_METHOD("make_default_arguments", "function"), &GizmoEntry::make_default_arguments);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "get_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "function", PROPERTY_HINT_ENUM, ""), "set_function", "get_function");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "arguments", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":ArgumentBinding"),
                 "set_arguments", "get_arguments");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "visibility", PROPERTY_HINT_ENUM, U"Éditeur et jeu:0,Éditeur:1,Jeu:2"), "set_visibility", "get_visibility");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "redraw", PROPERTY_HINT_ENUM, U"Sur changement:0,Chaque image:1,Une fois:2"), "set_redraw", "get_redraw");

    BIND_ENUM_CONSTANT(VISIBILITY_EVERYWHERE);
    BIND_ENUM_CONSTANT(VISIBILITY_EDITOR);
    BIND_ENUM_CONSTANT(VISIBILITY_GAME);
    BIND_ENUM_CONSTANT(REDRAW_ON_CHANGE);
    BIND_ENUM_CONSTANT(REDRAW_EVERY_FRAME);
    BIND_ENUM_CONSTANT(REDRAW_ONCE);
}

void GizmoEntry::_validate_property(PropertyInfo &p_property) const
{
    if (p_property.name == StringName("function"))
    {
        p_property.hint_string = String(",").join(GizmoCatalog::functions());
    }
}

TypedArray<ArgumentBinding> GizmoEntry::make_default_arguments(const String &p_function)
{
    TypedArray<ArgumentBinding> arguments;
    int count = GizmoCatalog::user_param_count(p_function);
    for (int i = 0; i < count; i++)
    {
        Ref<ArgumentBinding> binding;
        binding.instantiate();
        binding->set_expected_info(GizmoCatalog::user_param(p_function, i));
        binding->set_constant(GizmoCatalog::user_default(p_function, i));
        arguments.push_back(binding);
    }
    return arguments;
}

void GizmoEntry::set_enabled(bool p_enabled)
{
    enabled = p_enabled;
    emit_changed();
}
bool GizmoEntry::get_enabled() const { return enabled; }

void GizmoEntry::set_function(const String &p_function)
{
    if (p_function == function)
    {
        return;
    }
    function = p_function;
    arguments = make_default_arguments(function);
    notify_property_list_changed();
    emit_changed();
}
String GizmoEntry::get_function() const { return function; }

void GizmoEntry::restore(const String &p_function, const TypedArray<ArgumentBinding> &p_arguments)
{
    function = p_function;
    arguments = p_arguments;
    notify_property_list_changed();
    emit_changed();
}
void GizmoEntry::set_arguments(const TypedArray<ArgumentBinding> &p_arguments)
{
    arguments = p_arguments;
    emit_changed();
}
TypedArray<ArgumentBinding> GizmoEntry::get_arguments() const { return arguments; }
void GizmoEntry::set_visibility(Visibility p_visibility)
{
    visibility = p_visibility;
    emit_changed();
}
GizmoEntry::Visibility GizmoEntry::get_visibility() const { return visibility; }
void GizmoEntry::set_redraw(Redraw p_redraw)
{
    redraw = p_redraw;
    emit_changed();
}
GizmoEntry::Redraw GizmoEntry::get_redraw() const { return redraw; }

bool GizmoEntry::is_shown_now() const
{
    if (!enabled || function.is_empty())
    {
        return false;
    }
    bool editor = Engine::get_singleton()->is_editor_hint();
    return visibility == VISIBILITY_EVERYWHERE || (visibility == VISIBILITY_EDITOR && editor) || (visibility == VISIBILITY_GAME && !editor);
}

// =====================================================================================================
// GizmoRegistry
// =====================================================================================================

void GizmoRegistry::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_entries", "entries"), &GizmoRegistry::set_entries);
    ClassDB::bind_method(D_METHOD("get_entries"), &GizmoRegistry::get_entries);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "entries", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":GizmoEntry"),
                 "set_entries", "get_entries");
}

void GizmoRegistry::set_entries(const TypedArray<GizmoEntry> &p_entries)
{
    entries = p_entries;
    emit_changed();
}
TypedArray<GizmoEntry> GizmoRegistry::get_entries() const { return entries; }
