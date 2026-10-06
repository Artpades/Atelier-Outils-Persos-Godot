#ifndef VISUALS_SWAPPER_HPP
#define VISUALS_SWAPPER_HPP

#include "visuals_asset.hpp"

#include <godot_cpp/classes/node.hpp>

namespace godot
{
/// 🎭 Change dynamiquement l'apparence de la scène en appliquant des VisualsAsset.
/// Les chemins des registres sont résolus depuis ce nœud.
class VisualsSwapper : public Node
{
    GDCLASS(VisualsSwapper, Node)

  public:
    /// Applique p_visuals_asset aux nœuds cibles.
    void swap(const Ref<VisualsAsset> &p_visuals_asset);
    /// Applique le premier asset du catalogue dont suit_type_name vaut p_suit_type_name.
    void swap_by_name(const String &p_suit_type_name);

    void set_visuals_assets(const TypedArray<VisualsAsset> &p_assets) { visuals_assets = p_assets; }
    TypedArray<VisualsAsset> get_visuals_assets() const { return visuals_assets; }

  protected:
    static void _bind_methods();

  private:
    /// Catalogue des tenues ou configurations visuelles disponibles
    TypedArray<VisualsAsset> visuals_assets;
};
} // namespace godot

#endif // VISUALS_SWAPPER_HPP
