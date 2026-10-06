#include "pop_spawner.hpp"

#include "pop_instance.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/label_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/sprite3d.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

namespace
{
const char *SCREEN_LAYER_NAME = "PopScreenLayer";

/// Scène courante (parent par défaut des pops), sinon la racine.
Node *default_container(Node *p_anchor)
{
    SceneTree *tree = p_anchor->get_tree();
    Node *scene = tree->get_current_scene();
    return scene ? scene : (Node *)tree->get_root();
}

/// CanvasLayer des pops à l'écran (pop 2D d'une cible 3D), créé sous p_parent au premier besoin.
Node *screen_layer(Node *p_parent)
{
    if (Node *layer = p_parent->get_node_or_null(NodePath(SCREEN_LAYER_NAME)))
    {
        return layer;
    }
    CanvasLayer *layer = memnew(CanvasLayer);
    layer->set_name(SCREEN_LAYER_NAME);
    layer->set_layer(100);
    p_parent->add_child(layer);
    return layer;
}

Ref<Font> font_of(const PopStyle &p_style)
{
    return p_style.get_font().is_valid() ? p_style.get_font() : ThemeDB::get_singleton()->get_fallback_font();
}

Color text_color_of(const PopStyle &p_style, const PopRequest &p_request)
{
    return p_request.color.a > 0.0f ? p_request.color : p_style.get_text_color();
}

/// Largeurs de l'image et du texte, et position de départ (gauche) de la rangée centrée image + texte.
struct RowLayout
{
    float icon_width = 0.0f;
    float gap = 0.0f;
    float left = 0.0f;
};

RowLayout row_layout(const PopStyle &p_style, bool p_has_icon, float p_text_width)
{
    RowLayout row;
    row.icon_width = p_has_icon ? p_style.get_icon_size().x : 0.0f;
    row.gap = p_has_icon && p_text_width > 0.0f ? p_style.get_spacing() : 0.0f;
    row.left = -(row.icon_width + row.gap + p_text_width) / 2.0f;
    return row;
}
} // namespace

// 🎯 API publique

Node *PopSpawner::pop(const String &p_text, const Ref<Texture2D> &p_texture, const Ref<PackedScene> &p_scene, const Color &p_color, const Variant &p_at)
{
    ERR_FAIL_COND_V_MSG(!is_inside_tree(), nullptr, String(U"[PopSpawner] Le PopSpawner doit être dans l'arbre de scène."));
    Node *anchor = get_target();
    if (!anchor)
    {
        anchor = get_parent();
    }

    PopRequest request;
    request.text = p_text;
    request.texture = p_texture;
    request.scene = p_scene;
    request.color = p_color;
    request.at = p_at;
    request.offset = offset;
    request.container = get_container();
    request.unit_scale_3d = unit_scale_3d;
    request.screen_space_3d = screen_space_3d;

    Node *created = create_pop(anchor, style, request);
    if (!created)
    {
        return nullptr;
    }

    // Limite : les plus anciens disparaissent
    _prune();
    active_pops.push_back(ObjectID(created->get_instance_id()));
    while (max_pops > 0 && active_pops.size() > (uint32_t)max_pops)
    {
        if (Node *oldest = Object::cast_to<Node>(ObjectDB::get_instance(active_pops[0])))
        {
            oldest->queue_free();
        }
        active_pops.remove_at(0);
    }

    emit_signal("popped", created);
    return created;
}

Node *PopSpawner::pop_text(const String &p_text, const Color &p_color) { return pop(p_text, Ref<Texture2D>(), Ref<PackedScene>(), p_color); }

Node *PopSpawner::pop_texture(const Ref<Texture2D> &p_texture) { return pop(String(), p_texture); }

Node *PopSpawner::pop_scene(const Ref<PackedScene> &p_scene) { return pop(String(), Ref<Texture2D>(), p_scene); }

Node *PopSpawner::pop_number(double p_value, int p_decimals)
{
    Ref<PopStyle> number_style = style;
    if (number_style.is_null())
    {
        number_style.instantiate();
    }
    const String number = p_decimals > 0 ? String::num(p_value, p_decimals) : String::num_int64((int64_t)Math::round(p_value));
    const String text = p_value > 0.0 ? "+" + number : number;
    return pop_text(text, p_value < 0.0 ? number_style->get_negative_color() : number_style->get_positive_color());
}

void PopSpawner::clear()
{
    for (const ObjectID &id : active_pops)
    {
        if (Node *node = Object::cast_to<Node>(ObjectDB::get_instance(id)))
        {
            node->queue_free();
        }
    }
    active_pops.clear();
}

int PopSpawner::get_active_count()
{
    _prune();
    return (int)active_pops.size();
}

Array PopSpawner::get_active_pops()
{
    _prune();
    Array pops;
    for (const ObjectID &id : active_pops)
    {
        pops.push_back(ObjectDB::get_instance(id));
    }
    return pops;
}

void PopSpawner::_prune()
{
    // Pops arrivés en fin de vie (ou en cours de libération)
    for (int64_t i = (int64_t)active_pops.size() - 1; i >= 0; i--)
    {
        Node *node = Object::cast_to<Node>(ObjectDB::get_instance(active_pops[i]));
        if (!node || node->is_queued_for_deletion())
        {
            active_pops.remove_at(i);
        }
    }
}

// 🏭 Création

Node *PopSpawner::create_pop(Node *p_anchor, const Ref<PopStyle> &p_style, const PopRequest &p_request)
{
    ERR_FAIL_NULL_V_MSG(p_anchor, nullptr, String(U"[PopSpawner] Aucune cible pour le pop."));
    ERR_FAIL_COND_V_MSG(!p_anchor->is_inside_tree(), nullptr, String(U"[PopSpawner] La cible du pop doit être dans l'arbre de scène."));
    if (p_request.text.is_empty() && p_request.texture.is_null() && p_request.scene.is_null())
    {
        WARN_PRINT(String(U"[PopSpawner] Pop vide : ni texte, ni image, ni scène."));
        return nullptr;
    }

    Ref<PopStyle> style = p_style;
    if (style.is_null())
    {
        style.instantiate();
    }
    const Vector3 offset = p_request.offset;
    Node3D *anchor_3d = Object::cast_to<Node3D>(p_anchor);

    // 🧊 Pop 3D
    if (anchor_3d && !p_request.screen_space_3d)
    {
        const Vector3 origin = p_request.at.get_type() == Variant::VECTOR3 ? (Vector3)p_request.at : anchor_3d->get_global_position() + offset;
        PopInstance3D *pop = memnew(PopInstance3D);
        pop->set_name("Pop3D");
        _build_content_3d(pop, **style, p_request);
        (p_request.container ? p_request.container : default_container(p_anchor))->add_child(pop, true);
        pop->setup(style, origin, PopMotion::launch(**style, true), p_request.unit_scale_3d);
        return pop;
    }

    // 🟦 Pop 2D : position et conteneur selon la cible
    Vector2 origin;
    Node *container = p_request.container;
    if (anchor_3d)
    {
        // Cible 3D affichée à l'écran : position projetée par la caméra
        Camera3D *camera = anchor_3d->get_viewport()->get_camera_3d();
        const Vector3 position = p_request.at.get_type() == Variant::VECTOR3 ? (Vector3)p_request.at : anchor_3d->get_global_position() + offset;
        if (!camera || camera->is_position_behind(position))
        {
            return nullptr; // hors de la vue
        }
        origin = camera->unproject_position(position);
        if (!container)
        {
            container = screen_layer(default_container(p_anchor));
        }
    }
    else
    {
        if (Control *control = Object::cast_to<Control>(p_anchor))
        {
            origin = control->get_global_rect().get_center();
        }
        else if (Node2D *node_2d = Object::cast_to<Node2D>(p_anchor))
        {
            origin = node_2d->get_global_position();
        }
        origin += Vector2(offset.x, offset.y);
        if (p_request.at.get_type() == Variant::VECTOR2)
        {
            origin = p_request.at;
        }
        if (!container)
        {
            // Même espace que la cible : son CanvasLayer (interface), sinon la scène (monde)
            CanvasItem *canvas_item = Object::cast_to<CanvasItem>(p_anchor);
            CanvasLayer *layer = canvas_item ? canvas_item->get_canvas_layer_node() : nullptr;
            container = layer ? (Node *)layer : default_container(p_anchor);
        }
    }

    PopInstance2D *pop = memnew(PopInstance2D);
    pop->set_name("Pop2D");
    _build_content_2d(pop, **style, p_request);
    container->add_child(pop, true);
    pop->setup(style, origin, PopMotion::launch(**style, false));
    return pop;
}

void PopSpawner::_build_content_2d(Node *p_pop, const PopStyle &p_style, const PopRequest &p_request)
{
    // Scène derrière le reste, centrée
    if (p_request.scene.is_valid())
    {
        if (Node *instance = p_request.scene->instantiate())
        {
            p_pop->add_child(instance);
        }
    }

    Label *label = nullptr;
    Vector2 text_size;
    if (!p_request.text.is_empty())
    {
        Ref<LabelSettings> settings;
        settings.instantiate();
        settings->set_font(font_of(p_style));
        settings->set_font_size(p_style.get_font_size());
        settings->set_font_color(text_color_of(p_style, p_request));
        settings->set_outline_size(p_style.get_outline_size());
        settings->set_outline_color(p_style.get_outline_color());

        label = memnew(Label);
        label->set_name("Text");
        label->set_label_settings(settings);
        label->set_text(p_request.text);
        label->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        text_size = label->get_combined_minimum_size();
        label->set_size(text_size);
    }

    // Rangée centrée : image puis texte
    const RowLayout row = row_layout(p_style, p_request.texture.is_valid(), text_size.x);
    if (p_request.texture.is_valid())
    {
        const Vector2 texture_size = p_request.texture->get_size();
        Sprite2D *icon = memnew(Sprite2D);
        icon->set_name("Icon");
        icon->set_texture(p_request.texture);
        if (texture_size.x > 0 && texture_size.y > 0)
        {
            icon->set_scale(p_style.get_icon_size() / texture_size);
        }
        icon->set_position(Vector2(row.left + row.icon_width / 2.0f, 0.0f));
        p_pop->add_child(icon);
    }
    if (label)
    {
        label->set_position(Vector2(row.left + row.icon_width + row.gap, -text_size.y / 2.0f));
        p_pop->add_child(label);
    }
}

void PopSpawner::_build_content_3d(Node *p_pop, const PopStyle &p_style, const PopRequest &p_request)
{
    const float unit = p_request.unit_scale_3d;
    if (p_request.scene.is_valid())
    {
        if (Node *instance = p_request.scene->instantiate())
        {
            p_pop->add_child(instance);
        }
    }

    const Ref<Font> font = font_of(p_style);
    const float text_width = p_request.text.is_empty() ? 0.0f : font->get_string_size(p_request.text, HORIZONTAL_ALIGNMENT_LEFT, -1, p_style.get_font_size()).x;
    const RowLayout row = row_layout(p_style, p_request.texture.is_valid(), text_width);

    if (p_request.texture.is_valid())
    {
        const Vector2 texture_size = p_request.texture->get_size();
        Sprite3D *icon = memnew(Sprite3D);
        icon->set_name("Icon");
        icon->set_texture(p_request.texture);
        icon->set_pixel_size(texture_size.y > 0 ? unit * p_style.get_icon_size().y / texture_size.y : unit);
        icon->set_draw_flag(SpriteBase3D::FLAG_DISABLE_DEPTH_TEST, p_style.get_always_on_top_3d());
        icon->set_position(Vector3((row.left + row.icon_width / 2.0f) * unit, 0, 0));
        p_pop->add_child(icon);
    }
    if (!p_request.text.is_empty())
    {
        Label3D *label = memnew(Label3D);
        label->set_name("Text");
        label->set_text(p_request.text);
        label->set_font(font);
        label->set_font_size(p_style.get_font_size());
        label->set_modulate(text_color_of(p_style, p_request));
        label->set_outline_size(p_style.get_outline_size());
        label->set_outline_modulate(p_style.get_outline_color());
        label->set_pixel_size(unit);
        label->set_draw_flag(Label3D::FLAG_DISABLE_DEPTH_TEST, p_style.get_always_on_top_3d());
        label->set_render_priority(1); // devant l'image
        label->set_outline_render_priority(0);
        label->set_position(Vector3((row.left + row.icon_width + row.gap + text_width / 2.0f) * unit, 0, 0));
        p_pop->add_child(label);
    }
}

// 🎛️ Réglages

void PopSpawner::set_target(Node *p_target) { target_id = p_target ? ObjectID(p_target->get_instance_id()) : ObjectID(); }

Node *PopSpawner::get_target() const { return Object::cast_to<Node>(ObjectDB::get_instance(target_id)); }

void PopSpawner::set_container(Node *p_container) { container_id = p_container ? ObjectID(p_container->get_instance_id()) : ObjectID(); }

Node *PopSpawner::get_container() const { return Object::cast_to<Node>(ObjectDB::get_instance(container_id)); }

// 🔗 Liaisons

void PopSpawner::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("pop", "text", "texture", "scene", "color", "at"), &PopSpawner::pop,
            DEFVAL(String()), DEFVAL(Ref<Texture2D>()), DEFVAL(Ref<PackedScene>()), DEFVAL(Color(0, 0, 0, 0)), DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("pop_text", "text", "color"), &PopSpawner::pop_text, DEFVAL(Color(0, 0, 0, 0)));
    ClassDB::bind_method(D_METHOD("pop_texture", "texture"), &PopSpawner::pop_texture);
    ClassDB::bind_method(D_METHOD("pop_scene", "scene"), &PopSpawner::pop_scene);
    ClassDB::bind_method(D_METHOD("pop_number", "value", "decimals"), &PopSpawner::pop_number, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("clear"), &PopSpawner::clear);
    ClassDB::bind_method(D_METHOD("get_active_count"), &PopSpawner::get_active_count);
    ClassDB::bind_method(D_METHOD("get_active_pops"), &PopSpawner::get_active_pops);
    ClassDB::bind_method(D_METHOD("set_style", "value"), &PopSpawner::set_style);
    ClassDB::bind_method(D_METHOD("get_style"), &PopSpawner::get_style);
    ClassDB::bind_method(D_METHOD("set_target", "target"), &PopSpawner::set_target);
    ClassDB::bind_method(D_METHOD("get_target"), &PopSpawner::get_target);
    ClassDB::bind_method(D_METHOD("set_container", "container"), &PopSpawner::set_container);
    ClassDB::bind_method(D_METHOD("get_container"), &PopSpawner::get_container);
    ClassDB::bind_method(D_METHOD("set_offset", "value"), &PopSpawner::set_offset);
    ClassDB::bind_method(D_METHOD("get_offset"), &PopSpawner::get_offset);
    ClassDB::bind_method(D_METHOD("set_max_pops", "value"), &PopSpawner::set_max_pops);
    ClassDB::bind_method(D_METHOD("get_max_pops"), &PopSpawner::get_max_pops);
    ClassDB::bind_method(D_METHOD("set_unit_scale_3d", "value"), &PopSpawner::set_unit_scale_3d);
    ClassDB::bind_method(D_METHOD("get_unit_scale_3d"), &PopSpawner::get_unit_scale_3d);
    ClassDB::bind_method(D_METHOD("set_screen_space_3d", "value"), &PopSpawner::set_screen_space_3d);
    ClassDB::bind_method(D_METHOD("get_screen_space_3d"), &PopSpawner::get_screen_space_3d);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "style", PROPERTY_HINT_RESOURCE_TYPE, "PopStyle"), "set_style", "get_style");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target", PROPERTY_HINT_NODE_TYPE, "Node"), "set_target", "get_target");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "container", PROPERTY_HINT_NODE_TYPE, "Node"), "set_container", "get_container");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "offset"), "set_offset", "get_offset");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_pops", PROPERTY_HINT_RANGE, "0,512,1,or_greater"), "set_max_pops", "get_max_pops");

    ADD_GROUP("3D", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "unit_scale_3d", PROPERTY_HINT_RANGE, "0.0001,1,0.0001,or_greater,suffix:m/px"), "set_unit_scale_3d", "get_unit_scale_3d");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "screen_space_3d"), "set_screen_space_3d", "get_screen_space_3d");

    ADD_SIGNAL(MethodInfo("popped", PropertyInfo(Variant::OBJECT, "pop", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, "Node")));
}
