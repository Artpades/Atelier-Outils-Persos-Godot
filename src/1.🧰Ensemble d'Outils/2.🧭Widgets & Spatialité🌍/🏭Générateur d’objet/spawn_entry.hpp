#ifndef SPAWN_ENTRY_HPP
#define SPAWN_ENTRY_HPP

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// 🏭 Configuration d'un Spawner : quelles scènes instancier, et à quelles positions.
/// Les positions sont des chemins relatifs au Spawner ; vides, ce sont ses enfants Node2D / Node3D.
class SpawnEntry : public Resource
{
    GDCLASS(SpawnEntry, Resource)

  public:
    enum SelectionMode
    {
        SELECTION_SEQUENTIAL, // un élément à chaque spawn, dans l'ordre
        SELECTION_RANDOM,     // un élément au hasard à chaque spawn
        SELECTION_ALL,        // tous les éléments à chaque spawn
    };

    void set_positions(const TypedArray<NodePath> &p_positions) { positions = p_positions; }
    TypedArray<NodePath> get_positions() const { return positions; }
    void set_scenes(const TypedArray<PackedScene> &p_scenes) { scenes = p_scenes; }
    TypedArray<PackedScene> get_scenes() const { return scenes; }
    void set_positions_selection_mode(SelectionMode p_mode) { positions_selection_mode = p_mode; }
    SelectionMode get_positions_selection_mode() const { return positions_selection_mode; }
    void set_scenes_selection_mode(SelectionMode p_mode) { scenes_selection_mode = p_mode; }
    SelectionMode get_scenes_selection_mode() const { return scenes_selection_mode; }

  protected:
    static void _bind_methods();

  private:
    /// Chemins relatifs au Spawner des nœuds dont la transform globale est copiée (vide : enfants 2D / 3D du Spawner).
    TypedArray<NodePath> positions;
    TypedArray<PackedScene> scenes;
    SelectionMode positions_selection_mode = SELECTION_SEQUENTIAL;
    SelectionMode scenes_selection_mode = SELECTION_SEQUENTIAL;
};

/// 🏭♻️ SpawnEntry d'un PoolSpawner : ajoute la taille du pool pré-alloué pour chaque scène.
class SpawnPoolEntry : public SpawnEntry
{
    GDCLASS(SpawnPoolEntry, SpawnEntry)

  public:
    void set_initial_pool_size_per_scene(int p_size) { initial_pool_size_per_scene = p_size; }
    int get_initial_pool_size_per_scene() const { return initial_pool_size_per_scene; }

  protected:
    static void _bind_methods();

  private:
    /// Nombre d'instances pré-allouées par scène de la liste.
    int initial_pool_size_per_scene = 10;
};
} // namespace godot

VARIANT_ENUM_CAST(SpawnEntry::SelectionMode);

#endif // SPAWN_ENTRY_HPP
