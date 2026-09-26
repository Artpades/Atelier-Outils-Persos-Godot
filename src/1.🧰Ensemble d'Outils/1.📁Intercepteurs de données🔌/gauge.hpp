#ifndef GAUGE_HPP
#define GAUGE_HPP

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

namespace godot
{
/// 📊 Jauge dont la valeur est bornée entre un minimum et un maximum, avec détection de franchissement de seuils.
class Gauge : public Resource
{
    GDCLASS(Gauge, Resource)

  public:
    void set_current_value(float p_value);
    float get_current_value() const { return current_value; }

    void set_min_value(float p_value);
    float get_min_value() const { return min_value; }

    void set_max_value(float p_value);
    float get_max_value() const { return max_value; }

    void set_thresholds(const PackedFloat32Array &p_thresholds) { thresholds = p_thresholds; }
    PackedFloat32Array get_thresholds() const { return thresholds; }

    /// Position de la valeur courante dans la plage, en pourcentage (0 à 100).
    float get_percentage() const;

  protected:
    static void _bind_methods();

  private:
    /// Émet les signaux de seuil franchis entre previous_value et current_value.
    void check_thresholds();

    /// Valeur actuelle de la jauge.
    float current_value = 100.0f;
    /// Valeur avant le dernier changement, pour détecter le sens du franchissement.
    float previous_value = 100.0f;
    /// Borne basse de la jauge.
    float min_value = 0.0f;
    /// Borne haute de la jauge.
    float max_value = 100.0f;
    /// Seuils surveillés, en pourcentage de la plage [min_value, max_value].
    PackedFloat32Array thresholds;
};
} // namespace godot

#endif // GAUGE_HPP
