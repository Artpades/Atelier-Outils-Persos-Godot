#include "commun/module_entry.hpp"

#include "save_config.hpp"
#include "save_data.hpp"
#include "save_data_binder.hpp"
#include "save_file_access.hpp"
#include "save_manager.hpp"
#include "save_serializer.hpp"

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_CLASS(SaveConfig);
    GDREGISTER_CLASS(SaveData);
    GDREGISTER_CLASS(SaveDataBinder);
    GDREGISTER_ABSTRACT_CLASS(SaveSerializer);
    GDREGISTER_ABSTRACT_CLASS(SaveFileAccess);
    GDREGISTER_ABSTRACT_CLASS(SaveManager);
}

void uninitialize_module(ModuleInitializationLevel p_level) {}

GAMEPLAYTOOLKIT_MODULE(save, U"💾 Manipulateur de données")
