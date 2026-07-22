#ifndef MANAGER_BUILDER_HPP
#define MANAGER_BUILDER_HPP

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
class ManagerBuilder {
public:
  /// Retourne l'instance singleton existante du type T dans la scène,
  /// ou en crée une nouvelle si elle n'existe pas.
  /// NOTE: contrairement au C# (réflexion + new()), T est ici connu
  /// statiquement à la compilation : memnew(T) suffit, pas besoin de
  /// ClassDB::instantiate() par nom de classe.
  template <typename T> static T *make_getter(T *&p_instance) {
    // Si l'instance existe déjà, on la retourne directement
    if (p_instance != nullptr) {
      return p_instance;
    }

    SceneTree *scene_tree =
        Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (scene_tree == nullptr) {
      print_line_rich("[color=red]⚠️ ManagerBuilder: SceneTree introuvable "
                      "(main loop invalide)[/color]");
      return nullptr;
    }

    Window *root = scene_tree->get_root();

    // Recherche par nom de classe dans l'arbre existant
    // (fragile comme en C# : deux nœuds renommés au même nom collisionnent
    // silencieusement)
    Node *found = root->find_child(T::get_class_static(), true, false);
    p_instance = Object::cast_to<T>(found);

    // Si elle n'existe toujours pas, on en crée une nouvelle
    if (p_instance == nullptr) {
      T *singleton_object = memnew(T);
      p_instance = singleton_object;
      singleton_object->set_name(String(T::get_class_static()) +
                                 " (Singleton)");

      print_line_rich(
          vformat("[color=green]Un Singleton de %s a été créé ![/color]",
                  T::get_class_static()));

      // Ajout différé à la racine pour éviter les problèmes de cycle de vie
      root->call_deferred("add_child", singleton_object);
    }

    return p_instance;
  }

  /// <summary>
  /// Initialise le singleton : détruit les doublons ou s'enregistre comme
  /// instance unique. À appeler dans _enter_tree() ou _ready() du manager.
  /// </summary>
  template <typename T> static void init_instance(T *&p_instance, T *p_caller) {
    // Un doublon existe déjà → on détruit l'appelant
    if (p_instance != nullptr && p_instance != p_caller) {
      p_caller->queue_free();
    } else {
      // On s'enregistre comme instance canonique
      p_instance = p_caller;
    }
  }
};
} // namespace godot

#endif // MANAGER_BUILDER_HPP