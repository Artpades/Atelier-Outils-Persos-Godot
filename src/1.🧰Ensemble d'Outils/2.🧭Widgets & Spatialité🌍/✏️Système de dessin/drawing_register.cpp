#include "🎛️Atelier/drawing_studio.hpp"
#include "📄Document/drawing_document.hpp"
#include "📄Document/drawing_layer.hpp"
#include "🖌️Pinceaux/drawing_brush.hpp"
#include "🖼️Toile/drawing_canvas.hpp"
#include "🧑‍💻Éditeur/drawing_editor_plugin.hpp"

#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_drawing(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        // Éditeur : écran principal « Dessin » et menu contextuel du FileSystem (classes internes).
        GDREGISTER_INTERNAL_CLASS(DrawingContextMenuPlugin);
        GDREGISTER_INTERNAL_CLASS(DrawingEditorPlugin);
        EditorPlugins::add_by_type<DrawingEditorPlugin>();
        return;
    }
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_CLASS(DrawingBrush);    // @icon res://🖼️Editor Icons/🧭🌍/DrawingBrush Icon.png
    GDREGISTER_CLASS(DrawingLayer);    // @icon res://🖼️Editor Icons/🧭🌍/DrawingLayer Icon.png
    GDREGISTER_CLASS(DrawingDocument); // @icon res://🖼️Editor Icons/🧭🌍/DrawingDocument Icon.png
    GDREGISTER_CLASS(DrawingCanvas);   // @icon res://🖼️Editor Icons/🧭🌍/Canva Icon.png
    GDREGISTER_CLASS(DrawingStudio);   // @icon res://🖼️Editor Icons/🧭🌍/DrawingStudio Icon.png

    godot::print_line_rich(U"[color=dark_green]✏️Module Système de dessin chargé avec succès ![/color]");
}

static void uninitialize_drawing(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]✏️Module Système de dessin déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT drawing_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_drawing);
        init_obj.register_terminator(uninitialize_drawing);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
