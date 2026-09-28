#include "commun/module_entry.hpp"

#include "widget_property_modifier_builder.hpp"
#include "Callback pour les type/property_callback.hpp"
#include "🔍Inspecteur procédural/inspector_binding.hpp"
#include "🔍Inspecteur procédural/procedural_inspector.hpp"

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_CLASS(WidgetPropertyModifierBuilder);
    GDREGISTER_CLASS(PropertyCallback);
    GDREGISTER_INTERNAL_CLASS(InspectorBinding);
    GDREGISTER_CLASS(ProceduralInspector);
}

void uninitialize_module(ModuleInitializationLevel p_level) {}

GAMEPLAYTOOLKIT_MODULE(widgets, U"🧩 Générateur de Widget d'éditions")
