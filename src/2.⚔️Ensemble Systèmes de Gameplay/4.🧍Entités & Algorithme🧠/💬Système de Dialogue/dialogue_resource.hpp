#ifndef DIALOGUE_RESOURCE_HPP
#define DIALOGUE_RESOURCE_HPP

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// 💬 Nœud d'un dialogue (classe de base, non instanciable seule) : identifié par id, relié aux autres par
/// leur id (next...). Sous-classes : DialogueLine, DialogueChoice, DialogueBranch, DialogueAction, DialogueEnd.
class DialogueNode : public Resource
{
    GDCLASS(DialogueNode, Resource)

  public:
    void set_node_id(const StringName &p_id) { node_id = p_id; emit_changed(); }
    StringName get_node_id() const { return node_id; }
    void set_comment(const String &p_comment) { comment = p_comment; emit_changed(); }
    String get_comment() const { return comment; }

    /// Liens sortants : [{ "to": id, "label": texte, "slot": numéro }] (le slot identifie le lien à modifier).
    virtual Array get_links() const { return Array(); }
    /// Remplace la cible du lien n° p_slot (édition du graphe). p_slot = -1 : lien « suivant » par défaut.
    virtual void set_link(int /*p_slot*/, const StringName & /*p_to*/) {}
    /// Catégorie affichée dans le graphe et texte court.
    virtual String get_graph_kind() const { return "action"; }
    virtual String get_summary() const { return String(); }

  protected:
    static void _bind_methods();

  private:
    StringName node_id;
    String comment;
};

/// 🗨️ Réplique : un personnage dit un texte (BBCode, variables « {nom} »), puis passe à next.
class DialogueLine : public DialogueNode
{
    GDCLASS(DialogueLine, DialogueNode)

  public:
    void set_speaker(const String &p_speaker) { speaker = p_speaker; emit_changed(); }
    String get_speaker() const { return speaker; }
    void set_text(const String &p_text) { text = p_text; emit_changed(); }
    String get_text() const { return text; }
    void set_portrait(const Ref<Texture2D> &p_portrait) { portrait = p_portrait; emit_changed(); }
    Ref<Texture2D> get_portrait() const { return portrait; }
    void set_voice(const Ref<AudioStream> &p_voice) { voice = p_voice; emit_changed(); }
    Ref<AudioStream> get_voice() const { return voice; }
    void set_auto_advance_time(double p_time) { auto_advance_time = MAX(0.0, p_time); emit_changed(); }
    double get_auto_advance_time() const { return auto_advance_time; }
    void set_next(const StringName &p_next) { next = p_next; emit_changed(); }
    StringName get_next() const { return next; }

    Array get_links() const override;
    void set_link(int p_slot, const StringName &p_to) override;
    String get_graph_kind() const override { return "state"; }
    String get_summary() const override;

  protected:
    static void _bind_methods();

  private:
    String speaker;
    String text;
    Ref<Texture2D> portrait;
    Ref<AudioStream> voice;
    double auto_advance_time = 0.0;
    StringName next;
};

/// ☑️ Option d'un DialogueChoice : texte, nœud suivant, condition d'affichage, option à usage unique.
class DialogueOption : public Resource
{
    GDCLASS(DialogueOption, Resource)

  public:
    void set_text(const String &p_text) { text = p_text; emit_changed(); }
    String get_text() const { return text; }
    void set_next(const StringName &p_next) { next = p_next; emit_changed(); }
    StringName get_next() const { return next; }
    void set_condition(const String &p_condition) { condition = p_condition; emit_changed(); }
    String get_condition() const { return condition; }
    void set_once_only(bool p_once) { once_only = p_once; emit_changed(); }
    bool get_once_only() const { return once_only; }

  protected:
    static void _bind_methods();

  private:
    String text;
    StringName next;
    String condition;
    bool once_only = false;
};

/// 🔀 Choix : propose les options disponibles (condition remplie, pas déjà prises si once_only).
/// Sans option disponible, passe à fallback_next (ou termine le dialogue).
class DialogueChoice : public DialogueNode
{
    GDCLASS(DialogueChoice, DialogueNode)

  public:
    void set_prompt(const String &p_prompt) { prompt = p_prompt; emit_changed(); }
    String get_prompt() const { return prompt; }
    void set_options(const TypedArray<DialogueOption> &p_options) { options = p_options; emit_changed(); }
    TypedArray<DialogueOption> get_options() const { return options; }
    void set_fallback_next(const StringName &p_next) { fallback_next = p_next; emit_changed(); }
    StringName get_fallback_next() const { return fallback_next; }

    Array get_links() const override;
    void set_link(int p_slot, const StringName &p_to) override;
    String get_graph_kind() const override { return "composite"; }
    String get_summary() const override;

  protected:
    static void _bind_methods();

  private:
    String prompt;
    TypedArray<DialogueOption> options;
    StringName fallback_next;
};

/// ❔ Embranchement : va à next_if_true ou next_if_false selon une condition sur les variables.
class DialogueBranch : public DialogueNode
{
    GDCLASS(DialogueBranch, DialogueNode)

  public:
    void set_condition(const String &p_condition) { condition = p_condition; emit_changed(); }
    String get_condition() const { return condition; }
    void set_next_if_true(const StringName &p_next) { next_if_true = p_next; emit_changed(); }
    StringName get_next_if_true() const { return next_if_true; }
    void set_next_if_false(const StringName &p_next) { next_if_false = p_next; emit_changed(); }
    StringName get_next_if_false() const { return next_if_false; }

    Array get_links() const override;
    void set_link(int p_slot, const StringName &p_to) override;
    String get_graph_kind() const override { return "condition"; }
    String get_summary() const override { return condition; }

  protected:
    static void _bind_methods();

  private:
    String condition;
    StringName next_if_true;
    StringName next_if_false;
};

/// ⚙️ Action : modifie des variables (« or += 10 », « ami = true ») et/ou émet un événement de dialogue,
/// puis passe à next.
class DialogueAction : public DialogueNode
{
    GDCLASS(DialogueAction, DialogueNode)

  public:
    void set_assignments(const PackedStringArray &p_assignments) { assignments = p_assignments; emit_changed(); }
    PackedStringArray get_assignments() const { return assignments; }
    void set_event(const StringName &p_event) { event = p_event; emit_changed(); }
    StringName get_event() const { return event; }
    void set_event_arguments(const Array &p_arguments) { event_arguments = p_arguments; emit_changed(); }
    Array get_event_arguments() const { return event_arguments; }
    void set_next(const StringName &p_next) { next = p_next; emit_changed(); }
    StringName get_next() const { return next; }

    Array get_links() const override;
    void set_link(int p_slot, const StringName &p_to) override;
    String get_graph_kind() const override { return "action"; }
    String get_summary() const override;

  protected:
    static void _bind_methods();

  private:
    PackedStringArray assignments;
    StringName event;
    Array event_arguments;
    StringName next;
};

/// 🏁 Fin du dialogue.
class DialogueEnd : public DialogueNode
{
    GDCLASS(DialogueEnd, DialogueNode)

  public:
    String get_graph_kind() const override { return "machine"; }
    String get_summary() const override { return U"fin"; }

  protected:
    static void _bind_methods() {}
};

/// 📜 Dialogue complet : liste de nœuds et identifiant du nœud de départ (vide : le premier).
class DialogueResource : public Resource
{
    GDCLASS(DialogueResource, Resource)

  public:
    void set_nodes(const TypedArray<DialogueNode> &p_nodes) { nodes = p_nodes; emit_changed(); }
    TypedArray<DialogueNode> get_nodes() const { return nodes; }
    void set_start_id(const StringName &p_id) { start_id = p_id; emit_changed(); }
    StringName get_start_id() const { return start_id; }

    /// Nœud d'identifiant p_id (recherche linéaire, premier trouvé), nul si absent ou si p_id est vide.
    Ref<DialogueNode> get_dialogue_node(const StringName &p_id) const;
    bool has_dialogue_node(const StringName &p_id) const { return get_dialogue_node(p_id).is_valid(); }
    /// Nœud de start_id, ou le premier nœud si start_id est vide.
    Ref<DialogueNode> get_start_node() const;
    /// Identifiant libre de la forme « prefixe_N ».
    StringName generate_id(const String &p_prefix) const;
    /// Problèmes détectés : identifiants vides ou en double, liens cassés, départ introuvable.
    PackedStringArray validate() const;
    /// Instantané pour BehaviorGraphView : arbre de parcours depuis le départ (les autres liens en arcs).
    Dictionary get_graph_snapshot(const StringName &p_current = StringName(), const Dictionary &p_visited = Dictionary()) const;

  protected:
    static void _bind_methods();

  private:
    TypedArray<DialogueNode> nodes;
    StringName start_id;
};
} // namespace godot

#endif // DIALOGUE_RESOURCE_HPP
