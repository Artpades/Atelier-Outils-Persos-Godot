#ifndef POOL_SPAWNER_HPP
#define POOL_SPAWNER_HPP

#include "spawner.hpp"

#include <godot_cpp/templates/hash_map.hpp>

namespace godot
{
/// 🏭♻️ Spawner à réserve d'objets : les instances sont pré-allouées au démarrage (warm-up) puis
/// recyclées par despawn() au lieu d'être détruites.
///
/// Instance inactive : process_mode DISABLED et masquée. À sa réactivation, sa méthode
/// on_spawn_from_pool() est appelée si elle existe (réinitialiser points de vie, vélocité...).
/// Les entrées SpawnPoolEntry fixent la taille initiale du pool ; une SpawnEntry simple n'en pré-alloue aucune.
class PoolSpawner : public Spawner
{
    GDCLASS(PoolSpawner, Spawner)

  public:
    void _ready() override;

    /// Remet p_instance dans son pool (le détruit s'il n'appartient pas à ce PoolSpawner).
    void despawn(Node *p_instance);
    /// Nombre d'instances disponibles dans le pool de p_scene.
    int64_t get_available_count(const Ref<PackedScene> &p_scene) const;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

    Node *_acquire_instance(const Ref<PackedScene> &p_scene) override;
    void _on_instance_placed(Node *p_instance) override;

  private:
    struct Pool
    {
        Ref<PackedScene> scene;
        Vector<ObjectID> available;
    };

    // Clés : instance_id de la PackedScene / de l'instance (les ObjectID évitent tout pointeur périmé).
    HashMap<uint64_t, Pool> pools;
    // Instance -> scène d'origine, pour la recycler dans le bon pool
    HashMap<uint64_t, uint64_t> instance_to_scene;

    Pool &_get_or_create_pool(const Ref<PackedScene> &p_scene);
    Node *_create_instance(const Ref<PackedScene> &p_scene);
    static void _activate(Node *p_instance);
    static void _deactivate(Node *p_instance);
};
} // namespace godot

#endif // POOL_SPAWNER_HPP
