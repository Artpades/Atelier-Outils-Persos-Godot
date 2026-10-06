#ifndef BEHAVIOR_EDITOR_PLUGIN_HPP
#define BEHAVIOR_EDITOR_PLUGIN_HPP

#include <godot_cpp/classes/editor_debugger_plugin.hpp>
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot
{
/// Bouton « Éditeur de comportement » en tête de l'inspecteur des StateMachine, State, BehaviorTree et BTNode,
/// et « Éditeur de dialogue » pour DialogueResource, DialogueSystem et NPCController.
class BehaviorInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(BehaviorInspectorPlugin, EditorInspectorPlugin)

  public:
    bool _can_handle(Object *p_object) const override;
    void _parse_begin(Object *p_object) override;

  protected:
    static void _bind_methods() {}

  private:
    void _on_open_pressed(uint64_t p_node_id);
    void _on_open_dialogue_pressed(uint64_t p_object_id);
};

/// Réception, dans l'éditeur, des messages "behavior:*" du jeu en cours (voir BehaviorDebugger) :
/// liste des comportements vivants et instantanés des comportements observés.
class BehaviorDebuggerPlugin : public EditorDebuggerPlugin
{
    GDCLASS(BehaviorDebuggerPlugin, EditorDebuggerPlugin)

  public:
    BehaviorDebuggerPlugin();
    ~BehaviorDebuggerPlugin();
    static BehaviorDebuggerPlugin *get_singleton() { return singleton; }
    /// Retiré du débogueur (fermeture de l'éditeur) : plus aucun message ne doit être envoyé.
    static void detach() { singleton = nullptr; }

    bool _has_capture(const String &p_capture) const override;
    bool _capture(const String &p_message, const Array &p_data, int32_t p_session_id) override;
    void _setup_session(int32_t p_session_id) override;

    /// Chemins (relatifs à la scène du jeu) dont le jeu doit envoyer les instantanés.
    void set_watched(const PackedStringArray &p_paths);
    /// Dernier instantané reçu pour un chemin (vide si aucun).
    Dictionary get_snapshot(const String &p_path) const;
    /// Comportements vivants du jeu : [ [chemin, classe, nom], ... ].
    Array get_runners() const { return runners; }
    /// Vrai si une session de jeu est en cours.
    bool has_active_session() const { return !active_sessions.is_empty(); }

  protected:
    static void _bind_methods() {}

  private:
    static BehaviorDebuggerPlugin *singleton;
    Dictionary snapshots;
    Array runners;
    PackedStringArray watched;
    // Sessions de jeu en cours, d'après leurs signaux started / stopped (is_active() échoue pendant la
    // fermeture de l'éditeur, quand la session survit à son débogueur).
    PackedInt32Array active_sessions;

    void _send_watch();
    void _on_session_started(int32_t p_session_id);
    void _on_session_stopped(int32_t p_session_id);
};

/// Plugin d'éditeur du système de comportement (niveau EDITOR) : bouton de l'inspecteur, débogueur du
/// jeu en cours, et suivi de la sélection par la fenêtre BehaviorEditorWindow.
class BehaviorEditorPlugin : public EditorPlugin
{
    GDCLASS(BehaviorEditorPlugin, EditorPlugin)

  public:
    void _enter_tree() override;
    void _exit_tree() override;

  protected:
    static void _bind_methods() {}

  private:
    Ref<BehaviorInspectorPlugin> inspector_plugin;
    Ref<BehaviorDebuggerPlugin> debugger_plugin;

    void _on_selection_changed();
};
} // namespace godot

#endif // BEHAVIOR_EDITOR_PLUGIN_HPP
