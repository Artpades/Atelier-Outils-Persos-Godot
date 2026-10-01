#ifndef GIZMOS_API_HPP
#define GIZMOS_API_HPP

#include "commun/module_dependencies.hpp"

#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot
{
// 🐛 Accès aux Gizmos depuis les AUTRES modules.
// Sans le module Gizmos : create_* retourne un RID invalide et les autres appels ne font rien.
// Les arguments sont transmis tels quels aux méthodes liées de Gizmos2D / Gizmos3D (mêmes paramètres,
// mêmes valeurs par défaut).

namespace gizmos_2d_api
{
inline const modules::Dependency DEPENDENCY = {"Gizmos2D", U"🧑‍💻 Édition et Debug"};

inline bool is_available() { return modules::is_loaded("Gizmos2D"); }

inline RID create_2D_gizmos(Viewport *p_viewport) { return modules::call_static("Gizmos2D", "create_2D_gizmos", p_viewport); }
inline void clear_2D_gizmos(const RID &p_gizmo_rid) { modules::call_static("Gizmos2D", "clear_2D_gizmos", p_gizmo_rid); }
inline void free_gizmos(const RID &p_gizmo_rid) { modules::call_static("Gizmos2D", "free_gizmos", p_gizmo_rid); }

template <typename... Args> void draw_2D_line(const Args &...p_args) { modules::call_static("Gizmos2D", "draw_2D_line", p_args...); }
template <typename... Args> void draw_2D_arrow(const Args &...p_args) { modules::call_static("Gizmos2D", "draw_2D_arrow", p_args...); }
template <typename... Args> void draw_2D_circle(const Args &...p_args) { modules::call_static("Gizmos2D", "draw_2D_circle", p_args...); }
template <typename... Args> void draw_2D_rectangle(const Args &...p_args) { modules::call_static("Gizmos2D", "draw_2D_rectangle", p_args...); }
template <typename... Args> void draw_2D_text(const Args &...p_args) { modules::call_static("Gizmos2D", "draw_2D_text", p_args...); }
} // namespace gizmos_2d_api

namespace gizmos_3d_api
{
inline const modules::Dependency DEPENDENCY = {"Gizmos3D", U"🧑‍💻 Édition et Debug"};

inline bool is_available() { return modules::is_loaded("Gizmos3D"); }

inline RID create_3D_gizmo(Viewport *p_viewport) { return modules::call_static("Gizmos3D", "create_3D_gizmo", p_viewport); }
inline void free_3D_gizmo(const RID &p_gizmo_rid) { modules::call_static("Gizmos3D", "free_3D_gizmo", p_gizmo_rid); }

template <typename... Args> void draw_3D_cube(const Args &...p_args) { modules::call_static("Gizmos3D", "draw_3D_cube", p_args...); }
} // namespace gizmos_3d_api
} // namespace godot

#endif // GIZMOS_API_HPP
