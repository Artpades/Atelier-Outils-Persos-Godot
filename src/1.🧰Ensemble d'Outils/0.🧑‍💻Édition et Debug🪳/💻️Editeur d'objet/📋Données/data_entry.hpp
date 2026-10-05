#ifndef DATA_ENTRY_HPP
#define DATA_ENTRY_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔧Réglages/widget_settings.hpp"
#include "signal_link.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot
{
// Entrée du registre de données : configuration d'affichage d'UN membre (propriété, méthode ou signal).
// Port de DataEntry (C#), sans ExtraData : les réglages sont une Resource typée (WidgetSettings).
//
// Les valeurs des énumérations sont EXPLICITES et ne doivent jamais être renumérotées : elles sont
// enregistrées comme entiers dans les .tres. Toute nouvelle valeur s'ajoute à la fin.
class DataEntry : public Resource
{
    GDCLASS(DataEntry, Resource)

  public:
    // Catégorie de membre introspectable (get_property_list / get_method_list / get_signal_list).
    enum MemberKind
    {
        MEMBER_PROPERTY = 0,
        MEMBER_METHOD = 1,
        MEMBER_SIGNAL = 2,
        MEMBER_PREVIEW = 3, // aperçu personnalisé (PreviewWidget) : pas un membre de l'objet, jamais « manquant »
    };

    // Famille de donnée, déduite de l'introspection (WidgetTable::kind_of). Détermine les widgets autorisés.
    enum DataKind
    {
        KIND_UNSUPPORTED = 0,
        KIND_BOOL = 1,
        KIND_NUMBER = 2,
        KIND_TEXT = 3,
        KIND_ENUM = 4,
        KIND_FLAGS = 5,
        KIND_COLOR = 6,
        KIND_METHOD = 7,
        KIND_SIGNAL = 8,
        KIND_OBJECT = 9,
        KIND_NODE_PATH = 10,
        KIND_ARRAY = 11,
        KIND_DICTIONARY = 12,
        KIND_STRUCT = 13,
        KIND_VARIANT = 14,
        KIND_RID = 15,
        KIND_PACKED_ARRAY = 16,
        KIND_PREVIEW = 17,
    };

    // Widget choisi pour une donnée. Les choix valides dépendent du DataKind (WidgetTable::allowed_for).
    enum WidgetChoice
    {
        WIDGET_AUTO = 0,
        WIDGET_CHECK_BOX = 1,
        WIDGET_CHECK_BUTTON = 2,
        WIDGET_LABEL = 3,
        WIDGET_SPIN_BOX = 4,
        WIDGET_H_SLIDER = 5,
        WIDGET_V_SLIDER = 6,
        WIDGET_PROGRESS_BAR = 7,
        WIDGET_TEXTURE_PROGRESS_BAR = 8,
        WIDGET_LINE_EDIT = 9,
        WIDGET_TEXT_EDIT = 10,
        WIDGET_OPTION_BUTTON = 11,
        WIDGET_CHECK_BUTTON_LIST = 12,
        WIDGET_COLOR_PICKER_BUTTON = 13,
        WIDGET_COLOR_RECT_CHANNELS = 14,
        WIDGET_BUTTON = 15,
        WIDGET_BOX_CONTAINER = 16,
        WIDGET_GRID_CONTAINER = 17,
        WIDGET_PATH_BUTTON = 18,
        WIDGET_STRUCT_SPIN_BOXES = 19,
        WIDGET_CUSTOM = 20,
        WIDGET_VARIANT_SELECTOR = 21,
        WIDGET_CHECK_BOX_LIST = 22,
        WIDGET_FLOW_CONTAINER = 23,
        WIDGET_TAB_CONTAINER = 24,
        WIDGET_TABLE = 25, // liste d'objets / de dictionnaires en tableau (une ligne par élément, ou transposé)
        WIDGET_PREVIEW = 26, // aperçu (classe dans PreviewSettings::preview_class)
    };

    // Périmètre des membres scannés par DataRegistry::sync.
    enum InspectionScope
    {
        // Membres du script s'il y en a un, sinon ceux de la classe de l'objet (SCOPE_CLASS_ONLY).
        SCOPE_AUTO = 0,
        // Uniquement les membres déclarés par le script attaché.
        SCOPE_SCRIPT_ONLY = 1,
        // Tous les membres, y compris ceux du moteur (très long pour un Node).
        SCOPE_ALL = 2,
        // Uniquement les membres déclarés par la classe de l'objet (sans ses classes parentes).
        SCOPE_CLASS_ONLY = 3,
    };

  private:
    StringName data_name;
    MemberKind member = MEMBER_PROPERTY;
    DataKind kind = KIND_UNSUPPORTED;
    WidgetChoice widget_choice = WIDGET_AUTO;
    // Classe (native ou class_name) héritant de PropertyWidget, utilisée quand widget_choice vaut WIDGET_CUSTOM.
    StringName custom_widget_class;
    Ref<WidgetSettings> settings;
    String tooltip; // infobulle (BBCode, modèle dynamique {propriété} / {fonction()})
    bool is_vertical = false;
    bool enabled = true;
    bool is_missing = false;
    // Signal : liens en mode ressource (appliqués par DataRegistry::apply_links). Les liens natifs vivent dans la scène.
    TypedArray<SignalLink> links;

  protected:
    static void _bind_methods();
    // Restreint la liste de widget_choice aux choix autorisés pour le type (inspecteur Godot et ProceduralEditor).
    void _validate_property(PropertyInfo &p_property) const;

  public:
    void set_data_name(const StringName &p_name);
    StringName get_data_name() const;
    void set_member(MemberKind p_member);
    MemberKind get_member() const;
    void set_kind(DataKind p_kind);
    DataKind get_kind() const;
    void set_widget_choice(WidgetChoice p_choice);
    WidgetChoice get_widget_choice() const;
    void set_custom_widget_class(const StringName &p_class);
    StringName get_custom_widget_class() const;
    void set_settings(const Ref<WidgetSettings> &p_settings);
    Ref<WidgetSettings> get_settings() const;
    void set_tooltip(const String &p_tooltip);
    String get_tooltip() const;
    void set_is_vertical(bool p_vertical);
    bool get_is_vertical() const;
    void set_enabled(bool p_enabled);
    bool get_enabled() const;
    void set_is_missing(bool p_missing);
    bool get_is_missing() const;
    void set_links(const TypedArray<SignalLink> &p_links);
    TypedArray<SignalLink> get_links() const;

    // Widget effectivement utilisé : résout AUTO et remplace un choix devenu invalide pour le DataKind.
    WidgetChoice resolved_choice() const;
};
} // namespace godot

VARIANT_ENUM_CAST(DataEntry::MemberKind);
VARIANT_ENUM_CAST(DataEntry::DataKind);
VARIANT_ENUM_CAST(DataEntry::WidgetChoice);
VARIANT_ENUM_CAST(DataEntry::InspectionScope);

#endif // DATA_ENTRY_HPP
