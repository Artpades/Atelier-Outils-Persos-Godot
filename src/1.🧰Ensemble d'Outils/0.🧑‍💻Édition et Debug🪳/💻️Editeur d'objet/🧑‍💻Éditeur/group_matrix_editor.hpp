#ifndef GROUP_MATRIX_EDITOR_HPP
#define GROUP_MATRIX_EDITOR_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/data_registry.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/group_registry.hpp"

#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot
{
// Onglet « Groupes » de la fenêtre de configuration (éditeur, port de GroupMatrixEditor C#) : un bloc par
// groupe (ordre, nom, conteneur VBox / HBox / Grid / Panel, colonnes, orientation, suppression) avec ses
// données (retrait, ajout ; une donnée n'appartient qu'à un groupe). Les noms qui ne désignent aucune donnée
// ou une donnée manquante (is_missing) sont signalés par ⚠. Émet "registry_changed" après chaque modification.
class GroupMatrixEditor : public VBoxContainer
{
    GDCLASS(GroupMatrixEditor, VBoxContainer)

  private:
    Ref<GroupRegistry> groups;
    Ref<DataRegistry> data;
    Dictionary style_open; // ObjectID (int) des groupes dont l'éditeur de style est déplié
    ObjectID context_id;   // objet dont on liste les membres (uniforms liés)
    String context_type;

    VBoxContainer *list = nullptr; // enfants possédés
    LineEdit *new_group_name = nullptr;

    Ref<GroupEntry> _group(int64_t p_index) const;
    bool _is_missing(const String &p_data_name) const;
    void _emit_changed();
    void _refresh_deferred();

    void _on_move(int64_t p_from, int64_t p_to);
    void _on_name_changed(const String &p_text, int64_t p_index);
    void _on_type_selected(int64_t p_item, int64_t p_index);
    void _on_columns_changed(double p_value, int64_t p_index);
    void _on_vertical_toggled(bool p_on, int64_t p_index);
    void _on_delete(int64_t p_index);
    void _on_style_toggled(bool p_open, int64_t p_group_id);
    void _on_style_changed();
    void _on_remove_member(const String &p_name, int64_t p_index);
    void _on_add_member(int64_t p_item, int64_t p_index, uint64_t p_option_id);
    void _on_remove_page(const String &p_name, int64_t p_index);
    void _on_add_page(int64_t p_item, int64_t p_index, uint64_t p_option_id);
    void _on_new_group();

  protected:
    static void _bind_methods();

  public:
    GroupMatrixEditor();

    void set_registries(const Ref<GroupRegistry> &p_groups, const Ref<DataRegistry> &p_data);
    void refresh();
    void set_context_object(Object *p_object);
    void set_context_type(const String &p_type);
};
} // namespace godot

#endif // GROUP_MATRIX_EDITOR_HPP
