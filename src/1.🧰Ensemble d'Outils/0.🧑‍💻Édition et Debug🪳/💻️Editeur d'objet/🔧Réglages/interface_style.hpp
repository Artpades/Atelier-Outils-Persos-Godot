#ifndef INTERFACE_STYLE_HPP
#define INTERFACE_STYLE_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/signal_link.hpp"

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// Uniform d'un shader alimenté par une valeur de l'objet édité (même sources que les gizmos : valeur manuelle,
/// propriété, fonction). Mis à jour en direct par le SyncController de l'éditeur.
class ShaderParameterBinding : public Resource
{
    GDCLASS(ShaderParameterBinding, Resource)

  private:
    StringName parameter;
    Ref<ArgumentBinding> binding;

  protected:
    static void _bind_methods();

  public:
    void set_parameter(const StringName &p_parameter);
    StringName get_parameter() const;
    void set_binding(const Ref<ArgumentBinding> &p_binding);
    Ref<ArgumentBinding> get_binding(); // créé à la première lecture
    Ref<ArgumentBinding> get_stored_binding() const; // tel qu'enregistré (peut être nul)
};

/// Propagation d'un matériau aux éléments ajoutés plus tard (sous-objets dépliés, reconstruction) : objet
/// unique (créé au premier besoin, détruit au déchargement du module) dont la méthode est branchée sur
/// child_entered_tree des nœuds concernés.
class MaterialPropagator : public Object
{
    GDCLASS(MaterialPropagator, Object)

  private:
    inline static MaterialPropagator *singleton = nullptr;

  protected:
    static void _bind_methods() {}

  public:
    static MaterialPropagator *get_singleton();
    static void destroy();
    /// Élément ajouté sous un nœud propagé : il réutilise le matériau de son parent, ainsi que sa descendance.
    void on_child_entered(const Variant &p_child);
    /// p_enable : descendants de p_node en use_parent_material (sauf ceux qui ont leur propre matériau) et suivi
    /// des ajouts ; sinon, l'inverse.
    void propagate(Node *p_node, bool p_enable);
};

/// Style d'une partie de l'interface générée (interface entière, groupe ou widget). Le style le plus précis
/// l'emporte naturellement : le thème se transmet aux enfants sauf s'ils ont le leur, le matériau est propre
/// à chaque nœud.
///  - theme / theme_type_variation : apparence des contrôles ;
///  - background : StyleBox dessinée derrière l'élément (interface, groupe, widget entier) ;
///  - material : en pratique un ShaderMaterial « canvas_item ». propagate_material : les descendants le
///    réutilisent (use_parent_material), y compris ceux créés plus tard (sous-objets dépliés...) ;
///  - shader_parameters : uniforms liés à des valeurs de l'objet. Le matériau est alors DUPLIQUÉ pour chaque
///    élément stylé, pour que deux éléments n'écrivent pas dans le même matériau.
class InterfaceStyle : public Resource
{
    GDCLASS(InterfaceStyle, Resource)

  private:
    Ref<Theme> theme;
    StringName theme_type_variation;
    Ref<StyleBox> background;
    Ref<Material> material;
    bool propagate_material = true;
    Color modulate = Color(1, 1, 1, 1);
    Color self_modulate = Color(1, 1, 1, 1);
    TypedArray<ShaderParameterBinding> shader_parameters;


  protected:
    static void _bind_methods();

  public:
    static constexpr const char *DEFAULT_STYLE_SETTING = "gameplay_toolkit/object_editor/default_style";
    static void register_project_setting();
    /// Style par défaut du projet (réglage DEFAULT_STYLE_SETTING), nul si non renseigné.
    static Ref<InterfaceStyle> load_project_default();

    /// Applique le style à p_item (thème et variation si c'est un Control, teintes, matériau, propagation).
    /// Retourne le matériau posé (dupliqué si des uniforms sont liés).
    Ref<Material> apply_to(CanvasItem *p_item) const;
    /// Retire ce qu'un style a pu poser sur p_item (thème, variation, teintes, matériau, propagation).
    static void clear_from(CanvasItem *p_item);
    /// Retire tout style de p_item puis remet son style de base (p_base, peut être nul) ; sans matériau de base,
    /// p_item reprend la propagation du matériau de son parent si celui-ci en a une.
    static void restore(CanvasItem *p_item, const Ref<InterfaceStyle> &p_base);
    /// Uniforms liés : valeurs lues sur p_context (cible de l'éditeur) et écrites dans le matériau de p_item.
    void update_parameters(CanvasItem *p_item, Object *p_context) const;
    bool has_parameter_bindings() const;
    /// Uniforms du shader du matériau (liste de Godot : name, type, hint, hint_string...) ; vide sans shader.
    TypedArray<Dictionary> get_shader_uniforms() const;
    /// Lien de l'uniform p_parameter (nul s'il n'est pas lié).
    Ref<ShaderParameterBinding> find_parameter(const StringName &p_parameter) const;

    void set_theme(const Ref<Theme> &p_theme);
    Ref<Theme> get_theme() const;
    void set_theme_type_variation(const StringName &p_variation);
    StringName get_theme_type_variation() const;
    void set_background(const Ref<StyleBox> &p_background);
    Ref<StyleBox> get_background() const;
    void set_material(const Ref<Material> &p_material);
    Ref<Material> get_material() const;
    void set_propagate_material(bool p_propagate);
    bool get_propagate_material() const;
    void set_modulate(const Color &p_color);
    Color get_modulate() const;
    void set_self_modulate(const Color &p_color);
    Color get_self_modulate() const;
    void set_shader_parameters(const TypedArray<ShaderParameterBinding> &p_parameters);
    TypedArray<ShaderParameterBinding> get_shader_parameters() const;
};
} // namespace godot

#endif // INTERFACE_STYLE_HPP
