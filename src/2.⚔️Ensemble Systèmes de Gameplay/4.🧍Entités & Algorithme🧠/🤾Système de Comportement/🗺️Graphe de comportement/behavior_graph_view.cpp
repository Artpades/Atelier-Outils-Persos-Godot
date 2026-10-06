#include "behavior_graph_view.hpp"

#include "../🌿Arbre de Comportement/behavior_tree.hpp"
#include "../🤖HFSM/state_machine.hpp"
#include "../../💬Système de Dialogue/dialogue_system.hpp"

#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace
{
// Dimensions dans le repère du graphe (zoom 1)
constexpr float NODE_HEIGHT = 60.0f;
constexpr float NODE_MIN_WIDTH = 130.0f;
constexpr float NODE_MAX_WIDTH = 280.0f;
constexpr float NODE_PADDING = 12.0f;
constexpr float KIND_BAR_WIDTH = 6.0f;
constexpr float GAP_MAIN = 90.0f;  // entre deux niveaux (place pour les arcs des transitions)
constexpr float GAP_CROSS = 16.0f; // entre deux frères
constexpr float FOLD_RADIUS = 7.0f;
constexpr int NAME_FONT_SIZE = 15;
constexpr int SMALL_FONT_SIZE = 12;
constexpr float MIN_ZOOM = 0.2f;
constexpr float MAX_ZOOM = 3.0f;

const Color BACKGROUND(0.105f, 0.115f, 0.135f);
const Color GRID(1, 1, 1, 0.035f);
const Color NODE_BACKGROUND(0.16f, 0.17f, 0.2f);
const Color EDGE_IDLE(0.55f, 0.58f, 0.65f, 0.55f);
const Color ACTIVE_COLOR(0.3f, 0.9f, 1.0f);
const Color TEXT_MAIN(0.95f, 0.96f, 0.98f);
const Color TEXT_DIM(0.7f, 0.72f, 0.78f);
const Color LINK_AUTO(1.0f, 0.62f, 0.3f, 0.85f);
const Color LINK_EVENT(0.78f, 0.55f, 1.0f, 0.85f);
const Color SELECTION(1.0f, 1.0f, 1.0f, 0.9f);

/// Coupe p_text (avec « … ») pour qu'il tienne dans p_width pixels.
String fit_text(const Ref<Font> &p_font, const String &p_text, int p_size, float p_width)
{
    if (p_font.is_null() || p_font->get_string_size(p_text, HORIZONTAL_ALIGNMENT_LEFT, -1, p_size).x <= p_width)
    {
        return p_text;
    }
    String text = p_text;
    while (text.length() > 1 && p_font->get_string_size(text + U"…", HORIZONTAL_ALIGNMENT_LEFT, -1, p_size).x > p_width)
    {
        text = text.substr(0, text.length() - 1);
    }
    return text + U"…";
}

Vector2 bezier_point(const Vector2 &p_a, const Vector2 &p_b, const Vector2 &p_c, const Vector2 &p_d, float p_t)
{
    float u = 1.0f - p_t;
    return p_a * (u * u * u) + p_b * (3.0f * u * u * p_t) + p_c * (3.0f * u * p_t * p_t) + p_d * (p_t * p_t * p_t);
}
} // namespace

BehaviorGraphView::BehaviorGraphView()
{
    set_clip_contents(true);
    set_focus_mode(FOCUS_CLICK);
    set_default_cursor_shape(CURSOR_ARROW);
    placeholder_text = U"Aucun comportement à afficher : choisissez une StateMachine ou un BehaviorTree.";
    box_style.instantiate();
}

// 🎯 Source

Node *BehaviorGraphView::resolve_runner(Node *p_node)
{
    if (State *state = Object::cast_to<State>(p_node))
    {
        return state->get_state_machine();
    }
    if (BTNode *bt_node = Object::cast_to<BTNode>(p_node))
    {
        return bt_node->get_behavior_tree();
    }
    if (Object::cast_to<BehaviorTree>(p_node) || Object::cast_to<DialogueSystem>(p_node))
    {
        return p_node;
    }
    return nullptr;
}

void BehaviorGraphView::set_target(Node *p_target)
{
    Node *runner = resolve_runner(p_target);
    ObjectID id = runner ? ObjectID(runner->get_instance_id()) : ObjectID();
    if (id == target_id)
    {
        return;
    }
    target_id = id;
    fit_pending = true;
    structure_signature = String();
    if (runner)
    {
        refresh();
    }
    else
    {
        clear();
    }
}

Node *BehaviorGraphView::get_target() const { return Object::cast_to<Node>(ObjectDB::get_instance(target_id)); }

void BehaviorGraphView::set_target_path(const NodePath &p_path)
{
    target_path = p_path;
    if (is_inside_tree())
    {
        set_target(target_path.is_empty() ? nullptr : get_node_or_null(target_path));
    }
}

void BehaviorGraphView::set_live_update(bool p_live)
{
    live_update = p_live;
    refresh_accumulator = 0.0;
}

void BehaviorGraphView::refresh()
{
    Node *target = get_target();
    if (!target)
    {
        return;
    }
    Dictionary data = target->call("get_debug_snapshot");
    _apply_snapshot(data);
}

void BehaviorGraphView::set_snapshot(const Dictionary &p_snapshot) { _apply_snapshot(p_snapshot); }

void BehaviorGraphView::clear()
{
    snapshot = Dictionary();
    nodes.clear();
    index_of.clear();
    structure_signature = String();
    hovered_id = String();
    queue_redraw();
}

// 🧱 Modèle et disposition

void BehaviorGraphView::_apply_snapshot(const Dictionary &p_snapshot)
{
    bool was_empty = nodes.is_empty();
    snapshot = p_snapshot;
    nodes.clear();
    index_of.clear();

    Array raw = p_snapshot.get("nodes", Array());
    String signature = String(p_snapshot.get("type", "")) + ";";
    for (int i = 0; i < raw.size(); i++)
    {
        Dictionary data = raw[i];
        ViewNode node;
        node.id = data.get("id", "");
        node.parent = data.get("parent", "");
        node.name = data.get("name", "");
        node.class_name = data.get("class", "");
        node.kind = data.get("kind", "");
        node.info = data.get("info", "");
        node.edge_label = data.get("edge_label", "");
        node.status = data.get("status", -1);
        node.active = data.get("active", false);
        node.fresh = data.get("fresh", true);
        node.initial = data.get("initial", false);
        node.links = data.get("links", Array());
        if (node.id.is_empty() || index_of.has(node.id))
        {
            continue;
        }
        signature += node.id + "|" + node.parent + "|" + node.name + "|" + node.class_name + "|" + node.kind + "|" + node.edge_label + "|" + (node.initial ? "i" : "") + ";";
        for (int l = 0; l < node.links.size(); l++)
        {
            Dictionary link = node.links[l];
            signature += String(link.get("to", "")) + ">" + String(link.get("label", "")) + ",";
        }
        index_of[node.id] = int(nodes.size());
        nodes.push_back(node);
    }
    for (uint32_t i = 0; i < nodes.size(); i++)
    {
        HashMap<String, int>::ConstIterator parent = index_of.find(nodes[i].parent);
        if (parent != index_of.end() && parent->value != int(i))
        {
            nodes[i].parent_index = parent->value;
            nodes[parent->value].children.push_back(i);
        }
    }

    if (signature != structure_signature || was_empty)
    {
        structure_signature = signature;
        _layout();
        if (was_empty)
        {
            fit_pending = true;
        }
    }
    else
    {
        // Même structure : on garde les positions calculées.
        _layout();
    }
    if (fit_pending && get_size().x > 0 && get_size().y > 0 && !nodes.is_empty())
    {
        fit_to_view();
    }
    queue_redraw();
}

Vector2 BehaviorGraphView::_node_size(const ViewNode &p_node) const
{
    Ref<Font> font = get_theme_default_font();
    float width = NODE_MIN_WIDTH;
    if (font.is_valid())
    {
        float name_width = font->get_string_size(p_node.name, HORIZONTAL_ALIGNMENT_LEFT, -1, NAME_FONT_SIZE).x;
        float class_width = font->get_string_size(p_node.class_name + String(U" (initial)"), HORIZONTAL_ALIGNMENT_LEFT, -1, SMALL_FONT_SIZE).x;
        width = MAX(width, MAX(name_width, class_width) + 2.0f * NODE_PADDING + KIND_BAR_WIDTH);
    }
    return Vector2(CLAMP(width, NODE_MIN_WIDTH, NODE_MAX_WIDTH), NODE_HEIGHT);
}

int BehaviorGraphView::_visible_child_count(const ViewNode &p_node) const { return collapsed.has(p_node.id) ? 0 : int(p_node.children.size()); }

void BehaviorGraphView::_measure(int p_index, bool p_cross_is_y)
{
    ViewNode &node = nodes[p_index];
    float own = (p_cross_is_y ? node.size.y : node.size.x) + GAP_CROSS;
    float sum = 0.0f;
    if (!collapsed.has(node.id))
    {
        for (uint32_t c = 0; c < node.children.size(); c++)
        {
            _measure(node.children[c], p_cross_is_y);
            sum += nodes[node.children[c]].breadth;
        }
    }
    node.breadth = MAX(own, sum);
}

void BehaviorGraphView::_place(int p_index, float p_cross_start, bool p_cross_is_y)
{
    ViewNode &node = nodes[p_index];
    float cross_size = p_cross_is_y ? node.size.y : node.size.x;
    float cross = p_cross_start + (node.breadth - cross_size) * 0.5f;

    if (layout_mode == LAYOUT_TOP_DOWN)
    {
        node.position = Vector2(cross, node.depth * (NODE_HEIGHT + GAP_MAIN));
    }
    else if (node.side < 0)
    {
        float column = node.depth < int(column_left.size()) ? column_left[node.depth] : 0.0f;
        node.position = Vector2(-column - node.size.x, cross);
    }
    else
    {
        float column = node.depth < int(column_right.size()) ? column_right[node.depth] : 0.0f;
        node.position = Vector2(column, cross);
    }

    if (collapsed.has(node.id))
    {
        return;
    }
    float sum = 0.0f;
    for (uint32_t c = 0; c < node.children.size(); c++)
    {
        sum += nodes[node.children[c]].breadth;
    }
    float child_cross = p_cross_start + (node.breadth - sum) * 0.5f;
    for (uint32_t c = 0; c < node.children.size(); c++)
    {
        _place(node.children[c], child_cross, p_cross_is_y);
        child_cross += nodes[node.children[c]].breadth;
    }
}

void BehaviorGraphView::_layout()
{
    if (nodes.is_empty())
    {
        graph_bounds = Rect2();
        return;
    }
    const bool cross_is_y = layout_mode != LAYOUT_TOP_DOWN;
    LocalVector<int> roots;
    for (uint32_t i = 0; i < nodes.size(); i++)
    {
        ViewNode &node = nodes[i];
        node.size = _node_size(node);
        node.depth = 0;
        node.hidden = false;
        node.side = 1;
        for (int p = node.parent_index, guard = 0; p >= 0 && guard < 256; p = nodes[p].parent_index, guard++)
        {
            node.depth++;
            if (collapsed.has(nodes[p].id))
            {
                node.hidden = true;
            }
        }
        if (node.parent_index < 0)
        {
            roots.push_back(i);
        }
    }
    for (uint32_t r = 0; r < roots.size(); r++)
    {
        _measure(roots[r], cross_is_y);
    }

    // Carte mentale : les enfants de la racine sont répartis à droite puis à gauche (moitié de l'encombrement).
    ViewNode &main_root = nodes[roots[0]];
    LocalVector<int> right;
    LocalVector<int> left;
    if (layout_mode == LAYOUT_MIND_MAP && !collapsed.has(main_root.id))
    {
        float total = 0.0f;
        for (uint32_t c = 0; c < main_root.children.size(); c++)
        {
            total += nodes[main_root.children[c]].breadth;
        }
        float accumulated = 0.0f;
        for (uint32_t c = 0; c < main_root.children.size(); c++)
        {
            int child = main_root.children[c];
            float breadth = nodes[child].breadth;
            bool to_right = right.is_empty() || accumulated + breadth * 0.5f <= total * 0.5f;
            (to_right ? right : left).push_back(child);
            accumulated += breadth;
        }
        // Le côté gauche hérite son signe dans toute la branche.
        for (uint32_t i = 0; i < nodes.size(); i++)
        {
            for (int p = int(i); p >= 0; p = nodes[p].parent_index)
            {
                if (nodes[p].parent_index == int(roots[0]))
                {
                    nodes[i].side = left.find(p) >= 0 ? -1 : 1;
                    break;
                }
            }
        }
    }

    // Colonnes : largeur maximale par profondeur (et par côté pour la carte mentale).
    LocalVector<float> width_right;
    LocalVector<float> width_left;
    for (uint32_t i = 0; i < nodes.size(); i++)
    {
        const ViewNode &node = nodes[i];
        if (node.hidden)
        {
            continue;
        }
        LocalVector<float> &widths = node.side < 0 ? width_left : width_right;
        while (int(widths.size()) <= node.depth)
        {
            widths.push_back(0.0f);
        }
        widths[node.depth] = MAX(widths[node.depth], node.size.x);
    }
    column_right.clear();
    column_left.clear();
    if (layout_mode == LAYOUT_MIND_MAP)
    {
        float half_root = main_root.size.x * 0.5f;
        column_right.push_back(-half_root);
        column_left.push_back(-half_root);
        float x_right = half_root + GAP_MAIN;
        float x_left = half_root + GAP_MAIN;
        for (int d = 1; d < 64; d++)
        {
            column_right.push_back(x_right);
            column_left.push_back(x_left);
            x_right += (d < int(width_right.size()) ? width_right[d] : 0.0f) + GAP_MAIN;
            x_left += (d < int(width_left.size()) ? width_left[d] : 0.0f) + GAP_MAIN;
        }
    }
    else
    {
        float x = 0.0f;
        for (int d = 0; d < 64; d++)
        {
            column_right.push_back(x);
            x += (d < int(width_right.size()) ? width_right[d] : 0.0f) + GAP_MAIN;
        }
    }

    if (layout_mode == LAYOUT_MIND_MAP)
    {
        main_root.position = Vector2(-main_root.size.x * 0.5f, -main_root.size.y * 0.5f);
        for (int side = 0; side < 2; side++)
        {
            LocalVector<int> &branch = side == 0 ? right : left;
            float sum = 0.0f;
            for (uint32_t c = 0; c < branch.size(); c++)
            {
                sum += nodes[branch[c]].breadth;
            }
            float cross = -sum * 0.5f;
            for (uint32_t c = 0; c < branch.size(); c++)
            {
                _place(branch[c], cross, true);
                cross += nodes[branch[c]].breadth;
            }
        }
        for (uint32_t r = 1; r < roots.size(); r++)
        {
            _place(roots[r], graph_bounds.get_end().y + GAP_MAIN, true); // racines supplémentaires (rare)
        }
    }
    else
    {
        float cross = 0.0f;
        for (uint32_t r = 0; r < roots.size(); r++)
        {
            _place(roots[r], cross, cross_is_y);
            cross += nodes[roots[r]].breadth;
        }
    }

    bool first = true;
    for (uint32_t i = 0; i < nodes.size(); i++)
    {
        if (nodes[i].hidden)
        {
            continue;
        }
        Rect2 rect(nodes[i].position, nodes[i].size);
        graph_bounds = first ? rect : graph_bounds.merge(rect);
        first = false;
    }
}

Vector2 BehaviorGraphView::_out_direction(const ViewNode &p_node) const
{
    if (layout_mode == LAYOUT_TOP_DOWN)
    {
        return Vector2(0, 1);
    }
    return Vector2(float(p_node.side), 0);
}

Vector2 BehaviorGraphView::_anchor(const ViewNode &p_node, const Vector2 &p_direction) const
{
    Vector2 center = p_node.position + p_node.size * 0.5f;
    return center + Vector2(p_direction.x * p_node.size.x * 0.5f, p_direction.y * p_node.size.y * 0.5f);
}

Rect2 BehaviorGraphView::_screen_rect(const ViewNode &p_node) const { return Rect2(_to_screen(p_node.position), p_node.size * zoom); }

Vector2 BehaviorGraphView::_fold_button_center(const ViewNode &p_node) const
{
    Vector2 direction = _out_direction(p_node);
    return _to_screen(_anchor(p_node, direction) + direction * (FOLD_RADIUS + 3.0f));
}

bool BehaviorGraphView::_fold_button_hit(const ViewNode &p_node, const Vector2 &p_position) const
{
    return !p_node.children.is_empty() && p_position.distance_to(_fold_button_center(p_node)) <= (FOLD_RADIUS + 3.0f) * MAX(zoom, 0.6f);
}

int BehaviorGraphView::_node_index_at(const Vector2 &p_position) const
{
    for (int64_t i = int64_t(nodes.size()) - 1; i >= 0; i--)
    {
        const ViewNode &node = nodes[i];
        if (!node.hidden && (_screen_rect(node).has_point(p_position) || _fold_button_hit(node, p_position)))
        {
            return int(i);
        }
    }
    return -1;
}

// 🔍 Requêtes

PackedStringArray BehaviorGraphView::get_node_ids() const
{
    PackedStringArray ids;
    for (uint32_t i = 0; i < nodes.size(); i++)
    {
        ids.push_back(nodes[i].id);
    }
    return ids;
}

Dictionary BehaviorGraphView::get_node_data(const String &p_id) const
{
    Array raw = snapshot.get("nodes", Array());
    for (int i = 0; i < raw.size(); i++)
    {
        Dictionary data = raw[i];
        if (String(data.get("id", "")) == p_id)
        {
            return data;
        }
    }
    return Dictionary();
}

Rect2 BehaviorGraphView::get_node_rect(const String &p_id) const
{
    HashMap<String, int>::ConstIterator found = index_of.find(p_id);
    if (found == index_of.end() || nodes[found->value].hidden)
    {
        return Rect2();
    }
    return _screen_rect(nodes[found->value]);
}

String BehaviorGraphView::get_node_at(const Vector2 &p_position) const
{
    int index = _node_index_at(p_position);
    return index >= 0 ? nodes[index].id : String();
}

// 🎛️ Affichage

void BehaviorGraphView::set_layout_mode(LayoutMode p_mode)
{
    if (layout_mode == p_mode)
    {
        return;
    }
    layout_mode = p_mode;
    _layout();
    fit_to_view();
}

void BehaviorGraphView::set_show_transition_labels(bool p_show)
{
    show_transition_labels = p_show;
    queue_redraw();
}

void BehaviorGraphView::set_show_history(bool p_show)
{
    show_history = p_show;
    queue_redraw();
}

void BehaviorGraphView::set_placeholder_text(const String &p_text)
{
    placeholder_text = p_text;
    queue_redraw();
}

void BehaviorGraphView::set_selected_id(const String &p_id)
{
    selected_id = p_id;
    queue_redraw();
}

void BehaviorGraphView::set_zoom(float p_zoom)
{
    // Zoom autour du centre du contrôle
    Vector2 center = get_size() * 0.5f;
    Vector2 graph_point = (center - view_offset) / zoom;
    zoom = CLAMP(p_zoom, MIN_ZOOM, MAX_ZOOM);
    view_offset = center - graph_point * zoom;
    queue_redraw();
}

void BehaviorGraphView::set_view_offset(const Vector2 &p_offset)
{
    view_offset = p_offset;
    queue_redraw();
}

void BehaviorGraphView::fit_to_view()
{
    Vector2 size = get_size();
    if (nodes.is_empty() || size.x <= 0 || size.y <= 0)
    {
        fit_pending = true;
        return;
    }
    fit_pending = false;
    Rect2 bounds = graph_bounds.grow(40.0f);
    zoom = CLAMP(MIN(size.x / bounds.size.x, size.y / bounds.size.y), MIN_ZOOM, 1.25f);
    view_offset = size * 0.5f - bounds.get_center() * zoom;
    queue_redraw();
}

void BehaviorGraphView::center_on(const String &p_id)
{
    HashMap<String, int>::ConstIterator found = index_of.find(p_id);
    if (found == index_of.end())
    {
        return;
    }
    const ViewNode &node = nodes[found->value];
    view_offset = get_size() * 0.5f - (node.position + node.size * 0.5f) * zoom;
    queue_redraw();
}

void BehaviorGraphView::toggle_collapsed(const String &p_id)
{
    if (collapsed.has(p_id))
    {
        collapsed.erase(p_id);
    }
    else
    {
        collapsed.insert(p_id);
    }
    _layout();
    queue_redraw();
}

// 🎨 Dessin

Color BehaviorGraphView::_kind_color(const String &p_kind)
{
    if (p_kind == "machine" || p_kind == "tree")
    {
        return Color(0.65f, 0.48f, 0.98f);
    }
    if (p_kind == "compound")
    {
        return Color(0.36f, 0.56f, 0.98f);
    }
    if (p_kind == "state")
    {
        return Color(0.3f, 0.72f, 0.95f);
    }
    if (p_kind == "composite")
    {
        return Color(0.25f, 0.78f, 0.7f);
    }
    if (p_kind == "decorator")
    {
        return Color(0.92f, 0.76f, 0.3f);
    }
    if (p_kind == "condition")
    {
        return Color(0.98f, 0.55f, 0.25f);
    }
    if (p_kind == "action")
    {
        return Color(0.42f, 0.82f, 0.42f);
    }
    return Color(0.6f, 0.6f, 0.65f);
}

Color BehaviorGraphView::_status_color(int p_status)
{
    switch (p_status)
    {
    case BTNode::SUCCESS:
        return Color(0.35f, 0.86f, 0.45f);
    case BTNode::FAILURE:
        return Color(0.96f, 0.36f, 0.36f);
    case BTNode::RUNNING:
        return Color(1.0f, 0.76f, 0.2f);
    default:
        return TEXT_DIM;
    }
}

String BehaviorGraphView::_status_text(int p_status)
{
    switch (p_status)
    {
    case BTNode::SUCCESS:
        return "SUCCESS";
    case BTNode::FAILURE:
        return "FAILURE";
    case BTNode::RUNNING:
        return "RUNNING";
    default:
        return String();
    }
}

void BehaviorGraphView::_draw_bezier(const Vector2 &p_a, const Vector2 &p_b, const Vector2 &p_c, const Vector2 &p_d, const Color &p_color, float p_width)
{
    PackedVector2Array points;
    const int segments = 20;
    for (int i = 0; i <= segments; i++)
    {
        points.push_back(bezier_point(p_a, p_b, p_c, p_d, float(i) / segments));
    }
    draw_polyline(points, p_color, p_width, true);
}

void BehaviorGraphView::_draw_arrow_head(const Vector2 &p_tip, const Vector2 &p_direction, const Color &p_color, float p_size)
{
    Vector2 direction = p_direction.length_squared() > 0.0001f ? p_direction.normalized() : Vector2(1, 0);
    Vector2 normal(-direction.y, direction.x);
    PackedVector2Array triangle;
    triangle.push_back(p_tip);
    triangle.push_back(p_tip - direction * p_size + normal * p_size * 0.5f);
    triangle.push_back(p_tip - direction * p_size - normal * p_size * 0.5f);
    draw_colored_polygon(triangle, p_color);
}

void BehaviorGraphView::_draw_edge(const ViewNode &p_parent, const ViewNode &p_child)
{
    Vector2 direction = _out_direction(p_child);
    Vector2 start = _to_screen(_anchor(p_parent, direction) + direction * (p_parent.children.is_empty() ? 0.0f : 2.0f * FOLD_RADIUS + 3.0f));
    Vector2 end = _to_screen(_anchor(p_child, -direction));
    float reach = Math::abs((end - start).dot(direction)) * 0.5f;
    Color color = EDGE_IDLE;
    float width = 2.0f;
    if (p_child.active && p_child.status < 0)
    {
        color = ACTIVE_COLOR;
        width = 3.0f;
    }
    else if (p_child.status >= 0 && p_child.fresh)
    {
        color = _status_color(p_child.status);
        width = p_child.status == BTNode::RUNNING ? 3.0f : 2.0f;
    }
    _draw_bezier(start, start + direction * reach, end - direction * reach, end, color, MAX(1.0f, width * MIN(zoom, 1.5f)));

    Ref<Font> font = get_theme_default_font();
    if (!p_child.edge_label.is_empty() && show_transition_labels && zoom >= 0.55f && font.is_valid())
    {
        int font_size = MAX(8, int(Math::round(11.0f * zoom)));
        String label = fit_text(font, p_child.edge_label, font_size, 160.0f * zoom);
        Vector2 text_size = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
        Vector2 middle = bezier_point(start, start + direction * reach, end - direction * reach, end, 0.5f);
        Rect2 back(middle - text_size * 0.5f - Vector2(4, 2), text_size + Vector2(8, 4));
        draw_rect(back, Color(BACKGROUND, 0.9f));
        draw_string(font, Vector2(back.position.x + 4, back.position.y + 2 + font->get_ascent(font_size)), label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, TEXT_DIM);
    }
}

void BehaviorGraphView::_draw_link(const ViewNode &p_from, const ViewNode &p_to, const Dictionary &p_link, int p_rank)
{
    bool event = !String(p_link.get("event", "")).is_empty();
    Color color = event ? LINK_EVENT : LINK_AUTO;
    if (!p_from.active)
    {
        color.a *= 0.55f;
    }
    float width = MAX(1.0f, 1.6f * MIN(zoom, 1.5f));
    Vector2 direction = _out_direction(p_from);
    // Les arcs passent du côté du parent : ils ne traversent jamais la colonne des enfants.
    if (&p_from != &p_to)
    {
        direction = -direction;
    }
    Vector2 start;
    Vector2 end;
    Vector2 control_a;
    Vector2 control_b;
    if (&p_from == &p_to)
    {
        // Transition vers soi-même : petite boucle sur le côté
        Vector2 normal(-direction.y, direction.x);
        Vector2 anchor = _anchor(p_from, direction);
        start = _to_screen(anchor - normal * 10.0f);
        end = _to_screen(anchor + normal * 10.0f);
        control_a = start + (direction * 45.0f - normal * 25.0f) * zoom;
        control_b = end + (direction * 45.0f + normal * 25.0f) * zoom;
    }
    else
    {
        // Arc sur le côté « sortant » : les frères empilés sont reliés sans traverser les autres nœuds.
        start = _to_screen(_anchor(p_from, direction));
        end = _to_screen(_anchor(p_to, direction));
        float distance = start.distance_to(end) / MAX(zoom, 0.01f);
        float bulge = (34.0f + distance * 0.22f + p_rank * 16.0f) * zoom;
        control_a = start + direction * bulge;
        control_b = end + direction * bulge;
    }
    _draw_bezier(start, control_a, control_b, end, color, width);
    _draw_arrow_head(end, end - control_b, color, 10.0f * MIN(MAX(zoom, 0.6f), 1.5f));

    if (show_transition_labels && zoom >= 0.55f)
    {
        String label = p_link.get("label", "");
        Ref<Font> font = get_theme_default_font();
        if (!label.is_empty() && font.is_valid())
        {
            int font_size = MAX(8, int(Math::round(11.0f * zoom)));
            label = fit_text(font, label, font_size, 220.0f * zoom);
            Vector2 text_size = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
            Vector2 middle = bezier_point(start, control_a, control_b, end, 0.5f);
            Rect2 back(middle - text_size * 0.5f - Vector2(4, 2), text_size + Vector2(8, 4));
            draw_rect(back, Color(BACKGROUND, 0.85f));
            draw_rect(back, Color(color, 0.6f), false, 1.0f);
            draw_string(font, Vector2(back.position.x + 4, back.position.y + 2 + font->get_ascent(font_size)), label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(color, 1.0f));
        }
    }
}

void BehaviorGraphView::_draw_node(const ViewNode &p_node)
{
    Rect2 rect = _screen_rect(p_node);
    if (!Rect2(Vector2(), get_size()).grow(40.0f).intersects(rect))
    {
        return; // hors de la vue
    }
    Color kind_color = _kind_color(p_node.kind);
    bool stale = p_node.status >= 0 && !p_node.fresh;
    bool selected = p_node.id == selected_id;
    bool hovered = p_node.id == hovered_id;

    // Halo de l'état actif / du nœud en cours
    Color highlight = Color(0, 0, 0, 0);
    if (p_node.active)
    {
        highlight = p_node.status >= 0 ? _status_color(BTNode::RUNNING) : ACTIVE_COLOR;
    }
    else if (p_node.status >= 0 && p_node.fresh)
    {
        highlight = _status_color(p_node.status);
    }
    if (highlight.a > 0 && p_node.active)
    {
        box_style->set_bg_color(Color(highlight, 0.18f));
        box_style->set_border_width_all(0);
        box_style->set_corner_radius_all(int(10 * zoom));
        draw_style_box(box_style, rect.grow(5.0f * zoom));
    }

    Color border = highlight.a > 0 ? highlight : Color(kind_color, 0.55f);
    if (selected)
    {
        border = SELECTION;
    }
    box_style->set_bg_color(hovered ? NODE_BACKGROUND.lightened(0.08f) : NODE_BACKGROUND);
    box_style->set_border_color(border);
    box_style->set_border_width_all(MAX(1, int(Math::round((selected || highlight.a > 0 ? 2.5f : 1.0f) * MIN(zoom, 1.5f)))));
    box_style->set_corner_radius_all(int(6 * zoom));
    box_style->set_anti_aliased(true);
    draw_style_box(box_style, rect);
    // Barre de catégorie à gauche
    draw_rect(Rect2(rect.position + Vector2(1, 1) * zoom, Vector2(KIND_BAR_WIDTH * zoom, rect.size.y - 2 * zoom)), stale ? Color(kind_color, 0.5f) : kind_color);

    // Textes
    Ref<Font> font = get_theme_default_font();
    int name_size = int(Math::round(NAME_FONT_SIZE * zoom));
    int small_size = int(Math::round(SMALL_FONT_SIZE * zoom));
    if (font.is_valid() && name_size >= 6)
    {
        float left = rect.position.x + (KIND_BAR_WIDTH + NODE_PADDING * 0.75f) * zoom;
        float width = rect.size.x - (KIND_BAR_WIDTH + NODE_PADDING * 1.25f) * zoom;
        float y = rect.position.y + 6.0f * zoom;
        Color text = stale ? Color(TEXT_MAIN, 0.6f) : TEXT_MAIN;
        draw_string(font, Vector2(left, y + font->get_ascent(name_size)), fit_text(font, p_node.name, name_size, width), HORIZONTAL_ALIGNMENT_LEFT, -1, name_size, text);
        y += font->get_height(name_size);
        if (small_size < 5)
        {
            small_size = 0; // trop petit : seul le nom est affiché
        }
        String class_line = p_node.class_name + (p_node.initial ? String(U" (initial)") : String());
        if (small_size > 0)
        {
            draw_string(font, Vector2(left, y + font->get_ascent(small_size)), fit_text(font, class_line, small_size, width), HORIZONTAL_ALIGNMENT_LEFT, -1, small_size, Color(TEXT_DIM, stale ? 0.6f : 1.0f));
            y += font->get_height(small_size);
        }

        // Ligne d'état : pastille + statut + détail
        String status_line;
        Color status_color = TEXT_DIM;
        if (p_node.status >= 0)
        {
            status_line = _status_text(p_node.status);
            status_color = _status_color(p_node.status);
            if (stale)
            {
                status_color.a = 0.45f;
            }
        }
        else if (p_node.active)
        {
            status_line = "actif";
            status_color = ACTIVE_COLOR;
        }
        if (!p_node.info.is_empty())
        {
            status_line += (status_line.is_empty() ? String() : String(U" · ")) + p_node.info;
        }
        if (!status_line.is_empty() && small_size > 0)
        {
            float dot = 3.5f * zoom;
            if (p_node.status >= 0 || p_node.active)
            {
                draw_circle(Vector2(left + dot, y + font->get_height(small_size) * 0.5f), dot, status_color, true, -1, true);
                left += dot * 3.0f;
                width -= dot * 3.0f;
            }
            draw_string(font, Vector2(left, y + font->get_ascent(small_size)), fit_text(font, status_line, small_size, width), HORIZONTAL_ALIGNMENT_LEFT, -1, small_size, p_node.status >= 0 || p_node.active ? status_color : TEXT_DIM);
        }
    }

    // Pastille de repli des branches
    if (!p_node.children.is_empty())
    {
        Vector2 center = _fold_button_center(p_node);
        float radius = FOLD_RADIUS * MAX(zoom, 0.6f);
        bool folded = collapsed.has(p_node.id);
        draw_circle(center, radius, NODE_BACKGROUND.lightened(0.15f), true, -1, true);
        draw_circle(center, radius, Color(kind_color, 0.9f), false, 1.0f, true);
        float arm = radius * 0.55f;
        draw_line(center - Vector2(arm, 0), center + Vector2(arm, 0), TEXT_MAIN, 1.5f);
        if (folded)
        {
            draw_line(center - Vector2(0, arm), center + Vector2(0, arm), TEXT_MAIN, 1.5f);
            if (font.is_valid() && small_size >= 6)
            {
                Vector2 direction = _out_direction(p_node);
                String count = "+" + String::num_int64(p_node.children.size());
                Vector2 text_size = font->get_string_size(count, HORIZONTAL_ALIGNMENT_LEFT, -1, small_size);
                Vector2 position = center + direction * (radius + 4.0f) + Vector2(direction.x < 0 ? -text_size.x : 0.0f, direction.y > 0 ? text_size.y * 0.5f : font->get_ascent(small_size) * 0.5f);
                draw_string(font, position, count, HORIZONTAL_ALIGNMENT_LEFT, -1, small_size, TEXT_DIM);
            }
        }
    }
}

void BehaviorGraphView::_draw_overlay()
{
    Ref<Font> font = get_theme_default_font();
    if (font.is_null())
    {
        return;
    }
    const int size = 13;
    String type = snapshot.get("type", "");
    String title = (type == "hfsm" ? String(U"Machine à états") : type == "dialogue" ? String(U"Dialogue") : String(U"Arbre de comportement")) + U" « " + String(snapshot.get("name", "")) + U" »";
    bool running = snapshot.get("running", false);
    double time = snapshot.get("time", 0.0);
    title += running ? U" — en cours · " + String::num(time, 1) + " s" : String(U" — à l'arrêt");
    draw_string(font, Vector2(10, 8 + font->get_ascent(size)), title, HORIZONTAL_ALIGNMENT_LEFT, -1, size, Color(TEXT_MAIN, 0.75f));

    Array history = snapshot.get("history", Array());
    if (show_history && !history.is_empty())
    {
        int count = MIN(5, int(history.size()));
        float line = font->get_height(12);
        float top = get_size().y - 10 - line * (count + 1);
        draw_string(font, Vector2(10, top + font->get_ascent(12)), U"Dernières transitions :", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(TEXT_DIM, 0.8f));
        for (int i = 0; i < count; i++)
        {
            String entry = history[history.size() - count + i];
            float alpha = 0.45f + 0.55f * float(i + 1) / count;
            draw_string(font, Vector2(16, top + line * (i + 1) + font->get_ascent(12)), entry, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(TEXT_MAIN, alpha * 0.8f));
        }
    }
}

void BehaviorGraphView::_draw_graph()
{
    Vector2 size = get_size();
    draw_rect(Rect2(Vector2(), size), BACKGROUND);

    // Grille
    float step = 32.0f * zoom;
    if (step >= 8.0f)
    {
        for (float x = Math::fposmod(view_offset.x, step); x < size.x; x += step)
        {
            draw_line(Vector2(x, 0), Vector2(x, size.y), GRID);
        }
        for (float y = Math::fposmod(view_offset.y, step); y < size.y; y += step)
        {
            draw_line(Vector2(0, y), Vector2(size.x, y), GRID);
        }
    }

    Ref<Font> font = get_theme_default_font();
    if (nodes.is_empty())
    {
        if (font.is_valid())
        {
            draw_string(font, Vector2(20, size.y * 0.5f), placeholder_text, HORIZONTAL_ALIGNMENT_CENTER, size.x - 40, 15, TEXT_DIM);
        }
        return;
    }

    // Liens hiérarchiques, puis transitions, puis nœuds (au-dessus)
    for (uint32_t i = 0; i < nodes.size(); i++)
    {
        const ViewNode &node = nodes[i];
        if (!node.hidden && node.parent_index >= 0)
        {
            _draw_edge(nodes[node.parent_index], node);
        }
    }
    for (uint32_t i = 0; i < nodes.size(); i++)
    {
        const ViewNode &node = nodes[i];
        if (node.hidden)
        {
            continue;
        }
        HashMap<String, int> ranks; // plusieurs transitions vers la même cible : arcs décalés
        for (int l = 0; l < node.links.size(); l++)
        {
            Dictionary link = node.links[l];
            HashMap<String, int>::ConstIterator target = index_of.find(String(link.get("to", "")));
            if (target == index_of.end() || nodes[target->value].hidden)
            {
                continue;
            }
            String to = link.get("to", "");
            int rank = ranks.has(to) ? ranks[to] + 1 : 0;
            ranks[to] = rank;
            _draw_link(node, nodes[target->value], link, rank);
        }
    }
    for (uint32_t i = 0; i < nodes.size(); i++)
    {
        if (!nodes[i].hidden)
        {
            _draw_node(nodes[i]);
        }
    }

    // Glisser en cours : fantôme de la liaison / du déplacement
    if ((drag_mode == DRAG_NODE && drag_moved) || drag_mode == DRAG_LINK)
    {
        HashMap<String, int>::ConstIterator source = index_of.find(drag_id);
        if (source != index_of.end())
        {
            Vector2 from = _screen_rect(nodes[source->value]).get_center();
            Color color = drag_mode == DRAG_LINK ? LINK_AUTO : SELECTION;
            draw_dashed_line(from, drag_mouse, color, 2.0f, 8.0f);
            if (drag_mode == DRAG_LINK)
            {
                _draw_arrow_head(drag_mouse, drag_mouse - from, color, 12.0f);
            }
            String hovered = get_node_at(drag_mouse);
            if (!hovered.is_empty() && hovered != drag_id)
            {
                draw_rect(get_node_rect(hovered).grow(4), color, false, 2.0f);
            }
        }
    }
    _draw_overlay();
}

// 🖱️ Interaction

void BehaviorGraphView::_gui_input(const Ref<InputEvent> &p_event)
{
    Ref<InputEventMouseButton> button = p_event;
    if (button.is_valid())
    {
        Vector2 position = button->get_position();
        MouseButton index = button->get_button_index();
        if ((index == MOUSE_BUTTON_WHEEL_UP || index == MOUSE_BUTTON_WHEEL_DOWN) && button->is_pressed())
        {
            Vector2 graph_point = (position - view_offset) / zoom;
            float factor = index == MOUSE_BUTTON_WHEEL_UP ? 1.12f : 1.0f / 1.12f;
            zoom = CLAMP(zoom * factor, MIN_ZOOM, MAX_ZOOM);
            view_offset = position - graph_point * zoom;
            fit_pending = false;
            queue_redraw();
            accept_event();
            return;
        }
        if (index == MOUSE_BUTTON_LEFT && button->is_pressed())
        {
            grab_focus();
            int hit = _node_index_at(position);
            if (hit >= 0 && _fold_button_hit(nodes[hit], position))
            {
                toggle_collapsed(nodes[hit].id);
                accept_event();
                return;
            }
            if (hit >= 0)
            {
                String id = nodes[hit].id;
                selected_id = id;
                emit_signal("node_selected", id);
                if (button->is_double_click())
                {
                    emit_signal("node_activated", id);
                    drag_mode = DRAG_NONE;
                }
                else if (editable)
                {
                    drag_mode = button->is_shift_pressed() ? DRAG_LINK : DRAG_NODE;
                    drag_id = id;
                    drag_origin = position;
                    drag_mouse = position;
                    drag_moved = false;
                }
            }
            else
            {
                drag_mode = DRAG_PAN;
            }
            queue_redraw();
            accept_event();
            return;
        }
        if (index == MOUSE_BUTTON_LEFT && !button->is_pressed())
        {
            String target = get_node_at(position);
            if (drag_mode == DRAG_NODE && drag_moved && !target.is_empty() && target != drag_id)
            {
                emit_signal("node_dropped", drag_id, target);
            }
            else if (drag_mode == DRAG_LINK && !target.is_empty())
            {
                emit_signal("link_requested", drag_id, target);
            }
            drag_mode = DRAG_NONE;
            queue_redraw();
            accept_event();
            return;
        }
        if (index == MOUSE_BUTTON_MIDDLE)
        {
            drag_mode = button->is_pressed() ? DRAG_PAN : DRAG_NONE;
            accept_event();
            return;
        }
        if (index == MOUSE_BUTTON_RIGHT && button->is_pressed())
        {
            grab_focus();
            String id = get_node_at(position);
            if (!id.is_empty())
            {
                selected_id = id;
                emit_signal("node_selected", id);
            }
            emit_signal("node_context_menu_requested", id, get_screen_position() + position);
            queue_redraw();
            accept_event();
            return;
        }
        return;
    }

    Ref<InputEventMouseMotion> motion = p_event;
    if (motion.is_valid())
    {
        Vector2 position = motion->get_position();
        if (drag_mode == DRAG_PAN)
        {
            view_offset += motion->get_relative();
            fit_pending = false;
            queue_redraw();
        }
        else if (drag_mode == DRAG_NODE || drag_mode == DRAG_LINK)
        {
            drag_mouse = position;
            drag_moved = drag_moved || position.distance_to(drag_origin) > 6.0f;
            queue_redraw();
        }
        String hovered = get_node_at(position);
        if (hovered != hovered_id)
        {
            hovered_id = hovered;
            queue_redraw();
        }
        return;
    }

    Ref<InputEventKey> key = p_event;
    if (key.is_valid() && key->is_pressed() && !key->is_echo())
    {
        switch (key->get_keycode())
        {
        case KEY_F:
            fit_to_view();
            accept_event();
            break;
        case KEY_DELETE:
            if (editable && !selected_id.is_empty())
            {
                emit_signal("delete_requested", selected_id);
                accept_event();
            }
            break;
        case KEY_EQUAL:
        case KEY_PLUS:
        case KEY_KP_ADD:
            set_zoom(zoom * 1.2f);
            accept_event();
            break;
        case KEY_MINUS:
        case KEY_KP_SUBTRACT:
            set_zoom(zoom / 1.2f);
            accept_event();
            break;
        default:
            break;
        }
    }
}

String BehaviorGraphView::_get_tooltip(const Vector2 &p_at_position) const
{
    int index = _node_index_at(p_at_position);
    if (index < 0)
    {
        return String();
    }
    const ViewNode &node = nodes[index];
    String text = node.name + String(" (") + node.class_name + String(")\n") + String(U"Chemin : ") + node.id;
    if (node.status >= 0)
    {
        text += U"\nStatut : " + _status_text(node.status) + (node.fresh ? String() : String(U" (tick précédent)"));
    }
    if (node.active)
    {
        text += U"\nActif";
    }
    if (!node.info.is_empty())
    {
        text += "\n" + node.info;
    }
    for (int l = 0; l < node.links.size(); l++)
    {
        Dictionary link = node.links[l];
        text += U"\n→ " + String(link.get("to", "")) + "  " + String(link.get("label", ""));
    }
    return text;
}

void BehaviorGraphView::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_READY:
        set_process_internal(true);
        if (!target_path.is_empty() && !get_target())
        {
            set_target(get_node_or_null(target_path));
        }
        break;
    case NOTIFICATION_INTERNAL_PROCESS:
        if (live_update && target_id.is_valid())
        {
            if (!get_target())
            {
                target_id = ObjectID();
                clear();
                break;
            }
            refresh_accumulator += get_process_delta_time();
            if (refresh_accumulator >= refresh_interval)
            {
                refresh_accumulator = 0.0;
                refresh();
            }
        }
        break;
    case NOTIFICATION_RESIZED:
        if (fit_pending)
        {
            fit_to_view();
        }
        break;
    case NOTIFICATION_MOUSE_EXIT:
        if (!hovered_id.is_empty())
        {
            hovered_id = String();
            queue_redraw();
        }
        break;
    case NOTIFICATION_DRAW:
        _draw_graph();
        break;
    default:
        break;
    }
}

// 🔗 Liaisons

void BehaviorGraphView::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_target", "target"), &BehaviorGraphView::set_target);
    ClassDB::bind_method(D_METHOD("get_target"), &BehaviorGraphView::get_target);
    ClassDB::bind_method(D_METHOD("set_target_path", "path"), &BehaviorGraphView::set_target_path);
    ClassDB::bind_method(D_METHOD("get_target_path"), &BehaviorGraphView::get_target_path);
    ClassDB::bind_method(D_METHOD("set_live_update", "live"), &BehaviorGraphView::set_live_update);
    ClassDB::bind_method(D_METHOD("is_live_update"), &BehaviorGraphView::is_live_update);
    ClassDB::bind_method(D_METHOD("set_refresh_interval", "interval"), &BehaviorGraphView::set_refresh_interval);
    ClassDB::bind_method(D_METHOD("get_refresh_interval"), &BehaviorGraphView::get_refresh_interval);
    ClassDB::bind_method(D_METHOD("set_snapshot", "snapshot"), &BehaviorGraphView::set_snapshot);
    ClassDB::bind_method(D_METHOD("get_snapshot"), &BehaviorGraphView::get_snapshot);
    ClassDB::bind_method(D_METHOD("refresh"), &BehaviorGraphView::refresh);
    ClassDB::bind_method(D_METHOD("clear"), &BehaviorGraphView::clear);
    ClassDB::bind_method(D_METHOD("set_layout_mode", "mode"), &BehaviorGraphView::set_layout_mode);
    ClassDB::bind_method(D_METHOD("get_layout_mode"), &BehaviorGraphView::get_layout_mode);
    ClassDB::bind_method(D_METHOD("set_editable", "editable"), &BehaviorGraphView::set_editable);
    ClassDB::bind_method(D_METHOD("is_editable"), &BehaviorGraphView::is_editable);
    ClassDB::bind_method(D_METHOD("set_show_transition_labels", "show"), &BehaviorGraphView::set_show_transition_labels);
    ClassDB::bind_method(D_METHOD("get_show_transition_labels"), &BehaviorGraphView::get_show_transition_labels);
    ClassDB::bind_method(D_METHOD("set_show_history", "show"), &BehaviorGraphView::set_show_history);
    ClassDB::bind_method(D_METHOD("get_show_history"), &BehaviorGraphView::get_show_history);
    ClassDB::bind_method(D_METHOD("set_placeholder_text", "text"), &BehaviorGraphView::set_placeholder_text);
    ClassDB::bind_method(D_METHOD("get_placeholder_text"), &BehaviorGraphView::get_placeholder_text);
    ClassDB::bind_method(D_METHOD("set_selected_id", "id"), &BehaviorGraphView::set_selected_id);
    ClassDB::bind_method(D_METHOD("get_selected_id"), &BehaviorGraphView::get_selected_id);
    ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &BehaviorGraphView::set_zoom);
    ClassDB::bind_method(D_METHOD("get_zoom"), &BehaviorGraphView::get_zoom);
    ClassDB::bind_method(D_METHOD("set_view_offset", "offset"), &BehaviorGraphView::set_view_offset);
    ClassDB::bind_method(D_METHOD("get_view_offset"), &BehaviorGraphView::get_view_offset);
    ClassDB::bind_method(D_METHOD("fit_to_view"), &BehaviorGraphView::fit_to_view);
    ClassDB::bind_method(D_METHOD("center_on", "id"), &BehaviorGraphView::center_on);
    ClassDB::bind_method(D_METHOD("toggle_collapsed", "id"), &BehaviorGraphView::toggle_collapsed);
    ClassDB::bind_method(D_METHOD("is_collapsed", "id"), &BehaviorGraphView::is_collapsed);
    ClassDB::bind_method(D_METHOD("get_node_ids"), &BehaviorGraphView::get_node_ids);
    ClassDB::bind_method(D_METHOD("get_node_data", "id"), &BehaviorGraphView::get_node_data);
    ClassDB::bind_method(D_METHOD("get_node_rect", "id"), &BehaviorGraphView::get_node_rect);
    ClassDB::bind_method(D_METHOD("get_node_at", "position"), &BehaviorGraphView::get_node_at);
    ClassDB::bind_static_method("BehaviorGraphView", D_METHOD("resolve_runner", "node"), &BehaviorGraphView::resolve_runner);

    BIND_ENUM_CONSTANT(LAYOUT_TECH_TREE);
    BIND_ENUM_CONSTANT(LAYOUT_TOP_DOWN);
    BIND_ENUM_CONSTANT(LAYOUT_MIND_MAP);

    ADD_GROUP(U"🎯 Source", "");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "StateMachine,State,BehaviorTree,BTNode"), "set_target_path", "get_target_path");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "live_update"), "set_live_update", "is_live_update");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refresh_interval", PROPERTY_HINT_RANGE, "0,2,0.01,suffix:s"), "set_refresh_interval", "get_refresh_interval");
    ADD_GROUP(U"🎨 Affichage", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "layout_mode", PROPERTY_HINT_ENUM, "Tech Tree,Top Down,Mind Map"), "set_layout_mode", "get_layout_mode");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "editable"), "set_editable", "is_editable");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_transition_labels"), "set_show_transition_labels", "get_show_transition_labels");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_history"), "set_show_history", "get_show_history");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "placeholder_text", PROPERTY_HINT_MULTILINE_TEXT), "set_placeholder_text", "get_placeholder_text");

    ADD_SIGNAL(MethodInfo("node_selected", PropertyInfo(Variant::STRING, "id")));
    ADD_SIGNAL(MethodInfo("node_activated", PropertyInfo(Variant::STRING, "id")));
    ADD_SIGNAL(MethodInfo("node_context_menu_requested", PropertyInfo(Variant::STRING, "id"), PropertyInfo(Variant::VECTOR2, "screen_position")));
    ADD_SIGNAL(MethodInfo("node_dropped", PropertyInfo(Variant::STRING, "id"), PropertyInfo(Variant::STRING, "target_id")));
    ADD_SIGNAL(MethodInfo("link_requested", PropertyInfo(Variant::STRING, "from_id"), PropertyInfo(Variant::STRING, "to_id")));
    ADD_SIGNAL(MethodInfo("delete_requested", PropertyInfo(Variant::STRING, "id")));
}
