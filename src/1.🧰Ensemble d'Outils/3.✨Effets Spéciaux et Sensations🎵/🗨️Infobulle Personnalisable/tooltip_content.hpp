#ifndef TOOLTIP_CONTENT_HPP
#define TOOLTIP_CONTENT_HPP

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/video_stream.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot
{
/// 🗨️ Contenu d'une infobulle (CustomTooltip), affiché de haut en bas :
/// titre, icône, vidéo, texte BBCode, puis une scène personnalisée. Les éléments vides sont omis.
class TooltipContent : public Resource
{
    GDCLASS(TooltipContent, Resource)

  public:
    void set_title(const String &p_value);
    String get_title() const { return title; }
    void set_text(const String &p_value);
    String get_text() const { return text; }
    void set_icon(const Ref<Texture2D> &p_value);
    Ref<Texture2D> get_icon() const { return icon; }
    void set_icon_max_size(const Vector2 &p_value);
    Vector2 get_icon_max_size() const { return icon_max_size; }
    void set_video(const Ref<VideoStream> &p_value);
    Ref<VideoStream> get_video() const { return video; }
    void set_video_size(const Vector2 &p_value);
    Vector2 get_video_size() const { return video_size; }
    void set_video_loop(bool p_value);
    bool get_video_loop() const { return video_loop; }
    void set_video_muted(bool p_value);
    bool get_video_muted() const { return video_muted; }
    void set_custom_scene(const Ref<PackedScene> &p_value);
    Ref<PackedScene> get_custom_scene() const { return custom_scene; }
    void set_max_width(float p_value);
    float get_max_width() const { return max_width; }

  protected:
    static void _bind_methods();

  private:
    String title;
    String text; // BBCode
    Ref<Texture2D> icon;
    Vector2 icon_max_size = Vector2(64, 64);
    Ref<VideoStream> video;
    Vector2 video_size = Vector2(240, 135); // (0, 0) : taille native de la vidéo
    bool video_loop = true;
    bool video_muted = true;
    Ref<PackedScene> custom_scene;
    float max_width = 320.0f; // largeur au-delà de laquelle le texte revient à la ligne
};
} // namespace godot

#endif // TOOLTIP_CONTENT_HPP
