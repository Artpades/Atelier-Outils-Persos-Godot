#ifndef WIDGET_BEHAVIOR_HPP
#define WIDGET_BEHAVIOR_HPP

#include "signal_link.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔧Réglages/interface_style.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot
{
// État calculé d'un élément ciblé (donnée ou groupe) à partir de tous les comportements qui le visent.
struct BehaviorState
{
    bool visible = true;
    bool editable = true;
    Vector<Ref<InterfaceStyle>> styles; // styles conditionnels actifs, dans l'ordre des comportements
    PackedStringArray tooltips;         // infobulles conditionnelles actives (BBCode, modèles {…})
    Ref<Texture2D> icon;                // icône de remplacement (la dernière active l'emporte)
};

// Comportement conditionnel d'éléments de l'interface (registre des données, liste « behaviors ») :
// condition = propriété booléenne ou fonction qui renvoie un booléen (ArgumentBinding, source Propriété ou
// Fonction), éventuellement inversée ; cibles = noms de données ou de groupes.
// Les comportements fournis contribuent à un BehaviorState combiné (visible = aucun « Cacher si » actif et
// aucun « Montrer si » inactif...). Un comportement de script surcharge _apply(cible, actif), appelé quand
// l'état de sa condition change (et à chaque reconstruction de l'interface).
class WidgetBehavior : public Resource
{
    GDCLASS(WidgetBehavior, Resource)

  private:
    bool enabled = true;
    Ref<ArgumentBinding> condition;
    bool invert = false;
    PackedStringArray targets;

  protected:
    static void _bind_methods();

    GDVIRTUAL2(_apply, Control *, bool)

  public:
    // Remplace {chemin} par la valeur get_indexed(chemin) et {fonction()} par le retour de la fonction
    // (sans argument) de p_context. {{ et }} donnent des accolades.
    static String format_text(const String &p_template, Object *p_context);

    void set_enabled(bool p_enabled);
    bool get_enabled() const;
    void set_condition(const Ref<ArgumentBinding> &p_condition);
    Ref<ArgumentBinding> get_condition(); // créée à la première lecture (source Propriété, type booléen)
    Ref<ArgumentBinding> get_stored_condition() const; // telle qu'enregistrée (peut être nulle)
    void set_invert(bool p_invert);
    bool get_invert() const;
    void set_targets(const PackedStringArray &p_targets);
    PackedStringArray get_targets() const;

    // Condition évaluée sur p_context (cible de l'éditeur), inversion comprise.
    bool evaluate(Object *p_context);
    // Contribution d'un comportement fourni (ne fait rien pour un comportement de script).
    virtual void contribute(BehaviorState &r_state, bool p_active) const;
    // Comportement de script : _apply surchargé.
    bool has_script_apply() const;
    void apply_script(Control *p_target, bool p_active);
    // Nom affiché dans l'éditeur (« Montrer si »...).
    virtual String get_display_name() const;
};

class ShowIfBehavior : public WidgetBehavior
{
    GDCLASS(ShowIfBehavior, WidgetBehavior)

  protected:
    static void _bind_methods() {}

  public:
    void contribute(BehaviorState &r_state, bool p_active) const override;
    String get_display_name() const override;
};

class HideIfBehavior : public WidgetBehavior
{
    GDCLASS(HideIfBehavior, WidgetBehavior)

  protected:
    static void _bind_methods() {}

  public:
    void contribute(BehaviorState &r_state, bool p_active) const override;
    String get_display_name() const override;
};

// Activer si : condition fausse = widgets en lecture seule.
class EnableIfBehavior : public WidgetBehavior
{
    GDCLASS(EnableIfBehavior, WidgetBehavior)

  protected:
    static void _bind_methods() {}

  public:
    void contribute(BehaviorState &r_state, bool p_active) const override;
    String get_display_name() const override;
};

// Style si : thème / shader / teintes appliqués tant que la condition est vraie.
class StyleIfBehavior : public WidgetBehavior
{
    GDCLASS(StyleIfBehavior, WidgetBehavior)

  private:
    Ref<InterfaceStyle> style;

  protected:
    static void _bind_methods();

  public:
    void set_style(const Ref<InterfaceStyle> &p_style);
    Ref<InterfaceStyle> get_style() const;
    void contribute(BehaviorState &r_state, bool p_active) const override;
    String get_display_name() const override;
};

// Infobulle si : infobulle (BBCode, modèle dynamique) ajoutée tant que la condition est vraie. Plusieurs
// comportements peuvent viser le même widget : leurs infobulles actives s'ajoutent à celle de la donnée.
class TooltipIfBehavior : public WidgetBehavior
{
    GDCLASS(TooltipIfBehavior, WidgetBehavior)

  private:
    String tooltip;

  protected:
    static void _bind_methods();

  public:
    void set_tooltip(const String &p_tooltip);
    String get_tooltip() const;
    void contribute(BehaviorState &r_state, bool p_active) const override;
    String get_display_name() const override;
};

// Icône si : icône du widget remplacée tant que la condition est vraie.
class IconIfBehavior : public WidgetBehavior
{
    GDCLASS(IconIfBehavior, WidgetBehavior)

  private:
    Ref<Texture2D> icon;

  protected:
    static void _bind_methods();

  public:
    void set_icon(const Ref<Texture2D> &p_icon);
    Ref<Texture2D> get_icon() const;
    void contribute(BehaviorState &r_state, bool p_active) const override;
    String get_display_name() const override;
};
} // namespace godot

#endif // WIDGET_BEHAVIOR_HPP
