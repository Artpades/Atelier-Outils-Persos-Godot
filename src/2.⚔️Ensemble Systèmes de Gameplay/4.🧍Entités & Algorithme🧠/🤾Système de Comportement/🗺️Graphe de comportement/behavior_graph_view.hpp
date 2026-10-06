#ifndef BEHAVIOR_GRAPH_VIEW_HPP
#define BEHAVIOR_GRAPH_VIEW_HPP

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot
{
/// 🗺️ Vue en graphe (TechTree / MindMap) d'une StateMachine ou d'un BehaviorTree, pour éditer et déboguer.
///
/// La vue dessine un « instantané » (Dictionary renvoyé par get_debug_snapshot() des deux exécuteurs) :
///   - en direct : set_target(nœud) — l'instantané est relu tous les refresh_interval secondes ;
///   - à distance : set_snapshot(instantané) — utilisé par l'éditeur pour un jeu en cours d'exécution.
/// Hiérarchie dessinée en arbre (parent → enfants) ; les transitions de la HFSM en arcs fléchés.
/// Couleurs : état actif (cyan), RUNNING (ambre), SUCCESS (vert), FAILURE (rouge) ; les nœuds non
/// exécutés au dernier tick sont estompés.
///
/// Souris : molette = zoom, glisser le fond (ou clic milieu) = déplacer, clic = sélection,
/// double-clic = activation, clic droit = menu contextuel, pastille +/- = replier une branche.
/// En mode editable : glisser un nœud sur un autre = node_dropped ; Maj + glisser = link_requested ;
/// Suppr = delete_requested. Clavier : F = tout afficher.
/// Les identifiants de nœud sont les chemins relatifs à l'exécuteur ("." pour lui-même).
class BehaviorGraphView : public Control
{
    GDCLASS(BehaviorGraphView, Control)

  public:
    enum LayoutMode
    {
        LAYOUT_TECH_TREE, // racine à gauche, branches vers la droite
        LAYOUT_TOP_DOWN,  // racine en haut, branches vers le bas
        LAYOUT_MIND_MAP,  // racine au centre, branches réparties à gauche et à droite
    };

    BehaviorGraphView();

    void set_target(Node *p_target);
    Node *get_target() const;
    void set_target_path(const NodePath &p_path);
    NodePath get_target_path() const { return target_path; }
    void set_live_update(bool p_live);
    bool is_live_update() const { return live_update; }
    void set_refresh_interval(double p_interval) { refresh_interval = MAX(0.0, p_interval); }
    double get_refresh_interval() const { return refresh_interval; }

    void set_snapshot(const Dictionary &p_snapshot);
    Dictionary get_snapshot() const { return snapshot; }
    /// Relit immédiatement l'instantané de la cible.
    void refresh();
    void clear();

    void set_layout_mode(LayoutMode p_mode);
    LayoutMode get_layout_mode() const { return layout_mode; }
    void set_editable(bool p_editable) { editable = p_editable; }
    bool is_editable() const { return editable; }
    void set_show_transition_labels(bool p_show);
    bool get_show_transition_labels() const { return show_transition_labels; }
    void set_show_history(bool p_show);
    bool get_show_history() const { return show_history; }
    void set_placeholder_text(const String &p_text);
    String get_placeholder_text() const { return placeholder_text; }

    void set_selected_id(const String &p_id);
    String get_selected_id() const { return selected_id; }
    void set_zoom(float p_zoom);
    float get_zoom() const { return zoom; }
    void set_view_offset(const Vector2 &p_offset);
    Vector2 get_view_offset() const { return view_offset; }

    void fit_to_view();
    void center_on(const String &p_id);
    void toggle_collapsed(const String &p_id);
    bool is_collapsed(const String &p_id) const { return collapsed.has(p_id); }
    PackedStringArray get_node_ids() const;
    /// Données d'un nœud de l'instantané (vide si absent).
    Dictionary get_node_data(const String &p_id) const;
    /// Rectangle d'un nœud dans le repère du contrôle (vide s'il est masqué ou absent).
    Rect2 get_node_rect(const String &p_id) const;
    /// Identifiant du nœud sous un point du contrôle ("" s'il n'y en a pas).
    String get_node_at(const Vector2 &p_position) const;

    /// Résout l'exécuteur (StateMachine racine, BehaviorTree ou DialogueSystem) d'un nœud de comportement.
    static Node *resolve_runner(Node *p_node);

    void _gui_input(const Ref<InputEvent> &p_event) override;
    String _get_tooltip(const Vector2 &p_at_position) const override;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    struct ViewNode
    {
        String id;
        String parent;
        String name;
        String class_name;
        String kind;
        String info;
        String edge_label; // texte sur le lien depuis le parent (option de dialogue, vrai / faux...)
        int status = -1;
        bool active = false;
        bool fresh = true;
        bool initial = false;
        Array links;
        int parent_index = -1;
        LocalVector<int> children;
        int depth = 0;
        int side = 1; // carte mentale : 1 à droite de la racine, -1 à gauche
        bool hidden = false;
        float breadth = 0.0f;
        Vector2 size;
        Vector2 position; // coin haut-gauche, repère du graphe
    };

    enum DragMode
    {
        DRAG_NONE,
        DRAG_PAN,
        DRAG_NODE,
        DRAG_LINK,
    };

    // Source
    ObjectID target_id;
    NodePath target_path;
    bool live_update = true;
    double refresh_interval = 0.1;
    double refresh_accumulator = 0.0;

    // Modèle
    Dictionary snapshot;
    LocalVector<ViewNode> nodes;
    HashMap<String, int> index_of;
    String structure_signature;
    Rect2 graph_bounds;
    LocalVector<float> column_right; // décalage de chaque colonne (dispositions horizontales), par profondeur
    LocalVector<float> column_left;  // idem, côté gauche de la carte mentale

    // Affichage
    LayoutMode layout_mode = LAYOUT_TECH_TREE;
    bool editable = false;
    bool show_transition_labels = true;
    bool show_history = true;
    String placeholder_text;
    float zoom = 1.0f;
    Vector2 view_offset; // position écran = position graphe * zoom + view_offset
    bool fit_pending = true;
    String selected_id;
    String hovered_id;
    HashSet<String> collapsed;
    Ref<StyleBoxFlat> box_style;

    // Interaction
    DragMode drag_mode = DRAG_NONE;
    String drag_id;
    Vector2 drag_origin;
    Vector2 drag_mouse;
    bool drag_moved = false;

    void _apply_snapshot(const Dictionary &p_snapshot);
    void _layout();
    void _measure(int p_index, bool p_cross_is_y);
    void _place(int p_index, float p_cross_start, bool p_cross_is_y);
    Vector2 _node_size(const ViewNode &p_node) const;
    Vector2 _out_direction(const ViewNode &p_node) const;
    Vector2 _anchor(const ViewNode &p_node, const Vector2 &p_direction) const;
    Rect2 _screen_rect(const ViewNode &p_node) const;
    Vector2 _to_screen(const Vector2 &p_graph) const { return p_graph * zoom + view_offset; }
    int _node_index_at(const Vector2 &p_position) const;
    bool _fold_button_hit(const ViewNode &p_node, const Vector2 &p_position) const;
    Vector2 _fold_button_center(const ViewNode &p_node) const;
    int _visible_child_count(const ViewNode &p_node) const;

    void _draw_graph();
    void _draw_edge(const ViewNode &p_parent, const ViewNode &p_child);
    void _draw_link(const ViewNode &p_from, const ViewNode &p_to, const Dictionary &p_link, int p_rank);
    void _draw_node(const ViewNode &p_node);
    void _draw_overlay();
    void _draw_bezier(const Vector2 &p_a, const Vector2 &p_b, const Vector2 &p_c, const Vector2 &p_d, const Color &p_color, float p_width);
    void _draw_arrow_head(const Vector2 &p_tip, const Vector2 &p_direction, const Color &p_color, float p_size);

    static Color _kind_color(const String &p_kind);
    static Color _status_color(int p_status);
    static String _status_text(int p_status);
};
} // namespace godot

VARIANT_ENUM_CAST(BehaviorGraphView::LayoutMode);

#endif // BEHAVIOR_GRAPH_VIEW_HPP
