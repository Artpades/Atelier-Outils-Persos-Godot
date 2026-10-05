#ifndef DATA_REGISTRY_HPP
#define DATA_REGISTRY_HPP

#include "data_entry.hpp"
#include "group_registry.hpp"
#include "widget_behavior.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
// Registre des données : la liste des membres d'un objet et la façon de les afficher (port de DataRegistry C#).
// sync(cible) fusionne la liste des membres de la cible avec les entrées :
//  - un membre nouveau reçoit une entrée (widget AUTO) ;
//  - un membre connu garde sa configuration ;
//  - un membre disparu est marqué is_missing (jamais supprimé automatiquement).
// La cible n'est pas sérialisée (ObjectID).
class DataRegistry : public Resource
{
    GDCLASS(DataRegistry, Resource)

  private:
    TypedArray<DataEntry> entries;
    DataEntry::InspectionScope scope = DataEntry::SCOPE_AUTO;
    bool scan_properties = true;
    bool scan_methods = false;
    bool scan_signals = false;
    bool include_private_methods = false; // méthodes / signaux commençant par « _ »
    TypedArray<WidgetBehavior> behaviors; // comportements conditionnels (Montrer si, Cacher si...)
    ObjectID target_id;

    // Fusionne une catégorie de membres. Remplit r_seen avec les entrées encore présentes. Les nouveaux
    // membres placés sous un @export_group / @export_subgroup sont ajoutés au groupe correspondant de p_groups.
    bool _sync_kind(Object *p_target, DataEntry::MemberKind p_member, const Ref<GroupRegistry> &p_groups, TypedArray<DataEntry> &r_seen);

  protected:
    static void _bind_methods();

  public:
    // Descriptions des membres de p_target selon le périmètre (format get_property_list / get_method_list).
    static TypedArray<Dictionary> get_members(Object *p_target, DataEntry::MemberKind p_member, DataEntry::InspectionScope p_scope);

    void set_entries(const TypedArray<DataEntry> &p_entries);
    TypedArray<DataEntry> get_entries() const;
    void set_scope(DataEntry::InspectionScope p_scope);
    DataEntry::InspectionScope get_scope() const;
    void set_scan_properties(bool p_scan);
    bool get_scan_properties() const;
    void set_scan_methods(bool p_scan);
    bool get_scan_methods() const;
    void set_scan_signals(bool p_scan);
    bool get_scan_signals() const;
    void set_include_private_methods(bool p_include);
    bool get_include_private_methods() const;
    void set_behaviors(const TypedArray<WidgetBehavior> &p_behaviors);
    TypedArray<WidgetBehavior> get_behaviors() const;

    // Cible de la dernière synchronisation (nullptr si aucune ou libérée).
    Object *get_target() const;

    // Retourne l'entrée d'un membre, ou une Ref nulle.
    Ref<DataEntry> find(const StringName &p_name, DataEntry::MemberKind p_member = DataEntry::MEMBER_PROPERTY) const;

    // Mémorise p_target et fusionne ses membres avec les entrées. Émet "synced".
    // p_groups (optionnel) reçoit les groupes automatiques (@export_group) des NOUVEAUX membres.
    void sync(Object *p_target, const Ref<GroupRegistry> &p_groups = Ref<GroupRegistry>());

    // Connecte / déconnecte les liens en mode ressource de tous les signaux (idempotent).
    void apply_links(Object *p_source);
    void clear_links(Object *p_source);
};
} // namespace godot

#endif // DATA_REGISTRY_HPP
