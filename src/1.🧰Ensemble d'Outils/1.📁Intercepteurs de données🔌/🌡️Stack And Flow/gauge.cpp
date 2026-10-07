#include "gauge.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

#include <algorithm>
#include <vector>

using namespace godot;

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
    ClassDB::bind_method(D_METHOD("get_previous_value"), &Gauge::get_previous_value);

    ClassDB::bind_method(D_METHOD("get_ratio"), &Gauge::get_ratio);
    ClassDB::bind_method(D_METHOD("set_ratio", "ratio"), &Gauge::set_ratio);
    ClassDB::bind_method(D_METHOD("get_percent"), &Gauge::get_percent);
    ClassDB::bind_method(D_METHOD("add", "amount"), &Gauge::add);
    ClassDB::bind_method(D_METHOD("fill"), &Gauge::fill);
    ClassDB::bind_method(D_METHOD("deplete"), &Gauge::deplete);
    ClassDB::bind_method(D_METHOD("is_full"), &Gauge::is_full);
    ClassDB::bind_method(D_METHOD("is_empty"), &Gauge::is_empty);

    // Ordre important : au chargement, les bornes doivent être posées avant la valeur courante,
    // sinon une valeur supérieure au maximum par défaut serait rognée.
    ADD_GROUP(U"Propriétés", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_value"), "set_min_value", "get_min_value");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_value"), "set_max_value", "get_max_value");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "current_value"), "set_current_value", "get_current_value");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "thresholds"), "set_thresholds", "get_thresholds");

    ADD_SIGNAL(MethodInfo("min_value_changed", PropertyInfo(Variant::FLOAT, "new_value")));
    ADD_SIGNAL(MethodInfo("max_value_changed", PropertyInfo(Variant::FLOAT, "new_value")));
    ADD_SIGNAL(MethodInfo("current_value_changed", PropertyInfo(Variant::FLOAT, "new_value"), PropertyInfo(Variant::FLOAT, "previous_value")));
    ADD_SIGNAL(MethodInfo("threshold_crossed_up", PropertyInfo(Variant::FLOAT, "threshold_percent")));
    ADD_SIGNAL(MethodInfo("threshold_crossed_down", PropertyInfo(Variant::FLOAT, "threshold_percent")));
    ADD_SIGNAL(MethodInfo("filled"));
    ADD_SIGNAL(MethodInfo("depleted"));
}

void Gauge::_validate_property(PropertyInfo &p_property) const
{
    if (p_property.name == StringName("current_value"))
    {
        p_property.hint = PROPERTY_HINT_RANGE;
        p_property.hint_string = String::num(min_value) + "," + String::num(max_value) + ",0.01";
    }
}

#pragma region Getter / Setter

void Gauge::set_current_value(float p_value)
{
    const float clamped_value = Math::clamp(p_value, min_value, max_value);
    if (clamped_value == current_value)
    {
        return;
    }

    previous_value = current_value;
    current_value = clamped_value;

    emit_signal("current_value_changed", current_value, previous_value);
    _check_thresholds();

    if (current_value >= max_value)
    {
        emit_signal("filled");
    }
    else if (current_value <= min_value)
    {
        emit_signal("depleted");
    }
    emit_changed();
}

void Gauge::set_min_value(float p_value)
{
    if (p_value == min_value)
    {
        return;
    }
    min_value = p_value;

    // Garantit que le minimum ne dépasse jamais le maximum.
    if (min_value > max_value)
    {
        max_value = min_value;
        emit_signal("max_value_changed", max_value);
    }

    set_current_value(current_value);
    emit_signal("min_value_changed", min_value);
    notify_property_list_changed();
    emit_changed();
}

void Gauge::set_max_value(float p_value)
{
    if (p_value == max_value)
    {
        return;
    }
    max_value = p_value;

    // Garantit que le maximum n'est jamais inférieur au minimum.
    if (max_value < min_value)
    {
        min_value = max_value;
        emit_signal("min_value_changed", min_value);
    }

    set_current_value(current_value);
    emit_signal("max_value_changed", max_value);
    notify_property_list_changed();
    emit_changed();
}

#pragma endregion

#pragma region Utilitaires

float Gauge::get_ratio() const
{
    const float range = max_value - min_value;
    return range > 0.0f ? (current_value - min_value) / range : 0.0f;
}

void Gauge::set_ratio(float p_ratio)
{
    set_current_value(Math::lerp(min_value, max_value, p_ratio));
}

float Gauge::add(float p_amount)
{
    const float before = current_value;
    set_current_value(current_value + p_amount);
    return current_value - before;
}

#pragma endregion

#pragma region Seuils

float Gauge::_threshold_to_value(float p_percent) const
{
    // 0 % et 100 % tombent exactement sur les bornes (pas d'erreur d'arrondi qui empêcherait de les atteindre).
    if (p_percent <= 0.0f)
    {
        return min_value;
    }
    if (p_percent >= 100.0f)
    {
        return max_value;
    }
    return min_value + p_percent / 100.0f * (max_value - min_value);
}

void Gauge::_check_thresholds()
{
    if (thresholds.is_empty())
    {
        return;
    }

    const bool rising = current_value > previous_value;

    // Seuils triés dans l'ordre où la valeur les traverse : croissant en montée, décroissant en descente.
    std::vector<float> sorted(thresholds.ptr(), thresholds.ptr() + thresholds.size());
    if (rising)
    {
        std::sort(sorted.begin(), sorted.end());
    }
    else
    {
        std::sort(sorted.begin(), sorted.end(), [](float a, float b) { return a > b; });
    }

    for (const float percent : sorted)
    {
        const float threshold_value = _threshold_to_value(percent);
        if (rising && previous_value < threshold_value && current_value >= threshold_value)
        {
            emit_signal("threshold_crossed_up", percent);
        }
        else if (!rising && previous_value > threshold_value && current_value <= threshold_value)
        {
            emit_signal("threshold_crossed_down", percent);
        }
    }
}

#pragma endregion
