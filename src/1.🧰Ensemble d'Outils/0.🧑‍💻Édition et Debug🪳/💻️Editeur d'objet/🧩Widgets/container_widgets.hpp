#ifndef CONTAINER_WIDGETS_HPP
#define CONTAINER_WIDGETS_HPP

#include "property_widget.hpp"

#include <godot_cpp/classes/container.hpp>
#include <godot_cpp/classes/foldable_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
// Base des widgets qui contiennent d'autres widgets (collections, objets). Section repliable
// (FoldableContainer) dont le contenu n'est construit qu'à l'ouverture (dépliage paresseux), puis
// reconstruit seulement quand la STRUCTURE de la valeur change (taille, clés, types, objet) : les valeurs
// des éléments sont suivies par leurs propres widgets (enregistrés auprès du SyncController).
// Disposition des éléments selon le choix : BoxContainer (orientation is_vertical), GridContainer (colonnes),
// FlowContainer, TabContainer. Réglages : ContainerWidgetSettings.
class ContainerWidget : public PropertyWidget
{
    GDCLASS(ContainerWidget, PropertyWidget)

  private:
    FoldableContainer *foldable = nullptr; // enfants possédés
    Label *title_label = nullptr;
    Container *items = nullptr;
    VBoxContainer *footer = nullptr;
    String built_key;
    bool built = false;

    void _on_folding_changed(bool p_folded);
    void _set_title(const String &p_text);

  protected:
    static void _bind_methods() {}

    bool has_own_title() const override { return true; }
    Control *build_editor() override;
    void pull_value(const Variant &p_value) override;
    StringName get_settings_class() const override { return "ContainerWidgetSettings"; }

    // À surcharger.
    virtual String summary(const Variant &p_value) const;
    virtual String structure_key(const Variant &p_value) const;
    virtual void fill(const Variant &p_value) { (void)p_value; }

    // Utilitaires pour fill().
    void add_item(Control *p_control, const String &p_title);
    void add_footer(Control *p_control);
    void add_note(const String &p_text);
    int get_max_displayed() const;
    bool is_expanded() const;
    void clear_items();
    // Reconstruit le contenu avec la valeur actuelle (différé si appelé depuis un bouton du contenu).
    void rebuild_now();
    void rebuild_deferred();
};

// Array, Dictionary et Packed*Array : un widget par élément (accès par chemin), ajout / suppression /
// réordonnancement (Array, Packed), nouvelle clé (Dictionary). Chaque opération est UNE écriture de la
// collection complète, donc une seule action d'annulation. Typage lu dans le hint_string
// ("type[/hint]:hint_string", "clé;valeur" pour un Dictionary) ; sans typage, éléments Variant.
class CollectionWidget : public ContainerWidget
{
    GDCLASS(CollectionWidget, ContainerWidget)

  public:
    // Déclaration d'un type d'élément.
    struct ElementDecl
    {
        bool typed = false;
        Variant::Type type = Variant::NIL;
        int hint = 0;
        String hint_string;
        String class_name;
    };
    static ElementDecl parse_decl(const String &p_decl);
    static Variant::Type packed_element_type(Variant::Type p_packed_type);
    static Dictionary element_info(const ElementDecl &p_decl, const String &p_name);
    static Variant default_value(const ElementDecl &p_decl);

  private:
    Variant::Type collection_type = Variant::NIL;
    ElementDecl element_decl; // Array / Packed : élément ; Dictionary : valeur
    ElementDecl key_decl;     // Dictionary : clé
    LineEdit *new_key = nullptr;

    Variant _current_copy() const;
    void _on_move(int64_t p_from, int64_t p_to);
    void _on_remove(const Variant &p_key);
    void _on_add();
    void _on_create();

  protected:
    static void _bind_methods() {}
    Control *build_editor() override;
    String summary(const Variant &p_value) const override;
    String structure_key(const Variant &p_value) const override;
    void fill(const Variant &p_value) override;
};

// Object (et NodePath déplié) : affiche les propriétés du sous-objet (registre automatique, comme le
// ProceduralEditor). Anti-cycle : un objet déjà déplié plus haut n'est pas redéplié. Objet nul :
// « Nouveau… » (classes natives instanciables via ClassDB, class_name via la liste des classes globales ;
// pas pour les nœuds, qui se choisissent avec le sélecteur). « Effacer » remet la valeur à null.
class ObjectWidget : public ContainerWidget
{
    GDCLASS(ObjectWidget, ContainerWidget)

  private:
    OptionButton *class_choice = nullptr;
    ObjectID watched_id; // objet dont on écoute property_list_changed

    Object *_resolve(const Variant &p_value) const;
    bool _is_path_mode() const;
    PackedStringArray _base_classes() const;
    void _watch(Object *p_object);
    void _on_property_list_changed();
    void _on_create();
    void _on_clear();

  protected:
    static void _bind_methods() {}
    void _notification(int p_what);
    String summary(const Variant &p_value) const override;
    String structure_key(const Variant &p_value) const override;
    void fill(const Variant &p_value) override;

  public:
    // Classes instanciables (natives et class_name) héritant de l'une des classes de p_bases, hors nœuds.
    static PackedStringArray instantiable_classes(const PackedStringArray &p_bases);
    // Instancie une classe native ou un class_name (Variant nul en cas d'échec).
    static Variant instantiate_class(const String &p_class_name);
};
} // namespace godot

#endif // CONTAINER_WIDGETS_HPP
