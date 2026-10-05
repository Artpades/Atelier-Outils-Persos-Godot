#ifndef DATA_MATRIX_EDITOR_HPP
#define DATA_MATRIX_EDITOR_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/data_registry.hpp"

#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
// Tableau d'édition d'un DataRegistry (éditeur, port de DataMatrixEditor C#) : une ligne par entrée
// (ordre, nom, type, widget parmi les choix autorisés, classe Custom, vertical, affiché, suppression des
// entrées manquantes). Sous le tableau : les réglages (WidgetSettings) de l'entrée sélectionnée,
// choisis avec un EditorResourcePicker limité à la classe lue par le widget et édités avec un ProceduralEditor.
// Émet "registry_changed" après chaque modification.
class DataMatrixEditor : public VBoxContainer
{
    GDCLASS(DataMatrixEditor, VBoxContainer)

  private:
    Ref<DataRegistry> registry;
    int selected = -1;

    // Enfants créés et possédés par ce tableau.
    GridContainer *grid = nullptr;
    VBoxContainer *settings_panel = nullptr;

    // Réglages actuellement surveillés (signal "changed"), pour se déconnecter au changement de sélection.
    ObjectID watched_settings_id;
    // Objet dont on liste les membres pour les uniforms liés des styles (objet réel, sinon type d'exemple).
    ObjectID context_id;
    String context_type;

    Ref<DataEntry> _entry(int64_t p_index) const;
    void _emit_changed();
    void _clear(Node *p_container);
    void _watch_settings(const Ref<WidgetSettings> &p_settings);
    void _refresh_settings_panel();

    void _on_move(int64_t p_from, int64_t p_to);
    void _on_select(int64_t p_index);
    void _on_choice_selected(int64_t p_item, int64_t p_index, uint64_t p_option_id);
    void _on_custom_class_changed(const String &p_text, int64_t p_index);
    void _on_vertical_toggled(bool p_on, int64_t p_index);
    void _on_enabled_toggled(bool p_on, int64_t p_index);
    void _on_remove(int64_t p_index);
    void _on_remove_all_missing();
    void _on_settings_resource_changed(const Ref<Resource> &p_resource, int64_t p_index);
    void _on_settings_edited();
    void _set_tooltip(int64_t p_index, const String &p_tooltip);
    void _on_tooltip_submitted(const String &p_text, int64_t p_index);
    void _on_tooltip_focus_exited(int64_t p_index, uint64_t p_line_id);
    void _on_tooltip_text_changed(uint64_t p_edit_id, uint64_t p_preview_id);
    void _on_tooltip_edit_committed(int64_t p_index, uint64_t p_edit_id);
    void _fill_preview_menu(uint64_t p_menu_id);
    void _on_add_preview(int64_t p_id);
    void _on_preview_renamed(const String &p_text, int64_t p_index);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  public:
    DataMatrixEditor();

    void set_registry(const Ref<DataRegistry> &p_registry);
    Ref<DataRegistry> get_registry() const;
    void set_context_object(Object *p_object);
    void set_context_type(const String &p_type);
    // Classes d'aperçu proposées : fournies, puis class_name héritant de PreviewWidget.
    static PackedStringArray preview_types();
    // Reconstruit le tableau depuis le registre.
    void refresh();
};
} // namespace godot

#endif // DATA_MATRIX_EDITOR_HPP
