#include "drawing_editor_plugin.hpp"

#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

static const char *PLUGIN_NAME = "Dessin";

static bool is_image_path(const String &p_path)
{
    const String extension = p_path.get_extension().to_lower();
    return extension == "png" || extension == "jpg" || extension == "jpeg" || extension == "webp";
}

// --- DrawingContextMenuPlugin ---

void DrawingContextMenuPlugin::_popup_menu(const PackedStringArray &p_paths)
{
    if (p_paths.size() != 1 || !DrawingEditorPlugin::can_open(p_paths[0]))
    {
        return;
    }
    Ref<Texture2D> icon = EditorInterface::get_singleton()->get_editor_theme()->get_icon("Edit", "EditorIcons");
    add_context_menu_item(U"Ouvrir dans l'atelier de dessin", callable_mp(this, &DrawingContextMenuPlugin::_on_open), icon);
}

void DrawingContextMenuPlugin::_on_open(const Variant &p_paths)
{
    const PackedStringArray paths = p_paths;
    if (plugin && !paths.is_empty())
    {
        plugin->open_path(paths[0]);
    }
}

// --- DrawingEditorPlugin ---

void DrawingEditorPlugin::_enter_tree()
{
    studio = memnew(DrawingStudio);
    studio->set_name("DrawingStudio");
    studio->set_file_access(FileDialog::ACCESS_RESOURCES);
    studio->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    studio->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    studio->hide();
    studio->connect("document_saved", callable_mp(this, &DrawingEditorPlugin::_on_saved));
    EditorInterface::get_singleton()->get_editor_main_screen()->add_child(studio);

    context_menu.instantiate();
    context_menu->set_plugin(this);
    add_context_menu_plugin(EditorContextMenuPlugin::CONTEXT_SLOT_FILESYSTEM, context_menu);
}

void DrawingEditorPlugin::_exit_tree()
{
    if (context_menu.is_valid())
    {
        context_menu->set_plugin(nullptr);
        remove_context_menu_plugin(context_menu);
        context_menu.unref();
    }
    if (studio)
    {
        studio->queue_free();
        studio = nullptr;
    }
}

String DrawingEditorPlugin::_get_plugin_name() const { return PLUGIN_NAME; }

Ref<Texture2D> DrawingEditorPlugin::_get_plugin_icon() const { return EditorInterface::get_singleton()->get_editor_theme()->get_icon("Edit", "EditorIcons"); }

void DrawingEditorPlugin::_make_visible(bool p_visible)
{
    if (studio)
    {
        studio->set_visible(p_visible);
    }
}

bool DrawingEditorPlugin::_handles(Object *p_object) const { return Object::cast_to<DrawingDocument>(p_object) != nullptr; }

void DrawingEditorPlugin::_edit(Object *p_object)
{
    DrawingDocument *document = Object::cast_to<DrawingDocument>(p_object);
    if (studio && document && document != studio->get_document().ptr())
    {
        studio->edit_document(Ref<DrawingDocument>(document), document->get_path());
    }
}

bool DrawingEditorPlugin::can_open(const String &p_path)
{
    if (is_image_path(p_path))
    {
        return true;
    }
    const String extension = p_path.get_extension().to_lower();
    if (extension != "tres" && extension != "res")
    {
        return false;
    }
    return EditorInterface::get_singleton()->get_resource_filesystem()->get_file_type(p_path) == String("DrawingDocument");
}

void DrawingEditorPlugin::open_path(const String &p_path)
{
    if (!studio)
    {
        return;
    }
    EditorInterface::get_singleton()->set_main_screen_editor(PLUGIN_NAME);
    studio->open_file(p_path);
}

void DrawingEditorPlugin::_on_saved(const String &p_path)
{
    if (!p_path.begins_with("res://"))
    {
        return;
    }
    // Fichier du projet : le dock FileSystem et les ressources importées doivent refléter l'enregistrement.
    EditorFileSystem *filesystem = EditorInterface::get_singleton()->get_resource_filesystem();
    if (filesystem->get_file_type(p_path).is_empty())
    {
        filesystem->scan(); // nouveau fichier
    }
    else if (is_image_path(p_path))
    {
        PackedStringArray files;
        files.push_back(p_path);
        filesystem->reimport_files(files);
    }
    else
    {
        filesystem->update_file(p_path);
    }
}
