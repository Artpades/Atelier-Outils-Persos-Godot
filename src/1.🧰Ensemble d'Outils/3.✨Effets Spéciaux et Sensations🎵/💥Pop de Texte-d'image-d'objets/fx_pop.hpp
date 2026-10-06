#ifndef FX_POP_HPP
#define FX_POP_HPP

#include "1.🧰Ensemble d'Outils/3.✨Effets Spéciaux et Sensations🎵/effect.hpp"

#include "pop_style.hpp"

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace godot
{
/// 💥 Effet qui fait jaillir un pop (texte, image et/ou scène) de la cible à chaque jeu, count fois.
/// Même fonctionnement qu'un PopSpawner (voir PopSpawner) : idéal en effet enchaîné (« +1 » à la collecte,
/// « Paf ! » sur une secousse...).
class FXPop : public Effect
{
    GDCLASS(FXPop, Effect)

  public:
    void set_style(const Ref<PopStyle> &p_value) { style = p_value; }
    Ref<PopStyle> get_style() const { return style; }
    void set_text(const String &p_value) { text = p_value; }
    String get_text() const { return text; }
    void set_texture(const Ref<Texture2D> &p_value) { texture = p_value; }
    Ref<Texture2D> get_texture() const { return texture; }
    void set_scene(const Ref<PackedScene> &p_value) { scene = p_value; }
    Ref<PackedScene> get_scene() const { return scene; }
    void set_color(const Color &p_value) { color = p_value; }
    Color get_color() const { return color; }
    void set_count(int p_value) { count = p_value; }
    int get_count() const { return count; }
    void set_offset(const Vector3 &p_value) { offset = p_value; }
    Vector3 get_offset() const { return offset; }
    void set_unit_scale_3d(float p_value) { unit_scale_3d = p_value; }
    float get_unit_scale_3d() const { return unit_scale_3d; }
    void set_screen_space_3d(bool p_value) { screen_space_3d = p_value; }
    bool get_screen_space_3d() const { return screen_space_3d; }

  protected:
    static void _bind_methods();

    void _play(Node *p_target) override;

  private:
    Ref<PopStyle> style;
    String text;
    Ref<Texture2D> texture;
    Ref<PackedScene> scene;
    Color color = Color(0, 0, 0, 0); // alpha nul : couleur du style
    int count = 1;                   // pops créés à chaque jeu (gerbe de pièces...)
    Vector3 offset;
    float unit_scale_3d = 0.01f;
    bool screen_space_3d = false;
};
} // namespace godot

#endif // FX_POP_HPP
