#ifndef WIDGET_TABLE_HPP
#define WIDGET_TABLE_HPP

#include "data_entry.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot
{
/// Table des widgets autorisés par type de donnée et classification des membres (port de WidgetTable C#).
/// C++ uniquement (non exposée à GDScript). Les tables sont des tableaux C constants : aucune
/// destruction statique au déchargement de la bibliothèque (contrairement à un Vector/HashMap statique).
class WidgetTable
{
  public:
    WidgetTable() = delete; // Classe purement statique

    /// Vue sur une liste de choix constante.
    struct Choices
    {
        const DataEntry::WidgetChoice *data = nullptr;
        int size = 0;
    };

    /// Widgets autorisés pour un type de donnée (hors AUTO). Le premier est le défaut.
    static Choices allowed_for(DataEntry::DataKind p_kind);
    /// Widget utilisé quand le choix est AUTO.
    static DataEntry::WidgetChoice default_for(DataEntry::DataKind p_kind);
    /// Vrai si p_choice est valide pour p_kind (AUTO l'est toujours).
    static bool is_allowed(DataEntry::DataKind p_kind, DataEntry::WidgetChoice p_choice);

    /// Classe un membre à partir de sa description d'introspection (format get_property_list / get_method_list).
    static DataEntry::DataKind kind_of(const Dictionary &p_info, DataEntry::MemberKind p_member);
    /// Vrai pour les types dont le contenu s'organise dans un conteneur (orientation = DataEntry::is_vertical).
    static bool is_container_kind(DataEntry::DataKind p_kind);
    /// Vector2/2i/3/3i/4/4i, Rect2/2i, AABB, Plane, Quaternion, Basis, Transform2D/3D, Projection.
    static bool is_struct_type(Variant::Type p_type);
    /// Packed*Array.
    static bool is_packed_type(Variant::Type p_type);

    /// Nom lisible d'un choix (pour les messages).
    static String choice_name(DataEntry::WidgetChoice p_choice);
    /// Noms lisibles d'un type de donnée et d'une catégorie de membre.
    static String kind_name(DataEntry::DataKind p_kind);
    static String member_name(DataEntry::MemberKind p_member);
    /// Chaînes d'indice PROPERTY_HINT_ENUM avec valeurs explicites ("Nom:valeur,...").
    static String choice_hint_string();
    static String kind_hint_string();
    /// Choix proposés pour un type : AUTO (avec le nom du défaut) puis les choix autorisés.
    static String choice_hint_string_for(DataEntry::DataKind p_kind);

    /// Classe de réglages lue par le widget d'un choix (WidgetSettings si seuls les réglages communs servent).
    static StringName settings_class_for(DataEntry::WidgetChoice p_choice);
};
} // namespace godot

#endif // WIDGET_TABLE_HPP
