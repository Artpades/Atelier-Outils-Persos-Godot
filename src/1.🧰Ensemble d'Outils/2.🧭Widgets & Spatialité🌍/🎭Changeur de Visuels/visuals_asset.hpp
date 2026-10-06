#ifndef VISUALS_ASSET_HPP
#define VISUALS_ASSET_HPP

#include "visuals_register_resource.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// 🎭 Ensemble de registres visuels associés à une tenue ou à une configuration donnée.
class VisualsAsset : public Resource
{
    GDCLASS(VisualsAsset, Resource)

  public:
    /// Modifie l'apparence du nœud désigné par p_register (chemin résolu depuis p_context).
    static void swap_visuals(Node *p_context, const Ref<VisualsRegister> &p_register);
    /// Applique tous les registres de l'asset.
    void apply(Node *p_context) const;

    void set_suit_type_name(const String &p_name) { suit_type_name = p_name; }
    String get_suit_type_name() const { return suit_type_name; }
    void set_visuals_registers(const TypedArray<VisualsRegister> &p_registers) { visuals_registers = p_registers; }
    TypedArray<VisualsRegister> get_visuals_registers() const { return visuals_registers; }

  protected:
    static void _bind_methods();

  private:
    /// Identifiant unique du type de tenue ou de pouvoir
    String suit_type_name;
    TypedArray<VisualsRegister> visuals_registers;
};
} // namespace godot

#endif // VISUALS_ASSET_HPP
