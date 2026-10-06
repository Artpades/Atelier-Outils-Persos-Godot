#ifndef CUSTOM_TOOLTIP_HPP
#define CUSTOM_TOOLTIP_HPP

#include "tooltip_content.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
class CanvasLayer;
class PanelContainer;
class VBoxContainer;
class VideoStreamPlayer;

/// 🗨️ Infobulle riche (titre, icône, vidéo, texte BBCode, scène personnalisée) affichée au survol de la cible.
/// La cible est target, ou à défaut le parent : un Control, ou un CollisionObject2D / 3D (avec
/// input_pickable activé), c'est-à-dire tout nœud qui émet mouse_entered / mouse_exited.
/// L'infobulle est dessinée dans un CanvasLayer interne, au-dessus du jeu, et reste dans l'écran.
class CustomTooltip : public Node
{
    GDCLASS(CustomTooltip, Node)

  public:
    /// Affiche l'infobulle immédiatement (sans attendre delay), à la position de la souris.
    void show_tooltip();
    void hide_tooltip();
    bool is_tooltip_visible() const { return shown; }
    /// Panneau de l'infobulle (créé au premier affichage), pour le personnaliser par script.
    PanelContainer *get_panel() const { return panel; }

    void set_content(const Ref<TooltipContent> &p_content);
    Ref<TooltipContent> get_content() const { return content; }
    void set_target(Node *p_target);
    Node *get_target() const;
    void set_delay(double p_value) { delay = p_value; }
    double get_delay() const { return delay; }
    void set_follow_mouse(bool p_value) { follow_mouse = p_value; }
    bool get_follow_mouse() const { return follow_mouse; }
    void set_offset(const Vector2 &p_value) { offset = p_value; }
    Vector2 get_offset() const { return offset; }
    void set_fade_duration(double p_value) { fade_duration = p_value; }
    double get_fade_duration() const { return fade_duration; }
    void set_panel_style(const Ref<StyleBox> &p_value);
    Ref<StyleBox> get_panel_style() const { return panel_style; }
    void set_tooltip_theme(const Ref<Theme> &p_value);
    Ref<Theme> get_tooltip_theme() const { return tooltip_theme; }
    void set_layer(int p_value);
    int get_layer() const { return layer; }

    void _ready() override;
    void _process(double p_delta) override;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    Ref<TooltipContent> content;
    ObjectID target_id;
    double delay = 0.5;
    bool follow_mouse = true;
    Vector2 offset = Vector2(16, 16);
    double fade_duration = 0.15;
    Ref<StyleBox> panel_style;
    Ref<Theme> tooltip_theme;
    int layer = 128;

    // Interface (enfants internes, créés au premier affichage)
    CanvasLayer *canvas_layer = nullptr;
    PanelContainer *panel = nullptr;
    VBoxContainer *box = nullptr;
    VideoStreamPlayer *video_player = nullptr;
    Ref<Tween> fade_tween;

    ObjectID connected_target_id; // cible dont les signaux sont connectés
    bool hovering = false;
    double hover_time = 0.0;
    bool shown = false;
    bool content_dirty = true;

    Node *_resolve_target() const;
    void _connect_target();
    void _disconnect_target();
    void _on_mouse_entered();
    void _on_mouse_exited();
    void _on_content_changed() { content_dirty = true; }

    void _build_ui();
    void _rebuild_content();
    void _place_at_mouse();
    void _finish_hide();
    static void _ignore_mouse(Node *p_node);
};
} // namespace godot

#endif // CUSTOM_TOOLTIP_HPP
