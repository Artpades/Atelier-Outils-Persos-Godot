#ifndef PROCEDURAL_INSPECTOR_API_HPP
#define PROCEDURAL_INSPECTOR_API_HPP

#include "inspector_core.hpp"

#include "commun/module_dependencies.hpp"

#include <godot_cpp/classes/control.hpp>

namespace godot::procedural_inspector_api
{
// 🔍 Liaisons InspectorBinding depuis les AUTRES modules (via ProceduralInspector.bind_value).
// Sans le module, aucune liaison n'est créée : l'appelant doit vérifier la dépendance (modules::require).

inline const modules::Dependency DEPENDENCY = {"ProceduralInspector", U"🧩 Générateur de Widget d'éditions"};

/// Lie `p_owner` à la valeur retournée par `p_get()` : `p_pull(valeur)` est appelé à chaque changement.
/// `p_source` : objet lu par `p_get` ; la liaison s'arrête quand il est libéré.
/// `p_setter` (optionnel) : écrit la valeur lors d'un `commit(valeur)` sur la liaison retournée.
template <typename Get, typename Pull> Node *bind_value(Control *p_owner, Object *p_source, Get p_get, Pull p_pull, const Callable &p_setter = Callable())
{
    LambdaCallable::Function getter = [p_get](const Variant **, int) { return Variant(p_get()); };
    Variant binding = modules::call_static("ProceduralInspector", "bind_value", p_owner, Callable(memnew(LambdaCallable(p_source, getter))),
                                           lambda_callable_1(p_owner, p_pull), p_setter);
    return Object::cast_to<Node>((Object *)binding);
}

/// Force la relecture de toutes les liaisons sous `p_root`.
inline void refresh_bindings(Node *p_root) { modules::call_static("ProceduralInspector", "refresh_bindings", p_root); }
} // namespace godot::procedural_inspector_api

#endif // PROCEDURAL_INSPECTOR_API_HPP
