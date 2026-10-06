#ifndef CURSOR_CHANGER_HPP
#define CURSOR_CHANGER_HPP

#include "cursor_style.hpp"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// 👆 Remplace le curseur de la souris par un CursorStyle (image fixe ou animée), avec un style distinct
/// quand un bouton est enfoncé.
/// Activation : en permanence (tant que le nœud est dans l'arbre), au survol de la cible (target, ou à
/// défaut le parent : Control ou CollisionObject2D / 3D), ou manuelle (activate / deactivate).
/// Plusieurs CursorChanger peuvent être actifs : le dernier activé l'emporte, et à sa désactivation le
/// précédent reprend la main (ex. : curseur global du jeu + curseur de survol d'un bouton).
class CursorChanger : public Node
{
    GDCLASS(CursorChanger, Node)

  public:
    enum ActivationMode
    {
        ACTIVATION_ALWAYS,
        ACTIVATION_HOVER,
        ACTIVATION_MANUAL,
    };

    void activate();
    void deactivate();
    bool is_active() const;
    /// Vrai si ce CursorChanger est celui qui dessine le curseur actuellement.
    bool is_current() const;

    void set_style(const Ref<CursorStyle> &p_value);
    Ref<CursorStyle> get_style() const { return style; }
    void set_pressed_style(const Ref<CursorStyle> &p_value);
    Ref<CursorStyle> get_pressed_style() const { return pressed_style; }
    void set_pressed_button(MouseButton p_value) { pressed_button = p_value; }
    MouseButton get_pressed_button() const { return pressed_button; }
    void set_activation_mode(ActivationMode p_value);
    ActivationMode get_activation_mode() const { return activation_mode; }
    void set_target(Node *p_target);
    Node *get_target() const;

    void _ready() override;
    void _process(double p_delta) override;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    Ref<CursorStyle> style;
    Ref<CursorStyle> pressed_style;
    MouseButton pressed_button = MOUSE_BUTTON_LEFT;
    ActivationMode activation_mode = ACTIVATION_ALWAYS;
    ObjectID target_id;

    ObjectID connected_target_id;
    double style_time = 0.0; // temps écoulé dans le style affiché (animation)
    bool was_pressed = false;

    /// Active (mode permanent) ou se branche sur la cible (mode survol).
    void _start();
    void _connect_target();
    void _disconnect_target();
    /// Ce qui doit être affiché maintenant (le style « enfoncé » si le bouton est tenu).
    Ref<CursorStyle> _get_displayed_style() const;
    /// Applique le curseur de ce CursorChanger au système.
    void _apply(bool p_force);
};
} // namespace godot

VARIANT_ENUM_CAST(CursorChanger::ActivationMode);

#endif // CURSOR_CHANGER_HPP
