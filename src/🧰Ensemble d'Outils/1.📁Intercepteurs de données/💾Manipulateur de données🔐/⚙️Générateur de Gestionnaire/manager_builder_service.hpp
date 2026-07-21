// manager_builder.h
#pragma once

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <type_traits>

namespace godot
{

  // Retourne l'instance singleton existante du type T dans la scène,
  // ou en crée une nouvelle si elle n'existe pas.
  template <typename T>
    requires std::derived_from<T, Node>
  static T *make_getter(T *&instance);

  // Initialise le singleton : détruit les doublons ou s'enregistre comme
  // instance unique. À appeler dans _EnterTree() ou _Ready() du manager.
  template <typename T>
  static void init_instance(T *&instance, T *caller)
  {
    static_assert(std::is_base_of_v<godot::Node, T>::value,
                  "T doit hériter de godot::Node");

    // Un doublon existe déjà → on détruit l'appelant
    if (instance != nullptr && instance != caller)
    {
      caller->queue_free();
    }
    else
    {
      // On s'enregistre comme instance canonique
      instance = caller;
    }
  }
}
// namespace godot