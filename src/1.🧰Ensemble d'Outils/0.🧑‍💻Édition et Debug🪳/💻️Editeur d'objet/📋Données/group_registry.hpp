#ifndef GROUP_REGISTRY_HPP
#define GROUP_REGISTRY_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔧Réglages/interface_style.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
// Groupe de données défini par l'utilisateur (port de GroupEntry C#) : ses données sont affichées
// ensemble dans un conteneur, à la place de la PREMIÈRE d'entre elles, dans l'ordre de data_names.
class GroupEntry : public Resource
{
    GDCLASS(GroupEntry, Resource)

  public:
    // Valeurs enregistrées dans les .tres : jamais renumérotées.
    enum ContainerType
    {
        CONTAINER_VBOX = 0,
        CONTAINER_HBOX = 1,
        CONTAINER_GRID = 2,  // colonnes : columns
        CONTAINER_PANEL = 3, // cadre autour d'une boîte orientée par is_vertical
        CONTAINER_TABS = 4,  // onglets : un onglet par groupe de child_groups (ne contient que des groupes)
    };

  private:
    String group_name;
    ContainerType container_type = CONTAINER_VBOX;
    int columns = 2;
    bool is_vertical = true;
    PackedStringArray data_names;
    Ref<InterfaceStyle> style; // style du groupe (titre + conteneur)
    Ref<Texture2D> icon;       // icône devant le titre (et de l'onglet)
    PackedStringArray child_groups; // CONTAINER_TABS : groupes affichés en onglets, dans cet ordre

  protected:
    static void _bind_methods();

  public:
    void set_group_name(const String &p_name);
    String get_group_name() const;
    // VBox / HBox imposent l'orientation (comme le C#).
    void set_container_type(ContainerType p_type);
    ContainerType get_container_type() const;
    void set_columns(int p_columns);
    int get_columns() const;
    void set_is_vertical(bool p_vertical);
    bool get_is_vertical() const;
    void set_data_names(const PackedStringArray &p_names);
    PackedStringArray get_data_names() const;
    void set_style(const Ref<InterfaceStyle> &p_style);
    Ref<InterfaceStyle> get_style() const;
    void set_icon(const Ref<Texture2D> &p_icon);
    Ref<Texture2D> get_icon() const;
    void set_child_groups(const PackedStringArray &p_groups);
    PackedStringArray get_child_groups() const;
};

// Registre des groupes (port de GroupRegistry C#). Une donnée appartient à un seul groupe.
class GroupRegistry : public Resource
{
    GDCLASS(GroupRegistry, Resource)

  private:
    TypedArray<GroupEntry> groups;

  protected:
    static void _bind_methods();

  public:
    void set_groups(const TypedArray<GroupEntry> &p_groups);
    TypedArray<GroupEntry> get_groups() const;

    Ref<GroupEntry> find(const String &p_group_name) const;
    // Retourne le groupe p_group_name, créé s'il n'existe pas.
    Ref<GroupEntry> ensure(const String &p_group_name);
    // Premier groupe contenant p_data_name (Ref nulle si aucun).
    Ref<GroupEntry> group_of(const String &p_data_name) const;
    // Groupe en onglets qui contient le groupe p_group_name (nul s'il n'est dans aucun).
    Ref<GroupEntry> tabs_of(const String &p_group_name) const;
    void remove_data_name(const String &p_data_name);
    // Retire p_data_name de tous les groupes puis l'ajoute à p_group.
    void assign(const String &p_data_name, const Ref<GroupEntry> &p_group);
};
} // namespace godot

VARIANT_ENUM_CAST(GroupEntry::ContainerType);

#endif // GROUP_REGISTRY_HPP
