#ifndef GAUGE_HPP
#define GAUGE_HPP

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

namespace godot
{
/// 📊 Jauge de valeur contrôlée : une valeur courante bornée entre un minimum et un maximum.
/// Barres de vie, de mana, d'endurance, compteurs de ressources... Émet un signal à chaque changement
/// effectif de valeur et lorsqu'un seuil (en pourcentage de la plage min-max) est franchi.
class Gauge : public Resource
{
    GDCLASS(Gauge, Resource)
  private:
    /// Valeur minimale autorisée.
    float min_value = 0.0f;
    /// Valeur maximale autorisée.
    float max_value = 100.0f;
    /// Valeur courante, toujours comprise entre min_value et max_value.
    float current_value = 100.0f;
    /// Valeur avant le dernier changement (détection des franchissements de seuils).
    float previous_value = 100.0f;
    /// Seuils surveillés, en pourcentage de la plage min-max (0 = min, 100 = max).
    PackedFloat32Array thresholds;

    /// Valeur absolue d'un seuil exprimé en pourcentage.
    float _threshold_to_value(float p_percent) const;
    /// Émet les signaux de seuils franchis entre previous_value et current_value, dans l'ordre du franchissement.
    void _check_thresholds();

  protected:
    static void _bind_methods();
    /// Borne le curseur de current_value dans l'inspecteur entre min_value et max_value.
    void _validate_property(PropertyInfo &p_property) const;

  public:
#pragma region Getter / Setter
    /// Borne la valeur, puis émet current_value_changed, les seuils franchis, filled / depleted si elle a changé.
    void set_current_value(float p_value);
    float get_current_value() const { return current_value; }

    /// Repousse max_value si nécessaire, puis reborne la valeur courante.
    void set_min_value(float p_value);
    float get_min_value() const { return min_value; }

    /// Repousse min_value si nécessaire, puis reborne la valeur courante.
    void set_max_value(float p_value);
    float get_max_value() const { return max_value; }

    void set_thresholds(const PackedFloat32Array &p_thresholds) { thresholds = p_thresholds; }
    PackedFloat32Array get_thresholds() const { return thresholds; }

    float get_previous_value() const { return previous_value; }
#pragma endregion

#pragma region Utilitaires
    /// Remplissage entre 0 (min) et 1 (max). 0 si la plage est nulle.
    float get_ratio() const;
    void set_ratio(float p_ratio);
    /// Remplissage entre 0 et 100.
    float get_percent() const { return get_ratio() * 100.0f; }

    /// Ajoute p_amount (négatif pour retirer) et renvoie la variation réellement appliquée après bornage.
    float add(float p_amount);
    void fill() { set_current_value(max_value); }
    void deplete() { set_current_value(min_value); }
    bool is_full() const { return current_value >= max_value; }
    bool is_empty() const { return current_value <= min_value; }
#pragma endregion
};
} // namespace godot

#endif // GAUGE_HPP
