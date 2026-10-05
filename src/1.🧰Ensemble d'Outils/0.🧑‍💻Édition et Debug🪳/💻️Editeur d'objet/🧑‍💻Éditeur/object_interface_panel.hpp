#ifndef OBJECT_INTERFACE_PANEL_HPP
#define OBJECT_INTERFACE_PANEL_HPP

#include "data_matrix_editor.hpp"
#include "gizmo_list_editor.hpp"
#include "group_matrix_editor.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/layout_database.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🎯Gizmos/gizmo_host.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧱Construction/procedural_editor.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
// Panneau d'édition d'un objet ET de son interface (éditeur ; fenêtre de la barre d'outils) :
// - l'interface est celle du type de l'objet dans la base du projet (LayoutDatabase) ; s'il n'en a pas, une
//   nouvelle interface est préparée pour son type, ajoutée à la base seulement à l'enregistrement ;
// - onglets Aperçu (ProceduralEditor) / Données / Groupes / Gizmos (mêmes éditeurs que les autres fenêtres) ;
// - « Enregistrer » écrit la base (créée si besoin) ; les gizmos du type sont dessinés en direct.
// Une interface héritée d'une classe parente peut être copiée en une interface propre au type de l'objet.
class ObjectInterfacePanel : public VBoxContainer
{
    GDCLASS(ObjectInterfacePanel, VBoxContainer)

  private:
    ObjectID target_id;
    Ref<LayoutDatabase> database;
    Ref<TypeLayout> layout;
    String own_type;      // type le plus précis de la cible (class_name, sinon classe native)
    bool pending = false; // layout pas encore dans la base (ajouté à l'enregistrement)
    bool dirty = false;

    // Enfants possédés.
    Label *type_label = nullptr;
    Label *dirty_label = nullptr;
    Button *own_button = nullptr;
    TabContainer *tabs = nullptr;
    ProceduralEditor *preview = nullptr;
    DataMatrixEditor *data_matrix = nullptr;
    GroupMatrixEditor *group_matrix = nullptr;
    GizmoListEditor *gizmo_list = nullptr;
    GizmoHost *gizmo_host = nullptr;

    void _bind_layout();
    void _update_header();
    void _set_dirty(bool p_dirty);
    void _on_registry_changed();
    void _on_gizmos_changed();
    void _on_make_own();

  protected:
    static void _bind_methods();

  public:
    ObjectInterfacePanel();

    void set_target(Object *p_target);
    Object *get_target() const;
    Ref<TypeLayout> get_layout() const;
    bool is_pending() const;
    bool is_dirty() const;
    ProceduralEditor *get_preview() const;
    GizmoListEditor *get_gizmo_list() const;
    GizmoHost *get_gizmo_host() const;
    // Ajoute l'interface à la base si besoin et écrit la base à l'emplacement du réglage de projet.
    Error save();
};
} // namespace godot

#endif // OBJECT_INTERFACE_PANEL_HPP
