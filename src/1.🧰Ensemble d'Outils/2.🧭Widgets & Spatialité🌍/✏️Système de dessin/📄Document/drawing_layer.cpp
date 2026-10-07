#include "drawing_layer.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

#include <cstring>

using namespace godot;

Ref<DrawingLayer> DrawingLayer::create(const Vector2i &p_size, const String &p_name)
{
    Ref<DrawingLayer> layer;
    layer.instantiate();
    layer->layer_name = p_name;
    layer->resize(p_size);
    return layer;
}

// ⚙️ Réglages

void DrawingLayer::set_layer_name(const String &p_name)
{
    layer_name = p_name;
    emit_changed();
}

void DrawingLayer::set_visible(bool p_visible)
{
    visible = p_visible;
    emit_changed();
}

void DrawingLayer::set_opacity(float p_opacity)
{
    opacity = Math::clamp(p_opacity, 0.0f, 1.0f);
    emit_changed();
}

void DrawingLayer::set_blend_mode(LayerBlend p_mode)
{
    ERR_FAIL_INDEX(int(p_mode), int(LAYER_BLEND_MAX));
    blend_mode = p_mode;
    emit_changed();
}

void DrawingLayer::set_alpha_locked(bool p_locked)
{
    alpha_locked = p_locked;
    emit_changed();
}

void DrawingLayer::set_clipped(bool p_clipped)
{
    clipped = p_clipped;
    emit_changed();
}

void DrawingLayer::set_locked(bool p_locked)
{
    locked = p_locked;
    emit_changed();
}

// 🎨 Pixels

Ref<Image> DrawingLayer::get_image() const
{
    if (width <= 0 || height <= 0)
    {
        return Ref<Image>();
    }
    return Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, data);
}

void DrawingLayer::set_image(const Ref<Image> &p_image)
{
    ERR_FAIL_COND(p_image.is_null() || p_image->is_empty());
    Ref<Image> image = p_image->duplicate();
    if (image->is_compressed())
    {
        image->decompress();
    }
    image->clear_mipmaps();
    image->convert(Image::FORMAT_RGBA8);
    width = image->get_width();
    height = image->get_height();
    data = image->get_data();
    emit_changed();
}

void DrawingLayer::set_data(int p_width, int p_height, const PackedByteArray &p_data)
{
    ERR_FAIL_COND(p_width < 0 || p_height < 0 || p_data.size() != int64_t(p_width) * p_height * 4);
    width = p_width;
    height = p_height;
    data = p_data;
}

Color DrawingLayer::get_pixel(int p_x, int p_y) const
{
    ERR_FAIL_COND_V(p_x < 0 || p_y < 0 || p_x >= width || p_y >= height, Color());
    const uint8_t *p = data.ptr() + (size_t(p_y) * width + p_x) * 4;
    return Color(p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f, p[3] / 255.0f);
}

bool DrawingLayer::is_empty() const
{
    const uint8_t *p = data.ptr();
    const int64_t count = int64_t(width) * height;
    for (int64_t i = 0; i < count; i++)
    {
        if (p[i * 4 + 3] != 0)
        {
            return false;
        }
    }
    return true;
}

PackedByteArray DrawingLayer::get_region(const Rect2i &p_rect) const
{
    PackedByteArray bytes;
    ERR_FAIL_COND_V(!Rect2i(0, 0, width, height).encloses(p_rect), bytes);
    bytes.resize(int64_t(p_rect.size.x) * p_rect.size.y * 4);
    uint8_t *w = bytes.ptrw();
    const uint8_t *r = data.ptr();
    for (int y = 0; y < p_rect.size.y; y++)
    {
        memcpy(w + size_t(y) * p_rect.size.x * 4, r + (size_t(p_rect.position.y + y) * width + p_rect.position.x) * 4, size_t(p_rect.size.x) * 4);
    }
    return bytes;
}

void DrawingLayer::set_region(const Rect2i &p_rect, const PackedByteArray &p_bytes)
{
    ERR_FAIL_COND(!Rect2i(0, 0, width, height).encloses(p_rect));
    ERR_FAIL_COND(p_bytes.size() != int64_t(p_rect.size.x) * p_rect.size.y * 4);
    uint8_t *w = data.ptrw();
    const uint8_t *r = p_bytes.ptr();
    for (int y = 0; y < p_rect.size.y; y++)
    {
        memcpy(w + (size_t(p_rect.position.y + y) * width + p_rect.position.x) * 4, r + size_t(y) * p_rect.size.x * 4, size_t(p_rect.size.x) * 4);
    }
}

void DrawingLayer::resize(const Vector2i &p_size)
{
    const Vector2i size = p_size.max(Vector2i(1, 1));
    if (size.x == width && size.y == height)
    {
        return;
    }
    PackedByteArray resized;
    resized.resize(int64_t(size.x) * size.y * 4);
    resized.fill(0);
    const int copy_width = MIN(width, size.x);
    const int copy_height = MIN(height, size.y);
    uint8_t *w = resized.ptrw();
    const uint8_t *r = data.ptr();
    for (int y = 0; y < copy_height; y++)
    {
        memcpy(w + size_t(y) * size.x * 4, r + size_t(y) * width * 4, size_t(copy_width) * 4);
    }
    width = size.x;
    height = size.y;
    data = resized;
}

PackedByteArray DrawingLayer::get_png_data() const
{
    if (width <= 0 || height <= 0)
    {
        return PackedByteArray();
    }
    if (png_cache.is_empty() || png_cache_source.ptr() != data.ptr())
    {
        png_cache = get_image()->save_png_to_buffer();
        png_cache_source = data;
    }
    return png_cache;
}

void DrawingLayer::set_png_data(const PackedByteArray &p_png)
{
    if (p_png.is_empty())
    {
        return;
    }
    Ref<Image> image;
    image.instantiate();
    ERR_FAIL_COND_MSG(image->load_png_from_buffer(p_png) != OK, U"Pixels du calque illisibles (PNG invalide).");
    set_image(image);
}

// 🔗 Liaisons

void DrawingLayer::_bind_methods()
{
    ClassDB::bind_static_method("DrawingLayer", D_METHOD("create", "size", "name"), &DrawingLayer::create, DEFVAL(String()));

    ClassDB::bind_method(D_METHOD("set_layer_name", "name"), &DrawingLayer::set_layer_name);
    ClassDB::bind_method(D_METHOD("get_layer_name"), &DrawingLayer::get_layer_name);
    ClassDB::bind_method(D_METHOD("set_visible", "visible"), &DrawingLayer::set_visible);
    ClassDB::bind_method(D_METHOD("is_visible"), &DrawingLayer::is_visible);
    ClassDB::bind_method(D_METHOD("set_opacity", "opacity"), &DrawingLayer::set_opacity);
    ClassDB::bind_method(D_METHOD("get_opacity"), &DrawingLayer::get_opacity);
    ClassDB::bind_method(D_METHOD("set_blend_mode", "mode"), &DrawingLayer::set_blend_mode);
    ClassDB::bind_method(D_METHOD("get_blend_mode"), &DrawingLayer::get_blend_mode);
    ClassDB::bind_method(D_METHOD("set_alpha_locked", "locked"), &DrawingLayer::set_alpha_locked);
    ClassDB::bind_method(D_METHOD("is_alpha_locked"), &DrawingLayer::is_alpha_locked);
    ClassDB::bind_method(D_METHOD("set_clipped", "clipped"), &DrawingLayer::set_clipped);
    ClassDB::bind_method(D_METHOD("is_clipped"), &DrawingLayer::is_clipped);
    ClassDB::bind_method(D_METHOD("set_locked", "locked"), &DrawingLayer::set_locked);
    ClassDB::bind_method(D_METHOD("is_locked"), &DrawingLayer::is_locked);
    ClassDB::bind_method(D_METHOD("get_image"), &DrawingLayer::get_image);
    ClassDB::bind_method(D_METHOD("set_image", "image"), &DrawingLayer::set_image);
    ClassDB::bind_method(D_METHOD("get_size"), &DrawingLayer::get_size);
    ClassDB::bind_method(D_METHOD("get_pixel", "x", "y"), &DrawingLayer::get_pixel);
    ClassDB::bind_method(D_METHOD("is_empty"), &DrawingLayer::is_empty);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "layer_name"), "set_layer_name", "get_layer_name");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "visible"), "set_visible", "is_visible");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "opacity", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_opacity", "get_opacity");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "blend_mode", PROPERTY_HINT_ENUM, "Normal,Multiply,Screen,Overlay,Darken,Lighten,Color Dodge,Color Burn,Add,Subtract,Difference,Soft Light,Hard Light"),
                 "set_blend_mode", "get_blend_mode");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "alpha_locked"), "set_alpha_locked", "is_alpha_locked");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "clipped"), "set_clipped", "is_clipped");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "locked"), "set_locked", "is_locked");
    ClassDB::bind_method(D_METHOD("get_png_data"), &DrawingLayer::get_png_data);
    ClassDB::bind_method(D_METHOD("set_png_data", "png"), &DrawingLayer::set_png_data);

    // Pixels : sauvegardés avec la ressource (en PNG), sans éditeur dans l'inspecteur.
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "png_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_png_data", "get_png_data");

    BIND_ENUM_CONSTANT(LAYER_BLEND_NORMAL);
    BIND_ENUM_CONSTANT(LAYER_BLEND_MULTIPLY);
    BIND_ENUM_CONSTANT(LAYER_BLEND_SCREEN);
    BIND_ENUM_CONSTANT(LAYER_BLEND_OVERLAY);
    BIND_ENUM_CONSTANT(LAYER_BLEND_DARKEN);
    BIND_ENUM_CONSTANT(LAYER_BLEND_LIGHTEN);
    BIND_ENUM_CONSTANT(LAYER_BLEND_COLOR_DODGE);
    BIND_ENUM_CONSTANT(LAYER_BLEND_COLOR_BURN);
    BIND_ENUM_CONSTANT(LAYER_BLEND_ADD);
    BIND_ENUM_CONSTANT(LAYER_BLEND_SUBTRACT);
    BIND_ENUM_CONSTANT(LAYER_BLEND_DIFFERENCE);
    BIND_ENUM_CONSTANT(LAYER_BLEND_SOFT_LIGHT);
    BIND_ENUM_CONSTANT(LAYER_BLEND_HARD_LIGHT);
    BIND_ENUM_CONSTANT(LAYER_BLEND_MAX);
}
