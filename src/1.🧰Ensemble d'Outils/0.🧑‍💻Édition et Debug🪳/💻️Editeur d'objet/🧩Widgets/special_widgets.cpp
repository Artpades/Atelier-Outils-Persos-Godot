#include "special_widgets.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
// Code runtime appelant une API éditeur : uniquement dans l'éditeur ET si le singleton existe.
EditorInterface *editor_interface()
{
    return Engine::get_singleton()->is_editor_hint() ? EditorInterface::get_singleton() : nullptr;
}

Button *make_button(const String &p_text, const String &p_tooltip = String())
{
    Button *button = memnew(Button);
    button->set_text(p_text);
    button->set_tooltip_text(p_tooltip);
    return button;
}
} // namespace

// =====================================================================================================
// NodeSelectorDialog
// =====================================================================================================

void NodeSelectorDialog::open(Node *p_owner, Node *p_root, const PackedStringArray &p_valid_types, const Callable &p_callback)
{
    if (!p_owner || !p_root)
    {
        return;
    }
    NodeSelectorDialog *dialog = memnew(NodeSelectorDialog);
    dialog->set_title(U"Choisir un nœud");
    dialog->on_selected = p_callback;
    dialog->valid_types = p_valid_types;

    dialog->tree = memnew(Tree);
    dialog->tree->set_custom_minimum_size(Vector2(320, 420));
    dialog->add_child(dialog->tree);
    dialog->_add_node(p_root, nullptr);

    dialog->tree->connect("item_activated", callable_mp(dialog, &NodeSelectorDialog::_on_confirmed));
    dialog->connect("confirmed", callable_mp(dialog, &NodeSelectorDialog::_on_confirmed));
    dialog->connect("canceled", callable_mp(dialog, &NodeSelectorDialog::_on_closed));

    p_owner->add_child(dialog);
    dialog->popup_centered();
}

void NodeSelectorDialog::_add_node(Node *p_node, TreeItem *p_parent)
{
    TreeItem *item = tree->create_item(p_parent);
    item->set_text(0, String(p_node->get_name()) + "  (" + p_node->get_class() + ")");
    item->set_metadata(0, (int64_t)p_node->get_instance_id());

    bool valid = valid_types.is_empty();
    for (int i = 0; i < valid_types.size() && !valid; i++)
    {
        valid = p_node->is_class(valid_types[i]);
    }
    item->set_selectable(0, valid);
    if (!valid)
    {
        item->set_custom_color(0, Color(1, 1, 1, 0.4));
    }

    for (int i = 0; i < p_node->get_child_count(); i++)
    {
        _add_node(p_node->get_child(i), item);
    }
}

void NodeSelectorDialog::_on_confirmed()
{
    TreeItem *selected = tree->get_selected();
    Node *node = selected ? Object::cast_to<Node>(ObjectDB::get_instance(uint64_t(int64_t(selected->get_metadata(0))))) : nullptr;
    Callable callback = on_selected;
    on_selected = Callable(); // un seul appel (item_activated puis confirmed)
    if (node && callback.is_valid())
    {
        callback.call(node);
    }
    _on_closed();
}

void NodeSelectorDialog::_on_closed()
{
    hide();
    queue_free();
}

// =====================================================================================================
// PathWidget
// =====================================================================================================

Control *PathWidget::build_editor()
{
    object_mode = int(get_info().get("type", 0)) == Variant::OBJECT;
    bool read_only = is_read_only();

    HBoxContainer *row = memnew(HBoxContainer);
    line = memnew(LineEdit);
    line->set_h_size_flags(SIZE_EXPAND_FILL);
    line->set_editable(!read_only && !object_mode);
    line->connect("text_submitted", callable_mp(this, &PathWidget::_on_text_submitted));
    row->add_child(line);

    Button *pick = make_button(U"…", U"Choisir un nœud");
    pick->set_disabled(read_only);
    pick->connect("pressed", callable_mp(this, &PathWidget::_on_pick));
    row->add_child(pick);

    if (!object_mode && editor_interface())
    {
        Button *pick_property = make_button(U"Propriété…", U"Ajouter le chemin d'une propriété du nœud ciblé");
        pick_property->set_disabled(read_only);
        pick_property->connect("pressed", callable_mp(this, &PathWidget::_on_pick_property));
        row->add_child(pick_property);
    }
    if (object_mode && !read_only)
    {
        Button *clear = make_button(U"✕", U"Remettre à null");
        clear->connect("pressed", callable_mp(this, &PathWidget::_on_clear));
        row->add_child(clear);
    }
    return row;
}

Node *PathWidget::_base_node() const { return get_accessor().is_valid() ? Object::cast_to<Node>(get_accessor()->get_target()) : nullptr; }

PackedStringArray PathWidget::_valid_types() const
{
    Dictionary info = get_info();
    String class_name = info.get("class_name", "");
    if (object_mode && !class_name.is_empty())
    {
        return class_name.split(",", false);
    }
    int hint = info.get("hint", 0);
    if (hint == PROPERTY_HINT_NODE_PATH_VALID_TYPES || hint == PROPERTY_HINT_NODE_TYPE)
    {
        return String(info.get("hint_string", "")).split(",", false);
    }
    return PackedStringArray();
}

void PathWidget::pull_value(const Variant &p_value)
{
    if (!object_mode)
    {
        line->set_text(String(p_value));
        return;
    }
    Node *node = Object::cast_to<Node>(p_value.get_validated_object());
    Node *base = _base_node();
    if (!node)
    {
        line->set_text("<null>");
    }
    else if (base && base->is_inside_tree() && node->is_inside_tree())
    {
        line->set_text(String(base->get_path_to(node)));
    }
    else
    {
        line->set_text(node->get_name());
    }
}

void PathWidget::_apply_node(Node *p_node)
{
    if (!p_node)
    {
        return;
    }
    if (object_mode)
    {
        emit_value(p_node, true);
        return;
    }
    Node *base = _base_node();
    emit_value(base && base->is_inside_tree() ? base->get_path_to(p_node) : p_node->get_path(), true);
}

void PathWidget::_on_pick()
{
    Node *base = _base_node();
    if (EditorInterface *editor = editor_interface())
    {
        TypedArray<StringName> types;
        PackedStringArray valid = _valid_types();
        for (int i = 0; i < valid.size(); i++)
        {
            types.push_back(StringName(valid[i]));
        }
        Node *current = nullptr;
        Variant value = get_accessor()->read();
        if (object_mode)
        {
            current = Object::cast_to<Node>(value.get_validated_object());
        }
        else if (base && base->is_inside_tree() && !NodePath(value).is_empty())
        {
            current = base->get_node_or_null(value);
        }
        editor->popup_node_selector(callable_mp(this, &PathWidget::_on_editor_node_selected), types, current);
        return;
    }
    // En jeu : sélecteur maison sur la scène courante.
    SceneTree *tree = get_tree();
    Node *root = tree ? (tree->get_current_scene() ? tree->get_current_scene() : (Node *)tree->get_root()) : nullptr;
    NodeSelectorDialog::open(this, root, _valid_types(), callable_mp(this, &PathWidget::_on_runtime_node_selected));
}

void PathWidget::_on_editor_node_selected(const NodePath &p_path)
{
    EditorInterface *editor = editor_interface();
    Node *scene_root = editor ? editor->get_edited_scene_root() : nullptr;
    if (p_path.is_empty() || !scene_root)
    {
        return; // annulé
    }
    // Le chemin rendu par le sélecteur est relatif à la racine de la scène éditée.
    _apply_node(scene_root->get_node_or_null(p_path));
}

void PathWidget::_on_runtime_node_selected(Node *p_node) { _apply_node(p_node); }

void PathWidget::_on_pick_property()
{
    EditorInterface *editor = editor_interface();
    Node *base = _base_node();
    String text = String(get_accessor()->read());
    String node_part = text.contains(":") ? text.substr(0, text.find(":")) : text;
    Node *node = (base && base->is_inside_tree()) ? (node_part.is_empty() ? base : base->get_node_or_null(NodePath(node_part))) : nullptr;
    if (!editor || !node)
    {
        log_it(U"[color=orange]Choisir d'abord un nœud valide[/color]", this);
        return;
    }
    editor->popup_property_selector(node, callable_mp(this, &PathWidget::_on_editor_property_selected), PackedInt32Array(), String());
}

void PathWidget::_on_editor_property_selected(const NodePath &p_property)
{
    String property = p_property;
    if (property.is_empty())
    {
        return; // annulé
    }
    String text = String(get_accessor()->read());
    String node_part = text.contains(":") ? text.substr(0, text.find(":")) : text;
    emit_value(NodePath(node_part + (property.begins_with(":") ? property : ":" + property)), true);
}

void PathWidget::_on_text_submitted(const String &p_text)
{
    NodePath path = p_text;
    if (differs_from_object(path))
    {
        emit_value(path, true);
    }
}

void PathWidget::_on_clear() { emit_value(Variant(), true); }

// =====================================================================================================
// VariantWidget
// =====================================================================================================

namespace
{
const Variant::Type SELECTABLE_TYPES[] = {Variant::NIL,
                                          Variant::BOOL,
                                          Variant::INT,
                                          Variant::FLOAT,
                                          Variant::STRING,
                                          Variant::STRING_NAME,
                                          Variant::NODE_PATH,
                                          Variant::VECTOR2,
                                          Variant::VECTOR2I,
                                          Variant::VECTOR3,
                                          Variant::VECTOR3I,
                                          Variant::VECTOR4,
                                          Variant::VECTOR4I,
                                          Variant::RECT2,
                                          Variant::RECT2I,
                                          Variant::QUATERNION,
                                          Variant::PLANE,
                                          Variant::AABB,
                                          Variant::BASIS,
                                          Variant::TRANSFORM2D,
                                          Variant::TRANSFORM3D,
                                          Variant::PROJECTION,
                                          Variant::COLOR,
                                          Variant::ARRAY,
                                          Variant::DICTIONARY,
                                          Variant::PACKED_BYTE_ARRAY,
                                          Variant::PACKED_INT32_ARRAY,
                                          Variant::PACKED_INT64_ARRAY,
                                          Variant::PACKED_FLOAT32_ARRAY,
                                          Variant::PACKED_FLOAT64_ARRAY,
                                          Variant::PACKED_STRING_ARRAY,
                                          Variant::PACKED_VECTOR2_ARRAY,
                                          Variant::PACKED_VECTOR3_ARRAY,
                                          Variant::PACKED_COLOR_ARRAY,
                                          Variant::PACKED_VECTOR4_ARRAY};

String type_label(Variant::Type p_type) { return p_type == Variant::NIL ? String("null") : Variant::get_type_name(p_type); }
} // namespace

bool VariantWidget::convertible(Variant::Type p_from, Variant::Type p_to)
{
    auto scalar = [](Variant::Type t)
    { return t == Variant::BOOL || t == Variant::INT || t == Variant::FLOAT || t == Variant::STRING || t == Variant::STRING_NAME || t == Variant::NODE_PATH; };
    auto pair = [&](Variant::Type x, Variant::Type y) { return (p_from == x && p_to == y) || (p_from == y && p_to == x); };
    auto packed = [](Variant::Type t) { return t >= Variant::PACKED_BYTE_ARRAY && t <= Variant::PACKED_VECTOR4_ARRAY; };
    return (scalar(p_from) && scalar(p_to)) || pair(Variant::VECTOR2, Variant::VECTOR2I) || pair(Variant::VECTOR3, Variant::VECTOR3I) || pair(Variant::VECTOR4, Variant::VECTOR4I) ||
           pair(Variant::RECT2, Variant::RECT2I) || pair(Variant::TRANSFORM3D, Variant::BASIS) || (p_from == Variant::ARRAY && packed(p_to)) || (packed(p_from) && p_to == Variant::ARRAY);
}

Control *VariantWidget::build_editor()
{
    HBoxContainer *row = memnew(HBoxContainer);
    types = memnew(OptionButton);
    types->set_tooltip_text("Type de la valeur");
    types->set_disabled(is_read_only());
    for (Variant::Type type : SELECTABLE_TYPES)
    {
        types->add_item(type_label(type), type);
    }
    types->connect("item_selected", callable_mp(this, &VariantWidget::_on_type_selected));
    row->add_child(types);

    holder = memnew(VBoxContainer);
    holder->set_h_size_flags(SIZE_EXPAND_FILL);
    row->add_child(holder);
    return row;
}

void VariantWidget::pull_value(const Variant &p_value)
{
    if (int(p_value.get_type()) != shown_type)
    {
        _show(p_value.get_type());
    }
}

void VariantWidget::_show(Variant::Type p_type)
{
    shown_type = p_type;
    int index = types->get_item_index(p_type);
    if (index < 0)
    {
        // Type non proposé (Object, Callable...) : affiché tel quel.
        types->add_item(type_label(p_type), p_type);
        index = types->get_item_index(p_type);
    }
    types->select(index);

    TypedArray<Node> children = holder->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        holder->remove_child(child);
        child->queue_free();
    }

    if (p_type == Variant::NIL)
    {
        Label *null_label = memnew(Label);
        null_label->set_text("null");
        holder->add_child(null_label);
        return;
    }

    Dictionary info;
    info["name"] = get_accessor()->get_display_name();
    info["type"] = (int)p_type;
    info["hint"] = 0;
    info["hint_string"] = "";
    info["class_name"] = "";
    info["usage"] = int(get_info().get("usage", (int)PROPERTY_USAGE_DEFAULT)) & ~PROPERTY_USAGE_NIL_IS_VARIANT;

    // Même valeur (même chemin), décrite avec le type choisi ; sans libellé (déjà affiché par ce widget).
    Ref<DataEntry> entry = make_default_entry(info);
    Ref<WidgetSettings> no_label;
    no_label.instantiate();
    no_label->set_show_label(false);
    entry->set_settings(no_label);
    PropertyWidget *widget = build_child(get_accessor()->with_info(info), entry, get_ancestors());
    if (widget)
    {
        holder->add_child(widget);
    }
    else
    {
        Label *depth_note = memnew(Label);
        depth_note->set_text(PropertyWidget::stringify(get_accessor()->read()));
        holder->add_child(depth_note);
    }
}

void VariantWidget::_on_type_selected(int64_t p_index)
{
    Variant::Type type = (Variant::Type)types->get_item_id(p_index);
    Variant current = get_accessor()->read();
    Variant next;
    if (type != Variant::NIL)
    {
        next = convertible(current.get_type(), type) ? UtilityFunctions::type_convert(current, type) : Variant();
        if (next.get_type() != type)
        {
            next = UtilityFunctions::type_convert(Variant(), type);
        }
    }
    emit_value(next, true);
    _show(type);
}

// =====================================================================================================
// ArgumentStore / MethodWidget
// =====================================================================================================

void ArgumentStore::setup(const Dictionary &p_method_info)
{
    Array args = p_method_info.get("args", Array());
    Array defaults = p_method_info.get("default_args", Array());
    int first_default = args.size() - defaults.size();
    values.clear();
    infos.clear();
    for (int i = 0; i < args.size(); i++)
    {
        Dictionary arg = args[i];
        Dictionary info = arg.duplicate();
        info["display"] = arg.get("name", "");
        info["name"] = "arg_" + String::num_int64(i);
        int usage = PROPERTY_USAGE_DEFAULT;
        if (int(arg.get("type", 0)) == Variant::NIL)
        {
            usage |= PROPERTY_USAGE_NIL_IS_VARIANT; // paramètre non typé : valeur Variant
        }
        info["usage"] = usage;
        infos.push_back(info);
        values.push_back(i >= first_default ? defaults[i - first_default] : UtilityFunctions::type_convert(Variant(), int(arg.get("type", 0))));
    }
}

int ArgumentStore::get_count() const { return values.size(); }
Dictionary ArgumentStore::get_info(int p_index) const { return infos[p_index]; }
Array ArgumentStore::get_values() const { return values; }

bool ArgumentStore::_set(const StringName &p_name, const Variant &p_value)
{
    String name = p_name;
    if (!name.begins_with("arg_"))
    {
        return false;
    }
    int index = name.substr(4).to_int();
    if (index < 0 || index >= values.size())
    {
        return false;
    }
    values[index] = p_value;
    return true;
}

bool ArgumentStore::_get(const StringName &p_name, Variant &r_value) const
{
    String name = p_name;
    if (!name.begins_with("arg_"))
    {
        return false;
    }
    int index = name.substr(4).to_int();
    if (index < 0 || index >= values.size())
    {
        return false;
    }
    r_value = values[index];
    return true;
}

void ArgumentStore::_get_property_list(List<PropertyInfo> *p_list) const
{
    for (int i = 0; i < infos.size(); i++)
    {
        Dictionary info = infos[i];
        p_list->push_back(PropertyInfo((Variant::Type)(int)info.get("type", 0), String(info.get("name", "")), (PropertyHint)(int)info.get("hint", 0), info.get("hint_string", ""),
                                       (int)info.get("usage", 0), info.get("class_name", "")));
    }
}

String MethodWidget::signature(const Dictionary &p_method_info)
{
    Array args = p_method_info.get("args", Array());
    Array defaults = p_method_info.get("default_args", Array());
    int first_default = args.size() - defaults.size();
    String text = String(p_method_info.get("name", "")) + "(";
    for (int i = 0; i < args.size(); i++)
    {
        Dictionary arg = args[i];
        int type = arg.get("type", 0);
        text += (i > 0 ? ", " : "") + String(arg.get("name", "")) + ": " + (type == Variant::NIL ? String("Variant") : Variant::get_type_name((Variant::Type)type));
        if (i >= first_default)
        {
            text += " = " + UtilityFunctions::var_to_str(defaults[i - first_default]);
        }
    }
    int return_type = Dictionary(p_method_info.get("return", Dictionary())).get("type", 0);
    return text + ")" + (return_type == Variant::NIL ? String() : " -> " + Variant::get_type_name((Variant::Type)return_type));
}

Control *MethodWidget::build_editor()
{
    Dictionary info = get_info();
    VBoxContainer *box = memnew(VBoxContainer);

    Button *call = make_button(get_accessor()->get_property(), signature(info));
    call->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    call->connect("pressed", callable_mp(this, &MethodWidget::_on_call));
    box->add_child(call);

    // Arguments : valeurs locales (hors historique d'annulation), widgets construits par la fabrique.
    arguments.instantiate();
    arguments->setup(info);
    for (int i = 0; i < arguments->get_count(); i++)
    {
        Ref<ValueAccessor> accessor;
        accessor.instantiate();
        accessor->setup(arguments.ptr(), "arg_" + String::num_int64(i));
        accessor->set_undoable(false);
        accessor->set_display_name(String(arguments->get_info(i).get("display", "")));
        PropertyWidget *widget = build_child(accessor, make_default_entry(accessor->get_property_info()), get_ancestors());
        if (widget)
        {
            box->add_child(widget);
        }
    }

    result = memnew(Label);
    result->set_visible(false);
    result->set_modulate(Color(1, 1, 1, 0.8));
    box->add_child(result);
    return box;
}

void MethodWidget::_on_call()
{
    Object *target = get_accessor()->get_target();
    if (!target)
    {
        return;
    }
    Variant returned = target->callv(get_accessor()->get_property(), arguments->get_values());
    Dictionary return_info = get_info().get("return", Dictionary());
    bool has_return = int(return_info.get("type", 0)) != Variant::NIL || (int(return_info.get("usage", 0)) & PROPERTY_USAGE_NIL_IS_VARIANT) || returned.get_type() != Variant::NIL;
    if (has_return)
    {
        result->set_text(U"→ " + PropertyWidget::stringify(returned));
        result->set_visible(true);
    }
}
