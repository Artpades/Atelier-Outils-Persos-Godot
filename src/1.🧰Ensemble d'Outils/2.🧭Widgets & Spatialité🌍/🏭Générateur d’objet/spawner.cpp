#include "spawner.hpp"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void Spawner::set_parent_node(Node *p_node) { parent_node_id = p_node ? ObjectID(p_node->get_instance_id()) : ObjectID(); }

Node *Spawner::get_parent_node() const { return Object::cast_to<Node>(ObjectDB::get_instance(parent_node_id)); }

Node *Spawner::_get_spawn_parent() const
{
    Node *parent = get_parent_node();
    return parent ? parent : get_viewport();
}

// 🎯 API publique

void Spawner::spawn()
{
    ERR_FAIL_COND_MSG(!is_inside_tree(), String(U"[Spawner] Le Spawner doit être dans l'arbre de scène."));
    if (entries.is_empty())
    {
        WARN_PRINT(String(U"[Spawner] Aucune entrée définie pour le spawn."));
        return;
    }

    // Entrées ajoutées en jeu : les index séquentiels suivent
    if (sequential_scene_indices.size() < entries.size())
    {
        sequential_scene_indices.resize(entries.size());
        sequential_position_indices.resize(entries.size());
    }

    for (int64_t i = 0; i < entries.size(); i++)
    {
        _spawn_entry(entries[i], i);
    }

    emit_signal("spawned");
}

// 🧩 Logique de génération

Node *Spawner::_acquire_instance(const Ref<PackedScene> &p_scene)
{
    Node *instance = p_scene->instantiate();
    if (instance)
    {
        _get_spawn_parent()->add_child(instance);
    }
    return instance;
}

void Spawner::_spawn_entry(const Ref<SpawnEntry> &p_entry, int64_t p_entry_index)
{
    if (p_entry.is_null())
    {
        WARN_PRINT(String(U"[Spawner] Entrée nulle détectée."));
        return;
    }

    const Array scenes = p_entry->get_scenes();
    if (scenes.is_empty())
    {
        WARN_PRINT(String(U"[Spawner] Aucune scène définie dans l'entrée."));
        return;
    }

    const Array positions = _get_positions_of(p_entry);
    if (positions.is_empty())
    {
        WARN_PRINT(String(U"[Spawner] Aucune position définie dans l'entrée et aucun nœud enfant (Node2D/Node3D) disponible."));
        return;
    }

    const Array scenes_to_spawn = _select(scenes, p_entry->get_scenes_selection_mode(), sequential_scene_indices.write[p_entry_index]);
    const Array positions_to_use = _select(positions, p_entry->get_positions_selection_mode(), sequential_position_indices.write[p_entry_index]);

    for (int64_t i = 0; i < scenes_to_spawn.size(); i++)
    {
        Ref<PackedScene> scene = scenes_to_spawn[i];
        if (scene.is_null())
        {
            WARN_PRINT(String(U"[Spawner] Scène nulle détectée."));
            continue;
        }

        // Modulo : les positions sont réutilisées si elles sont moins nombreuses que les scènes
        const NodePath position_path = positions_to_use[i % positions_to_use.size()];
        Node *position_node = get_node_or_null(position_path);
        if (!position_node)
        {
            WARN_PRINT(String(U"[Spawner] Position introuvable : ") + String(position_path));
            continue;
        }

        Node *instance = _acquire_instance(scene);
        if (!instance)
        {
            continue;
        }
        _apply_transform(instance, position_node);
        _on_instance_placed(instance);
    }
}

Array Spawner::_get_positions_of(const Ref<SpawnEntry> &p_entry) const
{
    const Array entry_positions = p_entry->get_positions();
    if (!entry_positions.is_empty())
    {
        return entry_positions;
    }

    // Repli : les enfants 2D / 3D du Spawner, dans un tableau neuf (une copie d'Array partage ses
    // données : y ajouter les chemins modifierait la ressource partagée).
    Array positions;
    for (int64_t i = 0; i < get_child_count(); i++)
    {
        Node *child = get_child(i);
        if (Object::cast_to<Node3D>(child) || Object::cast_to<Node2D>(child))
        {
            positions.push_back(get_path_to(child));
        }
    }
    return positions;
}

// 🎛️ Assistants de sélection

Array Spawner::_select(const Array &p_items, SpawnEntry::SelectionMode p_mode, int64_t &r_sequence_index)
{
    Array selected;
    const int64_t count = p_items.size();
    switch (p_mode)
    {
    case SpawnEntry::SELECTION_ALL:
        return p_items.duplicate();
    case SpawnEntry::SELECTION_RANDOM:
        selected.push_back(p_items[UtilityFunctions::randi_range(0, count - 1)]);
        break;
    case SpawnEntry::SELECTION_SEQUENTIAL:
    default:
        // Modulo aussi en lecture : la liste a pu raccourcir depuis le dernier spawn
        r_sequence_index %= count;
        selected.push_back(p_items[r_sequence_index]);
        r_sequence_index = (r_sequence_index + 1) % count;
        break;
    }
    return selected;
}

void Spawner::_apply_transform(Node *p_instance, Node *p_target) const
{
    Node *spawn_parent = _get_spawn_parent();

    Node3D *target_3d = Object::cast_to<Node3D>(p_target);
    Node3D *instance_3d = Object::cast_to<Node3D>(p_instance);
    if (target_3d && instance_3d)
    {
        const Transform3D global = target_3d->get_global_transform();
        if (instance_3d->is_inside_tree())
        {
            instance_3d->set_global_transform(global);
        }
        else
        {
            // Pas encore dans l'arbre (ajout différé) : transform locale exprimée dans le repère du parent
            Node3D *parent_3d = Object::cast_to<Node3D>(spawn_parent);
            instance_3d->set_transform(parent_3d ? parent_3d->get_global_transform().affine_inverse() * global : global);
        }
        return;
    }

    Node2D *target_2d = Object::cast_to<Node2D>(p_target);
    Node2D *instance_2d = Object::cast_to<Node2D>(p_instance);
    if (target_2d && instance_2d)
    {
        const Transform2D global = target_2d->get_global_transform();
        if (instance_2d->is_inside_tree())
        {
            instance_2d->set_global_transform(global);
        }
        else
        {
            Node2D *parent_2d = Object::cast_to<Node2D>(spawn_parent);
            instance_2d->set_transform(parent_2d ? parent_2d->get_global_transform().affine_inverse() * global : global);
        }
    }
}

// 🔗 Liaisons

void Spawner::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("spawn"), &Spawner::spawn);
    ClassDB::bind_method(D_METHOD("set_entries", "entries"), &Spawner::set_entries);
    ClassDB::bind_method(D_METHOD("get_entries"), &Spawner::get_entries);
    ClassDB::bind_method(D_METHOD("set_parent_node", "node"), &Spawner::set_parent_node);
    ClassDB::bind_method(D_METHOD("get_parent_node"), &Spawner::get_parent_node);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "entries", PROPERTY_HINT_ARRAY_TYPE, "SpawnEntry"), "set_entries", "get_entries");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "parent_node", PROPERTY_HINT_NODE_TYPE, "Node"), "set_parent_node", "get_parent_node");

    ADD_SIGNAL(MethodInfo("spawned"));
}
