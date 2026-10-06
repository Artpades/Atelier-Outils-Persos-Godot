#ifndef SPAWNER_HPP
#define SPAWNER_HPP

#include "spawn_entry.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot
{
/// 🏭 Instancie les scènes de ses entrées (SpawnEntry) aux positions configurées.
/// Les instances sont ajoutées sous parent_node (par défaut : le Viewport).
class Spawner : public Node
{
    GDCLASS(Spawner, Node)

  public:
    /// Instancie et place les scènes de toutes les entrées, puis émet "spawned".
    void spawn();

    void set_entries(const TypedArray<SpawnEntry> &p_entries) { entries = p_entries; }
    TypedArray<SpawnEntry> get_entries() const { return entries; }
    void set_parent_node(Node *p_node);
    Node *get_parent_node() const;

  protected:
    static void _bind_methods();

    /// Fournit une instance de p_scene, déjà rattachée (ou en cours de rattachement) au parent de spawn.
    virtual Node *_acquire_instance(const Ref<PackedScene> &p_scene);
    /// Appelé une fois l'instance placée (le PoolSpawner l'active à ce moment-là).
    virtual void _on_instance_placed(Node * /*p_instance*/) {}

    /// parent_node s'il est défini, sinon le Viewport.
    Node *_get_spawn_parent() const;

    TypedArray<SpawnEntry> entries;

  private:
    // ObjectID : le parent peut être libéré sans prévenir le Spawner.
    ObjectID parent_node_id;

    // Index séquentiels par entrée (agrandis à la demande si des entrées sont ajoutées en jeu)
    Vector<int64_t> sequential_scene_indices;
    Vector<int64_t> sequential_position_indices;

    void _spawn_entry(const Ref<SpawnEntry> &p_entry, int64_t p_entry_index);
    /// Positions de l'entrée, ou à défaut les chemins des enfants Node2D / Node3D du Spawner.
    Array _get_positions_of(const Ref<SpawnEntry> &p_entry) const;
    /// Éléments de p_items choisis selon p_mode (r_sequence_index avance en mode séquentiel).
    static Array _select(const Array &p_items, SpawnEntry::SelectionMode p_mode, int64_t &r_sequence_index);
    /// Copie la transform globale de p_target sur p_instance (même hors de l'arbre).
    void _apply_transform(Node *p_instance, Node *p_target) const;
};
} // namespace godot

#endif // SPAWNER_HPP
