#include "scene_manager.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/⚙️Générateur de Gestionnaire/manager_builder_access.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

ObjectID SceneManager::instance_id;

namespace
{
const char32_t *SCENE_MANAGER_ICON = U"res://🖼️Editor Icons/🧭🌍/Scene Manager Icon.png";
}

void SceneManager::_log(const String &p_message) const
{
    if (log_activated)
    {
        log_it(p_message, this, SCENE_MANAGER_ICON);
    }
}

// 🛠️ Singleton et cycle de vie

SceneManager *SceneManager::get_instance() { return manager_builder::get<SceneManager>(instance_id); }

void SceneManager::_enter_tree()
{
    if (!manager_builder::init(instance_id, this))
    {
        return; // doublon : détruit par ManagerBuilder
    }
    // À chaque changement de scène effectif, load_finished notifie les abonnés
    get_tree()->connect("scene_changed", callable_mp(this, &SceneManager::_on_scene_changed));
}

void SceneManager::_exit_tree()
{
    SceneTree *tree = get_tree();
    const Callable on_scene_changed = callable_mp(this, &SceneManager::_on_scene_changed);
    if (tree->is_connected("scene_changed", on_scene_changed))
    {
        tree->disconnect("scene_changed", on_scene_changed);
    }
    manager_builder::release(instance_id, this);
}

void SceneManager::_ready()
{
    // _process ne tourne que pendant un chargement
    set_process(loading);
}

// 🚀 API publique

void SceneManager::load_scene_async(const String &p_scene_path)
{
    if (p_scene_path.is_empty())
    {
        _log(String(U"[color=red]⚠️ Aucune scène fournie pour le chargement ![/color]"));
        return;
    }

    // Un seul chargement à la fois
    if (loading)
    {
        _log(String(U"[color=yellow]⚠️ Chargement déjà en cours - Requête ignorée[/color]"));
        return;
    }

    // Requête de chargement en arrière-plan
    const Error error = ResourceLoader::get_singleton()->load_threaded_request(p_scene_path, "PackedScene");
    if (error != OK)
    {
        _log(String(U"[color=red]❌ Erreur au démarrage du chargement de ") + p_scene_path + " : " + itos(error) + "[/color]");
        return;
    }

    scene_path = p_scene_path;
    loading = true;
    progress_percent = 0.0f;
    set_process(true);
    emit_signal("loading_started");
    _log(String(U"[color=cyan]🚀 Chargement asynchrone démarré : ") + scene_path + "[/color]");
}

// 🔄 Suivi de progression

void SceneManager::_process(double /*p_delta*/)
{
    if (!loading)
    {
        set_process(false);
        return;
    }

    // Reçoit la progression (0.0 à 1.0) : Array partagé par référence
    Array progress;
    ResourceLoader *loader = ResourceLoader::get_singleton();
    const ResourceLoader::ThreadLoadStatus status = loader->load_threaded_get_status(scene_path, progress);

    switch (status)
    {
    case ResourceLoader::THREAD_LOAD_IN_PROGRESS:
    {
        progress_percent = progress.is_empty() ? 0.0f : (float)progress[0] * 100.0f;
        _log(String(U"[color=white]⏳ Progression : ") + String::num(progress_percent, 1) + "%[/color]");
        emit_signal("loading_progress", progress_percent);
        break;
    }
    case ResourceLoader::THREAD_LOAD_LOADED:
    {
        Ref<PackedScene> packed_scene = loader->load_threaded_get(scene_path);
        if (packed_scene.is_valid())
        {
            progress_percent = 100.0f;
            emit_signal("loading_progress", progress_percent);
            _log(String(U"[color=lime]✅ Chargement terminé ! Changement vers : ") + scene_path + "[/color]");
            // Changement de scène après la frame courante, pour éviter les conflits d'état
            awaiting_scene_change = true;
            get_tree()->call_deferred("change_scene_to_packed", packed_scene);
        }
        else
        {
            _log(String(U"[color=red]❌ La ressource chargée n'est pas une scène : ") + scene_path + "[/color]");
        }
        _finish_loading();
        break;
    }
    case ResourceLoader::THREAD_LOAD_FAILED:
        _log(String(U"[color=red]❌ Échec du chargement : ") + scene_path + "[/color]");
        // Récupérer le résultat (nul) libère la tâche de chargement, sinon conservée jusqu'à la fermeture
        loader->load_threaded_get(scene_path);
        _finish_loading();
        break;
    case ResourceLoader::THREAD_LOAD_INVALID_RESOURCE:
        _log(String(U"[color=red]⚠️ Ressource invalide : ") + scene_path + "[/color]");
        _finish_loading();
        break;
    }
}

void SceneManager::_finish_loading()
{
    loading = false;
    set_process(false);
}

void SceneManager::_on_scene_changed()
{
    // Seuls les changements demandés par ce gestionnaire concluent un chargement
    if (!awaiting_scene_change)
    {
        return;
    }
    awaiting_scene_change = false;
    emit_signal("load_finished");
}

// 🔗 Liaisons

void SceneManager::_bind_methods()
{
    ClassDB::bind_static_method("SceneManager", D_METHOD("get_instance"), &SceneManager::get_instance);
    ClassDB::bind_method(D_METHOD("load_scene_async", "scene_path"), &SceneManager::load_scene_async);
    ClassDB::bind_method(D_METHOD("is_loading"), &SceneManager::is_loading);
    ClassDB::bind_method(D_METHOD("get_progress_percent"), &SceneManager::get_progress_percent);
    ClassDB::bind_method(D_METHOD("set_log_activated", "activated"), &SceneManager::set_log_activated);
    ClassDB::bind_method(D_METHOD("get_log_activated"), &SceneManager::get_log_activated);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "log_activated"), "set_log_activated", "get_log_activated");

    ADD_SIGNAL(MethodInfo("loading_started"));
    ADD_SIGNAL(MethodInfo("loading_progress", PropertyInfo(Variant::FLOAT, "percent")));
    ADD_SIGNAL(MethodInfo("load_finished"));
}
