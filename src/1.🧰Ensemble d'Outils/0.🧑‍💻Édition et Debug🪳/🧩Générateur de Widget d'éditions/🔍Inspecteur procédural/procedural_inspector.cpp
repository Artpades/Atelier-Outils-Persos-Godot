#include "procedural_inspector.hpp"
#include "inspector_binding.hpp"
#include "inspector_widgets.hpp"

#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void ProceduralInspector::_ready()
{
    if (content != nullptr)
    {
        return;
    }

    PanelContainer *panel = memnew(PanelContainer);
    panel->set_name("Panel");
    panel->set_anchors_and_offsets_preset(PRESET_FULL_RECT);

    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_name("Scroll");
    scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    scroll->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->set_v_size_flags(SIZE_EXPAND_FILL);

    content = memnew(VBoxContainer);
    content->set_name("Content");
    content->set_h_size_flags(SIZE_EXPAND_FILL);

    scroll->add_child(content);
    panel->add_child(scroll);
    // Nœud interne : n'apparaît pas dans l'arbre de la scène et n'est pas enregistré.
    add_child(panel, false, INTERNAL_MODE_FRONT);

    rebuild();
}

void ProceduralInspector::set_target(Object *p_target)
{
    target_id = p_target != nullptr ? ObjectID(p_target->get_instance_id()) : ObjectID();
    rebuild();
}

Object *ProceduralInspector::get_target() const
{
    return target_id.is_null() ? nullptr : ObjectDB::get_instance(target_id);
}

void ProceduralInspector::set_target_path(const NodePath &p_path)
{
    target_path = p_path;
    rebuild();
}

void ProceduralInspector::set_target_resource(const Ref<Resource> &p_resource)
{
    target_resource = p_resource;
    rebuild();
}

void ProceduralInspector::set_scope(Scope p_scope)
{
    scope = p_scope;
    rebuild();
}

void ProceduralInspector::set_show_properties(bool p_show)
{
    show_properties = p_show;
    rebuild();
}

void ProceduralInspector::set_show_methods(bool p_show)
{
    show_methods = p_show;
    rebuild();
}

void ProceduralInspector::set_show_signals(bool p_show)
{
    show_signals = p_show;
    rebuild();
}

Object *ProceduralInspector::resolve_target() const
{
    if (Object *target = get_target())
    {
        return target;
    }
    if (target_resource.is_valid())
    {
        return target_resource.ptr();
    }
    if (!target_path.is_empty() && is_inside_tree())
    {
        return get_node_or_null(target_path);
    }
    return nullptr;
}

void ProceduralInspector::rebuild()
{
    if (content == nullptr)
    {
        return; // avant _ready : la construction aura lieu à l'entrée dans l'arbre
    }
    InspectorWidgets::Options options;
    options.scope = (InspectorWidgets::Scope)scope;
    options.properties = show_properties;
    options.methods = show_methods;
    options.signals = show_signals;
    InspectorWidgets::populate(content, resolve_target(), options);
}

void ProceduralInspector::refresh()
{
    InspectorBinding::refresh_all(content);
}

Node *ProceduralInspector::bind_value(Control *p_owner, const Callable &p_getter, const Callable &p_pull, const Callable &p_setter)
{
    ERR_FAIL_NULL_V(p_owner, nullptr);

    ValueAccessor accessor;
    accessor.get = [p_getter]() { return p_getter.call(); };
    accessor.is_alive = [p_getter]() { return p_getter.is_valid(); };
    if (p_setter.is_valid())
    {
        accessor.set = [p_setter](const Variant &p_value) { p_setter.call(p_value); };
    }
    return InspectorBinding::attach(p_owner, accessor, [p_pull](const Variant &p_value) { p_pull.call(p_value); });
}

void ProceduralInspector::refresh_bindings(Node *p_root) { InspectorBinding::refresh_all(p_root); }

void ProceduralInspector::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_target", "target"), &ProceduralInspector::set_target);
    ClassDB::bind_method(D_METHOD("get_target"), &ProceduralInspector::get_target);
    ClassDB::bind_method(D_METHOD("set_target_path", "path"), &ProceduralInspector::set_target_path);
    ClassDB::bind_method(D_METHOD("get_target_path"), &ProceduralInspector::get_target_path);
    ClassDB::bind_method(D_METHOD("set_target_resource", "resource"), &ProceduralInspector::set_target_resource);
    ClassDB::bind_method(D_METHOD("get_target_resource"), &ProceduralInspector::get_target_resource);
    ClassDB::bind_method(D_METHOD("set_scope", "scope"), &ProceduralInspector::set_scope);
    ClassDB::bind_method(D_METHOD("get_scope"), &ProceduralInspector::get_scope);
    ClassDB::bind_method(D_METHOD("set_show_properties", "show"), &ProceduralInspector::set_show_properties);
    ClassDB::bind_method(D_METHOD("get_show_properties"), &ProceduralInspector::get_show_properties);
    ClassDB::bind_method(D_METHOD("set_show_methods", "show"), &ProceduralInspector::set_show_methods);
    ClassDB::bind_method(D_METHOD("get_show_methods"), &ProceduralInspector::get_show_methods);
    ClassDB::bind_method(D_METHOD("set_show_signals", "show"), &ProceduralInspector::set_show_signals);
    ClassDB::bind_method(D_METHOD("get_show_signals"), &ProceduralInspector::get_show_signals);
    ClassDB::bind_method(D_METHOD("resolve_target"), &ProceduralInspector::resolve_target);
    ClassDB::bind_method(D_METHOD("rebuild"), &ProceduralInspector::rebuild);
    ClassDB::bind_method(D_METHOD("refresh"), &ProceduralInspector::refresh);
    ClassDB::bind_static_method("ProceduralInspector", D_METHOD("bind_value", "owner", "getter", "pull", "setter"), &ProceduralInspector::bind_value, DEFVAL(Callable()));
    ClassDB::bind_static_method("ProceduralInspector", D_METHOD("refresh_bindings", "root"), &ProceduralInspector::refresh_bindings);

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_path"), "set_target_path", "get_target_path");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target_resource", PROPERTY_HINT_RESOURCE_TYPE, "Resource"), "set_target_resource", "get_target_resource");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "scope", PROPERTY_HINT_ENUM, "Auto,Script uniquement,Tout"), "set_scope", "get_scope");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_properties"), "set_show_properties", "get_show_properties");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_methods"), "set_show_methods", "get_show_methods");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_signals"), "set_show_signals", "get_show_signals");

    BIND_ENUM_CONSTANT(SCOPE_AUTO);
    BIND_ENUM_CONSTANT(SCOPE_SCRIPT_ONLY);
    BIND_ENUM_CONSTANT(SCOPE_ALL);
}
