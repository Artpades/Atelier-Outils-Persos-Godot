#include "commun/module_entry.hpp"

#include "⚙️Générateur de Gestionnaire/manager_builder.hpp"
#include "✋Handlers/handlers_manager.hpp"
#include "🌳Hiérarchie de scène/scene_hierarchy_tree.hpp"
#include "🐛Gizmos/gizmos_2d.hpp"
#include "🐛Gizmos/gizmos_3d.hpp"
#include "📰Journal/custom_logger.hpp"
#include "🧩Générateur de Widget d'éditions/Callback pour les type/property_callback.hpp"
#include "🧩Générateur de Widget d'éditions/widget_property_modifier_builder.hpp"
#include "🧩Générateur de Widget d'éditions/🔍Inspecteur procédural/inspector_binding.hpp"
#include "🧩Générateur de Widget d'éditions/🔍Inspecteur procédural/procedural_inspector.hpp"
#include "🪟Fenêtre d'édition/window_editor.hpp"

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    // 📰 Journal
    GDREGISTER_ABSTRACT_CLASS(CustomLogger);

    // 🐛 Gizmos
    GDREGISTER_CLASS(Gizmos2D);
    GDREGISTER_CLASS(Gizmos3D);

    // ⚙️ Générateur de Gestionnaire
    GDREGISTER_CLASS(ManagerBuilder);

    // 🧩 Générateur de Widget d'éditions
    GDREGISTER_CLASS(WidgetPropertyModifierBuilder);
    GDREGISTER_CLASS(PropertyCallback);
    GDREGISTER_INTERNAL_CLASS(InspectorBinding);
    GDREGISTER_CLASS(ProceduralInspector);

    // 🪟 Fenêtre d'édition
    GDREGISTER_CLASS(WindowEditor);

    // 🌳 Hiérarchie de scène
    GDREGISTER_CLASS(SceneHierarchyTree);

    // ✋ Handlers
    GDREGISTER_CLASS(HandlersManager);
}

void uninitialize_module(ModuleInitializationLevel p_level) {}

GAMEPLAYTOOLKIT_MODULE(edition_debug, U"🧑‍💻 Édition et Debug")
