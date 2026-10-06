#ifndef FLOATING_DOCK_HPP
#define FLOATING_DOCK_HPP

#include <godot_cpp/classes/container.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
/// 🛟 Dock à grossissement (façon dock macOS) : ses enfants Control sont alignés, et ceux proches du
/// pointeur grossissent et s'écartent, l'élément survolé étant entouré d'un espace supplémentaire.
/// Fonctionne à la souris et au clavier / à la manette (grossissement autour de l'élément qui a le focus).
/// Les éléments agrandis débordent du dock du côté opposé à item_anchor (vers le haut pour un dock du bas).
class FloatingDock : public Container
{
    GDCLASS(FloatingDock, Container)

  public:
    enum ItemAnchor
    {
        ANCHOR_BEGIN,  // éléments collés au bord haut (ou gauche), grossissent vers le bas (ou la droite)
        ANCHOR_CENTER, // grossissent des deux côtés
        ANCHOR_END,    // éléments collés au bord bas (ou droit), grossissent vers le haut (ou la gauche)
    };
    enum ResizeMode
    {
        RESIZE_SIZE,  // agrandit la taille des éléments (icônes TextureRect en expand, boutons...)
        RESIZE_SCALE, // agrandit leur échelle (le contenu, texte compris, est mis à l'échelle)
    };

    /// Élément le plus proche du pointeur (nul si le dock n'est pas survolé).
    Control *get_hovered_item() const;
    /// Influence actuelle du pointeur sur l'élément p_index (0 : taille normale, 1 : grossissement maximal).
    float get_item_influence(int p_index) const;

    void set_vertical(bool p_value);
    bool is_vertical() const { return vertical; }
    void set_item_size(const Vector2 &p_value);
    Vector2 get_item_size() const { return item_size; }
    void set_separation(float p_value);
    float get_separation() const { return separation; }
    void set_magnification(float p_value);
    float get_magnification() const { return magnification; }
    void set_influence_radius(float p_value) { influence_radius = p_value; }
    float get_influence_radius() const { return influence_radius; }
    void set_hover_spacing(float p_value);
    float get_hover_spacing() const { return hover_spacing; }
    void set_hover_lift(float p_value);
    float get_hover_lift() const { return hover_lift; }
    void set_item_anchor(ItemAnchor p_value);
    ItemAnchor get_item_anchor() const { return item_anchor; }
    void set_resize_mode(ResizeMode p_value);
    ResizeMode get_resize_mode() const { return resize_mode; }
    void set_smoothing(float p_value) { smoothing = p_value; }
    float get_smoothing() const { return smoothing; }
    void set_hover_margin(float p_value) { hover_margin = p_value; }
    float get_hover_margin() const { return hover_margin; }
    void set_follow_focus(bool p_value) { follow_focus = p_value; }
    bool get_follow_focus() const { return follow_focus; }

    Vector2 _get_minimum_size() const override;
    void _process(double p_delta) override;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    bool vertical = false;
    Vector2 item_size = Vector2(64, 64); // (0, 0) : taille minimale de chaque élément
    float separation = 8.0f;
    float magnification = 1.8f;     // échelle de l'élément sous le pointeur
    float influence_radius = 160.0f; // distance (le long du dock) à laquelle le grossissement s'annule
    float hover_spacing = 12.0f;     // espace ajouté de chaque côté d'un élément, selon son influence
    float hover_lift = 0.0f;         // décalage des éléments influencés, loin de leur bord
    ItemAnchor item_anchor = ANCHOR_END;
    ResizeMode resize_mode = RESIZE_SIZE;
    float smoothing = 12.0f;   // vitesse d'animation (0 : immédiat)
    float hover_margin = 16.0f; // marge de la zone de survol autour du dock et des éléments
    bool follow_focus = true;

    LocalVector<float> influences; // par élément (enfants Control visibles, dans l'ordre)
    LocalVector<Rect2> item_rects; // rectangles placés au dernier agencement
    ObjectID hovered_item_id;

    void _get_items(LocalVector<Control *> &r_items) const;
    Vector2 _get_base_size(Control *p_item) const;
    /// Centres des éléments (le long du dock) à leur taille normale.
    void _get_rest_centers(const LocalVector<Control *> &p_items, LocalVector<float> &r_centers) const;
    /// Position du pointeur le long du dock : souris au-dessus du dock, sinon élément qui a le focus.
    bool _get_pointer(const LocalVector<Control *> &p_items, const LocalVector<float> &p_rest_centers, float &r_pointer) const;
    void _layout();
    void _relayout() { queue_sort(); update_minimum_size(); }
};
} // namespace godot

VARIANT_ENUM_CAST(FloatingDock::ItemAnchor);
VARIANT_ENUM_CAST(FloatingDock::ResizeMode);

#endif // FLOATING_DOCK_HPP
