#include "drawing_canvas.hpp"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_magnify_gesture.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_pan_gesture.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <cmath>
#include <cstring>

using namespace godot;

// Contour animé (« fourmis ») du masque de sélection : une ligne d'un pixel de l'écran le long du bord intérieur.
// Au-delà du bord du dessin, le masque compte comme non sélectionné (inside_bounds). Code ASCII uniquement
// (String(const char *) est lu en Latin-1).
static const char *SELECTION_SHADER = R"(
shader_type canvas_item;
render_mode unshaded;

float inside_bounds(vec2 uv) {
	return step(0.0, uv.x) * step(0.0, uv.y) * step(uv.x, 1.0) * step(uv.y, 1.0);
}

void fragment() {
	vec2 pixel = fwidth(UV);
	vec2 offsets[4] = vec2[](vec2(pixel.x, 0.0), vec2(-pixel.x, 0.0), vec2(0.0, pixel.y), vec2(0.0, -pixel.y));
	float inside = step(0.5, texture(TEXTURE, UV).r);
	float neighbours = 0.0;
	for (int i = 0; i < 4; i++) {
		vec2 uv = UV + offsets[i];
		neighbours += step(0.5, texture(TEXTURE, uv).r) * inside_bounds(uv);
	}
	float edge = inside * step(neighbours, 3.5);
	float stripe = step(0.5, fract((FRAGCOORD.x + FRAGCOORD.y) / 8.0 - TIME * 1.5));
	COLOR = vec4(vec3(stripe), edge);
}
)";

DrawingCanvas::DrawingCanvas()
{
    set_focus_mode(FOCUS_ALL);
    set_clip_contents(true);
    // Damier de transparence : texture 2×2 répétée, pixels nets (seul le fond de la toile est dessiné par le Control).
    set_texture_filter(TEXTURE_FILTER_NEAREST);
    set_texture_repeat(TEXTURE_REPEAT_ENABLED);
    set_default_cursor_shape(CURSOR_CROSS);

    Ref<Image> checker = Image::create_empty(2, 2, false, Image::FORMAT_RGB8);
    const Color light(0.86f, 0.86f, 0.88f);
    const Color dark(0.7f, 0.7f, 0.73f);
    checker->set_pixel(0, 0, light);
    checker->set_pixel(1, 1, light);
    checker->set_pixel(1, 0, dark);
    checker->set_pixel(0, 1, dark);
    checker_texture = ImageTexture::create_from_image(checker);

    Ref<Shader> shader;
    shader.instantiate();
    shader->set_code(SELECTION_SHADER);
    selection_material.instantiate();
    selection_material->set_shader(shader);

    fallback_brush = DrawingBrush::create_preset(DrawingBrush::PRESET_PEN);
    fallback_eraser = DrawingBrush::create_preset(DrawingBrush::PRESET_HARD_ERASER);
}

DrawingCanvas::~DrawingCanvas() { _free_items(); }

void DrawingCanvas::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_ENTER_TREE:
        _create_items();
        _ensure_document();
        _rebuild_tiles();
        _update_view();
        selection_dirty = true;
        set_process_internal(true);
        break;
    case NOTIFICATION_EXIT_TREE:
        if (drag == DRAG_STROKE)
        {
            _end_stroke();
        }
        drag = DRAG_NONE;
        if (commit_viewport)
        {
            _finish_commit(Ref<Image>());
        }
        _free_items();
        break;
    case NOTIFICATION_RESIZED:
        if (auto_fit)
        {
            _fit(); // le dessin reste ajusté tant que la vue n'a pas été déplacée à la main
        }
        _update_view();
        break;
    case NOTIFICATION_DRAW:
        _draw_background();
        break;
    case NOTIFICATION_INTERNAL_PROCESS:
        _update_tiles();
        _update_stroke_tiles();
        if (selection_dirty)
        {
            _update_selection();
        }
        if (overlay_dirty)
        {
            _update_overlay();
        }
        if (commit_viewport && ++commit_frames > 8)
        {
            _finish_commit(Ref<Image>()); // aucun rendu reçu : trait appliqué sans le shader
        }
        break;
    case NOTIFICATION_MOUSE_ENTER:
        hovering = true;
        overlay_dirty = true;
        break;
    case NOTIFICATION_MOUSE_EXIT:
        hovering = false;
        overlay_dirty = true;
        break;
    default:
        break;
    }
}

// 📄 Document

void DrawingCanvas::set_document(const Ref<DrawingDocument> &p_document)
{
    if (p_document == document)
    {
        return;
    }
    if (drag == DRAG_STROKE)
    {
        _end_stroke();
    }
    drag = DRAG_NONE;
    if (commit_viewport)
    {
        _finish_commit(Ref<Image>());
    }
    _disconnect_document();
    document = p_document;
    _connect_document();
    auto_fit = true;
    _rebuild_tiles();
    _clear_stroke_tiles();
    selection_dirty = true;
    _fit();
    _update_view();
    emit_signal("document_changed");
}

void DrawingCanvas::_connect_document()
{
    if (document.is_null())
    {
        return;
    }
    document->connect("region_changed", callable_mp(this, &DrawingCanvas::_on_region_changed));
    document->connect("stroke_changed", callable_mp(this, &DrawingCanvas::_on_stroke_changed));
    document->connect("layers_changed", callable_mp(this, &DrawingCanvas::_on_document_structure_changed));
    document->connect("changed", callable_mp(this, &DrawingCanvas::_on_document_structure_changed));
    document->connect("selection_changed", callable_mp(this, &DrawingCanvas::_on_selection_changed));
}

void DrawingCanvas::_disconnect_document()
{
    if (document.is_null())
    {
        return;
    }
    document->disconnect("region_changed", callable_mp(this, &DrawingCanvas::_on_region_changed));
    document->disconnect("stroke_changed", callable_mp(this, &DrawingCanvas::_on_stroke_changed));
    document->disconnect("layers_changed", callable_mp(this, &DrawingCanvas::_on_document_structure_changed));
    document->disconnect("changed", callable_mp(this, &DrawingCanvas::_on_document_structure_changed));
    document->disconnect("selection_changed", callable_mp(this, &DrawingCanvas::_on_selection_changed));
}

void DrawingCanvas::_ensure_document()
{
    if (document.is_null() && !Engine::get_singleton()->is_editor_hint())
    {
        set_document(DrawingDocument::create(default_document_size));
    }
}

void DrawingCanvas::_on_region_changed(const Rect2i &p_rect)
{
    if (tiles.empty())
    {
        return;
    }
    const Vector2i first = (p_rect.position / TILE_SIZE).max(Vector2i());
    const Vector2i last = ((p_rect.get_end() - Vector2i(1, 1)) / TILE_SIZE).min(tile_grid - Vector2i(1, 1));
    for (int y = first.y; y <= last.y; y++)
    {
        for (int x = first.x; x <= last.x; x++)
        {
            tiles_dirty[y * tile_grid.x + x] = true;
            any_tile_dirty = true;
        }
    }
}

void DrawingCanvas::_on_stroke_changed(const Rect2i &p_rect)
{
    if (stroke_tiles.empty())
    {
        return;
    }
    if (document.is_null() || !document->is_stroke_deferred())
    {
        _clear_stroke_tiles(); // trait différé annulé
        return;
    }
    const Vector2i first = (p_rect.position / TILE_SIZE).max(Vector2i());
    const Vector2i last = ((p_rect.get_end() - Vector2i(1, 1)) / TILE_SIZE).min(tile_grid - Vector2i(1, 1));
    for (int y = first.y; y <= last.y; y++)
    {
        for (int x = first.x; x <= last.x; x++)
        {
            stroke_tiles_dirty[y * tile_grid.x + x] = true;
            any_stroke_tile_dirty = true;
        }
    }
}

void DrawingCanvas::_on_document_structure_changed()
{
    const Vector2i expected = document.is_valid() ? (document->get_size() + Vector2i(TILE_SIZE - 1, TILE_SIZE - 1)) / TILE_SIZE : Vector2i();
    if (expected != tile_grid || (document.is_valid() && !tiles.empty() && tiles[0].is_valid() && Vector2i(tiles[0]->get_size()) != _tile_rect(0).size))
    {
        _rebuild_tiles();
        _clear_stroke_tiles();
        selection_dirty = true;
        _update_view();
    }
    queue_redraw(); // couleur du fond
}

void DrawingCanvas::_on_selection_changed() { selection_dirty = true; }

// 🛠️ Outils

void DrawingCanvas::set_tool(Tool p_tool)
{
    ERR_FAIL_INDEX(int(p_tool), int(TOOL_MAX));
    if (p_tool == tool)
    {
        return;
    }
    tool = p_tool;
    _update_cursor_shape();
    overlay_dirty = true;
    emit_signal("tool_changed", tool);
}

void DrawingCanvas::set_brush(const Ref<DrawingBrush> &p_brush)
{
    brush = p_brush;
    overlay_dirty = true;
}

void DrawingCanvas::set_eraser(const Ref<DrawingBrush> &p_eraser)
{
    eraser = p_eraser;
    overlay_dirty = true;
}

Ref<DrawingBrush> DrawingCanvas::get_active_brush() const
{
    if (tool == TOOL_ERASER)
    {
        return eraser.is_valid() ? eraser : fallback_eraser;
    }
    return brush.is_valid() ? brush : fallback_brush;
}

Ref<DrawingBrush> DrawingCanvas::_brush_for(Tool p_tool)
{
    if (p_tool == TOOL_ERASER)
    {
        return eraser.is_valid() ? eraser : fallback_eraser;
    }
    return brush.is_valid() ? brush : fallback_brush;
}

void DrawingCanvas::set_primary_color(const Color &p_color)
{
    if (p_color == primary_color)
    {
        return;
    }
    primary_color = p_color;
    overlay_dirty = true;
    emit_signal("primary_color_changed", primary_color);
}

void DrawingCanvas::set_secondary_color(const Color &p_color)
{
    if (p_color == secondary_color)
    {
        return;
    }
    secondary_color = p_color;
    emit_signal("secondary_color_changed", secondary_color);
}

void DrawingCanvas::swap_colors()
{
    const Color previous = primary_color;
    set_primary_color(secondary_color);
    set_secondary_color(previous);
}

// 🧭 Vue

Transform2D DrawingCanvas::get_view_transform() const
{
    const Vector2 document_size = document.is_valid() ? Vector2(document->get_size()) : Vector2(default_document_size);
    const Transform2D rotation_scale(Math::deg_to_rad(view_rotation), Size2(flip_h ? -zoom : zoom, zoom), 0.0f, get_size() * 0.5f + view_offset);
    return rotation_scale * Transform2D(0.0f, -document_size * 0.5f);
}

Vector2 DrawingCanvas::canvas_to_screen(const Vector2 &p_canvas_position) const { return get_view_transform().xform(p_canvas_position); }

Vector2 DrawingCanvas::screen_to_canvas(const Vector2 &p_screen_position) const { return get_view_transform().affine_inverse().xform(p_screen_position); }

void DrawingCanvas::_anchor(const Vector2 &p_canvas, const Vector2 &p_screen)
{
    // Décalage tel que le point p_canvas du dessin apparaisse en p_screen.
    const Vector2 document_size = document.is_valid() ? Vector2(document->get_size()) : Vector2(default_document_size);
    const Transform2D rotation_scale(Math::deg_to_rad(view_rotation), Size2(flip_h ? -zoom : zoom, zoom), 0.0f, Vector2());
    const Vector2 center = p_screen - rotation_scale.basis_xform(p_canvas - document_size * 0.5f);
    view_offset = center - get_size() * 0.5f;
    auto_fit = false;
    _update_view();
}

void DrawingCanvas::set_zoom(float p_zoom)
{
    zoom = Math::clamp(p_zoom, min_zoom, max_zoom);
    auto_fit = false;
    _update_view();
}

void DrawingCanvas::set_view_offset(const Vector2 &p_offset)
{
    view_offset = p_offset;
    auto_fit = false;
    _update_view();
}

void DrawingCanvas::set_view_rotation(float p_degrees)
{
    view_rotation = Math::wrapf(p_degrees, -180.0f, 180.0f);
    auto_fit = false;
    _update_view();
}

void DrawingCanvas::set_flip_h(bool p_flip)
{
    if (p_flip == flip_h)
    {
        return;
    }
    const Vector2 center = get_size() * 0.5f;
    const Vector2 anchor = screen_to_canvas(center);
    flip_h = p_flip;
    _anchor(anchor, center);
}

void DrawingCanvas::zoom_at(float p_factor, const Vector2 &p_screen_position)
{
    const Vector2 anchor = screen_to_canvas(p_screen_position);
    zoom = Math::clamp(zoom * p_factor, min_zoom, max_zoom);
    auto_fit = false;
    _anchor(anchor, p_screen_position);
}

void DrawingCanvas::rotate_at(float p_degrees, const Vector2 &p_screen_position)
{
    const Vector2 anchor = screen_to_canvas(p_screen_position);
    view_rotation = Math::wrapf(view_rotation + p_degrees, -180.0f, 180.0f);
    _anchor(anchor, p_screen_position);
}

void DrawingCanvas::fit_to_view()
{
    auto_fit = true;
    _fit();
    _update_view();
}

void DrawingCanvas::_fit()
{
    if (document.is_null() || get_size().x <= 1.0f || get_size().y <= 1.0f)
    {
        return;
    }
    const Vector2 document_size = document->get_size();
    const float angle = Math::deg_to_rad(view_rotation);
    const float c = std::fabs(std::cos(angle));
    const float s = std::fabs(std::sin(angle));
    const Vector2 rotated(document_size.x * c + document_size.y * s, document_size.x * s + document_size.y * c);
    const Vector2 available = get_size() * 0.92f;
    zoom = Math::clamp(MIN(available.x / rotated.x, available.y / rotated.y), min_zoom, max_zoom);
    view_offset = Vector2();
}

void DrawingCanvas::actual_size()
{
    zoom = Math::clamp(1.0f, min_zoom, max_zoom);
    view_offset = Vector2();
    auto_fit = false;
    _update_view();
}

void DrawingCanvas::reset_rotation()
{
    const Vector2 center = get_size() * 0.5f;
    const Vector2 anchor = screen_to_canvas(center);
    view_rotation = 0.0f;
    flip_h = false;
    _anchor(anchor, center);
}

void DrawingCanvas::_update_view()
{
    if (view_item.is_valid())
    {
        RenderingServer *rs = RenderingServer::get_singleton();
        rs->canvas_item_set_transform(view_item, get_view_transform());
        // Pixels nets en zoom avant (comme un logiciel de dessin), lissés en zoom arrière.
        rs->canvas_item_set_default_texture_filter(tiles_item, zoom >= 1.0f ? RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST : RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_LINEAR);
        rs->canvas_item_set_default_texture_filter(stroke_item, zoom >= 1.0f ? RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST : RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_LINEAR);
    }
    overlay_dirty = true;
    queue_redraw();
    emit_signal("view_changed");
}

// 🎨 Affichage

void DrawingCanvas::set_view_background(const Color &p_color)
{
    view_background = p_color;
    queue_redraw();
}

void DrawingCanvas::set_checkerboard(bool p_enabled)
{
    checkerboard = p_enabled;
    queue_redraw();
}

void DrawingCanvas::set_show_brush_cursor(bool p_enabled)
{
    show_brush_cursor = p_enabled;
    overlay_dirty = true;
}

Vector2 DrawingCanvas::get_pointer_position() const { return hovering ? screen_to_canvas(hover_position) : Vector2(-1, -1); }

void DrawingCanvas::_create_items()
{
    if (view_item.is_valid())
    {
        return;
    }
    RenderingServer *rs = RenderingServer::get_singleton();
    view_item = rs->canvas_item_create();
    rs->canvas_item_set_parent(view_item, get_canvas_item());
    rs->canvas_item_set_draw_index(view_item, 0);

    tiles_item = rs->canvas_item_create();
    rs->canvas_item_set_parent(tiles_item, view_item);
    rs->canvas_item_set_draw_index(tiles_item, 0);
    rs->canvas_item_set_default_texture_repeat(tiles_item, RenderingServer::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);

    stroke_item = rs->canvas_item_create();
    rs->canvas_item_set_parent(stroke_item, view_item);
    rs->canvas_item_set_draw_index(stroke_item, 1);
    rs->canvas_item_set_default_texture_repeat(stroke_item, RenderingServer::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);

    selection_item = rs->canvas_item_create();
    rs->canvas_item_set_parent(selection_item, view_item);
    rs->canvas_item_set_draw_index(selection_item, 2);
    rs->canvas_item_set_default_texture_filter(selection_item, RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
    rs->canvas_item_set_default_texture_repeat(selection_item, RenderingServer::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);
    rs->canvas_item_set_material(selection_item, selection_material->get_rid());

    overlay_item = rs->canvas_item_create();
    rs->canvas_item_set_parent(overlay_item, get_canvas_item());
    rs->canvas_item_set_draw_index(overlay_item, 1);
    rs->canvas_item_set_default_texture_filter(overlay_item, RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_LINEAR);

    // Les commandes de tuiles seront recréées avec les textures.
    tiles.assign(tiles.size(), Ref<ImageTexture>());
    tiles_dirty.assign(tiles_dirty.size(), true);
    any_tile_dirty = !tiles.empty();
    overlay_dirty = true;
}

void DrawingCanvas::_free_items()
{
    RenderingServer *rs = RenderingServer::get_singleton();
    if (!rs)
    {
        return;
    }
    for (RID *item : {&overlay_item, &selection_item, &stroke_item, &tiles_item, &view_item})
    {
        if (item->is_valid())
        {
            rs->free_rid(*item);
            *item = RID();
        }
    }
}

Rect2i DrawingCanvas::_tile_rect(int p_index) const
{
    const Vector2i cell(p_index % MAX(tile_grid.x, 1), p_index / MAX(tile_grid.x, 1));
    const Vector2i position = cell * TILE_SIZE;
    const Vector2i document_size = document.is_valid() ? document->get_size() : Vector2i();
    return Rect2i(position, (document_size - position).min(Vector2i(TILE_SIZE, TILE_SIZE)));
}

void DrawingCanvas::_rebuild_tiles()
{
    if (tiles_item.is_valid())
    {
        RenderingServer::get_singleton()->canvas_item_clear(tiles_item);
    }
    if (document.is_null())
    {
        tile_grid = Vector2i();
        tiles.clear();
        tiles_dirty.clear();
        stroke_tiles.clear();
        stroke_tiles_dirty.clear();
        any_tile_dirty = false;
        return;
    }
    tile_grid = (document->get_size() + Vector2i(TILE_SIZE - 1, TILE_SIZE - 1)) / TILE_SIZE;
    const size_t count = size_t(tile_grid.x) * tile_grid.y;
    tiles.assign(count, Ref<ImageTexture>());
    tiles_dirty.assign(count, true);
    stroke_tiles.assign(count, Ref<ImageTexture>());
    stroke_tiles_dirty.assign(count, false);
    any_tile_dirty = true;
    any_stroke_tile_dirty = false;
}

void DrawingCanvas::_update_tiles()
{
    if (!any_tile_dirty || document.is_null() || !tiles_item.is_valid())
    {
        return;
    }
    any_tile_dirty = false;
    const uint8_t *composite = document->get_composite_ptr();
    const int document_width = document->get_size().x;
    bool created = false;
    for (size_t i = 0; i < tiles.size(); i++)
    {
        if (!tiles_dirty[i])
        {
            continue;
        }
        tiles_dirty[i] = false;
        const Rect2i rect = _tile_rect(int(i));
        PackedByteArray bytes;
        bytes.resize(int64_t(rect.size.x) * rect.size.y * 4);
        uint8_t *w = bytes.ptrw();
        for (int y = 0; y < rect.size.y; y++)
        {
            memcpy(w + size_t(y) * rect.size.x * 4, composite + (size_t(rect.position.y + y) * document_width + rect.position.x) * 4, size_t(rect.size.x) * 4);
        }
        Ref<Image> image = Image::create_from_data(rect.size.x, rect.size.y, false, Image::FORMAT_RGBA8, bytes);
        if (tiles[i].is_null() || Vector2i(tiles[i]->get_size()) != rect.size)
        {
            tiles[i] = ImageTexture::create_from_image(image);
            created = true;
        }
        else
        {
            tiles[i]->update(image);
        }
    }
    if (created)
    {
        RenderingServer *rs = RenderingServer::get_singleton();
        rs->canvas_item_clear(tiles_item);
        for (size_t i = 0; i < tiles.size(); i++)
        {
            if (tiles[i].is_valid())
            {
                rs->canvas_item_add_texture_rect(tiles_item, Rect2(_tile_rect(int(i))), tiles[i]->get_rid());
            }
        }
    }
}

void DrawingCanvas::_update_stroke_tiles()
{
    if (!any_stroke_tile_dirty || document.is_null() || !stroke_item.is_valid())
    {
        return;
    }
    any_stroke_tile_dirty = false;
    bool created = false;
    for (size_t i = 0; i < stroke_tiles.size(); i++)
    {
        if (!stroke_tiles_dirty[i])
        {
            continue;
        }
        stroke_tiles_dirty[i] = false;
        Ref<Image> image = document->get_stroke_preview(_tile_rect(int(i)));
        if (image.is_null())
        {
            continue;
        }
        if (stroke_tiles[i].is_null())
        {
            stroke_tiles[i] = ImageTexture::create_from_image(image);
            created = true;
        }
        else
        {
            stroke_tiles[i]->update(image);
        }
    }
    if (created)
    {
        _rebuild_stroke_commands();
    }
}

void DrawingCanvas::_rebuild_stroke_commands()
{
    RenderingServer *rs = RenderingServer::get_singleton();
    rs->canvas_item_clear(stroke_item);
    for (size_t i = 0; i < stroke_tiles.size(); i++)
    {
        if (stroke_tiles[i].is_valid())
        {
            rs->canvas_item_add_texture_rect(stroke_item, Rect2(_tile_rect(int(i))), stroke_tiles[i]->get_rid());
        }
    }
}

void DrawingCanvas::_clear_stroke_tiles()
{
    stroke_tiles.assign(stroke_tiles.size(), Ref<ImageTexture>());
    stroke_tiles_dirty.assign(stroke_tiles_dirty.size(), false);
    any_stroke_tile_dirty = false;
    if (stroke_item.is_valid())
    {
        RenderingServer::get_singleton()->canvas_item_clear(stroke_item);
        RenderingServer::get_singleton()->canvas_item_set_material(stroke_item, RID());
    }
}

void DrawingCanvas::_update_selection()
{
    selection_dirty = false;
    if (!selection_item.is_valid())
    {
        return;
    }
    RenderingServer *rs = RenderingServer::get_singleton();
    rs->canvas_item_clear(selection_item);
    if (document.is_null() || !document->has_selection())
    {
        return;
    }
    Ref<Image> mask = document->get_selection_image();
    if (selection_texture.is_valid() && Vector2i(selection_texture->get_size()) == mask->get_size())
    {
        selection_texture->update(mask);
    }
    else
    {
        selection_texture = ImageTexture::create_from_image(mask);
    }
    rs->canvas_item_add_texture_rect(selection_item, Rect2(Vector2(), document->get_size()), selection_texture->get_rid());
}

void DrawingCanvas::_draw_background()
{
    draw_rect(Rect2(Vector2(), get_size()), view_background);
    if (document.is_null())
    {
        return;
    }
    const Vector2 document_size = document->get_size();
    const Transform2D transform = get_view_transform();
    PackedVector2Array corners;
    corners.push_back(transform.xform(Vector2()));
    corners.push_back(transform.xform(Vector2(document_size.x, 0)));
    corners.push_back(transform.xform(document_size));
    corners.push_back(transform.xform(Vector2(0, document_size.y)));
    const Color background = document->get_background_color();
    if (checkerboard && background.a < 1.0f)
    {
        // Damier fixe à l'écran (carrés de 16 px quel que soit le zoom).
        PackedVector2Array uvs;
        for (const Vector2 &corner : corners)
        {
            uvs.push_back(corner / 32.0f);
        }
        PackedColorArray colors;
        colors.push_back(Color(1, 1, 1));
        draw_polygon(corners, colors, uvs, checker_texture);
    }
    if (background.a > 0.0f)
    {
        draw_colored_polygon(corners, background);
    }
}

void DrawingCanvas::_update_overlay()
{
    overlay_dirty = false;
    if (!overlay_item.is_valid())
    {
        return;
    }
    RenderingServer *rs = RenderingServer::get_singleton();
    rs->canvas_item_clear(overlay_item);
    auto outline = [&](const PackedVector2Array &p_points)
    {
        PackedColorArray black;
        black.push_back(Color(0, 0, 0, 0.8f));
        PackedColorArray white;
        white.push_back(Color(1, 1, 1, 0.95f));
        rs->canvas_item_add_polyline(overlay_item, p_points, black, 3.0f, true);
        rs->canvas_item_add_polyline(overlay_item, p_points, white, 1.0f, true);
    };

    if (drag == DRAG_SELECT || drag == DRAG_LASSO)
    {
        PackedVector2Array points;
        if (drag == DRAG_LASSO)
        {
            for (const Vector2 &point : lasso_points)
            {
                points.push_back(canvas_to_screen(point));
            }
            if (!points.is_empty())
            {
                points.push_back(points[0]);
            }
        }
        else
        {
            const Vector2 a = drag_anchor;
            const Vector2 b = screen_to_canvas(drag_last);
            const Vector2 low = a.min(b);
            const Vector2 high = a.max(b);
            if (drag_tool == TOOL_SELECT_ELLIPSE)
            {
                const Vector2 center = (low + high) * 0.5f;
                const Vector2 radius = (high - low) * 0.5f;
                for (int i = 0; i <= 64; i++)
                {
                    const float t = Math_TAU * i / 64.0f;
                    points.push_back(canvas_to_screen(center + Vector2(std::cos(t) * radius.x, std::sin(t) * radius.y)));
                }
            }
            else
            {
                points.push_back(canvas_to_screen(low));
                points.push_back(canvas_to_screen(Vector2(high.x, low.y)));
                points.push_back(canvas_to_screen(high));
                points.push_back(canvas_to_screen(Vector2(low.x, high.y)));
                points.push_back(points[0]);
            }
        }
        if (points.size() >= 2)
        {
            outline(points);
        }
    }

    if (!hovering || hover_position.x < 0.0f)
    {
        return;
    }
    if (drag == DRAG_EYEDROPPER)
    {
        rs->canvas_item_add_circle(overlay_item, hover_position + Vector2(0, -28), 13.0f, Color(0, 0, 0, 0.8f));
        rs->canvas_item_add_circle(overlay_item, hover_position + Vector2(0, -28), 11.0f, primary_color);
        return;
    }
    const bool brush_tool = tool == TOOL_BRUSH || tool == TOOL_ERASER || drag == DRAG_STROKE;
    const bool navigating = drag == DRAG_PAN || drag == DRAG_ZOOM || drag == DRAG_ROTATE || drag == DRAG_GESTURE;
    if (show_brush_cursor && brush_tool && !navigating)
    {
        const Ref<DrawingBrush> active = drag == DRAG_STROKE && stroke_brush.is_valid() ? stroke_brush : get_active_brush();
        const float radius = MAX(active->get_size() * zoom * 0.5f, 1.5f);
        // Contour de la forme (rondeur et angle du pinceau, rotation et miroir de la vue compris).
        const Transform2D view = get_view_transform();
        const float angle = Math::deg_to_rad(active->get_angle());
        PackedVector2Array points;
        for (int i = 0; i <= 48; i++)
        {
            const float t = Math_TAU * i / 48.0f;
            const Vector2 local = Vector2(std::cos(t), std::sin(t) * active->get_roundness()).rotated(angle) * (radius / zoom);
            points.push_back(hover_position + view.basis_xform(local));
        }
        outline(points);
    }
}

void DrawingCanvas::_update_cursor_shape()
{
    CursorShape shape = CURSOR_CROSS;
    if (drag == DRAG_PAN || drag == DRAG_GESTURE)
    {
        shape = CURSOR_DRAG;
    }
    else if (tool == TOOL_PAN)
    {
        shape = CURSOR_POINTING_HAND;
    }
    set_default_cursor_shape(shape);
}

bool DrawingCanvas::_can_render_shaders() const
{
    DisplayServer *display = DisplayServer::get_singleton();
    return display && display->get_name() != String("headless") && is_inside_tree();
}

// 🖌️ Traits

bool DrawingCanvas::_begin_stroke(bool p_eraser)
{
    if (commit_viewport)
    {
        _finish_commit(Ref<Image>());
    }
    const Ref<DrawingBrush> active = _brush_for(p_eraser ? TOOL_ERASER : TOOL_BRUSH);
    const bool deferred = active->get_material().is_valid() && _can_render_shaders();
    if (!document->begin_stroke(active, primary_color, deferred))
    {
        emit_signal("stroke_rejected"); // calque masqué ou verrouillé
        return false;
    }
    stroke_brush = active;
    stabilizer.clear();
    velocity = 0.0f;
    stroke_waiting_first = true;
    stroke_start_msec = Time::get_singleton()->get_ticks_msec();
    if (deferred && stroke_item.is_valid())
    {
        _clear_stroke_tiles();
        RenderingServer::get_singleton()->canvas_item_set_material(stroke_item, active->get_material()->get_rid());
    }
    emit_signal("stroke_started");
    return true;
}

void DrawingCanvas::_push_sample(const Sample &p_sample)
{
    // Stabilisation : moyenne glissante des derniers points (fenêtre de 1 à 26 points).
    const size_t window = 1 + size_t(stroke_brush->get_stabilization() / 4);
    stabilizer.push_back(p_sample);
    while (stabilizer.size() > window)
    {
        stabilizer.erase(stabilizer.begin());
    }
    Vector2 position;
    float pressure = 0.0f;
    for (const Sample &sample : stabilizer)
    {
        position += sample.position;
        pressure += sample.pressure;
    }
    const float count = float(stabilizer.size());
    document->stroke_to(position / count, pressure / count, p_sample.velocity);
}

void DrawingCanvas::_stroke_input(const Vector2 &p_screen, float p_pressure, float p_velocity)
{
    Sample sample;
    sample.position = screen_to_canvas(p_screen);
    sample.pressure = p_pressure;
    sample.velocity = p_velocity;
    if (stroke_waiting_first)
    {
        // Le premier point attend le premier mouvement : un clic de stylet n'a pas encore de pression.
        stroke_waiting_first = false;
        Sample first = sample;
        first.position = screen_to_canvas(drag_start);
        first.velocity = 0.0f;
        _push_sample(first);
    }
    _push_sample(sample);
}

void DrawingCanvas::_end_stroke()
{
    drag = DRAG_NONE;
    if (document.is_null() || !document->is_stroking())
    {
        stroke_brush.unref();
        return;
    }
    if (stroke_waiting_first)
    {
        // Simple clic : un tampon à pleine pression.
        stroke_waiting_first = false;
        Sample tap;
        tap.position = screen_to_canvas(drag_start);
        tap.pressure = 1.0f;
        stabilizer.clear();
        _push_sample(tap);
    }
    else if (!stabilizer.empty())
    {
        // Le trait rattrape le dernier point (la moyenne glissante est en retard sur le pointeur).
        const Sample last = stabilizer.back();
        const size_t window = 1 + size_t(stroke_brush->get_stabilization() / 4);
        for (size_t i = 1; i < window; i++)
        {
            _push_sample(last);
        }
    }
    if (document->is_stroke_deferred())
    {
        _commit_deferred_stroke();
    }
    else
    {
        document->end_stroke();
    }
    stroke_brush.unref();
    stabilizer.clear();
    overlay_dirty = true;
    emit_signal("stroke_finished");
}

void DrawingCanvas::_cancel_stroke()
{
    drag = DRAG_NONE;
    if (document.is_valid())
    {
        document->cancel_stroke();
    }
    _clear_stroke_tiles();
    stroke_brush.unref();
    stabilizer.clear();
    overlay_dirty = true;
}

void DrawingCanvas::_commit_deferred_stroke()
{
    const Rect2i rect = document->get_stroke_rect();
    if (!rect.has_area() || !is_inside_tree() || stroke_brush.is_null() || stroke_brush->get_material().is_null())
    {
        document->end_stroke();
        _clear_stroke_tiles();
        return;
    }
    // Le trait (tuiles d'aperçu) est rendu une fois à travers le shader, dans un SubViewport à la taille de sa zone.
    any_stroke_tile_dirty = true;
    std::fill(stroke_tiles_dirty.begin(), stroke_tiles_dirty.end(), false);
    const Vector2i first = (rect.position / TILE_SIZE).max(Vector2i());
    const Vector2i last = ((rect.get_end() - Vector2i(1, 1)) / TILE_SIZE).min(tile_grid - Vector2i(1, 1));
    for (int y = first.y; y <= last.y; y++)
    {
        for (int x = first.x; x <= last.x; x++)
        {
            stroke_tiles_dirty[y * tile_grid.x + x] = true;
        }
    }
    _update_stroke_tiles();

    commit_viewport = memnew(SubViewport);
    commit_viewport->set_name("StrokeCommit");
    commit_viewport->set_size(rect.size);
    commit_viewport->set_transparent_background(true);
    commit_viewport->set_disable_3d(true);
    commit_viewport->set_update_mode(SubViewport::UPDATE_ONCE);
    add_child(commit_viewport, false, INTERNAL_MODE_BACK);
    commit_viewport->set_canvas_transform(Transform2D(0.0f, -Vector2(rect.position))); // une fois dans l'arbre (canevas créé)

    RenderingServer *rs = RenderingServer::get_singleton();
    commit_item = rs->canvas_item_create();
    rs->canvas_item_set_parent(commit_item, commit_viewport->find_world_2d()->get_canvas());
    rs->canvas_item_set_material(commit_item, stroke_brush->get_material()->get_rid());
    rs->canvas_item_set_default_texture_filter(commit_item, RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
    rs->canvas_item_set_default_texture_repeat(commit_item, RenderingServer::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);
    for (size_t i = 0; i < stroke_tiles.size(); i++)
    {
        if (stroke_tiles[i].is_valid())
        {
            rs->canvas_item_add_texture_rect(commit_item, Rect2(_tile_rect(int(i))), stroke_tiles[i]->get_rid());
        }
    }
    commit_position = rect.position;
    commit_frames = 0;
    rs->connect("frame_post_draw", callable_mp(this, &DrawingCanvas::_on_commit_rendered), CONNECT_ONE_SHOT | CONNECT_DEFERRED);
}

void DrawingCanvas::_on_commit_rendered()
{
    if (!commit_viewport)
    {
        return;
    }
    Ref<ViewportTexture> texture = commit_viewport->get_texture();
    _finish_commit(texture.is_valid() ? texture->get_image() : Ref<Image>());
}

void DrawingCanvas::_finish_commit(const Ref<Image> &p_image)
{
    RenderingServer *rs = RenderingServer::get_singleton();
    const Callable callback = callable_mp(this, &DrawingCanvas::_on_commit_rendered);
    if (rs->is_connected("frame_post_draw", callback))
    {
        rs->disconnect("frame_post_draw", callback);
    }
    if (document.is_valid() && document->is_stroking())
    {
        if (p_image.is_valid() && !p_image->is_empty())
        {
            // Le rendu sur fond transparent est prémultiplié par l'alpha : retour à l'alpha non prémultiplié.
            Ref<Image> image = p_image->duplicate();
            image->convert(Image::FORMAT_RGBA8);
            PackedByteArray bytes = image->get_data();
            uint8_t *w = bytes.ptrw();
            for (int64_t i = 0; i < bytes.size(); i += 4)
            {
                const int alpha = w[i + 3];
                if (alpha > 0 && alpha < 255)
                {
                    for (int c = 0; c < 3; c++)
                    {
                        w[i + c] = uint8_t(MIN(255, (w[i + c] * 255 + alpha / 2) / alpha));
                    }
                }
            }
            image->set_data(image->get_width(), image->get_height(), false, Image::FORMAT_RGBA8, bytes);
            document->commit_stroke_image(image, commit_position);
        }
        else
        {
            document->end_stroke();
        }
    }
    if (commit_item.is_valid())
    {
        rs->free_rid(commit_item);
        commit_item = RID();
    }
    if (commit_viewport)
    {
        commit_viewport->queue_free();
        commit_viewport = nullptr;
    }
    _clear_stroke_tiles();
}

// 🖱️ Entrées

float DrawingCanvas::_compute_velocity(const Vector2 &p_screen, const Vector2 &p_event_velocity)
{
    const uint64_t now = Time::get_singleton()->get_ticks_usec();
    float instant = velocity;
    if (p_event_velocity.length_squared() > 0.0f)
    {
        instant = p_event_velocity.length();
    }
    else if (last_motion_usec > 0)
    {
        const double seconds = double(now - last_motion_usec) / 1e6;
        if (seconds > 0.0005 && seconds < 0.25)
        {
            instant = float((p_screen - hover_position).length() / seconds);
        }
    }
    last_motion_usec = now;
    velocity = Math::lerp(velocity, instant, 0.4f);
    return velocity;
}

DrawingDocument::SelectionMode DrawingCanvas::_modifier_selection_mode() const
{
    Input *input = Input::get_singleton();
    const bool shift = input->is_key_pressed(KEY_SHIFT);
    const bool alt = input->is_key_pressed(KEY_ALT);
    if (shift && alt)
    {
        return DrawingDocument::SELECTION_INTERSECT;
    }
    if (shift)
    {
        return DrawingDocument::SELECTION_ADD;
    }
    if (alt)
    {
        return DrawingDocument::SELECTION_SUBTRACT;
    }
    return selection_mode;
}

void DrawingCanvas::_pick(const Vector2 &p_screen)
{
    const Vector2 position = screen_to_canvas(p_screen).floor();
    if (!Rect2i(Vector2i(), document->get_size()).has_point(Vector2i(position)))
    {
        return;
    }
    const Color color = document->pick_color(Vector2i(position), eyedropper_all_layers);
    if (color.a <= 0.0f)
    {
        return; // transparent : la couleur courante est gardée
    }
    set_primary_color(color);
    emit_signal("color_picked", color);
}

void DrawingCanvas::_press(const Vector2 &p_screen, bool /*p_from_touch*/)
{
    if (drag != DRAG_NONE || document.is_null())
    {
        return;
    }
    if (document->get_layer_count() == 0)
    {
        document->add_layer();
        document->clear_history();
    }
    drag_start = p_screen;
    drag_last = p_screen;
    hover_position = p_screen;
    Input *input = Input::get_singleton();
    const bool space = input->is_key_pressed(KEY_SPACE);
    const bool shift = input->is_key_pressed(KEY_SHIFT);
    const bool ctrl = input->is_key_pressed(KEY_CTRL) || input->is_key_pressed(KEY_META);
    const bool alt = input->is_key_pressed(KEY_ALT);
    if (space)
    {
        drag = shift ? DRAG_ROTATE : (ctrl ? DRAG_ZOOM : DRAG_PAN);
        drag_anchor = screen_to_canvas(p_screen);
        _update_cursor_shape();
        overlay_dirty = true;
        return;
    }

    Tool effective = tool;
    if (pen_eraser && last_pen_inverted && (effective == TOOL_BRUSH || effective == TOOL_ERASER))
    {
        effective = TOOL_ERASER;
    }
    if (alt && (effective == TOOL_BRUSH || effective == TOOL_ERASER || effective == TOOL_FILL))
    {
        effective = TOOL_EYEDROPPER;
    }
    drag_tool = effective;
    const Vector2 canvas = screen_to_canvas(p_screen);
    switch (effective)
    {
    case TOOL_BRUSH:
    case TOOL_ERASER:
        if (_begin_stroke(effective == TOOL_ERASER))
        {
            drag = DRAG_STROKE;
        }
        break;
    case TOOL_FILL:
        document->fill(Vector2i(canvas.floor()), primary_color, fill_tolerance, fill_reference, fill_gap_closing, fill_expand, fill_contiguous);
        break;
    case TOOL_EYEDROPPER:
        drag = DRAG_EYEDROPPER;
        _pick(p_screen);
        break;
    case TOOL_SELECT_RECT:
    case TOOL_SELECT_ELLIPSE:
        drag = DRAG_SELECT;
        drag_anchor = canvas;
        drag_selection_mode = _modifier_selection_mode();
        break;
    case TOOL_SELECT_LASSO:
        drag = DRAG_LASSO;
        lasso_points.clear();
        lasso_points.push_back(canvas);
        drag_selection_mode = _modifier_selection_mode();
        break;
    case TOOL_SELECT_WAND:
        document->select_by_color(Vector2i(canvas.floor()), wand_tolerance, fill_reference, true, _modifier_selection_mode());
        break;
    case TOOL_PAN:
        drag = DRAG_PAN;
        break;
    default:
        break;
    }
    _update_cursor_shape();
    overlay_dirty = true;
}

void DrawingCanvas::_motion(const Vector2 &p_screen, float p_pressure, float p_velocity)
{
    switch (drag)
    {
    case DRAG_STROKE:
        _stroke_input(p_screen, p_pressure, p_velocity);
        break;
    case DRAG_PAN:
        set_view_offset(view_offset + (p_screen - drag_last));
        break;
    case DRAG_ZOOM:
    {
        const Vector2 delta = p_screen - drag_last;
        zoom_at(std::pow(1.01f, delta.x - delta.y), drag_start);
        break;
    }
    case DRAG_ROTATE:
    {
        const Vector2 center = get_size() * 0.5f;
        const float angle = (drag_last - center).angle_to(p_screen - center);
        rotate_at(Math::rad_to_deg(angle), center);
        break;
    }
    case DRAG_EYEDROPPER:
        _pick(p_screen);
        break;
    case DRAG_LASSO:
    {
        const Vector2 canvas = screen_to_canvas(p_screen);
        if (lasso_points.is_empty() || canvas.distance_to(lasso_points[lasso_points.size() - 1]) * zoom >= 2.0f)
        {
            lasso_points.push_back(canvas);
        }
        break;
    }
    default:
        break;
    }
    drag_last = p_screen;
    overlay_dirty = true;
}

void DrawingCanvas::_release(const Vector2 &p_screen)
{
    switch (drag)
    {
    case DRAG_STROKE:
        _end_stroke();
        break;
    case DRAG_SELECT:
    {
        drag_last = p_screen;
        if ((p_screen - drag_start).length() < 3.0f)
        {
            if (drag_selection_mode == DrawingDocument::SELECTION_REPLACE)
            {
                document->deselect(); // simple clic : désélection
            }
            break;
        }
        const Vector2 b = screen_to_canvas(p_screen);
        const Vector2i low = Vector2i(drag_anchor.min(b).round());
        const Vector2i high = Vector2i(drag_anchor.max(b).round());
        const Rect2i rect(low, high - low);
        if (drag_tool == TOOL_SELECT_ELLIPSE)
        {
            document->select_ellipse(rect, drag_selection_mode);
        }
        else
        {
            document->select_rect(rect, drag_selection_mode);
        }
        break;
    }
    case DRAG_LASSO:
        lasso_points.push_back(screen_to_canvas(p_screen));
        if (lasso_points.size() >= 3)
        {
            document->select_polygon(lasso_points, drag_selection_mode);
        }
        else if (drag_selection_mode == DrawingDocument::SELECTION_REPLACE)
        {
            document->deselect();
        }
        lasso_points.clear();
        break;
    default:
        break;
    }
    drag = DRAG_NONE;
    _update_cursor_shape();
    overlay_dirty = true;
}

void DrawingCanvas::_handle_touch(const Ref<InputEvent> &p_event)
{
    Ref<InputEventScreenTouch> touch = p_event;
    Ref<InputEventScreenDrag> screen_drag = p_event;
    auto start_gesture = [this]()
    {
        Vector2 points[2];
        int found = 0;
        for (const KeyValue<int32_t, Vector2> &entry : touches)
        {
            if (found < 2)
            {
                points[found++] = entry.value;
            }
        }
        const Vector2 middle = (points[0] + points[1]) * 0.5f;
        gesture_start_distance = MAX(points[0].distance_to(points[1]), 1.0f);
        gesture_start_angle = (points[1] - points[0]).angle();
        drag_start_zoom = zoom;
        drag_start_rotation = view_rotation;
        drag_anchor = screen_to_canvas(middle);
    };

    if (touch.is_valid())
    {
        const int32_t index = touch->get_index();
        const Vector2 position = touch->get_position();
        if (touch->is_pressed())
        {
            touches[index] = position;
            if (touches.size() == 1 && !touch_locked && drag == DRAG_NONE)
            {
                touch_drawing_index = index;
                grab_focus();
                last_pen_inverted = false;
                hover_position = position;
                hovering = true;
                velocity = 0.0f;
                last_motion_usec = 0;
                _press(position, true);
            }
            else if (touches.size() == 2)
            {
                // Deuxième doigt : geste de navigation. Un trait tout juste commencé était le premier doigt du geste.
                if (drag == DRAG_STROKE)
                {
                    if (Time::get_singleton()->get_ticks_msec() - stroke_start_msec < 300)
                    {
                        _cancel_stroke();
                    }
                    else
                    {
                        _end_stroke();
                    }
                }
                lasso_points.clear();
                touch_drawing_index = -1;
                touch_locked = true;
                drag = DRAG_GESTURE;
                start_gesture();
            }
        }
        else
        {
            touches.erase(index);
            if (index == touch_drawing_index)
            {
                touch_drawing_index = -1;
                _release(position);
                hovering = false;
            }
            if (drag == DRAG_GESTURE && touches.size() < 2)
            {
                drag = DRAG_NONE;
            }
            if (touches.is_empty())
            {
                touch_locked = false;
            }
        }
        _update_cursor_shape();
        overlay_dirty = true;
        return;
    }

    if (screen_drag.is_valid())
    {
        const int32_t index = screen_drag->get_index();
        const Vector2 position = screen_drag->get_position();
        if (touches.has(index))
        {
            touches[index] = position;
        }
        if (drag == DRAG_GESTURE && touches.size() >= 2)
        {
            Vector2 points[2];
            int found = 0;
            for (const KeyValue<int32_t, Vector2> &entry : touches)
            {
                if (found < 2)
                {
                    points[found++] = entry.value;
                }
            }
            const Vector2 middle = (points[0] + points[1]) * 0.5f;
            zoom = Math::clamp(drag_start_zoom * points[0].distance_to(points[1]) / gesture_start_distance, min_zoom, max_zoom);
            if (touch_rotation)
            {
                view_rotation = Math::wrapf(drag_start_rotation + Math::rad_to_deg((points[1] - points[0]).angle() - gesture_start_angle), -180.0f, 180.0f);
            }
            auto_fit = false;
            _anchor(drag_anchor, middle);
        }
        else if (index == touch_drawing_index)
        {
            float pressure = screen_drag->get_pressure();
            if (pressure <= 0.0f)
            {
                pressure = 1.0f;
            }
            const float speed = _compute_velocity(position, screen_drag->get_velocity());
            hover_position = position;
            _motion(position, pressure, speed);
        }
    }
}

void DrawingCanvas::_gui_input(const Ref<InputEvent> &p_event)
{
    if (document.is_null())
    {
        return;
    }
    Ref<InputEventKey> key = p_event;
    if (key.is_valid())
    {
        if (shortcuts_enabled && _handle_shortcut(key))
        {
            accept_event();
        }
        else if (key->get_keycode() == KEY_SPACE)
        {
            accept_event(); // modificateur de navigation : ne doit pas activer un bouton
        }
        return;
    }
    if (Object::cast_to<InputEventScreenTouch>(*p_event) || Object::cast_to<InputEventScreenDrag>(*p_event))
    {
        if (touch_gestures)
        {
            _handle_touch(p_event);
            accept_event();
        }
        return;
    }
    Ref<InputEventMagnifyGesture> magnify = p_event;
    if (magnify.is_valid())
    {
        zoom_at(magnify->get_factor(), magnify->get_position());
        accept_event();
        return;
    }
    Ref<InputEventPanGesture> pan = p_event;
    if (pan.is_valid())
    {
        set_view_offset(view_offset - pan->get_delta() * 16.0f);
        accept_event();
        return;
    }

    Ref<InputEventMouseButton> button = p_event;
    if (button.is_valid())
    {
        if (touch_gestures && button->get_device() == InputEvent::DEVICE_ID_EMULATION)
        {
            return; // souris émulée depuis le tactile : les touchers sont traités directement
        }
        const Vector2 position = button->get_position();
        if (button->is_pressed())
        {
            switch (button->get_button_index())
            {
            case MOUSE_BUTTON_WHEEL_UP:
            case MOUSE_BUTTON_WHEEL_DOWN:
            {
                const float steps = button->get_factor() > 0.0f ? button->get_factor() : 1.0f;
                const float direction = button->get_button_index() == MOUSE_BUTTON_WHEEL_UP ? 1.0f : -1.0f;
                if (button->is_alt_pressed())
                {
                    rotate_at(15.0f * direction, position);
                }
                else
                {
                    zoom_at(std::pow(1.15f, steps * direction), position);
                }
                accept_event();
                break;
            }
            case MOUSE_BUTTON_MIDDLE:
                if (drag == DRAG_NONE)
                {
                    drag = DRAG_PAN;
                    drag_start = position;
                    drag_last = position;
                    _update_cursor_shape();
                }
                accept_event();
                break;
            case MOUSE_BUTTON_LEFT:
                grab_focus();
                hovering = true;
                _press(position, false);
                accept_event();
                break;
            default:
                break;
            }
        }
        else if (button->get_button_index() == MOUSE_BUTTON_LEFT || (button->get_button_index() == MOUSE_BUTTON_MIDDLE && drag == DRAG_PAN))
        {
            _release(position);
            accept_event();
        }
        return;
    }

    Ref<InputEventMouseMotion> motion = p_event;
    if (motion.is_valid())
    {
        if (touch_gestures && motion->get_device() == InputEvent::DEVICE_ID_EMULATION)
        {
            return;
        }
        const Vector2 position = motion->get_position();
        last_pen_inverted = motion->get_pen_inverted();
        float pressure = motion->get_pressure();
        if (pressure <= 0.0f && (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)))
        {
            pressure = 1.0f; // souris : pas de pression
        }
        const float speed = _compute_velocity(position, motion->get_velocity());
        hover_position = position;
        hovering = true;
        if (drag != DRAG_NONE)
        {
            _motion(position, pressure, speed);
            accept_event();
        }
        overlay_dirty = true;
    }
}

void DrawingCanvas::_unhandled_key_input(const Ref<InputEvent> &p_event)
{
    // Raccourcis quand la toile est survolée sans avoir le focus (le focus est sur un autre Control qui n'a pas
    // utilisé la touche).
    Ref<InputEventKey> key = p_event;
    if (key.is_null() || !shortcuts_enabled || !hovering || has_focus() || !is_visible_in_tree() || document.is_null())
    {
        return;
    }
    if (_handle_shortcut(key))
    {
        get_viewport()->set_input_as_handled();
    }
}

bool DrawingCanvas::_handle_shortcut(const Ref<InputEventKey> &p_key)
{
    if (!p_key->is_pressed())
    {
        return false;
    }
    const Key code = p_key->get_keycode();
    const bool shift = p_key->is_shift_pressed();
    const Vector2 center = get_size() * 0.5f;
    if (p_key->is_command_or_control_pressed())
    {
        switch (code)
        {
        case KEY_Z:
            shift ? document->redo() : document->undo();
            return true;
        case KEY_Y:
            document->redo();
            return true;
        case KEY_A:
            document->select_all();
            return true;
        case KEY_D:
            document->deselect();
            return true;
        case KEY_I:
            if (shift)
            {
                document->invert_selection();
                return true;
            }
            return false;
        case KEY_0:
        case KEY_KP_0:
            fit_to_view();
            return true;
        case KEY_1:
        case KEY_KP_1:
            actual_size();
            return true;
        case KEY_EQUAL:
        case KEY_PLUS:
        case KEY_KP_ADD:
            zoom_at(1.25f, center);
            return true;
        case KEY_MINUS:
        case KEY_KP_SUBTRACT:
            zoom_at(0.8f, center);
            return true;
        default:
            return false;
        }
    }
    if (p_key->is_alt_pressed())
    {
        return false;
    }
    const bool echo = p_key->is_echo();
    switch (code)
    {
    case KEY_BRACKETLEFT:
    case KEY_BRACKETRIGHT:
    {
        const Ref<DrawingBrush> active = get_active_brush();
        const float current = active->get_size();
        active->set_size(code == KEY_BRACKETLEFT ? MIN(current * 0.9f, current - 1.0f) : MAX(current * 1.1f, current + 1.0f));
        overlay_dirty = true;
        return true;
    }
    case KEY_MINUS:
        rotate_at(-15.0f, center);
        return true;
    case KEY_EQUAL:
    case KEY_ASCIICIRCUM:
        rotate_at(15.0f, center);
        return true;
    default:
        break;
    }
    if (echo)
    {
        return false;
    }
    switch (code)
    {
    case KEY_B:
        set_tool(TOOL_BRUSH);
        return true;
    case KEY_E:
        set_tool(TOOL_ERASER);
        return true;
    case KEY_G:
        set_tool(TOOL_FILL);
        return true;
    case KEY_I:
        set_tool(TOOL_EYEDROPPER);
        return true;
    case KEY_M:
        set_tool(tool == TOOL_SELECT_RECT ? TOOL_SELECT_ELLIPSE : TOOL_SELECT_RECT);
        return true;
    case KEY_L:
        set_tool(TOOL_SELECT_LASSO);
        return true;
    case KEY_W:
        set_tool(TOOL_SELECT_WAND);
        return true;
    case KEY_H:
        set_tool(TOOL_PAN);
        return true;
    case KEY_X:
        swap_colors();
        return true;
    case KEY_DELETE:
    case KEY_BACKSPACE:
        document->clear_selection_content();
        return true;
    case KEY_ESCAPE:
        if (drag == DRAG_SELECT || drag == DRAG_LASSO)
        {
            drag = DRAG_NONE;
            lasso_points.clear();
            overlay_dirty = true;
            return true;
        }
        return false;
    default:
        return false;
    }
}

// 🔗 Liaisons

void DrawingCanvas::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_document", "document"), &DrawingCanvas::set_document);
    ClassDB::bind_method(D_METHOD("get_document"), &DrawingCanvas::get_document);
    ClassDB::bind_method(D_METHOD("set_default_document_size", "size"), &DrawingCanvas::set_default_document_size);
    ClassDB::bind_method(D_METHOD("get_default_document_size"), &DrawingCanvas::get_default_document_size);

    ClassDB::bind_method(D_METHOD("set_tool", "tool"), &DrawingCanvas::set_tool);
    ClassDB::bind_method(D_METHOD("get_tool"), &DrawingCanvas::get_tool);
    ClassDB::bind_method(D_METHOD("set_brush", "brush"), &DrawingCanvas::set_brush);
    ClassDB::bind_method(D_METHOD("get_brush"), &DrawingCanvas::get_brush);
    ClassDB::bind_method(D_METHOD("set_eraser", "eraser"), &DrawingCanvas::set_eraser);
    ClassDB::bind_method(D_METHOD("get_eraser"), &DrawingCanvas::get_eraser);
    ClassDB::bind_method(D_METHOD("get_active_brush"), &DrawingCanvas::get_active_brush);
    ClassDB::bind_method(D_METHOD("set_primary_color", "color"), &DrawingCanvas::set_primary_color);
    ClassDB::bind_method(D_METHOD("get_primary_color"), &DrawingCanvas::get_primary_color);
    ClassDB::bind_method(D_METHOD("set_secondary_color", "color"), &DrawingCanvas::set_secondary_color);
    ClassDB::bind_method(D_METHOD("get_secondary_color"), &DrawingCanvas::get_secondary_color);
    ClassDB::bind_method(D_METHOD("swap_colors"), &DrawingCanvas::swap_colors);
    ClassDB::bind_method(D_METHOD("set_fill_tolerance", "tolerance"), &DrawingCanvas::set_fill_tolerance);
    ClassDB::bind_method(D_METHOD("get_fill_tolerance"), &DrawingCanvas::get_fill_tolerance);
    ClassDB::bind_method(D_METHOD("set_fill_gap_closing", "pixels"), &DrawingCanvas::set_fill_gap_closing);
    ClassDB::bind_method(D_METHOD("get_fill_gap_closing"), &DrawingCanvas::get_fill_gap_closing);
    ClassDB::bind_method(D_METHOD("set_fill_expand", "pixels"), &DrawingCanvas::set_fill_expand);
    ClassDB::bind_method(D_METHOD("get_fill_expand"), &DrawingCanvas::get_fill_expand);
    ClassDB::bind_method(D_METHOD("set_fill_reference", "reference"), &DrawingCanvas::set_fill_reference);
    ClassDB::bind_method(D_METHOD("get_fill_reference"), &DrawingCanvas::get_fill_reference);
    ClassDB::bind_method(D_METHOD("set_fill_contiguous", "contiguous"), &DrawingCanvas::set_fill_contiguous);
    ClassDB::bind_method(D_METHOD("get_fill_contiguous"), &DrawingCanvas::get_fill_contiguous);
    ClassDB::bind_method(D_METHOD("set_selection_mode", "mode"), &DrawingCanvas::set_selection_mode);
    ClassDB::bind_method(D_METHOD("get_selection_mode"), &DrawingCanvas::get_selection_mode);
    ClassDB::bind_method(D_METHOD("set_wand_tolerance", "tolerance"), &DrawingCanvas::set_wand_tolerance);
    ClassDB::bind_method(D_METHOD("get_wand_tolerance"), &DrawingCanvas::get_wand_tolerance);
    ClassDB::bind_method(D_METHOD("set_eyedropper_all_layers", "all"), &DrawingCanvas::set_eyedropper_all_layers);
    ClassDB::bind_method(D_METHOD("get_eyedropper_all_layers"), &DrawingCanvas::get_eyedropper_all_layers);

    ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &DrawingCanvas::set_zoom);
    ClassDB::bind_method(D_METHOD("get_zoom"), &DrawingCanvas::get_zoom);
    ClassDB::bind_method(D_METHOD("set_min_zoom", "zoom"), &DrawingCanvas::set_min_zoom);
    ClassDB::bind_method(D_METHOD("get_min_zoom"), &DrawingCanvas::get_min_zoom);
    ClassDB::bind_method(D_METHOD("set_max_zoom", "zoom"), &DrawingCanvas::set_max_zoom);
    ClassDB::bind_method(D_METHOD("get_max_zoom"), &DrawingCanvas::get_max_zoom);
    ClassDB::bind_method(D_METHOD("set_view_offset", "offset"), &DrawingCanvas::set_view_offset);
    ClassDB::bind_method(D_METHOD("get_view_offset"), &DrawingCanvas::get_view_offset);
    ClassDB::bind_method(D_METHOD("set_view_rotation", "degrees"), &DrawingCanvas::set_view_rotation);
    ClassDB::bind_method(D_METHOD("get_view_rotation"), &DrawingCanvas::get_view_rotation);
    ClassDB::bind_method(D_METHOD("set_flip_h", "flip"), &DrawingCanvas::set_flip_h);
    ClassDB::bind_method(D_METHOD("get_flip_h"), &DrawingCanvas::get_flip_h);
    ClassDB::bind_method(D_METHOD("zoom_at", "factor", "screen_position"), &DrawingCanvas::zoom_at);
    ClassDB::bind_method(D_METHOD("rotate_at", "degrees", "screen_position"), &DrawingCanvas::rotate_at);
    ClassDB::bind_method(D_METHOD("fit_to_view"), &DrawingCanvas::fit_to_view);
    ClassDB::bind_method(D_METHOD("actual_size"), &DrawingCanvas::actual_size);
    ClassDB::bind_method(D_METHOD("reset_rotation"), &DrawingCanvas::reset_rotation);
    ClassDB::bind_method(D_METHOD("get_view_transform"), &DrawingCanvas::get_view_transform);
    ClassDB::bind_method(D_METHOD("canvas_to_screen", "canvas_position"), &DrawingCanvas::canvas_to_screen);
    ClassDB::bind_method(D_METHOD("screen_to_canvas", "screen_position"), &DrawingCanvas::screen_to_canvas);

    ClassDB::bind_method(D_METHOD("set_shortcuts_enabled", "enabled"), &DrawingCanvas::set_shortcuts_enabled);
    ClassDB::bind_method(D_METHOD("get_shortcuts_enabled"), &DrawingCanvas::get_shortcuts_enabled);
    ClassDB::bind_method(D_METHOD("set_touch_gestures", "enabled"), &DrawingCanvas::set_touch_gestures);
    ClassDB::bind_method(D_METHOD("get_touch_gestures"), &DrawingCanvas::get_touch_gestures);
    ClassDB::bind_method(D_METHOD("set_touch_rotation", "enabled"), &DrawingCanvas::set_touch_rotation);
    ClassDB::bind_method(D_METHOD("get_touch_rotation"), &DrawingCanvas::get_touch_rotation);
    ClassDB::bind_method(D_METHOD("set_pen_eraser", "enabled"), &DrawingCanvas::set_pen_eraser);
    ClassDB::bind_method(D_METHOD("get_pen_eraser"), &DrawingCanvas::get_pen_eraser);

    ClassDB::bind_method(D_METHOD("set_view_background", "color"), &DrawingCanvas::set_view_background);
    ClassDB::bind_method(D_METHOD("get_view_background"), &DrawingCanvas::get_view_background);
    ClassDB::bind_method(D_METHOD("set_checkerboard", "enabled"), &DrawingCanvas::set_checkerboard);
    ClassDB::bind_method(D_METHOD("get_checkerboard"), &DrawingCanvas::get_checkerboard);
    ClassDB::bind_method(D_METHOD("set_show_brush_cursor", "enabled"), &DrawingCanvas::set_show_brush_cursor);
    ClassDB::bind_method(D_METHOD("get_show_brush_cursor"), &DrawingCanvas::get_show_brush_cursor);

    ClassDB::bind_method(D_METHOD("is_dragging"), &DrawingCanvas::is_dragging);
    ClassDB::bind_method(D_METHOD("is_stroking"), &DrawingCanvas::is_stroking);
    ClassDB::bind_method(D_METHOD("get_pointer_position"), &DrawingCanvas::get_pointer_position);

    ADD_GROUP(U"📄 Document", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "document", PROPERTY_HINT_RESOURCE_TYPE, "DrawingDocument"), "set_document", "get_document");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "default_document_size", PROPERTY_HINT_NONE, "suffix:px"), "set_default_document_size", "get_default_document_size");

    ADD_GROUP(U"🛠️ Outils", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "tool", PROPERTY_HINT_ENUM, "Brush,Eraser,Fill,Eyedropper,Select Rect,Select Ellipse,Select Lasso,Select Wand,Pan"), "set_tool", "get_tool");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "brush", PROPERTY_HINT_RESOURCE_TYPE, "DrawingBrush"), "set_brush", "get_brush");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "eraser", PROPERTY_HINT_RESOURCE_TYPE, "DrawingBrush"), "set_eraser", "get_eraser");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "primary_color"), "set_primary_color", "get_primary_color");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "secondary_color"), "set_secondary_color", "get_secondary_color");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fill_tolerance", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_fill_tolerance", "get_fill_tolerance");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "fill_gap_closing", PROPERTY_HINT_RANGE, "0,64,1,suffix:px"), "set_fill_gap_closing", "get_fill_gap_closing");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "fill_expand", PROPERTY_HINT_RANGE, "-32,32,1,suffix:px"), "set_fill_expand", "get_fill_expand");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "fill_reference", PROPERTY_HINT_ENUM, "Layer,All Layers"), "set_fill_reference", "get_fill_reference");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "fill_contiguous"), "set_fill_contiguous", "get_fill_contiguous");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "selection_mode", PROPERTY_HINT_ENUM, "Replace,Add,Subtract,Intersect"), "set_selection_mode", "get_selection_mode");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wand_tolerance", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_wand_tolerance", "get_wand_tolerance");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "eyedropper_all_layers"), "set_eyedropper_all_layers", "get_eyedropper_all_layers");

    ADD_GROUP(U"🧭 Vue", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "zoom", PROPERTY_HINT_RANGE, "0.001,64,0.001,exp", PROPERTY_USAGE_EDITOR), "set_zoom", "get_zoom");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_zoom", PROPERTY_HINT_RANGE, "0.001,1,0.001,exp"), "set_min_zoom", "get_min_zoom");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_zoom", PROPERTY_HINT_RANGE, "1,256,0.1,exp"), "set_max_zoom", "get_max_zoom");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "view_offset", PROPERTY_HINT_NONE, "suffix:px", PROPERTY_USAGE_EDITOR), "set_view_offset", "get_view_offset");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "view_rotation", PROPERTY_HINT_RANGE, "-180,180,0.1,degrees"), "set_view_rotation", "get_view_rotation");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_h"), "set_flip_h", "get_flip_h");

    ADD_GROUP(U"⌨️ Entrées", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shortcuts_enabled"), "set_shortcuts_enabled", "get_shortcuts_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "touch_gestures"), "set_touch_gestures", "get_touch_gestures");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "touch_rotation"), "set_touch_rotation", "get_touch_rotation");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "pen_eraser"), "set_pen_eraser", "get_pen_eraser");

    ADD_GROUP(U"🎨 Affichage", "");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "view_background"), "set_view_background", "get_view_background");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "checkerboard"), "set_checkerboard", "get_checkerboard");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_brush_cursor"), "set_show_brush_cursor", "get_show_brush_cursor");

    ADD_SIGNAL(MethodInfo("document_changed"));
    ADD_SIGNAL(MethodInfo("tool_changed", PropertyInfo(Variant::INT, "tool")));
    ADD_SIGNAL(MethodInfo("primary_color_changed", PropertyInfo(Variant::COLOR, "color")));
    ADD_SIGNAL(MethodInfo("secondary_color_changed", PropertyInfo(Variant::COLOR, "color")));
    ADD_SIGNAL(MethodInfo("color_picked", PropertyInfo(Variant::COLOR, "color")));
    ADD_SIGNAL(MethodInfo("view_changed"));
    ADD_SIGNAL(MethodInfo("stroke_started"));
    ADD_SIGNAL(MethodInfo("stroke_finished"));
    ADD_SIGNAL(MethodInfo("stroke_rejected"));

    BIND_ENUM_CONSTANT(TOOL_BRUSH);
    BIND_ENUM_CONSTANT(TOOL_ERASER);
    BIND_ENUM_CONSTANT(TOOL_FILL);
    BIND_ENUM_CONSTANT(TOOL_EYEDROPPER);
    BIND_ENUM_CONSTANT(TOOL_SELECT_RECT);
    BIND_ENUM_CONSTANT(TOOL_SELECT_ELLIPSE);
    BIND_ENUM_CONSTANT(TOOL_SELECT_LASSO);
    BIND_ENUM_CONSTANT(TOOL_SELECT_WAND);
    BIND_ENUM_CONSTANT(TOOL_PAN);
    BIND_ENUM_CONSTANT(TOOL_MAX);
}
