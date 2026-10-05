#include "preview_widget.hpp"

#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/visual_instance3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace
{
Label *make_note(const String &p_text)
{
    Label *label = memnew(Label);
    label->set_text(p_text);
    label->set_modulate(Color(1, 1, 1, 0.6));
    label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    return label;
}

void free_children(Node *p_node)
{
    TypedArray<Node> children = p_node->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        p_node->remove_child(child);
        child->queue_free();
    }
}
} // namespace

// =====================================================================================================
// PreviewStage3D
// =====================================================================================================

void PreviewStage3D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_content", "node"), &PreviewStage3D::set_content);
    ClassDB::bind_method(D_METHOD("get_content"), &PreviewStage3D::get_content);
    ClassDB::bind_method(D_METHOD("frame"), &PreviewStage3D::frame);
    ClassDB::bind_method(D_METHOD("set_orbit", "yaw", "pitch", "distance"), &PreviewStage3D::set_orbit);
    ClassDB::bind_method(D_METHOD("get_viewport_node"), &PreviewStage3D::get_viewport_node);
    ClassDB::bind_method(D_METHOD("get_camera"), &PreviewStage3D::get_camera);
}

PreviewStage3D::PreviewStage3D()
{
    set_stretch(true);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    viewport = memnew(SubViewport);
    viewport->set_use_own_world_3d(true); // monde isolé : la vraie scène n'est pas touchée
    viewport->set_update_mode(SubViewport::UPDATE_WHEN_VISIBLE);
    add_child(viewport);
    camera = memnew(Camera3D);
    viewport->add_child(camera);
    camera->set_current(true);
    DirectionalLight3D *light = memnew(DirectionalLight3D);
    light->set_rotation_degrees(Vector3(-50, 30, 0));
    viewport->add_child(light);
    content = memnew(Node3D);
    viewport->add_child(content);
    _update_camera();
}

void PreviewStage3D::_update_camera()
{
    Vector3 offset(Math::cos(pitch) * Math::sin(yaw), -Math::sin(pitch), Math::cos(pitch) * Math::cos(yaw));
    camera->look_at_from_position(center + offset * distance, center, Vector3(0, 1, 0));
}

void PreviewStage3D::set_content(Node3D *p_node)
{
    free_children(content);
    if (p_node)
    {
        content->add_child(p_node);
    }
    frame();
}

Node3D *PreviewStage3D::get_content() const { return content->get_child_count() > 0 ? Object::cast_to<Node3D>(content->get_child(0)) : nullptr; }

void PreviewStage3D::frame()
{
    // Boîte englobante des maillages (repère du contenu, transformations locales cumulées).
    AABB box;
    bool found = false;
    TypedArray<Node> nodes = content->find_children("*", "VisualInstance3D", true, false);
    for (int i = 0; i < nodes.size(); i++)
    {
        VisualInstance3D *instance = Object::cast_to<VisualInstance3D>(nodes[i]);
        Transform3D transform;
        for (Node *node = instance; node && node != content; node = node->get_parent())
        {
            if (Node3D *spatial = Object::cast_to<Node3D>(node))
            {
                transform = spatial->get_transform() * transform;
            }
        }
        AABB local = transform.xform(instance->get_aabb());
        box = found ? box.merge(local) : local;
        found = true;
    }
    center = found ? box.get_center() : Vector3();
    distance = found ? MAX(box.get_longest_axis_size() * 1.8, 0.5) : 3.0;
    _update_camera();
}

void PreviewStage3D::set_orbit(double p_yaw, double p_pitch, double p_distance)
{
    yaw = p_yaw;
    pitch = CLAMP(p_pitch, -1.5, 1.5);
    distance = MAX(p_distance, 0.05);
    _update_camera();
}

void PreviewStage3D::_gui_input(const Ref<InputEvent> &p_event)
{
    Ref<InputEventMouseButton> button = p_event;
    if (button.is_valid())
    {
        if (button->get_button_index() == MOUSE_BUTTON_WHEEL_UP || button->get_button_index() == MOUSE_BUTTON_WHEEL_DOWN)
        {
            set_orbit(yaw, pitch, distance * (button->get_button_index() == MOUSE_BUTTON_WHEEL_UP ? 0.9 : 1.1));
            accept_event();
        }
        else if (button->get_button_index() == MOUSE_BUTTON_LEFT)
        {
            dragging = button->is_pressed();
            accept_event();
        }
        return;
    }
    Ref<InputEventMouseMotion> motion = p_event;
    if (motion.is_valid() && dragging)
    {
        set_orbit(yaw - motion->get_relative().x * 0.01, pitch + motion->get_relative().y * 0.01, distance);
        accept_event();
    }
}

SubViewport *PreviewStage3D::get_viewport_node() const { return viewport; }
Camera3D *PreviewStage3D::get_camera() const { return camera; }

// =====================================================================================================
// PreviewStage2D
// =====================================================================================================

void PreviewStage2D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_content", "node"), &PreviewStage2D::set_content);
    ClassDB::bind_method(D_METHOD("get_content"), &PreviewStage2D::get_content);
    ClassDB::bind_method(D_METHOD("frame"), &PreviewStage2D::frame);
    ClassDB::bind_method(D_METHOD("get_viewport_node"), &PreviewStage2D::get_viewport_node);
    ClassDB::bind_method(D_METHOD("get_camera"), &PreviewStage2D::get_camera);
}

PreviewStage2D::PreviewStage2D()
{
    set_stretch(true);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    viewport = memnew(SubViewport); // chaque Viewport a son propre World2D
    viewport->set_update_mode(SubViewport::UPDATE_WHEN_VISIBLE);
    add_child(viewport);
    camera = memnew(Camera2D);
    viewport->add_child(camera);
    camera->set_enabled(true);
    content = memnew(Node2D);
    viewport->add_child(content);
}

void PreviewStage2D::set_content(Node2D *p_node)
{
    free_children(content);
    if (p_node)
    {
        content->add_child(p_node);
    }
    frame();
}

Node2D *PreviewStage2D::get_content() const { return content->get_child_count() > 0 ? Object::cast_to<Node2D>(content->get_child(0)) : nullptr; }

void PreviewStage2D::frame()
{
    // Rectangle des éléments qui exposent get_rect (Sprite2D...), transformations locales cumulées.
    Rect2 box;
    bool found = false;
    TypedArray<Node> nodes = content->find_children("*", "Node2D", true, false);
    for (int i = 0; i < nodes.size(); i++)
    {
        Node2D *node = Object::cast_to<Node2D>(nodes[i]);
        if (!node->has_method("get_rect"))
        {
            continue;
        }
        Transform2D transform;
        for (Node *walk = node; walk && walk != content; walk = walk->get_parent())
        {
            if (Node2D *n2 = Object::cast_to<Node2D>(walk))
            {
                transform = n2->get_transform() * transform;
            }
        }
        Rect2 local = transform.xform(Rect2(node->call("get_rect")));
        box = found ? box.merge(local) : local;
        found = true;
    }
    camera->set_position(found ? box.get_center() : Vector2());
    Vector2 size = get_size().x > 1 ? get_size() : Vector2(160, 160);
    double zoom = (found && box.size.x > 0 && box.size.y > 0) ? MIN(size.x / box.size.x, size.y / box.size.y) * 0.9 : 1.0;
    camera->set_zoom(Vector2(zoom, zoom));
}

void PreviewStage2D::_gui_input(const Ref<InputEvent> &p_event)
{
    Ref<InputEventMouseButton> button = p_event;
    if (button.is_valid())
    {
        if (button->get_button_index() == MOUSE_BUTTON_WHEEL_UP || button->get_button_index() == MOUSE_BUTTON_WHEEL_DOWN)
        {
            camera->set_zoom(camera->get_zoom() * (button->get_button_index() == MOUSE_BUTTON_WHEEL_UP ? 1.1 : 0.9));
            accept_event();
        }
        else if (button->get_button_index() == MOUSE_BUTTON_LEFT)
        {
            dragging = button->is_pressed();
            accept_event();
        }
        return;
    }
    Ref<InputEventMouseMotion> motion = p_event;
    if (motion.is_valid() && dragging)
    {
        camera->set_position(camera->get_position() - motion->get_relative() / camera->get_zoom());
        accept_event();
    }
}

SubViewport *PreviewStage2D::get_viewport_node() const { return viewport; }
Camera2D *PreviewStage2D::get_camera() const { return camera; }

// =====================================================================================================
// PreviewWidget
// =====================================================================================================

void PreviewWidget::_bind_methods()
{
    ClassDB::bind_static_method("PreviewWidget", D_METHOD("make_info", "name"), &PreviewWidget::make_info);
    ClassDB::bind_method(D_METHOD("rebuild_preview"), &PreviewWidget::rebuild_preview);
    ClassDB::bind_method(D_METHOD("get_source"), &PreviewWidget::get_source);
    ClassDB::bind_method(D_METHOD("get_preview_content"), &PreviewWidget::get_preview_content);
    ClassDB::bind_method(D_METHOD("get_preview_settings"), &PreviewWidget::get_preview_settings);
    ClassDB::bind_method(D_METHOD("make_stage_3d"), &PreviewWidget::make_stage_3d);
    ClassDB::bind_method(D_METHOD("make_stage_2d"), &PreviewWidget::make_stage_2d);
    ClassDB::bind_method(D_METHOD("instantiate_source_node", "source"), &PreviewWidget::instantiate_source_node);
    ClassDB::bind_method(D_METHOD("get_source_hint"), &PreviewWidget::get_source_hint);

    GDVIRTUAL_BIND(_build_preview, "source");
    GDVIRTUAL_BIND(_update_preview, "source", "delta");
    GDVIRTUAL_BIND(_on_source_changed, "source");
    GDVIRTUAL_BIND(_get_source_hint);
}

Dictionary PreviewWidget::make_info(const StringName &p_name)
{
    Dictionary info;
    info["name"] = p_name;
    info["type"] = Variant::NIL;
    info["usage"] = (int)(PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_NIL_IS_VARIANT | PROPERTY_USAGE_READ_ONLY);
    return info;
}

Ref<PreviewSettings> PreviewWidget::get_preview_settings() const { return get_settings(); }

Control *PreviewWidget::build_editor()
{
    holder = memnew(PanelContainer);
    Ref<PreviewSettings> settings = get_preview_settings();
    holder->set_custom_minimum_size(Vector2(0, settings.is_valid() ? settings->get_height() : 160));
    holder->set_h_size_flags(SIZE_EXPAND_FILL);
    holder->set_clip_contents(true);
    return holder;
}

void PreviewWidget::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY)
    {
        // Premier affichage sans attendre la relecture (l'accesseur d'un aperçu ne désigne aucune propriété).
        if (!content_built)
        {
            pull(read_value());
        }
        set_process(wants_process());
    }
}

Variant PreviewWidget::read_value() const
{
    Ref<ValueAccessor> accessor = get_accessor();
    Object *context = accessor.is_valid() ? accessor->get_target() : nullptr;
    Ref<PreviewSettings> settings = get_preview_settings();
    Ref<ArgumentBinding> source = settings.is_valid() ? settings->get_stored_source() : Ref<ArgumentBinding>();
    // Source absente, ou Propriété sans chemin : l'objet désigné (l'objet affiché par défaut).
    if (source.is_null())
    {
        return Variant(context);
    }
    if (source->get_source() == ArgumentBinding::SOURCE_PROPERTY && source->get_property_path().is_empty())
    {
        return Variant(source->resolve_object(context));
    }
    return source->resolve(context, Array());
}

void PreviewWidget::pull_value(const Variant &p_value)
{
    bool changed = !content_built || p_value != current_source;
    current_source = p_value;
    if (!content_built)
    {
        rebuild_preview();
    }
    else if (changed)
    {
        on_source_changed(p_value);
    }
}

void PreviewWidget::rebuild_preview()
{
    if (!holder)
    {
        return;
    }
    free_children(holder);
    Control *content = build_preview(current_source);
    if (!content)
    {
        content = make_note(U"Aucun aperçu");
    }
    holder->add_child(content);
    content_id = ObjectID(content->get_instance_id());
    content_built = true;
}

void PreviewWidget::_process(double p_delta) { update_preview(current_source, p_delta); }

Control *PreviewWidget::build_preview(const Variant &p_source)
{
    Control *result = nullptr;
    if (GDVIRTUAL_CALL(_build_preview, p_source, result))
    {
        return result;
    }
    return make_note(U"PreviewWidget : surcharger _build_preview(source)");
}

void PreviewWidget::update_preview(const Variant &p_source, double p_delta) { GDVIRTUAL_CALL(_update_preview, p_source, p_delta); }

void PreviewWidget::on_source_changed(const Variant &p_source)
{
    if (!GDVIRTUAL_CALL(_on_source_changed, p_source))
    {
        rebuild_preview();
    }
}

Dictionary PreviewWidget::get_source_hint() const
{
    Dictionary hint;
    GDVIRTUAL_CALL(_get_source_hint, hint);
    return hint;
}

bool PreviewWidget::wants_process() const
{
    Ref<PreviewSettings> settings = get_preview_settings();
    return GDVIRTUAL_IS_OVERRIDDEN(_update_preview) || (settings.is_valid() && settings->get_update_mode() == PreviewSettings::UPDATE_EVERY_FRAME);
}

Variant PreviewWidget::get_source() const { return current_source; }
Control *PreviewWidget::get_preview_content() const { return Object::cast_to<Control>(ObjectDB::get_instance(content_id)); }
PreviewStage3D *PreviewWidget::make_stage_3d() const { return memnew(PreviewStage3D); }
PreviewStage2D *PreviewWidget::make_stage_2d() const { return memnew(PreviewStage2D); }

Node *PreviewWidget::instantiate_source_node(const Variant &p_source) const
{
    Object *object = p_source.get_validated_object();
    if (Ref<PackedScene> scene = Ref<PackedScene>(Object::cast_to<PackedScene>(object)); scene.is_valid())
    {
        return scene->instantiate();
    }
    if (Node *node = Object::cast_to<Node>(object))
    {
        // Copie : l'original (souvent dans la scène éditée ou le jeu) n'est jamais déplacé ni modifié. Sans ses
        // scripts par défaut (pas de logique de jeu exécutée dans l'aperçu) ni ses connexions.
        Ref<PreviewSettings> settings = get_preview_settings();
        int flags = Node::DUPLICATE_GROUPS | Node::DUPLICATE_USE_INSTANTIATION;
        if (settings.is_valid() && settings->get_duplicate_scripts())
        {
            flags |= Node::DUPLICATE_SCRIPTS;
        }
        return node->duplicate(flags);
    }
    return nullptr;
}

// =====================================================================================================
// Aperçus fournis
// =====================================================================================================

Control *TexturePreview::build_preview(const Variant &p_source)
{
    Ref<Texture2D> texture = Ref<Texture2D>(Object::cast_to<Texture2D>(p_source.get_validated_object()));
    if (texture.is_null())
    {
        return make_note(U"Aucune texture");
    }
    TextureRect *rect = memnew(TextureRect);
    rect->set_texture(texture);
    rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    return rect;
}

Dictionary TexturePreview::get_source_hint() const
{
    Dictionary hint;
    hint["type"] = Variant::OBJECT;
    hint["class_name"] = "Texture2D";
    return hint;
}

Control *Model3DPreview::build_preview(const Variant &p_source)
{
    Object *object = p_source.get_validated_object();
    Node3D *model = nullptr;
    if (Mesh *mesh = Object::cast_to<Mesh>(object))
    {
        MeshInstance3D *instance = memnew(MeshInstance3D);
        instance->set_mesh(Ref<Mesh>(mesh));
        model = instance;
    }
    else if (Node *node = instantiate_source_node(p_source))
    {
        model = Object::cast_to<Node3D>(node);
        if (!model)
        {
            memdelete(node);
            return make_note(U"Pas une scène 3D");
        }
    }
    if (!model)
    {
        return make_note(U"Aucun modèle (Mesh, PackedScene ou Node3D)");
    }
    PreviewStage3D *stage = make_stage_3d();
    stage->set_content(model);
    return stage;
}

Control *Scene2DPreview::build_preview(const Variant &p_source)
{
    Object *object = p_source.get_validated_object();
    Node2D *scene = nullptr;
    if (Texture2D *texture = Object::cast_to<Texture2D>(object))
    {
        Sprite2D *sprite = memnew(Sprite2D);
        sprite->set_texture(Ref<Texture2D>(texture));
        scene = sprite;
    }
    else if (Node *node = instantiate_source_node(p_source))
    {
        scene = Object::cast_to<Node2D>(node);
        if (!scene)
        {
            memdelete(node);
            return make_note(U"Pas une scène 2D");
        }
    }
    if (!scene)
    {
        return make_note(U"Aucune scène (PackedScene, Node2D ou Texture2D)");
    }
    PreviewStage2D *stage = make_stage_2d();
    stage->set_content(scene);
    return stage;
}
