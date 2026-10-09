#ifndef WIDGET_SETTINGS_HPP
#define WIDGET_SETTINGS_HPP

#include "interface_style.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot
{
/// Réglages d'un widget (remplace le Dictionary ExtraData du C#) : propriétés exportées, donc validées
/// par l'inspecteur. Utilisable telle quelle pour les réglages communs ; chaque widget qui a besoin de
/// plus déclare sa sous-classe (PropertyWidget::get_settings_class).
/// Priorité : réglages de l'entrée > indice (hint) de la propriété > valeurs par défaut du widget.
class WidgetSettings : public Resource
{
    GDCLASS(WidgetSettings, Resource)

  public:
    /// Partie du widget qui reçoit le style. Valeurs enregistrées : jamais renumérotées.
    enum StyleTarget
    {
        STYLE_WIDGET = 0, // widget entier (libellé + valeur), fond dessiné derrière
        STYLE_VALUE = 1,  // contrôle de la valeur seulement
        STYLE_LABEL = 2,  // libellé seulement
    };
    /// Place de l'icône. Valeurs enregistrées : jamais renumérotées.
    enum IconPosition
    {
        ICON_BEFORE_LABEL = 0,
        ICON_BEFORE_VALUE = 1,
    };

  private:
    bool show_label = true;
    double label_ratio = 0.4;
    Ref<InterfaceStyle> style;
    StyleTarget style_target = STYLE_WIDGET;
    Ref<Texture2D> icon;
    IconPosition icon_position = ICON_BEFORE_LABEL;
    int icon_size = 16; // côté en pixels

  protected:
    static void _bind_methods();

  public:
    void set_style(const Ref<InterfaceStyle> &p_style);
    Ref<InterfaceStyle> get_style() const;
    void set_style_target(StyleTarget p_target);
    StyleTarget get_style_target() const;
    void set_icon(const Ref<Texture2D> &p_icon);
    Ref<Texture2D> get_icon() const;
    void set_icon_position(IconPosition p_position);
    IconPosition get_icon_position() const;
    void set_icon_size(int p_size);
    int get_icon_size() const;
    void set_show_label(bool p_show);
    bool get_show_label() const;
    void set_label_ratio(double p_ratio);
    double get_label_ratio() const;
};

/// Nombres (SpinBox, sliders, barres de progression). Présents, ces réglages REMPLACENT l'indice Range de la propriété.
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

/// TextureProgressBar : bornes de RangeWidgetSettings + textures (sans texture de progression : ProgressBar simple).
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

/// TextEdit : hauteur de la zone de texte.
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

/// Structures (un SpinBox par composante) : pas des composantes décimales.
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

/// Conteneurs (objets, collections) : disposition et dépliage.
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

/// Tableau (liste d'objets ou de dictionnaires) : une ligne par élément et une colonne par propriété / clé,
/// ou l'inverse (transpose). columns : propriétés affichées, dans cet ordre (vide : toutes, dans l'ordre de
/// découverte).
class TableWidgetSettings : public ContainerWidgetSettings
{
    GDCLASS(TableWidgetSettings, ContainerWidgetSettings)

  private:
    bool transpose = false;
    PackedStringArray table_columns;

  protected:
    static void _bind_methods();

  public:
    void set_transpose(bool p_transpose);
    bool get_transpose() const;
    void set_table_columns(const PackedStringArray &p_columns);
    PackedStringArray get_table_columns() const;
};

/// Aperçu personnalisé : classe (native ou class_name héritant de PreviewWidget), source (l'objet affiché si
/// source = Propriété sans chemin ; sinon une propriété, une fonction ou une valeur manuelle), hauteur,
/// mise à jour. Un aperçu de script peut déclarer ses propres réglages en héritant de PreviewSettings.
class PreviewSettings : public WidgetSettings
{
    GDCLASS(PreviewSettings, WidgetSettings)

  public:
    enum UpdateMode
    {
        UPDATE_ON_CHANGE = 0,   // reconstruit quand la source change
        UPDATE_EVERY_FRAME = 1, // _update_preview à chaque image (animation, « en action »)
    };

  private:
    StringName preview_class = "TexturePreview";
    Ref<ArgumentBinding> source;
    int height = 160;
    UpdateMode update_mode = UPDATE_ON_CHANGE;
    bool duplicate_scripts = false;

  protected:
    static void _bind_methods();

  public:
    void set_preview_class(const StringName &p_class);
    StringName get_preview_class() const;
    void set_source(const Ref<ArgumentBinding> &p_source);
    Ref<ArgumentBinding> get_source(); // créée à la première lecture (objet affiché)
    Ref<ArgumentBinding> get_stored_source() const;
    void set_height(int p_height);
    int get_height() const;
    void set_update_mode(UpdateMode p_mode);
    UpdateMode get_update_mode() const;
    void set_duplicate_scripts(bool p_duplicate);
    bool get_duplicate_scripts() const;
};
} // namespace godot

VARIANT_ENUM_CAST(WidgetSettings::StyleTarget);
VARIANT_ENUM_CAST(WidgetSettings::IconPosition);
VARIANT_ENUM_CAST(PreviewSettings::UpdateMode);

#endif // WIDGET_SETTINGS_HPP
