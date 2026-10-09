#ifndef TIME_MANIPULATOR_FX_HPP
#define TIME_MANIPULATOR_FX_HPP

#include <godot_cpp/classes/node.hpp>

namespace godot
{
/// ⏯️ Pilote la vitesse globale du jeu (Engine.time_scale) :
///  - set_time_scale() : ralenti / accéléré, rejoint progressivement (recovery_speed) ;
///  - trigger_hit_stop() : gel court et net, puis retour à la vitesse d'avant.
/// L'interpolation utilise le temps réel : elle progresse même quand le jeu est presque à l'arrêt.
/// À sa sortie de l'arbre, le nœud rend au jeu sa vitesse normale.
class TimeManipulatorFX : public Node
{
    GDCLASS(TimeManipulatorFX, Node)

  public:
    /// Vitesse visée (0.5 : ralenti de 50 %, 2 : deux fois plus vite). Les valeurs négatives sont ramenées à 0.
    void set_time_scale(double p_target_scale);
    /// Gel de p_duration secondes (temps réel) à la vitesse p_scale (0 : arrêt total). Ignoré si un gel est en cours.
    void trigger_hit_stop(double p_duration, double p_scale = 0.0);
    bool is_hit_stopping() const { return hit_stopping; }

    void set_recovery_speed(double p_value) { recovery_speed = p_value; }
    double get_recovery_speed() const { return recovery_speed; }
    void set_target_time_scale(double p_value) { set_time_scale(p_value); }
    double get_target_time_scale() const { return target_time_scale; }

    void _ready() override;
    void _process(double p_delta) override;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    double recovery_speed = 5.0; // vitesse de retour vers target_time_scale
    double target_time_scale = 1.0;
    bool hit_stopping = false;
    double scale_before_hit_stop = 1.0; // vitesse visée rétablie à la fin du gel
    uint64_t last_ticks_usec = 0;       // horloge réelle de la frame précédente (delta indépendant de time_scale)

    /// Fin du minuteur de gel : rétablit la vitesse d'avant le gel (sauf si le nœud a quitté l'arbre).
    void _on_hit_stop_finished();
};
} // namespace godot

#endif // TIME_MANIPULATOR_FX_HPP
