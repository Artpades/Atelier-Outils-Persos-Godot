#ifndef DRAWING_CANVAS_HPP
#define DRAWING_CANVAS_HPP

#include "../📄Document/drawing_document.hpp"
#include "../🖌️Pinceaux/drawing_brush.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/templates/hash_map.hpp>

#include <vector>

namespace godot
{
/// 🖼️ Toile interactive affichant un DrawingDocument et permettant d'y dessiner (souris, stylet, tactile).
///
/// Outils : pinceau, gomme, pot de peinture, pipette, sélections (rectangle, ellipse, lasso, baguette magique), main.
/// Navigation : zoom, déplacement, rotation et miroir horizontal de la vue (sans modifier le dessin).
///
/// Souris / stylet (raccourcis de Clip Studio Paint) :
/// - Molette : zoom au curseur ; Alt + molette : rotation. Clic du milieu ou Espace + glisser : déplacer la vue ;
///   Ctrl + Espace + glisser : zoom ; Maj + Espace + glisser : rotation.
/// - Alt + clic avec le pinceau, la gomme ou le pot : pipette temporaire. Bout gomme du stylet : gomme.
/// - Sélections : Maj ajoute, Alt soustrait, Maj + Alt intersecte. Un clic sans glisser désélectionne.
/// Tactile : un doigt dessine, deux doigts déplacent, zooment et tournent la vue (un trait tout juste commencé est annulé).
/// Clavier (toile survolée ou ayant le focus) : B pinceau, E gomme, G pot, I pipette, M sélection (rectangle /
/// ellipse), L lasso, W baguette, H main, X échange des couleurs, [ ] taille, - = rotation, Ctrl+Z / Ctrl+Y (ou
/// Ctrl+Maj+Z), Ctrl+A, Ctrl+D, Ctrl+Maj+I, Suppr, Ctrl+0 ajuster, Ctrl+1 taille réelle, Ctrl+plus / Ctrl+moins.
///
/// Affichage : l'image composée du document est découpée en tuiles mises à jour seulement là où elle change. Un pinceau
/// à shader (DrawingBrush.material) est affiché à travers son shader pendant le trait, puis rendu par le GPU et appliqué
/// au calque (sans rendu disponible, en headless : appliqué sans le shader).
class DrawingCanvas : public Control
{
    GDCLASS(DrawingCanvas, Control)

  public:
    enum Tool
    {
        TOOL_BRUSH,
        TOOL_ERASER,
        TOOL_FILL,
        TOOL_EYEDROPPER,
        TOOL_SELECT_RECT,
        TOOL_SELECT_ELLIPSE,
        TOOL_SELECT_LASSO,
        TOOL_SELECT_WAND,
        TOOL_PAN,
        TOOL_MAX,
    };

    DrawingCanvas();
    ~DrawingCanvas();

    void _notification(int p_what);
    void _gui_input(const Ref<InputEvent> &p_event) override;
    void _unhandled_key_input(const Ref<InputEvent> &p_event) override;

    // 📄 Document
    void set_document(const Ref<DrawingDocument> &p_document);
    Ref<DrawingDocument> get_document() const { return document; }
    /// Taille du document créé au démarrage du jeu si aucun n'est assigné.
    void set_default_document_size(const Vector2i &p_size) { default_document_size = p_size.max(Vector2i(1, 1)); }
    Vector2i get_default_document_size() const { return default_document_size; }

    // 🛠️ Outils
    void set_tool(Tool p_tool);
    Tool get_tool() const { return tool; }
    void set_brush(const Ref<DrawingBrush> &p_brush);
    Ref<DrawingBrush> get_brush() const { return brush; }
    void set_eraser(const Ref<DrawingBrush> &p_eraser);
    Ref<DrawingBrush> get_eraser() const { return eraser; }
    /// Pinceau de l'outil courant (la gomme avec TOOL_ERASER, sinon le pinceau).
    Ref<DrawingBrush> get_active_brush() const;
    void set_primary_color(const Color &p_color);
    Color get_primary_color() const { return primary_color; }
    void set_secondary_color(const Color &p_color);
    Color get_secondary_color() const { return secondary_color; }
    void swap_colors();

    void set_fill_tolerance(float p_tolerance) { fill_tolerance = Math::clamp(p_tolerance, 0.0f, 1.0f); }
    float get_fill_tolerance() const { return fill_tolerance; }
    void set_fill_gap_closing(int p_pixels) { fill_gap_closing = MAX(p_pixels, 0); }
    int get_fill_gap_closing() const { return fill_gap_closing; }
    void set_fill_expand(int p_pixels) { fill_expand = p_pixels; }
    int get_fill_expand() const { return fill_expand; }
    void set_fill_reference(DrawingDocument::FillReference p_reference) { fill_reference = p_reference; }
    DrawingDocument::FillReference get_fill_reference() const { return fill_reference; }
    void set_fill_contiguous(bool p_contiguous) { fill_contiguous = p_contiguous; }
    bool get_fill_contiguous() const { return fill_contiguous; }
    void set_selection_mode(DrawingDocument::SelectionMode p_mode) { selection_mode = p_mode; }
    DrawingDocument::SelectionMode get_selection_mode() const { return selection_mode; }
    void set_wand_tolerance(float p_tolerance) { wand_tolerance = Math::clamp(p_tolerance, 0.0f, 1.0f); }
    float get_wand_tolerance() const { return wand_tolerance; }
    void set_eyedropper_all_layers(bool p_all) { eyedropper_all_layers = p_all; }
    bool get_eyedropper_all_layers() const { return eyedropper_all_layers; }

    // 🧭 Vue
    void set_zoom(float p_zoom);
    float get_zoom() const { return zoom; }
    void set_min_zoom(float p_zoom) { min_zoom = CLAMP(p_zoom, 0.001f, max_zoom); }
    float get_min_zoom() const { return min_zoom; }
    void set_max_zoom(float p_zoom) { max_zoom = MAX(p_zoom, min_zoom); }
    float get_max_zoom() const { return max_zoom; }
    /// Décalage (pixels de l'écran) du centre du document par rapport au centre de la toile.
    void set_view_offset(const Vector2 &p_offset);
    Vector2 get_view_offset() const { return view_offset; }
    void set_view_rotation(float p_degrees);
    float get_view_rotation() const { return view_rotation; }
    void set_flip_h(bool p_flip);
    bool get_flip_h() const { return flip_h; }
    /// Zoome d'un facteur en gardant fixe le point p_screen_position (coordonnées locales de la toile).
    void zoom_at(float p_factor, const Vector2 &p_screen_position);
    /// Tourne la vue en gardant fixe le point p_screen_position.
    void rotate_at(float p_degrees, const Vector2 &p_screen_position);
    /// Document entier visible, centré (la rotation est conservée). Le reste au fil des redimensionnements de la
    /// toile, jusqu'au prochain zoom ou déplacement manuel.
    void fit_to_view();
    /// Zoom 100 %, document centré.
    void actual_size();
    /// Rotation nulle, sans miroir.
    void reset_rotation();
    Transform2D get_view_transform() const;
    Vector2 canvas_to_screen(const Vector2 &p_canvas_position) const;
    Vector2 screen_to_canvas(const Vector2 &p_screen_position) const;

    // ⌨️ Entrées
    void set_shortcuts_enabled(bool p_enabled) { shortcuts_enabled = p_enabled; }
    bool get_shortcuts_enabled() const { return shortcuts_enabled; }
    void set_touch_gestures(bool p_enabled) { touch_gestures = p_enabled; }
    bool get_touch_gestures() const { return touch_gestures; }
    void set_touch_rotation(bool p_enabled) { touch_rotation = p_enabled; }
    bool get_touch_rotation() const { return touch_rotation; }
    void set_pen_eraser(bool p_enabled) { pen_eraser = p_enabled; }
    bool get_pen_eraser() const { return pen_eraser; }

    // 🎨 Affichage
    void set_view_background(const Color &p_color);
    Color get_view_background() const { return view_background; }
    void set_checkerboard(bool p_enabled);
    bool get_checkerboard() const { return checkerboard; }
    void set_show_brush_cursor(bool p_enabled);
    bool get_show_brush_cursor() const { return show_brush_cursor; }

    // 🔎 État
    /// Vrai pendant un trait, une sélection, un glisser de la vue...
    bool is_dragging() const { return drag != DRAG_NONE; }
    bool is_stroking() const { return drag == DRAG_STROKE; }
    /// Position du pointeur sur le dessin (pixels), ou (-1, -1) hors de la toile.
    Vector2 get_pointer_position() const;

  protected:
    static void _bind_methods();

  private:
    enum DragMode
    {
        DRAG_NONE,
        DRAG_STROKE,
        DRAG_PAN,
        DRAG_ZOOM,
        DRAG_ROTATE,
        DRAG_EYEDROPPER,
        DRAG_SELECT,
        DRAG_LASSO,
        DRAG_GESTURE,
    };

    struct Sample
    {
        Vector2 position; // pixels du dessin
        float pressure = 1.0f;
        float velocity = 0.0f;
    };

    static constexpr int TILE_SIZE = 256;

    Ref<DrawingDocument> document;
    Vector2i default_document_size = Vector2i(1280, 720);

    Tool tool = TOOL_BRUSH;
    Ref<DrawingBrush> brush;
    Ref<DrawingBrush> eraser;
    Ref<DrawingBrush> fallback_brush;  // pinceau sans brush assigné (Plume G)
    Ref<DrawingBrush> fallback_eraser; // gomme sans eraser assignée (gomme dure)
    Color primary_color = Color(0, 0, 0, 1);
    Color secondary_color = Color(1, 1, 1, 1);
    float fill_tolerance = 0.1f;
    int fill_gap_closing = 0;
    int fill_expand = 1;
    DrawingDocument::FillReference fill_reference = DrawingDocument::FILL_REFERENCE_ALL_LAYERS;
    bool fill_contiguous = true;
    DrawingDocument::SelectionMode selection_mode = DrawingDocument::SELECTION_REPLACE;
    float wand_tolerance = 0.1f;
    bool eyedropper_all_layers = true;

    float zoom = 1.0f;
    float min_zoom = 0.02f;
    float max_zoom = 64.0f;
    Vector2 view_offset;
    float view_rotation = 0.0f;
    bool flip_h = false;
    bool auto_fit = true; // ajusté à chaque redimensionnement, jusqu'à une navigation manuelle

    bool shortcuts_enabled = true;
    bool touch_gestures = true;
    bool touch_rotation = true;
    bool pen_eraser = true;

    Color view_background = Color(0.24f, 0.24f, 0.27f);
    bool checkerboard = true;
    bool show_brush_cursor = true;

    // Rendu
    RID view_item;      // transformation de la vue (parent des suivants)
    RID tiles_item;     // tuiles de l'image composée
    RID stroke_item;    // trait différé (pinceau à shader)
    RID selection_item; // contour animé de la sélection
    RID overlay_item;   // repères à l'écran (curseur du pinceau, sélection en cours)
    Ref<ImageTexture> checker_texture;
    Ref<ShaderMaterial> selection_material;
    Ref<ImageTexture> selection_texture;
    Vector2i tile_grid;
    std::vector<Ref<ImageTexture>> tiles;
    std::vector<bool> tiles_dirty;
    bool any_tile_dirty = false;
    std::vector<Ref<ImageTexture>> stroke_tiles;
    std::vector<bool> stroke_tiles_dirty;
    bool any_stroke_tile_dirty = false;
    bool selection_dirty = false;
    bool overlay_dirty = true;

    // Validation GPU d'un trait à shader
    SubViewport *commit_viewport = nullptr;
    RID commit_item;
    Vector2i commit_position;
    int commit_frames = 0;

    // Entrées
    DragMode drag = DRAG_NONE;
    Tool drag_tool = TOOL_BRUSH; // outil effectif du glisser (peut différer de tool : pipette Alt, gomme du stylet)
    Vector2 drag_start;          // écran
    Vector2 drag_last;           // écran
    float drag_start_zoom = 1.0f;
    float drag_start_rotation = 0.0f;
    Vector2 drag_anchor; // dessin
    DrawingDocument::SelectionMode drag_selection_mode = DrawingDocument::SELECTION_REPLACE;
    PackedVector2Array lasso_points; // dessin
    Ref<DrawingBrush> stroke_brush;
    std::vector<Sample> stabilizer;
    bool stroke_waiting_first = false;
    uint64_t stroke_start_msec = 0;
    uint64_t last_motion_usec = 0;
    float velocity = 0.0f;
    float last_pressure = 1.0f;
    bool last_pen_inverted = false;
    Vector2 hover_position = Vector2(-1, -1);
    bool hovering = false;
    HashMap<int32_t, Vector2> touches;
    int32_t touch_drawing_index = -1;
    bool touch_locked = false; // après un geste à deux doigts, jusqu'au retrait de tous les doigts
    float gesture_start_distance = 1.0f;
    float gesture_start_angle = 0.0f;
    bool mouse_select_toggle_ellipse = false;

    // Document
    void _connect_document();
    void _disconnect_document();
    void _ensure_document();
    void _on_region_changed(const Rect2i &p_rect);
    void _on_stroke_changed(const Rect2i &p_rect);
    void _on_document_structure_changed();
    void _on_selection_changed();

    // Rendu
    void _create_items();
    void _free_items();
    void _rebuild_tiles();
    void _update_tiles();
    void _update_stroke_tiles();
    void _rebuild_stroke_commands();
    void _clear_stroke_tiles();
    void _update_selection();
    void _update_view();
    void _fit();
    void _update_overlay();
    void _update_cursor_shape();
    void _draw_background();
    Rect2i _tile_rect(int p_index) const;
    bool _can_render_shaders() const;

    // Traits
    Ref<DrawingBrush> _brush_for(Tool p_tool);
    bool _begin_stroke(bool p_eraser);
    void _stroke_input(const Vector2 &p_screen, float p_pressure, float p_velocity);
    void _end_stroke();
    void _cancel_stroke();
    void _push_sample(const Sample &p_sample);
    void _commit_deferred_stroke();
    void _on_commit_rendered();
    void _finish_commit(const Ref<Image> &p_image);

    // Entrées
    void _press(const Vector2 &p_screen, bool p_from_touch);
    void _motion(const Vector2 &p_screen, float p_pressure, float p_velocity);
    void _release(const Vector2 &p_screen);
    void _pick(const Vector2 &p_screen);
    void _handle_touch(const Ref<InputEvent> &p_event);
    bool _handle_shortcut(const Ref<InputEventKey> &p_key);
    void _anchor(const Vector2 &p_canvas, const Vector2 &p_screen);
    float _compute_velocity(const Vector2 &p_screen, const Vector2 &p_event_velocity);
    DrawingDocument::SelectionMode _modifier_selection_mode() const;
};
} // namespace godot

VARIANT_ENUM_CAST(DrawingCanvas::Tool);

#endif // DRAWING_CANVAS_HPP
