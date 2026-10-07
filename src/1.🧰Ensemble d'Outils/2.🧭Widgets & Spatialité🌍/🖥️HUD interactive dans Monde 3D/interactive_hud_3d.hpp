#ifndef INTERACTIVE_HUD_3D_HPP
#define INTERACTIVE_HUD_3D_HPP

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>

namespace godot
{
/// 🖥️ Interface graphique (Control) affichée sur un panneau dans le monde 3D, utilisable au curseur.
///
/// Le nœud crée lui-même (en enfants internes, non sauvegardés) un SubViewport qui affiche le contenu
/// (content_scene ou set_content) et un quad texturé par ce SubViewport. Le rayon du pointeur (souris, centre
/// de l'écran ou rayon fourni par code) est intersecté avec le quad : le point touché est converti en pixels
/// du SubViewport, où les évènements souris sont renvoyés. Boutons, curseurs, listes, champs de texte...
/// fonctionnent donc comme à l'écran.
///
/// - Plusieurs HUD : seul le plus proche du pointeur reçoit l'évènement (parmi ceux de même source de pointeur).
/// - Un bouton pressé sur le HUD le « capture » : tant qu'il n'est pas relâché, le HUD reçoit le pointeur même
///   en dehors du quad (glisser un curseur au-delà du bord), et le relâchement n'est jamais perdu.
/// - Clavier : renvoyé au HUD après un clic dessus, tant qu'un de ses Control a le focus (champ de texte...).
/// - Les clics passent par _unhandled_input : une interface à l'écran placée au-dessus garde la priorité.
class InteractiveHUD3D : public Node3D
{
    GDCLASS(InteractiveHUD3D, Node3D)

  public:
    enum PointerSource
    {
        POINTER_SOURCE_MOUSE,         // rayon de la caméra passant par le curseur de la souris
        POINTER_SOURCE_SCREEN_CENTER, // rayon passant par le centre de l'écran (souris capturée, viseur)
        POINTER_SOURCE_MANUAL,        // rayon fourni par code (set_pointer_ray : laser, manette, XR...)
    };

    enum BillboardMode
    {
        BILLBOARD_DISABLED, // orienté comme le nœud
        BILLBOARD_ENABLED,  // toujours face à la caméra
        BILLBOARD_Y,        // face à la caméra en tournant seulement autour de l'axe Y
    };

    void _notification(int p_what);
    void _process(double p_delta) override;
    void _input(const Ref<InputEvent> &p_event) override;
    void _unhandled_input(const Ref<InputEvent> &p_event) override;

    // 🧩 Contenu
    void set_content_scene(const Ref<PackedScene> &p_scene);
    Ref<PackedScene> get_content_scene() const { return content_scene; }
    /// Remplace le contenu par p_content (libère l'ancien). Un Control est étiré sur tout le SubViewport.
    void set_content(Node *p_content);
    Node *get_content() const;
    SubViewport *get_sub_viewport() const { return sub_viewport; }
    void set_viewport_size(const Vector2i &p_size);
    Vector2i get_viewport_size() const { return viewport_size; }
    void set_transparent_background(bool p_enabled);
    bool get_transparent_background() const { return transparent_background; }

    // 🎨 Affichage
    void set_pixel_size(float p_size);
    float get_pixel_size() const { return pixel_size; }
    /// Taille du quad en mètres (viewport_size × pixel_size).
    Vector2 get_quad_size() const { return Vector2(viewport_size) * pixel_size; }
    void set_billboard(BillboardMode p_mode);
    BillboardMode get_billboard() const { return billboard; }
    void set_double_sided(bool p_enabled);
    bool get_double_sided() const { return double_sided; }
    void set_unshaded(bool p_enabled);
    bool get_unshaded() const { return unshaded; }
    void set_always_on_top(bool p_enabled);
    bool get_always_on_top() const { return always_on_top; }
    void set_modulate(const Color &p_color);
    Color get_modulate() const { return modulate; }

    // 🖱️ Interaction
    void set_interactive(bool p_enabled);
    bool is_interactive() const { return interactive; }
    void set_pointer_source(PointerSource p_source);
    PointerSource get_pointer_source() const { return pointer_source; }
    void set_max_distance(float p_distance) { max_distance = p_distance; }
    float get_max_distance() const { return max_distance; }
    void set_forward_keyboard(bool p_enabled) { forward_keyboard = p_enabled; }
    bool get_forward_keyboard() const { return forward_keyboard; }
    void set_check_occlusion(bool p_enabled) { check_occlusion = p_enabled; }
    bool get_check_occlusion() const { return check_occlusion; }
    void set_occlusion_mask(uint32_t p_mask) { occlusion_mask = p_mask; }
    uint32_t get_occlusion_mask() const { return occlusion_mask; }

    /// Rayon du pointeur en mode POINTER_SOURCE_MANUAL (coordonnées globales).
    void set_pointer_ray(const Vector3 &p_origin, const Vector3 &p_direction);
    /// Retire le rayon manuel : le pointeur quitte le HUD (le bouton capturé est relâché).
    void clear_pointer_ray();
    /// Appui / relâchement d'un bouton au point visé par le rayon manuel (molette : MOUSE_BUTTON_WHEEL_*).
    void press_pointer(MouseButton p_button = MOUSE_BUTTON_LEFT);
    void release_pointer(MouseButton p_button = MOUSE_BUTTON_LEFT);

    // 🔎 Requêtes
    bool is_hovered() const { return hovered; }
    /// Vrai entre un appui sur le HUD et le relâchement du dernier bouton appuyé.
    bool is_captured() const { return captured_buttons != 0; }
    /// Vrai après un clic sur le HUD, jusqu'à un clic ailleurs : le clavier lui est alors renvoyé.
    bool is_focused() const { return focused; }
    /// Dernière position du pointeur sur le HUD, en pixels du SubViewport (-1, -1 hors du HUD).
    Vector2 get_pointer_position() const { return hovered || is_captured() ? last_pixel : Vector2(-1, -1); }
    /// Pixel du SubViewport touché par un rayon global, ou null (hors du quad, derrière, trop loin).
    Variant ray_to_hud(const Vector3 &p_origin, const Vector3 &p_direction) const;
    /// Pixel du SubViewport sous une position de l'écran (coordonnées du Viewport), ou null.
    Variant screen_to_hud(const Vector2 &p_screen_position) const;
    /// Position globale du point du quad correspondant à un pixel du SubViewport.
    Vector3 hud_to_world(const Vector2 &p_pixel) const;

  protected:
    static void _bind_methods();

  private:
    static constexpr const char *GROUP = "_interactive_hud_3d";

    Ref<PackedScene> content_scene;
    Vector2i viewport_size = Vector2i(512, 320);
    bool transparent_background = true;

    float pixel_size = 0.0025f;
    BillboardMode billboard = BILLBOARD_DISABLED;
    bool double_sided = true;
    bool unshaded = true;
    bool always_on_top = false;
    Color modulate = Color(1, 1, 1, 1);

    bool interactive = true;
    PointerSource pointer_source = POINTER_SOURCE_MOUSE;
    float max_distance = 0.0f; // 0 = illimitée
    bool forward_keyboard = true;
    bool check_occlusion = false;
    uint32_t occlusion_mask = 1;

    SubViewport *sub_viewport = nullptr;
    MeshInstance3D *mesh_instance = nullptr;
    Ref<QuadMesh> quad;
    Ref<StandardMaterial3D> material;

    bool hovered = false;
    bool focused = false;
    BitField<MouseButtonMask> captured_buttons = 0;
    Vector2 last_pixel = Vector2(-1, -1);
    /// Position du pointeur de la souris en coordonnées du Viewport (suivie dans _input).
    Vector2 mouse_position;
    bool has_manual_ray = false;
    Vector3 manual_origin;
    Vector3 manual_direction;

    void _build();
    void _update_mesh();
    void _update_material();
    void _update_billboard();

    /// Rayon actuel du pointeur selon pointer_source (false si pas de caméra ou pas de rayon manuel).
    bool _get_pointer_ray(Vector2 p_screen_position, Vector3 &r_origin, Vector3 &r_direction) const;
    /// Intersection du rayon avec le plan du quad. p_bounded : limitée au quad. Distance mesurée depuis p_origin.
    bool _intersect(const Vector3 &p_origin, const Vector3 &p_direction, bool p_bounded, Vector2 &r_pixel, float &r_distance) const;
    /// Intersection valide pour l'interaction : interactif, visible, à portée, non occulté.
    bool _hit(const Vector3 &p_origin, const Vector3 &p_direction, Vector2 &r_pixel, float &r_distance) const;
    bool _is_occluded(const Vector3 &p_origin, const Vector3 &p_point) const;
    /// Vrai si un autre HUD de même source de pointeur est touché plus près par ce rayon.
    bool _is_behind_another_hud(const Vector3 &p_origin, const Vector3 &p_direction, float p_distance) const;
    bool _can_interact() const;

    /// Met à jour le survol pour ce rayon et renvoie le pixel où renvoyer l'évènement (false : rien à renvoyer).
    bool _resolve_pointer(const Vector3 &p_origin, const Vector3 &p_direction, Vector2 &r_pixel);
    void _set_hovered(bool p_hovered);
    void _set_focused(bool p_focused);
    /// Renvoie au SubViewport une copie de p_event placée en p_pixel.
    void _forward_mouse(const Ref<InputEventMouse> &p_event, const Vector2 &p_pixel);
    void _send_motion(const Vector2 &p_pixel);
    void _send_button(MouseButton p_button, bool p_pressed, const Vector2 &p_pixel);
    /// Traite un appui / relâchement (capture, focus, signaux) avant son renvoi.
    void _on_button(MouseButton p_button, bool p_pressed, const Vector2 &p_pixel);
    void _release_all();
};
} // namespace godot

VARIANT_ENUM_CAST(InteractiveHUD3D::PointerSource);
VARIANT_ENUM_CAST(InteractiveHUD3D::BillboardMode);

#endif // INTERACTIVE_HUD_3D_HPP
