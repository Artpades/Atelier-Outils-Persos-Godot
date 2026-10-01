#ifndef MANAGER_BUILDER_API_HPP
#define MANAGER_BUILDER_API_HPP

#include "commun/module_dependencies.hpp"

#include <godot_cpp/classes/node.hpp>

namespace godot::manager_builder_api
{
// ⚙️ Accès au ManagerBuilder depuis les AUTRES modules (dépendance obligatoire des managers).
// Sans le module, les fonctions retournent nullptr / ne font rien : l'appelant doit vérifier la
// dépendance (modules::require) et se désactiver.

inline const modules::Dependency DEPENDENCY = {"ManagerBuilder", U"🧑‍💻 Édition et Debug"};

inline Node *make_getter(const StringName &p_class_name)
{
    return Object::cast_to<Node>((Object *)modules::call_static("ManagerBuilder", "make_getter", p_class_name));
}

inline Node *init_instance(const StringName &p_class_name, Node *p_caller)
{
    return Object::cast_to<Node>((Object *)modules::call_static("ManagerBuilder", "init_instance", p_class_name, p_caller));
}

inline void unregister_instance(const StringName &p_class_name, Node *p_caller)
{
    modules::call_static("ManagerBuilder", "unregister_instance", p_class_name, p_caller);
}
} // namespace godot::manager_builder_api

#endif // MANAGER_BUILDER_API_HPP
