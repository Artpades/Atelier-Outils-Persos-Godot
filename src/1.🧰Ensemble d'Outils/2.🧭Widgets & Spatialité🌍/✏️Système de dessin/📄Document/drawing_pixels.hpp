#ifndef DRAWING_PIXELS_HPP
#define DRAWING_PIXELS_HPP

#include "../🖌️Pinceaux/drawing_brush.hpp"
#include "drawing_layer.hpp"

#include <godot_cpp/core/math.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

/// Calculs de pixels du système de dessin (RGBA8, alpha non prémultiplié) : application d'un pinceau, fusion
/// des calques, régions par couleur et distances. Fonctions inline privées au module.
namespace drawing_pixels
{
using namespace godot;

inline uint8_t to_byte(float p_value) { return uint8_t(Math::clamp(p_value, 0.0f, 1.0f) * 255.0f + 0.5f); }

/// Applique une source (couleur sr, sg, sb d'alpha sa, 0-1) sur le pixel p_dst selon le mode du pinceau.
/// p_alpha_lock : l'alpha du pixel est conservé (« verrou de transparence »).
inline void paint(const uint8_t *p_src_pixel, uint8_t *p_out, float p_r, float p_g, float p_b, float p_a, DrawingBrush::BrushBlend p_mode, bool p_alpha_lock)
{
    const float dr = p_src_pixel[0] / 255.0f;
    const float dg = p_src_pixel[1] / 255.0f;
    const float db = p_src_pixel[2] / 255.0f;
    const float da = p_src_pixel[3] / 255.0f;
    if (p_a <= 0.0f || (p_mode == DrawingBrush::BRUSH_BLEND_ERASE && p_alpha_lock))
    {
        if (p_out != p_src_pixel)
        {
            p_out[0] = p_src_pixel[0];
            p_out[1] = p_src_pixel[1];
            p_out[2] = p_src_pixel[2];
            p_out[3] = p_src_pixel[3];
        }
        return;
    }
    float r = dr, g = dg, b = db, a = da;
    switch (p_mode)
    {
    case DrawingBrush::BRUSH_BLEND_ERASE:
        a = da * (1.0f - p_a);
        break;
    case DrawingBrush::BRUSH_BLEND_BEHIND:
    {
        const float source = p_a * (1.0f - da);
        a = da + source;
        if (a > 0.0f)
        {
            r = (dr * da + p_r * source) / a;
            g = (dg * da + p_g * source) / a;
            b = (db * da + p_b * source) / a;
        }
        break;
    }
    case DrawingBrush::BRUSH_BLEND_NORMAL:
    default:
    {
        const float backdrop = da * (1.0f - p_a);
        a = p_a + backdrop;
        if (a > 0.0f)
        {
            r = (p_r * p_a + dr * backdrop) / a;
            g = (p_g * p_a + dg * backdrop) / a;
            b = (p_b * p_a + db * backdrop) / a;
        }
        break;
    }
    }
    if (p_alpha_lock)
    {
        a = da;
    }
    p_out[0] = to_byte(r);
    p_out[1] = to_byte(g);
    p_out[2] = to_byte(b);
    p_out[3] = to_byte(a);
}

/// Fonction de fusion séparable B(fond, source) du W3C.
inline float blend_channel(DrawingLayer::LayerBlend p_mode, float p_cb, float p_cs)
{
    switch (p_mode)
    {
    case DrawingLayer::LAYER_BLEND_MULTIPLY:
        return p_cb * p_cs;
    case DrawingLayer::LAYER_BLEND_SCREEN:
        return p_cb + p_cs - p_cb * p_cs;
    case DrawingLayer::LAYER_BLEND_OVERLAY:
        return p_cb <= 0.5f ? 2.0f * p_cb * p_cs : 1.0f - 2.0f * (1.0f - p_cb) * (1.0f - p_cs);
    case DrawingLayer::LAYER_BLEND_DARKEN:
        return MIN(p_cb, p_cs);
    case DrawingLayer::LAYER_BLEND_LIGHTEN:
        return MAX(p_cb, p_cs);
    case DrawingLayer::LAYER_BLEND_COLOR_DODGE:
        if (p_cb <= 0.0f)
        {
            return 0.0f;
        }
        return p_cs >= 1.0f ? 1.0f : MIN(1.0f, p_cb / (1.0f - p_cs));
    case DrawingLayer::LAYER_BLEND_COLOR_BURN:
        if (p_cb >= 1.0f)
        {
            return 1.0f;
        }
        return p_cs <= 0.0f ? 0.0f : 1.0f - MIN(1.0f, (1.0f - p_cb) / p_cs);
    case DrawingLayer::LAYER_BLEND_ADD:
        return MIN(1.0f, p_cb + p_cs);
    case DrawingLayer::LAYER_BLEND_SUBTRACT:
        return MAX(0.0f, p_cb - p_cs);
    case DrawingLayer::LAYER_BLEND_DIFFERENCE:
        return std::fabs(p_cb - p_cs);
    case DrawingLayer::LAYER_BLEND_SOFT_LIGHT:
    {
        if (p_cs <= 0.5f)
        {
            return p_cb - (1.0f - 2.0f * p_cs) * p_cb * (1.0f - p_cb);
        }
        const float d = p_cb <= 0.25f ? ((16.0f * p_cb - 12.0f) * p_cb + 4.0f) * p_cb : std::sqrt(p_cb);
        return p_cb + (2.0f * p_cs - 1.0f) * (d - p_cb);
    }
    case DrawingLayer::LAYER_BLEND_HARD_LIGHT:
        return p_cs <= 0.5f ? 2.0f * p_cb * p_cs : 1.0f - 2.0f * (1.0f - p_cb) * (1.0f - p_cs);
    case DrawingLayer::LAYER_BLEND_NORMAL:
    default:
        return p_cs;
    }
}

/// Compose une source (couleur s, alpha p_sa) sur un fond c (alpha r_ca), modifiés sur place (W3C : fusion puis
/// « source-over »).
inline void blend_over(DrawingLayer::LayerBlend p_mode, float *r_c, float &r_ca, const float *p_s, float p_sa)
{
    if (p_sa <= 0.0f)
    {
        return;
    }
    if (r_ca <= 0.0f)
    {
        r_c[0] = p_s[0];
        r_c[1] = p_s[1];
        r_c[2] = p_s[2];
        r_ca = p_sa;
        return;
    }
    const float out_alpha = p_sa + r_ca * (1.0f - p_sa);
    for (int i = 0; i < 3; i++)
    {
        const float mixed = p_mode == DrawingLayer::LAYER_BLEND_NORMAL ? p_s[i] : (1.0f - r_ca) * p_s[i] + r_ca * blend_channel(p_mode, r_c[i], p_s[i]);
        r_c[i] = (p_sa * mixed + r_ca * r_c[i] * (1.0f - p_sa)) / out_alpha;
    }
    r_ca = out_alpha;
}

/// Vrai si deux pixels sont proches (écart maximal par composante ≤ p_tolerance, en octets). Deux pixels
/// transparents sont toujours semblables.
inline bool similar(const uint8_t *p_a, const uint8_t *p_b, int p_tolerance)
{
    if (p_a[3] == 0 && p_b[3] == 0)
    {
        return true;
    }
    for (int i = 0; i < 4; i++)
    {
        if (std::abs(int(p_a[i]) - int(p_b[i])) > p_tolerance)
        {
            return false;
        }
    }
    return true;
}

/// Remplissage par diffusion (4-voisins, par lignes) depuis p_seed sur les pixels où p_passable est non nul.
inline std::vector<uint8_t> flood(const std::vector<uint8_t> &p_passable, int p_width, int p_height, int p_seed_x, int p_seed_y)
{
    std::vector<uint8_t> region(p_passable.size(), 0);
    if (p_seed_x < 0 || p_seed_y < 0 || p_seed_x >= p_width || p_seed_y >= p_height || !p_passable[size_t(p_seed_y) * p_width + p_seed_x])
    {
        return region;
    }
    std::vector<std::pair<int, int>> stack;
    stack.emplace_back(p_seed_x, p_seed_y);
    auto open = [&](int p_x, int p_y) { return p_passable[size_t(p_y) * p_width + p_x] && !region[size_t(p_y) * p_width + p_x]; };
    while (!stack.empty())
    {
        auto [x, y] = stack.back();
        stack.pop_back();
        if (!open(x, y))
        {
            continue;
        }
        int left = x;
        while (left > 0 && open(left - 1, y))
        {
            left--;
        }
        int right = x;
        while (right < p_width - 1 && open(right + 1, y))
        {
            right++;
        }
        for (int i = left; i <= right; i++)
        {
            region[size_t(y) * p_width + i] = 1;
        }
        for (int dy : {-1, 1})
        {
            const int ny = y + dy;
            if (ny < 0 || ny >= p_height)
            {
                continue;
            }
            bool in_span = false;
            for (int i = left; i <= right; i++)
            {
                const bool can = open(i, ny);
                if (can && !in_span)
                {
                    stack.emplace_back(i, ny);
                }
                in_span = can;
            }
        }
    }
    return region;
}

/// Distance (chanfrein 1 / √2, en pixels) de chaque pixel au pixel « cible » (p_target non nul) le plus proche.
/// Sans cible : une très grande valeur partout.
inline std::vector<float> distance_to(const std::vector<uint8_t> &p_target, int p_width, int p_height)
{
    constexpr float FAR = 1e9f;
    constexpr float DIAGONAL = 1.41421356f;
    std::vector<float> d(p_target.size());
    for (size_t i = 0; i < d.size(); i++)
    {
        d[i] = p_target[i] ? 0.0f : FAR;
    }
    auto at = [&](int p_x, int p_y) -> float & { return d[size_t(p_y) * p_width + p_x]; };
    for (int y = 0; y < p_height; y++)
    {
        for (int x = 0; x < p_width; x++)
        {
            float &value = at(x, y);
            if (x > 0)
            {
                value = MIN(value, at(x - 1, y) + 1.0f);
            }
            if (y > 0)
            {
                value = MIN(value, at(x, y - 1) + 1.0f);
                if (x > 0)
                {
                    value = MIN(value, at(x - 1, y - 1) + DIAGONAL);
                }
                if (x < p_width - 1)
                {
                    value = MIN(value, at(x + 1, y - 1) + DIAGONAL);
                }
            }
        }
    }
    for (int y = p_height - 1; y >= 0; y--)
    {
        for (int x = p_width - 1; x >= 0; x--)
        {
            float &value = at(x, y);
            if (x < p_width - 1)
            {
                value = MIN(value, at(x + 1, y) + 1.0f);
            }
            if (y < p_height - 1)
            {
                value = MIN(value, at(x, y + 1) + 1.0f);
                if (x < p_width - 1)
                {
                    value = MIN(value, at(x + 1, y + 1) + DIAGONAL);
                }
                if (x > 0)
                {
                    value = MIN(value, at(x - 1, y + 1) + DIAGONAL);
                }
            }
        }
    }
    return d;
}

/// Agrandit (p_amount > 0) ou rétrécit (p_amount < 0) une région de p_amount pixels.
inline void grow(std::vector<uint8_t> &r_region, int p_width, int p_height, int p_amount)
{
    if (p_amount == 0)
    {
        return;
    }
    if (p_amount > 0)
    {
        const std::vector<float> d = distance_to(r_region, p_width, p_height);
        for (size_t i = 0; i < r_region.size(); i++)
        {
            r_region[i] = d[i] <= float(p_amount) ? 1 : 0;
        }
        return;
    }
    std::vector<uint8_t> outside(r_region.size());
    for (size_t i = 0; i < r_region.size(); i++)
    {
        outside[i] = r_region[i] ? 0 : 1;
    }
    const std::vector<float> d = distance_to(outside, p_width, p_height);
    for (size_t i = 0; i < r_region.size(); i++)
    {
        if (r_region[i] && d[i] <= float(-p_amount))
        {
            r_region[i] = 0;
        }
    }
}
} // namespace drawing_pixels

#endif // DRAWING_PIXELS_HPP
