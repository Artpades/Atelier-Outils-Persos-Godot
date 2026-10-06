#include "floating_dock.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void FloatingDock::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_READY:
        // Dans l'éditeur : agencement au repos seulement
        set_process(!Engine::get_singleton()->is_editor_hint());
        break;
    case NOTIFICATION_SORT_CHILDREN:
        _layout();
        break;
    default:
        break;
    }
}

// 🔄 Grossissement

void FloatingDock::_process(double p_delta)
{
    LocalVector<Control *> items;
    _get_items(items);
    if (influences.size() != items.size())
    {
        influences.resize(items.size());
        for (float &influence : influences)
        {
            influence = 0.0f;
        }
    }

    LocalVector<float> rest_centers;
    _get_rest_centers(items, rest_centers);
    float pointer = 0.0f;
    const bool active = _get_pointer(items, rest_centers, pointer);

    const float weight = smoothing > 0.0f ? 1.0f - Math::exp(-smoothing * (float)p_delta) : 1.0f;
    const float radius = Math::max(influence_radius, 1.0f);
    bool changed = false;
    int closest = -1;
    float closest_distance = radius;
    for (uint32_t i = 0; i < items.size(); i++)
    {
        float target = 0.0f;
        if (active)
        {
            // Atténuation en cosinus : 1 sous le pointeur, 0 à influence_radius
            const float distance = Math::abs(pointer - rest_centers[i]);
            target = distance < radius ? 0.5f * (1.0f + Math::cos(Math_PI * distance / radius)) : 0.0f;
            if (distance < closest_distance)
            {
                closest_distance = distance;
                closest = (int)i;
            }
        }
        const float next = Math::lerp(influences[i], target, weight);
        const float settled = Math::abs(next - target) < 0.001f ? target : next;
        changed = changed || settled != influences[i];
        influences[i] = settled;
    }
    if (changed)
    {
        _layout();
    }

    const ObjectID hovered = closest >= 0 ? ObjectID(items[closest]->get_instance_id()) : ObjectID();
    if (hovered != hovered_item_id)
    {
        hovered_item_id = hovered;
        emit_signal("hovered_item_changed", get_hovered_item());
    }
}

Control *FloatingDock::get_hovered_item() const { return Object::cast_to<Control>(ObjectDB::get_instance(hovered_item_id)); }

float FloatingDock::get_item_influence(int p_index) const
{
    return p_index >= 0 && (uint32_t)p_index < influences.size() ? influences[p_index] : 0.0f;
}

bool FloatingDock::_get_pointer(const LocalVector<Control *> &p_items, const LocalVector<float> &p_rest_centers, float &r_pointer) const
{
    const int main = vertical ? 1 : 0;

    // Souris : au-dessus du dock ou d'un élément agrandi (qui déborde du dock)
    const Vector2 mouse = get_local_mouse_position();
    Rect2 area(Vector2(), get_size());
    for (const Rect2 &rect : item_rects)
    {
        area = area.merge(rect);
    }
    if (area.grow(hover_margin).has_point(mouse))
    {
        r_pointer = mouse[main];
        return true;
    }

    // Clavier / manette : l'élément qui a le focus (ou dont un descendant l'a)
    if (follow_focus && is_inside_tree())
    {
        Control *focus = get_viewport()->gui_get_focus_owner();
        for (uint32_t i = 0; focus && i < p_items.size(); i++)
        {
            if (p_items[i] == focus || p_items[i]->is_ancestor_of(focus))
            {
                r_pointer = p_rest_centers[i];
                return true;
            }
        }
    }
    return false;
}

// 📐 Agencement

void FloatingDock::_get_items(LocalVector<Control *> &r_items) const
{
    for (int64_t i = 0; i < get_child_count(); i++)
    {
        Control *child = Object::cast_to<Control>(get_child(i));
        if (child && child->is_visible() && !child->is_set_as_top_level())
        {
            r_items.push_back(child);
        }
    }
}

Vector2 FloatingDock::_get_base_size(Control *p_item) const
{
    return item_size.x > 0 && item_size.y > 0 ? item_size : p_item->get_combined_minimum_size();
}

void FloatingDock::_get_rest_centers(const LocalVector<Control *> &p_items, LocalVector<float> &r_centers) const
{
    const int main = vertical ? 1 : 0;
    const Vector2 minimum = _get_minimum_size();
    float cursor = (get_size()[main] - minimum[main]) / 2.0f;
    for (Control *item : p_items)
    {
        const float length = _get_base_size(item)[main];
        r_centers.push_back(cursor + length / 2.0f);
        cursor += length + separation;
    }
}

void FloatingDock::_layout()
{
    LocalVector<Control *> items;
    _get_items(items);
    const int main = vertical ? 1 : 0;
    const int cross = 1 - main;
    const Vector2 dock_size = get_size();

    // Tailles et espacements selon l'influence de chaque élément
    LocalVector<Vector2> sizes;
    LocalVector<float> spacings;
    float total = items.is_empty() ? 0.0f : separation * (items.size() - 1);
    for (uint32_t i = 0; i < items.size(); i++)
    {
        const float influence = i < influences.size() ? influences[i] : 0.0f;
        const float scale = 1.0f + (magnification - 1.0f) * influence;
        sizes.push_back(_get_base_size(items[i]) * scale);
        spacings.push_back(hover_spacing * influence);
        total += sizes[i][main] + 2.0f * spacings[i];
    }

    // Rangée centrée dans le dock : elle s'élargit des deux côtés
    item_rects.clear();
    float cursor = (dock_size[main] - total) / 2.0f;
    for (uint32_t i = 0; i < items.size(); i++)
    {
        const float influence = i < influences.size() ? influences[i] : 0.0f;
        const Vector2 size = sizes[i];
        Vector2 position;
        cursor += spacings[i];
        position[main] = cursor;
        switch (item_anchor)
        {
        case ANCHOR_BEGIN:
            position[cross] = hover_lift * influence;
            break;
        case ANCHOR_CENTER:
            position[cross] = (dock_size[cross] - size[cross]) / 2.0f;
            break;
        case ANCHOR_END:
        default:
            position[cross] = dock_size[cross] - size[cross] - hover_lift * influence;
            break;
        }
        cursor += size[main] + spacings[i] + separation;

        const Rect2 rect(position, size);
        item_rects.push_back(rect);
        if (resize_mode == RESIZE_SCALE)
        {
            // Taille normale mise à l'échelle depuis le coin : le rendu occupe exactement rect
            const Vector2 base = _get_base_size(items[i]);
            items[i]->set_pivot_offset(Vector2());
            items[i]->set_size(base);
            items[i]->set_position(position);
            items[i]->set_scale(base[main] > 0 ? Vector2(1, 1) * (size[main] / base[main]) : Vector2(1, 1));
        }
        else
        {
            fit_child_in_rect(items[i], rect);
        }
    }
}

Vector2 FloatingDock::_get_minimum_size() const
{
    // Taille au repos : le grossissement déborde du dock sans le redimensionner
    const int main = vertical ? 1 : 0;
    const int cross = 1 - main;
    LocalVector<Control *> items;
    _get_items(items);
    Vector2 minimum;
    for (Control *item : items)
    {
        const Vector2 base = _get_base_size(item);
        minimum[main] += base[main];
        minimum[cross] = Math::max(minimum[cross], base[cross]);
    }
    if (!items.is_empty())
    {
        minimum[main] += separation * (items.size() - 1);
    }
    return minimum;
}

// 🎛️ Réglages

void FloatingDock::set_vertical(bool p_value)
{
    vertical = p_value;
    _relayout();
}
void FloatingDock::set_item_size(const Vector2 &p_value)
{
    item_size = p_value;
    _relayout();
}
void FloatingDock::set_separation(float p_value)
{
    separation = p_value;
    _relayout();
}
void FloatingDock::set_magnification(float p_value)
{
    magnification = p_value;
    _relayout();
}
void FloatingDock::set_hover_spacing(float p_value)
{
    hover_spacing = p_value;
    _relayout();
}
void FloatingDock::set_hover_lift(float p_value)
{
    hover_lift = p_value;
    _relayout();
}
void FloatingDock::set_item_anchor(ItemAnchor p_value)
{
    item_anchor = p_value;
    _relayout();
}
void FloatingDock::set_resize_mode(ResizeMode p_value)
{
    resize_mode = p_value;
    if (resize_mode == RESIZE_SIZE)
    {
        // Les éléments ne gardent pas l'échelle du mode précédent
        LocalVector<Control *> items;
        _get_items(items);
        for (Control *item : items)
        {
            item->set_scale(Vector2(1, 1));
        }
    }
    _relayout();
}

// 🔗 Liaisons

void FloatingDock::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_hovered_item"), &FloatingDock::get_hovered_item);
    ClassDB::bind_method(D_METHOD("get_item_influence", "index"), &FloatingDock::get_item_influence);
    ClassDB::bind_method(D_METHOD("set_vertical", "value"), &FloatingDock::set_vertical);
    ClassDB::bind_method(D_METHOD("is_vertical"), &FloatingDock::is_vertical);
    ClassDB::bind_method(D_METHOD("set_item_size", "value"), &FloatingDock::set_item_size);
    ClassDB::bind_method(D_METHOD("get_item_size"), &FloatingDock::get_item_size);
    ClassDB::bind_method(D_METHOD("set_separation", "value"), &FloatingDock::set_separation);
    ClassDB::bind_method(D_METHOD("get_separation"), &FloatingDock::get_separation);
    ClassDB::bind_method(D_METHOD("set_magnification", "value"), &FloatingDock::set_magnification);
    ClassDB::bind_method(D_METHOD("get_magnification"), &FloatingDock::get_magnification);
    ClassDB::bind_method(D_METHOD("set_influence_radius", "value"), &FloatingDock::set_influence_radius);
    ClassDB::bind_method(D_METHOD("get_influence_radius"), &FloatingDock::get_influence_radius);
    ClassDB::bind_method(D_METHOD("set_hover_spacing", "value"), &FloatingDock::set_hover_spacing);
    ClassDB::bind_method(D_METHOD("get_hover_spacing"), &FloatingDock::get_hover_spacing);
    ClassDB::bind_method(D_METHOD("set_hover_lift", "value"), &FloatingDock::set_hover_lift);
    ClassDB::bind_method(D_METHOD("get_hover_lift"), &FloatingDock::get_hover_lift);
    ClassDB::bind_method(D_METHOD("set_item_anchor", "value"), &FloatingDock::set_item_anchor);
    ClassDB::bind_method(D_METHOD("get_item_anchor"), &FloatingDock::get_item_anchor);
    ClassDB::bind_method(D_METHOD("set_resize_mode", "value"), &FloatingDock::set_resize_mode);
    ClassDB::bind_method(D_METHOD("get_resize_mode"), &FloatingDock::get_resize_mode);
    ClassDB::bind_method(D_METHOD("set_smoothing", "value"), &FloatingDock::set_smoothing);
    ClassDB::bind_method(D_METHOD("get_smoothing"), &FloatingDock::get_smoothing);
    ClassDB::bind_method(D_METHOD("set_hover_margin", "value"), &FloatingDock::set_hover_margin);
    ClassDB::bind_method(D_METHOD("get_hover_margin"), &FloatingDock::get_hover_margin);
    ClassDB::bind_method(D_METHOD("set_follow_focus", "value"), &FloatingDock::set_follow_focus);
    ClassDB::bind_method(D_METHOD("get_follow_focus"), &FloatingDock::get_follow_focus);

    BIND_ENUM_CONSTANT(ANCHOR_BEGIN);
    BIND_ENUM_CONSTANT(ANCHOR_CENTER);
    BIND_ENUM_CONSTANT(ANCHOR_END);
    BIND_ENUM_CONSTANT(RESIZE_SIZE);
    BIND_ENUM_CONSTANT(RESIZE_SCALE);

    ADD_GROUP("Layout", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "vertical"), "set_vertical", "is_vertical");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "item_size", PROPERTY_HINT_NONE, "suffix:px"), "set_item_size", "get_item_size");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "separation", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_separation", "get_separation");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "item_anchor", PROPERTY_HINT_ENUM, "Begin,Center,End"), "set_item_anchor", "get_item_anchor");

    ADD_GROUP("Magnification", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "magnification", PROPERTY_HINT_RANGE, "1,4,0.05,or_greater"), "set_magnification", "get_magnification");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "influence_radius", PROPERTY_HINT_RANGE, "1,1000,1,or_greater,suffix:px"), "set_influence_radius", "get_influence_radius");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hover_spacing", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_hover_spacing", "get_hover_spacing");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hover_lift", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_hover_lift", "get_hover_lift");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "resize_mode", PROPERTY_HINT_ENUM, "Size,Scale"), "set_resize_mode", "get_resize_mode");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "smoothing", PROPERTY_HINT_RANGE, "0,40,0.1,or_greater"), "set_smoothing", "get_smoothing");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hover_margin", PROPERTY_HINT_RANGE, "0,128,1,or_greater,suffix:px"), "set_hover_margin", "get_hover_margin");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "follow_focus"), "set_follow_focus", "get_follow_focus");

    ADD_SIGNAL(MethodInfo("hovered_item_changed", PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, "Control")));
}
