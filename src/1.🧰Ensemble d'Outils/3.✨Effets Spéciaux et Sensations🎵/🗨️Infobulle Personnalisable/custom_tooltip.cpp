#include "custom_tooltip.hpp"

#include <godot_cpp/classes/callback_tweener.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/video_stream_player.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

// 🎬 Cycle de vie

void CustomTooltip::_ready()
{
    _connect_target();
}

void CustomTooltip::_notification(int p_what)
{
    if (p_what == NOTIFICATION_EXIT_TREE)
    {
        _disconnect_target();
        hovering = false;
        if (shown)
        {
            _finish_hide();
        }
    }
}

void CustomTooltip::_process(double p_delta)
{
    if (connected_target_id.is_valid() && !ObjectDB::get_instance(connected_target_id))
    {
        // Cible libérée : mouse_exited ne viendra plus
        connected_target_id = ObjectID();
        hovering = false;
        hide_tooltip();
    }

    if (hovering && !shown)
    {
        hover_time += p_delta;
        if (hover_time >= delay)
        {
            show_tooltip();
        }
    }

    if (shown)
    {
        if (content_dirty)
        {
            _rebuild_content();
        }
        if (follow_mouse)
        {
            _place_at_mouse();
        }
    }
}

// 🎯 API publique

void CustomTooltip::show_tooltip()
{
    if (content.is_null() || !is_inside_tree())
    {
        return;
    }

    _build_ui();
    if (content_dirty)
    {
        _rebuild_content();
    }

    const bool was_shown = shown;
    shown = true;
    panel->show();
    _place_at_mouse();
    if (video_player && !video_player->is_playing())
    {
        video_player->play();
    }

    if (fade_tween.is_valid())
    {
        fade_tween->kill();
    }
    if (fade_duration > 0.0)
    {
        fade_tween = panel->create_tween();
        fade_tween->tween_property(panel, "modulate:a", 1.0, fade_duration * (1.0 - panel->get_modulate().a));
    }
    else
    {
        panel->set_modulate(Color(1, 1, 1, 1));
    }

    if (!was_shown)
    {
        emit_signal("tooltip_shown");
    }
}

void CustomTooltip::hide_tooltip()
{
    hover_time = 0.0;
    if (!shown || !panel)
    {
        return;
    }

    if (fade_tween.is_valid())
    {
        fade_tween->kill();
        fade_tween.unref();
    }
    if (fade_duration > 0.0 && panel->is_inside_tree())
    {
        fade_tween = panel->create_tween();
        fade_tween->tween_property(panel, "modulate:a", 0.0, fade_duration * panel->get_modulate().a);
        fade_tween->tween_callback(callable_mp(this, &CustomTooltip::_finish_hide));
        shown = false; // un nouveau survol pendant le fondu relance l'affichage
        emit_signal("tooltip_hidden");
        return;
    }

    shown = false;
    _finish_hide();
    emit_signal("tooltip_hidden");
}

void CustomTooltip::_finish_hide()
{
    if (shown || !panel)
    {
        return; // réaffichée pendant le fondu
    }
    panel->hide();
    panel->set_modulate(Color(1, 1, 1, 0));
    if (video_player)
    {
        video_player->stop();
    }
}

// 🎛️ Réglages

void CustomTooltip::set_content(const Ref<TooltipContent> &p_content)
{
    const Callable on_changed = callable_mp(this, &CustomTooltip::_on_content_changed);
    if (content.is_valid() && content->is_connected("changed", on_changed))
    {
        content->disconnect("changed", on_changed);
    }
    content = p_content;
    if (content.is_valid())
    {
        content->connect("changed", on_changed);
    }
    content_dirty = true;
    if (shown && content.is_null())
    {
        hide_tooltip();
    }
}

void CustomTooltip::set_target(Node *p_target)
{
    _disconnect_target();
    target_id = p_target ? ObjectID(p_target->get_instance_id()) : ObjectID();
    if (is_node_ready())
    {
        _connect_target();
    }
}

Node *CustomTooltip::get_target() const { return Object::cast_to<Node>(ObjectDB::get_instance(target_id)); }

void CustomTooltip::set_panel_style(const Ref<StyleBox> &p_value)
{
    panel_style = p_value;
    if (panel)
    {
        if (panel_style.is_valid())
        {
            panel->add_theme_stylebox_override("panel", panel_style);
        }
        else
        {
            panel->remove_theme_stylebox_override("panel");
        }
    }
}

void CustomTooltip::set_tooltip_theme(const Ref<Theme> &p_value)
{
    tooltip_theme = p_value;
    if (panel)
    {
        panel->set_theme(tooltip_theme);
        content_dirty = true;
    }
}

void CustomTooltip::set_layer(int p_value)
{
    layer = p_value;
    if (canvas_layer)
    {
        canvas_layer->set_layer(layer);
    }
}

// 🖱️ Survol de la cible

Node *CustomTooltip::_resolve_target() const
{
    Node *target = get_target();
    return target ? target : get_parent();
}

void CustomTooltip::_connect_target()
{
    Node *target = _resolve_target();
    if (!target)
    {
        return;
    }
    if (!target->has_signal("mouse_entered") || !target->has_signal("mouse_exited"))
    {
        WARN_PRINT(String(U"[CustomTooltip] La cible « ") + target->get_name() + String(U" » n'émet pas mouse_entered / mouse_exited (Control ou CollisionObject attendu)."));
        return;
    }
    target->connect("mouse_entered", callable_mp(this, &CustomTooltip::_on_mouse_entered));
    target->connect("mouse_exited", callable_mp(this, &CustomTooltip::_on_mouse_exited));
    connected_target_id = target->get_instance_id();
}

void CustomTooltip::_disconnect_target()
{
    Node *target = Object::cast_to<Node>(ObjectDB::get_instance(connected_target_id));
    connected_target_id = ObjectID();
    if (!target)
    {
        return;
    }
    const Callable entered = callable_mp(this, &CustomTooltip::_on_mouse_entered);
    const Callable exited = callable_mp(this, &CustomTooltip::_on_mouse_exited);
    if (target->is_connected("mouse_entered", entered))
    {
        target->disconnect("mouse_entered", entered);
    }
    if (target->is_connected("mouse_exited", exited))
    {
        target->disconnect("mouse_exited", exited);
    }
}

void CustomTooltip::_on_mouse_entered()
{
    hovering = true;
    hover_time = 0.0;
}

void CustomTooltip::_on_mouse_exited()
{
    hovering = false;
    hide_tooltip();
}

// 🧱 Construction de l'interface

void CustomTooltip::_build_ui()
{
    if (canvas_layer)
    {
        return;
    }

    canvas_layer = memnew(CanvasLayer);
    canvas_layer->set_name("TooltipLayer");
    canvas_layer->set_layer(layer);
    add_child(canvas_layer, false, Node::INTERNAL_MODE_BACK);

    panel = memnew(PanelContainer);
    panel->set_name("TooltipPanel");
    panel->set_theme(tooltip_theme);
    if (panel_style.is_valid())
    {
        panel->add_theme_stylebox_override("panel", panel_style);
    }
    panel->set_modulate(Color(1, 1, 1, 0));
    panel->hide();
    canvas_layer->add_child(panel);

    box = memnew(VBoxContainer);
    panel->add_child(box);
    _ignore_mouse(panel);
}

void CustomTooltip::_rebuild_content()
{
    content_dirty = false;
    if (!box)
    {
        return;
    }

    for (int64_t i = box->get_child_count() - 1; i >= 0; i--)
    {
        Node *child = box->get_child(i);
        box->remove_child(child);
        child->queue_free();
    }
    video_player = nullptr;
    if (content.is_null())
    {
        return;
    }

    if (!content->get_title().is_empty())
    {
        Label *title = memnew(Label);
        title->set_name("Title");
        title->set_theme_type_variation("HeaderSmall");
        title->set_text(content->get_title());
        box->add_child(title);
    }

    Ref<Texture2D> icon = content->get_icon();
    if (icon.is_valid())
    {
        // Icône réduite (proportions conservées) pour tenir dans icon_max_size
        Vector2 size = icon->get_size();
        const Vector2 max_size = content->get_icon_max_size();
        if (max_size.x > 0 && max_size.y > 0 && size.x > 0 && size.y > 0)
        {
            size *= Math::min(1.0f, Math::min(max_size.x / size.x, max_size.y / size.y));
        }
        TextureRect *icon_rect = memnew(TextureRect);
        icon_rect->set_name("Icon");
        icon_rect->set_texture(icon);
        icon_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
        icon_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        icon_rect->set_custom_minimum_size(size);
        box->add_child(icon_rect);
    }

    Ref<VideoStream> video = content->get_video();
    if (video.is_valid())
    {
        video_player = memnew(VideoStreamPlayer);
        video_player->set_name("Video");
        video_player->set_stream(video);
        video_player->set_loop(content->get_video_loop());
        video_player->set_volume(content->get_video_muted() ? 0.0f : 1.0f);
        const Vector2 video_size = content->get_video_size();
        if (video_size.x > 0 && video_size.y > 0)
        {
            video_player->set_expand(true);
            video_player->set_custom_minimum_size(video_size);
        }
        box->add_child(video_player);
        if (shown)
        {
            video_player->play();
        }
    }

    if (!content->get_text().is_empty())
    {
        RichTextLabel *text = memnew(RichTextLabel);
        text->set_name("Text");
        text->set_use_bbcode(true);
        text->set_fit_content(true);
        text->set_scroll_active(false);
        text->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
        text->set_text(content->get_text());
        box->add_child(text);
        // Texte court : largeur naturelle ; texte long : retour à la ligne à max_width
        const float max_width = content->get_max_width();
        if (text->get_content_width() > max_width)
        {
            text->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
            text->set_custom_minimum_size(Vector2(max_width, 0));
        }
    }

    Ref<PackedScene> scene = content->get_custom_scene();
    if (scene.is_valid())
    {
        if (Node *instance = scene->instantiate())
        {
            box->add_child(instance);
        }
    }

    // Survoler l'infobulle ne doit pas faire sortir la souris de la cible
    _ignore_mouse(box);
    panel->reset_size();
}

void CustomTooltip::_place_at_mouse()
{
    if (!panel || !panel->is_inside_tree())
    {
        return;
    }
    panel->reset_size(); // le contenu a pu changer de taille

    Viewport *viewport = panel->get_viewport();
    const Rect2 visible = viewport->get_visible_rect();
    const Vector2 mouse = viewport->get_mouse_position();
    const Vector2 size = panel->get_size();

    // Côté opposé si l'infobulle sortirait de l'écran, puis maintien dans l'écran
    Vector2 position = mouse + offset;
    if (position.x + size.x > visible.get_end().x)
    {
        position.x = mouse.x - offset.x - size.x;
    }
    if (position.y + size.y > visible.get_end().y)
    {
        position.y = mouse.y - offset.y - size.y;
    }
    position.x = Math::clamp(position.x, visible.position.x, Math::max(visible.position.x, visible.get_end().x - size.x));
    position.y = Math::clamp(position.y, visible.position.y, Math::max(visible.position.y, visible.get_end().y - size.y));
    panel->set_position(position);
}

void CustomTooltip::_ignore_mouse(Node *p_node)
{
    if (Control *control = Object::cast_to<Control>(p_node))
    {
        control->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    }
    for (int64_t i = 0; i < p_node->get_child_count(); i++)
    {
        _ignore_mouse(p_node->get_child(i));
    }
}

// 🔗 Liaisons

void CustomTooltip::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("show_tooltip"), &CustomTooltip::show_tooltip);
    ClassDB::bind_method(D_METHOD("hide_tooltip"), &CustomTooltip::hide_tooltip);
    ClassDB::bind_method(D_METHOD("is_tooltip_visible"), &CustomTooltip::is_tooltip_visible);
    ClassDB::bind_method(D_METHOD("get_panel"), &CustomTooltip::get_panel);
    ClassDB::bind_method(D_METHOD("set_content", "content"), &CustomTooltip::set_content);
    ClassDB::bind_method(D_METHOD("get_content"), &CustomTooltip::get_content);
    ClassDB::bind_method(D_METHOD("set_target", "target"), &CustomTooltip::set_target);
    ClassDB::bind_method(D_METHOD("get_target"), &CustomTooltip::get_target);
    ClassDB::bind_method(D_METHOD("set_delay", "value"), &CustomTooltip::set_delay);
    ClassDB::bind_method(D_METHOD("get_delay"), &CustomTooltip::get_delay);
    ClassDB::bind_method(D_METHOD("set_follow_mouse", "value"), &CustomTooltip::set_follow_mouse);
    ClassDB::bind_method(D_METHOD("get_follow_mouse"), &CustomTooltip::get_follow_mouse);
    ClassDB::bind_method(D_METHOD("set_offset", "value"), &CustomTooltip::set_offset);
    ClassDB::bind_method(D_METHOD("get_offset"), &CustomTooltip::get_offset);
    ClassDB::bind_method(D_METHOD("set_fade_duration", "value"), &CustomTooltip::set_fade_duration);
    ClassDB::bind_method(D_METHOD("get_fade_duration"), &CustomTooltip::get_fade_duration);
    ClassDB::bind_method(D_METHOD("set_panel_style", "value"), &CustomTooltip::set_panel_style);
    ClassDB::bind_method(D_METHOD("get_panel_style"), &CustomTooltip::get_panel_style);
    ClassDB::bind_method(D_METHOD("set_tooltip_theme", "value"), &CustomTooltip::set_tooltip_theme);
    ClassDB::bind_method(D_METHOD("get_tooltip_theme"), &CustomTooltip::get_tooltip_theme);
    ClassDB::bind_method(D_METHOD("set_layer", "value"), &CustomTooltip::set_layer);
    ClassDB::bind_method(D_METHOD("get_layer"), &CustomTooltip::get_layer);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "content", PROPERTY_HINT_RESOURCE_TYPE, "TooltipContent"), "set_content", "get_content");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target", PROPERTY_HINT_NODE_TYPE, "Node"), "set_target", "get_target");

    ADD_GROUP("Behavior", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "delay", PROPERTY_HINT_RANGE, "0,3,0.01,or_greater,suffix:s"), "set_delay", "get_delay");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "follow_mouse"), "set_follow_mouse", "get_follow_mouse");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset", PROPERTY_HINT_NONE, "suffix:px"), "set_offset", "get_offset");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fade_duration", PROPERTY_HINT_RANGE, "0,1,0.01,or_greater,suffix:s"), "set_fade_duration", "get_fade_duration");

    ADD_GROUP("Appearance", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "panel_style", PROPERTY_HINT_RESOURCE_TYPE, "StyleBox"), "set_panel_style", "get_panel_style");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "tooltip_theme", PROPERTY_HINT_RESOURCE_TYPE, "Theme"), "set_tooltip_theme", "get_tooltip_theme");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "layer", PROPERTY_HINT_RANGE, "-128,128,1"), "set_layer", "get_layer");

    ADD_SIGNAL(MethodInfo("tooltip_shown"));
    ADD_SIGNAL(MethodInfo("tooltip_hidden"));
}
