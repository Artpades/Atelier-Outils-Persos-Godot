#ifndef DRAWING_DOCUMENT_HPP
#define DRAWING_DOCUMENT_HPP

#include "../🖌️Pinceaux/drawing_brush.hpp"
#include "drawing_layer.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <vector>

namespace godot
{
/// 📄 Dessin matriciel multi-calques : le moteur du système de dessin, utilisable sans interface (scripts, tests).
///
/// - Calques (DrawingLayer, index 0 = le plus bas) : ajout, suppression, duplication, déplacement, fusion.
/// - Traits au pinceau (begin_stroke / stroke_to / end_stroke) appliqués au calque courant, limités à la sélection.
/// - Remplissage (pot de peinture) : tolérance, référence (calque ou tous les calques), fermeture des trous,
///   extension de la zone.
/// - Sélection (masque 0-255) : rectangle, ellipse, polygone (lasso), par couleur (baguette magique), modes
///   remplacer / ajouter / soustraire / intersection.
/// - Historique : chaque opération est annulable (undo / redo), jusqu'à history_limit étapes.
/// - Image composée (calques fusionnés, sans le fond) tenue à jour : region_changed signale les zones modifiées.
///
/// La ressource se sauvegarde (.tres / .res) avec ses calques ; l'historique et la sélection ne le sont pas.
class DrawingDocument : public Resource
{
    GDCLASS(DrawingDocument, Resource)

  public:
    enum SelectionMode
    {
        SELECTION_REPLACE,
        SELECTION_ADD,
        SELECTION_SUBTRACT,
        SELECTION_INTERSECT,
    };

    enum FillReference
    {
        FILL_REFERENCE_LAYER,      // couleurs du calque courant
        FILL_REFERENCE_ALL_LAYERS, // couleurs de l'image composée (« Tous les calques » de CSP)
    };

    DrawingDocument();

    /// Document de la taille donnée, avec un calque vide.
    static Ref<DrawingDocument> create(const Vector2i &p_size, const Color &p_background = Color(1, 1, 1, 1));
    /// Document à la taille de l'image, l'image formant son unique calque (fond transparent).
    static Ref<DrawingDocument> create_from_image(const Ref<Image> &p_image);

    // 📐 Document
    void set_size(const Vector2i &p_size);
    Vector2i get_size() const { return size; }
    void set_background_color(const Color &p_color);
    Color get_background_color() const { return background_color; }
    void set_history_limit(int p_limit);
    int get_history_limit() const { return history_limit; }

    // 🗂️ Calques
    void set_layers(const TypedArray<DrawingLayer> &p_layers);
    TypedArray<DrawingLayer> get_layers() const;
    int get_layer_count() const { return int(layers.size()); }
    Ref<DrawingLayer> get_layer(int p_index) const;
    int find_layer(const Ref<DrawingLayer> &p_layer) const;
    void set_current_layer(int p_index);
    int get_current_layer() const { return current_layer; }
    Ref<DrawingLayer> get_current() const { return get_layer(current_layer); }
    /// Nouveau calque vide inséré en p_index (-1 : juste au-dessus du calque courant), qui devient le calque courant.
    Ref<DrawingLayer> add_layer(const String &p_name = String(), int p_index = -1);
    /// Insère un calque existant (recadré à la taille du document).
    void insert_layer(const Ref<DrawingLayer> &p_layer, int p_index = -1);
    void remove_layer(int p_index);
    Ref<DrawingLayer> duplicate_layer(int p_index);
    void move_layer(int p_from, int p_to);
    /// Fusionne le calque p_index dans celui du dessous (selon son mode de fusion et son opacité).
    bool merge_down(int p_index);
    /// Efface tout le calque.
    void clear_layer(int p_index);
    /// Modifie un réglage d'un calque de façon annulable (les changements successifs d'un même réglage, en moins
    /// d'une seconde, forment une seule étape : glisser un curseur d'opacité).
    void set_layer_property(int p_index, const StringName &p_property, const Variant &p_value);

    // 🖌️ Traits
    /// Commence un trait sur le calque courant. Faux si le calque est absent, masqué ou verrouillé.
    /// p_deferred_commit : le calque n'est pas modifié pendant le trait (aperçu par get_stroke_preview) ; le trait est
    /// appliqué par commit_stroke_image (rendu d'un shader) ou end_stroke.
    bool begin_stroke(const Ref<DrawingBrush> &p_brush, const Color &p_color, bool p_deferred_commit = false);
    /// Prolonge le trait jusqu'à p_position (pixels du dessin), avec la pression (0-1) et la vitesse (pixels/s).
    void stroke_to(const Vector2 &p_position, float p_pressure = 1.0f, float p_velocity = 0.0f);
    void end_stroke();
    /// Annule le trait en cours (le calque retrouve son état d'avant le trait, rien dans l'historique).
    void cancel_stroke();
    bool is_stroking() const { return stroke.active; }
    bool is_stroke_deferred() const { return stroke.active && stroke.deferred; }
    Ref<DrawingBrush> get_stroke_brush() const { return stroke.brush; }
    /// Zone touchée par le trait en cours.
    Rect2i get_stroke_rect() const { return stroke.rect; }
    /// Trait en cours sous forme d'image RGBA8 (couleur du pinceau, alpha = couverture × sélection) sur p_rect.
    Ref<Image> get_stroke_preview(const Rect2i &p_rect) const;
    /// Termine un trait différé en appliquant p_image (RGBA8, alpha non prémultiplié) placée en p_position.
    void commit_stroke_image(const Ref<Image> &p_image, const Vector2i &p_position);
    /// Trace un segment avec p_brush en un seul trait (pression constante).
    void draw_line(const Vector2 &p_from, const Vector2 &p_to, const Ref<DrawingBrush> &p_brush, const Color &p_color, float p_pressure = 1.0f);

    // 🪣 Remplissage
    /// Pot de peinture en p_position sur le calque courant. Faux si rien n'a été rempli.
    /// p_tolerance (0-1) : écart de couleur accepté. p_gap_closing (pixels) : traverse sans fuir les trous des traits
    /// plus étroits que cette valeur. p_expand (pixels) : agrandit (ou rétrécit si négatif) la zone remplie.
    bool fill(const Vector2i &p_position,
              const Color &p_color,
              float p_tolerance = 0.0f,
              FillReference p_reference = FILL_REFERENCE_LAYER,
              int p_gap_closing = 0,
              int p_expand = 0,
              bool p_contiguous = true);
    /// Remplit la sélection (ou tout le calque courant) avec p_color.
    void fill_selection(const Color &p_color);
    /// Efface le contenu de la sélection (ou tout le calque courant).
    void clear_selection_content();

    // 💧 Pipette
    /// Couleur affichée en p_position (tous les calques, sur le fond) ou couleur du calque courant.
    Color pick_color(const Vector2i &p_position, bool p_all_layers = true) const;

    // ⬚ Sélection
    bool has_selection() const { return selection_active; }
    void select_rect(const Rect2i &p_rect, SelectionMode p_mode = SELECTION_REPLACE);
    void select_ellipse(const Rect2i &p_rect, SelectionMode p_mode = SELECTION_REPLACE);
    /// Polygone (lasso) en pixels du dessin, règle pair-impair.
    void select_polygon(const PackedVector2Array &p_points, SelectionMode p_mode = SELECTION_REPLACE);
    /// Baguette magique.
    void select_by_color(const Vector2i &p_position, float p_tolerance = 0.0f, FillReference p_reference = FILL_REFERENCE_LAYER, bool p_contiguous = true, SelectionMode p_mode = SELECTION_REPLACE);
    void select_all();
    void deselect();
    void invert_selection();
    /// Valeur du masque (0-1) ; 1 partout sans sélection.
    float get_selection_value(int p_x, int p_y) const;
    /// Masque (FORMAT_L8), ou null sans sélection.
    Ref<Image> get_selection_image() const;
    Rect2i get_selection_bounds() const;
    const PackedByteArray &get_selection_data() const { return selection; }

    // 🖼️ Image composée
    /// Calques fusionnés (RGBA8), sur le fond si p_with_background.
    Ref<Image> get_composite_image(bool p_with_background = true) const;
    Error export_png(const String &p_path, bool p_with_background = true) const;
    /// Recompose toute l'image (après une modification directe des pixels d'un calque, par exemple).
    void refresh_composite();
    /// Composition des calques (sans le fond) : 4 octets par pixel. Accès C++ (affichage par DrawingCanvas).
    const uint8_t *get_composite_ptr() const;

    // ↩️ Historique
    bool undo();
    bool redo();
    bool can_undo() const { return !undo_stack.empty(); }
    bool can_redo() const { return !redo_stack.empty(); }
    String get_undo_name() const { return undo_stack.empty() ? String() : undo_stack.back().name; }
    String get_redo_name() const { return redo_stack.empty() ? String() : redo_stack.back().name; }
    int get_undo_count() const { return int(undo_stack.size()); }
    int get_redo_count() const { return int(redo_stack.size()); }
    void clear_history();

  protected:
    static void _bind_methods();

  private:
    struct HistoryOp
    {
        enum Type
        {
            PIXELS,         // layer, rect, before, after
            SELECTION,      // before, after, had_before, has_after
            LAYER_INSERT,   // layer, index
            LAYER_REMOVE,   // layer, index
            LAYER_MOVE,     // index (de), index_to (vers)
            LAYER_PROPERTY, // layer, property, value_before, value_after
        };
        Type type = PIXELS;
        Ref<DrawingLayer> layer;
        Rect2i rect;
        PackedByteArray before;
        PackedByteArray after;
        bool had_before = false;
        bool has_after = false;
        int index = 0;
        int index_to = 0;
        StringName property;
        Variant value_before;
        Variant value_after;
    };

    struct HistoryEntry
    {
        String name;
        std::vector<HistoryOp> ops;
        uint64_t time_msec = 0;
    };

    struct StrokeState
    {
        bool active = false;
        bool deferred = false;
        Ref<DrawingBrush> brush;
        Color color;
        Ref<DrawingLayer> layer;
        PackedByteArray original; // pixels du calque au début du trait (copie partagée)
        Rect2i rect;
        bool has_last = false;
        Vector2 last_position;
        float last_pressure = 1.0f;
        float last_velocity = 0.0f;
        float distance_to_next = 0.0f;
        float direction = 0.0f;
    };

    Vector2i size = Vector2i(1024, 768);
    Color background_color = Color(1, 1, 1, 1);
    int history_limit = 64;
    std::vector<Ref<DrawingLayer>> layers;
    int current_layer = 0;

    PackedByteArray selection; // 1 octet par pixel (0-255), valable si selection_active
    bool selection_active = false;

    mutable PackedByteArray composite; // calques fusionnés, sans le fond
    mutable bool composite_valid = false;

    StrokeState stroke;
    std::vector<float> stroke_mask; // couverture du trait en cours (0-1), nulle hors de stroke.rect

    std::vector<HistoryEntry> undo_stack;
    std::vector<HistoryEntry> redo_stack;
    int layer_counter = 0;

    Rect2i _bounds() const { return Rect2i(Vector2i(), size); }
    String _next_layer_name();
    void _connect_layer(const Ref<DrawingLayer> &p_layer);
    void _disconnect_layer(const Ref<DrawingLayer> &p_layer);
    void _on_layer_changed();
    void _layers_modified(); // recomposition complète + signal layers_changed

    void _ensure_composite() const;
    void _compose(const Rect2i &p_rect) const;
    void _update_composite(const Rect2i &p_rect);

    // Traits
    float _dab_step(float p_pressure, float p_velocity) const;
    void _dab(const Vector2 &p_center, float p_pressure, float p_velocity, Rect2i &r_rect);
    void _apply_stroke(const Rect2i &p_rect);
    void _finish_stroke(const Rect2i &p_rect);
    void _reset_stroke();

    // Régions
    /// Pixels (0/1) semblables à celui de p_seed dans l'image de référence, contigus ou non, trous fermés.
    std::vector<uint8_t> _region_by_color(const Vector2i &p_seed, float p_tolerance, FillReference p_reference, bool p_contiguous, int p_gap_closing);
    /// Applique une couleur sur p_layer là où p_coverage (0-255, ou tout le document si vide) × sélection.
    void _paint_coverage(const Ref<DrawingLayer> &p_layer, const std::vector<uint8_t> &p_coverage, const Rect2i &p_rect, const Color &p_color, DrawingBrush::BrushBlend p_mode, const String &p_name);
    void _set_selection(const std::vector<uint8_t> &p_shape, SelectionMode p_mode, const String &p_name);

    // Historique
    void _push_entry(HistoryEntry &&p_entry);
    void _apply_op(const HistoryOp &p_op, bool p_undo);
    HistoryOp _pixels_op(const Ref<DrawingLayer> &p_layer, const Rect2i &p_rect, const PackedByteArray &p_before) const;
};
} // namespace godot

VARIANT_ENUM_CAST(DrawingDocument::SelectionMode);
VARIANT_ENUM_CAST(DrawingDocument::FillReference);

#endif // DRAWING_DOCUMENT_HPP
