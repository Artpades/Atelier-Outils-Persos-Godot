#ifndef POP_SPAWNER_HPP
#define POP_SPAWNER_HPP

#include "pop_style.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot
{
/// Demande de pop : contenu (texte, image et/ou scène) et options de placement.
struct PopRequest
{
    String text;
    Ref<Texture2D> texture;
    Ref<PackedScene> scene;
    Color color = Color(0, 0, 0, 0); // couleur du texte ; alpha nul : text_color du style
    Variant at;                      // position globale (Vector2 / Vector3), sinon celle de l'ancre
    Vector3 offset;                  // décalage depuis l'ancre (2D : x, y avec y vers le bas)
    Node *container = nullptr;       // parent des pops, sinon choisi automatiquement
    float unit_scale_3d = 0.01f;
    bool screen_space_3d = false;
};

/// 💥 Fait jaillir du texte, des images et/ou des objets (scènes) depuis sa cible : chiffres de dégâts,
/// pièces ramassées, onomatopées, icônes de statut...
///
/// La cible est target, ou à défaut le parent : un Node2D ou un Control (pop 2D), un Node3D (pop 3D, ou pop
/// 2D à sa position à l'écran avec screen_space_3d). Les pops sont ajoutés au conteneur (container, sinon
/// le CanvasLayer de la cible ou la scène courante) : ils survivent à la cible et se libèrent seuls.
/// Mouvement et apparence : PopStyle (style par défaut si aucun).
class PopSpawner : public Node
{
    GDCLASS(PopSpawner, Node)

  public:
    /// Pop combinant texte, image et scène (chacun facultatif). p_color : couleur du texte (alpha nul :
    /// couleur du style). p_at : position globale (Vector2 / Vector3), sinon celle de la cible.
    Node *pop(const String &p_text = String(), const Ref<Texture2D> &p_texture = Ref<Texture2D>(), const Ref<PackedScene> &p_scene = Ref<PackedScene>(),
            const Color &p_color = Color(0, 0, 0, 0), const Variant &p_at = Variant());
    Node *pop_text(const String &p_text, const Color &p_color = Color(0, 0, 0, 0));
    Node *pop_texture(const Ref<Texture2D> &p_texture);
    Node *pop_scene(const Ref<PackedScene> &p_scene);
    /// Nombre signé (« +12 », « -3.5 ») coloré par positive_color / negative_color du style.
    Node *pop_number(double p_value, int p_decimals = 0);
    /// Libère tous les pops encore visibles de ce PopSpawner.
    void clear();
    int get_active_count();
    Array get_active_pops();

    /// Crée un pop pour p_anchor (utilisé aussi par FXPop). Renvoie le pop, ou nullptr si impossible.
    static Node *create_pop(Node *p_anchor, const Ref<PopStyle> &p_style, const PopRequest &p_request);

    void set_style(const Ref<PopStyle> &p_value) { style = p_value; }
    Ref<PopStyle> get_style() const { return style; }
    void set_target(Node *p_target);
    Node *get_target() const;
    void set_container(Node *p_container);
    Node *get_container() const;
    void set_offset(const Vector3 &p_value) { offset = p_value; }
    Vector3 get_offset() const { return offset; }
    void set_max_pops(int p_value) { max_pops = p_value; }
    int get_max_pops() const { return max_pops; }
    void set_unit_scale_3d(float p_value) { unit_scale_3d = p_value; }
    float get_unit_scale_3d() const { return unit_scale_3d; }
    void set_screen_space_3d(bool p_value) { screen_space_3d = p_value; }
    bool get_screen_space_3d() const { return screen_space_3d; }

  protected:
    static void _bind_methods();

  private:
    Ref<PopStyle> style;
    ObjectID target_id;
    ObjectID container_id;
    Vector3 offset;
    int max_pops = 64; // 0 : sans limite (au-delà, les plus anciens disparaissent)
    float unit_scale_3d = 0.01f;
    bool screen_space_3d = false;

    LocalVector<ObjectID> active_pops; // du plus ancien au plus récent

    /// Oublie les pops libérés ou en cours de libération.
    void _prune();
    /// Ajoute sous p_pop la scène, l'image et le texte de la demande (rangée centrée).
    static void _build_content_2d(Node *p_pop, const PopStyle &p_style, const PopRequest &p_request);
    static void _build_content_3d(Node *p_pop, const PopStyle &p_style, const PopRequest &p_request);
};
} // namespace godot

#endif // POP_SPAWNER_HPP
