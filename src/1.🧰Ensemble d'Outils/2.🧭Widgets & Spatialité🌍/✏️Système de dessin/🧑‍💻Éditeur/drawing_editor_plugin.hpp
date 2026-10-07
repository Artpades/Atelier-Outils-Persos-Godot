#ifndef DRAWING_EDITOR_PLUGIN_HPP
#define DRAWING_EDITOR_PLUGIN_HPP

#include "../🎛️Atelier/drawing_studio.hpp"

#include <godot_cpp/classes/editor_context_menu_plugin.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>

namespace godot
{
class DrawingEditorPlugin;

/// Entrée « Ouvrir dans l'atelier de dessin » du menu contextuel du dock FileSystem (images et DrawingDocument).
class DrawingContextMenuPlugin : public EditorContextMenuPlugin
{
    GDCLASS(DrawingContextMenuPlugin, EditorContextMenuPlugin)

  public:
    void _popup_menu(const PackedStringArray &p_paths) override;
    void set_plugin(DrawingEditorPlugin *p_plugin) { plugin = p_plugin; }

  protected:
    static void _bind_methods() {}

  private:
    DrawingEditorPlugin *plugin = nullptr;
    void _on_open(const Variant &p_paths);
};

/// Plugin d'éditeur du système de dessin : écran principal « Dessin » (un DrawingStudio), ouverture des images et
/// des DrawingDocument du projet, réimportation des images enregistrées.
class DrawingEditorPlugin : public EditorPlugin
{
    GDCLASS(DrawingEditorPlugin, EditorPlugin)

  public:
    void _enter_tree() override;
    void _exit_tree() override;
    bool _has_main_screen() const override { return true; }
    String _get_plugin_name() const override;
    Ref<Texture2D> _get_plugin_icon() const override;
    void _make_visible(bool p_visible) override;
    bool _handles(Object *p_object) const override;
    void _edit(Object *p_object) override;

    /// Vrai pour une image (.png, .jpg, .jpeg, .webp) ou un DrawingDocument (.tres, .res).
    static bool can_open(const String &p_path);
    /// Affiche l'écran « Dessin » et y ouvre le fichier.
    void open_path(const String &p_path);
    DrawingStudio *get_studio() const { return studio; }

  protected:
    static void _bind_methods() {}

  private:
    DrawingStudio *studio = nullptr;
    Ref<DrawingContextMenuPlugin> context_menu;

    void _on_saved(const String &p_path);
};
} // namespace godot

#endif // DRAWING_EDITOR_PLUGIN_HPP
