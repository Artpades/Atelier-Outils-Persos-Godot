#include "data_registry.hpp"
#include "widget_table.hpp"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/property_info.hpp>

using namespace godot;

void DataRegistry::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_entries", "entries"), &DataRegistry::set_entries);
    ClassDB::bind_method(D_METHOD("get_entries"), &DataRegistry::get_entries);
    ClassDB::bind_method(D_METHOD("set_scope", "scope"), &DataRegistry::set_scope);
    ClassDB::bind_method(D_METHOD("get_scope"), &DataRegistry::get_scope);
    ClassDB::bind_method(D_METHOD("set_scan_properties", "scan"), &DataRegistry::set_scan_properties);
    ClassDB::bind_method(D_METHOD("get_scan_properties"), &DataRegistry::get_scan_properties);
    ClassDB::bind_method(D_METHOD("set_scan_methods", "scan"), &DataRegistry::set_scan_methods);
    ClassDB::bind_method(D_METHOD("get_scan_methods"), &DataRegistry::get_scan_methods);
    ClassDB::bind_method(D_METHOD("set_scan_signals", "scan"), &DataRegistry::set_scan_signals);
    ClassDB::bind_method(D_METHOD("get_scan_signals"), &DataRegistry::get_scan_signals);
    ClassDB::bind_method(D_METHOD("set_include_private_methods", "include"), &DataRegistry::set_include_private_methods);
    ClassDB::bind_method(D_METHOD("get_include_private_methods"), &DataRegistry::get_include_private_methods);
    ClassDB::bind_method(D_METHOD("get_target"), &DataRegistry::get_target);
    ClassDB::bind_method(D_METHOD("find", "name", "member"), &DataRegistry::find, DEFVAL(DataEntry::MEMBER_PROPERTY));
    ClassDB::bind_method(D_METHOD("sync", "target", "groups"), &DataRegistry::sync, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("apply_links", "source"), &DataRegistry::apply_links);
    ClassDB::bind_method(D_METHOD("clear_links", "source"), &DataRegistry::clear_links);
    ClassDB::bind_static_method("DataRegistry", D_METHOD("get_members", "target", "member", "scope"), &DataRegistry::get_members);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "entries", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":DataEntry"),
                 "set_entries", "get_entries");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "scope", PROPERTY_HINT_ENUM, "Auto:0,Script seulement:1,Tout:2,Classe seulement:3"), "set_scope", "get_scope");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scan_properties"), "set_scan_properties", "get_scan_properties");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scan_methods"), "set_scan_methods", "get_scan_methods");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scan_signals"), "set_scan_signals", "get_scan_signals");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "include_private_methods"), "set_include_private_methods", "get_include_private_methods");

    ADD_SIGNAL(MethodInfo("synced"));
}

void DataRegistry::set_entries(const TypedArray<DataEntry> &p_entries) { entries = p_entries; }
TypedArray<DataEntry> DataRegistry::get_entries() const { return entries; }
void DataRegistry::set_scope(DataEntry::InspectionScope p_scope) { scope = p_scope; }
DataEntry::InspectionScope DataRegistry::get_scope() const { return scope; }
void DataRegistry::set_scan_properties(bool p_scan) { scan_properties = p_scan; }
bool DataRegistry::get_scan_properties() const { return scan_properties; }
void DataRegistry::set_scan_methods(bool p_scan) { scan_methods = p_scan; }
bool DataRegistry::get_scan_methods() const { return scan_methods; }
void DataRegistry::set_scan_signals(bool p_scan) { scan_signals = p_scan; }
bool DataRegistry::get_scan_signals() const { return scan_signals; }
void DataRegistry::set_include_private_methods(bool p_include) { include_private_methods = p_include; }
bool DataRegistry::get_include_private_methods() const { return include_private_methods; }

Object *DataRegistry::get_target() const { return ObjectDB::get_instance(target_id); }

Ref<DataEntry> DataRegistry::find(const StringName &p_name, DataEntry::MemberKind p_member) const
{
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<DataEntry> entry = entries[i];
        if (entry.is_valid() && entry->get_data_name() == p_name && entry->get_member() == p_member)
        {
            return entry;
        }
    }
    return Ref<DataEntry>();
}

TypedArray<Dictionary> DataRegistry::get_members(Object *p_target, DataEntry::MemberKind p_member, DataEntry::InspectionScope p_scope)
{
    if (!p_target)
    {
        return TypedArray<Dictionary>();
    }

    // 1. Membres du script (SCRIPT_ONLY, ou AUTO quand un script est attaché).
    Ref<Script> script = p_target->get_script();
    bool script_first = p_scope == DataEntry::SCOPE_SCRIPT_ONLY || (p_scope == DataEntry::SCOPE_AUTO && script.is_valid());
    if (script_first && script.is_valid())
    {
        TypedArray<Dictionary> list = p_member == DataEntry::MEMBER_METHOD   ? script->get_script_method_list()
                                      : p_member == DataEntry::MEMBER_SIGNAL ? script->get_script_signal_list()
                                                                             : script->get_script_property_list();
        if (list.size() > 0 || p_scope == DataEntry::SCOPE_SCRIPT_ONLY)
        {
            return list;
        }
    }
    if (p_scope == DataEntry::SCOPE_SCRIPT_ONLY)
    {
        return TypedArray<Dictionary>(); // pas de script
    }

    // 2. Tous les membres.
    if (p_scope == DataEntry::SCOPE_ALL)
    {
        return p_member == DataEntry::MEMBER_METHOD ? p_target->get_method_list() : p_member == DataEntry::MEMBER_SIGNAL ? p_target->get_signal_list() : p_target->get_property_list();
    }

    // 3. Membres de la classe de l'objet seulement (CLASS_ONLY, ou AUTO sans script).
    StringName class_name = p_target->get_class();
    if (p_member != DataEntry::MEMBER_PROPERTY)
    {
        ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
        return p_member == DataEntry::MEMBER_METHOD ? cdb->class_get_method_list(class_name, true) : cdb->class_get_signal_list(class_name, true);
    }

    // Propriétés : on prend la section de get_property_list() placée sous la catégorie de la classe, afin
    // d'inclure aussi les propriétés dynamiques (_get_property_list), absentes de class_get_property_list.
    TypedArray<Dictionary> all = p_target->get_property_list();
    TypedArray<Dictionary> result;
    bool inside = false;
    for (int i = 0; i < all.size(); i++)
    {
        Dictionary info = all[i];
        int usage = info.get("usage", 0);
        if (usage & PROPERTY_USAGE_CATEGORY)
        {
            inside = StringName(info.get("name", "")) == class_name;
            continue;
        }
        if (inside)
        {
            result.push_back(info);
        }
    }
    return result;
}

bool DataRegistry::_sync_kind(Object *p_target, DataEntry::MemberKind p_member, const Ref<GroupRegistry> &p_groups, TypedArray<DataEntry> &r_seen)
{
    bool changed = false;
    TypedArray<Dictionary> members = get_members(p_target, p_member, scope);
    Dictionary names; // ensemble des noms déjà traités (les doublons sont ignorés)
    String current_group;
    String current_subgroup;

    for (int i = 0; i < members.size(); i++)
    {
        Dictionary info = members[i];
        String name = info.get("name", "");
        if (name.is_empty())
        {
            continue;
        }

        if (p_member == DataEntry::MEMBER_PROPERTY)
        {
            int usage = info.get("usage", 0);
            // Groupes de l'inspecteur : mémorisés pour les membres qui suivent.
            if (usage & PROPERTY_USAGE_CATEGORY)
            {
                current_group = String();
                current_subgroup = String();
                continue;
            }
            if (usage & PROPERTY_USAGE_GROUP)
            {
                current_group = name;
                current_subgroup = String();
                continue;
            }
            if (usage & PROPERTY_USAGE_SUBGROUP)
            {
                current_subgroup = name;
                continue;
            }
            if (!(usage & PROPERTY_USAGE_EDITOR) || name == "script")
            {
                continue;
            }
        }
        else if (name.begins_with("_") && !include_private_methods)
        {
            continue;
        }

        if (names.has(name))
        {
            continue;
        }
        names[name] = true;

        DataEntry::DataKind kind = WidgetTable::kind_of(info, p_member);
        Ref<DataEntry> entry = find(name, p_member);
        if (entry.is_null())
        {
            entry.instantiate();
            entry->set_data_name(name);
            entry->set_member(p_member);
            entry->set_kind(kind);
            entry->set_is_vertical(WidgetTable::is_container_kind(kind));
            entries.push_back(entry);
            changed = true;

            // Regroupement automatique : uniquement pour un NOUVEAU membre non encore placé dans un groupe
            // (les membres connus ne sont jamais déplacés).
            if (p_groups.is_valid() && !current_group.is_empty() && p_groups->group_of(name).is_null())
            {
                String group_name = current_subgroup.is_empty() ? current_group : current_group + " / " + current_subgroup;
                p_groups->assign(name, p_groups->ensure(group_name));
            }
        }
        else if (entry->get_kind() != kind)
        {
            entry->set_kind(kind);
            if (!WidgetTable::is_allowed(kind, entry->get_widget_choice()))
            {
                entry->set_widget_choice(DataEntry::WIDGET_AUTO);
            }
            changed = true;
        }
        r_seen.push_back(entry);
    }
    return changed;
}

void DataRegistry::sync(Object *p_target, const Ref<GroupRegistry> &p_groups)
{
    if (!p_target)
    {
        return;
    }
    target_id = p_target->get_instance_id();

    TypedArray<DataEntry> seen;
    bool changed = false;
    if (scan_properties)
    {
        changed |= _sync_kind(p_target, DataEntry::MEMBER_PROPERTY, p_groups, seen);
    }
    if (scan_methods)
    {
        changed |= _sync_kind(p_target, DataEntry::MEMBER_METHOD, p_groups, seen);
    }
    if (scan_signals)
    {
        changed |= _sync_kind(p_target, DataEntry::MEMBER_SIGNAL, p_groups, seen);
    }

    // Membres disparus : marqués, jamais supprimés (la configuration est conservée).
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<DataEntry> entry = entries[i];
        if (entry.is_null())
        {
            continue;
        }
        // Catégories non scannées : leurs entrées ne sont ni confirmées ni marquées manquantes.
        DataEntry::MemberKind member = entry->get_member();
        bool scanned = (member == DataEntry::MEMBER_PROPERTY && scan_properties) || (member == DataEntry::MEMBER_METHOD && scan_methods) ||
                       (member == DataEntry::MEMBER_SIGNAL && scan_signals);
        if (!scanned)
        {
            continue;
        }
        bool missing = !seen.has(entry);
        if (entry->get_is_missing() != missing)
        {
            entry->set_is_missing(missing);
            changed = true;
        }
    }

    if (changed)
    {
        emit_changed();
    }
    emit_signal("synced");
}

void DataRegistry::apply_links(Object *p_source)
{
    if (!p_source)
    {
        return;
    }
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<DataEntry> entry = entries[i];
        if (entry.is_null() || entry->get_member() != DataEntry::MEMBER_SIGNAL || entry->get_is_missing())
        {
            continue;
        }
        TypedArray<SignalLink> links = entry->get_links();
        for (int l = 0; l < links.size(); l++)
        {
            Ref<SignalLink> link = links[l];
            if (link.is_valid())
            {
                link->connect_to(p_source, entry->get_data_name()); // déconnecte d'abord : idempotent
            }
        }
    }
}

void DataRegistry::clear_links(Object *p_source)
{
    if (!p_source)
    {
        return;
    }
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<DataEntry> entry = entries[i];
        if (entry.is_null() || entry->get_member() != DataEntry::MEMBER_SIGNAL)
        {
            continue;
        }
        TypedArray<SignalLink> links = entry->get_links();
        for (int l = 0; l < links.size(); l++)
        {
            Ref<SignalLink> link = links[l];
            if (link.is_valid())
            {
                link->disconnect_from(p_source);
            }
        }
    }
}
