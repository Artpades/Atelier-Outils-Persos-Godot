#include "struct_widget.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"

#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

namespace
{
// Tables constantes (aucune destruction statique). Les noms de membres sont ceux de Variant::get_named.
using C = StructWidget::Component;

const C VEC2[] = {{"x", "x"}, {"y", "y"}};
const C VEC3[] = {{"x", "x"}, {"y", "y"}, {"z", "z"}};
const C VEC4[] = {{"x", "x"}, {"y", "y"}, {"z", "z"}, {"w", "w"}};
const C RECT[] = {{"pos x", "position.x"}, {"pos y", "position.y"}, {"taille x", "size.x"}, {"taille y", "size.y"}};
const C BOX[] = {{"pos x", "position.x"}, {"pos y", "position.y"}, {"pos z", "position.z"}, {"taille x", "size.x"}, {"taille y", "size.y"}, {"taille z", "size.z"}};
const C PLANE_C[] = {{"normale x", "normal.x"}, {"normale y", "normal.y"}, {"normale z", "normal.z"}, {"d", "d"}};
const C BASIS_C[] = {{"x.x", "x.x"}, {"x.y", "x.y"}, {"x.z", "x.z"}, {"y.x", "y.x"}, {"y.y", "y.y"}, {"y.z", "y.z"}, {"z.x", "z.x"}, {"z.y", "z.y"}, {"z.z", "z.z"}};
const C T2D[] = {{"x.x", "x.x"}, {"x.y", "x.y"}, {"y.x", "y.x"}, {"y.y", "y.y"}, {"origine x", "origin.x"}, {"origine y", "origin.y"}};
const C T3D[] = {{"x.x", "basis.x.x"}, {"x.y", "basis.x.y"}, {"x.z", "basis.x.z"}, {"y.x", "basis.y.x"},   {"y.y", "basis.y.y"},  {"y.z", "basis.y.z"},
                 {"z.x", "basis.z.x"}, {"z.y", "basis.z.y"}, {"z.z", "basis.z.z"}, {"origine x", "origin.x"}, {"origine y", "origin.y"}, {"origine z", "origin.z"}};
const C PROJ[] = {{"x.x", "x.x"}, {"x.y", "x.y"}, {"x.z", "x.z"}, {"x.w", "x.w"}, {"y.x", "y.x"}, {"y.y", "y.y"}, {"y.z", "y.z"}, {"y.w", "y.w"},
                  {"z.x", "z.x"}, {"z.y", "z.y"}, {"z.z", "z.z"}, {"z.w", "z.w"}, {"w.x", "w.x"}, {"w.y", "w.y"}, {"w.z", "w.z"}, {"w.w", "w.w"}};

template <int N> StructWidget::Layout make_layout(const C (&p_components)[N], int p_columns, bool p_int)
{
    StructWidget::Layout layout;
    layout.components = p_components;
    layout.count = N;
    layout.columns = p_columns;
    layout.is_int = p_int;
    return layout;
}
} // namespace

StructWidget::Layout StructWidget::layout_for(Variant::Type p_type)
{
    switch (p_type)
    {
    case Variant::VECTOR2:
        return make_layout(VEC2, 2, false);
    case Variant::VECTOR2I:
        return make_layout(VEC2, 2, true);
    case Variant::VECTOR3:
        return make_layout(VEC3, 3, false);
    case Variant::VECTOR3I:
        return make_layout(VEC3, 3, true);
    case Variant::VECTOR4:
    case Variant::QUATERNION:
        return make_layout(VEC4, 4, false);
    case Variant::VECTOR4I:
        return make_layout(VEC4, 4, true);
    case Variant::RECT2:
        return make_layout(RECT, 2, false);
    case Variant::RECT2I:
        return make_layout(RECT, 2, true);
    case Variant::AABB:
        return make_layout(BOX, 3, false);
    case Variant::PLANE:
        return make_layout(PLANE_C, 4, false);
    case Variant::BASIS:
        return make_layout(BASIS_C, 3, false);
    case Variant::TRANSFORM2D:
        return make_layout(T2D, 2, false);
    case Variant::TRANSFORM3D:
        return make_layout(T3D, 3, false);
    case Variant::PROJECTION:
        return make_layout(PROJ, 4, false);
    default:
        return Layout();
    }
}

Variant StructWidget::get_path(const Variant &p_root, const String &p_path)
{
    Variant value = p_root;
    PackedStringArray keys = p_path.split(".");
    for (int i = 0; i < keys.size(); i++)
    {
        bool valid = false;
        value = value.get_named(StringName(keys[i]), valid);
        if (!valid)
        {
            return Variant();
        }
    }
    return value;
}

namespace
{
bool set_path_recursive(Variant &r_value, const PackedStringArray &p_keys, int p_index, const Variant &p_new)
{
    bool valid = false;
    StringName key = p_keys[p_index];
    if (p_index == p_keys.size() - 1)
    {
        r_value.set_named(key, p_new, valid);
        return valid;
    }
    // Niveau intermédiaire (ex: basis d'un Transform3D) : copié à la lecture, donc modifié puis réécrit.
    Variant child = r_value.get_named(key, valid);
    if (!valid || !set_path_recursive(child, p_keys, p_index + 1, p_new))
    {
        return false;
    }
    r_value.set_named(key, child, valid);
    return valid;
}
} // namespace

bool StructWidget::set_path(Variant &r_root, const String &p_path, const Variant &p_value)
{
    PackedStringArray keys = p_path.split(".");
    return keys.size() > 0 && set_path_recursive(r_root, keys, 0, p_value);
}

Control *StructWidget::build_editor()
{
    layout = layout_for((Variant::Type)(int)get_info().get("type", 0));

    Ref<StructWidgetSettings> struct_settings = get_settings();
    double step = layout.is_int ? 1.0 : struct_settings.is_valid() && struct_settings->get_step() > 0.0 ? struct_settings->get_step() : 0.001;

    GridContainer *grid = memnew(GridContainer);
    grid->set_columns(MAX(layout.columns, 1));
    for (int i = 0; i < layout.count; i++)
    {
        SpinBox *spin = memnew(SpinBox);
        spin->set_prefix(String(layout.components[i].label));
        spin->set_min(-1e12);
        spin->set_max(1e12);
        spin->set_step(step);
        spin->set_use_rounded_values(layout.is_int);
        spin->set_allow_greater(true);
        spin->set_allow_lesser(true);
        spin->set_editable(!is_read_only());
        spin->set_h_size_flags(SIZE_EXPAND_FILL);
        spin->connect("value_changed", callable_mp(this, &StructWidget::_on_component_changed).bind(i));
        spins.push_back(spin);
        grid->add_child(spin);
    }
    return grid;
}

void StructWidget::pull_value(const Variant &p_value)
{
    current = p_value;
    for (int i = 0; i < spins.size(); i++)
    {
        spins[i]->set_value_no_signal(double(get_path(current, layout.components[i].path)));
    }
}

void StructWidget::_on_component_changed(double p_value, int64_t p_index)
{
    if (p_index < 0 || p_index >= layout.count)
    {
        return;
    }
    Variant component = layout.is_int ? Variant((int64_t)Math::round(p_value)) : Variant(p_value);
    Variant value = current;
    if (!set_path(value, layout.components[p_index].path, component))
    {
        log_it(U"[color=red]Composante « " + String(layout.components[p_index].path) + U" » non modifiable[/color]", this);
        return;
    }
    current = value;
    emit_value(value, true);
}
