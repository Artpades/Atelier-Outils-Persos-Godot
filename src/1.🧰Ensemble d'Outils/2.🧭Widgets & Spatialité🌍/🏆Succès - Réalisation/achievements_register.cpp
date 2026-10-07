#include "achievement.hpp"
#include "achievement_condition.hpp"
#include "achievement_manager.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_achievements(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : les classes ne doivent être enregistrées qu'une fois
    }

    GDREGISTER_CLASS(AchievementCondition);           // @icon res://🖼️Editor Icons/🧭🌍/AchievementCondition Icon.png
    GDREGISTER_CLASS(AchievementCounterCondition);    // @icon res://🖼️Editor Icons/🧭🌍/AchievementCounterCondition Icon.png
    GDREGISTER_CLASS(AchievementCollectionCondition); // @icon res://🖼️Editor Icons/🧭🌍/AchievementCollectionCondition Icon.png
    GDREGISTER_CLASS(Achievement);                    // @icon res://🖼️Editor Icons/🧭🌍/Achievement Icon.png
    GDREGISTER_CLASS(AchievementManager);             // @icon res://🖼️Editor Icons/🧭🌍/AchievementManager Icon.png

    godot::print_line_rich(U"[color=dark_green]🏆Module de succès chargé avec succès ![/color]");
}

static void uninitialize_achievements(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]🏆Module de succès déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT achievements_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_achievements);
        init_obj.register_terminator(uninitialize_achievements);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
