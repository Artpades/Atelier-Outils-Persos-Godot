#ifndef LAYOUT_DATABASE_WINDOW_HPP
#define LAYOUT_DATABASE_WINDOW_HPP

#include "data_matrix_editor.hpp"
#include "gizmo_list_editor.hpp"
#include "group_matrix_editor.hpp"
#include "behavior_list_editor.hpp"
#include "interface_style_editor.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/layout_database.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧱Construction/procedural_editor.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// Fenêtre « Interfaces par type » (éditeur) : édite la LayoutDatabase du projet (réglage
/// gameplay_toolkit/object_editor/layout_database). À gauche les types ; à droite, pour le type choisi, ses
/// réglages (ProceduralEditor), puis les onglets Données / Groupes / Gizmos (mêmes éditeurs que les autres
/// fenêtres). Les membres d'un type sont lus sur une instance d'exemple (créée puis libérée).
/// Les modifications sont en mémoire jusqu'à « Enregistrer ». Une seule fenêtre ouverte à la fois.
class LayoutDatabaseWindow : public Window
{
    GDCLASS(LayoutDatabaseWindow, Window)

  private:
    static ObjectID current_window_id;

    Ref<LayoutDatabase> database;
    int selected = -1;

    /// Enfants possédés.
    Label *path_label = nullptr;
    Label *dirty_label = nullptr;
    Button *create_button = nullptr;
    VBoxContainer *type_list = nullptr;
    LineEdit *new_type = nullptr;
    VBoxContainer *details = nullptr;
    ProceduralEditor *layout_editor = nullptr;
    DataMatrixEditor *data_matrix = nullptr;
    GroupMatrixEditor *group_matrix = nullptr;
    GizmoListEditor *gizmo_list = nullptr;
    InterfaceStyleEditor *style_editor = nullptr;
    BehaviorListEditor *behavior_list = nullptr;

    Ref<TypeLayout> _layout(int p_index) const;
    void _build_ui();
    void _refresh_types();
    void _select(int64_t p_index);
    void _set_dirty(bool p_dirty);
    void _add_type(const String &p_type);

    void _on_create();
    void _on_save();
    void _on_add_typed();
    void _on_pick_type();
    void _on_type_picked(const String &p_type);
    void _on_remove_type(int64_t p_index);
    void _on_sync_members();
    void _on_changed();
    void _on_close_requested();

  protected:
    static void _bind_methods();

  public:
    static LayoutDatabaseWindow *open();
    static void close_current();
    /// Crée une instance d'exemple d'un type (classe native ou class_name) ; Variant nul si impossible.
    static Variant instantiate_sample(const String &p_type);
    Ref<LayoutDatabase> get_database() const;
};
} // namespace godot

#endif // LAYOUT_DATABASE_WINDOW_HPP
