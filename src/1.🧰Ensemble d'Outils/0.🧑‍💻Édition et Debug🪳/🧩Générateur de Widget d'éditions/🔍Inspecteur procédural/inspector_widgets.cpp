#include "inspector_widgets.hpp"
#include "inspector_binding.hpp"

#include <algorithm>
#include <memory>
#include <vector>

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/color_picker_button.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot
{
namespace InspectorWidgets
{
namespace
{
// ==================== 🧰 Petits utilitaires ====================

Object *resolve(ObjectID p_id)
{
    return p_id.is_null() ? nullptr : ObjectDB::get_instance(p_id);
}

Variant::Type type_of(const Dictionary &p_info)
{
    return (Variant::Type)(int)p_info.get("type", (int)Variant::NIL);
}

String type_text(const Dictionary &p_info)
{
    Variant::Type type = type_of(p_info);
    String class_name = p_info.get("class_name", "");
    if (type == Variant::NIL)
    {
        return "Variant";
    }
    if (type == Variant::OBJECT && !class_name.is_empty())
    {
        return class_name;
    }
    return Variant::get_type_name(type);
}

String describe_object(Object *p_object)
{
    if (p_object == nullptr)
    {
        return "<null>";
    }
    if (Node *node = Object::cast_to<Node>(p_object))
    {
        return String(node->get_name()) + " (" + node->get_class() + ")";
    }
    if (Resource *resource = Object::cast_to<Resource>(p_object))
    {
        if (!resource->get_path().is_empty())
        {
            return resource->get_path();
        }
    }
    return p_object->get_class();
}

Label *make_hint(const String &p_text)
{
    Label *label = memnew(Label);
    label->set_text(p_text);
    label->set_modulate(Color(1, 1, 1, 0.6));
    label->set_clip_text(true);
    label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    return label;
}

HBoxContainer *make_hbox()
{
    HBoxContainer *box = memnew(HBoxContainer);
    box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    return box;
}

VBoxContainer *make_vbox()
{
    VBoxContainer *box = memnew(VBoxContainer);
    box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    return box;
}

Button *make_button(const String &p_text, const String &p_tooltip = "")
{
    Button *button = memnew(Button);
    button->set_text(p_text);
    button->set_tooltip_text(p_tooltip);
    return button;
}

/// Conteneur indenté (contenu d'une section, arguments d'une méthode...).
MarginContainer *indented(Control *p_content, int p_margin = 14)
{
    MarginContainer *margin = memnew(MarginContainer);
    margin->add_theme_constant_override("margin_left", p_margin);
    margin->add_child(p_content);
    return margin;
}

/// Place `p_editor` à droite du libellé de la valeur (sans libellé si `label` est vide).
Control *labeled(const ValueAccessor &p_accessor, Control *p_editor)
{
    p_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    p_editor->set_stretch_ratio(0.6);
    if (p_accessor.label.is_empty())
    {
        return p_editor;
    }

    HBoxContainer *row = make_hbox();
    Label *label = memnew(Label);
    label->set_text(p_accessor.label);
    label->set_tooltip_text(p_accessor.info.get("name", ""));
    label->set_mouse_filter(Control::MOUSE_FILTER_STOP);
    label->set_clip_text(true);
    label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    label->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
    label->set_stretch_ratio(0.4);
    row->add_child(label);
    row->add_child(p_editor);
    return row;
}

/// Variante de `p_accessor` présentant un autre type (Variant, éléments de collection...).
ValueAccessor retyped(const ValueAccessor &p_accessor, Variant::Type p_type, const String &p_label)
{
    ValueAccessor result = p_accessor;
    result.info = p_accessor.info.duplicate();
    result.info["type"] = (int)p_type;
    result.info["usage"] = p_accessor.usage() & ~(int64_t)PROPERTY_USAGE_NIL_IS_VARIANT;
    result.label = p_label;
    return result;
}


// ==================== 📂 Section repliable ====================

/// En-tête bascule + corps indenté. Les appelants ajoutent leurs propres boutons à `header_row`
/// et se connectent à `header` ("toggled") pour construire le contenu à la première ouverture.
struct Section
{
    VBoxContainer *root = nullptr;
    HBoxContainer *header_row = nullptr;
    Button *header = nullptr;
    VBoxContainer *content = nullptr;

    bool is_open() const { return header->is_pressed(); }

    void set_title(const String &p_title) const
    {
        header->set_meta("title", p_title);
        header->set_text((is_open() ? String(U"▼ ") : String(U"▶ ")) + p_title);
    }
};

Section make_section()
{
    Section section;
    section.root = make_vbox();
    section.header_row = make_hbox();
    section.header = memnew(Button);
    section.header->set_toggle_mode(true);
    section.header->set_flat(true);
    section.header->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    section.header->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    section.header->set_clip_text(true);
    section.content = make_vbox();

    MarginContainer *body = indented(section.content);
    body->set_visible(false);

    section.header_row->add_child(section.header);
    section.root->add_child(section.header_row);
    section.root->add_child(body);

    Button *header = section.header;
    header->connect("toggled", lambda_callable_1(header, [header, body](const Variant &p_open) {
                        body->set_visible(p_open);
                        header->set_text((bool(p_open) ? String(U"▼ ") : String(U"▶ ")) + String(header->get_meta("title", "")));
                    }));
    return section;
}


// ==================== 🔎 Analyse des indices (hint_string) ====================

struct EnumItem
{
    String label;
    Variant value;
};

/// "A,B:5,C" → (A,0) (B,5) (C,6). Flags : valeurs implicites en puissances de deux.
std::vector<EnumItem> parse_enum(const String &p_hint_string, bool p_flags, bool p_string_values)
{
    std::vector<EnumItem> items;
    int64_t next = p_flags ? 1 : 0;
    PackedStringArray parts = p_hint_string.split(",", false);
    for (int i = 0; i < parts.size(); i++)
    {
        String part = parts[i].strip_edges();
        if (part.is_empty())
        {
            continue;
        }
        if (p_string_values)
        {
            items.push_back({ part, part });
            continue;
        }
        int64_t value = next;
        String label = part;
        int64_t colon = part.rfind(":");
        if (colon > 0 && part.substr(colon + 1).is_valid_int())
        {
            label = part.substr(0, colon);
            value = part.substr(colon + 1).to_int();
        }
        items.push_back({ label, value });
        next = p_flags ? value << 1 : value + 1;
    }
    return items;
}

struct RangeHint
{
    double min = 0, max = 0, step = 0;
    bool has_bounds = false, or_greater = false, or_less = false;
    String suffix;
};

/// "min,max[,step][,or_greater][,or_less][,suffix:m]"
RangeHint parse_range(const String &p_hint_string, bool p_is_int)
{
    RangeHint range;
    int numbers = 0;
    PackedStringArray parts = p_hint_string.split(",", false);
    for (int i = 0; i < parts.size(); i++)
    {
        String part = parts[i].strip_edges();
        if (numbers < 3 && part.is_valid_float())
        {
            double n = part.to_float();
            if (numbers == 0)
            {
                range.min = n;
            }
            else if (numbers == 1)
            {
                range.max = n;
                range.has_bounds = true;
            }
            else
            {
                range.step = n;
            }
            numbers++;
        }
        else if (part == "or_greater")
        {
            range.or_greater = true;
        }
        else if (part == "or_less")
        {
            range.or_less = true;
        }
        else if (part.begins_with("suffix:"))
        {
            range.suffix = part.substr(7);
        }
    }
    if (p_is_int && range.step <= 0)
    {
        range.step = 1;
    }
    return range;
}

/// Élément d'un tableau typé déclaré par PROPERTY_HINT_TYPE_STRING : "type/indice:chaîne" (ex : "2/2:Idle,Run").
Dictionary parse_element_decl(const ValueAccessor &p_accessor)
{
    Dictionary decl;
    if (p_accessor.hint() != PROPERTY_HINT_TYPE_STRING)
    {
        return decl;
    }
    String text = p_accessor.hint_string();
    int64_t slash = text.find("/");
    if (slash <= 0)
    {
        return decl;
    }
    int64_t colon = text.find(":", slash);
    decl["hint"] = (colon < 0 ? text.substr(slash + 1) : text.substr(slash + 1, colon - slash - 1)).to_int();
    decl["hint_string"] = colon < 0 ? String() : text.substr(colon + 1);
    return decl;
}


// ==================== 📍 Sélecteurs (nœud, ressource) ====================

void add_tree_item(Tree *p_tree, TreeItem *p_parent, Node *p_node)
{
    TreeItem *item = p_tree->create_item(p_parent);
    item->set_text(0, String(p_node->get_name()) + "  (" + p_node->get_class() + ")");
    item->set_metadata(0, (int64_t)p_node->get_instance_id());
    for (int i = 0; i < p_node->get_child_count(); i++)
    {
        add_tree_item(p_tree, item, p_node->get_child(i));
    }
}

/// Ouvre l'arbre de la scène ; `p_on_picked` reçoit le nœud validé.
void pick_node(Control *p_owner, std::function<void(Node *)> p_on_picked)
{
    Node *root = find_scene_root();
    if (root == nullptr)
    {
        UtilityFunctions::push_warning(String(U"[ProceduralInspector] Aucune scène disponible pour choisir un nœud."));
        return;
    }

    Tree *tree = memnew(Tree);
    tree->set_custom_minimum_size(Vector2(360, 380));
    add_tree_item(tree, nullptr, root);

    ConfirmationDialog *dialog = memnew(ConfirmationDialog);
    dialog->set_title(String(U"Choisir un nœud"));
    dialog->set_ok_button_text("Choisir");
    dialog->add_child(tree);

    dialog->connect("confirmed", lambda_callable(dialog, [dialog, tree, p_on_picked]() {
                        if (TreeItem *selected = tree->get_selected())
                        {
                            Object *picked = ObjectDB::get_instance((uint64_t)(int64_t)selected->get_metadata(0));
                            if (Node *node = Object::cast_to<Node>(picked))
                            {
                                p_on_picked(node);
                            }
                        }
                        dialog->queue_free();
                    }));
    dialog->connect("canceled", lambda_callable(dialog, [dialog]() { dialog->queue_free(); }));

    p_owner->add_child(dialog);
    dialog->popup_centered(Vector2i(400, 480));
}

/// Ouvre un sélecteur de fichier `res://` ; `p_on_picked` reçoit la ressource chargée.
void pick_resource(Control *p_owner, std::function<void(const Ref<Resource> &)> p_on_picked)
{
    FileDialog *dialog = memnew(FileDialog);
    dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    dialog->set_access(FileDialog::ACCESS_RESOURCES);
    dialog->set_title("Choisir une ressource");

    dialog->connect("file_selected", lambda_callable_1(dialog, [dialog, p_on_picked](const Variant &p_path) {
                        Ref<Resource> resource = ResourceLoader::get_singleton()->load(p_path);
                        if (resource.is_valid())
                        {
                            p_on_picked(resource);
                        }
                        dialog->queue_free();
                    }));
    dialog->connect("canceled", lambda_callable(dialog, [dialog]() { dialog->queue_free(); }));

    p_owner->add_child(dialog);
    dialog->popup_centered(Vector2i(700, 460));
}

/// Nœud d'où résoudre un chemin : le contexte s'il est un Node dans l'arbre, sinon la racine de la scène.
Node *path_base(ObjectID p_context)
{
    Node *base = Object::cast_to<Node>(resolve(p_context));
    return base != nullptr && base->is_inside_tree() ? base : find_scene_root();
}


// ==================== ☑️ Widgets de valeurs ====================

Control *build_bool(const ValueAccessor &p_accessor)
{
    CheckBox *box = memnew(CheckBox);
    box->set_disabled(p_accessor.read_only());

    InspectorBinding *binding = InspectorBinding::attach(box, p_accessor, [box](const Variant &p_value) {
        box->set_pressed_no_signal(p_value.booleanize());
    });
    box->connect("toggled", lambda_callable_1(box, [binding](const Variant &p_pressed) { binding->commit(p_pressed); }));
    return labeled(p_accessor, box);
}

Control *build_number(const ValueAccessor &p_accessor)
{
    bool is_int = p_accessor.type() == Variant::INT;
    RangeHint range = parse_range(p_accessor.hint() == PROPERTY_HINT_RANGE ? p_accessor.hint_string() : String(), is_int);

    SpinBox *spin = memnew(SpinBox);
    spin->set_min(range.has_bounds ? range.min : -1e12);
    spin->set_max(range.has_bounds ? range.max : 1e12);
    spin->set_step(range.step > 0 ? range.step : (is_int ? 1.0 : 0.001));
    spin->set_use_rounded_values(is_int);
    spin->set_allow_greater(range.or_greater || !range.has_bounds);
    spin->set_allow_lesser(range.or_less || !range.has_bounds);
    spin->set_suffix(range.suffix);
    spin->set_editable(!p_accessor.read_only());
    spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);

    // Avec des bornes, un curseur partage la valeur du SpinBox (Range::share) : un seul signal à écouter.
    Control *editor = spin;
    if (range.has_bounds)
    {
        HSlider *slider = memnew(HSlider);
        slider->share(spin);
        slider->set_editable(!p_accessor.read_only());
        slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        slider->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
        slider->set_stretch_ratio(2.0);
        HBoxContainer *row = make_hbox();
        row->add_child(slider);
        row->add_child(spin);
        editor = row;
    }

    InspectorBinding *binding = InspectorBinding::attach(editor, p_accessor, [spin](const Variant &p_value) {
        spin->set_value_no_signal((double)p_value);
    });
    spin->connect("value_changed", lambda_callable_1(spin, [binding, is_int](const Variant &p_value) {
                      double value = p_value;
                      binding->commit(is_int ? Variant((int64_t)Math::round(value)) : Variant(value));
                  }));
    return labeled(p_accessor, editor);
}

Control *build_string(const ValueAccessor &p_accessor)
{
    bool string_name = p_accessor.type() == Variant::STRING_NAME;
    bool read_only = p_accessor.read_only();
    auto wrap = [string_name](const String &p_text) { return string_name ? Variant(StringName(p_text)) : Variant(p_text); };

    if (p_accessor.hint() == PROPERTY_HINT_MULTILINE_TEXT)
    {
        TextEdit *edit = memnew(TextEdit);
        edit->set_editable(!read_only);
        edit->set_custom_minimum_size(Vector2(0, 80));
        InspectorBinding *binding = InspectorBinding::attach(edit, p_accessor, [edit](const Variant &p_value) {
            String text = p_value;
            if (edit->get_text() != text)
            {
                edit->set_text(text);
            }
        });
        edit->connect("text_changed", lambda_callable(edit, [binding, edit, wrap]() { binding->commit(wrap(edit->get_text())); }));
        return labeled(p_accessor, edit);
    }

    LineEdit *edit = memnew(LineEdit);
    edit->set_editable(!read_only);
    InspectorBinding *binding = InspectorBinding::attach(edit, p_accessor, [edit](const Variant &p_value) {
        String text = p_value;
        if (edit->get_text() == text)
        {
            return;
        }
        int caret = edit->get_caret_column();
        edit->set_text(text);
        edit->set_caret_column(caret);
    });
    edit->connect("text_changed", lambda_callable_1(edit, [binding, wrap](const Variant &p_text) { binding->commit(wrap(p_text)); }));
    return labeled(p_accessor, edit);
}

bool same_enum_value(const Variant &p_a, const Variant &p_b)
{
    if (p_a.get_type() == Variant::INT || p_b.get_type() == Variant::INT)
    {
        return (int64_t)p_a == (int64_t)p_b;
    }
    return String(p_a) == String(p_b);
}

Control *build_enum(const ValueAccessor &p_accessor)
{
    Variant::Type type = p_accessor.type();
    bool flags = p_accessor.hint() == PROPERTY_HINT_FLAGS;
    bool read_only = p_accessor.read_only();
    std::vector<EnumItem> items = parse_enum(p_accessor.hint_string(), flags, type != Variant::INT);
    auto wrap = [type](const Variant &p_value) { return type == Variant::STRING_NAME ? Variant(StringName(String(p_value))) : p_value; };

    if (!flags)
    {
        OptionButton *option = memnew(OptionButton);
        option->set_disabled(read_only);
        for (size_t i = 0; i < items.size(); i++)
        {
            option->add_item(items[i].label, (int)i);
        }
        InspectorBinding *binding = InspectorBinding::attach(option, p_accessor, [option, items](const Variant &p_value) {
            int index = -1;
            for (size_t i = 0; i < items.size(); i++)
            {
                if (same_enum_value(items[i].value, p_value))
                {
                    index = (int)i;
                    break;
                }
            }
            option->select(index);
        });
        option->connect("item_selected", lambda_callable_1(option, [binding, items, wrap](const Variant &p_index) {
                            int index = p_index;
                            if (index >= 0 && index < (int)items.size())
                            {
                                binding->commit(wrap(items[index].value));
                            }
                        }));
        return labeled(p_accessor, option);
    }

    // Flags : une case par bit, cochages multiples.
    VBoxContainer *box = make_vbox();
    std::vector<CheckBox *> checks;
    for (const EnumItem &item : items)
    {
        CheckBox *check = memnew(CheckBox);
        check->set_text(item.label);
        check->set_disabled(read_only);
        checks.push_back(check);
        box->add_child(check);
    }
    InspectorBinding *binding = InspectorBinding::attach(box, p_accessor, [checks, items](const Variant &p_value) {
        int64_t mask = p_value;
        for (size_t i = 0; i < items.size(); i++)
        {
            int64_t bit = items[i].value;
            checks[i]->set_pressed_no_signal(bit != 0 && (mask & bit) == bit);
        }
    });
    for (CheckBox *check : checks)
    {
        check->connect("toggled", lambda_callable_1(check, [binding, checks, items](const Variant &) {
                           int64_t mask = 0;
                           for (size_t i = 0; i < items.size(); i++)
                           {
                               if (checks[i]->is_pressed())
                               {
                                   mask |= (int64_t)items[i].value;
                               }
                           }
                           binding->commit(mask);
                       }));
    }
    return labeled(p_accessor, box);
}

Control *build_color(const ValueAccessor &p_accessor)
{
    ColorPickerButton *picker = memnew(ColorPickerButton);
    picker->set_edit_alpha(p_accessor.hint() != PROPERTY_HINT_COLOR_NO_ALPHA);
    picker->set_disabled(p_accessor.read_only());
    picker->set_custom_minimum_size(Vector2(0, 28));

    InspectorBinding *binding = InspectorBinding::attach(picker, p_accessor, [picker](const Variant &p_value) {
        picker->set_pick_color(p_value);
    });
    picker->connect("color_changed", lambda_callable_1(picker, [binding](const Variant &p_color) { binding->commit(p_color); }));
    return labeled(p_accessor, picker);
}

/// Composante d'une structure : `outer` (ex : "x", "position"), et `inner` pour un sous-champ (Rect2 : position.x).
struct Component
{
    const char *label;
    const char *outer;
    const char *inner;
};

bool struct_components(Variant::Type p_type, std::vector<Component> &r_components, bool &r_is_int)
{
    static const std::vector<Component> xy = { { "X", "x", nullptr }, { "Y", "y", nullptr } };
    static const std::vector<Component> xyz = { { "X", "x", nullptr }, { "Y", "y", nullptr }, { "Z", "z", nullptr } };
    static const std::vector<Component> xyzw = { { "X", "x", nullptr }, { "Y", "y", nullptr }, { "Z", "z", nullptr }, { "W", "w", nullptr } };
    static const std::vector<Component> plane = { { "X", "x", nullptr }, { "Y", "y", nullptr }, { "Z", "z", nullptr }, { "D", "d", nullptr } };
    static const std::vector<Component> rect = {
        { "X", "position", "x" }, { "Y", "position", "y" }, { "W", "size", "x" }, { "H", "size", "y" }
    };

    r_is_int = p_type == Variant::VECTOR2I || p_type == Variant::VECTOR3I || p_type == Variant::VECTOR4I || p_type == Variant::RECT2I;
    switch (p_type)
    {
    case Variant::VECTOR2:
    case Variant::VECTOR2I:
        r_components = xy;
        return true;
    case Variant::VECTOR3:
    case Variant::VECTOR3I:
        r_components = xyz;
        return true;
    case Variant::VECTOR4:
    case Variant::VECTOR4I:
    case Variant::QUATERNION:
        r_components = xyzw;
        return true;
    case Variant::PLANE:
        r_components = plane;
        return true;
    case Variant::RECT2:
    case Variant::RECT2I:
        r_components = rect;
        return true;
    default:
        return false;
    }
}

double read_component(const Variant &p_value, const Component &p_component)
{
    bool valid = false;
    Variant value = p_value.get_named(p_component.outer, valid);
    if (p_component.inner != nullptr)
    {
        value = value.get_named(p_component.inner, valid);
    }
    return valid ? (double)value : 0.0;
}

void write_component(Variant &r_value, const Component &p_component, double p_number, bool p_is_int)
{
    Variant number = p_is_int ? Variant((int64_t)Math::round(p_number)) : Variant(p_number);
    bool valid = false;
    if (p_component.inner == nullptr)
    {
        r_value.set_named(p_component.outer, number, valid);
        return;
    }
    Variant sub = r_value.get_named(p_component.outer, valid);
    sub.set_named(p_component.inner, number, valid);
    r_value.set_named(p_component.outer, sub, valid);
}

Control *build_struct(const ValueAccessor &p_accessor, const std::vector<Component> &p_components, bool p_is_int)
{
    GridContainer *grid = memnew(GridContainer);
    grid->set_columns((int)p_components.size());
    std::vector<SpinBox *> spins;
    for (const Component &component : p_components)
    {
        SpinBox *spin = memnew(SpinBox);
        spin->set_prefix(component.label);
        spin->set_min(-1e12);
        spin->set_max(1e12);
        spin->set_step(p_is_int ? 1.0 : 0.001);
        spin->set_use_rounded_values(p_is_int);
        spin->set_editable(!p_accessor.read_only());
        spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        spin->set_custom_minimum_size(Vector2(64, 0));
        spins.push_back(spin);
        grid->add_child(spin);
    }

    InspectorBinding *binding = InspectorBinding::attach(grid, p_accessor, [spins, p_components](const Variant &p_value) {
        for (size_t i = 0; i < spins.size(); i++)
        {
            spins[i]->set_value_no_signal(read_component(p_value, p_components[i]));
        }
    });
    for (SpinBox *spin : spins)
    {
        spin->connect("value_changed", lambda_callable_1(spin, [binding, spins, p_components, p_is_int](const Variant &) {
                          Variant value = binding->get_accessor().get();
                          for (size_t i = 0; i < spins.size(); i++)
                          {
                              write_component(value, p_components[i], spins[i]->get_value(), p_is_int);
                          }
                          binding->commit(value);
                      }));
    }
    return labeled(p_accessor, grid);
}

Control *build_node_path(const ValueAccessor &p_accessor)
{
    bool read_only = p_accessor.read_only();
    ObjectID context = p_accessor.context;

    LineEdit *edit = memnew(LineEdit);
    edit->set_editable(!read_only);
    edit->set_placeholder(String(U"Chemin du nœud"));
    edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    Label *status = memnew(Label);
    status->set_custom_minimum_size(Vector2(96, 0));
    status->set_clip_text(true);
    Button *pick = make_button(String(U"🔍"), String(U"Choisir un nœud dans la scène"));
    pick->set_disabled(read_only);

    HBoxContainer *row = make_hbox();
    row->add_child(edit);
    row->add_child(status);
    row->add_child(pick);

    auto update_status = [edit, status, context]() {
        String text = edit->get_text();
        Node *base = path_base(context);
        Node *node = text.is_empty() || base == nullptr ? nullptr : base->get_node_or_null(NodePath(text));
        status->set_text(text.is_empty() ? String() : node != nullptr ? String(U"✔ ") + node->get_class() : String(U"✘ introuvable"));
        status->set_modulate(node != nullptr ? Color(0.5, 1, 0.5) : Color(1, 0.5, 0.5));
    };

    InspectorBinding *binding = InspectorBinding::attach(row, p_accessor, [edit, update_status](const Variant &p_value) {
        String text = String(NodePath(p_value));
        if (edit->get_text() != text)
        {
            edit->set_text(text);
        }
        update_status();
    });

    auto commit_text = [binding, edit, update_status]() {
        if (String(NodePath(binding->get_accessor().get())) != edit->get_text())
        {
            binding->commit(NodePath(edit->get_text()));
        }
        update_status();
    };
    edit->connect("text_submitted", lambda_callable_1(edit, [commit_text](const Variant &) { commit_text(); }));
    edit->connect("focus_exited", lambda_callable(edit, commit_text));
    pick->connect("pressed", lambda_callable(pick, [pick, context, edit, commit_text]() {
                      pick_node(pick, [context, edit, commit_text](Node *p_node) {
                          Node *base = path_base(context);
                          edit->set_text(base != nullptr ? String(base->get_path_to(p_node)) : String(p_node->get_path()));
                          commit_text();
                      });
                  }));
    return labeled(p_accessor, row);
}

Control *build_object(const ValueAccessor &p_accessor, int p_depth)
{
    bool read_only = p_accessor.read_only();
    String class_name = p_accessor.info.get("class_name", "");
    bool wants_resource = !class_name.is_empty() && ClassDBSingleton::get_singleton()->is_parent_class(class_name, "Resource");
    String label = p_accessor.label.is_empty() ? String("Objet") : p_accessor.label;

    Section section = make_section();
    auto current = std::make_shared<ObjectID>();

    auto fill = [section, current, p_depth]() {
        clear(section.content);
        Object *object = resolve(*current);
        if (object == nullptr || !section.is_open())
        {
            return;
        }
        if (p_depth + 1 > MAX_DEPTH)
        {
            section.content->add_child(make_hint("(profondeur maximale atteinte)"));
            return;
        }
        Options options;
        options.properties = true;
        options.methods = false;
        options.signals = false;
        options.depth = p_depth + 1;
        populate(section.content, object, options);
    };
    // Le contenu n'est construit qu'à l'ouverture : évite le coût des grandes hiérarchies
    // et les cycles infinis (objet qui se référence lui-même).
    section.header->connect("toggled", lambda_callable_1(section.header, [fill](const Variant &) { fill(); }));

    InspectorBinding *binding = InspectorBinding::attach(
        section.root, p_accessor,
        [section, current, label, fill](const Variant &p_value) {
            Object *object = p_value;
            *current = object != nullptr ? ObjectID(object->get_instance_id()) : ObjectID();
            section.set_title(object == nullptr ? label + String(" : <null>") : label + String("  (") + describe_object(object) + ")");
            section.header->set_disabled(object == nullptr);
            fill();
        },
        [](const Variant &p_value) {
            Object *object = p_value;
            uint64_t id = object != nullptr ? object->get_instance_id() : 0;
            return (uint32_t)(id ^ (id >> 32));
        });

    if (!read_only)
    {
        // Assignation : un nœud de la scène, ou une ressource (fichier) si la classe déclarée en est une.
        Button *pick = make_button(String(U"🔍"), wants_resource ? "Choisir une ressource" : String(U"Choisir un nœud dans la scène"));
        Button *reset = make_button(String(U"✕"), "Vider");
        section.header_row->add_child(pick);
        section.header_row->add_child(reset);
        pick->connect("pressed", lambda_callable(pick, [pick, binding, wants_resource, class_name]() {
                          if (wants_resource)
                          {
                              pick_resource(pick, [binding](const Ref<Resource> &p_resource) { binding->commit(p_resource); });
                              return;
                          }
                          pick_node(pick, [binding, class_name](Node *p_node) {
                              if (!class_name.is_empty() && !p_node->is_class(class_name))
                              {
                                  UtilityFunctions::push_warning("[ProceduralInspector] ", p_node->get_name(), " n'est pas un ", class_name, ".");
                                  return;
                              }
                              binding->commit(p_node);
                          });
                      }));
        reset->connect("pressed", lambda_callable(reset, [binding]() { binding->commit(Variant()); }));
    }
    return section.root;
}

/// Clé structurelle d'une collection : taille, clés et types (les valeurs sont suivies par leurs propres widgets).
/// Reconstruire à chaque changement de valeur ferait perdre le focus pendant la saisie.
uint32_t collection_structure_key(const Variant &p_value)
{
    uint32_t hash = (uint32_t)p_value.get_type();
    if (p_value.get_type() == Variant::ARRAY)
    {
        Array array = p_value;
        hash = hash * 31u + (uint32_t)array.size();
        for (int i = 0; i < array.size(); i++)
        {
            hash = hash * 31u + (uint32_t)array[i].get_type();
        }
    }
    else if (p_value.get_type() == Variant::DICTIONARY)
    {
        Dictionary dict = p_value;
        Array keys = dict.keys();
        for (int i = 0; i < keys.size(); i++)
        {
            hash = hash * 31u + keys[i].hash();
            hash = hash * 31u + (uint32_t)dict[keys[i]].get_type();
        }
    }
    return hash;
}

String collection_summary(const Variant &p_value)
{
    if (p_value.get_type() == Variant::ARRAY)
    {
        return String(U"Array · ") + String::num_int64(Array(p_value).size());
    }
    if (p_value.get_type() == Variant::DICTIONARY)
    {
        return String(U"Dictionary · ") + String::num_int64(Dictionary(p_value).size());
    }
    return "<null>";
}

/// Accès à l'élément `p_key` (index ou clé) de la collection de `p_parent`.
/// Chaque lecture/écriture relit la collection fraîche puis la réécrit dans la source : indispensable pour
/// déclencher le setter de la propriété, et pour les sources qui renvoient une copie à chaque lecture.
ValueAccessor element_accessor(const ValueAccessor &p_parent, const Variant &p_key, Variant::Type p_type, const Dictionary &p_decl, const String &p_label)
{
    ValueAccessor element;
    element.get = [p_parent, p_key]() {
        Variant collection = p_parent.get();
        if (collection.get_type() == Variant::ARRAY)
        {
            Array array = collection;
            int index = p_key;
            return index < array.size() ? array[index] : Variant();
        }
        if (collection.get_type() == Variant::DICTIONARY)
        {
            return Dictionary(collection).get(p_key, Variant());
        }
        return Variant();
    };
    element.set = [p_parent, p_key](const Variant &p_value) {
        Variant collection = p_parent.get();
        if (collection.get_type() == Variant::ARRAY)
        {
            Array array = collection;
            int index = p_key;
            if (index < array.size())
            {
                array[index] = p_value;
                p_parent.set(array);
            }
        }
        else if (collection.get_type() == Variant::DICTIONARY)
        {
            Dictionary dict = collection;
            dict[p_key] = p_value;
            p_parent.set(dict);
        }
    };
    element.is_alive = p_parent.is_alive;
    element.context = p_parent.context;
    element.label = p_label;

    bool typed = p_type != Variant::NIL;
    element.info["name"] = p_label;
    element.info["type"] = (int)p_type;
    element.info["hint"] = p_decl.get("hint", 0);
    element.info["hint_string"] = p_decl.get("hint_string", "");
    element.info["usage"] = (int64_t)(p_parent.read_only() ? PROPERTY_USAGE_READ_ONLY : 0) | (typed ? 0 : (int64_t)PROPERTY_USAGE_NIL_IS_VARIANT);
    return element;
}

Control *build_collection(const ValueAccessor &p_accessor, int p_depth)
{
    bool read_only = p_accessor.read_only();
    Variant::Type declared = p_accessor.type();
    Dictionary decl = parse_element_decl(p_accessor);
    String prefix = p_accessor.label.is_empty() ? String() : p_accessor.label + String("  ");

    Section section = make_section();
    auto binding_ref = std::make_shared<InspectorBinding *>(nullptr);

    auto fill = [p_accessor, section, binding_ref, read_only, declared, decl, p_depth]() {
        clear(section.content);
        if (!section.is_open())
        {
            return;
        }
        auto refresh = [binding_ref]() {
            if (*binding_ref != nullptr)
            {
                (*binding_ref)->refresh(true);
            }
        };
        Variant value = p_accessor.get();

        if (value.get_type() == Variant::ARRAY)
        {
            Array array = value;
            Variant::Type element_type = (Variant::Type)array.get_typed_builtin();
            for (int i = 0; i < array.size(); i++)
            {
                ValueAccessor element = element_accessor(p_accessor, i, element_type, decl, "[" + String::num_int64(i) + "]");
                HBoxContainer *row = make_hbox();
                row->add_child(build_value_widget(element, p_depth + 1));
                if (!read_only)
                {
                    Button *remove = make_button(String(U"✕"), String(U"Supprimer l'élément"));
                    remove->connect("pressed", lambda_callable(remove, [p_accessor, i, refresh]() {
                                        Variant current = p_accessor.get();
                                        if (current.get_type() == Variant::ARRAY && i < Array(current).size())
                                        {
                                            Array array = current;
                                            array.remove_at(i);
                                            p_accessor.set(array);
                                        }
                                        refresh();
                                    }));
                    row->add_child(remove);
                }
                section.content->add_child(row);
            }
            if (!read_only)
            {
                Button *add = make_button(String(U"+ Ajouter un élément"));
                add->connect("pressed", lambda_callable(add, [p_accessor, element_type, refresh]() {
                                 Variant current = p_accessor.get();
                                 if (current.get_type() == Variant::ARRAY)
                                 {
                                     Array array = current;
                                     array.append(default_for(element_type));
                                     p_accessor.set(array);
                                 }
                                 refresh();
                             }));
                section.content->add_child(add);
            }
            return;
        }

        if (value.get_type() == Variant::DICTIONARY)
        {
            Dictionary dict = value;
            Variant::Type key_type = (Variant::Type)dict.get_typed_key_builtin();
            Variant::Type value_type = (Variant::Type)dict.get_typed_value_builtin();
            Array keys = dict.keys();
            for (int i = 0; i < keys.size(); i++)
            {
                Variant key = keys[i];
                String key_text = key.get_type() == Variant::STRING ? String(key) : UtilityFunctions::var_to_str(key);
                ValueAccessor element = element_accessor(p_accessor, key, value_type, Dictionary(), key_text);
                HBoxContainer *row = make_hbox();
                row->add_child(build_value_widget(element, p_depth + 1));
                if (!read_only)
                {
                    Button *remove = make_button(String(U"✕"), String(U"Supprimer l'entrée"));
                    remove->connect("pressed", lambda_callable(remove, [p_accessor, key, refresh]() {
                                        Variant current = p_accessor.get();
                                        if (current.get_type() == Variant::DICTIONARY)
                                        {
                                            Dictionary dict = current;
                                            dict.erase(key);
                                            p_accessor.set(dict);
                                        }
                                        refresh();
                                    }));
                    row->add_child(remove);
                }
                section.content->add_child(row);
            }
            if (!read_only)
            {
                // Clé saisie en texte puis convertie selon le type de clé déclaré (texte par défaut).
                LineEdit *key_edit = memnew(LineEdit);
                key_edit->set_placeholder(key_type == Variant::NIL ? String(U"clé") : String(U"clé (") + Variant::get_type_name(key_type) + ")");
                key_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
                Button *add = make_button("+ Ajouter");
                add->connect("pressed", lambda_callable(add, [p_accessor, key_edit, key_type, value_type, refresh]() {
                                 String text = key_edit->get_text().strip_edges();
                                 if (text.is_empty())
                                 {
                                     return;
                                 }
                                 Variant key = text;
                                 if (key_type == Variant::INT)
                                 {
                                     key = text.to_int();
                                 }
                                 else if (key_type == Variant::FLOAT)
                                 {
                                     key = text.to_float();
                                 }
                                 else if (key_type == Variant::STRING_NAME)
                                 {
                                     key = StringName(text);
                                 }
                                 Variant current = p_accessor.get();
                                 if (current.get_type() != Variant::DICTIONARY)
                                 {
                                     return;
                                 }
                                 Dictionary dict = current;
                                 if (dict.has(key))
                                 {
                                     UtilityFunctions::push_warning(String(U"[ProceduralInspector] La clé "), text, String(U" existe déjà."));
                                     return;
                                 }
                                 dict[key] = default_for(value_type);
                                 p_accessor.set(dict);
                                 key_edit->clear();
                                 refresh();
                             }));
                HBoxContainer *row = make_hbox();
                row->add_child(key_edit);
                row->add_child(add);
                section.content->add_child(row);
            }
            return;
        }

        // Collection jamais initialisée (null).
        if (!read_only && (declared == Variant::ARRAY || declared == Variant::DICTIONARY))
        {
            Button *create = make_button(declared == Variant::ARRAY ? String(U"Créer le tableau") : String(U"Créer le dictionnaire"));
            create->connect("pressed", lambda_callable(create, [p_accessor, declared, refresh]() {
                                p_accessor.set(declared == Variant::ARRAY ? Variant(Array()) : Variant(Dictionary()));
                                refresh();
                            }));
            section.content->add_child(create);
        }
    };

    section.header->connect("toggled", lambda_callable_1(section.header, [fill](const Variant &) { fill(); }));

    *binding_ref = InspectorBinding::attach(
        section.root, p_accessor,
        [section, prefix, fill](const Variant &p_value) {
            section.set_title(prefix + String("(") + collection_summary(p_value) + ")");
            fill();
        },
        [section](const Variant &p_value) {
            // Fermée : seule la taille compte (titre). Ouverte : la structure complète.
            return section.is_open() ? collection_structure_key(p_value) : (uint32_t)collection_summary(p_value).hash();
        });
    return section.root;
}

Control *build_callable(const ValueAccessor &p_accessor)
{
    Button *button = make_button(String(U"▶ Appeler"), String(U"Exécute la Callable stockée"));
    button->set_disabled(p_accessor.read_only());
    ValueAccessor accessor = p_accessor;
    button->connect("pressed", lambda_callable(button, [accessor]() {
                        if (!accessor.alive())
                        {
                            return;
                        }
                        Callable callable = accessor.get();
                        if (callable.is_valid())
                        {
                            callable.call();
                        }
                    }));
    return labeled(p_accessor, button);
}

/// Conversion d'un type à l'autre que Godot sait faire sans erreur.
bool convertible(Variant::Type p_from, Variant::Type p_to)
{
    auto scalar = [](Variant::Type t) {
        return t == Variant::BOOL || t == Variant::INT || t == Variant::FLOAT || t == Variant::STRING || t == Variant::STRING_NAME || t == Variant::NODE_PATH;
    };
    auto pair = [p_from, p_to](Variant::Type a, Variant::Type b) { return (p_from == a && p_to == b) || (p_from == b && p_to == a); };
    return (scalar(p_from) && scalar(p_to)) || pair(Variant::VECTOR2, Variant::VECTOR2I) || pair(Variant::VECTOR3, Variant::VECTOR3I) ||
           pair(Variant::VECTOR4, Variant::VECTOR4I) || pair(Variant::RECT2, Variant::RECT2I);
}

/// Valeur au type libre : un OptionButton choisit le type, puis le widget de ce type édite la valeur.
Control *build_variant(const ValueAccessor &p_accessor, int p_depth)
{
    static const Variant::Type selectable[] = {
        Variant::NIL,     Variant::BOOL,     Variant::INT,     Variant::FLOAT,    Variant::STRING,   Variant::STRING_NAME,
        Variant::NODE_PATH, Variant::VECTOR2, Variant::VECTOR2I, Variant::VECTOR3, Variant::VECTOR3I, Variant::VECTOR4,
        Variant::RECT2,   Variant::COLOR,    Variant::ARRAY,   Variant::DICTIONARY,
    };

    OptionButton *option = memnew(OptionButton);
    option->set_disabled(p_accessor.read_only());
    option->set_tooltip_text("Type de la valeur");
    for (Variant::Type type : selectable)
    {
        option->add_item(type == Variant::NIL ? String("null") : Variant::get_type_name(type), (int)type);
    }
    VBoxContainer *holder = make_vbox();
    HBoxContainer *row = make_hbox();
    row->add_child(option);
    row->add_child(holder);

    auto shown = std::make_shared<int>(-1);
    auto show = [p_accessor, p_depth, option, holder, shown](Variant::Type p_type) {
        *shown = (int)p_type;
        if (option->get_item_index((int)p_type) < 0) // type non proposé (Object, Callable...) : affiché tel quel
        {
            option->add_item(Variant::get_type_name(p_type), (int)p_type);
        }
        option->select(option->get_item_index((int)p_type));

        clear(holder);
        if (p_type == Variant::NIL)
        {
            holder->add_child(make_hint("null"));
            return;
        }
        holder->add_child(build_value_widget(retyped(p_accessor, p_type, String()), p_depth));
    };

    InspectorBinding *binding = InspectorBinding::attach(
        row, p_accessor,
        [show, shown](const Variant &p_value) {
            if ((int)p_value.get_type() != *shown)
            {
                show(p_value.get_type());
            }
        },
        [](const Variant &p_value) { return (uint32_t)p_value.get_type(); });

    option->connect("item_selected", lambda_callable_1(option, [option, binding, show](const Variant &p_index) {
                        Variant::Type type = (Variant::Type)option->get_item_id(p_index);
                        Variant current = binding->get_accessor().get();
                        Variant next;
                        if (type != Variant::NIL)
                        {
                            next = convertible(current.get_type(), type) ? UtilityFunctions::type_convert(current, type) : Variant();
                            if (next.get_type() != type)
                            {
                                next = default_for(type);
                            }
                        }
                        binding->commit(next);
                        show(type);
                    }));
    return labeled(p_accessor, row);
}

/// Type sans widget dédié (Transform, Basis, Packed*Array, Rid...) : texte en lecture seule, mis à jour en direct.
Control *build_fallback(const ValueAccessor &p_accessor)
{
    Label *label = make_hint("");
    InspectorBinding::attach(label, p_accessor, [label](const Variant &p_value) {
        label->set_text(p_value.get_type() == Variant::NIL ? String("<null>") : p_value.stringify());
        label->set_tooltip_text(label->get_text());
    });
    return labeled(p_accessor, label);
}


// ==================== 📋 Membres ====================

Control *make_header(const String &p_text, bool p_category)
{
    VBoxContainer *box = make_vbox();
    Label *label = memnew(Label);
    if (p_category)
    {
        box->add_child(memnew(HSeparator));
        label->set_text(p_text.to_upper());
        label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    }
    else
    {
        label->set_text(p_text);
        label->set_modulate(Color(1, 1, 1, 0.7));
    }
    box->add_child(label);
    return box;
}

void add_properties(Container *p_container, Object *p_target, const Options &p_options)
{
    TypedArray<Dictionary> list = get_members(p_target, MemberKind::PROPERTY, p_options.scope);
    for (int i = 0; i < list.size(); i++)
    {
        Dictionary info = list[i];
        String name = info.get("name", "");
        int64_t usage = info.get("usage", 0);
        if (name.is_empty())
        {
            continue;
        }
        if (usage & (PROPERTY_USAGE_CATEGORY | PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP))
        {
            p_container->add_child(make_header(name, (usage & PROPERTY_USAGE_CATEGORY) != 0));
            continue;
        }
        if ((usage & PROPERTY_USAGE_EDITOR) == 0 || name == "script")
        {
            continue;
        }
        p_container->add_child(build_value_widget(property_accessor(p_target, info), p_options.depth));
    }
}

/// Méthodes ou signaux sans doublon ni membre « privé » (préfixe _), triés par nom.
std::vector<Dictionary> public_members(Object *p_target, MemberKind p_kind, Scope p_scope)
{
    std::vector<Dictionary> result;
    TypedArray<Dictionary> list = get_members(p_target, p_kind, p_scope);
    for (int i = 0; i < list.size(); i++)
    {
        Dictionary info = list[i];
        String name = info.get("name", "");
        if (name.is_empty() || name.begins_with("_"))
        {
            continue;
        }
        bool duplicate = std::any_of(result.begin(), result.end(), [&name](const Dictionary &d) { return String(d["name"]) == name; });
        if (!duplicate)
        {
            result.push_back(info);
        }
    }
    std::sort(result.begin(), result.end(), [](const Dictionary &a, const Dictionary &b) { return String(a["name"]).naturalnocasecmp_to(String(b["name"])) < 0; });
    return result;
}

/// Arguments d'appel : valeurs par défaut déclarées pour les derniers, valeur « vide » du type pour les autres.
Array default_arguments(const Dictionary &p_info)
{
    Array args = p_info.get("args", Array());
    Array defaults = p_info.get("default_args", Array());
    Array result;
    int64_t first_default = args.size() - defaults.size();
    for (int64_t i = 0; i < args.size(); i++)
    {
        result.append(i >= first_default ? defaults[i - first_default] : default_for(type_of(args[i])));
    }
    return result;
}

/// Callable de connexion adapté au nombre d'arguments : les arguments du signal en trop sont ignorés (unbind),
/// ceux qui manquent à la méthode sont liés avec une valeur par défaut (bindv).
Callable connection_callable(Object *p_receiver, const Dictionary &p_method, int64_t p_signal_argc)
{
    Callable callable(p_receiver, StringName(p_method["name"]));
    Array args = p_method.get("args", Array());
    Array defaults = p_method.get("default_args", Array());
    bool vararg = ((int64_t)p_method.get("flags", 0) & METHOD_FLAG_VARARG) != 0;
    int64_t total = args.size();
    int64_t required = total - defaults.size();

    if (!vararg && p_signal_argc > total)
    {
        return callable.unbind(p_signal_argc - total);
    }
    if (p_signal_argc < required)
    {
        Array bound;
        for (int64_t i = p_signal_argc; i < required; i++)
        {
            bound.append(default_for(type_of(args[i])));
        }
        return callable.bindv(bound);
    }
    return callable;
}

} // namespace

// ==================== 🌐 API publique ====================

void clear(Node *p_node)
{
    if (p_node == nullptr)
    {
        return;
    }
    for (int i = p_node->get_child_count() - 1; i >= 0; i--)
    {
        Node *child = p_node->get_child(i);
        p_node->remove_child(child);
        child->queue_free();
    }
}

Node *find_scene_root()
{
    Engine *engine = Engine::get_singleton();
    if (engine->is_editor_hint())
    {
        if (EditorInterface *editor = EditorInterface::get_singleton())
        {
            if (Node *edited = editor->get_edited_scene_root())
            {
                return edited;
            }
        }
    }
    SceneTree *tree = Object::cast_to<SceneTree>(engine->get_main_loop());
    if (tree == nullptr)
    {
        return nullptr;
    }
    return tree->get_current_scene() != nullptr ? tree->get_current_scene() : (Node *)tree->get_root();
}

void notify_edited(Object *p_object)
{
    if (Resource *resource = Object::cast_to<Resource>(p_object))
    {
        resource->emit_changed();
    }
    if (Engine::get_singleton()->is_editor_hint())
    {
        if (EditorInterface *editor = EditorInterface::get_singleton())
        {
            editor->mark_scene_as_unsaved();
        }
    }
}

Variant default_for(Variant::Type p_type)
{
    switch (p_type)
    {
    case Variant::BOOL:
        return false;
    case Variant::INT:
        return (int64_t)0;
    case Variant::FLOAT:
        return 0.0;
    case Variant::STRING:
        return String();
    case Variant::STRING_NAME:
        return StringName();
    case Variant::NODE_PATH:
        return NodePath();
    case Variant::VECTOR2:
        return Vector2();
    case Variant::VECTOR2I:
        return Vector2i();
    case Variant::VECTOR3:
        return Vector3();
    case Variant::VECTOR3I:
        return Vector3i();
    case Variant::VECTOR4:
        return Vector4();
    case Variant::VECTOR4I:
        return Vector4i();
    case Variant::RECT2:
        return Rect2();
    case Variant::RECT2I:
        return Rect2i();
    case Variant::QUATERNION:
        return Quaternion();
    case Variant::PLANE:
        return Plane(Vector3(0, 1, 0), 0);
    case Variant::COLOR:
        return Color(1, 1, 1);
    case Variant::TRANSFORM2D:
        return Transform2D();
    case Variant::TRANSFORM3D:
        return Transform3D();
    case Variant::BASIS:
        return Basis();
    case Variant::ARRAY:
        return Array();
    case Variant::DICTIONARY:
        return Dictionary();
    default:
        return Variant();
    }
}

String signature(const Dictionary &p_info)
{
    PackedStringArray parts;
    Array args = p_info.get("args", Array());
    for (int i = 0; i < args.size(); i++)
    {
        Dictionary arg = args[i];
        parts.append(String(arg.get("name", "")) + ": " + type_text(arg));
    }
    return String(p_info.get("name", "")) + "(" + String(", ").join(parts) + ")";
}

TypedArray<Dictionary> get_members(Object *p_target, MemberKind p_kind, Scope p_scope)
{
    Ref<Script> script = p_target->get_script();
    bool script_only = p_scope == Scope::SCRIPT_ONLY || (p_scope == Scope::AUTO && script.is_valid());

    if (script_only && script.is_valid())
    {
        TypedArray<Dictionary> list = p_kind == MemberKind::METHOD   ? script->get_script_method_list()
                                      : p_kind == MemberKind::SIGNAL ? script->get_script_signal_list()
                                                                     : script->get_script_property_list();
        if (list.size() > 0 || p_scope == Scope::SCRIPT_ONLY)
        {
            return list;
        }
    }

    // Sans script, en mode Auto : méthodes et signaux de la classe elle-même uniquement (pas ceux hérités de
    // Node, Object...). Les propriétés restent complètes, comme dans l'Inspecteur de Godot.
    if (p_scope == Scope::AUTO && p_kind != MemberKind::PROPERTY)
    {
        ClassDBSingleton *class_db = ClassDBSingleton::get_singleton();
        StringName class_name = p_target->get_class();
        TypedArray<Dictionary> own =
            p_kind == MemberKind::METHOD ? class_db->class_get_method_list(class_name, true) : class_db->class_get_signal_list(class_name, true);
        if (own.size() > 0)
        {
            return own;
        }
    }

    return p_kind == MemberKind::METHOD   ? p_target->get_method_list()
           : p_kind == MemberKind::SIGNAL ? p_target->get_signal_list()
                                          : p_target->get_property_list();
}

ValueAccessor property_accessor(Object *p_object, const Dictionary &p_info)
{
    ObjectID id(p_object->get_instance_id());
    StringName name = p_info.get("name", "");

    ValueAccessor accessor;
    accessor.get = [id, name]() {
        Object *object = resolve(id);
        return object != nullptr ? object->get(name) : Variant();
    };
    accessor.set = [id, name](const Variant &p_value) {
        if (Object *object = resolve(id))
        {
            object->set(name, p_value);
            notify_edited(object);
        }
    };
    accessor.is_alive = [id]() { return resolve(id) != nullptr; };
    accessor.info = p_info;
    accessor.label = String(name).capitalize();
    accessor.context = id;
    return accessor;
}

Control *build_value_widget(const ValueAccessor &p_accessor, int p_depth)
{
    Variant::Type type = p_accessor.type();
    int hint = p_accessor.hint();
    switch (type)
    {
    case Variant::BOOL:
        return build_bool(p_accessor);
    case Variant::INT:
        return hint == PROPERTY_HINT_ENUM || hint == PROPERTY_HINT_FLAGS ? build_enum(p_accessor) : build_number(p_accessor);
    case Variant::FLOAT:
        return build_number(p_accessor);
    case Variant::STRING:
    case Variant::STRING_NAME:
        return hint == PROPERTY_HINT_ENUM ? build_enum(p_accessor) : build_string(p_accessor);
    case Variant::COLOR:
        return build_color(p_accessor);
    case Variant::NODE_PATH:
        return build_node_path(p_accessor);
    case Variant::OBJECT:
        return build_object(p_accessor, p_depth);
    case Variant::ARRAY:
    case Variant::DICTIONARY:
        return build_collection(p_accessor, p_depth);
    case Variant::CALLABLE:
        return build_callable(p_accessor);
    case Variant::NIL:
        return (p_accessor.usage() & PROPERTY_USAGE_NIL_IS_VARIANT) ? build_variant(p_accessor, p_depth) : build_fallback(p_accessor);
    default:
    {
        std::vector<Component> components;
        bool is_int = false;
        return struct_components(type, components, is_int) ? build_struct(p_accessor, components, is_int) : build_fallback(p_accessor);
    }
    }
}

Control *build_method_widget(Object *p_object, const Dictionary &p_method_info)
{
    ObjectID id(p_object->get_instance_id());
    StringName name = p_method_info.get("name", "");
    Array arg_infos = p_method_info.get("args", Array());
    // Array partagé (sémantique de référence) : les widgets d'arguments et le bouton voient les mêmes valeurs.
    Array args = default_arguments(p_method_info);

    VBoxContainer *root = make_vbox();
    HBoxContainer *row = make_hbox();
    Button *call = make_button(String(name), signature(p_method_info));
    call->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    Label *result = make_hint("");
    row->add_child(call);
    row->add_child(result);
    root->add_child(row);

    call->connect("pressed", lambda_callable(call, [id, name, args, result]() {
                      Object *object = resolve(id);
                      if (object == nullptr)
                      {
                          return;
                      }
                      Variant returned = object->callv(name, args);
                      result->set_text(returned.get_type() == Variant::NIL ? String(U"✔") : String(U"→ ") + returned.stringify());
                  }));

    if (arg_infos.size() > 0)
    {
        VBoxContainer *arg_box = make_vbox();
        for (int i = 0; i < arg_infos.size(); i++)
        {
            Dictionary arg_info = Dictionary(arg_infos[i]).duplicate();
            ValueAccessor accessor;
            accessor.get = [args, i]() { return args[i]; };
            accessor.set = [args, i](const Variant &p_value) {
                Array shared = args;
                shared[i] = p_value;
            };
            accessor.info = arg_info;
            accessor.info["usage"] = (int64_t)PROPERTY_USAGE_DEFAULT | (type_of(arg_info) == Variant::NIL ? (int64_t)PROPERTY_USAGE_NIL_IS_VARIANT : 0);
            accessor.label = String(arg_info.get("name", "")) + " : " + type_text(arg_info);
            accessor.context = id;
            arg_box->add_child(build_value_widget(accessor, 0));
        }
        root->add_child(indented(arg_box));
    }
    return root;
}

Control *build_signal_widget(Object *p_object, const Dictionary &p_signal_info)
{
    ObjectID id(p_object->get_instance_id());
    StringName signal = p_signal_info.get("name", "");
    int64_t signal_argc = Array(p_signal_info.get("args", Array())).size();

    VBoxContainer *root = make_vbox();

    // ---- en-tête : signature + émission
    HBoxContainer *header = make_hbox();
    Label *title = memnew(Label);
    title->set_text(String(U"📡 ") + signature(p_signal_info));
    title->set_clip_text(true);
    title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    Button *emit = make_button(String(U"▶ Émettre"), String(U"Émet le signal avec des arguments par défaut"));
    header->add_child(title);
    header->add_child(emit);
    root->add_child(header);

    emit->connect("pressed", lambda_callable(emit, [id, signal, p_signal_info]() {
                      if (Object *object = resolve(id))
                      {
                          Array call_args = default_arguments(p_signal_info);
                          call_args.push_front(signal);
                          object->callv("emit_signal", call_args);
                      }
                  }));

    // ---- connexions existantes (rafraîchies si elles changent ailleurs, ex : onglet « Nœud »)
    VBoxContainer *list = make_vbox();
    root->add_child(indented(list));

    ValueAccessor connections;
    connections.get = [id, signal]() {
        Object *object = resolve(id);
        return object != nullptr ? Variant(object->get_signal_connection_list(signal)) : Variant();
    };
    connections.is_alive = [id]() { return resolve(id) != nullptr; };

    auto binding_ref = std::make_shared<InspectorBinding *>(nullptr);
    auto refresh_list = [binding_ref]() {
        if (*binding_ref != nullptr)
        {
            (*binding_ref)->refresh(true);
        }
    };

    // La liaison est rattachée à `root` et non à `list` : `clear(list)` la supprimerait elle-même.
    *binding_ref = InspectorBinding::attach(root, connections, [list, id, signal, refresh_list](const Variant &p_value) {
        clear(list);
        Array entries = p_value;
        for (int i = 0; i < entries.size(); i++)
        {
            Callable callable = Dictionary(entries[i]).get("callable", Callable());
            StringName method = callable.get_method();
            if (method.is_empty()) // lambdas internes (widgets...) : pas une connexion « utilisateur »
            {
                continue;
            }
            Object *receiver = callable.get_object();
            String receiver_text = receiver == resolve(id) ? String("(cet objet)") : describe_object(receiver);

            HBoxContainer *row = make_hbox();
            Label *label = memnew(Label);
            label->set_text(String(U"→ ") + receiver_text + String(U" ▸ ") + String(method) + "()");
            label->set_tooltip_text(callable.is_custom() ? String(U"Arguments liés ou ignorés (bind / unbind)") : String());
            label->set_clip_text(true);
            label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            Button *remove = make_button(String(U"✕"), "Retirer la connexion");
            remove->connect("pressed", lambda_callable(remove, [id, signal, callable, refresh_list]() {
                                if (Object *object = resolve(id))
                                {
                                    if (object->is_connected(signal, callable))
                                    {
                                        object->disconnect(signal, callable);
                                        notify_edited(object);
                                    }
                                }
                                refresh_list();
                            }));
            row->add_child(label);
            row->add_child(remove);
            list->add_child(row);
        }
    });

    // ---- ajout d'une connexion : objet cible (chemin), méthode, bouton
    LineEdit *path = memnew(LineEdit);
    path->set_placeholder("(cet objet)");
    path->set_tooltip_text(String(U"Chemin du nœud à appeler, relatif à l'émetteur (ou à la racine de la scène). Vide : l'émetteur lui-même."));
    path->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    Button *pick = make_button(String(U"🔍"), String(U"Choisir l'objet dans l'arbre de la scène"));
    Label *status = memnew(Label);
    status->set_custom_minimum_size(Vector2(90, 0));
    status->set_clip_text(true);
    OptionButton *methods_option = memnew(OptionButton);
    methods_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    methods_option->set_clip_text(true);
    Button *connect_button = make_button("+ Connecter", String(U"Appeler cette méthode à chaque émission du signal"));

    HBoxContainer *target_row = make_hbox();
    target_row->add_child(path);
    target_row->add_child(pick);
    target_row->add_child(status);
    HBoxContainer *method_row = make_hbox();
    method_row->add_child(methods_option);
    method_row->add_child(connect_button);
    VBoxContainer *add_box = make_vbox();
    add_box->add_child(target_row);
    add_box->add_child(method_row);
    root->add_child(indented(add_box));

    auto methods = std::make_shared<std::vector<Dictionary>>();
    auto resolve_receiver = [id, path]() -> Object * {
        String text = path->get_text().strip_edges();
        if (text.is_empty())
        {
            return resolve(id);
        }
        Node *base = path_base(id);
        return base != nullptr ? base->get_node_or_null(NodePath(text)) : nullptr;
    };
    auto refresh_methods = [resolve_receiver, methods, methods_option, status]() {
        Object *receiver = resolve_receiver();
        status->set_text(receiver != nullptr ? String(U"✔ ") + receiver->get_class() : String(U"✘ introuvable"));
        status->set_modulate(receiver != nullptr ? Color(0.5, 1, 0.5) : Color(1, 0.5, 0.5));
        methods_option->clear();
        methods->clear();
        if (receiver == nullptr)
        {
            return;
        }
        *methods = public_members(receiver, MemberKind::METHOD, Scope::AUTO);
        for (const Dictionary &method : *methods)
        {
            methods_option->add_item(signature(method));
        }
    };

    path->connect("text_submitted", lambda_callable_1(path, [refresh_methods](const Variant &) { refresh_methods(); }));
    path->connect("focus_exited", lambda_callable(path, refresh_methods));
    pick->connect("pressed", lambda_callable(pick, [pick, id, path, refresh_methods]() {
                      pick_node(pick, [id, path, refresh_methods](Node *p_node) {
                          Node *base = path_base(id);
                          bool self = p_node == resolve(id);
                          path->set_text(self || base == nullptr ? String() : String(base->get_path_to(p_node)));
                          refresh_methods();
                      });
                  }));
    connect_button->connect("pressed", lambda_callable(connect_button, [id, signal, signal_argc, methods, methods_option, resolve_receiver, refresh_list]() {
                         Object *source = resolve(id);
                         Object *receiver = resolve_receiver();
                         int index = methods_option->get_selected();
                         if (source == nullptr || receiver == nullptr || index < 0 || index >= (int)methods->size())
                         {
                             UtilityFunctions::push_warning(String(U"[ProceduralInspector] Choisissez un objet valide et une méthode."));
                             return;
                         }
                         Callable callable = connection_callable(receiver, (*methods)[index], signal_argc);
                         if (source->is_connected(signal, callable))
                         {
                             UtilityFunctions::push_warning(String(U"[ProceduralInspector] Cette méthode est déjà connectée au signal."));
                             return;
                         }
                         // Dans l'éditeur : connexion persistante, enregistrée avec la scène.
                         uint32_t flags = Engine::get_singleton()->is_editor_hint() ? (uint32_t)Object::CONNECT_PERSIST : 0u;
                         source->connect(signal, callable, flags);
                         notify_edited(source);
                         refresh_list();
                     }));

    refresh_methods();
    return root;
}

void populate(Container *p_container, Object *p_target, const Options &p_options)
{
    clear(p_container);
    if (p_target == nullptr)
    {
        return;
    }

    if (p_options.properties)
    {
        add_properties(p_container, p_target, p_options);
    }
    if (p_options.methods)
    {
        std::vector<Dictionary> methods = public_members(p_target, MemberKind::METHOD, p_options.scope);
        if (!methods.empty())
        {
            p_container->add_child(make_header(String(U"Méthodes"), true));
        }
        for (const Dictionary &method : methods)
        {
            p_container->add_child(build_method_widget(p_target, method));
        }
    }
    if (p_options.signals)
    {
        std::vector<Dictionary> signals = public_members(p_target, MemberKind::SIGNAL, p_options.scope);
        if (!signals.empty())
        {
            p_container->add_child(make_header("Signaux", true));
        }
        for (const Dictionary &signal : signals)
        {
            p_container->add_child(build_signal_widget(p_target, signal));
        }
    }
}

} // namespace InspectorWidgets
} // namespace godot
