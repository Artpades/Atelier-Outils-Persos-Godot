#include "save_data_binder.hpp"
#include "save_manager.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/object.hpp>

using namespace godot;

void SaveDataBinder::set_node_target(Node *p_node) { node_target_id = p_node ? ObjectID(p_node->get_instance_id()) : ObjectID(); }

Node *SaveDataBinder::get_node_target() const
{
    if (node_target_id.is_null())
    {
        return nullptr;
    }
    return Object::cast_to<Node>(ObjectDB::get_instance(node_target_id));
}

Object *SaveDataBinder::get_target() const
{
    Node *node = get_node_target();
    if (node)
    {
        return node;
    }
    return resource_target.ptr();
}

// 💾 Pull vers SaveData

void SaveDataBinder::pull_to_save_data()
{
    Object *target = get_target();
    if (!target || save_data.is_null())
    {
        return;
    }

    // Dictionary est partagé par référence : écrire dans cette copie modifie celui du SaveData.
    Dictionary data = save_data->get_data();
    for (int64_t i = 0; i < tracked_properties.size(); i++)
    {
        const String &property = tracked_properties[i];
        data[property] = target->get(property);
    }
}

// 🧾 Push depuis SaveData

void SaveDataBinder::push_from_save_data()
{
    Object *target = get_target();
    if (!target || save_data.is_null())
    {
        return;
    }

    Dictionary data = save_data->get_data();
    for (int64_t i = 0; i < tracked_properties.size(); i++)
    {
        const String &property = tracked_properties[i];
        if (data.has(property))
        {
            target->set(property, data[property]);
        }
    }
}

// 💾 Sauvegarde complète

void SaveDataBinder::save()
{
    ERR_FAIL_COND_MSG(save_data.is_null(), "SaveDataBinder : aucun SaveData assigné.");

    pull_to_save_data();
    SaveManager::save(save_data, save_data->get_file_config());
}

void SaveDataBinder::load()
{
    ERR_FAIL_COND_MSG(save_data.is_null(), "SaveDataBinder : aucun SaveData assigné.");

    Ref<SaveData> loaded = SaveManager::load(save_data, save_data->get_file_config());
    if (loaded.is_null())
    {
        return;
    }
    save_data->set_data(loaded->get_data());
    push_from_save_data();
}

void SaveDataBinder::delete_save()
{
    ERR_FAIL_COND_MSG(save_data.is_null(), "SaveDataBinder : aucun SaveData assigné.");

    SaveManager::delete_save(save_data, save_data->get_file_config());
}

void SaveDataBinder::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_save_data", "save_data"), &SaveDataBinder::set_save_data);
    ClassDB::bind_method(D_METHOD("get_save_data"), &SaveDataBinder::get_save_data);
    ClassDB::bind_method(D_METHOD("set_node_target", "node"), &SaveDataBinder::set_node_target);
    ClassDB::bind_method(D_METHOD("get_node_target"), &SaveDataBinder::get_node_target);
    ClassDB::bind_method(D_METHOD("set_resource_target", "resource"), &SaveDataBinder::set_resource_target);
    ClassDB::bind_method(D_METHOD("get_resource_target"), &SaveDataBinder::get_resource_target);
    ClassDB::bind_method(D_METHOD("set_tracked_properties", "properties"), &SaveDataBinder::set_tracked_properties);
    ClassDB::bind_method(D_METHOD("get_tracked_properties"), &SaveDataBinder::get_tracked_properties);

    ClassDB::bind_method(D_METHOD("pull_to_save_data"), &SaveDataBinder::pull_to_save_data);
    ClassDB::bind_method(D_METHOD("push_from_save_data"), &SaveDataBinder::push_from_save_data);
    ClassDB::bind_method(D_METHOD("save"), &SaveDataBinder::save);
    ClassDB::bind_method(D_METHOD("load"), &SaveDataBinder::load);
    ClassDB::bind_method(D_METHOD("delete"), &SaveDataBinder::delete_save);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "save_data", PROPERTY_HINT_RESOURCE_TYPE, "SaveData"), "set_save_data", "get_save_data");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "node_target", PROPERTY_HINT_NODE_TYPE, "Node"), "set_node_target", "get_node_target");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "resource_target", PROPERTY_HINT_RESOURCE_TYPE, "Resource"), "set_resource_target", "get_resource_target");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "tracked_properties"), "set_tracked_properties", "get_tracked_properties");
}