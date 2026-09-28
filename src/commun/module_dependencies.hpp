#ifndef MODULE_DEPENDENCIES_HPP
#define MODULE_DEPENDENCIES_HPP

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot::modules
{
// 🔗 Dépendances entre modules
//
// Chaque module est une GDExtension séparée (sa propre bibliothèque, sa propre copie de godot-cpp) : une
// classe d'un autre module ne peut donc PAS être appelée en C++ directement (pas d'édition de liens entre
// bibliothèques, et Object::cast_to ne connaît que les classes du module courant).
// Tout passe par la ClassDB du moteur : on teste la présence de la classe, puis on l'appelle par son nom.
// Si le module n'est pas installé (son .gdextension absent du projet), la classe n'existe simplement pas.

/// Une classe fournie par un autre module, et le nom lisible de ce module (pour les messages).
struct Dependency
{
    const char *class_name;
    const char32_t *module_name;
};

/// Vrai si la classe est enregistrée, c.-à-d. si le module qui la fournit est chargé.
inline bool is_loaded(const StringName &p_class)
{
    return ClassDBSingleton::get_singleton()->class_exists(p_class);
}

/// Appelle une méthode statique liée (bind_static_method) d'une classe d'un autre module.
/// Ne fait rien et retourne un Variant nul si le module est absent.
template <typename... Args> Variant call_static(const StringName &p_class, const StringName &p_method, const Args &...p_args)
{
    if (!is_loaded(p_class))
    {
        return Variant();
    }
    return ClassDBSingleton::get_singleton()->class_call_static(p_class, p_method, p_args...);
}

/// Message affiché pour un module manquant.
inline String missing_message(const String &p_user, const Dependency &p_dependency)
{
    return p_user + String(U" nécessite le module « ") + String(p_dependency.module_name) + String(U" » (classe ") + p_dependency.class_name +
           String(U"). Ajoutez son .gdextension au projet.");
}

/// Avertissements « module manquant », à renvoyer depuis _get_configuration_warnings().
template <size_t N> PackedStringArray missing_warnings(const String &p_user, const Dependency (&p_dependencies)[N])
{
    PackedStringArray warnings;
    for (const Dependency &dependency : p_dependencies)
    {
        if (!is_loaded(dependency.class_name))
        {
            warnings.push_back(missing_message(p_user, dependency));
        }
    }
    return warnings;
}

/// Dépendances obligatoires : à tester à l'entrée d'une fonctionnalité.
/// Faux (avec une erreur par module manquant) si l'objet ne peut pas fonctionner.
template <size_t N> bool require(const String &p_user, const Dependency (&p_dependencies)[N])
{
    bool ok = true;
    for (const Dependency &dependency : p_dependencies)
    {
        if (!is_loaded(dependency.class_name))
        {
            ERR_PRINT(missing_message(p_user, dependency));
            ok = false;
        }
    }
    return ok;
}
} // namespace godot::modules

#endif // MODULE_DEPENDENCIES_HPP
