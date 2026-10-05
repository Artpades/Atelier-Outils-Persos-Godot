#include "widget_behavior.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/node_path.hpp>

using namespace godot;

// =====================================================================================================
// WidgetBehavior
// =====================================================================================================

void WidgetBehavior::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &WidgetBehavior::set_enabled);
    ClassDB::bind_method(D_METHOD("get_enabled"), &WidgetBehavior::get_enabled);
    ClassDB::bind_method(D_METHOD("set_condition", "condition"), &WidgetBehavior::set_condition);
    ClassDB::bind_method(D_METHOD("get_condition"), &WidgetBehavior::get_condition);
    ClassDB::bind_method(D_METHOD("get_stored_condition"), &WidgetBehavior::get_stored_condition);
    ClassDB::bind_method(D_METHOD("set_invert", "invert"), &WidgetBehavior::set_invert);
    ClassDB::bind_method(D_METHOD("get_invert"), &WidgetBehavior::get_invert);
    ClassDB::bind_method(D_METHOD("set_targets", "targets"), &WidgetBehavior::set_targets);
    ClassDB::bind_method(D_METHOD("get_targets"), &WidgetBehavior::get_targets);
    ClassDB::bind_method(D_METHOD("evaluate", "context"), &WidgetBehavior::evaluate);
    ClassDB::bind_method(D_METHOD("get_display_name"), &WidgetBehavior::get_display_name);
    ClassDB::bind_static_method("WidgetBehavior", D_METHOD("format_text", "template", "context"), &WidgetBehavior::format_text);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "get_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "condition", PROPERTY_HINT_RESOURCE_TYPE, "ArgumentBinding"), "set_condition", "get_stored_condition");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "invert"), "set_invert", "get_invert");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "targets"), "set_targets", "get_targets");

    GDVIRTUAL_BIND(_apply, "target", "active");
}

String WidgetBehavior::format_text(const String &p_template, Object *p_context)
{
    String result;
    int length = p_template.length();
    for (int i = 0; i < length; i++)
    {
        char32_t c = p_template[i];
        if (c == '{' && i + 1 < length && p_template[i + 1] == '{')
        {
            result += "{";
            i++;
            continue;
        }
        if (c == '}' && i + 1 < length && p_template[i + 1] == '}')
        {
            result += "}";
            i++;
            continue;
        }
        int close = c == '{' ? p_template.find("}", i + 1) : -1;
        if (close < 0)
        {
            result += String::chr(c);
            continue;
        }
        String key = p_template.substr(i + 1, close - i - 1).strip_edges();
        Variant value;
        if (p_context && key.ends_with("()"))
        {
            StringName method = key.substr(0, key.length() - 2);
            value = p_context->has_method(method) ? p_context->call(method) : Variant(U"‹ " + key + U" ? ›");
        }
        else if (p_context && !key.is_empty())
        {
            value = p_context->get_indexed(NodePath(key));
        }
        result += value.stringify();
        i = close;
    }
    return result;
}

void WidgetBehavior::set_enabled(bool p_enabled)
{
    enabled = p_enabled;
    emit_changed();
}
bool WidgetBehavior::get_enabled() const { return enabled; }
void WidgetBehavior::set_condition(const Ref<ArgumentBinding> &p_condition)
{
    condition = p_condition;
    emit_changed();
}
// Getter de la propriété : sans création (Godot l'appelle aussi pour calculer la valeur par défaut).
Ref<ArgumentBinding> WidgetBehavior::get_stored_condition() const { return condition; }
Ref<ArgumentBinding> WidgetBehavior::get_condition()
{
    if (condition.is_null())
    {
        condition.instantiate();
        condition->set_source(ArgumentBinding::SOURCE_PROPERTY);
        Dictionary expected;
        expected["type"] = Variant::BOOL;
        condition->set_expected_info(expected);
    }
    return condition;
}
void WidgetBehavior::set_invert(bool p_invert)
{
    invert = p_invert;
    emit_changed();
}
bool WidgetBehavior::get_invert() const { return invert; }
void WidgetBehavior::set_targets(const PackedStringArray &p_targets)
{
    targets = p_targets;
    emit_changed();
}
PackedStringArray WidgetBehavior::get_targets() const { return targets; }

bool WidgetBehavior::evaluate(Object *p_context)
{
    bool value = get_condition()->resolve(p_context, Array()).booleanize();
    return invert ? !value : value;
}

void WidgetBehavior::contribute(BehaviorState &r_state, bool p_active) const
{
    (void)r_state;
    (void)p_active;
}

bool WidgetBehavior::has_script_apply() const { return GDVIRTUAL_IS_OVERRIDDEN(_apply); }

void WidgetBehavior::apply_script(Control *p_target, bool p_active) { GDVIRTUAL_CALL(_apply, p_target, p_active); }

String WidgetBehavior::get_display_name() const
{
    // Comportement de script : nom de sa classe (class_name), sinon celui du script.
    Ref<Resource> script = get_script();
    if (script.is_valid())
    {
        String global = script->call("get_global_name");
        return global.is_empty() ? script->get_path().get_file().get_basename() : global;
    }
    return get_class();
}

// =====================================================================================================
// Comportements fournis
// =====================================================================================================

void ShowIfBehavior::contribute(BehaviorState &r_state, bool p_active) const
{
    if (!p_active)
    {
        r_state.visible = false;
    }
}
String ShowIfBehavior::get_display_name() const { return "Montrer si"; }

void HideIfBehavior::contribute(BehaviorState &r_state, bool p_active) const
{
    if (p_active)
    {
        r_state.visible = false;
    }
}
String HideIfBehavior::get_display_name() const { return "Cacher si"; }

void EnableIfBehavior::contribute(BehaviorState &r_state, bool p_active) const
{
    if (!p_active)
    {
        r_state.editable = false;
    }
}
String EnableIfBehavior::get_display_name() const { return "Activer si"; }

void StyleIfBehavior::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_style", "style"), &StyleIfBehavior::set_style);
    ClassDB::bind_method(D_METHOD("get_style"), &StyleIfBehavior::get_style);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "style", PROPERTY_HINT_RESOURCE_TYPE, "InterfaceStyle"), "set_style", "get_style");
}
void StyleIfBehavior::set_style(const Ref<InterfaceStyle> &p_style)
{
    style = p_style;
    emit_changed();
}
Ref<InterfaceStyle> StyleIfBehavior::get_style() const { return style; }
void StyleIfBehavior::contribute(BehaviorState &r_state, bool p_active) const
{
    if (p_active && style.is_valid())
    {
        r_state.styles.push_back(style);
    }
}
String StyleIfBehavior::get_display_name() const { return "Style si"; }

void TooltipIfBehavior::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_tooltip", "tooltip"), &TooltipIfBehavior::set_tooltip);
    ClassDB::bind_method(D_METHOD("get_tooltip"), &TooltipIfBehavior::get_tooltip);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "tooltip", PROPERTY_HINT_MULTILINE_TEXT), "set_tooltip", "get_tooltip");
}
void TooltipIfBehavior::set_tooltip(const String &p_tooltip)
{
    tooltip = p_tooltip;
    emit_changed();
}
String TooltipIfBehavior::get_tooltip() const { return tooltip; }
void TooltipIfBehavior::contribute(BehaviorState &r_state, bool p_active) const
{
    if (p_active && !tooltip.is_empty())
    {
        r_state.tooltips.push_back(tooltip);
    }
}
String TooltipIfBehavior::get_display_name() const { return "Infobulle si"; }

void IconIfBehavior::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_icon", "icon"), &IconIfBehavior::set_icon);
    ClassDB::bind_method(D_METHOD("get_icon"), &IconIfBehavior::get_icon);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "icon", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_icon", "get_icon");
}
void IconIfBehavior::set_icon(const Ref<Texture2D> &p_icon)
{
    icon = p_icon;
    emit_changed();
}
Ref<Texture2D> IconIfBehavior::get_icon() const { return icon; }
void IconIfBehavior::contribute(BehaviorState &r_state, bool p_active) const
{
    if (p_active && icon.is_valid())
    {
        r_state.icon = icon;
    }
}
String IconIfBehavior::get_display_name() const { return U"Icône si"; }
