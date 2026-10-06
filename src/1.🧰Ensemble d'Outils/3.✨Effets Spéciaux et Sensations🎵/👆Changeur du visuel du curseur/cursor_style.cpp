#include "cursor_style.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

Ref<Texture2D> CursorStyle::get_frame_at(double p_time) const
{
    const int64_t count = frames.size();
    if (count == 0)
    {
        return Ref<Texture2D>();
    }
    if (count == 1 || frames_per_second <= 0.0)
    {
        return frames[0];
    }
    const int64_t index = (int64_t)Math::floor(Math::max(0.0, p_time) * frames_per_second) % count;
    return frames[index];
}

void CursorStyle::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_frame_at", "time"), &CursorStyle::get_frame_at);
    ClassDB::bind_method(D_METHOD("set_frames", "value"), &CursorStyle::set_frames);
    ClassDB::bind_method(D_METHOD("get_frames"), &CursorStyle::get_frames);
    ClassDB::bind_method(D_METHOD("set_frames_per_second", "value"), &CursorStyle::set_frames_per_second);
    ClassDB::bind_method(D_METHOD("get_frames_per_second"), &CursorStyle::get_frames_per_second);
    ClassDB::bind_method(D_METHOD("set_hotspot", "value"), &CursorStyle::set_hotspot);
    ClassDB::bind_method(D_METHOD("get_hotspot"), &CursorStyle::get_hotspot);
    ClassDB::bind_method(D_METHOD("set_shape", "value"), &CursorStyle::set_shape);
    ClassDB::bind_method(D_METHOD("get_shape"), &CursorStyle::get_shape);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "frames", PROPERTY_HINT_ARRAY_TYPE, "Texture2D"), "set_frames", "get_frames");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frames_per_second", PROPERTY_HINT_RANGE, "0,60,0.1,or_greater"), "set_frames_per_second", "get_frames_per_second");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "hotspot", PROPERTY_HINT_NONE, "suffix:px"), "set_hotspot", "get_hotspot");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "shape", PROPERTY_HINT_ENUM,
                         "Arrow,IBeam,Pointing Hand,Cross,Wait,Busy,Drag,Can Drop,Forbidden,Vertical Size,Horizontal Size,Secondary Diagonal Size,Main Diagonal Size,Move,Vertical Split,Horizontal Split,Help"),
            "set_shape", "get_shape");
}
