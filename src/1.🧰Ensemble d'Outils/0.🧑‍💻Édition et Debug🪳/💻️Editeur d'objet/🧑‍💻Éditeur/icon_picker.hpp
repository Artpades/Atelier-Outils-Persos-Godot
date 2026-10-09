#ifndef ICON_PICKER_HPP
#define ICON_PICKER_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔌Accès/undo_backend.hpp"

#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// Choix d'une icône de l'éditeur Godot (éditeur). Les icônes de l'éditeur n'existent pas dans un jeu exporté :
/// l'icône choisie est COPIÉE dans le projet (ICON_DIR/<nom>.res, ImageTexture) et c'est cette copie qui est
/// renvoyée, pour fonctionner aussi en jeu.
class EditorIconPicker : public AcceptDialog
{
    GDCLASS(EditorIconPicker, AcceptDialog)

  private:
    Callable on_picked;
    LineEdit *filter = nullptr; // enfants possédés
    ItemList *list = nullptr;

    void _fill(const String &p_filter);
    void _on_filter_changed(const String &p_text);
    void _on_confirmed();
    void _on_item_activated(int64_t p_index);

  protected:
    static void _bind_methods();

  public:
    static constexpr const char *ICON_DIR = "res://object_editor_icons/";

    /// Ouvre le choix ; p_on_picked(texture) reçoit la copie enregistrée dans le projet.
    static EditorIconPicker *open(const Callable &p_on_picked);
    /// Copie (ou relit si elle existe déjà) l'icône p_name de l'éditeur ; nulle si elle n'existe pas.
    static Ref<Texture2D> copy_editor_icon(const String &p_name);
};

/// Ligne d'édition d'une icône (Texture2D) posée sur une propriété d'un objet : sélecteur de ressource +
/// « Icône de l'éditeur… ». Modifications enregistrées dans l'historique d'annulation ; émet "icon_changed".
class IconPickerRow : public HBoxContainer
{
    GDCLASS(IconPickerRow, HBoxContainer)

  private:
    ObjectID owner_id;
    StringName property;
    Ref<UndoBackend> backend;

    void _commit(const Ref<Resource> &p_icon);
    void _on_resource_changed(const Ref<Resource> &p_resource);
    void _on_editor_icon_pressed();
    void _on_editor_icon_picked(const Ref<Texture2D> &p_icon);

  protected:
    static void _bind_methods();

  public:
    void setup(Object *p_owner, const StringName &p_property, const String &p_label);
};
} // namespace godot

#endif // ICON_PICKER_HPP
