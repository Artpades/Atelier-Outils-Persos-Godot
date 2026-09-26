#include "gauge.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void Gauge::set_current_value(float p_value)
{
    float clamped_value = Math::clamp(p_value, min_value, max_value);
    if (clamped_value == current_value)
    {
        return;
    }

    previous_value = current_value;
    current_value = clamped_value;

    emit_signal("current_value_changed", current_value);
    check_thresholds();
}

void Gauge::set_min_value(float p_value)
{
    min_value = p_value;

    // La borne haute suit la borne basse pour que la plage reste valide.
    if (min_value > max_value)
    {
        max_value = min_value;
        emit_signal("max_value_changed", max_value);
    }

    set_current_value(current_value);
    emit_signal("min_value_changed", min_value);
}

void Gauge::set_max_value(float p_value)
{
    max_value = p_value;

    // La borne basse suit la borne haute pour que la plage reste valide.
    if (max_value < min_value)
    {
        min_value = max_value;
        emit_signal("min_value_changed", min_value);
    }

    set_current_value(current_value);
    emit_signal("max_value_changed", max_value);
}

float Gauge::get_percentage() const
{
    float range = max_value - min_value;
    if (Math::is_zero_approx(range))
    {
        return 100.0f;
    }
    return (current_value - min_value) / range * 100.0f;
}

void Gauge::check_thresholds()
{
    for (float threshold : thresholds)
    {
        // Conversion du pourcentage en valeur absolue dans la plage [min_value, max_value].
        float threshold_value = min_value + threshold / 100.0f * (max_value - min_value);

        if (previous_value < threshold_value && current_value >= threshold_value)
        {
            emit_signal("threshold_crossed_up", threshold);
        }
        else if (previous_value > threshold_value && current_value <= threshold_value)
        {
            emit_signal("threshold_crossed_down", threshold);
        }
    }
}

void Gauge::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_current_value", "value"), &Gauge::set_current_value);
    ClassDB::bind_method(D_METHOD("get_current_value"), &Gauge::get_current_value);
    ClassDB::bind_method(D_METHOD("set_min_value", "value"), &Gauge::set_min_value);
    ClassDB::bind_method(D_METHOD("get_min_value"), &Gauge::get_min_value);
    ClassDB::bind_method(D_METHOD("set_max_value", "value"), &Gauge::set_max_value);
    ClassDB::bind_method(D_METHOD("get_max_value"), &Gauge::get_max_value);
    ClassDB::bind_method(D_METHOD("set_thresholds", "thresholds"), &Gauge::set_thresholds);
    ClassDB::bind_method(D_METHOD("get_thresholds"), &Gauge::get_thresholds);
    ClassDB::bind_method(D_METHOD("get_percentage"), &Gauge::get_percentage);

    // Les bornes sont déclarées avant current_value : au chargement d'une ressource, les propriétés sont
    // assignées dans cet ordre, et current_value serait sinon clampée par les bornes par défaut.
    ADD_GROUP("Propriétés", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_value"), "set_min_value", "get_min_value");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_value"), "set_max_value", "get_max_value");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "current_value"), "set_current_value", "get_current_value");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "thresholds", PROPERTY_HINT_TYPE_STRING,
                              vformat("%d/%d:%s", Variant::FLOAT, PROPERTY_HINT_RANGE, "0,100,0.1,suffix:%")),
                 "set_thresholds", "get_thresholds");

    ADD_SIGNAL(MethodInfo("min_value_changed", PropertyInfo(Variant::FLOAT, "new_value")));
    ADD_SIGNAL(MethodInfo("current_value_changed", PropertyInfo(Variant::FLOAT, "new_value")));
    ADD_SIGNAL(MethodInfo("max_value_changed", PropertyInfo(Variant::FLOAT, "new_value")));
    ADD_SIGNAL(MethodInfo("threshold_crossed_up", PropertyInfo(Variant::FLOAT, "threshold_percent")));
    ADD_SIGNAL(MethodInfo("threshold_crossed_down", PropertyInfo(Variant::FLOAT, "threshold_percent")));
}
