#include "rich_tooltip.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/widget_behavior.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

using namespace godot;

void RichTooltip::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_delay", "delay"), &RichTooltip::set_delay);
    ClassDB::bind_method(D_METHOD("get_delay"), &RichTooltip::get_delay);
    ClassDB::bind_method(D_METHOD("build_text", "widget"), &RichTooltip::build_text);
    ClassDB::bind_method(D_METHOD("show_for", "widget"), &RichTooltip::show_for);
    ClassDB::bind_method(D_METHOD("hide_tooltip"), &RichTooltip::hide_tooltip);
    ClassDB::bind_method(D_METHOD("is_shown"), &RichTooltip::is_shown);
    ClassDB::bind_method(D_METHOD("get_shown_text"), &RichTooltip::get_shown_text);
}

RichTooltip::RichTooltip()
{
    set_process_mode(PROCESS_MODE_ALWAYS);
    popup = memnew(PopupPanel);
    popup->set_flag(Window::FLAG_NO_FOCUS, true);
    popup->set_flag(Window::FLAG_MOUSE_PASSTHROUGH, true);
    popup->set_visible(false);
    label = memnew(RichTextLabel);
    label->set_use_bbcode(true);
    label->set_fit_content(true);
    label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
    label->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    popup->add_child(label);
    add_child(popup, false, INTERNAL_MODE_BACK);
}

void RichTooltip::setup(Control *p_root)
{
    root_id = p_root ? ObjectID(p_root->get_instance_id()) : ObjectID();
    set_process(p_root != nullptr);
}

void RichTooltip::set_context(Object *p_context) { context_id = p_context ? ObjectID(p_context->get_instance_id()) : ObjectID(); }
void RichTooltip::set_delay(double p_delay) { delay = MAX(p_delay, 0.0); }
double RichTooltip::get_delay() const { return delay; }

PropertyWidget *RichTooltip::_widget_under_mouse() const
{
    Control *root = Object::cast_to<Control>(ObjectDB::get_instance(root_id));
    Viewport *viewport = get_viewport();
    Control *hovered = viewport ? viewport->gui_get_hovered_control() : nullptr;
    if (!root || !hovered || !root->is_ancestor_of(hovered))
    {
        return nullptr;
    }
    // Widget le plus proche qui a des infobulles (un sous-widget d'une collection peut avoir les siennes).
    for (Node *node = hovered; node && node != root; node = node->get_parent())
    {
        PropertyWidget *widget = Object::cast_to<PropertyWidget>(node);
        if (widget && !widget->get_tooltip_texts().is_empty())
        {
            return widget;
        }
    }
    return nullptr;
}

void RichTooltip::_process(double p_delta)
{
    PropertyWidget *widget = _widget_under_mouse();
    uint64_t id = widget ? (uint64_t)widget->get_instance_id() : 0;
    if (id != (uint64_t)hovered_id)
    {
        hovered_id = ObjectID(id);
        hover_time = 0.0;
        hide_tooltip();
        return;
    }
    if (!widget || popup->is_visible())
    {
        return;
    }
    hover_time += p_delta;
    if (hover_time >= delay)
    {
        show_for(widget);
    }
}

String RichTooltip::build_text(PropertyWidget *p_widget) const
{
    if (!p_widget)
    {
        return String();
    }
    Object *context = ObjectDB::get_instance(context_id);
    PackedStringArray parts;
    PackedStringArray texts = p_widget->get_tooltip_texts();
    for (const String &text : texts)
    {
        parts.push_back(WidgetBehavior::format_text(text, context));
    }
    return String("\n\n").join(parts);
}

void RichTooltip::show_for(PropertyWidget *p_widget)
{
    String text = build_text(p_widget);
    if (text.is_empty())
    {
        hide_tooltip();
        return;
    }
    label->set_text(text);
    // Position : souris + décalage, dans le repère de la fenêtre qui affichera la bulle (intégrée au viewport,
    // ou fenêtre native positionnée en coordonnées écran).
    Viewport *viewport = get_viewport();
    Vector2 mouse = viewport ? viewport->get_mouse_position() : Vector2();
    Vector2i position = Vector2i(mouse) + Vector2i(14, 18);
    if (viewport && !viewport->is_embedding_subwindows())
    {
        if (Window *window = Object::cast_to<Window>(viewport))
        {
            position += window->get_position();
        }
    }
    popup->reset_size();
    popup->popup(Rect2i(position, Vector2i()));
}

void RichTooltip::hide_tooltip()
{
    if (popup->is_visible())
    {
        popup->hide();
    }
}

bool RichTooltip::is_shown() const { return popup->is_visible(); }
String RichTooltip::get_shown_text() const { return popup->is_visible() ? label->get_text() : String(); }
