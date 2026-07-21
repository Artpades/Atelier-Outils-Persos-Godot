#ifndef WINDOW_EDITOR_H
#define WINDOW_EDITOR_H

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot
{

  class WindowEditor : public Resource
  {
    GDCLASS(WindowEditor, Resource)

  private:
    // --- Configuration (equivalent des @export GDScript) ---
    Ref<PackedScene> prefab_window;
    String window_title = "[Insérer un titre de fenêtre !]";
    Vector2i window_position_on_creation = Vector2i(400, 400);
    Vector2i window_size_on_creation = Vector2i(400, 400);
    bool always_on_top = true;
    bool transparent_bg = false;
    bool own_world_3d = true;

    // --- État interne ---
    bool is_window_opened = false;
    Window *_viewport_window = nullptr;

  protected:
    static void _bind_methods();

  public:
    WindowEditor();
    ~WindowEditor();

    // Getters / setters requis par ClassDB pour chaque propriété exportée.
    void set_prefab_window(const Ref<PackedScene> &p_scene);
    Ref<PackedScene> get_prefab_window() const;

    void set_window_title(const String &p_title);
    String get_window_title() const;

    void set_window_position_on_creation(const Vector2i &p_pos);
    Vector2i get_window_position_on_creation() const;

    void set_window_size_on_creation(const Vector2i &p_size);
    Vector2i get_window_size_on_creation() const;

    void set_always_on_top(bool p_value);
    bool get_always_on_top() const;

    void set_transparent_bg(bool p_value);
    bool get_transparent_bg() const;

    void set_own_world_3d(bool p_value);
    bool get_own_world_3d() const;

    bool get_is_window_opened() const;

    // Cycle de vie de la fenêtre.
    void open_window();
    void close_window();

    // Point d'extension virtuel (surchargeable côté C++ ou GDScript via
    // _bind_vararg).
    virtual void _on_open_window();

    // Retourne le Callable utilisé par le bouton outil de l'Inspecteur.
    Callable get_open_window_callable() const;
  };

} // namespace godot

#endif // WINDOW_EDITOR_H