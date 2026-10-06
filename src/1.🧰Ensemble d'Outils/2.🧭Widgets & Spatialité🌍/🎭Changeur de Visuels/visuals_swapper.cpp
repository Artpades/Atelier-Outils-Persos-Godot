#include "visuals_swapper.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void VisualsSwapper::swap(const Ref<VisualsAsset> &p_visuals_asset)
{
    if (p_visuals_asset.is_null())
    {
        return;
    }
    // Ce nœud sert de contexte pour la résolution des chemins
    p_visuals_asset->apply(this);
}

void VisualsSwapper::swap_by_name(const String &p_suit_type_name)
{
    for (int64_t i = 0; i < visuals_assets.size(); i++)
    {
        Ref<VisualsAsset> asset = visuals_assets[i];
        if (asset.is_valid() && asset->get_suit_type_name() == p_suit_type_name)
        {
            swap(asset);
            return;
        }
    }
}

void VisualsSwapper::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("swap", "visuals_asset"), &VisualsSwapper::swap);
    ClassDB::bind_method(D_METHOD("swap_by_name", "suit_type_name"), &VisualsSwapper::swap_by_name);
    ClassDB::bind_method(D_METHOD("set_visuals_assets", "assets"), &VisualsSwapper::set_visuals_assets);
    ClassDB::bind_method(D_METHOD("get_visuals_assets"), &VisualsSwapper::get_visuals_assets);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "visuals_assets", PROPERTY_HINT_ARRAY_TYPE, "VisualsAsset"), "set_visuals_assets", "get_visuals_assets");
}
