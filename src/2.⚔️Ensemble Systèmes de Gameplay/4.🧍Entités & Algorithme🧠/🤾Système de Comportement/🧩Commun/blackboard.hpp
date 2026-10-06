#ifndef BEHAVIOR_BLACKBOARD_HPP
#define BEHAVIOR_BLACKBOARD_HPP

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot
{
/// 📋 Tableau noir : valeurs partagées par les états d'une StateMachine et les nœuds d'un BehaviorTree
/// (une même ressource peut être donnée aux deux pour qu'ils communiquent).
/// Les clés sont des chaînes ; celles qui sont des identifiants valides sont utilisables directement
/// dans les expressions des conditions (ex. « vie < 20 and cible != null »).
/// Locale à la scène par défaut : chaque instance d'une scène a ses propres valeurs.
class Blackboard : public Resource
{
    GDCLASS(Blackboard, Resource)

  public:
    Blackboard() { set_local_to_scene(true); }

    void set_values(const Dictionary &p_values);
    Dictionary get_values() const { return values; }

    /// N'émet value_changed que si la valeur change réellement.
    void set_value(const String &p_key, const Variant &p_value);
    Variant get_value(const String &p_key, const Variant &p_default = Variant()) const;
    bool has_value(const String &p_key) const { return values.has(p_key); }
    void erase_value(const String &p_key);
    void clear();
    PackedStringArray get_keys() const;

  protected:
    static void _bind_methods();

  private:
    Dictionary values;
};
} // namespace godot

#endif // BEHAVIOR_BLACKBOARD_HPP
