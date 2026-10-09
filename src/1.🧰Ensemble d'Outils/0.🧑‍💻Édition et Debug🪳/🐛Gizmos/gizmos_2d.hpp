#ifndef GIZMOS_2D_HPP
#define GIZMOS_2D_HPP

#include <godot_cpp/classes/ref_counted.hpp>

namespace godot
{
/// 🐛 Version exposée à GDScript : Gizmos2D.draw_2D_line(...). Classe purement statique.
// Les méthodes sont liées directement aux fonctions de Gizmos (voir _bind_methods) ;
// en C++, utiliser Gizmos::... directement.
class Gizmos2D : public RefCounted
{
    GDCLASS(Gizmos2D, RefCounted)

  protected:
    static void _bind_methods();
};

} // namespace godot

#endif // GIZMOS_2D_HPP
