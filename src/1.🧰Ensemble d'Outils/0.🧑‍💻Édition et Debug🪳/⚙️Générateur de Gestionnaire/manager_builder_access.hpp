#ifndef MANAGER_BUILDER_ACCESS_HPP
#define MANAGER_BUILDER_ACCESS_HPP

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// ⚙️ Accès au ManagerBuilder depuis un AUTRE module GDExtension.
//
// ManagerBuilder est compilé dans le module tool_editor : un autre module ne peut pas inclure
// manager_builder.hpp (son code ne serait pas lié). Ces fonctions passent donc par ClassDB
// (class_call_static). Définies dans le header (comme log_it) pour être utilisables partout.
//
// Utilisation dans un gestionnaire (équivalent du MakeGetter / InitInstance C#) :
//
//     static ObjectID instance_id;
//     static MonManager *get_instance() { return manager_builder::get<MonManager>(instance_id); }
//     void _enter_tree() override { if (!manager_builder::init(instance_id, this)) return; ... }
//     void _exit_tree() override { manager_builder::release(instance_id, this); }
//
// r_cache garde l'ObjectID de l'instance canonique : sans le module tool_editor, l'instance placée
// en scène reste accessible (mais aucune instance n'est créée à la demande).
namespace manager_builder
{
/// true si la classe ManagerBuilder est chargée (bibliothèque tool_editor présente).
inline bool is_available()
{
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    return cdb && cdb->class_exists("ManagerBuilder");
}

/// Retourne le singleton de T (créé par ManagerBuilder s'il n'existe pas encore).
template <typename T>
T *get(ObjectID &r_cache)
{
    if (is_available())
    {
        Object *object = ClassDBSingleton::get_singleton()->class_call_static("ManagerBuilder", "make_getter", StringName(T::get_class_static()));
        T *instance = Object::cast_to<T>(object);
        r_cache = instance ? ObjectID(instance->get_instance_id()) : ObjectID();
        return instance;
    }
    return Object::cast_to<T>(ObjectDB::get_instance(r_cache));
}

/// Enregistre p_self comme instance canonique. Retourne false si une autre instance existait déjà
// (p_self est alors détruit par queue_free et ne doit plus s'initialiser).
template <typename T>
bool init(ObjectID &r_cache, T *p_self)
{
    if (is_available())
    {
        Object *canonical = ClassDBSingleton::get_singleton()->class_call_static("ManagerBuilder", "init_instance", StringName(T::get_class_static()), static_cast<Object *>(p_self));
        r_cache = canonical ? ObjectID(canonical->get_instance_id()) : ObjectID();
        return canonical == p_self;
    }

    T *existing = Object::cast_to<T>(ObjectDB::get_instance(r_cache));
    if (existing && existing != p_self)
    {
        p_self->queue_free();
        return false;
    }
    r_cache = ObjectID(p_self->get_instance_id());
    return true;
}

/// Désenregistre p_self s'il est l'instance canonique (sans effet sinon).
template <typename T>
void release(ObjectID &r_cache, T *p_self)
{
    if (is_available())
    {
        ClassDBSingleton::get_singleton()->class_call_static("ManagerBuilder", "unregister_instance", StringName(T::get_class_static()), static_cast<Object *>(p_self));
    }
    if (r_cache == ObjectID(p_self->get_instance_id()))
    {
        r_cache = ObjectID();
    }
}
} // namespace manager_builder
} // namespace godot

#endif // MANAGER_BUILDER_ACCESS_HPP
