#ifndef SCENE_MANAGER_HPP
#define SCENE_MANAGER_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// 🎬 Gestionnaire de scènes asynchrone (singleton via ManagerBuilder).
///
/// Charge une scène en arrière-plan (ResourceLoader.load_threaded_request) puis en fait la scène
/// courante. Ses signaux permettent de piloter une UI de chargement :
///   loading_started -> loading_progress(percent) ... -> load_finished
/// Utilisable comme Autoload ou nœud placé en scène ; accès global par SceneManager.get_instance().
class SceneManager : public Node
{
    GDCLASS(SceneManager, Node)

  public:
    static SceneManager *get_instance();

    void _enter_tree() override;
    void _exit_tree() override;
    void _ready() override;
    void _process(double p_delta) override;

    /// Démarre le chargement asynchrone de la scène p_scene_path (ignoré si un chargement est en cours).
    void load_scene_async(const String &p_scene_path);

    bool is_loading() const { return loading; }
    /// Progression du chargement en cours, de 0 à 100.
    float get_progress_percent() const { return progress_percent; }

    void set_log_activated(bool p_activated) { log_activated = p_activated; }
    bool get_log_activated() const { return log_activated; }

  protected:
    static void _bind_methods();

  private:
    static ObjectID instance_id;

    String scene_path;
    bool loading = false;
    // Vrai entre le changement de scène demandé et son application : seul celui-ci émet load_finished.
    bool awaiting_scene_change = false;
    float progress_percent = 0.0f;
    bool log_activated = false;

    void _finish_loading();
    void _on_scene_changed();
    void _log(const String &p_message) const;
};
} // namespace godot

#endif // SCENE_MANAGER_HPP
