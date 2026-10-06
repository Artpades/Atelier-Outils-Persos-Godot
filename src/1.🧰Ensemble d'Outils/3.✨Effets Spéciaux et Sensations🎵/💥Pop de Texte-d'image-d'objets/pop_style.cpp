#include "pop_style.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

float PopStyle::get_scale_at(double p_age) const
{
    if (scale_curve.is_valid())
    {
        return scale_curve->sample(lifetime > 0.0 ? (float)Math::clamp(p_age / lifetime, 0.0, 1.0) : 1.0f);
    }

    float scale = 1.0f;
    if (p_age < pop_in_duration)
    {
        // Apparition : grossit au-delà de 1 (pop_scale) sur 60 % de la durée, puis revient à 1
        const float u = (float)(p_age / pop_in_duration);
        scale = u < 0.6f ? Math::lerp(start_scale, pop_scale, Math::smoothstep(0.0f, 1.0f, u / 0.6f))
                         : Math::lerp(pop_scale, 1.0f, Math::smoothstep(0.0f, 1.0f, (u - 0.6f) / 0.4f));
    }
    const double end_start = lifetime - fade_out_duration;
    if (fade_out_duration > 0.0 && p_age > end_start)
    {
        // Fin de vie : réduction jusqu'à end_scale
        scale *= Math::lerp(1.0f, end_scale, (float)Math::clamp((p_age - end_start) / fade_out_duration, 0.0, 1.0));
    }
    return scale;
}

float PopStyle::get_alpha_at(double p_age) const
{
    if (fade_out_duration <= 0.0)
    {
        return 1.0f;
    }
    return 1.0f - (float)Math::clamp((p_age - (lifetime - fade_out_duration)) / fade_out_duration, 0.0, 1.0);
}

#define BIND_POP_PROPERTY(m_variant_type, m_name, ...)                                                     \
    ClassDB::bind_method(D_METHOD("set_" #m_name, "value"), &PopStyle::set_##m_name);                    \
    ClassDB::bind_method(D_METHOD("get_" #m_name), &PopStyle::get_##m_name);                             \
    ADD_PROPERTY(PropertyInfo(Variant::m_variant_type, #m_name, ##__VA_ARGS__), "set_" #m_name, "get_" #m_name)

void PopStyle::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_scale_at", "age"), &PopStyle::get_scale_at);
    ClassDB::bind_method(D_METHOD("get_alpha_at", "age"), &PopStyle::get_alpha_at);

    ADD_GROUP("Motion", "");
    BIND_POP_PROPERTY(FLOAT, lifetime, PROPERTY_HINT_RANGE, "0.05,5,0.01,or_greater,suffix:s");
    BIND_POP_PROPERTY(FLOAT, speed_min, PROPERTY_HINT_RANGE, "0,1000,1,or_greater,suffix:px/s");
    BIND_POP_PROPERTY(FLOAT, speed_max, PROPERTY_HINT_RANGE, "0,1000,1,or_greater,suffix:px/s");
    BIND_POP_PROPERTY(FLOAT, direction_degrees, PROPERTY_HINT_RANGE, "-180,180,1,degrees");
    BIND_POP_PROPERTY(FLOAT, spread_degrees, PROPERTY_HINT_RANGE, "0,360,1,degrees");
    BIND_POP_PROPERTY(FLOAT, gravity, PROPERTY_HINT_RANGE, U"-2000,4000,1,or_less,or_greater,suffix:px/s²");
    BIND_POP_PROPERTY(FLOAT, damping, PROPERTY_HINT_RANGE, "0,20,0.1,or_greater");
    BIND_POP_PROPERTY(FLOAT, spawn_radius, PROPERTY_HINT_RANGE, "0,200,1,or_greater,suffix:px");
    BIND_POP_PROPERTY(FLOAT, spin_min, PROPERTY_HINT_RANGE, U"-1080,1080,1,or_less,or_greater,suffix:°/s");
    BIND_POP_PROPERTY(FLOAT, spin_max, PROPERTY_HINT_RANGE, U"-1080,1080,1,or_less,or_greater,suffix:°/s");

    ADD_GROUP("Floor", "");
    BIND_POP_PROPERTY(BOOL, use_floor);
    BIND_POP_PROPERTY(FLOAT, floor_offset, PROPERTY_HINT_RANGE, "0,500,1,or_greater,suffix:px");
    BIND_POP_PROPERTY(FLOAT, bounce, PROPERTY_HINT_RANGE, "0,1,0.01");
    BIND_POP_PROPERTY(FLOAT, floor_friction, PROPERTY_HINT_RANGE, "0,1,0.01");

    ADD_GROUP("Animation", "");
    BIND_POP_PROPERTY(FLOAT, pop_in_duration, PROPERTY_HINT_RANGE, "0,1,0.01,or_greater,suffix:s");
    BIND_POP_PROPERTY(FLOAT, start_scale, PROPERTY_HINT_RANGE, "0,2,0.01,or_greater");
    BIND_POP_PROPERTY(FLOAT, pop_scale, PROPERTY_HINT_RANGE, "0,3,0.01,or_greater");
    BIND_POP_PROPERTY(FLOAT, end_scale, PROPERTY_HINT_RANGE, "0,2,0.01,or_greater");
    BIND_POP_PROPERTY(FLOAT, fade_out_duration, PROPERTY_HINT_RANGE, "0,2,0.01,or_greater,suffix:s");
    BIND_POP_PROPERTY(OBJECT, scale_curve, PROPERTY_HINT_RESOURCE_TYPE, "Curve");

    ADD_GROUP("Appearance", "");
    BIND_POP_PROPERTY(OBJECT, font, PROPERTY_HINT_RESOURCE_TYPE, "Font");
    BIND_POP_PROPERTY(INT, font_size, PROPERTY_HINT_RANGE, "1,256,1,or_greater,suffix:px");
    BIND_POP_PROPERTY(COLOR, text_color);
    BIND_POP_PROPERTY(COLOR, positive_color);
    BIND_POP_PROPERTY(COLOR, negative_color);
    BIND_POP_PROPERTY(INT, outline_size, PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px");
    BIND_POP_PROPERTY(COLOR, outline_color);
    BIND_POP_PROPERTY(VECTOR2, icon_size, PROPERTY_HINT_NONE, "suffix:px");
    BIND_POP_PROPERTY(FLOAT, spacing, PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px");
    BIND_POP_PROPERTY(INT, z_index, PROPERTY_HINT_RANGE, "-4096,4096,1");
    BIND_POP_PROPERTY(BOOL, always_on_top_3d);
}
