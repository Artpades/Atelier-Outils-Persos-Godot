#ifndef CHARACTER_REGISTRY_HPP
#define CHARACTER_REGISTRY_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot
{
/// 📇 Registre des personnages (Character2D / Character3D) présents dans l'arbre de scène, rempli
/// automatiquement. Méthodes statiques : CharacterRegister.get_nearest(moi, 300, true)...
class CharacterRegister : public Object
{
    GDCLASS(CharacterRegister, Object)

  public:
    static void register_character(Node *p_character);
    static void unregister_character(Node *p_character);

    static Array get_characters();
    static Array get_faction_members(const StringName &p_faction);
    /// Personnage le plus proche de p_from (lui exclu). max_distance < 0 : sans limite.
    static Node *get_nearest(Node *p_from, double p_max_distance = -1.0, bool p_hostile_only = false, bool p_alive_only = true);
    static Array get_in_radius(Node *p_from, double p_radius, bool p_hostile_only = false, bool p_alive_only = true);
    /// Personnages qui ont un PlayerController enfant.
    static Array get_players();
    static Node *get_first_player();

  protected:
    static void _bind_methods();

  private:
    static LocalVector<ObjectID> characters;
};
} // namespace godot

#endif // CHARACTER_REGISTRY_HPP
