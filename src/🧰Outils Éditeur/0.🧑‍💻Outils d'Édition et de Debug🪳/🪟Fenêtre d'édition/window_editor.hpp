#ifndef WINDOW_EDITOR_HPP
#define WINDOW_EDITOR_HPP

#include "godot_cpp/core/binder_common.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>
// Enumréation de modes de tweening à l'ouverture de la fenêtre.
enum WindowOpenningAnimationMode
{
    // Aucun effet d'ouverture, la fenêtre apparaît instantanément.
    O_NONE = 0,
    // Effet d'ouverture de type "popup", la fenêtre agrandit sa taille progressivement
    // et sa position va en direction du centre de l'écran (qui est en haut à gauche),
    //  comme si elle "popait" à l'écran.
    O_POPUP = 1,
    // Effet d'ouverture avec la largeur qui augemente progressivement puis la hauteur,
    //  comme si la fenêtre se "dépliait" depuis le centre de l'écran.
    O_NUMERIC_LIKE = 2
};

// Enumréation de modes de tweening à la fermeture de la fenêtre.
enum WindowClosingAnimationMode
{
    // Aucun effet de fermeture,
    C_NONE = 0,
    // Effet de fermeture de type "popup
    C_POPUP = 1,
    // Effet de fermeture avec la largeur qui diminue progressivement puis la hauteur,
    C_NUMERIC_LIKE = 2
};

namespace godot
{

// Une classe permettant de créer une fenêtre dans l'éditeur et de lui ajouter des éléments d'interfaces. Cette classe est un Resource, ce qui permet de la sauvegarder dans un fichier .tres et de la
// réutiliser dans d'autres projets.
class WindowEditor : public Resource
{
    GDCLASS(WindowEditor, Resource)
  private:
    // Fenêtre d'édition qui sera à créer et détruite à chaque ouverture/fermeture de la fenêtre d'édition
    Window *window = nullptr;
    String window_title = U"[Titre de la fenêtre]";
    Vector2i window_position;
    Vector2i window_size = Vector2i(400, 300); // Taille par défaut de la fenêtre

    Callable open_window_button;

    // Mode d'animation d'ouverture
    WindowOpenningAnimationMode open_anim_mode = WindowOpenningAnimationMode::O_NONE;
    // Mode d'animation de fermeture
    WindowClosingAnimationMode close_anim_mode = WindowClosingAnimationMode::C_NONE;

    Ref<Tween> close_tween;

  protected:
    static void _bind_methods();

  public:
    WindowEditor();
    ~WindowEditor();

    Window *get_window() const;
    void set_window(Window *value);

    String get_window_title() const;
    void set_window_title(const String &value);

    Vector2i get_window_position() const;
    void set_window_position(const Vector2i &value);

    Vector2i get_window_size() const;
    void set_window_size(const Vector2i &value);

    Callable get_open_window_button();
    void set_open_window_button(const Callable &value);

    WindowOpenningAnimationMode get_anim_open_mode() const;
    void set_anim_open_mode(WindowOpenningAnimationMode value);

    WindowClosingAnimationMode get_anim_close_mode() const;
    void set_anim_close_mode(WindowClosingAnimationMode value);

    // Ouvre la fenêtre d'édition
    void open_window();
    // Ferme la fenêtre d'édition
    void close_window();
    // Libère la fenêtre de la mémoire
    void free_window();
};

} // namespace godot

VARIANT_ENUM_CAST(WindowOpenningAnimationMode);
VARIANT_ENUM_CAST(WindowClosingAnimationMode);

#endif // WINDOW_EDITOR_HPP