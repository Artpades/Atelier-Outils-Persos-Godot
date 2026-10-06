#ifndef PLAYER_FX_HPP
#define PLAYER_FX_HPP

#include "effect.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot
{
/// ✨ Joue une liste d'effets (special_effects) sur un nœud cible et les met à jour à chaque frame.
/// La cible par défaut est node_to_play_effects, ou à défaut le parent du PlayerFX.
/// Les boutons « Jouer effet » / « Arrêter effet » de l'inspecteur prévisualisent les effets dans l'éditeur.
class PlayerFX : public Node
{
    GDCLASS(PlayerFX, Node)

  public:
    /// Joue les effets sur p_node (ou sur la cible par défaut si p_node est nul).
    void play(Node *p_node = nullptr);
    /// Arrête les effets sur p_node (ou sur la cible du dernier play()).
    void stop(Node *p_node = nullptr);

    void set_node_to_play_effects(Node *p_node);
    Node *get_node_to_play_effects() const;
    void set_special_effects(const TypedArray<Effect> &p_effects);
    TypedArray<Effect> get_special_effects() const { return special_effects; }
    /// node_to_play_effects, ou le parent du PlayerFX.
    Node *get_default_target() const;

    void _process(double p_delta) override;
    void _physics_process(double p_delta) override;

  protected:
    static void _bind_methods();

  private:
    TypedArray<Effect> special_effects;
    ObjectID node_to_play_effects_id;
    ObjectID current_target_id; // cible du dernier play(), mise à jour ensuite à chaque frame

    Node *_get_current_target() const;
    void _link_effects();

    // Boutons de l'inspecteur
    Callable _get_play_button() { return callable_mp(this, &PlayerFX::_on_play_button); }
    Callable _get_stop_button() { return callable_mp(this, &PlayerFX::_on_stop_button); }
    void _on_play_button() { play(); }
    void _on_stop_button() { stop(); }
};
} // namespace godot

#endif // PLAYER_FX_HPP
