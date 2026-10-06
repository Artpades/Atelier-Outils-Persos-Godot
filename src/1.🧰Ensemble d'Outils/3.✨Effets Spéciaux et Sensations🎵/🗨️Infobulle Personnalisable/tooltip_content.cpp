#include "tooltip_content.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// emit_changed : une infobulle affichée se reconstruit quand son contenu est modifié.
void TooltipContent::set_title(const String &p_value)
{
    title = p_value;
    emit_changed();
}
void TooltipContent::set_text(const String &p_value)
{
    text = p_value;
    emit_changed();
}
void TooltipContent::set_icon(const Ref<Texture2D> &p_value)
{
    icon = p_value;
    emit_changed();
}
void TooltipContent::set_icon_max_size(const Vector2 &p_value)
{
    icon_max_size = p_value;
    emit_changed();
}
void TooltipContent::set_video(const Ref<VideoStream> &p_value)
{
    video = p_value;
    emit_changed();
}
void TooltipContent::set_video_size(const Vector2 &p_value)
{
    video_size = p_value;
    emit_changed();
}
void TooltipContent::set_video_loop(bool p_value)
{
    video_loop = p_value;
    emit_changed();
}
void TooltipContent::set_video_muted(bool p_value)
{
    video_muted = p_value;
    emit_changed();
}
void TooltipContent::set_custom_scene(const Ref<PackedScene> &p_value)
{
    custom_scene = p_value;
    emit_changed();
}
void TooltipContent::set_max_width(float p_value)
{
    max_width = p_value;
    emit_changed();
}

void TooltipContent::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_title", "value"), &TooltipContent::set_title);
    ClassDB::bind_method(D_METHOD("get_title"), &TooltipContent::get_title);
    ClassDB::bind_method(D_METHOD("set_text", "value"), &TooltipContent::set_text);
    ClassDB::bind_method(D_METHOD("get_text"), &TooltipContent::get_text);
    ClassDB::bind_method(D_METHOD("set_icon", "value"), &TooltipContent::set_icon);
    ClassDB::bind_method(D_METHOD("get_icon"), &TooltipContent::get_icon);
    ClassDB::bind_method(D_METHOD("set_icon_max_size", "value"), &TooltipContent::set_icon_max_size);
    ClassDB::bind_method(D_METHOD("get_icon_max_size"), &TooltipContent::get_icon_max_size);
    ClassDB::bind_method(D_METHOD("set_video", "value"), &TooltipContent::set_video);
    ClassDB::bind_method(D_METHOD("get_video"), &TooltipContent::get_video);
    ClassDB::bind_method(D_METHOD("set_video_size", "value"), &TooltipContent::set_video_size);
    ClassDB::bind_method(D_METHOD("get_video_size"), &TooltipContent::get_video_size);
    ClassDB::bind_method(D_METHOD("set_video_loop", "value"), &TooltipContent::set_video_loop);
    ClassDB::bind_method(D_METHOD("get_video_loop"), &TooltipContent::get_video_loop);
    ClassDB::bind_method(D_METHOD("set_video_muted", "value"), &TooltipContent::set_video_muted);
    ClassDB::bind_method(D_METHOD("get_video_muted"), &TooltipContent::get_video_muted);
    ClassDB::bind_method(D_METHOD("set_custom_scene", "value"), &TooltipContent::set_custom_scene);
    ClassDB::bind_method(D_METHOD("get_custom_scene"), &TooltipContent::get_custom_scene);
    ClassDB::bind_method(D_METHOD("set_max_width", "value"), &TooltipContent::set_max_width);
    ClassDB::bind_method(D_METHOD("get_max_width"), &TooltipContent::get_max_width);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "text", PROPERTY_HINT_MULTILINE_TEXT), "set_text", "get_text");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_width", PROPERTY_HINT_RANGE, "50,1000,1,or_greater,suffix:px"), "set_max_width", "get_max_width");

    ADD_GROUP("Icon", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "icon", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_icon", "get_icon");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "icon_max_size", PROPERTY_HINT_NONE, "suffix:px"), "set_icon_max_size", "get_icon_max_size");

    ADD_GROUP("Video", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "video", PROPERTY_HINT_RESOURCE_TYPE, "VideoStream"), "set_video", "get_video");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "video_size", PROPERTY_HINT_NONE, "suffix:px"), "set_video_size", "get_video_size");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "video_loop"), "set_video_loop", "get_video_loop");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "video_muted"), "set_video_muted", "get_video_muted");

    ADD_GROUP("Custom", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "custom_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_custom_scene", "get_custom_scene");
}
