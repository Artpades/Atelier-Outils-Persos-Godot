#include "drawing_document.hpp"
#include "drawing_pixels.hpp"

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace godot;
namespace px = drawing_pixels;

static PackedByteArray copy_region(const PackedByteArray &p_source, int p_width, const Rect2i &p_rect)
{
    PackedByteArray bytes;
    bytes.resize(int64_t(p_rect.size.x) * p_rect.size.y * 4);
    uint8_t *w = bytes.ptrw();
    const uint8_t *r = p_source.ptr();
    for (int y = 0; y < p_rect.size.y; y++)
    {
        memcpy(w + size_t(y) * p_rect.size.x * 4, r + (size_t(p_rect.position.y + y) * p_width + p_rect.position.x) * 4, size_t(p_rect.size.x) * 4);
    }
    return bytes;
}

static Rect2i merge_rect(const Rect2i &p_a, const Rect2i &p_b)
{
    if (!p_a.has_area())
    {
        return p_b;
    }
    return p_b.has_area() ? p_a.merge(p_b) : p_a;
}

DrawingDocument::DrawingDocument() {}

Ref<DrawingDocument> DrawingDocument::create(const Vector2i &p_size, const Color &p_background)
{
    Ref<DrawingDocument> document;
    document.instantiate();
    document->set_size(p_size);
    document->background_color = p_background;
    document->add_layer();
    document->clear_history();
    return document;
}

Ref<DrawingDocument> DrawingDocument::create_from_image(const Ref<Image> &p_image)
{
    ERR_FAIL_COND_V(p_image.is_null() || p_image->is_empty(), Ref<DrawingDocument>());
    Ref<DrawingDocument> document;
    document.instantiate();
    document->set_size(p_image->get_size());
    document->background_color = Color(1, 1, 1, 0);
    Ref<DrawingLayer> layer = document->add_layer();
    layer->set_image(p_image);
    document->clear_history();
    return document;
}

// 📐 Document

void DrawingDocument::set_size(const Vector2i &p_size)
{
    const Vector2i new_size = p_size.max(Vector2i(1, 1)).min(Vector2i(16384, 16384));
    if (new_size == size && composite.size() == int64_t(size.x) * size.y * 4)
    {
        return;
    }
    if (stroke.active)
    {
        cancel_stroke();
    }
    size = new_size;
    for (const Ref<DrawingLayer> &layer : layers)
    {
        layer->resize(size);
    }
    selection = PackedByteArray();
    selection_active = false;
    stroke_mask.clear();
    composite_valid = false;
    clear_history();
    _layers_modified();
    emit_signal("selection_changed");
    emit_changed();
}

void DrawingDocument::set_background_color(const Color &p_color)
{
    background_color = p_color;
    emit_changed();
}

void DrawingDocument::set_history_limit(int p_limit)
{
    history_limit = MAX(p_limit, 1);
    while (int(undo_stack.size()) > history_limit)
    {
        undo_stack.erase(undo_stack.begin());
    }
}

// 🗂️ Calques

String DrawingDocument::_next_layer_name()
{
    layer_counter++;
    return String(U"Calque ") + String::num_int64(layer_counter);
}

void DrawingDocument::_connect_layer(const Ref<DrawingLayer> &p_layer)
{
    const Callable callback = callable_mp(this, &DrawingDocument::_on_layer_changed);
    if (!p_layer->is_connected("changed", callback))
    {
        p_layer->connect("changed", callback);
    }
}

void DrawingDocument::_disconnect_layer(const Ref<DrawingLayer> &p_layer)
{
    const Callable callback = callable_mp(this, &DrawingDocument::_on_layer_changed);
    if (p_layer->is_connected("changed", callback))
    {
        p_layer->disconnect("changed", callback);
    }
}

void DrawingDocument::_on_layer_changed()
{
    for (const Ref<DrawingLayer> &layer : layers)
    {
        if (layer->get_size() != size)
        {
            layer->resize(size); // contenu remplacé par set_image : ramené à la taille du document
        }
    }
    _layers_modified();
}

void DrawingDocument::_layers_modified()
{
    composite_valid = false;
    _ensure_composite();
    emit_signal("region_changed", _bounds());
    emit_signal("layers_changed");
}

void DrawingDocument::set_layers(const TypedArray<DrawingLayer> &p_layers)
{
    if (stroke.active)
    {
        cancel_stroke();
    }
    for (const Ref<DrawingLayer> &layer : layers)
    {
        _disconnect_layer(layer);
    }
    layers.clear();
    for (int64_t i = 0; i < p_layers.size(); i++)
    {
        Ref<DrawingLayer> layer = p_layers[i];
        if (layer.is_null() || find_layer(layer) >= 0)
        {
            continue;
        }
        layer->resize(size);
        _connect_layer(layer);
        layers.push_back(layer);
    }
    layer_counter = int(layers.size());
    current_layer = CLAMP(current_layer, 0, MAX(int(layers.size()) - 1, 0));
    clear_history();
    _layers_modified();
}

TypedArray<DrawingLayer> DrawingDocument::get_layers() const
{
    TypedArray<DrawingLayer> result;
    for (const Ref<DrawingLayer> &layer : layers)
    {
        result.push_back(layer);
    }
    return result;
}

Ref<DrawingLayer> DrawingDocument::get_layer(int p_index) const
{
    if (p_index < 0 || p_index >= int(layers.size()))
    {
        return Ref<DrawingLayer>();
    }
    return layers[p_index];
}

int DrawingDocument::find_layer(const Ref<DrawingLayer> &p_layer) const
{
    for (size_t i = 0; i < layers.size(); i++)
    {
        if (layers[i] == p_layer)
        {
            return int(i);
        }
    }
    return -1;
}

void DrawingDocument::set_current_layer(int p_index)
{
    const int index = CLAMP(p_index, 0, MAX(int(layers.size()) - 1, 0));
    if (index == current_layer)
    {
        return;
    }
    if (stroke.active)
    {
        end_stroke();
    }
    current_layer = index;
    emit_signal("current_layer_changed", current_layer);
}

Ref<DrawingLayer> DrawingDocument::add_layer(const String &p_name, int p_index)
{
    Ref<DrawingLayer> layer = DrawingLayer::create(size, p_name.is_empty() ? _next_layer_name() : p_name);
    insert_layer(layer, p_index);
    return layer;
}

void DrawingDocument::insert_layer(const Ref<DrawingLayer> &p_layer, int p_index)
{
    ERR_FAIL_COND(p_layer.is_null());
    ERR_FAIL_COND_MSG(find_layer(p_layer) >= 0, U"Ce calque appartient déjà au document.");
    if (stroke.active)
    {
        end_stroke();
    }
    p_layer->resize(size);
    const int index = p_index < 0 ? (layers.empty() ? 0 : current_layer + 1) : CLAMP(p_index, 0, int(layers.size()));
    layers.insert(layers.begin() + index, p_layer);
    _connect_layer(p_layer);
    current_layer = index;

    HistoryEntry entry;
    entry.name = U"Nouveau calque";
    HistoryOp op;
    op.type = HistoryOp::LAYER_INSERT;
    op.layer = p_layer;
    op.index = index;
    entry.ops.push_back(op);
    _push_entry(std::move(entry));
    _layers_modified();
    emit_signal("current_layer_changed", current_layer);
}

void DrawingDocument::remove_layer(int p_index)
{
    ERR_FAIL_INDEX(p_index, int(layers.size()));
    ERR_FAIL_COND_MSG(layers.size() <= 1, U"Le document doit garder au moins un calque.");
    if (stroke.active)
    {
        cancel_stroke();
    }
    Ref<DrawingLayer> layer = layers[p_index];
    _disconnect_layer(layer);
    layers.erase(layers.begin() + p_index);
    if (current_layer > p_index || current_layer >= int(layers.size()))
    {
        current_layer = MAX(current_layer - 1, 0);
    }

    HistoryEntry entry;
    entry.name = U"Supprimer le calque";
    HistoryOp op;
    op.type = HistoryOp::LAYER_REMOVE;
    op.layer = layer;
    op.index = p_index;
    entry.ops.push_back(op);
    _push_entry(std::move(entry));
    _layers_modified();
    emit_signal("current_layer_changed", current_layer);
}

Ref<DrawingLayer> DrawingDocument::duplicate_layer(int p_index)
{
    ERR_FAIL_INDEX_V(p_index, int(layers.size()), Ref<DrawingLayer>());
    const Ref<DrawingLayer> source = layers[p_index];
    Ref<DrawingLayer> copy;
    copy.instantiate();
    copy->set_data(size.x, size.y, source->get_data());
    copy->set_layer_name(source->get_layer_name() + String(U" (copie)"));
    copy->set_visible(source->is_visible());
    copy->set_opacity(source->get_opacity());
    copy->set_blend_mode(source->get_blend_mode());
    copy->set_alpha_locked(source->is_alpha_locked());
    copy->set_clipped(source->is_clipped());
    insert_layer(copy, p_index + 1);
    if (!undo_stack.empty())
    {
        undo_stack.back().name = U"Dupliquer le calque";
    }
    return copy;
}

void DrawingDocument::move_layer(int p_from, int p_to)
{
    ERR_FAIL_INDEX(p_from, int(layers.size()));
    const int to = CLAMP(p_to, 0, int(layers.size()) - 1);
    if (p_from == to)
    {
        return;
    }
    if (stroke.active)
    {
        end_stroke();
    }
    const Ref<DrawingLayer> current = get_current();
    Ref<DrawingLayer> layer = layers[p_from];
    layers.erase(layers.begin() + p_from);
    layers.insert(layers.begin() + to, layer);
    current_layer = MAX(find_layer(current), 0);

    HistoryEntry entry;
    entry.name = U"Déplacer le calque";
    HistoryOp op;
    op.type = HistoryOp::LAYER_MOVE;
    op.index = p_from;
    op.index_to = to;
    entry.ops.push_back(op);
    _push_entry(std::move(entry));
    _layers_modified();
    emit_signal("current_layer_changed", current_layer);
}

bool DrawingDocument::merge_down(int p_index)
{
    ERR_FAIL_INDEX_V(p_index, int(layers.size()), false);
    if (p_index == 0)
    {
        return false;
    }
    const Ref<DrawingLayer> upper = layers[p_index];
    const Ref<DrawingLayer> lower = layers[p_index - 1];
    if (lower->is_locked())
    {
        return false;
    }
    if (stroke.active)
    {
        end_stroke();
    }

    const PackedByteArray before = lower->get_data();
    if (upper->is_visible())
    {
        const uint8_t *source = upper->ptr();
        uint8_t *target = lower->ptrw();
        const float opacity = upper->get_opacity();
        const DrawingLayer::LayerBlend mode = upper->get_blend_mode();
        const bool clipped = upper->is_clipped();
        const int64_t count = int64_t(size.x) * size.y;
        for (int64_t i = 0; i < count; i++)
        {
            const uint8_t *s = source + i * 4;
            uint8_t *t = target + i * 4;
            float alpha = s[3] / 255.0f * opacity;
            if (clipped)
            {
                alpha *= t[3] / 255.0f;
            }
            if (alpha <= 0.0f)
            {
                continue;
            }
            float c[3] = {t[0] / 255.0f, t[1] / 255.0f, t[2] / 255.0f};
            float ca = t[3] / 255.0f;
            const float sc[3] = {s[0] / 255.0f, s[1] / 255.0f, s[2] / 255.0f};
            px::blend_over(mode, c, ca, sc, alpha);
            t[0] = px::to_byte(c[0]);
            t[1] = px::to_byte(c[1]);
            t[2] = px::to_byte(c[2]);
            t[3] = px::to_byte(ca);
        }
    }

    _disconnect_layer(upper);
    layers.erase(layers.begin() + p_index);
    current_layer = p_index - 1;

    HistoryEntry entry;
    entry.name = U"Fusionner avec le calque inférieur";
    entry.ops.push_back(_pixels_op(lower, _bounds(), before));
    HistoryOp remove;
    remove.type = HistoryOp::LAYER_REMOVE;
    remove.layer = upper;
    remove.index = p_index;
    entry.ops.push_back(remove);
    _push_entry(std::move(entry));
    _layers_modified();
    emit_signal("current_layer_changed", current_layer);
    return true;
}

void DrawingDocument::clear_layer(int p_index)
{
    ERR_FAIL_INDEX(p_index, int(layers.size()));
    const Ref<DrawingLayer> layer = layers[p_index];
    if (layer->is_locked())
    {
        return;
    }
    if (stroke.active && stroke.layer == layer)
    {
        cancel_stroke();
    }
    const PackedByteArray before = layer->get_data();
    PackedByteArray cleared;
    cleared.resize(before.size());
    cleared.fill(0);
    layer->set_data(size.x, size.y, cleared);

    HistoryEntry entry;
    entry.name = U"Effacer le calque";
    entry.ops.push_back(_pixels_op(layer, _bounds(), before));
    _push_entry(std::move(entry));
    _update_composite(_bounds());
}

static String property_label(const StringName &p_property)
{
    if (p_property == StringName("opacity"))
    {
        return U"Opacité du calque";
    }
    if (p_property == StringName("visible"))
    {
        return U"Visibilité du calque";
    }
    if (p_property == StringName("blend_mode"))
    {
        return U"Mode de fusion";
    }
    if (p_property == StringName("layer_name"))
    {
        return U"Renommer le calque";
    }
    if (p_property == StringName("alpha_locked"))
    {
        return U"Verrou de transparence";
    }
    if (p_property == StringName("clipped"))
    {
        return U"Écrêtage";
    }
    if (p_property == StringName("locked"))
    {
        return U"Verrou du calque";
    }
    return String(U"Calque : ") + String(p_property);
}

void DrawingDocument::set_layer_property(int p_index, const StringName &p_property, const Variant &p_value)
{
    ERR_FAIL_INDEX(p_index, int(layers.size()));
    const Ref<DrawingLayer> layer = layers[p_index];
    const Variant before = layer->get(p_property);
    if (before == p_value)
    {
        return;
    }
    if (stroke.active)
    {
        end_stroke();
    }
    layer->set(p_property, p_value); // recomposition par _on_layer_changed
    const Variant after = layer->get(p_property);

    const uint64_t now = Time::get_singleton()->get_ticks_msec();
    if (!undo_stack.empty())
    {
        HistoryEntry &last = undo_stack.back();
        if (last.ops.size() == 1 && last.ops[0].type == HistoryOp::LAYER_PROPERTY && last.ops[0].layer == layer && last.ops[0].property == p_property && now - last.time_msec < 1000)
        {
            last.ops[0].value_after = after;
            last.time_msec = now;
            redo_stack.clear();
            emit_signal("history_changed");
            return;
        }
    }
    HistoryEntry entry;
    entry.name = property_label(p_property);
    HistoryOp op;
    op.type = HistoryOp::LAYER_PROPERTY;
    op.layer = layer;
    op.property = p_property;
    op.value_before = before;
    op.value_after = after;
    entry.ops.push_back(op);
    _push_entry(std::move(entry));
}

// 🖼️ Composition

void DrawingDocument::_ensure_composite() const
{
    const int64_t bytes = int64_t(size.x) * size.y * 4;
    if (composite_valid && composite.size() == bytes)
    {
        return;
    }
    if (composite.size() != bytes)
    {
        composite.resize(bytes);
    }
    composite_valid = true;
    _compose(_bounds());
}

void DrawingDocument::_compose(const Rect2i &p_rect) const
{
    const Rect2i rect = p_rect.intersection(_bounds());
    if (!rect.has_area())
    {
        return;
    }
    struct Source
    {
        const uint8_t *pixels;
        float opacity;
        DrawingLayer::LayerBlend mode;
        bool visible;
        bool clipped;
    };
    std::vector<Source> sources;
    sources.reserve(layers.size());
    for (const Ref<DrawingLayer> &layer : layers)
    {
        sources.push_back({layer->ptr(), layer->get_opacity(), layer->get_blend_mode(), layer->is_visible(), layer->is_clipped()});
    }

    uint8_t *out = composite.ptrw();
    for (int y = rect.position.y; y < rect.get_end().y; y++)
    {
        for (int x = rect.position.x; x < rect.get_end().x; x++)
        {
            const size_t i = (size_t(y) * size.x + x) * 4;
            float c[3] = {0.0f, 0.0f, 0.0f};
            float ca = 0.0f;
            // Base d'écrêtage : dernier calque non écrêté rencontré (le calque le plus bas s'il est lui-même écrêté).
            float base_alpha = 1.0f;
            bool base_visible = true;
            for (const Source &source : sources)
            {
                const uint8_t *p = source.pixels + i;
                const float alpha = p[3] / 255.0f;
                if (!source.clipped)
                {
                    base_visible = source.visible;
                    base_alpha = alpha * source.opacity;
                }
                if (!source.visible || (source.clipped && !base_visible))
                {
                    continue;
                }
                float sa = alpha * source.opacity;
                if (source.clipped)
                {
                    sa *= base_alpha;
                }
                if (sa <= 0.0f)
                {
                    continue;
                }
                const float sc[3] = {p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f};
                px::blend_over(source.mode, c, ca, sc, sa);
            }
            out[i] = px::to_byte(c[0]);
            out[i + 1] = px::to_byte(c[1]);
            out[i + 2] = px::to_byte(c[2]);
            out[i + 3] = px::to_byte(ca);
        }
    }
}

void DrawingDocument::_update_composite(const Rect2i &p_rect)
{
    const Rect2i rect = p_rect.intersection(_bounds());
    if (!rect.has_area())
    {
        return;
    }
    if (composite_valid)
    {
        _compose(rect);
    }
    else
    {
        _ensure_composite();
    }
    emit_signal("region_changed", rect);
}

void DrawingDocument::refresh_composite()
{
    composite_valid = false;
    _ensure_composite();
    emit_signal("region_changed", _bounds());
}

const uint8_t *DrawingDocument::get_composite_ptr() const
{
    _ensure_composite();
    return composite.ptr();
}

Ref<Image> DrawingDocument::get_composite_image(bool p_with_background) const
{
    _ensure_composite();
    if (!p_with_background || background_color.a <= 0.0f)
    {
        return Image::create_from_data(size.x, size.y, false, Image::FORMAT_RGBA8, composite);
    }
    PackedByteArray flattened = composite;
    uint8_t *w = flattened.ptrw();
    const Color bg = background_color;
    const int64_t count = int64_t(size.x) * size.y;
    for (int64_t i = 0; i < count; i++)
    {
        uint8_t *p = w + i * 4;
        float c[3] = {bg.r, bg.g, bg.b};
        float ca = bg.a;
        const float sc[3] = {p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f};
        px::blend_over(DrawingLayer::LAYER_BLEND_NORMAL, c, ca, sc, p[3] / 255.0f);
        p[0] = px::to_byte(c[0]);
        p[1] = px::to_byte(c[1]);
        p[2] = px::to_byte(c[2]);
        p[3] = px::to_byte(ca);
    }
    return Image::create_from_data(size.x, size.y, false, Image::FORMAT_RGBA8, flattened);
}

Error DrawingDocument::export_png(const String &p_path, bool p_with_background) const
{
    Ref<Image> image = get_composite_image(p_with_background);
    ERR_FAIL_COND_V(image.is_null(), ERR_CANT_CREATE);
    return image->save_png(p_path);
}

// 🖌️ Traits

bool DrawingDocument::begin_stroke(const Ref<DrawingBrush> &p_brush, const Color &p_color, bool p_deferred_commit)
{
    ERR_FAIL_COND_V(p_brush.is_null(), false);
    if (stroke.active)
    {
        end_stroke();
    }
    const Ref<DrawingLayer> layer = get_current();
    if (layer.is_null() || layer->is_locked() || !layer->is_visible())
    {
        return false;
    }
    const size_t count = size_t(size.x) * size.y;
    if (stroke_mask.size() != count)
    {
        stroke_mask.assign(count, 0.0f);
    }
    stroke = StrokeState();
    stroke.active = true;
    stroke.deferred = p_deferred_commit;
    stroke.brush = p_brush;
    stroke.color = p_color;
    stroke.layer = layer;
    stroke.original = layer->get_data();
    return true;
}

float DrawingDocument::_dab_step(float p_pressure, float p_velocity) const { return MAX(0.5f, stroke.brush->get_spacing() * stroke.brush->get_dab_size(p_pressure, p_velocity)); }

void DrawingDocument::_dab(const Vector2 &p_center, float p_pressure, float p_velocity, Rect2i &r_rect)
{
    const DrawingBrush *brush = stroke.brush.ptr();
    float diameter = brush->get_dab_size(p_pressure, p_velocity);
    const float opacity = brush->get_dab_opacity(p_pressure, p_velocity);
    if (opacity <= 0.0f || diameter <= 0.0f)
    {
        return;
    }
    // Sous un pixel, le tampon garde un pixel de large et s'estompe (trait fin plutôt que pointillés).
    float area = 1.0f;
    if (diameter < 1.0f)
    {
        area = diameter;
        diameter = 1.0f;
    }
    const float radius = diameter * 0.5f;
    const float angle = Math::deg_to_rad(brush->get_angle()) + (brush->get_follow_direction() ? stroke.direction : 0.0f);
    const float cos_a = std::cos(-angle);
    const float sin_a = std::sin(-angle);
    const float roundness = brush->get_roundness();
    const float flow = brush->get_flow();
    // Rayon de l'enveloppe : la diagonale d'une forme texturée dépasse le cercle.
    const float reach = brush->has_tip_texture() ? radius * 1.42f : radius;

    const Rect2i dab_rect = Rect2i(int(std::floor(p_center.x - reach - 1.0f)), int(std::floor(p_center.y - reach - 1.0f)), 0, 0)
                                .expand(Vector2i(int(std::ceil(p_center.x + reach + 1.0f)), int(std::ceil(p_center.y + reach + 1.0f))))
                                .intersection(_bounds());
    if (!dab_rect.has_area())
    {
        return;
    }
    for (int y = dab_rect.position.y; y < dab_rect.get_end().y; y++)
    {
        for (int x = dab_rect.position.x; x < dab_rect.get_end().x; x++)
        {
            const float dx = x + 0.5f - p_center.x;
            const float dy = y + 0.5f - p_center.y;
            const float local_x = dx * cos_a - dy * sin_a;
            const float local_y = (dx * sin_a + dy * cos_a) / roundness;
            float value = brush->get_tip_coverage(local_x, local_y, radius);
            if (value <= 0.0f)
            {
                continue;
            }
            value *= brush->get_grain(x + 0.5f, y + 0.5f) * opacity * area;
            float &mask = stroke_mask[size_t(y) * size.x + x];
            if (value > mask)
            {
                // Les tampons tendent vers l'opacité du trait sans la dépasser ; la densité règle l'accumulation.
                mask += (value - mask) * flow;
            }
        }
    }
    r_rect = merge_rect(r_rect, dab_rect);
}

void DrawingDocument::stroke_to(const Vector2 &p_position, float p_pressure, float p_velocity)
{
    if (!stroke.active)
    {
        return;
    }
    const float pressure = Math::clamp(p_pressure, 0.0f, 1.0f);
    const float velocity = MAX(p_velocity, 0.0f);
    Rect2i dirty;
    if (!stroke.has_last)
    {
        stroke.has_last = true;
        _dab(p_position, pressure, velocity, dirty);
        stroke.distance_to_next = _dab_step(pressure, velocity);
    }
    else
    {
        const Vector2 delta = p_position - stroke.last_position;
        const float length = delta.length();
        if (length > 1e-4f)
        {
            stroke.direction = delta.angle();
            float travelled = 0.0f;
            float remaining = length;
            while (stroke.distance_to_next <= remaining)
            {
                remaining -= stroke.distance_to_next;
                travelled += stroke.distance_to_next;
                const float t = travelled / length;
                const float dab_pressure = Math::lerp(stroke.last_pressure, pressure, t);
                const float dab_velocity = Math::lerp(stroke.last_velocity, velocity, t);
                _dab(stroke.last_position + delta * t, dab_pressure, dab_velocity, dirty);
                stroke.distance_to_next = _dab_step(dab_pressure, dab_velocity);
            }
            stroke.distance_to_next -= remaining;
        }
    }
    stroke.last_position = p_position;
    stroke.last_pressure = pressure;
    stroke.last_velocity = velocity;

    if (!dirty.has_area())
    {
        return;
    }
    stroke.rect = merge_rect(stroke.rect, dirty);
    if (stroke.deferred)
    {
        emit_signal("stroke_changed", dirty);
        return;
    }
    _apply_stroke(dirty);
    _update_composite(dirty);
}

void DrawingDocument::_apply_stroke(const Rect2i &p_rect)
{
    const Rect2i rect = p_rect.intersection(_bounds());
    if (!rect.has_area())
    {
        return;
    }
    const uint8_t *original = stroke.original.ptr();
    uint8_t *target = stroke.layer->ptrw();
    const uint8_t *mask_selection = selection_active ? selection.ptr() : nullptr;
    const DrawingBrush::BrushBlend mode = stroke.brush->get_blend_mode();
    const bool alpha_lock = stroke.layer->is_alpha_locked();
    const Color color = stroke.color;
    for (int y = rect.position.y; y < rect.get_end().y; y++)
    {
        for (int x = rect.position.x; x < rect.get_end().x; x++)
        {
            const size_t i = size_t(y) * size.x + x;
            float alpha = stroke_mask[i] * color.a;
            if (mask_selection)
            {
                alpha *= mask_selection[i] / 255.0f;
            }
            px::paint(original + i * 4, target + i * 4, color.r, color.g, color.b, alpha, mode, alpha_lock);
        }
    }
}

void DrawingDocument::_finish_stroke(const Rect2i &p_rect)
{
    const Rect2i rect = p_rect.intersection(_bounds());
    if (rect.has_area())
    {
        HistoryEntry entry;
        entry.name = stroke.brush->get_name().is_empty() ? String(U"Trait") : stroke.brush->get_name();
        entry.ops.push_back(_pixels_op(stroke.layer, rect, copy_region(stroke.original, size.x, rect)));
        _reset_stroke();
        _push_entry(std::move(entry));
    }
    else
    {
        _reset_stroke();
    }
    emit_signal("stroke_finished");
}

void DrawingDocument::_reset_stroke()
{
    const Rect2i rect = stroke.rect.intersection(_bounds());
    for (int y = rect.position.y; y < rect.get_end().y; y++)
    {
        std::fill_n(stroke_mask.begin() + size_t(y) * size.x + rect.position.x, rect.size.x, 0.0f);
    }
    stroke = StrokeState();
}

void DrawingDocument::end_stroke()
{
    if (!stroke.active)
    {
        return;
    }
    const Rect2i rect = stroke.rect;
    if (stroke.deferred && rect.has_area())
    {
        _apply_stroke(rect); // sans rendu du shader : trait appliqué tel quel
        _update_composite(rect);
    }
    _finish_stroke(rect);
}

void DrawingDocument::cancel_stroke()
{
    if (!stroke.active)
    {
        return;
    }
    const Rect2i rect = stroke.rect;
    const bool deferred = stroke.deferred;
    if (!deferred)
    {
        stroke.layer->set_data(size.x, size.y, stroke.original);
    }
    _reset_stroke();
    if (rect.has_area())
    {
        if (deferred)
        {
            emit_signal("stroke_changed", rect);
        }
        else
        {
            _update_composite(rect);
        }
    }
}

Ref<Image> DrawingDocument::get_stroke_preview(const Rect2i &p_rect) const
{
    const Rect2i rect = p_rect.intersection(_bounds());
    if (!rect.has_area())
    {
        return Ref<Image>();
    }
    PackedByteArray bytes;
    bytes.resize(int64_t(rect.size.x) * rect.size.y * 4);
    uint8_t *w = bytes.ptrw();
    const bool has_mask = stroke.active && stroke_mask.size() == size_t(size.x) * size.y;
    const uint8_t *mask_selection = selection_active ? selection.ptr() : nullptr;
    const Color color = stroke.color;
    const uint8_t r = px::to_byte(color.r);
    const uint8_t g = px::to_byte(color.g);
    const uint8_t b = px::to_byte(color.b);
    for (int y = 0; y < rect.size.y; y++)
    {
        for (int x = 0; x < rect.size.x; x++)
        {
            const size_t i = size_t(rect.position.y + y) * size.x + rect.position.x + x;
            float alpha = has_mask ? stroke_mask[i] * color.a : 0.0f;
            if (mask_selection)
            {
                alpha *= mask_selection[i] / 255.0f;
            }
            uint8_t *p = w + (size_t(y) * rect.size.x + x) * 4;
            p[0] = r;
            p[1] = g;
            p[2] = b;
            p[3] = px::to_byte(alpha);
        }
    }
    return Image::create_from_data(rect.size.x, rect.size.y, false, Image::FORMAT_RGBA8, bytes);
}

void DrawingDocument::commit_stroke_image(const Ref<Image> &p_image, const Vector2i &p_position)
{
    ERR_FAIL_COND_MSG(!stroke.active, U"Aucun trait en cours.");
    if (p_image.is_null() || p_image->is_empty())
    {
        end_stroke();
        return;
    }
    Ref<Image> image = p_image->duplicate();
    if (image->is_compressed())
    {
        image->decompress();
    }
    image->convert(Image::FORMAT_RGBA8);
    const Rect2i rect = Rect2i(p_position, image->get_size()).intersection(_bounds());
    if (rect.has_area())
    {
        const PackedByteArray bytes = image->get_data();
        const uint8_t *source = bytes.ptr();
        const int image_width = image->get_width();
        const uint8_t *original = stroke.original.ptr();
        uint8_t *target = stroke.layer->ptrw();
        const uint8_t *mask_selection = selection_active ? selection.ptr() : nullptr;
        const DrawingBrush::BrushBlend mode = stroke.brush->get_blend_mode();
        const bool alpha_lock = stroke.layer->is_alpha_locked();
        for (int y = rect.position.y; y < rect.get_end().y; y++)
        {
            for (int x = rect.position.x; x < rect.get_end().x; x++)
            {
                const size_t i = size_t(y) * size.x + x;
                const uint8_t *s = source + (size_t(y - p_position.y) * image_width + (x - p_position.x)) * 4;
                float alpha = s[3] / 255.0f;
                if (mask_selection)
                {
                    alpha *= mask_selection[i] / 255.0f;
                }
                px::paint(original + i * 4, target + i * 4, s[0] / 255.0f, s[1] / 255.0f, s[2] / 255.0f, alpha, mode, alpha_lock);
            }
        }
        _update_composite(rect);
    }
    _finish_stroke(merge_rect(stroke.rect, rect));
}

void DrawingDocument::draw_line(const Vector2 &p_from, const Vector2 &p_to, const Ref<DrawingBrush> &p_brush, const Color &p_color, float p_pressure)
{
    if (!begin_stroke(p_brush, p_color))
    {
        return;
    }
    stroke_to(p_from, p_pressure, 0.0f);
    stroke_to(p_to, p_pressure, 0.0f);
    end_stroke();
}

// 🪣 Remplissage

std::vector<uint8_t> DrawingDocument::_region_by_color(const Vector2i &p_seed, float p_tolerance, FillReference p_reference, bool p_contiguous, int p_gap_closing)
{
    const Ref<DrawingLayer> layer = get_current();
    const uint8_t *reference = (p_reference == FILL_REFERENCE_LAYER && layer.is_valid()) ? layer->ptr() : get_composite_ptr();
    const int width = size.x;
    const int height = size.y;
    const size_t count = size_t(width) * height;
    uint8_t seed[4];
    memcpy(seed, reference + (size_t(p_seed.y) * width + p_seed.x) * 4, 4);
    const int tolerance = int(std::lround(Math::clamp(p_tolerance, 0.0f, 1.0f) * 255.0f));

    std::vector<uint8_t> similar(count);
    for (size_t i = 0; i < count; i++)
    {
        similar[i] = px::similar(reference + i * 4, seed, tolerance) ? 1 : 0;
    }
    if (!p_contiguous)
    {
        return similar;
    }
    if (p_gap_closing <= 0)
    {
        return px::flood(similar, width, height, p_seed.x, p_seed.y);
    }

    // Fermeture des trous : la diffusion ne passe que par les pixels à plus de p_gap_closing d'un trait, puis la zone
    // regagne les pixels semblables le long des traits.
    std::vector<uint8_t> barrier(count);
    for (size_t i = 0; i < count; i++)
    {
        barrier[i] = similar[i] ? 0 : 1;
    }
    const std::vector<float> to_barrier = px::distance_to(barrier, width, height);
    std::vector<uint8_t> passable(count);
    for (size_t i = 0; i < count; i++)
    {
        passable[i] = similar[i] && to_barrier[i] > float(p_gap_closing) ? 1 : 0;
    }
    if (!passable[size_t(p_seed.y) * width + p_seed.x])
    {
        return px::flood(similar, width, height, p_seed.x, p_seed.y); // clic dans une zone plus étroite que les trous
    }
    const std::vector<uint8_t> core = px::flood(passable, width, height, p_seed.x, p_seed.y);
    const std::vector<float> to_core = px::distance_to(core, width, height);
    std::vector<uint8_t> region(count);
    for (size_t i = 0; i < count; i++)
    {
        region[i] = similar[i] && to_core[i] <= float(p_gap_closing) + 1.0f ? 1 : 0;
    }
    return region;
}

void DrawingDocument::_paint_coverage(
    const Ref<DrawingLayer> &p_layer, const std::vector<uint8_t> &p_coverage, const Rect2i &p_rect, const Color &p_color, DrawingBrush::BrushBlend p_mode, const String &p_name)
{
    const Rect2i rect = p_rect.intersection(_bounds());
    if (!rect.has_area())
    {
        return;
    }
    const PackedByteArray before = p_layer->get_region(rect);
    uint8_t *target = p_layer->ptrw();
    const uint8_t *mask_selection = selection_active ? selection.ptr() : nullptr;
    const bool alpha_lock = p_layer->is_alpha_locked();
    for (int y = rect.position.y; y < rect.get_end().y; y++)
    {
        for (int x = rect.position.x; x < rect.get_end().x; x++)
        {
            const size_t i = size_t(y) * size.x + x;
            float alpha = p_color.a * (p_coverage.empty() ? 1.0f : p_coverage[i] / 255.0f);
            if (mask_selection)
            {
                alpha *= mask_selection[i] / 255.0f;
            }
            px::paint(target + i * 4, target + i * 4, p_color.r, p_color.g, p_color.b, alpha, p_mode, alpha_lock);
        }
    }
    HistoryEntry entry;
    entry.name = p_name;
    entry.ops.push_back(_pixels_op(p_layer, rect, before));
    _push_entry(std::move(entry));
    _update_composite(rect);
}

bool DrawingDocument::fill(const Vector2i &p_position, const Color &p_color, float p_tolerance, FillReference p_reference, int p_gap_closing, int p_expand, bool p_contiguous)
{
    const Ref<DrawingLayer> layer = get_current();
    if (layer.is_null() || layer->is_locked() || !layer->is_visible() || !_bounds().has_point(p_position))
    {
        return false;
    }
    if (stroke.active)
    {
        end_stroke();
    }
    std::vector<uint8_t> region = _region_by_color(p_position, p_tolerance, p_reference, p_contiguous, p_gap_closing);
    px::grow(region, size.x, size.y, p_expand);

    Rect2i bounds;
    for (int y = 0; y < size.y; y++)
    {
        for (int x = 0; x < size.x; x++)
        {
            uint8_t &value = region[size_t(y) * size.x + x];
            if (value)
            {
                value = 255;
                bounds = merge_rect(bounds, Rect2i(x, y, 1, 1));
            }
        }
    }
    if (!bounds.has_area())
    {
        return false;
    }
    _paint_coverage(layer, region, bounds, p_color, DrawingBrush::BRUSH_BLEND_NORMAL, U"Remplissage");
    return true;
}

void DrawingDocument::fill_selection(const Color &p_color)
{
    const Ref<DrawingLayer> layer = get_current();
    if (layer.is_null() || layer->is_locked())
    {
        return;
    }
    if (stroke.active)
    {
        end_stroke();
    }
    _paint_coverage(layer, std::vector<uint8_t>(), selection_active ? get_selection_bounds() : _bounds(), p_color, DrawingBrush::BRUSH_BLEND_NORMAL, U"Remplir la sélection");
}

void DrawingDocument::clear_selection_content()
{
    const Ref<DrawingLayer> layer = get_current();
    if (layer.is_null() || layer->is_locked())
    {
        return;
    }
    if (stroke.active)
    {
        end_stroke();
    }
    _paint_coverage(layer, std::vector<uint8_t>(), selection_active ? get_selection_bounds() : _bounds(), Color(0, 0, 0, 1), DrawingBrush::BRUSH_BLEND_ERASE, U"Effacer");
}

// 💧 Pipette

Color DrawingDocument::pick_color(const Vector2i &p_position, bool p_all_layers) const
{
    if (!_bounds().has_point(p_position))
    {
        return Color(0, 0, 0, 0);
    }
    const size_t i = (size_t(p_position.y) * size.x + p_position.x) * 4;
    if (!p_all_layers)
    {
        const Ref<DrawingLayer> layer = get_current();
        if (layer.is_null())
        {
            return Color(0, 0, 0, 0);
        }
        const uint8_t *p = layer->ptr() + i;
        return Color(p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f, p[3] / 255.0f);
    }
    const uint8_t *p = get_composite_ptr() + i;
    float c[3] = {background_color.r, background_color.g, background_color.b};
    float ca = background_color.a;
    const float sc[3] = {p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f};
    px::blend_over(DrawingLayer::LAYER_BLEND_NORMAL, c, ca, sc, p[3] / 255.0f);
    return ca > 0.0f ? Color(c[0], c[1], c[2], ca) : Color(0, 0, 0, 0);
}

// ⬚ Sélection

void DrawingDocument::_set_selection(const std::vector<uint8_t> &p_shape, SelectionMode p_mode, const String &p_name)
{
    if (stroke.active)
    {
        end_stroke();
    }
    const size_t count = size_t(size.x) * size.y;
    ERR_FAIL_COND(p_shape.size() != count);
    const PackedByteArray before = selection;
    const bool had = selection_active;
    PackedByteArray result;
    result.resize(int64_t(count));
    uint8_t *r = result.ptrw();
    const uint8_t *old = had ? selection.ptr() : nullptr;
    bool any = false;
    for (size_t i = 0; i < count; i++)
    {
        const int shape = p_shape[i];
        const int previous = old ? old[i] : 0;
        int value = shape;
        switch (p_mode)
        {
        case SELECTION_ADD:
            value = MAX(previous, shape);
            break;
        case SELECTION_SUBTRACT:
            value = previous * (255 - shape) / 255;
            break;
        case SELECTION_INTERSECT:
            value = old ? MIN(previous, shape) : shape; // sans sélection, tout est sélectionnable
            break;
        case SELECTION_REPLACE:
        default:
            break;
        }
        r[i] = uint8_t(value);
        any = any || value != 0;
    }
    if (!had && !any)
    {
        return;
    }
    selection = any ? result : PackedByteArray();
    selection_active = any;

    HistoryEntry entry;
    entry.name = p_name;
    HistoryOp op;
    op.type = HistoryOp::SELECTION;
    op.before = before;
    op.after = selection;
    op.had_before = had;
    op.has_after = any;
    entry.ops.push_back(op);
    _push_entry(std::move(entry));
    emit_signal("selection_changed");
}

void DrawingDocument::select_rect(const Rect2i &p_rect, SelectionMode p_mode)
{
    const Rect2i rect = p_rect.abs().intersection(_bounds());
    if (!rect.has_area() && p_mode == SELECTION_REPLACE)
    {
        deselect();
        return;
    }
    std::vector<uint8_t> shape(size_t(size.x) * size.y, 0);
    for (int y = rect.position.y; y < rect.get_end().y; y++)
    {
        std::fill_n(shape.begin() + size_t(y) * size.x + rect.position.x, rect.size.x, uint8_t(255));
    }
    _set_selection(shape, p_mode, U"Sélection rectangulaire");
}

void DrawingDocument::select_ellipse(const Rect2i &p_rect, SelectionMode p_mode)
{
    const Rect2i ellipse = p_rect.abs();
    const Rect2i rect = ellipse.intersection(_bounds());
    if (!rect.has_area() && p_mode == SELECTION_REPLACE)
    {
        deselect();
        return;
    }
    std::vector<uint8_t> shape(size_t(size.x) * size.y, 0);
    const Vector2 center = Vector2(ellipse.position) + Vector2(ellipse.size) * 0.5f;
    const Vector2 radius = Vector2(ellipse.size) * 0.5f;
    const float edge = MAX(MIN(radius.x, radius.y), 1.0f);
    for (int y = rect.position.y; y < rect.get_end().y; y++)
    {
        for (int x = rect.position.x; x < rect.get_end().x; x++)
        {
            const float nx = (x + 0.5f - center.x) / MAX(radius.x, 0.5f);
            const float ny = (y + 0.5f - center.y) / MAX(radius.y, 0.5f);
            const float coverage = Math::clamp((1.0f - std::sqrt(nx * nx + ny * ny)) * edge + 0.5f, 0.0f, 1.0f);
            shape[size_t(y) * size.x + x] = px::to_byte(coverage);
        }
    }
    _set_selection(shape, p_mode, U"Sélection elliptique");
}

void DrawingDocument::select_polygon(const PackedVector2Array &p_points, SelectionMode p_mode)
{
    if (p_points.size() < 3)
    {
        if (p_mode == SELECTION_REPLACE)
        {
            deselect();
        }
        return;
    }
    std::vector<uint8_t> shape(size_t(size.x) * size.y, 0);
    float top = p_points[0].y;
    float bottom = p_points[0].y;
    for (const Vector2 &point : p_points)
    {
        top = MIN(top, point.y);
        bottom = MAX(bottom, point.y);
    }
    const int first_row = MAX(int(std::floor(top)), 0);
    const int last_row = MIN(int(std::ceil(bottom)), size.y - 1);
    std::vector<float> crossings;
    const int64_t count = p_points.size();
    for (int y = first_row; y <= last_row; y++)
    {
        const float row = y + 0.5f;
        crossings.clear();
        for (int64_t i = 0; i < count; i++)
        {
            const Vector2 a = p_points[i];
            const Vector2 b = p_points[(i + 1) % count];
            if ((a.y <= row) != (b.y <= row))
            {
                crossings.push_back(a.x + (row - a.y) * (b.x - a.x) / (b.y - a.y));
            }
        }
        std::sort(crossings.begin(), crossings.end());
        for (size_t k = 0; k + 1 < crossings.size(); k += 2)
        {
            const int from = MAX(int(std::ceil(crossings[k] - 0.5f)), 0);
            const int to = MIN(int(std::ceil(crossings[k + 1] - 0.5f)), size.x);
            for (int x = from; x < to; x++)
            {
                shape[size_t(y) * size.x + x] = 255;
            }
        }
    }
    _set_selection(shape, p_mode, U"Lasso");
}

void DrawingDocument::select_by_color(const Vector2i &p_position, float p_tolerance, FillReference p_reference, bool p_contiguous, SelectionMode p_mode)
{
    if (!_bounds().has_point(p_position))
    {
        return;
    }
    std::vector<uint8_t> shape = _region_by_color(p_position, p_tolerance, p_reference, p_contiguous, 0);
    for (uint8_t &value : shape)
    {
        value = value ? 255 : 0;
    }
    _set_selection(shape, p_mode, U"Sélection automatique");
}

void DrawingDocument::select_all() { _set_selection(std::vector<uint8_t>(size_t(size.x) * size.y, 255), SELECTION_REPLACE, U"Tout sélectionner"); }

void DrawingDocument::deselect()
{
    if (!selection_active)
    {
        return;
    }
    _set_selection(std::vector<uint8_t>(size_t(size.x) * size.y, 0), SELECTION_REPLACE, U"Désélectionner");
}

void DrawingDocument::invert_selection()
{
    if (!selection_active)
    {
        select_all();
        return;
    }
    std::vector<uint8_t> shape(size_t(size.x) * size.y);
    const uint8_t *current = selection.ptr();
    for (size_t i = 0; i < shape.size(); i++)
    {
        shape[i] = 255 - current[i];
    }
    _set_selection(shape, SELECTION_REPLACE, U"Inverser la sélection");
}

float DrawingDocument::get_selection_value(int p_x, int p_y) const
{
    if (!selection_active)
    {
        return 1.0f;
    }
    if (p_x < 0 || p_y < 0 || p_x >= size.x || p_y >= size.y)
    {
        return 0.0f;
    }
    return selection[size_t(p_y) * size.x + p_x] / 255.0f;
}

Ref<Image> DrawingDocument::get_selection_image() const
{
    if (!selection_active)
    {
        return Ref<Image>();
    }
    return Image::create_from_data(size.x, size.y, false, Image::FORMAT_L8, selection);
}

Rect2i DrawingDocument::get_selection_bounds() const
{
    if (!selection_active)
    {
        return Rect2i();
    }
    const uint8_t *mask = selection.ptr();
    int left = size.x, top = size.y, right = -1, bottom = -1;
    for (int y = 0; y < size.y; y++)
    {
        const uint8_t *row = mask + size_t(y) * size.x;
        for (int x = 0; x < size.x; x++)
        {
            if (row[x])
            {
                left = MIN(left, x);
                right = MAX(right, x);
                top = MIN(top, y);
                bottom = MAX(bottom, y);
            }
        }
    }
    return right < 0 ? Rect2i() : Rect2i(left, top, right - left + 1, bottom - top + 1);
}

// ↩️ Historique

DrawingDocument::HistoryOp DrawingDocument::_pixels_op(const Ref<DrawingLayer> &p_layer, const Rect2i &p_rect, const PackedByteArray &p_before) const
{
    HistoryOp op;
    op.type = HistoryOp::PIXELS;
    op.layer = p_layer;
    op.rect = p_rect;
    op.before = p_before;
    // Tout le calque : copie partagée des données (pas de recopie tant qu'elles ne changent pas).
    op.after = p_rect == _bounds() ? p_layer->get_data() : p_layer->get_region(p_rect);
    return op;
}

void DrawingDocument::_push_entry(HistoryEntry &&p_entry)
{
    p_entry.time_msec = Time::get_singleton()->get_ticks_msec();
    undo_stack.push_back(std::move(p_entry));
    while (int(undo_stack.size()) > history_limit)
    {
        undo_stack.erase(undo_stack.begin());
    }
    redo_stack.clear();
    emit_signal("history_changed");
}

void DrawingDocument::_apply_op(const HistoryOp &p_op, bool p_undo)
{
    switch (p_op.type)
    {
    case HistoryOp::PIXELS:
    {
        const PackedByteArray &bytes = p_undo ? p_op.before : p_op.after;
        if (p_op.rect == _bounds() && bytes.size() == int64_t(size.x) * size.y * 4)
        {
            p_op.layer->set_data(size.x, size.y, bytes);
        }
        else
        {
            p_op.layer->set_region(p_op.rect, bytes);
        }
        if (find_layer(p_op.layer) >= 0)
        {
            _update_composite(p_op.rect);
        }
        break;
    }
    case HistoryOp::SELECTION:
        selection_active = p_undo ? p_op.had_before : p_op.has_after;
        selection = selection_active ? (p_undo ? p_op.before : p_op.after) : PackedByteArray();
        emit_signal("selection_changed");
        break;
    case HistoryOp::LAYER_INSERT:
    case HistoryOp::LAYER_REMOVE:
    {
        const bool insert = (p_op.type == HistoryOp::LAYER_INSERT) != p_undo;
        if (insert)
        {
            const int index = CLAMP(p_op.index, 0, int(layers.size()));
            layers.insert(layers.begin() + index, p_op.layer);
            _connect_layer(p_op.layer);
            current_layer = index;
        }
        else
        {
            const int index = find_layer(p_op.layer);
            if (index >= 0)
            {
                _disconnect_layer(p_op.layer);
                layers.erase(layers.begin() + index);
                if (current_layer > index)
                {
                    current_layer--;
                }
                current_layer = CLAMP(current_layer, 0, MAX(int(layers.size()) - 1, 0));
            }
        }
        break;
    }
    case HistoryOp::LAYER_MOVE:
    {
        const int from = p_undo ? p_op.index_to : p_op.index;
        const int to = p_undo ? p_op.index : p_op.index_to;
        if (from >= 0 && from < int(layers.size()))
        {
            Ref<DrawingLayer> layer = layers[from];
            layers.erase(layers.begin() + from);
            layers.insert(layers.begin() + CLAMP(to, 0, int(layers.size())), layer);
            current_layer = CLAMP(to, 0, int(layers.size()) - 1);
        }
        break;
    }
    case HistoryOp::LAYER_PROPERTY:
        p_op.layer->set(p_op.property, p_undo ? p_op.value_before : p_op.value_after);
        break;
    }
}

bool DrawingDocument::undo()
{
    if (stroke.active)
    {
        end_stroke();
    }
    if (undo_stack.empty())
    {
        return false;
    }
    HistoryEntry entry = std::move(undo_stack.back());
    undo_stack.pop_back();
    const int previous_layer = current_layer;
    bool structural = false;
    for (auto it = entry.ops.rbegin(); it != entry.ops.rend(); ++it)
    {
        _apply_op(*it, true);
        structural = structural || it->type == HistoryOp::LAYER_INSERT || it->type == HistoryOp::LAYER_REMOVE || it->type == HistoryOp::LAYER_MOVE;
    }
    redo_stack.push_back(std::move(entry));
    if (structural)
    {
        _layers_modified();
    }
    if (current_layer != previous_layer || structural)
    {
        emit_signal("current_layer_changed", current_layer);
    }
    emit_signal("history_changed");
    return true;
}

bool DrawingDocument::redo()
{
    if (stroke.active)
    {
        end_stroke();
    }
    if (redo_stack.empty())
    {
        return false;
    }
    HistoryEntry entry = std::move(redo_stack.back());
    redo_stack.pop_back();
    const int previous_layer = current_layer;
    bool structural = false;
    for (const HistoryOp &op : entry.ops)
    {
        _apply_op(op, false);
        structural = structural || op.type == HistoryOp::LAYER_INSERT || op.type == HistoryOp::LAYER_REMOVE || op.type == HistoryOp::LAYER_MOVE;
    }
    undo_stack.push_back(std::move(entry));
    if (structural)
    {
        _layers_modified();
    }
    if (current_layer != previous_layer || structural)
    {
        emit_signal("current_layer_changed", current_layer);
    }
    emit_signal("history_changed");
    return true;
}

void DrawingDocument::clear_history()
{
    undo_stack.clear();
    redo_stack.clear();
    emit_signal("history_changed");
}

// 🔗 Liaisons

void DrawingDocument::_bind_methods()
{
    ClassDB::bind_static_method("DrawingDocument", D_METHOD("create", "size", "background"), &DrawingDocument::create, DEFVAL(Color(1, 1, 1, 1)));
    ClassDB::bind_static_method("DrawingDocument", D_METHOD("create_from_image", "image"), &DrawingDocument::create_from_image);

    ClassDB::bind_method(D_METHOD("set_size", "size"), &DrawingDocument::set_size);
    ClassDB::bind_method(D_METHOD("get_size"), &DrawingDocument::get_size);
    ClassDB::bind_method(D_METHOD("set_background_color", "color"), &DrawingDocument::set_background_color);
    ClassDB::bind_method(D_METHOD("get_background_color"), &DrawingDocument::get_background_color);
    ClassDB::bind_method(D_METHOD("set_history_limit", "limit"), &DrawingDocument::set_history_limit);
    ClassDB::bind_method(D_METHOD("get_history_limit"), &DrawingDocument::get_history_limit);

    ClassDB::bind_method(D_METHOD("set_layers", "layers"), &DrawingDocument::set_layers);
    ClassDB::bind_method(D_METHOD("get_layers"), &DrawingDocument::get_layers);
    ClassDB::bind_method(D_METHOD("get_layer_count"), &DrawingDocument::get_layer_count);
    ClassDB::bind_method(D_METHOD("get_layer", "index"), &DrawingDocument::get_layer);
    ClassDB::bind_method(D_METHOD("find_layer", "layer"), &DrawingDocument::find_layer);
    ClassDB::bind_method(D_METHOD("set_current_layer", "index"), &DrawingDocument::set_current_layer);
    ClassDB::bind_method(D_METHOD("get_current_layer"), &DrawingDocument::get_current_layer);
    ClassDB::bind_method(D_METHOD("get_current"), &DrawingDocument::get_current);
    ClassDB::bind_method(D_METHOD("add_layer", "name", "index"), &DrawingDocument::add_layer, DEFVAL(String()), DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("insert_layer", "layer", "index"), &DrawingDocument::insert_layer, DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("remove_layer", "index"), &DrawingDocument::remove_layer);
    ClassDB::bind_method(D_METHOD("duplicate_layer", "index"), &DrawingDocument::duplicate_layer);
    ClassDB::bind_method(D_METHOD("move_layer", "from", "to"), &DrawingDocument::move_layer);
    ClassDB::bind_method(D_METHOD("merge_down", "index"), &DrawingDocument::merge_down);
    ClassDB::bind_method(D_METHOD("clear_layer", "index"), &DrawingDocument::clear_layer);
    ClassDB::bind_method(D_METHOD("set_layer_property", "index", "property", "value"), &DrawingDocument::set_layer_property);

    ClassDB::bind_method(D_METHOD("begin_stroke", "brush", "color", "deferred_commit"), &DrawingDocument::begin_stroke, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("stroke_to", "position", "pressure", "velocity"), &DrawingDocument::stroke_to, DEFVAL(1.0f), DEFVAL(0.0f));
    ClassDB::bind_method(D_METHOD("end_stroke"), &DrawingDocument::end_stroke);
    ClassDB::bind_method(D_METHOD("cancel_stroke"), &DrawingDocument::cancel_stroke);
    ClassDB::bind_method(D_METHOD("is_stroking"), &DrawingDocument::is_stroking);
    ClassDB::bind_method(D_METHOD("is_stroke_deferred"), &DrawingDocument::is_stroke_deferred);
    ClassDB::bind_method(D_METHOD("get_stroke_brush"), &DrawingDocument::get_stroke_brush);
    ClassDB::bind_method(D_METHOD("get_stroke_rect"), &DrawingDocument::get_stroke_rect);
    ClassDB::bind_method(D_METHOD("get_stroke_preview", "rect"), &DrawingDocument::get_stroke_preview);
    ClassDB::bind_method(D_METHOD("commit_stroke_image", "image", "position"), &DrawingDocument::commit_stroke_image);
    ClassDB::bind_method(D_METHOD("draw_line", "from", "to", "brush", "color", "pressure"), &DrawingDocument::draw_line, DEFVAL(1.0f));

    ClassDB::bind_method(D_METHOD("fill", "position", "color", "tolerance", "reference", "gap_closing", "expand", "contiguous"), &DrawingDocument::fill, DEFVAL(0.0f), DEFVAL(FILL_REFERENCE_LAYER),
                         DEFVAL(0), DEFVAL(0), DEFVAL(true));
    ClassDB::bind_method(D_METHOD("fill_selection", "color"), &DrawingDocument::fill_selection);
    ClassDB::bind_method(D_METHOD("clear_selection_content"), &DrawingDocument::clear_selection_content);
    ClassDB::bind_method(D_METHOD("pick_color", "position", "all_layers"), &DrawingDocument::pick_color, DEFVAL(true));

    ClassDB::bind_method(D_METHOD("has_selection"), &DrawingDocument::has_selection);
    ClassDB::bind_method(D_METHOD("select_rect", "rect", "mode"), &DrawingDocument::select_rect, DEFVAL(SELECTION_REPLACE));
    ClassDB::bind_method(D_METHOD("select_ellipse", "rect", "mode"), &DrawingDocument::select_ellipse, DEFVAL(SELECTION_REPLACE));
    ClassDB::bind_method(D_METHOD("select_polygon", "points", "mode"), &DrawingDocument::select_polygon, DEFVAL(SELECTION_REPLACE));
    ClassDB::bind_method(D_METHOD("select_by_color", "position", "tolerance", "reference", "contiguous", "mode"), &DrawingDocument::select_by_color, DEFVAL(0.0f), DEFVAL(FILL_REFERENCE_LAYER),
                         DEFVAL(true), DEFVAL(SELECTION_REPLACE));
    ClassDB::bind_method(D_METHOD("select_all"), &DrawingDocument::select_all);
    ClassDB::bind_method(D_METHOD("deselect"), &DrawingDocument::deselect);
    ClassDB::bind_method(D_METHOD("invert_selection"), &DrawingDocument::invert_selection);
    ClassDB::bind_method(D_METHOD("get_selection_value", "x", "y"), &DrawingDocument::get_selection_value);
    ClassDB::bind_method(D_METHOD("get_selection_image"), &DrawingDocument::get_selection_image);
    ClassDB::bind_method(D_METHOD("get_selection_bounds"), &DrawingDocument::get_selection_bounds);

    ClassDB::bind_method(D_METHOD("get_composite_image", "with_background"), &DrawingDocument::get_composite_image, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("export_png", "path", "with_background"), &DrawingDocument::export_png, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("refresh_composite"), &DrawingDocument::refresh_composite);

    ClassDB::bind_method(D_METHOD("undo"), &DrawingDocument::undo);
    ClassDB::bind_method(D_METHOD("redo"), &DrawingDocument::redo);
    ClassDB::bind_method(D_METHOD("can_undo"), &DrawingDocument::can_undo);
    ClassDB::bind_method(D_METHOD("can_redo"), &DrawingDocument::can_redo);
    ClassDB::bind_method(D_METHOD("get_undo_name"), &DrawingDocument::get_undo_name);
    ClassDB::bind_method(D_METHOD("get_redo_name"), &DrawingDocument::get_redo_name);
    ClassDB::bind_method(D_METHOD("get_undo_count"), &DrawingDocument::get_undo_count);
    ClassDB::bind_method(D_METHOD("get_redo_count"), &DrawingDocument::get_redo_count);
    ClassDB::bind_method(D_METHOD("clear_history"), &DrawingDocument::clear_history);

    // Ordre important au chargement : la taille avant les calques (recadrés à la taille du document).
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "size", PROPERTY_HINT_NONE, "suffix:px"), "set_size", "get_size");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "background_color"), "set_background_color", "get_background_color");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "history_limit", PROPERTY_HINT_RANGE, "1,1000,1"), "set_history_limit", "get_history_limit");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "layers", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":DrawingLayer"),
                 "set_layers", "get_layers");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "current_layer"), "set_current_layer", "get_current_layer");

    ADD_SIGNAL(MethodInfo("region_changed", PropertyInfo(Variant::RECT2I, "rect")));
    ADD_SIGNAL(MethodInfo("stroke_changed", PropertyInfo(Variant::RECT2I, "rect")));
    ADD_SIGNAL(MethodInfo("stroke_finished"));
    ADD_SIGNAL(MethodInfo("layers_changed"));
    ADD_SIGNAL(MethodInfo("current_layer_changed", PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(MethodInfo("selection_changed"));
    ADD_SIGNAL(MethodInfo("history_changed"));

    BIND_ENUM_CONSTANT(SELECTION_REPLACE);
    BIND_ENUM_CONSTANT(SELECTION_ADD);
    BIND_ENUM_CONSTANT(SELECTION_SUBTRACT);
    BIND_ENUM_CONSTANT(SELECTION_INTERSECT);

    BIND_ENUM_CONSTANT(FILL_REFERENCE_LAYER);
    BIND_ENUM_CONSTANT(FILL_REFERENCE_ALL_LAYERS);
}
