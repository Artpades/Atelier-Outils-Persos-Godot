#include "signal_link.hpp"

#include "type_utils.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_custom.hpp>

using namespace godot;

namespace
{
// Callable d'un lien en mode ressource : transmet tous les arguments du signal (nombre quelconque) à
// SignalLink::invoke. Ne garde que des ObjectID : un lien ou un émetteur libéré rend l'appel sans effet.
class SignalLinkCallable : public CallableCustom
{
    ObjectID link_id;
    ObjectID source_id;

    static bool _equal(const CallableCustom *p_a, const CallableCustom *p_b)
    {
        const SignalLinkCallable *a = static_cast<const SignalLinkCallable *>(p_a);
        const SignalLinkCallable *b = static_cast<const SignalLinkCallable *>(p_b);
        return a->link_id == b->link_id && a->source_id == b->source_id;
    }
    static bool _less(const CallableCustom *p_a, const CallableCustom *p_b)
    {
        const SignalLinkCallable *a = static_cast<const SignalLinkCallable *>(p_a);
        const SignalLinkCallable *b = static_cast<const SignalLinkCallable *>(p_b);
        return a->link_id == b->link_id ? (uint64_t)a->source_id < (uint64_t)b->source_id : (uint64_t)a->link_id < (uint64_t)b->link_id;
    }

  public:
    SignalLinkCallable(ObjectID p_link, ObjectID p_source) : link_id(p_link), source_id(p_source) {}

    uint32_t hash() const override { return (uint32_t)(((uint64_t)link_id * 31u) ^ (uint64_t)source_id); }
    String get_as_text() const override { return "SignalLink::invoke"; }
    CompareEqualFunc get_compare_equal_func() const override { return &SignalLinkCallable::_equal; }
    CompareLessFunc get_compare_less_func() const override { return &SignalLinkCallable::_less; }
    // L'émetteur porte la connexion : s'il est libéré, Godot la retire.
    ObjectID get_object() const override { return source_id; }

    void call(const Variant **p_arguments, int p_argcount, Variant &r_return_value, GDExtensionCallError &r_call_error) const override
    {
        r_call_error.error = GDEXTENSION_CALL_OK;
        r_return_value = Variant();
        SignalLink *link = Object::cast_to<SignalLink>(ObjectDB::get_instance(link_id));
        Object *source = ObjectDB::get_instance(source_id);
        if (!link || !source)
        {
            return;
        }
        Array args;
        for (int i = 0; i < p_argcount; i++)
        {
            args.push_back(*p_arguments[i]);
        }
        link->invoke(source, args);
    }
};
} // namespace

// =====================================================================================================
// ArgumentBinding
// =====================================================================================================

void ArgumentBinding::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_source", "source"), &ArgumentBinding::set_source);
    ClassDB::bind_method(D_METHOD("get_source"), &ArgumentBinding::get_source);
    ClassDB::bind_method(D_METHOD("set_constant", "value"), &ArgumentBinding::set_constant);
    ClassDB::bind_method(D_METHOD("get_constant"), &ArgumentBinding::get_constant);
    ClassDB::bind_method(D_METHOD("set_object_path", "path"), &ArgumentBinding::set_object_path);
    ClassDB::bind_method(D_METHOD("get_object_path"), &ArgumentBinding::get_object_path);
    ClassDB::bind_method(D_METHOD("set_object_resource", "resource"), &ArgumentBinding::set_object_resource);
    ClassDB::bind_method(D_METHOD("get_object_resource"), &ArgumentBinding::get_object_resource);
    ClassDB::bind_method(D_METHOD("set_property_path", "path"), &ArgumentBinding::set_property_path);
    ClassDB::bind_method(D_METHOD("get_property_path"), &ArgumentBinding::get_property_path);
    ClassDB::bind_method(D_METHOD("set_method", "method"), &ArgumentBinding::set_method);
    ClassDB::bind_method(D_METHOD("get_method"), &ArgumentBinding::get_method);
    ClassDB::bind_method(D_METHOD("set_signal_index", "index"), &ArgumentBinding::set_signal_index);
    ClassDB::bind_method(D_METHOD("get_signal_index"), &ArgumentBinding::get_signal_index);
    ClassDB::bind_method(D_METHOD("set_expected_info", "info"), &ArgumentBinding::set_expected_info);
    ClassDB::bind_method(D_METHOD("get_expected_info"), &ArgumentBinding::get_expected_info);
    ClassDB::bind_method(D_METHOD("resolve_object", "context"), &ArgumentBinding::resolve_object);
    ClassDB::bind_method(D_METHOD("resolve", "context", "signal_args"), &ArgumentBinding::resolve);
    ClassDB::bind_static_method("ArgumentBinding", D_METHOD("constant_error", "value"), &ArgumentBinding::constant_error);
    ClassDB::bind_static_method("ArgumentBinding", D_METHOD("compatible_members", "object", "source", "expected"), &ArgumentBinding::compatible_members);
    ClassDB::bind_static_method("ArgumentBinding", D_METHOD("is_method_const", "object", "method"), &ArgumentBinding::is_method_const);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "source", PROPERTY_HINT_ENUM, U"Constante:0,Propriété:1,Argument du signal:2,Fonction:3"), "set_source", "get_source");
    ADD_PROPERTY(PropertyInfo(Variant::NIL, "constant", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "set_constant", "get_constant");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "object_path"), "set_object_path", "get_object_path");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "object_resource", PROPERTY_HINT_RESOURCE_TYPE, "Resource"), "set_object_resource", "get_object_resource");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "property_path"), "set_property_path", "get_property_path");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "method"), "set_method", "get_method");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "signal_index", PROPERTY_HINT_RANGE, "0,16,1"), "set_signal_index", "get_signal_index");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "expected_info", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_expected_info", "get_expected_info");

    BIND_ENUM_CONSTANT(SOURCE_CONSTANT);
    BIND_ENUM_CONSTANT(SOURCE_PROPERTY);
    BIND_ENUM_CONSTANT(SOURCE_SIGNAL_ARG);
    BIND_ENUM_CONSTANT(SOURCE_METHOD);
}

String ArgumentBinding::constant_error(const Variant &p_value)
{
    if (p_value.get_type() != Variant::OBJECT)
    {
        return String();
    }
    Object *object = p_value.get_validated_object();
    if (!object || Object::cast_to<Resource>(object))
    {
        return String();
    }
    return U"Un objet " + object->get_class() + U" ne peut pas être enregistré comme constante : seules les Resources le peuvent "
                                                  U"(pour un nœud, utiliser une source « Propriété » avec son chemin).";
}

TypedArray<Dictionary> ArgumentBinding::compatible_members(Object *p_object, Source p_source, const Dictionary &p_expected)
{
    TypedArray<Dictionary> result;
    if (!p_object)
    {
        return result;
    }
    if (p_source == SOURCE_PROPERTY)
    {
        TypedArray<Dictionary> properties = p_object->get_property_list();
        for (int i = 0; i < properties.size(); i++)
        {
            Dictionary property = properties[i];
            int usage = property.get("usage", 0);
            // Catégories, groupes et propriétés internes exclus.
            if ((usage & (PROPERTY_USAGE_CATEGORY | PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP | PROPERTY_USAGE_INTERNAL)) ||
                !(usage & (PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_SCRIPT_VARIABLE)) || !TypeUtils::is_compatible(property, p_expected))
            {
                continue;
            }
            Dictionary member;
            member["name"] = property.get("name", "");
            member["type"] = property.get("type", 0);
            member["class_name"] = property.get("class_name", "");
            member["const"] = true; // une lecture de propriété n'a pas d'effet de bord attendu
            result.push_back(member);
        }
        return result;
    }
    if (p_source != SOURCE_METHOD)
    {
        return result;
    }
    TypedArray<Dictionary> methods = p_object->get_method_list();
    Dictionary seen;
    for (int i = 0; i < methods.size(); i++)
    {
        Dictionary info = methods[i];
        String name = info.get("name", "");
        Array args = info.get("args", Array());
        Array defaults = info.get("default_args", Array());
        Dictionary ret = info.get("return", Dictionary());
        int ret_type = ret.get("type", 0);
        bool returns_variant = ret_type == Variant::NIL && ((int)ret.get("usage", 0) & PROPERTY_USAGE_NIL_IS_VARIANT);
        if (name.is_empty() || name.begins_with("_") || seen.has(name) || args.size() > defaults.size() || (ret_type == Variant::NIL && !returns_variant) ||
            !TypeUtils::is_compatible(ret, p_expected))
        {
            continue;
        }
        seen[name] = true;
        Dictionary member;
        member["name"] = name;
        member["type"] = ret_type;
        member["class_name"] = ret.get("class_name", "");
        member["const"] = ((int)info.get("flags", 0) & METHOD_FLAG_CONST) != 0;
        result.push_back(member);
    }
    return result;
}

bool ArgumentBinding::is_method_const(Object *p_object, const StringName &p_method)
{
    if (!p_object)
    {
        return false;
    }
    TypedArray<Dictionary> methods = p_object->get_method_list();
    for (int i = 0; i < methods.size(); i++)
    {
        Dictionary info = methods[i];
        if (StringName(info.get("name", "")) == p_method)
        {
            return ((int)info.get("flags", 0) & METHOD_FLAG_CONST) != 0;
        }
    }
    return false;
}

Node *ArgumentBinding::fallback_base_node()
{
    SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (!tree)
    {
        return nullptr;
    }
    return tree->get_current_scene() ? tree->get_current_scene() : (Node *)tree->get_root();
}

void ArgumentBinding::set_source(Source p_source)
{
    source = p_source;
    emit_changed();
}
ArgumentBinding::Source ArgumentBinding::get_source() const { return source; }
void ArgumentBinding::set_constant(const Variant &p_value)
{
    String error = constant_error(p_value);
    if (!error.is_empty())
    {
        log_it(U"[color=orange]" + error + U"[/color]", "ArgumentBinding");
        constant = Variant();
    }
    else
    {
        constant = p_value;
    }
    emit_changed();
}
Variant ArgumentBinding::get_constant() const { return constant; }
void ArgumentBinding::set_object_path(const NodePath &p_path)
{
    object_path = p_path;
    emit_changed();
}
NodePath ArgumentBinding::get_object_path() const { return object_path; }
void ArgumentBinding::set_object_resource(const Ref<Resource> &p_resource)
{
    object_resource = p_resource;
    emit_changed();
}
Ref<Resource> ArgumentBinding::get_object_resource() const { return object_resource; }
void ArgumentBinding::set_property_path(const NodePath &p_path)
{
    property_path = p_path;
    emit_changed();
}
NodePath ArgumentBinding::get_property_path() const { return property_path; }
void ArgumentBinding::set_method(const StringName &p_method)
{
    method = p_method;
    emit_changed();
}
StringName ArgumentBinding::get_method() const { return method; }
void ArgumentBinding::set_signal_index(int p_index)
{
    signal_index = MAX(p_index, 0);
    emit_changed();
}
int ArgumentBinding::get_signal_index() const { return signal_index; }
void ArgumentBinding::set_expected_info(const Dictionary &p_info)
{
    expected_info = p_info;
    notify_property_list_changed();
}
Dictionary ArgumentBinding::get_expected_info() const { return expected_info; }

void ArgumentBinding::_validate_property(PropertyInfo &p_property) const
{
    int type = expected_info.get("type", 0);
    if (p_property.name != StringName("constant") || type == Variant::NIL)
    {
        return;
    }
    p_property.type = (Variant::Type)type;
    p_property.hint = (PropertyHint)(int)expected_info.get("hint", 0);
    p_property.hint_string = expected_info.get("hint_string", "");
    p_property.class_name = expected_info.get("class_name", "");
    p_property.usage = PROPERTY_USAGE_DEFAULT;
}

Object *ArgumentBinding::resolve_object(Object *p_context) const
{
    if (object_resource.is_valid())
    {
        return object_resource.ptr();
    }
    if (object_path.is_empty())
    {
        return p_context;
    }
    Node *base = Object::cast_to<Node>(p_context);
    if (!base || !base->is_inside_tree())
    {
        base = fallback_base_node();
    }
    return base ? base->get_node_or_null(object_path) : nullptr;
}

Variant ArgumentBinding::resolve(Object *p_context, const Array &p_signal_args) const
{
    switch (source)
    {
    case SOURCE_SIGNAL_ARG:
        return signal_index < p_signal_args.size() ? p_signal_args[signal_index] : Variant();
    case SOURCE_PROPERTY:
    {
        Object *object = resolve_object(p_context);
        return (object && !property_path.is_empty()) ? object->get_indexed(property_path) : Variant();
    }
    case SOURCE_METHOD:
    {
        Object *object = resolve_object(p_context);
        return (object && method != StringName() && object->has_method(method)) ? object->call(method) : Variant();
    }
    case SOURCE_CONSTANT:
    default:
        return constant;
    }
}

// =====================================================================================================
// SignalLink
// =====================================================================================================

void SignalLink::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &SignalLink::set_enabled);
    ClassDB::bind_method(D_METHOD("get_enabled"), &SignalLink::get_enabled);
    ClassDB::bind_method(D_METHOD("set_target_path", "path"), &SignalLink::set_target_path);
    ClassDB::bind_method(D_METHOD("get_target_path"), &SignalLink::get_target_path);
    ClassDB::bind_method(D_METHOD("set_target_resource", "resource"), &SignalLink::set_target_resource);
    ClassDB::bind_method(D_METHOD("get_target_resource"), &SignalLink::get_target_resource);
    ClassDB::bind_method(D_METHOD("set_method", "method"), &SignalLink::set_method);
    ClassDB::bind_method(D_METHOD("get_method"), &SignalLink::get_method);
    ClassDB::bind_method(D_METHOD("set_arguments", "arguments"), &SignalLink::set_arguments);
    ClassDB::bind_method(D_METHOD("get_arguments"), &SignalLink::get_arguments);
    ClassDB::bind_method(D_METHOD("resolve_target", "source"), &SignalLink::resolve_target);
    ClassDB::bind_method(D_METHOD("connect_to", "source", "signal"), &SignalLink::connect_to);
    ClassDB::bind_method(D_METHOD("disconnect_from", "source"), &SignalLink::disconnect_from);
    ClassDB::bind_method(D_METHOD("is_connected_to", "source"), &SignalLink::is_connected_to);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "get_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_path"), "set_target_path", "get_target_path");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target_resource", PROPERTY_HINT_RESOURCE_TYPE, "Resource"), "set_target_resource", "get_target_resource");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "method"), "set_method", "get_method");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "arguments", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":ArgumentBinding"),
                 "set_arguments", "get_arguments");
}

void SignalLink::set_enabled(bool p_enabled)
{
    enabled = p_enabled;
    emit_changed();
}
bool SignalLink::get_enabled() const { return enabled; }
void SignalLink::set_target_path(const NodePath &p_path)
{
    target_path = p_path;
    emit_changed();
}
NodePath SignalLink::get_target_path() const { return target_path; }
void SignalLink::set_target_resource(const Ref<Resource> &p_resource)
{
    target_resource = p_resource;
    emit_changed();
}
Ref<Resource> SignalLink::get_target_resource() const { return target_resource; }
void SignalLink::set_method(const StringName &p_method)
{
    method = p_method;
    emit_changed();
}
StringName SignalLink::get_method() const { return method; }
void SignalLink::set_arguments(const TypedArray<ArgumentBinding> &p_arguments)
{
    arguments = p_arguments;
    emit_changed();
}
TypedArray<ArgumentBinding> SignalLink::get_arguments() const { return arguments; }

Object *SignalLink::resolve_target(Object *p_source) const
{
    if (target_resource.is_valid())
    {
        return target_resource.ptr();
    }
    if (target_path.is_empty())
    {
        return p_source;
    }
    Node *base = Object::cast_to<Node>(p_source);
    if (!base || !base->is_inside_tree())
    {
        base = ArgumentBinding::fallback_base_node();
    }
    return base ? base->get_node_or_null(target_path) : nullptr;
}

void SignalLink::connect_to(Object *p_source, const StringName &p_signal)
{
    disconnect_from(p_source); // idempotent : jamais deux connexions du même lien sur le même émetteur
    if (!p_source || !enabled || String(method).is_empty() || !p_source->has_signal(p_signal))
    {
        return;
    }
    Callable callable(memnew(SignalLinkCallable(ObjectID(get_instance_id()), ObjectID(p_source->get_instance_id()))));
    p_source->connect(p_signal, callable);
    Array entry;
    entry.push_back(p_signal);
    entry.push_back(callable);
    live[(int64_t)p_source->get_instance_id()] = entry;
}

void SignalLink::disconnect_from(Object *p_source)
{
    if (!p_source)
    {
        return;
    }
    int64_t key = (int64_t)p_source->get_instance_id();
    if (!live.has(key))
    {
        return;
    }
    Array entry = live[key];
    live.erase(key);
    StringName signal = entry[0];
    Callable callable = entry[1];
    if (p_source->is_connected(signal, callable))
    {
        p_source->disconnect(signal, callable);
    }
}

bool SignalLink::is_connected_to(Object *p_source) const
{
    if (!p_source || !live.has((int64_t)p_source->get_instance_id()))
    {
        return false;
    }
    Array entry = live[(int64_t)p_source->get_instance_id()];
    return p_source->is_connected(entry[0], entry[1]);
}

void SignalLink::invoke(Object *p_source, const Array &p_signal_args)
{
    if (!enabled || String(method).is_empty())
    {
        return;
    }
    Object *target = resolve_target(p_source);
    if (!target || !target->has_method(method))
    {
        log_it(U"[color=orange]Lien : cible ou méthode « " + String(method) + U" » introuvable[/color]", "SignalLink");
        return;
    }
    // Protection contre la réentrée : une méthode qui réémet le signal qui l'a appelée bouclerait.
    if (invoking)
    {
        log_it(U"[color=orange]Appel récursif de « " + String(method) + U" » ignoré (la méthode réémet le signal qui la déclenche)[/color]", "SignalLink");
        return;
    }
    Array values;
    for (int i = 0; i < arguments.size(); i++)
    {
        Ref<ArgumentBinding> binding = arguments[i];
        values.push_back(binding.is_valid() ? binding->resolve(p_source, p_signal_args) : Variant());
    }
    invoking = true;
    target->callv(method, values);
    invoking = false;
}
