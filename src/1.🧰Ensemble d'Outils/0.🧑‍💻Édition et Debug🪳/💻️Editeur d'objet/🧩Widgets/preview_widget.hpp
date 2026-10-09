#ifndef PREVIEW_WIDGET_HPP
#define PREVIEW_WIDGET_HPP

#include "property_widget.hpp"

#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/sub_viewport_container.hpp>

namespace godot
{
/// Scène 3D isolée pour un aperçu : SubViewport avec son propre monde (rien ne touche la vraie scène), caméra
/// en orbite autour du contenu (glisser : tourner, molette : zoom), lumière. Le contenu est libéré avec la scène.
class PreviewStage3D : public SubViewportContainer
{
    GDCLASS(PreviewStage3D, SubViewportContainer)

  private:
    SubViewport *viewport = nullptr; // enfants possédés
    Camera3D *camera = nullptr;
    Node3D *content = nullptr;
    double yaw = 0.7;
    double pitch = -0.4;
    double distance = 3.0;
    Vector3 center;
    bool dragging = false;

    void _update_camera();

  protected:
    static void _bind_methods();

  public:
    PreviewStage3D();
    void _gui_input(const Ref<InputEvent> &p_event) override;

    /// Remplace le contenu (p_node possédé ensuite par la scène ; nul : vide) puis recadre.
    void set_content(Node3D *p_node);
    Node3D *get_content() const;
    /// Cadre la caméra sur la boîte englobante des maillages du contenu.
    void frame();
    void set_orbit(double p_yaw, double p_pitch, double p_distance);
    SubViewport *get_viewport_node() const;
    Camera3D *get_camera() const;
};

/// Scène 2D isolée pour un aperçu : SubViewport (son propre monde 2D), Camera2D (glisser : déplacer,
/// molette : zoom). Le contenu est libéré avec la scène.
class PreviewStage2D : public SubViewportContainer
{
    GDCLASS(PreviewStage2D, SubViewportContainer)

  private:
    SubViewport *viewport = nullptr; // enfants possédés
    Camera2D *camera = nullptr;
    Node2D *content = nullptr;
    bool dragging = false;

  protected:
    static void _bind_methods();

  public:
    PreviewStage2D();
    void _gui_input(const Ref<InputEvent> &p_event) override;

    void set_content(Node2D *p_node);
    Node2D *get_content() const;
    /// Centre la caméra sur le contenu (rectangles des Sprite2D et autres get_rect) et ajuste le zoom.
    void frame();
    SubViewport *get_viewport_node() const;
    Camera2D *get_camera() const;
};

/// Aperçu personnalisé (n'édite aucune propriété) : une entrée MEMBER_PREVIEW du registre, placée, groupée,
/// stylée et ciblée par les comportements comme une donnée. Sa SOURCE (PreviewSettings::source) est l'objet
/// affiché, ou une propriété / une fonction / une valeur manuelle ; elle est relue par le SyncController.
/// Extension (C++ ou script, class_name ... extends PreviewWidget) :
///  - _build_preview(source) -> Control : construit l'aperçu (appelé à la première source et à chaque changement) ;
///  - _update_preview(source, delta) : à chaque image (mise à jour « Chaque image », ou si surchargée) ;
///  - _on_source_changed(source) : par défaut, reconstruit l'aperçu ;
///  - _get_source_hint() -> Dictionary : type attendu ({type, class_name}) pour proposer les membres compatibles.
/// Aides : make_stage_3d(), make_stage_2d(), instantiate_source_node(source) (PackedScene instanciée ou nœud
/// dupliqué, scripts compris seulement si duplicate_scripts).
class PreviewWidget : public PropertyWidget
{
    GDCLASS(PreviewWidget, PropertyWidget)

  private:
    PanelContainer *holder = nullptr; // enfant possédé (contient l'aperçu)
    ObjectID content_id;
    Variant current_source;
    bool content_built = false;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

    GDVIRTUAL1R(Control *, _build_preview, Variant)
    GDVIRTUAL2(_update_preview, Variant, double)
    GDVIRTUAL1(_on_source_changed, Variant)
    GDVIRTUAL0RC(Dictionary, _get_source_hint)

    Control *build_editor() override;
    void pull_value(const Variant &p_value) override;
    StringName get_settings_class() const override { return "PreviewSettings"; }

  public:
    /// Description d'un aperçu pour un ValueAccessor (pas une propriété de l'objet).
    static Dictionary make_info(const StringName &p_name);

    void _process(double p_delta) override;
    Variant read_value() const override;

    /// Équivalents C++ des méthodes virtuelles de script (_build_preview, _update_preview, _on_source_changed,
    /// _get_source_hint) ; wants_process : vrai si _update_preview est surchargée ou si la mise à jour est « Chaque image ».
    virtual Control *build_preview(const Variant &p_source);
    virtual void update_preview(const Variant &p_source, double p_delta);
    virtual void on_source_changed(const Variant &p_source);
    virtual Dictionary get_source_hint() const;
    virtual bool wants_process() const;

    void rebuild_preview();
    Variant get_source() const;
    Control *get_preview_content() const;
    Ref<PreviewSettings> get_preview_settings() const;
    PreviewStage3D *make_stage_3d() const;
    PreviewStage2D *make_stage_2d() const;
    /// PackedScene : instance ; Node : copie (scripts selon duplicate_scripts) ; nul sinon. À ajouter à une scène.
    Node *instantiate_source_node(const Variant &p_source) const;
};

/// Image : Texture2D (source ou valeur manuelle).
class TexturePreview : public PreviewWidget
{
    GDCLASS(TexturePreview, PreviewWidget)

  protected:
    static void _bind_methods() {}

  public:
    Control *build_preview(const Variant &p_source) override;
    Dictionary get_source_hint() const override;
};

/// Modèle 3D : Mesh, PackedScene 3D ou Node3D (copie), dans une scène 3D isolée.
class Model3DPreview : public PreviewWidget
{
    GDCLASS(Model3DPreview, PreviewWidget)

  protected:
    static void _bind_methods() {}

  public:
    Control *build_preview(const Variant &p_source) override;
};

/// Scène 2D : PackedScene 2D, Node2D (copie) ou Texture2D (sprite), dans une scène 2D isolée.
class Scene2DPreview : public PreviewWidget
{
    GDCLASS(Scene2DPreview, PreviewWidget)

  protected:
    static void _bind_methods() {}

  public:
    Control *build_preview(const Variant &p_source) override;
};
} // namespace godot

#endif // PREVIEW_WIDGET_HPP
