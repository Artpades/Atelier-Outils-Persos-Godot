#ifndef PROPERTY_WIDGET_HPP
#define PROPERTY_WIDGET_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/data_entry.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔌Accès/value_accessor.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔧Réglages/widget_settings.hpp"
#include "widget_context.hpp"

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace godot
{
// Ligne d'édition d'une valeur : libellé + éditeur. Le widget n'écrit JAMAIS dans l'objet : il émet
// value_changed(value, committing) et le ProceduralEditor fait passer l'écriture par l'UndoBackend.
// committing=false pendant un glissement ou une frappe, true à la validation.
//
// Utilisée telle quelle, elle affiche la valeur en lecture seule (choix « Label »).
// Widget personnalisé (choix « Custom ») : hériter de PropertyWidget (en C++ ou en GDScript avec class_name)
// et surcharger _build(info, settings) -> Control et _pull(value), puis appeler emit_value() depuis les signaux.
class PropertyWidget : public BoxContainer
{
    GDCLASS(PropertyWidget, BoxContainer)

  private:
    Ref<ValueAccessor> accessor;
    Dictionary info;
    Ref<WidgetSettings> settings;
    DataEntry::WidgetChoice style = DataEntry::WIDGET_LABEL;
    bool read_only = false;
    bool pulling = false;
    // Édition commencée (emit_value avec committing=false) mais pas encore validée.
    bool editing = false;

    // Dernière valeur connue (lue dans l'objet), utilisée par le SyncController pour détecter les changements.
    Variant last_value;
    bool has_last = false;

    Label *value_label = nullptr; // enfant possédé (affichage par défaut)
    // Élément qui a reçu le style des réglages (le widget, sa valeur ou son libellé) ; fond dessiné si c'est le widget.
    ObjectID styled_item_id;
    Ref<StyleBox> background;
    // Icône (réglages, remplaçable par un comportement « Icône si »), libellé, verrouillage et infobulles.
    ObjectID icon_rect_id;
    ObjectID name_label_id;
    Ref<Texture2D> base_icon;
    bool locked = false;
    PackedStringArray tooltip_texts;

    static void _set_controls_editable(Node *p_node, bool p_editable);

    // Contexte de construction des widgets enfants (collections, objets...) ; nul pour un widget isolé.
    Ref<WidgetContext> context;
    int depth = 0;
    PackedInt64Array ancestors; // ObjectID des objets dépliés au-dessus (anti-cycle)
    Ref<DataEntry> entry;       // configuration d'origine (peut être nulle)

  protected:
    static void _bind_methods();
    void _notification(int p_what);

    GDVIRTUAL2R(Control *, _build, Dictionary, Ref<WidgetSettings>)
    GDVIRTUAL1(_pull, Variant)

    // Construit l'éditeur (sans le libellé). Par défaut : _build du script, sinon un Label en lecture seule.
    virtual Control *build_editor();
    // Affiche une valeur SANS émettre value_changed. Par défaut : _pull du script, sinon texte du Label.
    virtual void pull_value(const Variant &p_value);
    // Classe de réglages attendue (un réglage d'une autre classe ne garde que les réglages communs).
    virtual StringName get_settings_class() const { return "WidgetSettings"; }

    bool is_pulling() const { return pulling; }

    // Vrai si le widget affiche lui-même son titre (conteneurs repliables, méthodes) : pas de libellé à gauche.
    virtual bool has_own_title() const { return false; }

    // Construit un widget enfant (profondeur + 1) via le contexte ; nullptr si profondeur maximale atteinte.
    PropertyWidget *build_child(const Ref<ValueAccessor> &p_accessor, const Ref<DataEntry> &p_entry, const PackedInt64Array &p_ancestors) const;
    int get_depth() const { return depth; }
    PackedInt64Array get_ancestors() const { return ancestors; }
    Ref<WidgetContext> get_context() const { return context; }

  public:
    // Nom lisible d'une propriété (my_property → « My Property »).
    static String display_name(const StringName &p_property);
    // Texte d'une valeur pour un affichage en lecture seule.
    static String stringify(const Variant &p_value);

    // Entrée de configuration par défaut (widget AUTO) pour une valeur décrite par p_info.
    static Ref<DataEntry> make_default_entry(const Dictionary &p_info, DataEntry::MemberKind p_member = DataEntry::MEMBER_PROPERTY);

    // Contexte de construction (à donner avant setup).
    void set_context(const Ref<WidgetContext> &p_context, int p_depth, const PackedInt64Array &p_ancestors);
    void set_entry(const Ref<DataEntry> &p_entry) { entry = p_entry; }
    Ref<DataEntry> get_entry() const { return entry; }
    // Faux pour un widget sans valeur à synchroniser (ex: bouton de méthode).
    virtual bool is_value_widget() const { return true; }
    // Valeur suivie par le SyncController (par défaut : la valeur pointée par l'accesseur ; un widget de
    // signal suit l'empreinte de ses liens).
    virtual Variant read_value() const;

    // Construit la ligne. À appeler une seule fois, avant l'ajout à l'arbre.
    void setup(const Ref<ValueAccessor> &p_accessor, const Ref<WidgetSettings> &p_settings, DataEntry::WidgetChoice p_style, bool p_vertical);

    // Affiche p_value sans émettre value_changed (appelé par le SyncController).
    void pull(const Variant &p_value);
    // À appeler par l'éditeur quand l'utilisateur modifie la valeur. Ignoré pendant un pull ou en lecture seule.
    void emit_value(const Variant &p_value, bool p_committing);
    // Valide p_value seulement si une édition est en cours (perte du focus, fermeture d'un popup...) :
    // sans modification de l'utilisateur, rien n'est réécrit (jamais de valeur périmée réécrite après un undo).
    void commit_if_editing(const Variant &p_value);
    // Vrai entre une modification non validée (committing=false) et sa validation.
    bool has_uncommitted_edit() const;
    // Vrai si p_value diffère de la valeur actuelle de l'objet (même type et même contenu sinon).
    bool differs_from_object(const Variant &p_value) const;

    Ref<ValueAccessor> get_accessor() const;
    StringName get_property_name() const;
    Dictionary get_info() const;
    Ref<WidgetSettings> get_settings() const;
    // Élément stylé par settings->style (nul sans style) : le ProceduralEditor y relie les uniforms du shader.
    CanvasItem *get_styled_item() const;

    // Icône affichée : p_icon, ou celle des réglages si p_icon est nulle.
    void set_icon_override(const Ref<Texture2D> &p_icon);
    Ref<Texture2D> get_displayed_icon() const;
    // Verrouillage par un comportement (« Activer si » faux) : contrôles désactivés, aucune valeur émise.
    void set_locked(bool p_locked);
    bool is_locked() const;
    // Infobulles (BBCode, modèles {…}) affichées au survol par l'infobulle enrichie de l'éditeur. Vide :
    // infobulle simple de Godot (nom de la propriété).
    void set_tooltip_texts(const PackedStringArray &p_texts);
    PackedStringArray get_tooltip_texts() const;
    DataEntry::WidgetChoice get_style() const;
    bool is_read_only() const;

    // Vrai si un contrôle de ce widget a le focus clavier ET que sa fenêtre est active
    // (une fenêtre inactive garde son focus interne : l'utilisateur n'est alors pas en train d'éditer).
    bool has_edit_focus() const;

    bool has_last_value() const;
    Variant get_last_value() const;
    void set_last_value(const Variant &p_value);
};
} // namespace godot

#endif // PROPERTY_WIDGET_HPP
