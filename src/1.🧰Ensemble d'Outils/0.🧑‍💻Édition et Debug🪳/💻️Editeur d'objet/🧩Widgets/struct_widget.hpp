#ifndef STRUCT_WIDGET_HPP
#define STRUCT_WIDGET_HPP

#include "property_widget.hpp"

#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot
{
// Structures (Vector2/2i/3/3i/4/4i, Rect2/2i, AABB, Plane, Quaternion, Basis, Transform2D/3D, Projection) :
// un SpinBox par composante. Les structures sont copiées à la lecture : chaque modification reconstruit la
// valeur complète (composante écrite par chemin, ex: "basis.x.y") puis la réécrit sur la propriété racine.
class StructWidget : public PropertyWidget
{
    GDCLASS(StructWidget, PropertyWidget)

  public:
    // Description d'une composante : libellé + chemin de membres nommés depuis la valeur racine.
    struct Component
    {
        const char *label;
        const char *path;
    };
    struct Layout
    {
        const Component *components = nullptr;
        int count = 0;
        int columns = 1;
        bool is_int = false;
    };
    static Layout layout_for(Variant::Type p_type);

    // Lecture / écriture d'une composante par chemin "a.b.c" (Variant::get_named / set_named, avec
    // reconstruction des niveaux intermédiaires, eux aussi copiés à la lecture).
    static Variant get_path(const Variant &p_root, const String &p_path);
    static bool set_path(Variant &r_root, const String &p_path, const Variant &p_value);

  private:
    Layout layout;
    Vector<SpinBox *> spins;
    Variant current; // dernière valeur complète affichée

    void _on_component_changed(double p_value, int64_t p_index);

  protected:
    static void _bind_methods() {}
    Control *build_editor() override;
    void pull_value(const Variant &p_value) override;
    StringName get_settings_class() const override { return "StructWidgetSettings"; }
};
} // namespace godot

#endif // STRUCT_WIDGET_HPP
