#ifndef SPECIAL_WIDGETS_HPP
#define SPECIAL_WIDGETS_HPP

#include "property_widget.hpp"

#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot
{
/// Sélecteur de nœud en jeu (l'éditeur utilise EditorInterface::popup_node_selector) : arbre de la scène
/// courante ; les nœuds d'un type non autorisé sont grisés. Se libère à la fermeture.
class NodeSelectorDialog : public AcceptDialog
{
    GDCLASS(NodeSelectorDialog, AcceptDialog)

  private:
    Tree *tree = nullptr; // enfant possédé
    Callable on_selected; // appelé avec le Node choisi
    PackedStringArray valid_types;

    void _add_node(Node *p_node, TreeItem *p_parent);
    void _on_confirmed();
    void _on_closed();

  protected:
    static void _bind_methods() {}

  public:
    /// Ouvre le sélecteur sous p_owner ; p_callback(Node) est appelé à la validation.
    static void open(Node *p_owner, Node *p_root, const PackedStringArray &p_valid_types, const Callable &p_callback);
};

/// NodePath, ou Object de type nœud (choix « PathButton ») : chemin + bouton « … » (sélecteur de nœud) ;
/// pour un NodePath dans l'éditeur, « Propriété… » ajoute un sous-chemin de propriété (popup_property_selector).
/// Le chemin est relatif au nœud qui porte la propriété.
class PathWidget : public PropertyWidget
{
    GDCLASS(PathWidget, PropertyWidget)

  private:
    LineEdit *line = nullptr; // enfants possédés
    bool object_mode = false; // Object (nœud) plutôt que NodePath

    Node *_base_node() const;
    PackedStringArray _valid_types() const;
    void _apply_node(Node *p_node);
    void _on_pick();
    void _on_pick_property();
    void _on_editor_node_selected(const NodePath &p_path);
    void _on_editor_property_selected(const NodePath &p_property);
    void _on_runtime_node_selected(Node *p_node);
    void _on_text_submitted(const String &p_text);
    void _on_clear();

  protected:
    static void _bind_methods() {}
    Control *build_editor() override;
    void pull_value(const Variant &p_value) override;
};

/// Variant : choix du type (OptionButton) + widget du type choisi. Changer de type convertit la valeur
/// quand c'est possible (scalaires entre eux, vecteurs entiers / décimaux...), sinon valeur par défaut du type.
class VariantWidget : public PropertyWidget
{
    GDCLASS(VariantWidget, PropertyWidget)

  private:
    OptionButton *types = nullptr; // enfants possédés
    VBoxContainer *holder = nullptr;
    int shown_type = -1;

    void _show(Variant::Type p_type);
    void _on_type_selected(int64_t p_index);

  protected:
    static void _bind_methods() {}
    Control *build_editor() override;
    void pull_value(const Variant &p_value) override;

  public:
    static bool convertible(Variant::Type p_from, Variant::Type p_to);
};

/// Valeurs locales des arguments d'une méthode : propriétés dynamiques "arg_0", "arg_1"... décrites comme
/// les paramètres de la méthode, initialisées avec default_args (alignés sur les derniers paramètres).
class ArgumentStore : public RefCounted
{
    GDCLASS(ArgumentStore, RefCounted)

  private:
    Array values;
    Array infos; // Dictionary par argument

  protected:
    static void _bind_methods() {}
    bool _set(const StringName &p_name, const Variant &p_value);
    bool _get(const StringName &p_name, Variant &r_value) const;
    void _get_property_list(List<PropertyInfo> *p_list) const;

  public:
    void setup(const Dictionary &p_method_info);
    int get_count() const;
    Dictionary get_info(int p_index) const;
    Array get_values() const;
};

/// Méthode (choix « Button ») : bouton au nom de la méthode, un widget par argument (valeurs locales, hors
/// historique d'annulation), appel par callv, résultat affiché si la méthode retourne une valeur.
class MethodWidget : public PropertyWidget
{
    GDCLASS(MethodWidget, PropertyWidget)

  private:
    Ref<ArgumentStore> arguments;
    Label *result = nullptr; // enfant possédé

    void _on_call();

  protected:
    static void _bind_methods() {}
    bool has_own_title() const override { return true; }
    Control *build_editor() override;
    void pull_value(const Variant &p_value) override { (void)p_value; }

  public:
    bool is_value_widget() const override { return false; }
    /// Signature lisible : nom(a: int, b: String = "x") -> int
    static String signature(const Dictionary &p_method_info);
};
} // namespace godot

#endif // SPECIAL_WIDGETS_HPP
