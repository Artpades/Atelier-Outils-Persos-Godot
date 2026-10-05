#ifndef SIGNAL_WIDGET_HPP
#define SIGNAL_WIDGET_HPP

#include "property_widget.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/signal_link.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot
{
// Signal (choix « Button ») : liste de liens « signal → méthode » à la manière d'un UnityEvent.
// Deux modes, choisis automatiquement et affichés :
//  - NATIF (éditeur, émetteur ET cible = nœuds de la scène éditée) : connexion CONNECT_PERSIST enregistrée
//    dans la scène, Callable(cible, méthode).bind(constantes...).unbind(S - k). Seule correspondance
//    possible : les k premiers paramètres reçoivent les k premiers arguments du signal, les autres sont des
//    constantes (contrainte de bind / unbind, expliquée dans l'interface).
//  - RESSOURCE (tous les autres cas) : SignalLink enregistrés dans DataEntry::links ; correspondance libre
//    (ArgumentBinding par paramètre : constante, propriété, argument du signal).
// Toute modification est UNE action d'annulation (UndoBackend::commit_calls).
class SignalWidget : public PropertyWidget
{
    GDCLASS(SignalWidget, PropertyWidget)

  private:
    VBoxContainer *links_box = nullptr; // enfants possédés
    // Liens affichés : Dictionary { "link": SignalLink, "native": bool, "callable": Callable (natif connecté), "k": int }
    Array items;
    Array native_drafts; // liens natifs pas encore complets (pas de méthode) : non connectés
    Variant shown_fingerprint;

    Object *_source() const;
    StringName _signal() const;
    Array _signal_args() const;
    bool _native_mode() const; // émetteur dans la scène éditée (éditeur)
    Dictionary _method_info(Object *p_target, const StringName &p_method) const;
    void _adapt_arguments(const Ref<SignalLink> &p_link, const Dictionary &p_method_info, bool p_native, int p_k) const;
    Ref<UndoBackend> _backend() const;
    void _commit(const String &p_name, const Array &p_do, const Array &p_undo);
    void _reapply_resource_links();

    void _rebuild();
    void _rebuild_deferred();
    void _collect_items();
    Control *_build_item(int p_index);
    void _build_native_parameters(VBoxContainer *p_box, int p_index, const Dictionary &p_method_info);
    void _build_resource_parameters(VBoxContainer *p_box, int p_index, const Dictionary &p_method_info);
    PropertyWidget *_build_constant(const Ref<ArgumentBinding> &p_binding, const Dictionary &p_param, bool p_undoable);

    void _commit_native(int64_t p_index);
    // Affectations [objet, propriété, valeur] en UNE action annulable.
    void _commit_sets(const Array &p_sets);

    void _on_emit();
    void _on_add();
    void _on_remove(int64_t p_index);
    void _on_enabled_toggled(bool p_on, int64_t p_index);
    void _on_target_submitted(const String &p_text, int64_t p_index);
    void _on_pick_node(int64_t p_index);
    void _on_node_picked(const NodePath &p_path, int64_t p_index);
    void _on_runtime_node_picked(Node *p_node, int64_t p_index);
    void _apply_target_node(Node *p_node, int64_t p_index);
    void _on_pick_resource(int64_t p_index);
    void _on_resource_picked(const String &p_path, int64_t p_index);
    void _on_method_selected(int64_t p_item, int64_t p_index, uint64_t p_option_id);
    void _on_pick_method(int64_t p_index);
    void _on_method_picked(const String &p_method, int64_t p_index);
    void _set_method(const StringName &p_method, int64_t p_index);
    void _on_k_changed(double p_value, int64_t p_index);
    void _on_native_constant_changed(const Variant &p_value, bool p_committing, int64_t p_index);
    void _on_source_selected(int64_t p_item, int64_t p_index, int64_t p_param, uint64_t p_option_id);
    void _on_property_path_submitted(const String &p_text, int64_t p_index, int64_t p_param, bool p_object_part);

  protected:
    static void _bind_methods() {}
    bool has_own_title() const override { return true; }
    Control *build_editor() override;
    void pull_value(const Variant &p_value) override;

  public:
    // Empreinte des liens (connexions natives + liens ressource) : suivie par le SyncController pour
    // refléter les changements externes (annulation dans l'éditeur...).
    Variant read_value() const override;

    // Vrai si un lien natif est possible entre ces objets (éditeur, nœuds de la scène éditée).
    static bool is_native_eligible(Object *p_source, Object *p_target);
    // Callable(cible, méthode).bind(constantes...).unbind(p_unbinds).
    static Callable make_native_callable(Object *p_target, const StringName &p_method, const Array &p_constants, int p_unbinds);
    // Plus grand k tel que les k premiers arguments du signal alimentent les k premiers paramètres.
    static int max_native_prefix(const Array &p_signal_args, const Array &p_method_args);
};
} // namespace godot

#endif // SIGNAL_WIDGET_HPP
