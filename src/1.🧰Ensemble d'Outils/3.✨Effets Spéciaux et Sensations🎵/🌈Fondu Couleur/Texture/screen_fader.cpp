#include "screen_fader.hpp"

#include <godot_cpp/classes/callback_tweener.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace
{
// Code du shader en ASCII uniquement (const char * : lu en Latin-1)
const char *FADER_SHADER = R"(
shader_type canvas_item;

uniform vec4 fade_color : source_color = vec4(0.0, 0.0, 0.0, 1.0);
uniform bool use_texture = false;
uniform sampler2D overlay_texture : source_color, filter_linear;
uniform bool use_mask = false;
uniform sampler2D mask_texture : filter_linear;
uniform float progress : hint_range(0.0, 1.0) = 0.0;
uniform float smoothness : hint_range(0.001, 1.0) = 0.1;

void fragment() {
	vec4 base = fade_color;
	if (use_texture) {
		base *= texture(overlay_texture, UV);
	}
	float alpha = progress;
	if (use_mask) {
		// Pixel de masque m : couvert quand progress le depasse (fondu sur smoothness)
		float m = texture(mask_texture, UV).r;
		alpha = smoothstep(m, m + smoothness, progress * (1.0 + smoothness));
	}
	COLOR = vec4(base.rgb, base.a * alpha);
}
)";
} // namespace

void ScreenFader::_notification(int p_what)
{
    if (p_what == NOTIFICATION_ENTER_TREE)
    {
        _build_overlay();
    }
}

// 🎯 API publique

void ScreenFader::fade_out(double p_duration) { fade_to(1.0, p_duration); }

void ScreenFader::fade_in(double p_duration) { fade_to(0.0, p_duration); }

void ScreenFader::fade_to(double p_progress, double p_duration)
{
    const double target = Math::clamp(p_progress, 0.0, 1.0);
    const double time = p_duration < 0.0 ? duration : p_duration;

    if (tween.is_valid())
    {
        tween->kill();
        tween.unref();
    }
    if (time <= 0.0 || !is_inside_tree())
    {
        set_progress(target);
        _on_fade_finished();
        return;
    }

    tween = create_tween();
    if (always_process)
    {
        tween->set_pause_mode(Tween::TWEEN_PAUSE_PROCESS);
        tween->set_ignore_time_scale(true);
    }
    tween->tween_property(this, "progress", target, time)->set_trans(transition)->set_ease(ease_type);
    tween->tween_callback(callable_mp(this, &ScreenFader::_on_fade_finished));
}

bool ScreenFader::is_fading() const { return tween.is_valid() && tween->is_running(); }

void ScreenFader::_on_fade_finished()
{
    tween.unref();
    emit_signal("fade_finished", progress);
    if (progress >= 1.0)
    {
        emit_signal("faded_out");
    }
    else if (progress <= 0.0)
    {
        emit_signal("faded_in");
    }
}

// 🧱 Voile

void ScreenFader::_build_overlay()
{
    if (overlay)
    {
        return;
    }

    Ref<Shader> shader;
    shader.instantiate();
    shader->set_code(FADER_SHADER);
    material.instantiate();
    material->set_shader(shader);

    overlay = memnew(ColorRect);
    overlay->set_name("FadeOverlay");
    overlay->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    overlay->set_material(material);
    add_child(overlay, false, Node::INTERNAL_MODE_BACK);
    _update_overlay();
}

void ScreenFader::_update_overlay()
{
    if (!overlay)
    {
        return;
    }
    material->set_shader_parameter("fade_color", color);
    material->set_shader_parameter("use_texture", texture.is_valid());
    material->set_shader_parameter("overlay_texture", texture);
    material->set_shader_parameter("use_mask", mask.is_valid());
    material->set_shader_parameter("mask_texture", mask);
    material->set_shader_parameter("smoothness", Math::max(mask_smoothness, 0.001));
    material->set_shader_parameter("progress", progress);

    // Invisible (et sans coût) quand l'écran est découvert
    overlay->set_visible(progress > 0.0);
    overlay->set_mouse_filter(block_input && progress > 0.0 ? Control::MOUSE_FILTER_STOP : Control::MOUSE_FILTER_IGNORE);
}

// 🎛️ Réglages

void ScreenFader::set_progress(double p_value)
{
    progress = Math::clamp(p_value, 0.0, 1.0);
    _update_overlay();
}

void ScreenFader::set_color(const Color &p_value)
{
    color = p_value;
    _update_overlay();
}

void ScreenFader::set_texture(const Ref<Texture2D> &p_value)
{
    texture = p_value;
    _update_overlay();
}

void ScreenFader::set_mask(const Ref<Texture2D> &p_value)
{
    mask = p_value;
    _update_overlay();
}

void ScreenFader::set_mask_smoothness(double p_value)
{
    mask_smoothness = p_value;
    _update_overlay();
}

void ScreenFader::set_block_input(bool p_value)
{
    block_input = p_value;
    _update_overlay();
}

// 🔗 Liaisons

void ScreenFader::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("fade_out", "duration"), &ScreenFader::fade_out, DEFVAL(-1.0));
    ClassDB::bind_method(D_METHOD("fade_in", "duration"), &ScreenFader::fade_in, DEFVAL(-1.0));
    ClassDB::bind_method(D_METHOD("fade_to", "progress", "duration"), &ScreenFader::fade_to, DEFVAL(-1.0));
    ClassDB::bind_method(D_METHOD("is_fading"), &ScreenFader::is_fading);
    ClassDB::bind_method(D_METHOD("is_covered"), &ScreenFader::is_covered);
    ClassDB::bind_method(D_METHOD("set_progress", "value"), &ScreenFader::set_progress);
    ClassDB::bind_method(D_METHOD("get_progress"), &ScreenFader::get_progress);
    ClassDB::bind_method(D_METHOD("set_color", "value"), &ScreenFader::set_color);
    ClassDB::bind_method(D_METHOD("get_color"), &ScreenFader::get_color);
    ClassDB::bind_method(D_METHOD("set_texture", "value"), &ScreenFader::set_texture);
    ClassDB::bind_method(D_METHOD("get_texture"), &ScreenFader::get_texture);
    ClassDB::bind_method(D_METHOD("set_mask", "value"), &ScreenFader::set_mask);
    ClassDB::bind_method(D_METHOD("get_mask"), &ScreenFader::get_mask);
    ClassDB::bind_method(D_METHOD("set_mask_smoothness", "value"), &ScreenFader::set_mask_smoothness);
    ClassDB::bind_method(D_METHOD("get_mask_smoothness"), &ScreenFader::get_mask_smoothness);
    ClassDB::bind_method(D_METHOD("set_duration", "value"), &ScreenFader::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &ScreenFader::get_duration);
    ClassDB::bind_method(D_METHOD("set_transition", "value"), &ScreenFader::set_transition);
    ClassDB::bind_method(D_METHOD("get_transition"), &ScreenFader::get_transition);
    ClassDB::bind_method(D_METHOD("set_ease_type", "value"), &ScreenFader::set_ease_type);
    ClassDB::bind_method(D_METHOD("get_ease_type"), &ScreenFader::get_ease_type);
    ClassDB::bind_method(D_METHOD("set_block_input", "value"), &ScreenFader::set_block_input);
    ClassDB::bind_method(D_METHOD("get_block_input"), &ScreenFader::get_block_input);
    ClassDB::bind_method(D_METHOD("set_always_process", "value"), &ScreenFader::set_always_process);
    ClassDB::bind_method(D_METHOD("get_always_process"), &ScreenFader::get_always_process);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "progress", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_progress", "get_progress");

    ADD_GROUP("Appearance", "");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mask", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_mask", "get_mask");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mask_smoothness", PROPERTY_HINT_RANGE, "0.001,1,0.001"), "set_mask_smoothness", "get_mask_smoothness");

    ADD_GROUP("Animation", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0,5,0.01,or_greater,suffix:s"), "set_duration", "get_duration");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "transition", PROPERTY_HINT_ENUM, "Linear,Sine,Quint,Quart,Quad,Expo,Elastic,Cubic,Circ,Bounce,Back,Spring"), "set_transition", "get_transition");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ease_type", PROPERTY_HINT_ENUM, "In,Out,In Out,Out In"), "set_ease_type", "get_ease_type");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "always_process"), "set_always_process", "get_always_process");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "block_input"), "set_block_input", "get_block_input");

    ADD_SIGNAL(MethodInfo("fade_finished", PropertyInfo(Variant::FLOAT, "progress")));
    ADD_SIGNAL(MethodInfo("faded_out"));
    ADD_SIGNAL(MethodInfo("faded_in"));
}
