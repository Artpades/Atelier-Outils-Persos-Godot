#ifndef SAVE_DATA_BINDER_HPP
#define SAVE_DATA_BINDER_HPP

#include "save_data.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot
{
/// 🔗 Pont entre un SaveData et un Node ou une Resource cible : synchronise les propriétés listées dans
/// `tracked_properties`, sans code de sérialisation spécifique à chaque type d'objet.
///
/// Cible : `node_target` s'il est défini, sinon `resource_target`.
class SaveDataBinder : public Node
{
    GDCLASS(SaveDataBinder, Node)

  public:
    void set_save_data(const Ref<SaveData> &p_save_data) { save_data = p_save_data; }
    Ref<SaveData> get_save_data() const { return save_data; }

    void set_node_target(Node *p_node);
    Node *get_node_target() const;

    void set_resource_target(const Ref<Resource> &p_resource) { resource_target = p_resource; }
    Ref<Resource> get_resource_target() const { return resource_target; }

    void set_tracked_properties(const PackedStringArray &p_properties) { tracked_properties = p_properties; }
    PackedStringArray get_tracked_properties() const { return tracked_properties; }

    /// Lit les propriétés suivies sur la cible et les écrit dans les données du SaveData.
    void pull_to_save_data();
    /// Applique les valeurs du SaveData sur les propriétés suivies de la cible.
    void push_from_save_data();

    /// Lit les propriétés de la cible, les écrit dans le SaveData puis sauvegarde sur disque.
    void save();
    /// Charge les données depuis le disque et les applique sur la cible.
    void load();
    /// Supprime le fichier de sauvegarde associé au SaveData.
    void delete_save();

  protected:
    static void _bind_methods();

  private:
    Ref<SaveData> save_data;
    // ObjectID plutôt qu'un pointeur : le nœud cible peut être libéré sans prévenir le binder.
    ObjectID node_target_id;
    Ref<Resource> resource_target;
    PackedStringArray tracked_properties;

    /// Cible active : node_target, sinon resource_target.
    Object *get_target() const;
};
} // namespace godot

#endif // SAVE_DATA_BINDER_HPP
