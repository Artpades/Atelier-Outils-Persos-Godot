#ifndef CHARACTER_2D_HPP
#define CHARACTER_2D_HPP

#include "character_core.hpp"

#include <godot_cpp/classes/character_body2d.hpp>

namespace godot
{
/// 🧑 Personnage 2D (CharacterBody2D) : une entité qui porte des composants facultatifs — vie
/// (HealthComponent), déplacement (GroundMovement2D, SwimMovement2D... ou MovementDirector du module
/// Mouvement & Physique), capacités (AbilitySystem), contrôleurs (PlayerController, AutomateController,
/// NPCController), comportements, Interactable...
///
/// Ses propriétés propres : identité, faction, hostilités. Les propriétés de ses composants sont aussi
/// éditables depuis son inspecteur (groupes « 🧩 », expose_components). L'intention de déplacement
/// (set_move_input, request_jump, set_running) est transmise au MovementDirector, sinon à chaque
/// MovementComponent ; elle est coupée si le personnage est mort ou porte un tag de movement_blocking_tags.
/// view_mode : vue de côté (seul x compte pour l'IA) ou de dessus ; AUTO suit le mode du déplacement au sol.
class Character2D : public CharacterBody2D, public CharacterAPI
{
    GDCLASS(Character2D, CharacterBody2D)
    CHARACTER_COMMON_DECLARATIONS()

  public:
    enum ViewMode
    {
        VIEW_AUTO,
        VIEW_SIDE,
        VIEW_TOP_DOWN,
    };

    Character2D();
    void set_view_mode(ViewMode p_mode) { view_mode = p_mode; }
    ViewMode get_view_mode() const { return view_mode; }
    bool is_side_view() const;
    bool is_3d() const override { return false; }

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    ViewMode view_mode = VIEW_AUTO;
};
} // namespace godot

VARIANT_ENUM_CAST(Character2D::ViewMode);

#endif // CHARACTER_2D_HPP
