#ifndef INTERFACE_STYLE_EDITOR_HPP
#define INTERFACE_STYLE_EDITOR_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔌Accès/undo_backend.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔧Réglages/interface_style.hpp"

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
/// Éditeur d'un InterfaceStyle (éditeur) posé sur une propriété d'un objet propriétaire (TypeLayout.style,
/// GroupEntry.style, WidgetSettings.style, ProceduralEditor.style) :
/// - le style lui-même : créer, charger, partager, retirer (EditorResourcePicker) ;
/// - thème, fond (StyleBox), matériau (ShaderMaterial...) : sélecteurs de ressources ; variation, propagation et
///   teintes : ProceduralEditor ;
/// - uniforms du shader : par uniform, « Valeur du matériau » (non lié) ou une source comme pour les gizmos
///   (valeur manuelle, propriété, fonction) avec la liste des membres compatibles de l'objet de contexte.
/// Toute modification est une action d'annulation de l'éditeur ; « style_changed » est émis après chacune.
class InterfaceStyleEditor : public VBoxContainer
{
    GDCLASS(InterfaceStyleEditor, VBoxContainer)

  private:
    ObjectID owner_id;
    StringName property;
    ObjectID context_id;
    String context_type;
    Ref<UndoBackend> backend;
    Ref<InterfaceStyle> watched; // style dont on écoute "changed"
    Ref<Material> last_material;
    int last_parameter_count = 0;
    bool refresh_queued = false;

    Ref<InterfaceStyle> _style() const;
    void _watch(const Ref<InterfaceStyle> &p_style);
    void _queue_refresh();
    void _commit(const String &p_name, const Array &p_do, const Array &p_undo);
    void _commit_set(Object *p_object, const StringName &p_property, const Variant &p_value);
    Object *_resolve_context(Variant &r_sample) const;

    void _on_style_resource_changed(const Ref<Resource> &p_resource);
    void _on_resource_changed(const Ref<Resource> &p_resource, const String &p_property);
    void _on_resource_selected(const Ref<Resource> &p_resource, bool p_inspect);
    void _on_style_changed();
    void _on_source_selected(int64_t p_item, const String &p_uniform, uint64_t p_option_id);
    void _on_member_selected(int64_t p_item, const String &p_uniform, uint64_t p_option_id);

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  public:
    InterfaceStyleEditor();

    /// Style édité : p_owner.get(p_property).
    void set_owner(Object *p_owner, const StringName &p_property);
    void set_context_object(Object *p_object);
    void set_context_type(const String &p_type);
    void refresh();
};
} // namespace godot

#endif // INTERFACE_STYLE_EDITOR_HPP
