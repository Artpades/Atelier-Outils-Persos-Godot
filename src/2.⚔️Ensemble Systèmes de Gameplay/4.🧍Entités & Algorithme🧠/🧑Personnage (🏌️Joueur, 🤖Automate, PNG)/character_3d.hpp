#ifndef CHARACTER_3D_HPP
#define CHARACTER_3D_HPP

#include "character_core.hpp"

#include <godot_cpp/classes/character_body3d.hpp>

namespace godot
{
/// 🧑 Personnage 3D (CharacterBody3D) : même principe que Character2D — une entité porteuse de composants
/// facultatifs (vie, déplacement GroundMovement3D / MovementDirector, capacités, contrôleurs...).
/// L'entrée de déplacement transmise est dans le plan horizontal du monde (x → X, y → Z).
class Character3D : public CharacterBody3D, public CharacterAPI
{
    GDCLASS(Character3D, CharacterBody3D)
    CHARACTER_COMMON_DECLARATIONS()

  public:
    Character3D();
    bool is_3d() const override { return true; }

  protected:
    static void _bind_methods();
    void _notification(int p_what);
};
} // namespace godot

#endif // CHARACTER_3D_HPP
