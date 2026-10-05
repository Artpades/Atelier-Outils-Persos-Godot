#include "🐛Gizmos/gizmos_2d.hpp"
#include "🐛Gizmos/gizmos_3d.hpp"
#include "🐛Gizmos/gizmos.hpp"

#include "📰Journal Console/custom_logger.hpp"

#include "⚙️Générateur de Gestionnaire/manager_builder.hpp"

#include "🪟Fenêtre d'édition/window_editor.hpp"

#include "💻️Editeur d'objet/📋Données/data_registry.hpp"
#include "💻️Editeur d'objet/🎯Gizmos/gizmo_host.hpp"
#include "💻️Editeur d'objet/🔌Accès/runtime_undo_backend.hpp"
#include "💻️Editeur d'objet/🔧Réglages/widget_settings.hpp"
#include "💻️Editeur d'objet/🧑‍💻Éditeur/editor_undo_backend.hpp"
#include "💻️Editeur d'objet/🧑‍💻Éditeur/group_matrix_editor.hpp"
#include "💻️Editeur d'objet/🧑‍💻Éditeur/gizmo_config_window.hpp"
#include "💻️Editeur d'objet/🧑‍💻Éditeur/interface_config_window.hpp"
#include "💻️Editeur d'objet/🧑‍💻Éditeur/interface_style_editor.hpp"
#include "💻️Editeur d'objet/🧑‍💻Éditeur/layout_database_window.hpp"
#include "💻️Editeur d'objet/🧑‍💻Éditeur/object_interface_panel.hpp"
#include "💻️Editeur d'objet/🧑‍💻Éditeur/layout_database_export_plugin.hpp"
#include "💻️Editeur d'objet/🧑‍💻Éditeur/object_editor_plugin.hpp"
#include "💻️Editeur d'objet/🧩Widgets/container_widgets.hpp"
#include "💻️Editeur d'objet/🧩Widgets/scalar_widgets.hpp"
#include "💻️Editeur d'objet/🧩Widgets/signal_widget.hpp"
#include "💻️Editeur d'objet/🧩Widgets/special_widgets.hpp"
#include "💻️Editeur d'objet/🧩Widgets/struct_widget.hpp"
#include "💻️Editeur d'objet/🧪Démo/object_editor_demo.hpp"
#include "💻️Editeur d'objet/🧱Construction/layout_linker.hpp"
#include "💻️Editeur d'objet/🧱Construction/object_editor_window.hpp"
#include "💻️Editeur d'objet/🧱Construction/procedural_editor.hpp"

#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

#include "editor_test.hpp"

using namespace godot;

// Enregistrement des outils

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        // Éditeur d'objet : annulation via EditorUndoRedoManager (sa base UndoBackend est enregistrée au niveau SCENE).
        GDREGISTER_CLASS(EditorUndoBackend);
        // Fenêtre « Configurer l'interface » et son point d'entrée dans l'inspecteur (classes internes).
        GDREGISTER_INTERNAL_CLASS(DataMatrixEditor);
        GDREGISTER_INTERNAL_CLASS(GroupMatrixEditor);
        GDREGISTER_INTERNAL_CLASS(GizmoListEditor);
        GDREGISTER_INTERNAL_CLASS(InterfaceStyleEditor);
        GDREGISTER_INTERNAL_CLASS(GizmoConfigWindow);
        GDREGISTER_ABSTRACT_CLASS(LayoutDatabaseWindow); // abstraite : open() statique accessible aux scripts de test
        GDREGISTER_INTERNAL_CLASS(InterfaceConfigWindow);
        GDREGISTER_INTERNAL_CLASS(ObjectInterfacePanel);
        GDREGISTER_INTERNAL_CLASS(LayoutDatabaseExportPlugin);
        GDREGISTER_INTERNAL_CLASS(ProceduralEditorInspectorPlugin);
        GDREGISTER_INTERNAL_CLASS(ObjectEditorPlugin);
        EditorPlugins::add_by_type<ObjectEditorPlugin>();

        // Classe interne : un plugin n'a pas à apparaître dans la liste « Créer un nœud ».
        GDREGISTER_INTERNAL_CLASS(EditorTest);
        // L'éditeur instancie et libère le plugin ; godot-cpp le retire au déchargement du niveau EDITOR.
        EditorPlugins::add_by_type<EditorTest>();
    }

    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }
    GDREGISTER_INTERNAL_CLASS(GizmoResources);
    GDREGISTER_CLASS(L);

    GDREGISTER_CLASS(ManagerBuilder);
    GDREGISTER_CLASS(WindowEditor);
    GDREGISTER_CLASS(Gizmos2D);
    GDREGISTER_CLASS(Gizmos3D);

    // Éditeur d'objet (runtime). Ordre : une classe parente avant ses classes filles.
    GDREGISTER_INTERNAL_CLASS(MaterialPropagator);
    GDREGISTER_CLASS(ShaderParameterBinding);
    GDREGISTER_CLASS(InterfaceStyle);
    GDREGISTER_CLASS(WidgetSettings);
    GDREGISTER_CLASS(RangeWidgetSettings);
    GDREGISTER_CLASS(TextureProgressWidgetSettings);
    GDREGISTER_CLASS(TextEditWidgetSettings);
    GDREGISTER_CLASS(StructWidgetSettings);
    GDREGISTER_CLASS(ContainerWidgetSettings);
    GDREGISTER_CLASS(ArgumentBinding);
    GDREGISTER_CLASS(SignalLink);
    GDREGISTER_CLASS(DataEntry);
    GDREGISTER_CLASS(DataRegistry);
    GDREGISTER_CLASS(GroupEntry);
    GDREGISTER_CLASS(GroupRegistry);
    GDREGISTER_CLASS(ValueAccessor);
    GDREGISTER_ABSTRACT_CLASS(UndoBackend);
    GDREGISTER_CLASS(RuntimeUndoBackend);
    GDREGISTER_INTERNAL_CLASS(WidgetContext);
    GDREGISTER_CLASS(PropertyWidget);
    GDREGISTER_CLASS(BoolWidget);
    GDREGISTER_CLASS(NumberWidget);
    GDREGISTER_CLASS(TextWidget);
    GDREGISTER_CLASS(EnumWidget);
    GDREGISTER_CLASS(ColorWidget);
    GDREGISTER_CLASS(StructWidget);
    GDREGISTER_CLASS(ContainerWidget);
    GDREGISTER_CLASS(CollectionWidget);
    GDREGISTER_CLASS(ObjectWidget);
    GDREGISTER_CLASS(PathWidget);
    GDREGISTER_CLASS(VariantWidget);
    GDREGISTER_INTERNAL_CLASS(ArgumentStore);
    GDREGISTER_CLASS(MethodWidget);
    GDREGISTER_CLASS(SignalWidget);
    GDREGISTER_INTERNAL_CLASS(NodeSelectorDialog);
    GDREGISTER_CLASS(WidgetFactory);
    GDREGISTER_CLASS(SyncController);
    GDREGISTER_INTERNAL_CLASS(ProceduralEditorContext);
    GDREGISTER_CLASS(ProceduralEditor);
    GDREGISTER_CLASS(ObjectEditorWindow);
    GDREGISTER_CLASS(GizmoEntry);
    GDREGISTER_CLASS(GizmoRegistry);
    GDREGISTER_CLASS(GizmoHost);
    GDREGISTER_CLASS(TypeLayout);
    GDREGISTER_CLASS(LayoutDatabase);
    GDREGISTER_ABSTRACT_CLASS(LayoutLinker); // visible des scripts (get_singleton), non instanciable
    GDREGISTER_CLASS(ObjectEditorDemoResource);
    GDREGISTER_CLASS(ObjectEditorDemo);

    // Interfaces par type : réglage de projet, et application des liens par type aux nœuds en jeu.
    LayoutDatabase::register_project_setting();
    InterfaceStyle::register_project_setting();
    LayoutLinker::create();

    godot::print_line_rich(U"[color=dark_green]🛠️Module d'outils d'edition et de debug chargé avec succès ![/color]");
}

static void uninitialize(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    LayoutLinker::destroy();
    MaterialPropagator::destroy();
    // Ressources partagées des gizmos 3D (shader, matériaux, meshes) : libérées tant que le moteur existe encore.
    GizmoResources::destroy();
    godot::print_line_rich(U"[color=dark_red]🛠️Module d'outils d'edition et de debug déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT tool_editor_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize);
        init_obj.register_terminator(uninitialize);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
