#ifndef CURSOR_STYLE_HPP
#define CURSOR_STYLE_HPP

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// 👆 Apparence d'un curseur : une image, ou plusieurs images jouées en boucle (curseur animé).
/// Les images d'un curseur matériel sont limitées à 256 × 256 pixels.
class CursorStyle : public Resource
{
    GDCLASS(CursorStyle, Resource)

  public:
    /// Image à afficher p_time secondes après l'activation du style (nulle si aucune image).
    Ref<Texture2D> get_frame_at(double p_time) const;

    void set_frames(const TypedArray<Texture2D> &p_value) { frames = p_value; }
    TypedArray<Texture2D> get_frames() const { return frames; }
    void set_frames_per_second(double p_value) { frames_per_second = p_value; }
    double get_frames_per_second() const { return frames_per_second; }
    void set_hotspot(const Vector2 &p_value) { hotspot = p_value; }
    Vector2 get_hotspot() const { return hotspot; }
    void set_shape(Input::CursorShape p_value) { shape = p_value; }
    Input::CursorShape get_shape() const { return shape; }

  protected:
    static void _bind_methods();

  private:
    TypedArray<Texture2D> frames;
    double frames_per_second = 10.0;
    Vector2 hotspot; // point actif de l'image (pointe de la flèche...), en pixels
    Input::CursorShape shape = Input::CURSOR_ARROW; // forme du curseur système remplacée
};
} // namespace godot

#endif // CURSOR_STYLE_HPP
