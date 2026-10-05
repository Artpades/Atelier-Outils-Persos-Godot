#include "gizmo_host.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🐛Gizmos/gizmos.hpp"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void GizmoHost::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_registry", "registry"), &GizmoHost::set_registry);
    ClassDB::bind_method(D_METHOD("get_registry"), &GizmoHost::get_registry);
    ClassDB::bind_method(D_METHOD("set_target_path", "path"), &GizmoHost::set_target_path);
    ClassDB::bind_method(D_METHOD("get_target_path"), &GizmoHost::get_target_path);
    ClassDB::bind_method(D_METHOD("set_viewport_path", "path"), &GizmoHost::set_viewport_path);
    ClassDB::bind_method(D_METHOD("get_viewport_path"), &GizmoHost::get_viewport_path);
    ClassDB::bind_method(D_METHOD("set_target_object", "object"), &GizmoHost::set_target_object);
    ClassDB::bind_method(D_METHOD("get_target_object"), &GizmoHost::get_target_object);
    ClassDB::bind_method(D_METHOD("resolve_target"), &GizmoHost::resolve_target);
    ClassDB::bind_method(D_METHOD("resolve_viewport"), &GizmoHost::resolve_viewport);
    ClassDB::bind_method(D_METHOD("free_all"), &GizmoHost::free_all);
    ClassDB::bind_method(D_METHOD("get_live_rid_count"), &GizmoHost::get_live_rid_count);
    ClassDB::bind_method(D_METHOD("get_draw_count"), &GizmoHost::get_draw_count);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "registry", PROPERTY_HINT_RESOURCE_TYPE, "GizmoRegistry"), "set_registry", "get_registry");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_path"), "set_target_path", "get_target_path");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "viewport_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Viewport"), "set_viewport_path", "get_viewport_path");
}

GizmoHost::GizmoHost() { set_process(true); }

void GizmoHost::_notification(int p_what)
{
    if (p_what == NOTIFICATION_PREDELETE)
    {
        free_all(); // propriétaire unique des RID : rien ne doit lui survivre
    }
}

void GizmoHost::_exit_tree() { free_all(); } // recréés paresseusement au retour dans l'arbre

void GizmoHost::_process(double p_delta)
{
    (void)p_delta;
    _update();
}

void GizmoHost::set_registry(const Ref<GizmoRegistry> &p_registry)
{
    registry = p_registry;
    free_all();
}
Ref<GizmoRegistry> GizmoHost::get_registry() const { return registry; }
void GizmoHost::set_target_path(const NodePath &p_path) { target_path = p_path; }
NodePath GizmoHost::get_target_path() const { return target_path; }
void GizmoHost::set_viewport_path(const NodePath &p_path)
{
    viewport_path = p_path;
    free_all();
}
NodePath GizmoHost::get_viewport_path() const { return viewport_path; }

void GizmoHost::set_target_object(Object *p_object)
{
    target_object_id = p_object ? ObjectID(p_object->get_instance_id()) : ObjectID();
    free_all();
}

Object *GizmoHost::get_target_object() const { return ObjectDB::get_instance(target_object_id); }

Object *GizmoHost::resolve_target() const
{
    if (target_object_id.is_valid())
    {
        return ObjectDB::get_instance(target_object_id); // nullptr si libérée
    }
    if (!is_inside_tree())
    {
        return nullptr;
    }
    if (!target_path.is_empty())
    {
        return get_node_or_null(target_path);
    }
    return get_parent() ? get_parent() : (Node *)this;
}

Viewport *GizmoHost::resolve_viewport() const
{
    if (!is_inside_tree())
    {
        return nullptr;
    }
    if (!viewport_path.is_empty())
    {
        return Object::cast_to<Viewport>(get_node_or_null(viewport_path));
    }
    if (target_object_id.is_valid())
    {
        // Cible directe : son viewport (dans l'éditeur, celui de la vue 2D / le World3D de la vue 3D).
        Node *target = Object::cast_to<Node>(ObjectDB::get_instance(target_object_id));
        return (target && target->is_inside_tree()) ? target->get_viewport() : nullptr;
    }
    return get_viewport();
}

void GizmoHost::_free_slot(const Variant &p_key)
{
    if (!live.has(p_key))
    {
        return;
    }
    Dictionary slot = live[p_key];
    RID rid = slot.get("rid", RID());
    if (rid.is_valid())
    {
        if (GizmoCatalog::is_3d(slot.get("function", "")))
        {
            Gizmos::free_3D_gizmo(rid);
        }
        else
        {
            Gizmos::free_2D_gizmo(rid);
        }
    }
    live.erase(p_key);
}

void GizmoHost::free_all()
{
    Array keys = live.keys();
    for (int i = 0; i < keys.size(); i++)
    {
        _free_slot(keys[i]);
    }
    current_viewport_id = ObjectID();
}

int GizmoHost::get_live_rid_count() const { return live.size(); }

Variant GizmoHost::_checked_argument(const Ref<GizmoEntry> &p_entry, int p_index, const Variant &p_value)
{
    String function = p_entry->get_function();
    Dictionary param = GizmoCatalog::user_param(function, p_index);
    int expected = param.get("type", 0);
    Variant::Type actual = p_value.get_type();
    if (expected == Variant::NIL || (int)actual == expected || (expected == Variant::OBJECT && actual == Variant::NIL))
    {
        return p_value;
    }
    bool numeric = (expected == Variant::INT || expected == Variant::FLOAT) && (actual == Variant::INT || actual == Variant::FLOAT);
    String key = String::num_int64((int64_t)p_entry->get_instance_id()) + ":" + String::num_int64(p_index);
    if (!numeric && !warned.has(key))
    {
        warned[key] = true;
        log_it(U"[color=orange]Gizmo « " + function + U" », paramètre « " + String(param.get("name", "")) + U" » : valeur " +
                   (actual == Variant::NIL ? String("nulle (source introuvable ?)") : Variant::get_type_name(actual)) + U", attendu " +
                   Variant::get_type_name((Variant::Type)expected) + (actual == Variant::NIL ? String(U" : valeur par défaut utilisée") : String(U" : conversion")) + "[/color]",
               this);
    }
    return actual == Variant::NIL ? GizmoCatalog::user_default(function, p_index) : UtilityFunctions::type_convert(p_value, expected);
}
int64_t GizmoHost::get_draw_count() const { return draw_count; }

void GizmoHost::_update()
{
    Viewport *viewport = resolve_viewport();
    if (!viewport || registry.is_null())
    {
        free_all();
        return;
    }
    // Viewport changé : les gizmos existants appartiennent à l'ancien canvas / World3D.
    if (current_viewport_id != ObjectID(viewport->get_instance_id()))
    {
        free_all();
        current_viewport_id = ObjectID(viewport->get_instance_id());
    }

    Object *target = resolve_target();
    Dictionary seen;
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    TypedArray<GizmoEntry> entries = registry->get_entries();
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<GizmoEntry> entry = entries[i];
        if (entry.is_null())
        {
            continue;
        }
        int64_t key = (int64_t)entry->get_instance_id();
        String function = entry->get_function();
        if (!known_functions.has(function))
        {
            known_functions[function] = !GizmoCatalog::method_info(function).is_empty();
        }
        if (!entry->is_shown_now() || !bool(known_functions[function]))
        {
            continue; // pas marquée « vue » : son RID éventuel est libéré plus bas
        }
        seen[key] = true;

        // Fonction changée (ex: 2D -> 3D) : l'ancien gizmo ne convient plus.
        if (live.has(key) && String(Dictionary(live[key]).get("function", "")) != function)
        {
            _free_slot(key);
        }
        if (!live.has(key))
        {
            RID rid = GizmoCatalog::is_3d(function) ? Gizmos::create_3D_gizmo(viewport) : Gizmos::create_2D_gizmo(viewport);
            if (!rid.is_valid())
            {
                continue;
            }
            Dictionary slot;
            slot["rid"] = rid;
            slot["function"] = function;
            slot["drawn"] = false;
            live[key] = slot;
        }
        Dictionary slot = live[key];

        // Arguments : RID du gizmo, puis les sources résolues sur la cible.
        Array args;
        args.push_back(slot["rid"]);
        TypedArray<ArgumentBinding> bindings = entry->get_arguments();
        for (int b = 0; b < bindings.size(); b++)
        {
            Ref<ArgumentBinding> binding = bindings[b];
            args.push_back(_checked_argument(entry, b, binding.is_valid() ? binding->resolve(target, Array()) : Variant()));
        }

        bool drawn = slot.get("drawn", false);
        bool draw = !drawn;
        if (entry->get_redraw() == GizmoEntry::REDRAW_EVERY_FRAME)
        {
            draw = true;
        }
        else if (entry->get_redraw() == GizmoEntry::REDRAW_ON_CHANGE && drawn)
        {
            Variant last = slot.get("args", Variant());
            draw = last.get_type() != Variant::ARRAY || !last.hash_compare(args);
        }
        if (!draw)
        {
            continue;
        }

        Array call;
        call.push_back(GizmoCatalog::class_of(function));
        call.push_back(GizmoCatalog::method_of(function));
        call.append_array(args);
        cdb->callv("class_call_static", call);
        slot["args"] = args.duplicate(true);
        slot["drawn"] = true;
        draw_count++;
    }

    // Entrées retirées, désactivées ou masquées dans ce contexte : leur RID est libéré.
    Array keys = live.keys();
    for (int i = 0; i < keys.size(); i++)
    {
        if (!seen.has(keys[i]))
        {
            _free_slot(keys[i]);
        }
    }
}
