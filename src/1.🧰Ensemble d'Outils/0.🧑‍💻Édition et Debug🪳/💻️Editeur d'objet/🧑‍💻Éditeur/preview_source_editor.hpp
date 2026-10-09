#ifndef PREVIEW_SOURCE_EDITOR_HPP
#define PREVIEW_SOURCE_EDITOR_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔌Accès/undo_backend.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔧Réglages/widget_settings.hpp"

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// Source d'un aperçu (éditeur) : « Objet affiché » (ou un nœud au chemin object_path), propriété, fonction ou
/// valeur manuelle. Les membres proposés sont ceux du type attendu par la classe d'aperçu
/// (PreviewWidget::get_source_hint, lu sur une instance temporaire). Annulable ; émet "source_changed".
class PreviewSourceEditor : public VBoxContainer
{
    GDCLASS(PreviewSourceEditor, VBoxContainer)

  private:
    Ref<PreviewSettings> settings;
    Ref<UndoBackend> backend;
    ObjectID context_id;
    Dictionary hint;
    /// « Propriété » choisi mais aucune propriété encore sélectionnée : même donnée que « Objet affiché »
    /// (propriété sans chemin), le mode est donc retenu ici pour afficher la liste des membres.
    bool property_pending = false;

    void _commit_set(Object *p_object, const StringName &p_property, const Variant &p_value);
    void _on_mode_selected(int64_t p_item, uint64_t p_option_id);
    void _on_member_selected(int64_t p_item, uint64_t p_option_id);
    void _on_constant_resource(const Ref<Resource> &p_resource);

  protected:
    static void _bind_methods();

  public:
    static constexpr int MODE_DISPLAYED = 100; // pas -1 : pour un OptionButton, -1 = identifiant de l'index

    /// Type attendu par la classe d'aperçu p_class (instance temporaire) ; vide si inconnu.
    static Dictionary source_hint_of(const StringName &p_class);
    PreviewSourceEditor();
    void setup(const Ref<PreviewSettings> &p_settings, Object *p_context);
    void refresh();
};
} // namespace godot

#endif // PREVIEW_SOURCE_EDITOR_HPP
