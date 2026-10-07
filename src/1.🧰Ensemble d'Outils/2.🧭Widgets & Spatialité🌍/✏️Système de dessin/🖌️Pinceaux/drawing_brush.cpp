#include "drawing_brush.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <cmath>

using namespace godot;

// 🧻 Couverture

float DrawingBrush::Coverage::sample(float p_u, float p_v, bool p_wrap) const
{
    if (values.empty())
    {
        return 0.0f;
    }
    float x = p_u * width - 0.5f;
    float y = p_v * height - 0.5f;
    if (!p_wrap && (x < -1.0f || y < -1.0f || x > width || y > height))
    {
        return 0.0f;
    }
    const float fx = std::floor(x);
    const float fy = std::floor(y);
    const float tx = x - fx;
    const float ty = y - fy;
    auto at = [&](int p_x, int p_y) -> float
    {
        if (p_wrap)
        {
            p_x = ((p_x % width) + width) % width;
            p_y = ((p_y % height) + height) % height;
        }
        else if (p_x < 0 || p_y < 0 || p_x >= width || p_y >= height)
        {
            return 0.0f;
        }
        return values[size_t(p_y) * width + p_x];
    };
    const int ix = int(fx);
    const int iy = int(fy);
    const float top = Math::lerp(at(ix, iy), at(ix + 1, iy), tx);
    const float bottom = Math::lerp(at(ix, iy + 1), at(ix + 1, iy + 1), tx);
    return Math::lerp(top, bottom, ty);
}

DrawingBrush::Coverage DrawingBrush::make_coverage(const Ref<Texture2D> &p_texture, bool p_luminance_only)
{
    Coverage coverage;
    if (p_texture.is_null())
    {
        return coverage;
    }
    Ref<Image> image = p_texture->get_image();
    if (image.is_null() || image->is_empty())
    {
        return coverage;
    }
    image = image->duplicate();
    if (image->is_compressed())
    {
        image->decompress();
    }
    image->convert(Image::FORMAT_RGBA8);
    const bool use_alpha = !p_luminance_only && image->detect_alpha() != Image::ALPHA_NONE;
    coverage.width = image->get_width();
    coverage.height = image->get_height();
    coverage.values.resize(size_t(coverage.width) * coverage.height);
    const PackedByteArray data = image->get_data();
    const uint8_t *pixels = data.ptr();
    for (size_t i = 0; i < coverage.values.size(); i++)
    {
        const uint8_t *p = pixels + i * 4;
        const float luminance = (0.2126f * p[0] + 0.7152f * p[1] + 0.0722f * p[2]) / 255.0f;
        if (p_luminance_only)
        {
            coverage.values[i] = luminance; // grain : clair = la peinture accroche
        }
        else
        {
            coverage.values[i] = use_alpha ? p[3] / 255.0f : 1.0f - luminance; // forme : opaque ou sombre = encre
        }
    }
    return coverage;
}

// 🧪 Préréglages

Ref<ImageTexture> DrawingBrush::create_grain_texture(int p_size, int p_seed, float p_contrast)
{
    ERR_FAIL_COND_V(p_size < 4 || p_size > 4096, Ref<ImageTexture>());
    // Bruit de valeur à deux octaves, répétable (bords raccordés) : grain de papier pour le crayon.
    auto hash = [p_seed](int p_x, int p_y) -> float
    {
        uint32_t h = uint32_t(p_x) * 374761393u + uint32_t(p_y) * 668265263u + uint32_t(p_seed) * 2246822519u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return float((h ^ (h >> 16)) & 0xffff) / 65535.0f;
    };
    auto noise = [&](float p_x, float p_y, int p_period) -> float
    {
        const int x0 = int(std::floor(p_x));
        const int y0 = int(std::floor(p_y));
        const float tx = p_x - x0;
        const float ty = p_y - y0;
        const float sx = tx * tx * (3 - 2 * tx);
        const float sy = ty * ty * (3 - 2 * ty);
        auto h = [&](int p_i, int p_j) { return hash(((p_i % p_period) + p_period) % p_period, ((p_j % p_period) + p_period) % p_period); };
        return Math::lerp(Math::lerp(h(x0, y0), h(x0 + 1, y0), sx), Math::lerp(h(x0, y0 + 1), h(x0 + 1, y0 + 1), sx), sy);
    };
    Ref<Image> image = Image::create_empty(p_size, p_size, false, Image::FORMAT_L8);
    PackedByteArray data;
    data.resize(p_size * p_size);
    uint8_t *w = data.ptrw();
    for (int y = 0; y < p_size; y++)
    {
        for (int x = 0; x < p_size; x++)
        {
            float v = 0.65f * noise(x / 2.0f, y / 2.0f, p_size / 2) + 0.35f * hash(x, y);
            v = Math::clamp((v - 0.5f) * p_contrast + 0.5f, 0.0f, 1.0f);
            w[y * p_size + x] = uint8_t(v * 255.0f);
        }
    }
    image->set_data(p_size, p_size, false, Image::FORMAT_L8, data);
    return ImageTexture::create_from_image(image);
}

Ref<DrawingBrush> DrawingBrush::create_preset(Preset p_preset)
{
    Ref<DrawingBrush> brush;
    brush.instantiate();
    switch (p_preset)
    {
    case PRESET_PEN:
        brush->set_name(U"Plume G");
        brush->set_size(8.0f);
        brush->set_hardness(1.0f);
        brush->set_spacing(0.08f);
        brush->set_size_dynamics(DYNAMIC_PRESSURE);
        brush->set_size_min(0.15f);
        brush->set_stabilization(10);
        break;
    case PRESET_PENCIL:
        brush->set_name(U"Crayon");
        brush->set_size(5.0f);
        brush->set_hardness(0.6f);
        brush->set_spacing(0.15f);
        brush->set_flow(0.6f);
        brush->set_size_dynamics(DYNAMIC_PRESSURE);
        brush->set_size_min(0.6f);
        brush->set_opacity_dynamics(DYNAMIC_PRESSURE);
        brush->set_opacity_min(0.2f);
        brush->set_texture(create_grain_texture(128, 7, 1.6f));
        brush->set_texture_strength(0.85f);
        brush->set_stabilization(4);
        break;
    case PRESET_MARKER:
        brush->set_name(U"Feutre");
        brush->set_size(16.0f);
        brush->set_hardness(0.95f);
        brush->set_size_dynamics(0);
        brush->set_opacity(0.85f);
        brush->set_stabilization(4);
        break;
    case PRESET_AIRBRUSH:
        brush->set_name(U"Aérographe");
        brush->set_size(80.0f);
        brush->set_hardness(0.0f);
        brush->set_flow(0.08f);
        brush->set_spacing(0.06f);
        brush->set_size_dynamics(0);
        brush->set_opacity_dynamics(DYNAMIC_PRESSURE);
        brush->set_stabilization(0);
        break;
    case PRESET_WATERCOLOR:
        brush->set_name(U"Lavis");
        brush->set_size(40.0f);
        brush->set_hardness(0.3f);
        brush->set_opacity(0.5f);
        brush->set_flow(0.4f);
        brush->set_size_dynamics(DYNAMIC_VELOCITY);
        brush->set_size_min(0.5f);
        brush->set_opacity_dynamics(DYNAMIC_PRESSURE);
        brush->set_opacity_min(0.3f);
        brush->set_stabilization(8);
        break;
    case PRESET_INK_VELOCITY:
        brush->set_name(U"Encre à la vitesse");
        brush->set_size(14.0f);
        brush->set_hardness(1.0f);
        brush->set_spacing(0.06f);
        brush->set_size_dynamics(DYNAMIC_PRESSURE | DYNAMIC_VELOCITY);
        brush->set_size_min(0.1f);
        brush->set_stabilization(12);
        break;
    case PRESET_HARD_ERASER:
        brush->set_name(U"Gomme dure");
        brush->set_size(24.0f);
        brush->set_hardness(1.0f);
        brush->set_size_dynamics(0);
        brush->set_blend_mode(BRUSH_BLEND_ERASE);
        brush->set_stabilization(0);
        break;
    case PRESET_SOFT_ERASER:
        brush->set_name(U"Gomme douce");
        brush->set_size(60.0f);
        brush->set_hardness(0.0f);
        brush->set_flow(0.3f);
        brush->set_size_dynamics(0);
        brush->set_opacity_dynamics(DYNAMIC_PRESSURE);
        brush->set_blend_mode(BRUSH_BLEND_ERASE);
        brush->set_stabilization(0);
        break;
    }
    return brush;
}

// ⚙️ Propriétés

void DrawingBrush::set_size(float p_size)
{
    size = Math::clamp(p_size, 0.5f, 2000.0f);
    emit_changed();
}

void DrawingBrush::set_opacity(float p_opacity)
{
    opacity = Math::clamp(p_opacity, 0.0f, 1.0f);
    emit_changed();
}

void DrawingBrush::set_flow(float p_flow)
{
    flow = Math::clamp(p_flow, 0.01f, 1.0f);
    emit_changed();
}

void DrawingBrush::set_hardness(float p_hardness)
{
    hardness = Math::clamp(p_hardness, 0.0f, 1.0f);
    emit_changed();
}

void DrawingBrush::set_spacing(float p_spacing)
{
    spacing = Math::clamp(p_spacing, 0.01f, 5.0f);
    emit_changed();
}

void DrawingBrush::set_angle(float p_degrees)
{
    angle = p_degrees;
    emit_changed();
}

void DrawingBrush::set_roundness(float p_roundness)
{
    roundness = Math::clamp(p_roundness, 0.05f, 1.0f);
    emit_changed();
}

void DrawingBrush::set_follow_direction(bool p_enabled)
{
    follow_direction = p_enabled;
    emit_changed();
}

void DrawingBrush::set_tip_texture(const Ref<Texture2D> &p_texture)
{
    const Callable refresh = callable_mp(this, &DrawingBrush::_update_tip_cache);
    if (tip_texture.is_valid() && tip_texture->is_connected("changed", refresh))
    {
        tip_texture->disconnect("changed", refresh);
    }
    tip_texture = p_texture;
    if (tip_texture.is_valid())
    {
        // Une texture générée en différé (NoiseTexture2D...) prévient quand elle est prête.
        tip_texture->connect("changed", refresh);
    }
    _update_tip_cache();
}

void DrawingBrush::set_blend_mode(BrushBlend p_mode)
{
    blend_mode = p_mode;
    emit_changed();
}

void DrawingBrush::set_texture(const Ref<Texture2D> &p_texture)
{
    const Callable refresh = callable_mp(this, &DrawingBrush::_update_grain_cache);
    if (texture.is_valid() && texture->is_connected("changed", refresh))
    {
        texture->disconnect("changed", refresh);
    }
    texture = p_texture;
    if (texture.is_valid())
    {
        texture->connect("changed", refresh);
    }
    _update_grain_cache();
}

void DrawingBrush::set_texture_scale(float p_scale)
{
    texture_scale = Math::clamp(p_scale, 0.05f, 50.0f);
    emit_changed();
}

void DrawingBrush::set_texture_strength(float p_strength)
{
    texture_strength = Math::clamp(p_strength, 0.0f, 1.0f);
    emit_changed();
}

void DrawingBrush::set_material(const Ref<ShaderMaterial> &p_material)
{
    material = p_material;
    emit_changed();
}

void DrawingBrush::set_size_dynamics(int p_flags)
{
    size_dynamics = p_flags & (DYNAMIC_PRESSURE | DYNAMIC_VELOCITY);
    emit_changed();
}

void DrawingBrush::set_size_min(float p_min)
{
    size_min = Math::clamp(p_min, 0.0f, 1.0f);
    emit_changed();
}

void DrawingBrush::set_opacity_dynamics(int p_flags)
{
    opacity_dynamics = p_flags & (DYNAMIC_PRESSURE | DYNAMIC_VELOCITY);
    emit_changed();
}

void DrawingBrush::set_opacity_min(float p_min)
{
    opacity_min = Math::clamp(p_min, 0.0f, 1.0f);
    emit_changed();
}

void DrawingBrush::set_pressure_curve(const Ref<Curve> &p_curve)
{
    pressure_curve = p_curve;
    emit_changed();
}

void DrawingBrush::set_velocity_curve(const Ref<Curve> &p_curve)
{
    velocity_curve = p_curve;
    emit_changed();
}

void DrawingBrush::set_velocity_max(float p_velocity)
{
    velocity_max = MAX(p_velocity, 1.0f);
    emit_changed();
}

void DrawingBrush::set_stabilization(int p_amount)
{
    stabilization = CLAMP(p_amount, 0, 100);
    emit_changed();
}

// 📈 Dynamique

float DrawingBrush::_dynamic_input(int p_flags, float p_pressure, float p_velocity) const
{
    float input = 1.0f;
    if (p_flags & DYNAMIC_PRESSURE)
    {
        const float pressure = Math::clamp(p_pressure, 0.0f, 1.0f);
        input *= pressure_curve.is_valid() ? pressure_curve->sample(pressure) : pressure;
    }
    if (p_flags & DYNAMIC_VELOCITY)
    {
        // Sans courbe : 1 à l'arrêt, 0 à velocity_max (le trait s'affine en allant vite, comme à l'encre).
        const float velocity = Math::clamp(p_velocity / velocity_max, 0.0f, 1.0f);
        input *= velocity_curve.is_valid() ? velocity_curve->sample(velocity) : 1.0f - velocity;
    }
    return Math::clamp(input, 0.0f, 1.0f);
}

float DrawingBrush::get_size_factor(float p_pressure, float p_velocity) const { return size_dynamics ? Math::lerp(size_min, 1.0f, _dynamic_input(size_dynamics, p_pressure, p_velocity)) : 1.0f; }

float DrawingBrush::get_opacity_factor(float p_pressure, float p_velocity) const
{
    return opacity_dynamics ? Math::lerp(opacity_min, 1.0f, _dynamic_input(opacity_dynamics, p_pressure, p_velocity)) : 1.0f;
}

// 🔘 Forme et grain

float DrawingBrush::get_tip_coverage(float p_local_x, float p_local_y, float p_radius) const
{
    if (!tip_cache.is_empty())
    {
        // La forme est inscrite dans le carré du diamètre, proportions conservées.
        float u = p_local_x / (2.0f * p_radius);
        float v = p_local_y / (2.0f * p_radius);
        if (tip_cache.width >= tip_cache.height)
        {
            v *= float(tip_cache.width) / tip_cache.height;
        }
        else
        {
            u *= float(tip_cache.height) / tip_cache.width;
        }
        return tip_cache.sample(u + 0.5f, v + 0.5f, false);
    }
    const float distance = std::sqrt(p_local_x * p_local_x + p_local_y * p_local_y);
    const float soft = (1.0f - hardness) * p_radius; // largeur du dégradé du bord, en pixels
    if (soft <= 1.0f)
    {
        // Bord net : anticrénelage d'un pixel centré sur le rayon (le trait a bien la taille du pinceau).
        return Math::clamp(p_radius + 0.5f - distance, 0.0f, 1.0f);
    }
    const float coverage = Math::clamp((p_radius - distance) / soft, 0.0f, 1.0f);
    return coverage * coverage * (3.0f - 2.0f * coverage);
}

float DrawingBrush::get_grain(float p_x, float p_y) const
{
    if (grain_cache.is_empty() || texture_strength <= 0.0f)
    {
        return 1.0f;
    }
    const float grain = grain_cache.sample(p_x / (grain_cache.width * texture_scale), p_y / (grain_cache.height * texture_scale), true);
    return Math::lerp(1.0f, grain, texture_strength);
}

void DrawingBrush::_update_tip_cache()
{
    tip_cache = make_coverage(tip_texture, false);
    emit_changed();
}

void DrawingBrush::_update_grain_cache()
{
    grain_cache = make_coverage(texture, true);
    emit_changed();
}

// 🔗 Liaisons

void DrawingBrush::_validate_property(PropertyInfo &p_property) const
{
    if ((p_property.name == StringName("hardness")) && tip_texture.is_valid())
    {
        p_property.usage |= PROPERTY_USAGE_READ_ONLY; // sans effet sur une forme texturée
    }
}

void DrawingBrush::_bind_methods()
{
    ClassDB::bind_static_method("DrawingBrush", D_METHOD("create_preset", "preset"), &DrawingBrush::create_preset);
    ClassDB::bind_static_method("DrawingBrush", D_METHOD("create_grain_texture", "size", "seed", "contrast"), &DrawingBrush::create_grain_texture, DEFVAL(128), DEFVAL(7), DEFVAL(1.6f));

    ClassDB::bind_method(D_METHOD("set_size", "size"), &DrawingBrush::set_size);
    ClassDB::bind_method(D_METHOD("get_size"), &DrawingBrush::get_size);
    ClassDB::bind_method(D_METHOD("set_opacity", "opacity"), &DrawingBrush::set_opacity);
    ClassDB::bind_method(D_METHOD("get_opacity"), &DrawingBrush::get_opacity);
    ClassDB::bind_method(D_METHOD("set_flow", "flow"), &DrawingBrush::set_flow);
    ClassDB::bind_method(D_METHOD("get_flow"), &DrawingBrush::get_flow);
    ClassDB::bind_method(D_METHOD("set_hardness", "hardness"), &DrawingBrush::set_hardness);
    ClassDB::bind_method(D_METHOD("get_hardness"), &DrawingBrush::get_hardness);
    ClassDB::bind_method(D_METHOD("set_spacing", "spacing"), &DrawingBrush::set_spacing);
    ClassDB::bind_method(D_METHOD("get_spacing"), &DrawingBrush::get_spacing);
    ClassDB::bind_method(D_METHOD("set_angle", "degrees"), &DrawingBrush::set_angle);
    ClassDB::bind_method(D_METHOD("get_angle"), &DrawingBrush::get_angle);
    ClassDB::bind_method(D_METHOD("set_roundness", "roundness"), &DrawingBrush::set_roundness);
    ClassDB::bind_method(D_METHOD("get_roundness"), &DrawingBrush::get_roundness);
    ClassDB::bind_method(D_METHOD("set_follow_direction", "enabled"), &DrawingBrush::set_follow_direction);
    ClassDB::bind_method(D_METHOD("get_follow_direction"), &DrawingBrush::get_follow_direction);
    ClassDB::bind_method(D_METHOD("set_tip_texture", "texture"), &DrawingBrush::set_tip_texture);
    ClassDB::bind_method(D_METHOD("get_tip_texture"), &DrawingBrush::get_tip_texture);
    ClassDB::bind_method(D_METHOD("set_blend_mode", "mode"), &DrawingBrush::set_blend_mode);
    ClassDB::bind_method(D_METHOD("get_blend_mode"), &DrawingBrush::get_blend_mode);
    ClassDB::bind_method(D_METHOD("set_texture", "texture"), &DrawingBrush::set_texture);
    ClassDB::bind_method(D_METHOD("get_texture"), &DrawingBrush::get_texture);
    ClassDB::bind_method(D_METHOD("set_texture_scale", "scale"), &DrawingBrush::set_texture_scale);
    ClassDB::bind_method(D_METHOD("get_texture_scale"), &DrawingBrush::get_texture_scale);
    ClassDB::bind_method(D_METHOD("set_texture_strength", "strength"), &DrawingBrush::set_texture_strength);
    ClassDB::bind_method(D_METHOD("get_texture_strength"), &DrawingBrush::get_texture_strength);
    ClassDB::bind_method(D_METHOD("set_material", "material"), &DrawingBrush::set_material);
    ClassDB::bind_method(D_METHOD("get_material"), &DrawingBrush::get_material);
    ClassDB::bind_method(D_METHOD("set_size_dynamics", "flags"), &DrawingBrush::set_size_dynamics);
    ClassDB::bind_method(D_METHOD("get_size_dynamics"), &DrawingBrush::get_size_dynamics);
    ClassDB::bind_method(D_METHOD("set_size_min", "min"), &DrawingBrush::set_size_min);
    ClassDB::bind_method(D_METHOD("get_size_min"), &DrawingBrush::get_size_min);
    ClassDB::bind_method(D_METHOD("set_opacity_dynamics", "flags"), &DrawingBrush::set_opacity_dynamics);
    ClassDB::bind_method(D_METHOD("get_opacity_dynamics"), &DrawingBrush::get_opacity_dynamics);
    ClassDB::bind_method(D_METHOD("set_opacity_min", "min"), &DrawingBrush::set_opacity_min);
    ClassDB::bind_method(D_METHOD("get_opacity_min"), &DrawingBrush::get_opacity_min);
    ClassDB::bind_method(D_METHOD("set_pressure_curve", "curve"), &DrawingBrush::set_pressure_curve);
    ClassDB::bind_method(D_METHOD("get_pressure_curve"), &DrawingBrush::get_pressure_curve);
    ClassDB::bind_method(D_METHOD("set_velocity_curve", "curve"), &DrawingBrush::set_velocity_curve);
    ClassDB::bind_method(D_METHOD("get_velocity_curve"), &DrawingBrush::get_velocity_curve);
    ClassDB::bind_method(D_METHOD("set_velocity_max", "velocity"), &DrawingBrush::set_velocity_max);
    ClassDB::bind_method(D_METHOD("get_velocity_max"), &DrawingBrush::get_velocity_max);
    ClassDB::bind_method(D_METHOD("set_stabilization", "amount"), &DrawingBrush::set_stabilization);
    ClassDB::bind_method(D_METHOD("get_stabilization"), &DrawingBrush::get_stabilization);

    ClassDB::bind_method(D_METHOD("get_size_factor", "pressure", "velocity"), &DrawingBrush::get_size_factor);
    ClassDB::bind_method(D_METHOD("get_opacity_factor", "pressure", "velocity"), &DrawingBrush::get_opacity_factor);
    ClassDB::bind_method(D_METHOD("get_dab_size", "pressure", "velocity"), &DrawingBrush::get_dab_size);
    ClassDB::bind_method(D_METHOD("get_dab_opacity", "pressure", "velocity"), &DrawingBrush::get_dab_opacity);

    ADD_GROUP(U"🔘 Forme", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "size", PROPERTY_HINT_RANGE, "0.5,2000,0.5,exp,suffix:px"), "set_size", "get_size");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "opacity", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_opacity", "get_opacity");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flow", PROPERTY_HINT_RANGE, "0.01,1,0.01"), "set_flow", "get_flow");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hardness", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_hardness", "get_hardness");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spacing", PROPERTY_HINT_RANGE, "0.01,5,0.01"), "set_spacing", "get_spacing");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "angle", PROPERTY_HINT_RANGE, "-180,180,1,suffix:°"), "set_angle", "get_angle");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "roundness", PROPERTY_HINT_RANGE, "0.05,1,0.01"), "set_roundness", "get_roundness");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "follow_direction"), "set_follow_direction", "get_follow_direction");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "tip_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_tip_texture", "get_tip_texture");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "blend_mode", PROPERTY_HINT_ENUM, "Normal,Behind,Erase"), "set_blend_mode", "get_blend_mode");

    ADD_GROUP(U"🧻 Grain", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "texture_scale", PROPERTY_HINT_RANGE, "0.05,50,0.05,exp"), "set_texture_scale", "get_texture_scale");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "texture_strength", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_texture_strength", "get_texture_strength");

    ADD_GROUP(U"✨ Shader", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"), "set_material", "get_material");

    ADD_GROUP(U"📈 Dynamique", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "size_dynamics", PROPERTY_HINT_FLAGS, "Pressure,Velocity"), "set_size_dynamics", "get_size_dynamics");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "size_min", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_size_min", "get_size_min");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "opacity_dynamics", PROPERTY_HINT_FLAGS, "Pressure,Velocity"), "set_opacity_dynamics", "get_opacity_dynamics");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "opacity_min", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_opacity_min", "get_opacity_min");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "pressure_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_pressure_curve", "get_pressure_curve");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "velocity_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_velocity_curve", "get_velocity_curve");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "velocity_max", PROPERTY_HINT_RANGE, "1,20000,1,exp,suffix:px/s"), "set_velocity_max", "get_velocity_max");

    ADD_GROUP(U"🪶 Stabilisation", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "stabilization", PROPERTY_HINT_RANGE, "0,100,1"), "set_stabilization", "get_stabilization");

    BIND_ENUM_CONSTANT(BRUSH_BLEND_NORMAL);
    BIND_ENUM_CONSTANT(BRUSH_BLEND_BEHIND);
    BIND_ENUM_CONSTANT(BRUSH_BLEND_ERASE);

    BIND_ENUM_CONSTANT(DYNAMIC_PRESSURE);
    BIND_ENUM_CONSTANT(DYNAMIC_VELOCITY);

    BIND_ENUM_CONSTANT(PRESET_PEN);
    BIND_ENUM_CONSTANT(PRESET_PENCIL);
    BIND_ENUM_CONSTANT(PRESET_MARKER);
    BIND_ENUM_CONSTANT(PRESET_AIRBRUSH);
    BIND_ENUM_CONSTANT(PRESET_WATERCOLOR);
    BIND_ENUM_CONSTANT(PRESET_INK_VELOCITY);
    BIND_ENUM_CONSTANT(PRESET_HARD_ERASER);
    BIND_ENUM_CONSTANT(PRESET_SOFT_ERASER);
}
