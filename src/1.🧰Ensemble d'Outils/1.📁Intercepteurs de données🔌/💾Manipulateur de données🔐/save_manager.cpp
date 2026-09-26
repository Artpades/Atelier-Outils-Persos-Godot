#include "save_manager.hpp"
#include "save_file_access.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal/custom_logger.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

namespace
{
const char32_t *SAVE_MANAGER_ICON = U"res://🖼️Editor Icons/💾🔐/SaveManager Icon.png";
}

void SaveManager::save(const Ref<SaveData> &p_save_data, const Ref<SaveConfig> &p_config)
{
    ERR_FAIL_COND_MSG(p_save_data.is_null(), "SaveManager : SaveData nul.");

    Error error = SaveFileAccess::write(p_save_data, p_config);
    if (error != OK)
    {
        CustomLogger::log("SaveManager", SAVE_MANAGER_ICON,
                          String(U"[color=red]❌ Échec de la sauvegarde de ") + p_save_data->get_system_id() + " (erreur " + itos(error) + ")[/color]");
        return;
    }
    CustomLogger::log("SaveManager", SAVE_MANAGER_ICON,
                      String(U"[color=green]✅ Sauvegarde de ") + p_save_data->get_system_id() + String(U" effectuée ![/color]"));
}

Ref<SaveData> SaveManager::load(const Ref<SaveData> &p_save_data, const Ref<SaveConfig> &p_config)
{
    ERR_FAIL_COND_V_MSG(p_save_data.is_null(), Ref<SaveData>(), "SaveManager : SaveData nul.");

    Ref<SaveData> data = SaveFileAccess::read(p_save_data->get_system_id(), p_config, p_save_data->get_is_encrypted(), p_save_data->get_encryption_key());
    if (data.is_null())
    {
        CustomLogger::log("SaveManager", SAVE_MANAGER_ICON,
                          String(U"[color=yellow]Aucune sauvegarde trouvée pour ") + p_save_data->get_system_id() + "[/color]");
    }
    return data;
}

void SaveManager::delete_save(const Ref<SaveData> &p_save_data, const Ref<SaveConfig> &p_config)
{
    ERR_FAIL_COND_MSG(p_save_data.is_null(), "SaveManager : SaveData nul.");

    SaveFileAccess::delete_file(p_save_data->get_system_id(), p_config, p_save_data->get_is_encrypted());
}

void SaveManager::_bind_methods()
{
    ClassDB::bind_static_method("SaveManager", D_METHOD("save", "save_data", "config"), &SaveManager::save);
    ClassDB::bind_static_method("SaveManager", D_METHOD("load", "save_data", "config"), &SaveManager::load);
    ClassDB::bind_static_method("SaveManager", D_METHOD("delete", "save_data", "config"), &SaveManager::delete_save);
}
