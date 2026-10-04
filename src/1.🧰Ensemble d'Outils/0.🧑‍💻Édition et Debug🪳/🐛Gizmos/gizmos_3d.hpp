#ifndef GIZMOS_3D_HPP
#define GIZMOS_3D_HPP

#include "gizmos.hpp"

#include <godot_cpp/classes/ref_counted.hpp>

namespace godot
{
// Version exposée à GDScript : Gizmos3D.draw_3D_line(...)
// Les méthodes sont liées directement aux fonctions de Gizmos (voir _bind_methods) ;
// en C++, utiliser Gizmos::... directement.
class Gizmos3D : public RefCounted
{
    GDCLASS(Gizmos3D, RefCounted)

  protected:
    static void _bind_methods();
};

} // namespace godot

#endif // GIZMOS_3D_HPP
