#ifndef DRAWING_STUDIO_HPP
#define DRAWING_STUDIO_HPP

#include "../🖼️Toile/drawing_canvas.hpp"

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/button_group.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/color_picker.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// 🎛️ Atelier de dessin complet, inspiré de Clip Studio Paint : une DrawingCanvas entourée de ses panneaux.
///
/// - Barre du haut : fichier (nouveau, ouvrir, enregistrer), annuler / rétablir, vue (ajuster, 100 %, rotation, miroir).
/// - À gauche : outils, puis les réglages de l'outil (pinceau : préréglage, taille, opacité, densité, dureté,
///   espacement, stabilisation, dynamique pression / vitesse de la taille et de l'opacité, grain...).
/// - À droite : couleurs (principale / secondaire) et calques (visibilité, mode de fusion, opacité, verrous,
///   écrêtage, ordre, fusion).
///
/// Fichiers : ouvre les images (.png, .jpg, .webp) et les DrawingDocument (.tres, .res) ; enregistre en image
/// (calques fusionnés) ou en DrawingDocument (calques conservés). Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Maj+S.
/// Utilisé tel quel par le plugin d'éditeur (écran principal « Dessin »).
class DrawingStudio : public Control
{
    GDCLASS(DrawingStudio, Control)

  public:
    DrawingStudio();

    void _notification(int p_what);
    void _gui_input(const Ref<InputEvent> &p_event) override;
    Vector2 _get_minimum_size() const override;

    DrawingCanvas *get_canvas() const { return canvas; }
    void set_document(const Ref<DrawingDocument> &p_document);
    Ref<DrawingDocument> get_document() const { return canvas->get_document(); }

    void set_brushes(const TypedArray<DrawingBrush> &p_brushes);
    TypedArray<DrawingBrush> get_brushes() const { return brushes; }
    void set_erasers(const TypedArray<DrawingBrush> &p_erasers);
    TypedArray<DrawingBrush> get_erasers() const { return erasers; }
    void set_show_file_buttons(bool p_show);
    bool get_show_file_buttons() const { return show_file_buttons; }
    /// Accès des boîtes de dialogue de fichier (FileDialog.Access : 0 ressources, 1 user://, 2 système de fichiers).
    void set_file_access(int p_access);
    int get_file_access() const { return file_access; }
    String get_current_path() const { return current_path; }

    /// Nouveau document (fond opaque de p_background, ou transparent si son alpha est nul).
    void new_document(const Vector2i &p_size, const Color &p_background = Color(1, 1, 1, 1));
    /// Ouvre une image ou un DrawingDocument.
    Error open_file(const String &p_path);
    /// Édite un document déjà chargé (enregistré ensuite en p_path, s'il est donné).
    void edit_document(const Ref<DrawingDocument> &p_document, const String &p_path = String());
    /// Enregistre au chemin courant (sinon ouvre la boîte « Enregistrer sous »).
    Error save();
    /// Enregistre en p_path : image (calques fusionnés) selon l'extension, ou DrawingDocument (.tres / .res).
    Error save_as(const String &p_path);
    /// Affiche un message dans la barre d'état.
    void show_message(const String &p_message);

  protected:
    static void _bind_methods();

  private:
    DrawingCanvas *canvas = nullptr;
    TypedArray<DrawingBrush> brushes;
    TypedArray<DrawingBrush> erasers;
    TypedArray<DrawingBrush> builtin_brushes; // préréglages utilisés quand brushes est vide
    TypedArray<DrawingBrush> builtin_erasers;
    Array grain_textures; // [nom, Texture2D ou null] proposés dans les réglages du pinceau
    bool show_file_buttons = true;
    int file_access = FileDialog::ACCESS_FILESYSTEM;
    String current_path;

    VBoxContainer *root = nullptr;
    HBoxContainer *file_buttons = nullptr;
    Button *undo_button = nullptr;
    Button *redo_button = nullptr;
    Button *flip_button = nullptr;
    Label *zoom_label = nullptr;
    Label *status_label = nullptr;
    Ref<ButtonGroup> tool_group;
    Button *tool_buttons[DrawingCanvas::TOOL_MAX] = {};
    VBoxContainer *tool_options = nullptr;
    HSlider *size_slider = nullptr; // taille du pinceau, resynchronisée si elle change ailleurs ([ ])
    ColorPicker *color_picker = nullptr;
    ColorRect *primary_swatch = nullptr;
    ColorRect *secondary_swatch = nullptr;
    VBoxContainer *layer_list = nullptr;
    LineEdit *layer_name = nullptr;
    OptionButton *layer_blend = nullptr;
    HSlider *layer_opacity = nullptr;
    CheckBox *layer_alpha_lock = nullptr;
    CheckBox *layer_clip = nullptr;
    CheckBox *layer_lock = nullptr;
    Button *layer_remove = nullptr;
    Button *layer_merge = nullptr;
    FileDialog *open_dialog = nullptr;
    FileDialog *save_dialog = nullptr;
    ConfirmationDialog *new_dialog = nullptr;
    SpinBox *new_width = nullptr;
    SpinBox *new_height = nullptr;
    CheckBox *new_transparent = nullptr;
    bool updating = false;
    bool tool_options_dirty = true;
    bool layers_dirty = true;

    void _build();
    void _build_top_bar();
    void _build_left_panel(HBoxContainer *p_parent);
    void _build_right_panel(HBoxContainer *p_parent);
    void _build_dialogs();
    Button *_add_button(Container *p_parent, const String &p_text, const String &p_tooltip, const Callable &p_callback);
    HSlider *_add_slider(Container *p_parent,
                         const String &p_label,
                         double p_min,
                         double p_max,
                         double p_step,
                         double p_value,
                         const Callable &p_callback,
                         bool p_exponential = false,
                         const String &p_suffix = String());
    CheckBox *_add_check(Container *p_parent, const String &p_text, bool p_pressed, const Callable &p_callback);
    OptionButton *_add_options(Container *p_parent, const String &p_label, const PackedStringArray &p_items, int p_selected, const Callable &p_callback);

    void _connect_document();
    void _disconnect_document();
    void _ensure_brushes();
    TypedArray<DrawingBrush> _brush_list(bool p_eraser) const;
    Ref<DrawingBrush> _edited_brush() const;
    void _rebuild_tool_options();
    void _build_brush_options(bool p_eraser);
    void _rebuild_layers();
    void _refresh_history();
    void _refresh_colors();
    void _refresh_zoom();

    // Rappels
    void _on_tool_pressed(int p_tool);
    void _on_tool_changed(int p_tool);
    void _on_document_changed();
    void _on_layers_changed();
    void _on_history_changed();
    void _on_view_changed();
    void _on_primary_color_changed(const Color &p_color);
    void _on_picker_color_changed(const Color &p_color);
    void _on_stroke_rejected();
    void _on_preset_selected(int p_index, bool p_eraser);
    void _on_brush_property(double p_value, const StringName &p_property, double p_scale);
    void _on_brush_flag(bool p_pressed, const StringName &p_property, int p_flag);
    void _on_brush_bool(bool p_pressed, const StringName &p_property);
    void _on_brush_blend(int p_index);
    void _on_grain_selected(int p_index);
    void _on_canvas_property(double p_value, const StringName &p_property, double p_scale);
    void _on_canvas_bool(bool p_pressed, const StringName &p_property);
    void _on_canvas_option(int p_index, const StringName &p_property);
    void _on_layer_selected(int p_index);
    void _on_layer_visibility(bool p_visible, int p_index);
    void _on_layer_name_submitted(const String &p_text);
    void _on_layer_blend(int p_index);
    void _on_layer_opacity(double p_value);
    void _on_layer_flag(bool p_pressed, const StringName &p_property);
    void _on_add_layer();
    void _on_duplicate_layer();
    void _on_remove_layer();
    void _on_move_layer(int p_direction);
    void _on_merge_down();
    void _on_clear_layer();
    void _on_undo();
    void _on_redo();
    void _on_rotate(float p_degrees);
    void _on_flip(bool p_pressed);
    void _on_new_pressed();
    void _on_new_confirmed();
    void _on_open_pressed();
    void _on_save_pressed();
    void _on_save_as_pressed();
    void _on_open_selected(const String &p_path);
    void _on_save_selected(const String &p_path);
};
} // namespace godot

#endif // DRAWING_STUDIO_HPP
