#include "layout_database.hpp"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/property_info.hpp>

using namespace godot;

// =====================================================================================================
// TypeLayout
// =====================================================================================================

void TypeLayout::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_type_name", "type"), &TypeLayout::set_type_name);
    ClassDB::bind_method(D_METHOD("get_type_name"), &TypeLayout::get_type_name);
    ClassDB::bind_method(D_METHOD("set_include_subclasses", "include"), &TypeLayout::set_include_subclasses);
    ClassDB::bind_method(D_METHOD("get_include_subclasses"), &TypeLayout::get_include_subclasses);
    ClassDB::bind_method(D_METHOD("set_priority", "priority"), &TypeLayout::set_priority);
    ClassDB::bind_method(D_METHOD("get_priority"), &TypeLayout::get_priority);
    ClassDB::bind_method(D_METHOD("set_data_registry", "registry"), &TypeLayout::set_data_registry);
    ClassDB::bind_method(D_METHOD("get_data_registry"), &TypeLayout::get_data_registry);
    ClassDB::bind_method(D_METHOD("set_group_registry", "registry"), &TypeLayout::set_group_registry);
    ClassDB::bind_method(D_METHOD("get_group_registry"), &TypeLayout::get_group_registry);
    ClassDB::bind_method(D_METHOD("set_gizmo_registry", "registry"), &TypeLayout::set_gizmo_registry);
    ClassDB::bind_method(D_METHOD("get_gizmo_registry"), &TypeLayout::get_gizmo_registry);
    ClassDB::bind_method(D_METHOD("ensure_registries"), &TypeLayout::ensure_registries);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "type_name"), "set_type_name", "get_type_name");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "include_subclasses"), "set_include_subclasses", "get_include_subclasses");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "data_registry", PROPERTY_HINT_RESOURCE_TYPE, "DataRegistry"), "set_data_registry", "get_data_registry");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "group_registry", PROPERTY_HINT_RESOURCE_TYPE, "GroupRegistry"), "set_group_registry", "get_group_registry");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "gizmo_registry", PROPERTY_HINT_RESOURCE_TYPE, "GizmoRegistry"), "set_gizmo_registry", "get_gizmo_registry");
}

void TypeLayout::set_type_name(const String &p_type)
{
    type_name = p_type;
    emit_changed();
}
String TypeLayout::get_type_name() const { return type_name; }
void TypeLayout::set_include_subclasses(bool p_include)
{
    include_subclasses = p_include;
    emit_changed();
}
bool TypeLayout::get_include_subclasses() const { return include_subclasses; }
void TypeLayout::set_priority(int p_priority)
{
    priority = p_priority;
    emit_changed();
}
int TypeLayout::get_priority() const { return priority; }
void TypeLayout::set_data_registry(const Ref<DataRegistry> &p_registry)
{
    data_registry = p_registry;
    emit_changed();
}
Ref<DataRegistry> TypeLayout::get_data_registry() const { return data_registry; }
void TypeLayout::set_group_registry(const Ref<GroupRegistry> &p_registry)
{
    group_registry = p_registry;
    emit_changed();
}
Ref<GroupRegistry> TypeLayout::get_group_registry() const { return group_registry; }
void TypeLayout::set_gizmo_registry(const Ref<GizmoRegistry> &p_registry)
{
    gizmo_registry = p_registry;
    emit_changed();
}
Ref<GizmoRegistry> TypeLayout::get_gizmo_registry() const { return gizmo_registry; }

void TypeLayout::ensure_registries()
{
    if (data_registry.is_null())
    {
        data_registry.instantiate();
    }
    if (group_registry.is_null())
    {
        group_registry.instantiate();
    }
    if (gizmo_registry.is_null())
    {
        gizmo_registry.instantiate();
    }
}

// =====================================================================================================
// LayoutDatabase
// =====================================================================================================

void LayoutDatabase::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_layouts", "layouts"), &LayoutDatabase::set_layouts);
    ClassDB::bind_method(D_METHOD("get_layouts"), &LayoutDatabase::get_layouts);
    ClassDB::bind_method(D_METHOD("find_layout", "object"), &LayoutDatabase::find_layout);
    ClassDB::bind_method(D_METHOD("find_by_type", "type_name"), &LayoutDatabase::find_by_type);
    ClassDB::bind_method(D_METHOD("ensure_type", "type_name"), &LayoutDatabase::ensure_type);
    ClassDB::bind_static_method("LayoutDatabase", D_METHOD("get_project_path"), &LayoutDatabase::get_project_path);
    ClassDB::bind_static_method("LayoutDatabase", D_METHOD("load_project_database"), &LayoutDatabase::load_project_database);
    ClassDB::bind_static_method("LayoutDatabase", D_METHOD("type_chain", "object"), &LayoutDatabase::type_chain);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "layouts", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":TypeLayout"),
                 "set_layouts", "get_layouts");
}

void LayoutDatabase::register_project_setting()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();
    if (!settings->has_setting(SETTING_PATH))
    {
        settings->set_setting(SETTING_PATH, DEFAULT_PATH);
    }
    // Valeur initiale = défaut : non écrite dans project.godot tant qu'elle n'est pas modifiée.
    settings->set_initial_value(SETTING_PATH, DEFAULT_PATH);
    Dictionary info;
    info["name"] = SETTING_PATH;
    info["type"] = Variant::STRING;
    info["hint"] = PROPERTY_HINT_FILE;
    info["hint_string"] = "*.tres,*.res";
    settings->add_property_info(info);
}

String LayoutDatabase::get_project_path() { return ProjectSettings::get_singleton()->get_setting(SETTING_PATH, DEFAULT_PATH); }

Ref<LayoutDatabase> LayoutDatabase::load_project_database()
{
    String path = get_project_path();
    ResourceLoader *loader = ResourceLoader::get_singleton();
    if (path.is_empty() || !loader->exists(path))
    {
        return Ref<LayoutDatabase>();
    }
    return loader->load(path);
}

PackedStringArray LayoutDatabase::type_chain(Object *p_object)
{
    PackedStringArray chain;
    if (!p_object)
    {
        return chain;
    }
    // 1. Scripts : class_name du script attaché puis de ses scripts de base.
    Ref<Script> script = p_object->get_script();
    for (int guard = 0; script.is_valid() && guard < 32; guard++)
    {
        String global_name = script->get_global_name();
        if (!global_name.is_empty())
        {
            chain.push_back(global_name);
        }
        script = script->get_base_script();
    }
    // 2. Classes natives (moteur et GDExtension).
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    for (String cls = p_object->get_class(); !cls.is_empty(); cls = cdb->get_parent_class(cls))
    {
        chain.push_back(cls);
    }
    return chain;
}

void LayoutDatabase::set_layouts(const TypedArray<TypeLayout> &p_layouts)
{
    layouts = p_layouts;
    emit_changed();
}
TypedArray<TypeLayout> LayoutDatabase::get_layouts() const { return layouts; }

Ref<TypeLayout> LayoutDatabase::find_layout(Object *p_object) const
{
    if (!p_object)
    {
        return Ref<TypeLayout>();
    }
    // 1. Surcharge de l'instance.
    if (p_object->has_meta(INSTANCE_META))
    {
        Ref<TypeLayout> instance_layout = p_object->get_meta(INSTANCE_META);
        if (instance_layout.is_valid())
        {
            return instance_layout;
        }
    }
    // 2-3. Type le plus spécifique, puis priorité.
    PackedStringArray chain = type_chain(p_object);
    Ref<TypeLayout> best;
    int best_rank = INT32_MAX;
    for (int i = 0; i < layouts.size(); i++)
    {
        Ref<TypeLayout> layout = layouts[i];
        if (layout.is_null())
        {
            continue;
        }
        int rank = chain.find(layout->get_type_name());
        if (rank < 0 || (rank > 0 && !layout->get_include_subclasses()))
        {
            continue;
        }
        if (rank < best_rank || (rank == best_rank && layout->get_priority() > best->get_priority()))
        {
            best = layout;
            best_rank = rank;
        }
    }
    return best;
}

Ref<TypeLayout> LayoutDatabase::find_by_type(const String &p_type_name) const
{
    for (int i = 0; i < layouts.size(); i++)
    {
        Ref<TypeLayout> layout = layouts[i];
        if (layout.is_valid() && layout->get_type_name() == p_type_name)
        {
            return layout;
        }
    }
    return Ref<TypeLayout>();
}

Ref<TypeLayout> LayoutDatabase::ensure_type(const String &p_type_name)
{
    Ref<TypeLayout> layout = find_by_type(p_type_name);
    if (layout.is_null())
    {
        layout.instantiate();
        layout->set_type_name(p_type_name);
        layout->ensure_registries();
        layouts.push_back(layout);
        emit_changed();
    }
    return layout;
}
