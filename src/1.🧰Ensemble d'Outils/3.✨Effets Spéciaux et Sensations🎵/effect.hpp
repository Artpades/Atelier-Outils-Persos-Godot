#ifndef EFFECT_HPP
#define EFFECT_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// ✨ Effet programmé, joué sur un nœud cible par un PlayerFX (ou directement par play_fx / stop_fx).
/// Un effet peut enchaîner d'autres effets : effects_on_play_fx sont joués / mis à jour / arrêtés avec lui,
/// effects_on_stop_fx sont joués à son arrêt.
///
/// Les classes C++ dérivées surchargent _play / _process / _physics_process / _stop ; un script dérivé
/// surcharge les méthodes virtuelles _play_fx / _process_fx / _physics_process_fx / _stop_fx.
/// L'état d'un effet (position initiale, minuteur...) est stocké dans la ressource : elle est locale à la
/// scène par défaut, pour que chaque instance de scène ait la sienne.
class Effect : public Resource
{
    GDCLASS(Effect, Resource)

  public:
    Effect();

    void play_fx(Node *p_target = nullptr);
    void process_fx(double p_delta, Node *p_target = nullptr);
    void physics_process_fx(double p_delta, Node *p_target = nullptr);
    /// Arrête l'effet (sans effet s'il n'est pas actif : rien à restaurer).
    void stop_fx(Node *p_target = nullptr);
    bool is_activated() const { return activated; }

    void set_effects_on_play_fx(const TypedArray<Effect> &p_effects);
    TypedArray<Effect> get_effects_on_play_fx() const { return effects_on_play_fx; }
    void set_effects_on_stop_fx(const TypedArray<Effect> &p_effects);
    TypedArray<Effect> get_effects_on_stop_fx() const { return effects_on_stop_fx; }

    /// PlayerFX qui joue l'effet (transmis aux effets enchaînés) : les NodePath des effets lui sont relatifs.
    void set_player_fx(Node *p_player);
    Node *get_player_fx() const;

  protected:
    static void _bind_methods();

    virtual void _play(Node *p_target);
    virtual void _process(double p_delta, Node *p_target);
    virtual void _physics_process(double p_delta, Node *p_target);
    virtual void _stop(Node *p_target);

    /// Vrai pendant _play si l'effet était déjà actif (relancé) : l'état d'origine capturé au premier jeu
    /// (position, échelle, couleur...) doit alors être conservé, la cible étant déjà modifiée.
    bool _is_restarting() const { return restarting; }

    /// Nœud au chemin p_path, relatif au PlayerFX (nullptr sans PlayerFX ou si introuvable).
    Node *_get_node_from_player(const NodePath &p_path) const;

    GDVIRTUAL1(_play_fx, Node *)
    GDVIRTUAL2(_process_fx, double, Node *)
    GDVIRTUAL2(_physics_process_fx, double, Node *)
    GDVIRTUAL1(_stop_fx, Node *)

  private:
    // Opérations en cours : protège d'un effet qui s'enchaîne lui-même (récursion infinie).
    enum Operation
    {
        OPERATION_PLAY = 1 << 0,
        OPERATION_PROCESS = 1 << 1,
        OPERATION_PHYSICS_PROCESS = 1 << 2,
        OPERATION_STOP = 1 << 3,
        OPERATION_LINK = 1 << 4,
    };

    TypedArray<Effect> effects_on_play_fx;
    TypedArray<Effect> effects_on_stop_fx;
    ObjectID player_fx_id;
    bool activated = false;
    bool restarting = false;
    uint32_t running_operations = 0;
};
} // namespace godot

#endif // EFFECT_HPP
