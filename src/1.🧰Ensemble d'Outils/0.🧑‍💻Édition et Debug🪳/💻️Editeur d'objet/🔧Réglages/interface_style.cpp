#include "interface_style.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

// =====================================================================================================
// ShaderParameterBinding
// =====================================================================================================

void ShaderParameterBinding::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_parameter", "parameter"), &ShaderParameterBinding::set_parameter);
    ClassDB::bind_method(D_METHOD("get_parameter"), &ShaderParameterBinding::get_parameter);
    ClassDB::bind_method(D_METHOD("set_binding", "binding"), &ShaderParameterBinding::set_binding);
    ClassDB::bind_method(D_METHOD("get_binding"), &ShaderParameterBinding::get_binding);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "parameter"), "set_parameter", "get_parameter");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "binding", PROPERTY_HINT_RESOURCE_TYPE, "ArgumentBinding"), "set_binding", "get_binding");
}

void ShaderParameterBinding::set_parameter(const StringName &p_parameter)
{
    parameter = p_parameter;
    emit_changed();
}
StringName ShaderParameterBinding::get_parameter() const { return parameter; }
void ShaderParameterBinding::set_binding(const Ref<ArgumentBinding> &p_binding)
{
    binding = p_binding;
    emit_changed();
}
Ref<ArgumentBinding> ShaderParameterBinding::get_binding()
{
    if (binding.is_null())
    {
        binding.instantiate();
    }
    return binding;
}

// =====================================================================================================
// InterfaceStyle
// =====================================================================================================

void InterfaceStyle::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_theme", "theme"), &InterfaceStyle::set_theme);
    ClassDB::bind_method(D_METHOD("get_theme"), &InterfaceStyle::get_theme);
    ClassDB::bind_method(D_METHOD("set_theme_type_variation", "variation"), &InterfaceStyle::set_theme_type_variation);
    ClassDB::bind_method(D_METHOD("get_theme_type_variation"), &InterfaceStyle::get_theme_type_variation);
    ClassDB::bind_method(D_METHOD("set_background", "background"), &InterfaceStyle::set_background);
    ClassDB::bind_method(D_METHOD("get_background"), &InterfaceStyle::get_background);
    ClassDB::bind_method(D_METHOD("set_material", "material"), &InterfaceStyle::set_material);
    ClassDB::bind_method(D_METHOD("get_material"), &InterfaceStyle::get_material);
    ClassDB::bind_method(D_METHOD("set_propagate_material", "propagate"), &InterfaceStyle::set_propagate_material);
    ClassDB::bind_method(D_METHOD("get_propagate_material"), &InterfaceStyle::get_propagate_material);
    ClassDB::bind_method(D_METHOD("set_modulate", "color"), &InterfaceStyle::set_modulate);
    ClassDB::bind_method(D_METHOD("get_modulate"), &InterfaceStyle::get_modulate);
    ClassDB::bind_method(D_METHOD("set_self_modulate", "color"), &InterfaceStyle::set_self_modulate);
    ClassDB::bind_method(D_METHOD("get_self_modulate"), &InterfaceStyle::get_self_modulate);
    ClassDB::bind_method(D_METHOD("set_shader_parameters", "parameters"), &InterfaceStyle::set_shader_parameters);
    ClassDB::bind_method(D_METHOD("get_shader_parameters"), &InterfaceStyle::get_shader_parameters);
    ClassDB::bind_method(D_METHOD("apply_to", "item"), &InterfaceStyle::apply_to);
    ClassDB::bind_static_method("InterfaceStyle", D_METHOD("clear_from", "item"), &InterfaceStyle::clear_from);
    ClassDB::bind_method(D_METHOD("update_parameters", "item", "context"), &InterfaceStyle::update_parameters);
    ClassDB::bind_method(D_METHOD("has_parameter_bindings"), &InterfaceStyle::has_parameter_bindings);
    ClassDB::bind_method(D_METHOD("get_shader_uniforms"), &InterfaceStyle::get_shader_uniforms);
    ClassDB::bind_method(D_METHOD("find_parameter", "parameter"), &InterfaceStyle::find_parameter);
    ClassDB::bind_static_method("InterfaceStyle", D_METHOD("load_project_default"), &InterfaceStyle::load_project_default);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "theme", PROPERTY_HINT_RESOURCE_TYPE, "Theme"), "set_theme", "get_theme");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "theme_type_variation"), "set_theme_type_variation", "get_theme_type_variation");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "background", PROPERTY_HINT_RESOURCE_TYPE, "StyleBox"), "set_background", "get_background");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "CanvasItemMaterial,ShaderMaterial"), "set_material", "get_material");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "propagate_material"), "set_propagate_material", "get_propagate_material");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "modulate"), "set_modulate", "get_modulate");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "self_modulate"), "set_self_modulate", "get_self_modulate");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "shader_parameters", PROPERTY_HINT_ARRAY_TYPE, "ShaderParameterBinding"), "set_shader_parameters",
                 "get_shader_parameters");
}

void InterfaceStyle::register_project_setting()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();
    if (!settings->has_setting(DEFAULT_STYLE_SETTING))
    {
        settings->set_setting(DEFAULT_STYLE_SETTING, String());
    }
    settings->set_initial_value(DEFAULT_STYLE_SETTING, String());
    Dictionary info;
    info["name"] = DEFAULT_STYLE_SETTING;
    info["type"] = Variant::STRING;
    info["hint"] = PROPERTY_HINT_FILE;
    info["hint_string"] = "*.tres,*.res";
    settings->add_property_info(info);
}

Ref<InterfaceStyle> InterfaceStyle::load_project_default()
{
    String path = ProjectSettings::get_singleton()->get_setting(DEFAULT_STYLE_SETTING, String());
    if (path.is_empty() || !ResourceLoader::get_singleton()->exists(path))
    {
        return Ref<InterfaceStyle>();
    }
    return ResourceLoader::get_singleton()->load(path);
}

MaterialPropagator *MaterialPropagator::get_singleton()
{
    if (!singleton)
    {
        singleton = memnew(MaterialPropagator);
    }
    return singleton;
}

void MaterialPropagator::destroy()
{
    if (singleton)
    {
        memdelete(singleton);
        singleton = nullptr;
    }
}

void MaterialPropagator::on_child_entered(const Variant &p_child)
{
    Node *node = Object::cast_to<Node>(p_child.get_validated_object());
    CanvasItem *item = Object::cast_to<CanvasItem>(node);
    // Un élément qui a son propre matériau (style plus précis) garde le sien, ainsi que sa descendance.
    if (!node || (item && item->get_material().is_valid()))
    {
        return;
    }
    if (item)
    {
        item->set_use_parent_material(true);
    }
    propagate(node, true);
}

void MaterialPropagator::propagate(Node *p_node, bool p_enable)
{
    if (!p_node)
    {
        return;
    }
    Callable hook = callable_mp(this, &MaterialPropagator::on_child_entered);
    TypedArray<Node> children = p_node->get_children(true);
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        CanvasItem *item = Object::cast_to<CanvasItem>(child);
        if (item && item->get_material().is_valid())
        {
            continue;
        }
        if (item)
        {
            item->set_use_parent_material(p_enable);
        }
        propagate(child, p_enable);
    }
    if (p_enable && !p_node->is_connected("child_entered_tree", hook))
    {
        p_node->connect("child_entered_tree", hook);
    }
    else if (!p_enable && p_node->is_connected("child_entered_tree", hook))
    {
        p_node->disconnect("child_entered_tree", hook);
    }
}

Ref<Material> InterfaceStyle::apply_to(CanvasItem *p_item) const
{
    if (!p_item)
    {
        return Ref<Material>();
    }
    if (Control *control = Object::cast_to<Control>(p_item))
    {
        if (theme.is_valid())
        {
            control->set_theme(theme);
        }
        if (theme_type_variation != StringName())
        {
            control->set_theme_type_variation(theme_type_variation);
        }
    }
    p_item->set_modulate(modulate);
    p_item->set_self_modulate(self_modulate);
    Ref<Material> applied = material;
    if (applied.is_valid() && has_parameter_bindings())
    {
        applied = material->duplicate(); // valeurs propres à cet élément
    }
    p_item->set_material(applied);
    if (applied.is_valid())
    {
        p_item->set_use_parent_material(false); // son propre matériau, même sous un élément propagé
    }
    if (applied.is_valid() && propagate_material)
    {
        MaterialPropagator::get_singleton()->propagate(p_item, true);
    }
    return applied;
}

void InterfaceStyle::clear_from(CanvasItem *p_item)
{
    if (!p_item)
    {
        return;
    }
    if (Control *control = Object::cast_to<Control>(p_item))
    {
        control->set_theme(Ref<Theme>());
        control->set_theme_type_variation(StringName());
    }
    p_item->set_modulate(Color(1, 1, 1, 1));
    p_item->set_self_modulate(Color(1, 1, 1, 1));
    p_item->set_material(Ref<Material>());
    MaterialPropagator::get_singleton()->propagate(p_item, false);
}

void InterfaceStyle::update_parameters(CanvasItem *p_item, Object *p_context) const
{
    Ref<ShaderMaterial> shader_material = p_item ? Ref<ShaderMaterial>(p_item->get_material()) : Ref<ShaderMaterial>();
    if (shader_material.is_null())
    {
        return;
    }
    for (int i = 0; i < shader_parameters.size(); i++)
    {
        Ref<ShaderParameterBinding> parameter = shader_parameters[i];
        if (parameter.is_null() || parameter->get_parameter() == StringName())
        {
            continue;
        }
        Variant value = parameter->get_binding()->resolve(p_context, Array());
        if (value.get_type() != Variant::NIL)
        {
            shader_material->set_shader_parameter(parameter->get_parameter(), value);
        }
    }
}

bool InterfaceStyle::has_parameter_bindings() const
{
    for (int i = 0; i < shader_parameters.size(); i++)
    {
        Ref<ShaderParameterBinding> parameter = shader_parameters[i];
        if (parameter.is_valid() && parameter->get_parameter() != StringName())
        {
            return true;
        }
    }
    return false;
}

TypedArray<Dictionary> InterfaceStyle::get_shader_uniforms() const
{
    TypedArray<Dictionary> uniforms;
    Ref<ShaderMaterial> shader_material = material;
    Ref<Shader> shader = shader_material.is_valid() ? shader_material->get_shader() : Ref<Shader>();
    if (shader.is_null())
    {
        return uniforms;
    }
    Array list = shader->get_shader_uniform_list(false);
    for (int i = 0; i < list.size(); i++)
    {
        uniforms.push_back(list[i]);
    }
    return uniforms;
}

Ref<ShaderParameterBinding> InterfaceStyle::find_parameter(const StringName &p_parameter) const
{
    for (int i = 0; i < shader_parameters.size(); i++)
    {
        Ref<ShaderParameterBinding> parameter = shader_parameters[i];
        if (parameter.is_valid() && parameter->get_parameter() == p_parameter)
        {
            return parameter;
        }
    }
    return Ref<ShaderParameterBinding>();
}

void InterfaceStyle::set_theme(const Ref<Theme> &p_theme)
{
    theme = p_theme;
    emit_changed();
}
Ref<Theme> InterfaceStyle::get_theme() const { return theme; }
void InterfaceStyle::set_theme_type_variation(const StringName &p_variation)
{
    theme_type_variation = p_variation;
    emit_changed();
}
StringName InterfaceStyle::get_theme_type_variation() const { return theme_type_variation; }
void InterfaceStyle::set_background(const Ref<StyleBox> &p_background)
{
    background = p_background;
    emit_changed();
}
Ref<StyleBox> InterfaceStyle::get_background() const { return background; }
void InterfaceStyle::set_material(const Ref<Material> &p_material)
{
    material = p_material;
    emit_changed();
}
Ref<Material> InterfaceStyle::get_material() const { return material; }
void InterfaceStyle::set_propagate_material(bool p_propagate)
{
    propagate_material = p_propagate;
    emit_changed();
}
bool InterfaceStyle::get_propagate_material() const { return propagate_material; }
void InterfaceStyle::set_modulate(const Color &p_color)
{
    modulate = p_color;
    emit_changed();
}
Color InterfaceStyle::get_modulate() const { return modulate; }
void InterfaceStyle::set_self_modulate(const Color &p_color)
{
    self_modulate = p_color;
    emit_changed();
}
Color InterfaceStyle::get_self_modulate() const { return self_modulate; }
void InterfaceStyle::set_shader_parameters(const TypedArray<ShaderParameterBinding> &p_parameters)
{
    shader_parameters = p_parameters;
    emit_changed();
}
TypedArray<ShaderParameterBinding> InterfaceStyle::get_shader_parameters() const { return shader_parameters; }
