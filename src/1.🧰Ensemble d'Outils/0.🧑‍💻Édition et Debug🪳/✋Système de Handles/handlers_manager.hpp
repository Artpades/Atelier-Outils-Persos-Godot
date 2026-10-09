#ifndef HANDLERS_MANAGER_HPP
#define HANDLERS_MANAGER_HPP

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/plane.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot
{

/// ✋ HandlersManager : poignées 2D / 3D survolables et déplaçables à la souris (signaux de survol et de déplacement).
// NOTE : classe compilée dans tool_editor mais NON enregistrée (aucun GDREGISTER_CLASS) : inaccessible
// depuis Godot tant que l'enregistrement n'est pas ajouté à tool_editor_register.cpp.
//
// Singleton (via ManagerBuilder) responsable de la détection de survol ET du déplacement
// au curseur (drag) des poignées (handles) 2D et 3D, à chaque frame.
//
// Le Viewport est fourni EXPLICITEMENT à l'enregistrement (comme Gizmos::create_2D_gizmo
// / Gizmos::create_3D_gizmo), et non deviné en interne : un handle peut donc vivre dans
// le viewport du jeu, un des viewports de l'éditeur principal, OU une fenêtre d'éditeur
// personnalisée (WindowEditor) — HandlersManager ne fait aucune hypothèse sur son origine.
//
// Le Viewport est stocké par ObjectID (pas par pointeur brut) : s'il est détruit (ex: une
// WindowEditor fermée) sans que le handle soit désenregistré, le handle est simplement
// ignoré à chaque frame (résolution via UtilityFunctions::instance_from_id) plutôt que de
// déréférencer un pointeur périmé.
//
// - Handle 2D : Rect2 en coordonnées LOCALES à son Viewport, testé avec Rect2::has_point().
// - Handle 3D : AABB en espace MONDE (du World3D de son Viewport), testé avec
//   AABB::intersects_ray() contre un rayon projeté depuis Viewport::get_camera_3d().
//
// Le changement d'état de survol et le déplacement émettent chacun un signal plutôt que
// d'appeler un Callable stocké : la modification de propriété reste la responsabilité d'un
// autre objet qui écoute ces signaux — pas de couplage direct ici.
class HandlersManager : public Node
{
    GDCLASS(HandlersManager, Node)

  private:
    /// État d'une poignée 2D (zone, couleurs, survol / glissement, viewport, gizmo de dessin).
    struct Handle2D
    {
        Rect2 rect;
        Color default_color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        Color hover_color = Color(1.0f, 0.9f, 0.1f, 1.0f);
        bool hovered = false;
        bool dragging = false;
        Vector2 drag_offset; // souris - rect.position au moment du clic, figé pendant le drag
        ObjectID viewport_id;
        RID gizmo_rid; // créé paresseusement au premier _process
    };

    /// État d'une poignée 3D (AABB monde, plan de glissement figé au clic, caméra optionnelle, gizmo).
    struct Handle3D
    {
        AABB aabb;
        Color default_color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        Color hover_color = Color(1.0f, 0.9f, 0.1f, 1.0f);
        bool hovered = false;
        bool dragging = false;
        Plane drag_plane; // figé au moment du clic : passe par le centre de l'AABB, face caméra
        ObjectID viewport_id;
        ObjectID camera_id; // invalide (0) si aucune caméra explicite fournie : fallback sur viewport->get_camera_3d()
        RID gizmo_rid;
    };

    HashMap<StringName, Handle2D> _handles_2d;
    HashMap<StringName, Handle3D> _handles_3d;

    /// Redessine le gizmo de la poignée (rectangle plein / cube fil de fer) avec la couleur de survol ou par défaut.
    void _redraw_handle_2d(Handle2D &p_handle);
    void _redraw_handle_3d(Handle3D &p_handle);

    /// Détection du survol, début / fin du glissement (bouton gauche) et émission des signaux, chaque frame.
    void _process_2d_handles();
    void _process_3d_handles();

    // Détermine si un Viewport est effectivement visible à l'écran (pas juste "existant") :
    // - Window : Window::is_visible().
    // - SubViewport : remonte jusqu'à un ancêtre Control (typiquement un
    //   SubViewportContainer) et se fie à Control::is_visible_in_tree(). Sans ancêtre
    //   Control trouvé, on ne peut pas déterminer l'état : on laisse passer (fail-open)
    //   plutôt que de bloquer un cas d'usage non prévu.
    static bool _is_viewport_effectively_visible(Viewport *p_viewport);

  protected:
    static void _bind_methods();

  public:
    HandlersManager();
    ~HandlersManager();

    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double p_delta) override;

    /// Instance singleton (créée via ManagerBuilder au premier appel).
    static HandlersManager *get_singleton();

    // --- Handles 2D ---
    // p_rect : zone de détection en coordonnées locales à p_viewport.
    // p_viewport : le Viewport dans lequel ce handle vit (jeu, éditeur principal, ou
    // fenêtre d'édition personnalisée type WindowEditor) — utilisé pour créer le gizmo
    // (canvas) et lire la position souris (Viewport::get_mouse_position()).
    void register_handle_2d(
        const StringName &p_id, const Rect2 &p_rect, Viewport *p_viewport, const Color &p_default_color = Color(1.0f, 1.0f, 1.0f, 1.0f), const Color &p_hover_color = Color(1.0f, 0.9f, 0.1f, 1.0f));
    /// Retire la poignée et libère son gizmo.
    void unregister_handle_2d(const StringName &p_id);
    // À appeler chaque frame par l'appelant si le handle suit un objet qui bouge pour une
    // raison EXTÉRIEURE au drag (le HandlersManager ne connaît pas cette logique-là).
    void set_handle_2d_rect(const StringName &p_id, const Rect2 &p_rect);
    bool is_handle_2d_registered(const StringName &p_id) const;
    bool is_handle_2d_hovered(const StringName &p_id) const;
    bool is_handle_2d_dragging(const StringName &p_id) const;
    Rect2 get_handle_2d_rect(const StringName &p_id) const;

    // --- Handles 3D ---
    // p_aabb_world : boîte englobante en espace MONDE (déjà transformée par l'appelant).
    // p_viewport : le Viewport dont le World3D/scenario sera utilisé pour le gizmo.
    // p_camera : caméra à utiliser pour le raycast de détection. Si nullptr (défaut),
    // retombe sur p_viewport->get_camera_3d() à chaque frame. À fournir explicitement si
    // l'auto-détection ne renvoie pas la bonne caméra (ex: caméra de navigation de
    // l'éditeur qui n'est pas forcément récupérable via get_camera_3d() sur le viewport
    // de la scène éditée — à vérifier au cas par cas, pas une certitude générale).
    void register_handle_3d(const StringName &p_id,
                            const AABB &p_aabb_world,
                            Viewport *p_viewport,
                            Camera3D *p_camera = nullptr,
                            const Color &p_default_color = Color(1.0f, 1.0f, 1.0f, 1.0f),
                            const Color &p_hover_color = Color(1.0f, 0.9f, 0.1f, 1.0f));
    /// Retire la poignée et libère son gizmo.
    void unregister_handle_3d(const StringName &p_id);
    /// Remplace la boîte de la poignée (objet déplacé par une autre logique que le glissement).
    void set_handle_3d_aabb(const StringName &p_id, const AABB &p_aabb_world);
    bool is_handle_3d_registered(const StringName &p_id) const;
    bool is_handle_3d_hovered(const StringName &p_id) const;
    bool is_handle_3d_dragging(const StringName &p_id) const;
    AABB get_handle_3d_aabb(const StringName &p_id) const;
};

} // namespace godot

#endif // HANDLERS_MANAGER_HPP