#ifndef PROCEDURAL_EDITOR_HPP
#define PROCEDURAL_EDITOR_HPP

#include "rich_tooltip.hpp"
#include "sync_controller.hpp"
#include "widget_factory.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/data_registry.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/group_registry.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/layout_database.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🎯Gizmos/gizmo_host.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/widget_context.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔌Accès/undo_backend.hpp"

#include <godot_cpp/classes/container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
class ProceduralEditor;

/// Contexte de construction des widgets imbriqués (collections, objets, Variant, méthodes) : délègue au
/// ProceduralEditor, qui configure le widget et le relie à la synchronisation et à l'annulation.
class ProceduralEditorContext : public WidgetContext
{
    GDCLASS(ProceduralEditorContext, WidgetContext)

  private:
    ObjectID editor_id;

  protected:
    static void _bind_methods() {}

  public:
    void set_editor(ProceduralEditor *p_editor);
    PropertyWidget *build_widget(const Ref<ValueAccessor> &p_accessor, const Ref<DataEntry> &p_entry, int p_depth, const PackedInt64Array &p_ancestors) override;
    int get_max_depth() const override;
    Ref<UndoBackend> get_undo_backend() const override;
    void links_changed() override;
    Ref<LayoutDatabase> get_layout_database() const override;
};

/// Interface d'édition générée pour un objet, utilisable en jeu comme dans l'éditeur.
/// Les membres affichés et leurs widgets viennent du DataRegistry (créé et synchronisé automatiquement si
/// aucun n'est fourni). Toute écriture passe par l'UndoBackend (RuntimeUndoBackend par défaut ;
/// l'éditeur injecte un EditorUndoBackend). Un seul SyncController met à jour les widgets.
/// Ctrl+Z / Ctrl+Maj+Z (ou Ctrl+Y) annulent / rétablissent quand la fenêtre a le focus (en jeu : historique
/// propre ; dans l'éditeur : celui de l'éditeur). Ils sont interceptés AVANT les champs de saisie, qui sinon
/// les consommeraient pour leur propre annulation de texte ; exception : une saisie de texte non validée
/// garde l'annulation de texte du champ.
class ProceduralEditor : public VBoxContainer
{
    GDCLASS(ProceduralEditor, VBoxContainer)

  public:
    /// Méta posée sur un raccourci Ctrl+Z / Ctrl+Maj+Z retransmis à l'éditeur : il ne doit pas être réintercepté.
    static constexpr const char *FORWARDED_SHORTCUT_META = "procedural_editor_forwarded";

  private:
    ObjectID target_id;
    /// Cible déclarée dans la scène (résolue au _ready en jeu ; dans l'éditeur, par la fenêtre de configuration).
    NodePath target_path;
    Ref<Resource> target_resource;
    Ref<DataRegistry> registry;
    /// Groupes (affichés à la place de leur première donnée) ; créé avec le registre s'il n'existe pas.
    Ref<GroupRegistry> group_registry;
    Ref<UndoBackend> undo_backend;
    Ref<ProceduralEditorContext> context;

    /// Interfaces par type : base utilisée (layout_database, sinon celle du projet) et registres ACTIFS pour
    /// la cible : ceux de l'éditeur s'ils lui ont été donnés, sinon ceux du TypeLayout de la cible, sinon des
    /// registres automatiques (interface générale).
    Ref<LayoutDatabase> layout_database;
    bool use_layouts = true;
    bool draw_gizmos = true;
    /// Style global : celui de l'éditeur, sinon celui de l'interface du type, sinon le style par défaut du projet.
    /// styles_enabled = false pour les éditeurs internes des fenêtres de configuration (thème de l'éditeur Godot).
    Ref<InterfaceStyle> style;
    bool styles_enabled = true;
    bool global_style_applied = false;
    Ref<StyleBox> global_background;
    /// Comportements : groupes construits (nom -> [ObjectID du nœud extérieur, ObjectID de la racine]),
    /// dernier état appliqué par cible (signature), éléments qui ont reçu un style conditionnel, état des
    /// comportements de script (« ObjectID:cible » -> actif).
    Dictionary group_nodes;
    Dictionary behavior_signatures;
    Dictionary behavior_styled;
    Dictionary script_behavior_states;
    RichTooltip *rich_tooltip = nullptr; // enfant interne possédé
    Ref<TypeLayout> active_layout;
    Ref<DataRegistry> active_registry;
    Ref<GroupRegistry> active_groups;
    GizmoHost *gizmo_host = nullptr; // enfant interne possédé : gizmos du type de l'objet affiché
    int max_depth = 4;
    Dictionary top_widgets; // nom de donnée -> ObjectID du widget de premier niveau
    Array applied_links;    // liens en mode ressource connectés par cet éditeur : [SignalLink, ObjectID de l'émetteur]
    Ref<WidgetFactory> factory;

    /// Enfants créés et possédés par l'éditeur.
    Label *status = nullptr;
    ScrollContainer *scroll = nullptr;
    VBoxContainer *content = nullptr;
    SyncController *sync = nullptr;

    void _clear_widgets();
    void _apply_global_style();
    void _evaluate_behaviors();
    Control *_behavior_target(const String &p_name, CanvasItem **r_style_item, Ref<InterfaceStyle> *r_base_style) const;
    void _resolve_registries(Object *p_target);
    void _update_gizmos();
    /// Conteneur d'un groupe (ajouté au contenu) ; r_root reçoit sa racine (titre + avertissements).
    /// p_parent : TabContainer d'un groupe en onglets (la racine devient une page, sans titre), sinon le contenu.
    Container *_make_group_container(const Ref<GroupEntry> &p_group, VBoxContainer *&r_root, Container *p_parent = nullptr);
    static Dictionary _find_method(Object *p_target, const StringName &p_method);
    static Dictionary _find_signal(Object *p_target, const StringName &p_signal);
    void _show_status(const String &p_text);
    void _on_value_changed(const Variant &p_value, bool p_committing, uint64_t p_widget_id);
    void _on_edit_discarded(uint64_t p_widget_id);
    void _on_target_lost();
    /// PropertyWidget contenant p_control (nullptr si aucun).
    PropertyWidget *_widget_of(Node *p_control) const;

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  public:
    ProceduralEditor();

    void _ready() override;
    void _input(const Ref<InputEvent> &p_event) override;

    /// Change la cible : synchronise le registre (créé s'il n'existe pas) puis reconstruit l'interface.
    void set_target(Object *p_target);
    Object *get_target() const;

    /// Cible déclarée : target_resource si renseignée, sinon le nœud au chemin target_path (relatif à cet éditeur).
    void set_target_path(const NodePath &p_path);
    NodePath get_target_path() const;
    void set_target_resource(const Ref<Resource> &p_resource);
    Ref<Resource> get_target_resource() const;
    Object *resolve_declared_target() const;

    /// Remplace le registre (resynchronisé avec la cible actuelle, puis interface reconstruite).
    void set_registry(const Ref<DataRegistry> &p_registry);
    Ref<DataRegistry> get_registry() const;
    void set_group_registry(const Ref<GroupRegistry> &p_groups);
    Ref<GroupRegistry> get_group_registry() const;

    /// Oublie les registres de l'éditeur (sans reconstruire) : la prochaine cible prendra l'interface de son type.
    void clear_registries();
    void set_layout_database(const Ref<LayoutDatabase> &p_database);
    Ref<LayoutDatabase> get_layout_database() const;
    /// Base réellement utilisée : layout_database, sinon la base du projet.
    Ref<LayoutDatabase> get_effective_layout_database() const;
    void set_use_layouts(bool p_use);
    bool get_use_layouts() const;
    void set_draw_gizmos(bool p_draw);
    bool get_draw_gizmos() const;
    void set_style(const Ref<InterfaceStyle> &p_style);
    Ref<InterfaceStyle> get_style() const;
    void set_styles_enabled(bool p_enabled);
    bool get_styles_enabled() const;
    Ref<InterfaceStyle> get_effective_style() const;
    /// Éléments dont des uniforms de shader sont liés (mis à jour en direct).
    int get_styled_count() const;
    /// Réévalue les comportements (fait aussi après chaque relecture et chaque reconstruction).
    void evaluate_behaviors();
    RichTooltip *get_rich_tooltip() const;
    Ref<TypeLayout> get_active_layout() const;
    Ref<DataRegistry> get_active_registry() const;
    Ref<GroupRegistry> get_active_group_registry() const;
    GizmoHost *get_gizmo_host() const;

    /// Profondeur maximale des valeurs imbriquées (collections, sous-objets).
    void set_max_depth(int p_depth);
    int get_max_depth() const;

    /// Crée, configure et relie (synchronisation, annulation) le widget d'une valeur ; nullptr au-delà de max_depth.
    PropertyWidget *build_widget(const Ref<ValueAccessor> &p_accessor, const Ref<DataEntry> &p_entry, int p_depth, const PackedInt64Array &p_ancestors);

    void set_undo_backend(const Ref<UndoBackend> &p_backend);
    Ref<UndoBackend> get_undo_backend(); // crée un RuntimeUndoBackend si aucun n'a été fourni

    void set_factory(const Ref<WidgetFactory> &p_factory);
    Ref<WidgetFactory> get_factory() const;

    /// Défilement vertical (désactivé : l'éditeur prend la hauteur de son contenu, pour s'intégrer dans une liste).
    void set_scroll_enabled(bool p_enabled);
    bool is_scroll_enabled() const;

    void set_sync_interval(double p_interval);
    double get_sync_interval() const;

    /// Reconstruit tous les widgets depuis le registre.
    void rebuild();
    /// Widget d'une propriété (nullptr si absente).
    PropertyWidget *get_widget(const StringName &p_property) const;

    bool undo();
    bool redo();

    /// Liens de signaux en mode ressource du registre : retire ceux déjà connectés par cet éditeur puis
    /// connecte ceux du registre à la cible. En jeu uniquement (rien ne se déclenche pendant l'édition).
    void reapply_links();
};
} // namespace godot

#endif // PROCEDURAL_EDITOR_HPP
