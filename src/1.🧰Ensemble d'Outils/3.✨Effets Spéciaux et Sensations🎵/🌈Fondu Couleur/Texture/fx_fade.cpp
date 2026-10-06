#include "fx_fade.hpp"

#include <godot_cpp/classes/callback_tweener.hpp>
#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

namespace
{
// Mélange la texture affichée (TEXTURE) et la suivante ; la couleur du sommet porte modulate / self_modulate.
const char *CROSSFADE_SHADER = R"(
shader_type canvas_item;

uniform sampler2D next_texture : source_color, filter_linear;
uniform float progress : hint_range(0.0, 1.0) = 0.0;

varying vec4 vertex_color;

void vertex() {
	vertex_color = COLOR;
}

void fragment() {
	COLOR = mix(texture(TEXTURE, UV), texture(next_texture, UV), progress) * vertex_color;
}
)";
} // namespace

// 🎬 Cycle de vie

void FXFade::_play(Node *p_target)
{
    if (!p_target)
    {
        return;
    }

    if (mode == FADE_COLOR)
    {
        const Variant current = p_target->get_indexed(NodePath(color_property));
        if (current.get_type() != Variant::COLOR)
        {
            WARN_PRINT(String(U"[FXFade] « ") + p_target->get_name() + String(U" » n'a pas de propriété couleur « ") + color_property + U" ».");
            return;
        }
        if (!_is_restarting())
        {
            original_color = current; // relancé : la couleur actuelle est déjà modifiée
        }
        _fade_color_to(p_target, color);
    }
    else
    {
        const Variant current = p_target->get_indexed(NodePath(texture_property));
        if (current.get_type() != Variant::OBJECT && current.get_type() != Variant::NIL)
        {
            WARN_PRINT(String(U"[FXFade] « ") + p_target->get_name() + String(U" » n'a pas de propriété texture « ") + texture_property + U" ».");
            return;
        }
        _finish_crossfade(); // fondu précédent interrompu : sa texture finale est posée
        if (!_is_restarting())
        {
            original_texture = p_target->get_indexed(NodePath(texture_property));
        }
        _fade_texture_to(p_target, texture);
    }
}

void FXFade::_stop(Node *p_target)
{
    if (!p_target || !revert_on_stop)
    {
        return;
    }
    if (mode == FADE_COLOR)
    {
        if (p_target->get_indexed(NodePath(color_property)).get_type() == Variant::COLOR)
        {
            _fade_color_to(p_target, original_color);
        }
    }
    else
    {
        _fade_texture_to(p_target, original_texture);
    }
}

// 🎨 Couleur

void FXFade::_fade_color_to(Node *p_target, const Color &p_color)
{
    Ref<Tween> fade = _new_tween(p_target);
    if (fade.is_null())
    {
        p_target->set_indexed(NodePath(color_property), p_color);
        return;
    }
    fade->tween_property(p_target, NodePath(color_property), p_color, duration)->set_trans(transition)->set_ease(ease_type);
}

// 🖼️ Texture

void FXFade::_fade_texture_to(Node *p_target, const Ref<Texture2D> &p_texture)
{
    _finish_crossfade();
    const NodePath property(texture_property);
    const Ref<Texture2D> current = p_target->get_indexed(property);
    Ref<Tween> fade = _new_tween(p_target);

    // Rien à mélanger : changement immédiat
    if (fade.is_null() || current == p_texture || current.is_null() || p_texture.is_null())
    {
        if (fade.is_valid())
        {
            fade->kill();
        }
        p_target->set_indexed(property, p_texture);
        return;
    }

    if (CanvasItem *canvas_item = Object::cast_to<CanvasItem>(p_target))
    {
        if (crossfade_material.is_null())
        {
            Ref<Shader> shader;
            shader.instantiate();
            shader->set_code(CROSSFADE_SHADER);
            crossfade_material.instantiate();
            crossfade_material->set_shader(shader);
        }
        material_before_crossfade = canvas_item->get_material();
        pending_texture = p_texture;
        crossfade_target_id = p_target->get_instance_id();

        crossfade_material->set_shader_parameter("next_texture", p_texture);
        crossfade_material->set_shader_parameter("progress", 0.0);
        canvas_item->set_material(crossfade_material);

        fade->tween_property(crossfade_material.ptr(), "shader_parameter/progress", 1.0, duration)->set_trans(transition)->set_ease(ease_type);
        fade->tween_callback(callable_mp(this, &FXFade::_finish_crossfade));
        return;
    }

    // Autre nœud : s'efface, change de texture, réapparaît
    const Variant modulate = p_target->get("modulate");
    if (modulate.get_type() == Variant::COLOR)
    {
        const float alpha = ((Color)modulate).a;
        fade->tween_property(p_target, "modulate:a", 0.0, duration * 0.5f)->set_trans(transition)->set_ease(ease_type);
        fade->tween_callback(Callable(p_target, "set_indexed").bind(property, p_texture));
        fade->tween_property(p_target, "modulate:a", alpha, duration * 0.5f)->set_trans(transition)->set_ease(ease_type);
        return;
    }

    fade->kill();
    p_target->set_indexed(property, p_texture);
}

void FXFade::_finish_crossfade()
{
    Node *target = Object::cast_to<Node>(ObjectDB::get_instance(crossfade_target_id));
    crossfade_target_id = ObjectID();
    if (target && pending_texture.is_valid())
    {
        target->set_indexed(NodePath(texture_property), pending_texture);
        if (CanvasItem *canvas_item = Object::cast_to<CanvasItem>(target))
        {
            canvas_item->set_material(material_before_crossfade);
        }
    }
    pending_texture.unref();
    material_before_crossfade.unref();
}

Ref<Tween> FXFade::_new_tween(Node *p_target)
{
    // L'animation précédente ne doit plus agir sur la cible
    if (tween.is_valid())
    {
        tween->kill();
        tween.unref();
    }
    if (duration <= 0.0f || !p_target->is_inside_tree())
    {
        return Ref<Tween>();
    }
    tween = p_target->create_tween();
    return tween;
}

// 🔗 Liaisons

void FXFade::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_mode", "value"), &FXFade::set_mode);
    ClassDB::bind_method(D_METHOD("get_mode"), &FXFade::get_mode);
    ClassDB::bind_method(D_METHOD("set_color", "value"), &FXFade::set_color);
    ClassDB::bind_method(D_METHOD("get_color"), &FXFade::get_color);
    ClassDB::bind_method(D_METHOD("set_color_property", "value"), &FXFade::set_color_property);
    ClassDB::bind_method(D_METHOD("get_color_property"), &FXFade::get_color_property);
    ClassDB::bind_method(D_METHOD("set_texture", "value"), &FXFade::set_texture);
    ClassDB::bind_method(D_METHOD("get_texture"), &FXFade::get_texture);
    ClassDB::bind_method(D_METHOD("set_texture_property", "value"), &FXFade::set_texture_property);
    ClassDB::bind_method(D_METHOD("get_texture_property"), &FXFade::get_texture_property);
    ClassDB::bind_method(D_METHOD("set_duration", "value"), &FXFade::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &FXFade::get_duration);
    ClassDB::bind_method(D_METHOD("set_transition", "value"), &FXFade::set_transition);
    ClassDB::bind_method(D_METHOD("get_transition"), &FXFade::get_transition);
    ClassDB::bind_method(D_METHOD("set_ease_type", "value"), &FXFade::set_ease_type);
    ClassDB::bind_method(D_METHOD("get_ease_type"), &FXFade::get_ease_type);
    ClassDB::bind_method(D_METHOD("set_revert_on_stop", "value"), &FXFade::set_revert_on_stop);
    ClassDB::bind_method(D_METHOD("get_revert_on_stop"), &FXFade::get_revert_on_stop);

    BIND_ENUM_CONSTANT(FADE_COLOR);
    BIND_ENUM_CONSTANT(FADE_TEXTURE);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Color,Texture"), "set_mode", "get_mode");

    ADD_GROUP("Color", "");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "color_property", PROPERTY_HINT_PLACEHOLDER_TEXT, "modulate"), "set_color_property", "get_color_property");

    ADD_GROUP("Texture", "");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "texture_property", PROPERTY_HINT_PLACEHOLDER_TEXT, "texture"), "set_texture_property", "get_texture_property");

    ADD_GROUP("Animation", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0,5,0.01,or_greater,suffix:s"), "set_duration", "get_duration");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "transition", PROPERTY_HINT_ENUM, "Linear,Sine,Quint,Quart,Quad,Expo,Elastic,Cubic,Circ,Bounce,Back,Spring"), "set_transition", "get_transition");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ease_type", PROPERTY_HINT_ENUM, "In,Out,In Out,Out In"), "set_ease_type", "get_ease_type");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "revert_on_stop"), "set_revert_on_stop", "get_revert_on_stop");
}
