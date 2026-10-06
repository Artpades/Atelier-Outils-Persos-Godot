#include "pool_spawner.hpp"

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

// 🚀 Cycle de vie

void PoolSpawner::_ready()
{
    Node *parent = _get_spawn_parent();

    // Pré-allocation des pools (warm-up)
    for (int64_t i = 0; i < entries.size(); i++)
    {
        Ref<SpawnEntry> entry = entries[i];
        if (entry.is_null())
        {
            continue;
        }
        Ref<SpawnPoolEntry> pool_entry = entry;
        const int pool_size = pool_entry.is_valid() ? pool_entry->get_initial_pool_size_per_scene() : 0;

        const Array scenes = entry->get_scenes();
        for (int64_t s = 0; s < scenes.size(); s++)
        {
            Ref<PackedScene> scene = scenes[s];
            // Une scène présente dans plusieurs entrées n'est pré-allouée qu'une fois
            if (scene.is_null() || pools.has(scene->get_instance_id()))
            {
                continue;
            }

            Pool &pool = _get_or_create_pool(scene);
            for (int n = 0; n < pool_size; n++)
            {
                Node *instance = _create_instance(scene);
                if (!instance)
                {
                    break;
                }
                _deactivate(instance);
                pool.available.push_back(ObjectID(instance->get_instance_id()));
                // Ajout différé : le parent peut être en train d'initialiser ses enfants
                parent->call_deferred("add_child", instance);
            }
        }
    }
}

void PoolSpawner::_notification(int p_what)
{
    if (p_what != NOTIFICATION_PREDELETE)
    {
        return;
    }
    // Les instances inactives n'appartiennent qu'au pool : elles disparaissent avec lui.
    // (Les instances actives restent en jeu.)
    for (KeyValue<uint64_t, Pool> &pair : pools)
    {
        for (const ObjectID &id : pair.value.available)
        {
            if (Node *instance = Object::cast_to<Node>(ObjectDB::get_instance(id)))
            {
                instance->queue_free();
            }
        }
    }
}

// 🎯 API publique

void PoolSpawner::despawn(Node *p_instance)
{
    ERR_FAIL_NULL(p_instance);

    const uint64_t instance_id = p_instance->get_instance_id();
    HashMap<uint64_t, uint64_t>::Iterator scene_it = instance_to_scene.find(instance_id);
    Pool *pool = scene_it ? pools.getptr(scene_it->value) : nullptr;
    if (!pool)
    {
        // L'objet n'appartient pas au pool : destruction classique
        WARN_PRINT(String(U"[PoolSpawner] ") + String(p_instance->get_name()) + String(U" n'appartient pas à ce pool : destruction standard."));
        p_instance->queue_free();
        return;
    }

    if (pool->available.has(ObjectID(instance_id)))
    {
        WARN_PRINT(String(U"[PoolSpawner] ") + String(p_instance->get_name()) + String(U" est déjà dans le pool."));
        return;
    }

    _deactivate(p_instance);
    pool->available.push_back(ObjectID(instance_id));
    emit_signal("despawned", p_instance);
}

int64_t PoolSpawner::get_available_count(const Ref<PackedScene> &p_scene) const
{
    if (p_scene.is_null())
    {
        return 0;
    }
    const Pool *pool = pools.getptr(p_scene->get_instance_id());
    return pool ? pool->available.size() : 0;
}

// 🧩 Logique interne du pool

PoolSpawner::Pool &PoolSpawner::_get_or_create_pool(const Ref<PackedScene> &p_scene)
{
    const uint64_t scene_id = p_scene->get_instance_id();
    if (!pools.has(scene_id))
    {
        Pool pool;
        pool.scene = p_scene; // garde la scène chargée tant que le pool existe (son id reste valide)
        pools.insert(scene_id, pool);
    }
    return pools[scene_id];
}

Node *PoolSpawner::_create_instance(const Ref<PackedScene> &p_scene)
{
    Node *instance = p_scene->instantiate();
    if (instance)
    {
        instance_to_scene.insert(instance->get_instance_id(), p_scene->get_instance_id());
    }
    return instance;
}

Node *PoolSpawner::_acquire_instance(const Ref<PackedScene> &p_scene)
{
    Pool &pool = _get_or_create_pool(p_scene);

    // Les instances libérées entre-temps (queue_free extérieur) sont ignorées
    while (!pool.available.is_empty())
    {
        const ObjectID id = pool.available[pool.available.size() - 1];
        pool.available.remove_at(pool.available.size() - 1);
        if (Node *instance = Object::cast_to<Node>(ObjectDB::get_instance(id)))
        {
            return instance;
        }
        instance_to_scene.erase((uint64_t)id);
    }

    // Pool épuisé : agrandissement dynamique
    WARN_PRINT(String(U"[PoolSpawner] Pool épuisé pour ") + p_scene->get_path() + String(U" : allocation d'une nouvelle instance."));
    Node *instance = _create_instance(p_scene);
    if (instance)
    {
        _get_spawn_parent()->add_child(instance);
    }
    return instance;
}

void PoolSpawner::_on_instance_placed(Node *p_instance) { _activate(p_instance); }

void PoolSpawner::_activate(Node *p_instance)
{
    p_instance->set_process_mode(PROCESS_MODE_INHERIT);
    if (Node3D *node_3d = Object::cast_to<Node3D>(p_instance))
    {
        node_3d->set_visible(true);
    }
    else if (CanvasItem *canvas_item = Object::cast_to<CanvasItem>(p_instance))
    {
        canvas_item->set_visible(true);
    }

    // Réinitialisation de l'état interne de l'objet (points de vie, vélocité...)
    if (p_instance->has_method("on_spawn_from_pool"))
    {
        p_instance->call("on_spawn_from_pool");
    }
}

void PoolSpawner::_deactivate(Node *p_instance)
{
    p_instance->set_process_mode(PROCESS_MODE_DISABLED);
    if (Node3D *node_3d = Object::cast_to<Node3D>(p_instance))
    {
        node_3d->set_visible(false);
    }
    else if (CanvasItem *canvas_item = Object::cast_to<CanvasItem>(p_instance))
    {
        canvas_item->set_visible(false);
    }
}

// 🔗 Liaisons

void PoolSpawner::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("despawn", "instance"), &PoolSpawner::despawn);
    ClassDB::bind_method(D_METHOD("get_available_count", "scene"), &PoolSpawner::get_available_count);

    ADD_SIGNAL(MethodInfo("despawned", PropertyInfo(Variant::OBJECT, "instance", PROPERTY_HINT_NODE_TYPE, "Node")));
}
