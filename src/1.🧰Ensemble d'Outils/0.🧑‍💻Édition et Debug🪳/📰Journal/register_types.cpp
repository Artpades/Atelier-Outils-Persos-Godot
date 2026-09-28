#include "commun/module_entry.hpp"

#include "custom_logger.hpp"

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_ABSTRACT_CLASS(CustomLogger);
}

void uninitialize_module(ModuleInitializationLevel p_level) {}

GAMEPLAYTOOLKIT_MODULE(journal, U"📰 Journal")
