#ifndef INPUT_EDITOR_PLUGIN_HPP
#define INPUT_EDITOR_PLUGIN_HPP

#include <godot_cpp/classes/editor_debugger_plugin.hpp>
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot
{
/// Bouton « 🕹️ Éditeur d'entrées » en tête de l'inspecteur des PlayerInput et des InputContext.
class InputInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(InputInspectorPlugin, EditorInspectorPlugin)

  public:
    bool _can_handle(Object *p_object) const override;
    void _parse_begin(Object *p_object) override;

  protected:
    static void _bind_methods() {}

  private:
    void _on_open_pressed(uint64_t p_object_id);
};

/// Réception, dans l'éditeur, des messages "input:*" du jeu en cours (voir InputDebugger) et envoi des
/// commandes de débogage (observation, simulation, activation de contexte).
class InputDebuggerPlugin : public EditorDebuggerPlugin
{
    GDCLASS(InputDebuggerPlugin, EditorDebuggerPlugin)

  public:
    InputDebuggerPlugin();
    ~InputDebuggerPlugin();
    static InputDebuggerPlugin *get_singleton() { return singleton; }
    static void detach() { singleton = nullptr; }

    bool _has_capture(const String &p_capture) const override;
    bool _capture(const String &p_message, const Array &p_data, int32_t p_session_id) override;
    void _setup_session(int32_t p_session_id) override;

    void set_watched(const PackedStringArray &p_paths);
    Dictionary get_snapshot(const String &p_path) const { return snapshots.get(p_path, Dictionary()); }
    /// PlayerInput du jeu : [ [chemin, nom], ... ].
    Array get_players() const { return players; }
    bool has_active_session() const { return !active_sessions.is_empty(); }
    void send_simulate(const String &p_path, const StringName &p_action, const Variant &p_value);
    void send_set_context(const String &p_path, const StringName &p_context, bool p_enabled);

  protected:
    static void _bind_methods() {}

  private:
    static InputDebuggerPlugin *singleton;
    Dictionary snapshots;
    Array players;
    PackedStringArray watched;
    PackedInt32Array active_sessions;

    void _send(const String &p_message, const Array &p_data);
    void _on_session_started(int32_t p_session_id);
    void _on_session_stopped(int32_t p_session_id);
};

/// Plugin d'éditeur du module Entrées de périphérique : bouton de l'inspecteur et débogueur.
class InputEditorPlugin : public EditorPlugin
{
    GDCLASS(InputEditorPlugin, EditorPlugin)

  public:
    void _enter_tree() override;
    void _exit_tree() override;

  protected:
    static void _bind_methods() {}

  private:
    Ref<InputInspectorPlugin> inspector_plugin;
    Ref<InputDebuggerPlugin> debugger_plugin;
};
} // namespace godot

#endif // INPUT_EDITOR_PLUGIN_HPP
