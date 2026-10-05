#ifndef OBJECT_EDITOR_WINDOW_HPP
#define OBJECT_EDITOR_WINDOW_HPP

#include "procedural_editor.hpp"

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
// Fenêtre d'édition d'un objet, UNIQUE et réutilisée, en jeu (fenêtre de debug : open_for) comme dans
// l'éditeur (ouverture automatique à la sélection, gérée par le plugin). Interface : celle du type de l'objet
// (LayoutDatabase), sinon générale ; gizmos du type dessinés pour l'objet affiché.
// « Épingler » : la fenêtre garde son objet quand la sélection change (follow() est alors ignoré).
class ObjectEditorWindow : public Window
{
    GDCLASS(ObjectEditorWindow, Window)

  private:
    static ObjectID current_window_id;

    ProceduralEditor *editor = nullptr; // enfants possédés
    Label *target_label = nullptr;
    CheckBox *pin = nullptr;
    Panel *background = nullptr;

    void _build_ui(const Ref<UndoBackend> &p_backend);
    void _show(Object *p_target);
    void _on_close_requested();

  protected:
    static void _bind_methods();

  public:
    // Ouvre la fenêtre (ou la réutilise) sur p_target, même épinglée. p_backend : annulation à utiliser
    // (nul : historique propre au jeu).
    static ObjectEditorWindow *open_for(Object *p_target, const Ref<UndoBackend> &p_backend = Ref<UndoBackend>());
    // Fenêtre ouverte et non épinglée : affiche p_target. Retourne faux sinon.
    static bool follow(Object *p_target);
    static ObjectEditorWindow *get_current();
    static void close_current();

    bool is_pinned() const;
    void set_pinned(bool p_pinned);
    ProceduralEditor *get_editor() const;
};
} // namespace godot

#endif // OBJECT_EDITOR_WINDOW_HPP
