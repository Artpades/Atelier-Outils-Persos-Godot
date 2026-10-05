#ifndef WIDGET_SETTINGS_HPP
#define WIDGET_SETTINGS_HPP

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot
{
// Réglages d'un widget (remplace le Dictionary ExtraData du C#) : propriétés exportées, donc validées
// par l'inspecteur. Utilisable telle quelle pour les réglages communs ; chaque widget qui a besoin de
// plus déclare sa sous-classe (PropertyWidget::get_settings_class).
// Priorité : réglages de l'entrée > indice (hint) de la propriété > valeurs par défaut du widget.
class WidgetSettings : public Resource
{
    GDCLASS(WidgetSettings, Resource)

  private:
    bool show_label = true;
    double label_ratio = 0.4;

  protected:
    static void _bind_methods();

  public:
    void set_show_label(bool p_show);
    bool get_show_label() const;
    void set_label_ratio(double p_ratio);
    double get_label_ratio() const;
};

// Nombres (SpinBox, sliders, barres de progression). Présents, ces réglages REMPLACENT l'indice Range de la propriété.
class RangeWidgetSettings : public WidgetSettings
{
    GDCLASS(RangeWidgetSettings, WidgetSettings)

  private:
    double min_value = 0.0;
    double max_value = 100.0;
    double step = 0.0; // 0 = automatique (1 pour un entier, 0.001 pour un décimal)
    bool allow_greater = false;
    bool allow_lesser = false;
    String suffix;

  protected:
    static void _bind_methods();

  public:
    void set_min_value(double p_value);
    double get_min_value() const;
    void set_max_value(double p_value);
    double get_max_value() const;
    void set_step(double p_step);
    double get_step() const;
    void set_allow_greater(bool p_allow);
    bool get_allow_greater() const;
    void set_allow_lesser(bool p_allow);
    bool get_allow_lesser() const;
    void set_suffix(const String &p_suffix);
    String get_suffix() const;
};

// TextureProgressBar : bornes de RangeWidgetSettings + textures (sans texture de progression : ProgressBar simple).
class TextureProgressWidgetSettings : public RangeWidgetSettings
{
    GDCLASS(TextureProgressWidgetSettings, RangeWidgetSettings)

  private:
    Ref<Texture2D> progress_texture;
    Ref<Texture2D> under_texture;

  protected:
    static void _bind_methods();

  public:
    void set_progress_texture(const Ref<Texture2D> &p_texture);
    Ref<Texture2D> get_progress_texture() const;
    void set_under_texture(const Ref<Texture2D> &p_texture);
    Ref<Texture2D> get_under_texture() const;
};

// TextEdit : hauteur de la zone de texte.
class TextEditWidgetSettings : public WidgetSettings
{
    GDCLASS(TextEditWidgetSettings, WidgetSettings)

  private:
    double height = 80.0;

  protected:
    static void _bind_methods();

  public:
    void set_height(double p_height);
    double get_height() const;
};

// Structures (un SpinBox par composante) : pas des composantes décimales.
class StructWidgetSettings : public WidgetSettings
{
    GDCLASS(StructWidgetSettings, WidgetSettings)

  private:
    double step = 0.001;

  protected:
    static void _bind_methods();

  public:
    void set_step(double p_step);
    double get_step() const;
};

// Conteneurs (objets, collections) : disposition et dépliage.
class ContainerWidgetSettings : public WidgetSettings
{
    GDCLASS(ContainerWidgetSettings, WidgetSettings)

  private:
    int columns = 2;          // GridContainer
    bool foldable = true;     // section repliable (FoldableContainer)
    bool start_expanded = false;
    int max_displayed = 100;  // éléments affichés au plus (grosses collections)

  protected:
    static void _bind_methods();

  public:
    void set_columns(int p_columns);
    int get_columns() const;
    void set_foldable(bool p_foldable);
    bool get_foldable() const;
    void set_start_expanded(bool p_expanded);
    bool get_start_expanded() const;
    void set_max_displayed(int p_max);
    int get_max_displayed() const;
};
} // namespace godot

#endif // WIDGET_SETTINGS_HPP
