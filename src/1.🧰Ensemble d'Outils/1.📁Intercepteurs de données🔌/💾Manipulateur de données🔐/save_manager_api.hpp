#ifndef SAVE_MANAGER_API_HPP
#define SAVE_MANAGER_API_HPP

#include "commun/module_dependencies.hpp"

#include <godot_cpp/classes/resource.hpp>

namespace godot::save_manager_api
{
// 💾 Accès au SaveManager depuis les AUTRES modules. Les SaveData / SaveConfig y sont manipulés comme des
// Resource (leur classe C++ n'existe pas hors du module) : propriétés et méthodes via get() / call().
// Sans le module, rien n'est écrit et load() retourne une référence nulle : l'appelant doit vérifier la
// dépendance (modules::require).

inline const modules::Dependency DEPENDENCY = {"SaveManager", U"💾 Manipulateur de données"};

inline void save(const Ref<Resource> &p_save_data, const Ref<Resource> &p_config) { modules::call_static("SaveManager", "save", p_save_data, p_config); }

inline Ref<Resource> load(const Ref<Resource> &p_save_data, const Ref<Resource> &p_config)
{
    return modules::call_static("SaveManager", "load", p_save_data, p_config);
}
} // namespace godot::save_manager_api

#endif // SAVE_MANAGER_API_HPP
