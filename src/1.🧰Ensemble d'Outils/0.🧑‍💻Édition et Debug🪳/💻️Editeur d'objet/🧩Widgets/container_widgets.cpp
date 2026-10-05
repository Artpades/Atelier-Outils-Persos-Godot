#include "container_widgets.hpp"
#include "preview_widget.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/data_registry.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/widget_table.hpp"

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/flow_container.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
Button *make_button(const String &p_text, const String &p_tooltip = String())
{
    Button *button = memnew(Button);
    button->set_text(p_text);
    button->set_tooltip_text(p_tooltip);
    return button;
}

bool is_packed(Variant::Type p_type) { return p_type >= Variant::PACKED_BYTE_ARRAY && p_type <= Variant::PACKED_VECTOR4_ARRAY; }

// Taille d'un Array / Dictionary / Packed*Array (Variant::call n'est pas const : copie, sans coût pour ces types).
int64_t collection_size(const Variant &p_value)
{
    Variant copy = p_value;
    return copy.call("size");
}
} // namespace

// =====================================================================================================
// ContainerWidget
// =====================================================================================================

Control *ContainerWidget::build_editor()
{
    Ref<ContainerWidgetSettings> container_settings = get_settings();

    VBoxContainer *body = memnew(VBoxContainer);
    body->set_h_size_flags(SIZE_EXPAND_FILL);

    switch (get_style())
    {
    case DataEntry::WIDGET_GRID_CONTAINER:
    {
        GridContainer *grid = memnew(GridContainer);
        grid->set_columns(container_settings.is_valid() ? container_settings->get_columns() : 2);
        items = grid;
        break;
    }
    case DataEntry::WIDGET_FLOW_CONTAINER:
    {
        FlowContainer *flow = memnew(FlowContainer);
        flow->set_vertical(is_vertical());
        items = flow;
        break;
    }
    case DataEntry::WIDGET_TAB_CONTAINER:
        items = memnew(TabContainer);
        break;
    default:
    {
        BoxContainer *box = memnew(BoxContainer);
        box->set_vertical(is_vertical());
        items = box;
        break;
    }
    }
    items->set_h_size_flags(SIZE_EXPAND_FILL);
    body->add_child(items);

    footer = memnew(VBoxContainer);
    body->add_child(footer);

    if (container_settings.is_null() || container_settings->get_foldable())
    {
        foldable = memnew(FoldableContainer);
        foldable->set_folded(!(container_settings.is_valid() && container_settings->get_start_expanded()));
        foldable->add_child(body);
        foldable->connect("folding_changed", callable_mp(this, &ContainerWidget::_on_folding_changed));
        return foldable;
    }

    VBoxContainer *outer = memnew(VBoxContainer);
    title_label = memnew(Label);
    outer->add_child(title_label);
    outer->add_child(body);
    return outer;
}

void ContainerWidget::_set_title(const String &p_text)
{
    if (foldable)
    {
        foldable->set_title(p_text);
    }
    else if (title_label)
    {
        title_label->set_text(p_text);
    }
}

String ContainerWidget::summary(const Variant &p_value) const { return Variant::get_type_name(p_value.get_type()); }

String ContainerWidget::structure_key(const Variant &p_value) const { return String::num_int64(p_value.get_type()); }

void ContainerWidget::pull_value(const Variant &p_value)
{
    String text = summary(p_value);
    _set_title(get_accessor().is_valid() ? get_accessor()->get_display_name() + (text.is_empty() ? String() : "  (" + text + ")") : text);

    if (!is_expanded())
    {
        built = false;
        return;
    }
    if (!built || structure_key(p_value) != built_key)
    {
        rebuild_now();
    }
}

bool ContainerWidget::is_expanded() const { return foldable ? !foldable->is_folded() : true; }

int ContainerWidget::get_max_displayed() const
{
    Ref<ContainerWidgetSettings> container_settings = get_settings();
    return container_settings.is_valid() ? container_settings->get_max_displayed() : 100;
}

void ContainerWidget::_on_folding_changed(bool p_folded)
{
    if (p_folded)
    {
        clear_items();
        built = false;
    }
    else
    {
        rebuild_now();
    }
}

void ContainerWidget::clear_items()
{
    Container *containers[] = {items, footer};
    for (Container *container : containers)
    {
        TypedArray<Node> children = container->get_children();
        for (int i = 0; i < children.size(); i++)
        {
            Node *child = Object::cast_to<Node>(children[i]);
            container->remove_child(child);
            child->queue_free();
        }
    }
}

void ContainerWidget::rebuild_now()
{
    clear_items();
    Variant value = get_accessor().is_valid() ? get_accessor()->read() : Variant();
    fill(value);
    built_key = structure_key(value);
    built = true;
}

void ContainerWidget::rebuild_deferred() { callable_mp(this, &ContainerWidget::rebuild_now).call_deferred(); }

void ContainerWidget::add_item(Control *p_control, const String &p_title)
{
    p_control->set_h_size_flags(SIZE_EXPAND_FILL);
    items->add_child(p_control);
    if (TabContainer *tabs = Object::cast_to<TabContainer>(items))
    {
        tabs->set_tab_title(tabs->get_tab_count() - 1, p_title);
    }
}

void ContainerWidget::add_footer(Control *p_control) { footer->add_child(p_control); }

void ContainerWidget::add_note(const String &p_text)
{
    Label *note = memnew(Label);
    note->set_text(p_text);
    note->set_modulate(Color(1, 1, 1, 0.7));
    footer->add_child(note);
}

// =====================================================================================================
// CollectionWidget
// =====================================================================================================

CollectionWidget::ElementDecl CollectionWidget::parse_decl(const String &p_decl)
{
    ElementDecl decl;
    String text = p_decl.strip_edges();
    if (text.is_empty())
    {
        return decl;
    }
    int colon = text.find(":");
    String head = colon >= 0 ? text.substr(0, colon) : text;
    String tail = colon >= 0 ? text.substr(colon + 1) : String();

    if (head.is_valid_int() || head.contains("/"))
    {
        // Format "type[/hint]:hint_string" (PROPERTY_HINT_TYPE_STRING).
        decl.typed = true;
        decl.type = (Variant::Type)head.get_slice("/", 0).to_int();
        decl.hint = head.contains("/") ? (int)head.get_slice("/", 1).to_int() : 0;
        decl.hint_string = tail;
    }
    else
    {
        // Format "NomDeType" (PROPERTY_HINT_ARRAY_TYPE / DICTIONARY_TYPE) : type intégré ou classe.
        decl.typed = true;
        decl.type = Variant::OBJECT;
        decl.hint_string = head;
        for (int t = 0; t < Variant::VARIANT_MAX; t++)
        {
            if (Variant::get_type_name((Variant::Type)t) == head)
            {
                decl.type = (Variant::Type)t;
                decl.hint_string = String();
                break;
            }
        }
        if (decl.type == Variant::OBJECT)
        {
            decl.hint = PROPERTY_HINT_RESOURCE_TYPE;
        }
    }
    if (decl.type == Variant::OBJECT)
    {
        decl.class_name = decl.hint_string;
    }
    return decl;
}

Variant::Type CollectionWidget::packed_element_type(Variant::Type p_packed_type)
{
    switch (p_packed_type)
    {
    case Variant::PACKED_BYTE_ARRAY:
    case Variant::PACKED_INT32_ARRAY:
    case Variant::PACKED_INT64_ARRAY:
        return Variant::INT;
    case Variant::PACKED_FLOAT32_ARRAY:
    case Variant::PACKED_FLOAT64_ARRAY:
        return Variant::FLOAT;
    case Variant::PACKED_STRING_ARRAY:
        return Variant::STRING;
    case Variant::PACKED_VECTOR2_ARRAY:
        return Variant::VECTOR2;
    case Variant::PACKED_VECTOR3_ARRAY:
        return Variant::VECTOR3;
    case Variant::PACKED_COLOR_ARRAY:
        return Variant::COLOR;
    case Variant::PACKED_VECTOR4_ARRAY:
        return Variant::VECTOR4;
    default:
        return Variant::NIL;
    }
}

Dictionary CollectionWidget::element_info(const ElementDecl &p_decl, const String &p_name)
{
    Dictionary info;
    info["name"] = p_name;
    if (p_decl.typed)
    {
        info["type"] = (int)p_decl.type;
        info["hint"] = p_decl.hint;
        info["hint_string"] = p_decl.hint_string;
        info["class_name"] = p_decl.class_name;
        info["usage"] = (int)PROPERTY_USAGE_DEFAULT;
    }
    else
    {
        // Élément non typé : valeur Variant (type au choix).
        info["type"] = (int)Variant::NIL;
        info["hint"] = 0;
        info["hint_string"] = "";
        info["class_name"] = "";
        info["usage"] = (int)(PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
    }
    return info;
}

Variant CollectionWidget::default_value(const ElementDecl &p_decl)
{
    if (!p_decl.typed || p_decl.type == Variant::OBJECT || p_decl.type == Variant::NIL)
    {
        return Variant();
    }
    return UtilityFunctions::type_convert(Variant(), p_decl.type);
}

Control *CollectionWidget::build_editor()
{
    Dictionary info = get_info();
    collection_type = (Variant::Type)(int)info.get("type", 0);
    int hint = info.get("hint", 0);
    String hint_string = info.get("hint_string", "");

    if (collection_type == Variant::ARRAY && (hint == PROPERTY_HINT_TYPE_STRING || hint == PROPERTY_HINT_ARRAY_TYPE))
    {
        element_decl = parse_decl(hint_string);
    }
    else if (collection_type == Variant::DICTIONARY && (hint == PROPERTY_HINT_TYPE_STRING || hint == PROPERTY_HINT_DICTIONARY_TYPE))
    {
        key_decl = parse_decl(hint_string.get_slice(";", 0));
        element_decl = parse_decl(hint_string.get_slice(";", 1));
    }
    else if (is_packed(collection_type))
    {
        element_decl.typed = true;
        element_decl.type = packed_element_type(collection_type);
    }
    return ContainerWidget::build_editor();
}

String CollectionWidget::summary(const Variant &p_value) const
{
    Variant::Type type = p_value.get_type();
    if (type == Variant::NIL)
    {
        return "null";
    }
    int64_t size = collection_size(p_value);
    return Variant::get_type_name(type) + U" · " + String::num_int64(size);
}

String CollectionWidget::structure_key(const Variant &p_value) const
{
    Variant::Type type = p_value.get_type();
    String key = String::num_int64(type);
    if (type == Variant::ARRAY)
    {
        Array array = p_value;
        key += ":" + String::num_int64(array.size());
        for (int i = 0; i < array.size(); i++)
        {
            key += "," + String::num_int64(array[i].get_type());
        }
    }
    else if (type == Variant::DICTIONARY)
    {
        Dictionary dictionary = p_value;
        Array keys = dictionary.keys();
        for (int i = 0; i < keys.size(); i++)
        {
            key += ";" + PropertyWidget::stringify(keys[i]) + "=" + String::num_int64(dictionary[keys[i]].get_type());
        }
    }
    else if (is_packed(type))
    {
        key += ":" + String::num_int64(collection_size(p_value));
    }
    return key;
}

void CollectionWidget::fill(const Variant &p_value)
{
    Variant::Type type = p_value.get_type();
    bool read_only = is_read_only();
    Ref<ValueAccessor> accessor = get_accessor();

    if (type == Variant::NIL)
    {
        if (!read_only && (collection_type == Variant::ARRAY || collection_type == Variant::DICTIONARY))
        {
            Button *create = make_button(collection_type == Variant::ARRAY ? U"Créer le tableau" : U"Créer le dictionnaire");
            create->connect("pressed", callable_mp(this, &CollectionWidget::_on_create));
            add_footer(create);
        }
        else
        {
            add_note("null");
        }
        return;
    }

    int max_displayed = get_max_displayed();
    if (type == Variant::DICTIONARY)
    {
        Dictionary dictionary = p_value;
        Array keys = dictionary.keys();
        for (int i = 0; i < keys.size() && i < max_displayed; i++)
        {
            Variant key = keys[i];
            String text = PropertyWidget::stringify(key);
            Dictionary info = element_info(element_decl, text);
            PropertyWidget *widget = build_child(accessor->child(key, info, text), make_default_entry(info), get_ancestors());

            HBoxContainer *row = memnew(HBoxContainer);
            if (widget)
            {
                widget->set_h_size_flags(SIZE_EXPAND_FILL);
                row->add_child(widget);
            }
            else
            {
                Label *depth_note = memnew(Label);
                depth_note->set_text(text + U" : … (profondeur maximale)");
                row->add_child(depth_note);
            }
            if (!read_only)
            {
                Button *remove = make_button(U"✕", U"Supprimer l'entrée");
                remove->connect("pressed", callable_mp(this, &CollectionWidget::_on_remove).bind(key));
                row->add_child(remove);
            }
            add_item(row, text);
        }
        if (keys.size() > max_displayed)
        {
            add_note(U"… " + String::num_int64(keys.size() - max_displayed) + U" autres entrées");
        }
        if (!read_only)
        {
            HBoxContainer *add_row = memnew(HBoxContainer);
            new_key = memnew(LineEdit);
            new_key->set_placeholder(U"Nouvelle clé");
            new_key->set_h_size_flags(SIZE_EXPAND_FILL);
            add_row->add_child(new_key);
            Button *add = make_button("+ Ajouter");
            add->connect("pressed", callable_mp(this, &CollectionWidget::_on_add));
            add_row->add_child(add);
            add_footer(add_row);
        }
        return;
    }

    // Array / Packed*Array
    int64_t size = collection_size(p_value);
    for (int64_t i = 0; i < size && i < max_displayed; i++)
    {
        String text = "[" + String::num_int64(i) + "]";
        Dictionary info = element_info(element_decl, text);
        PropertyWidget *widget = build_child(accessor->child(i, info, text), make_default_entry(info), get_ancestors());

        HBoxContainer *row = memnew(HBoxContainer);
        if (widget)
        {
            widget->set_h_size_flags(SIZE_EXPAND_FILL);
            row->add_child(widget);
        }
        else
        {
            Label *depth_note = memnew(Label);
            depth_note->set_text(text + U" : … (profondeur maximale)");
            row->add_child(depth_note);
        }
        if (!read_only)
        {
            Button *up = make_button(U"▲", "Monter");
            up->set_disabled(i == 0);
            up->connect("pressed", callable_mp(this, &CollectionWidget::_on_move).bind(i, i - 1));
            Button *down = make_button(U"▼", "Descendre");
            down->set_disabled(i == size - 1);
            down->connect("pressed", callable_mp(this, &CollectionWidget::_on_move).bind(i, i + 1));
            Button *remove = make_button(U"✕", U"Supprimer l'élément");
            remove->connect("pressed", callable_mp(this, &CollectionWidget::_on_remove).bind(i));
            row->add_child(up);
            row->add_child(down);
            row->add_child(remove);
        }
        add_item(row, text);
    }
    if (size > max_displayed)
    {
        add_note(U"… " + String::num_int64(size - max_displayed) + U" autres éléments");
    }
    if (!read_only)
    {
        Button *add = make_button(U"+ Ajouter un élément");
        add->connect("pressed", callable_mp(this, &CollectionWidget::_on_add));
        add_footer(add);
    }
}

Variant CollectionWidget::_current_copy() const { return ValueAccessor::copy_collection(get_accessor()->read()); }

void CollectionWidget::_on_move(int64_t p_from, int64_t p_to)
{
    Variant collection = _current_copy();
    int64_t size = collection.call("size");
    if (p_from < 0 || p_to < 0 || p_from >= size || p_to >= size)
    {
        return;
    }
    Variant moved = collection.get(p_from);
    collection.set(p_from, collection.get(p_to));
    collection.set(p_to, moved);
    emit_value(collection, true);
    rebuild_deferred();
}

void CollectionWidget::_on_remove(const Variant &p_key)
{
    Variant collection = _current_copy();
    if (collection.get_type() == Variant::DICTIONARY)
    {
        collection.call("erase", p_key);
    }
    else
    {
        collection.call("remove_at", p_key);
    }
    emit_value(collection, true);
    rebuild_deferred();
}

void CollectionWidget::_on_add()
{
    Variant collection = _current_copy();
    if (collection.get_type() == Variant::DICTIONARY)
    {
        String text = new_key ? new_key->get_text().strip_edges() : String();
        Variant key;
        if (key_decl.typed && key_decl.type == Variant::STRING)
        {
            key = text;
        }
        else if (key_decl.typed && key_decl.type == Variant::STRING_NAME)
        {
            key = StringName(text);
        }
        else
        {
            // Saisie au format de var_to_str (ex: 12, "texte", Vector2(1, 2)) ; un texte simple reste une chaîne.
            key = UtilityFunctions::str_to_var(text);
            if (key.get_type() == Variant::NIL && !text.is_empty())
            {
                key = text;
            }
            if (key_decl.typed)
            {
                key = UtilityFunctions::type_convert(key, key_decl.type);
            }
        }
        if (bool(collection.call("has", key)))
        {
            log_it(U"[color=orange]La clé " + PropertyWidget::stringify(key) + U" existe déjà[/color]", this);
            return;
        }
        collection.set(key, default_value(element_decl));
    }
    else
    {
        collection.call("append", default_value(element_decl));
    }
    emit_value(collection, true);
    rebuild_deferred();
}

void CollectionWidget::_on_create()
{
    Variant created;
    if (collection_type == Variant::ARRAY)
    {
        created = element_decl.typed ? Array(Array(), element_decl.type, StringName(element_decl.class_name), Variant()) : Array();
    }
    else if (key_decl.typed || element_decl.typed)
    {
        Variant::Type key_type = key_decl.typed ? key_decl.type : Variant::NIL;
        Variant::Type value_type = element_decl.typed ? element_decl.type : Variant::NIL;
        created = Dictionary(Dictionary(), key_type, StringName(key_decl.class_name), Variant(), value_type, StringName(element_decl.class_name), Variant());
    }
    else
    {
        created = Dictionary();
    }
    emit_value(created, true);
    rebuild_deferred();
}

// =====================================================================================================
// ObjectWidget
// =====================================================================================================

void ObjectWidget::_notification(int p_what)
{
    if (p_what == NOTIFICATION_PREDELETE)
    {
        _watch(nullptr);
    }
}

bool ObjectWidget::_is_path_mode() const { return int(get_info().get("type", 0)) == Variant::NODE_PATH; }

Object *ObjectWidget::_resolve(const Variant &p_value) const
{
    if (_is_path_mode())
    {
        Node *base = get_accessor().is_valid() ? Object::cast_to<Node>(get_accessor()->get_target()) : nullptr;
        NodePath path = p_value;
        return (base && base->is_inside_tree() && !path.is_empty()) ? base->get_node_or_null(path) : nullptr;
    }
    // Objet libéré : get_validated_object() renvoie nullptr au lieu d'un pointeur invalide.
    return p_value.get_type() == Variant::OBJECT ? p_value.get_validated_object() : nullptr;
}

String ObjectWidget::summary(const Variant &p_value) const
{
    Object *object = _resolve(p_value);
    if (!object)
    {
        return _is_path_mode() ? (String(p_value).is_empty() ? String("vide") : String(p_value) + " : introuvable") : String("<null>");
    }
    Node *node = Object::cast_to<Node>(object);
    return node ? String(node->get_name()) + " : " + node->get_class() : object->get_class();
}

String ObjectWidget::structure_key(const Variant &p_value) const
{
    Object *object = _resolve(p_value);
    return object ? String::num_uint64(object->get_instance_id()) : String("null");
}

PackedStringArray ObjectWidget::_base_classes() const
{
    Dictionary info = get_info();
    PackedStringArray bases;
    String class_name = info.get("class_name", "");
    if (!class_name.is_empty())
    {
        bases.push_back(class_name);
    }
    else if (int(info.get("hint", 0)) == PROPERTY_HINT_RESOURCE_TYPE)
    {
        bases = String(info.get("hint_string", "")).split(",", false);
    }
    if (bases.is_empty())
    {
        bases.push_back("Resource");
    }
    return bases;
}

PackedStringArray ObjectWidget::instantiable_classes(const PackedStringArray &p_bases)
{
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    Dictionary found;

    // Classes natives (moteur et GDExtension).
    for (int b = 0; b < p_bases.size(); b++)
    {
        String base = p_bases[b].strip_edges();
        if (!cdb->class_exists(base))
        {
            continue;
        }
        PackedStringArray candidates = cdb->get_inheriters_from_class(base);
        candidates.push_back(base);
        for (int i = 0; i < candidates.size(); i++)
        {
            if (cdb->can_instantiate(candidates[i]) && !cdb->is_parent_class(candidates[i], "Node"))
            {
                found[candidates[i]] = true;
            }
        }
    }

    // Classes de script globales (class_name) : absentes de ClassDB, héritage suivi via "base".
    TypedArray<Dictionary> globals = ProjectSettings::get_singleton()->get_global_class_list();
    Dictionary base_of;
    Dictionary path_of;
    for (int i = 0; i < globals.size(); i++)
    {
        Dictionary global = globals[i];
        base_of[global.get("class", "")] = global.get("base", "");
        path_of[global.get("class", "")] = global.get("path", "");
    }
    Array script_classes = base_of.keys();
    for (int i = 0; i < script_classes.size(); i++)
    {
        String name = script_classes[i];
        bool inherits = false;
        for (String current = name; !current.is_empty() && !inherits;)
        {
            if (p_bases.has(current))
            {
                inherits = true;
            }
            else if (base_of.has(current))
            {
                current = String(base_of[current]);
            }
            else
            {
                for (int b = 0; b < p_bases.size() && !inherits; b++)
                {
                    inherits = cdb->class_exists(current) && cdb->is_parent_class(current, p_bases[b]);
                }
                break;
            }
        }
        if (!inherits)
        {
            continue;
        }
        Ref<Script> script = ResourceLoader::get_singleton()->load(path_of[name]);
        if (script.is_valid() && script->can_instantiate() && !cdb->is_parent_class(script->get_instance_base_type(), "Node"))
        {
            found[name] = true;
        }
    }

    PackedStringArray result;
    Array names = found.keys();
    names.sort();
    for (int i = 0; i < names.size(); i++)
    {
        result.push_back(names[i]);
    }
    return result;
}

Variant ObjectWidget::instantiate_class(const String &p_class_name)
{
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    if (cdb->class_exists(p_class_name))
    {
        return cdb->instantiate(p_class_name); // un RefCounted reste vivant grâce au Variant
    }
    TypedArray<Dictionary> globals = ProjectSettings::get_singleton()->get_global_class_list();
    for (int i = 0; i < globals.size(); i++)
    {
        Dictionary global = globals[i];
        if (String(global.get("class", "")) == p_class_name)
        {
            Ref<Script> script = ResourceLoader::get_singleton()->load(global.get("path", ""));
            return script.is_valid() ? script->call("new") : Variant();
        }
    }
    return Variant();
}

void ObjectWidget::_watch(Object *p_object)
{
    Callable callback = callable_mp(this, &ObjectWidget::_on_property_list_changed);
    if (Object *previous = ObjectDB::get_instance(watched_id))
    {
        if (previous->is_connected("property_list_changed", callback))
        {
            previous->disconnect("property_list_changed", callback);
        }
    }
    watched_id = ObjectID();
    if (p_object)
    {
        p_object->connect("property_list_changed", callback);
        watched_id = ObjectID(p_object->get_instance_id());
    }
}

void ObjectWidget::_on_property_list_changed() { rebuild_deferred(); }

void ObjectWidget::fill(const Variant &p_value)
{
    Object *object = _resolve(p_value);
    bool read_only = is_read_only();
    _watch(object);

    if (!object)
    {
        if (_is_path_mode())
        {
            add_note(String(p_value).is_empty() ? String("Aucun chemin") : U"Nœud introuvable : " + String(p_value));
            return;
        }
        PackedStringArray classes = read_only ? PackedStringArray() : instantiable_classes(_base_classes());
        if (classes.is_empty())
        {
            add_note("<null>");
            return;
        }
        HBoxContainer *row = memnew(HBoxContainer);
        class_choice = memnew(OptionButton);
        for (int i = 0; i < classes.size(); i++)
        {
            class_choice->add_item(classes[i]);
        }
        class_choice->set_h_size_flags(SIZE_EXPAND_FILL);
        row->add_child(class_choice);
        Button *create = make_button("Nouveau", "Instancier la classe choisie");
        create->connect("pressed", callable_mp(this, &ObjectWidget::_on_create));
        row->add_child(create);
        add_footer(row);
        return;
    }

    uint64_t id = object->get_instance_id();
    if (get_ancestors().has((int64_t)id))
    {
        add_note(U"↺ Objet déjà affiché plus haut (référence circulaire)");
        return;
    }
    if (!read_only && !_is_path_mode())
    {
        Button *clear = make_button("Effacer", U"Remettre la valeur à null");
        clear->connect("pressed", callable_mp(this, &ObjectWidget::_on_clear));
        add_footer(clear);
    }

    // Interface du type de l'objet (LayoutDatabase), sinon registre automatique.
    Ref<LayoutDatabase> database = get_context().is_valid() ? get_context()->get_layout_database() : Ref<LayoutDatabase>();
    Ref<TypeLayout> layout = database.is_valid() ? database->find_layout(object) : Ref<TypeLayout>();
    Ref<DataRegistry> registry;
    if (layout.is_valid())
    {
        layout->ensure_registries();
        registry = layout->get_data_registry();
    }
    else
    {
        registry.instantiate();
    }
    registry->sync(object);
    PackedInt64Array chain = get_ancestors();
    chain.push_back((int64_t)id);

    int shown = 0;
    TypedArray<DataEntry> entries = registry->get_entries();
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<DataEntry> entry = entries[i];
        // Propriétés, et aperçus de l'interface du type (aperçu automatique pour tout objet de ce type).
        bool preview = entry.is_valid() && entry->get_member() == DataEntry::MEMBER_PREVIEW;
        if (entry.is_null() || !entry->get_enabled() || entry->get_is_missing() || (entry->get_member() != DataEntry::MEMBER_PROPERTY && !preview))
        {
            continue;
        }
        Ref<ValueAccessor> accessor;
        accessor.instantiate();
        accessor->setup(object, entry->get_data_name());
        if (preview)
        {
            accessor->set_property_info(PreviewWidget::make_info(entry->get_data_name()));
        }
        if (accessor->get_property_info().is_empty())
        {
            continue;
        }
        PropertyWidget *widget = build_child(accessor, entry, chain);
        if (!widget)
        {
            add_note(U"… profondeur maximale atteinte");
            break;
        }
        add_item(widget, accessor->get_display_name());
        shown++;
    }
    if (shown == 0)
    {
        add_note(U"Aucune propriété affichable");
    }
}

void ObjectWidget::_on_create()
{
    if (!class_choice || class_choice->get_selected() < 0)
    {
        return;
    }
    String class_name = class_choice->get_item_text(class_choice->get_selected());
    Variant created = instantiate_class(class_name);
    if (created.get_type() != Variant::OBJECT)
    {
        log_it(U"[color=red]Impossible d'instancier « " + class_name + U" »[/color]", this);
        return;
    }
    emit_value(created, true);
    rebuild_deferred();
}

void ObjectWidget::_on_clear()
{
    emit_value(Variant(), true);
    rebuild_deferred();
}

// =====================================================================================================
// TableWidget
// =====================================================================================================

PackedStringArray TableWidget::columns_for(const Array &p_elements, const PackedStringArray &p_filter)
{
    if (!p_filter.is_empty())
    {
        return p_filter;
    }
    PackedStringArray columns;
    bool value_column = false;
    for (int i = 0; i < p_elements.size(); i++)
    {
        Variant element = p_elements[i];
        if (Object *object = element.get_validated_object())
        {
            TypedArray<Dictionary> members = DataRegistry::get_members(object, DataEntry::MEMBER_PROPERTY, DataEntry::SCOPE_AUTO);
            for (int m = 0; m < members.size(); m++)
            {
                Dictionary member = members[m];
                String name = member.get("name", "");
                // Catégories et groupes (« Stat.gd », @export_group) : pas des propriétés.
                if ((int)member.get("usage", 0) & (PROPERTY_USAGE_CATEGORY | PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP))
                {
                    continue;
                }
                if (!name.is_empty() && !columns.has(name))
                {
                    columns.push_back(name);
                }
            }
        }
        else if (element.get_type() == Variant::DICTIONARY)
        {
            Array keys = Dictionary(element).keys();
            for (int k = 0; k < keys.size(); k++)
            {
                String name = PropertyWidget::stringify(keys[k]);
                if (!columns.has(name))
                {
                    columns.push_back(name);
                }
            }
        }
        else if (element.get_type() != Variant::NIL && element.get_type() != Variant::OBJECT)
        {
            value_column = true;
        }
    }
    if (value_column || columns.is_empty())
    {
        columns.insert(0, VALUE_COLUMN);
    }
    return columns;
}

String TableWidget::structure_key(const Variant &p_value) const
{
    // Structure de la collection + identité de chaque élément objet + clés des dictionnaires.
    String key = CollectionWidget::structure_key(p_value);
    if (p_value.get_type() != Variant::ARRAY)
    {
        return key;
    }
    Array elements = p_value;
    for (int i = 0; i < elements.size(); i++)
    {
        Variant element = elements[i];
        Object *object = element.get_validated_object();
        key += "|" + (object                                     ? String::num_int64((int64_t)object->get_instance_id())
                      : element.get_type() == Variant::DICTIONARY ? Variant(Dictionary(element).keys()).stringify()
                                                                  : String::num_int64(element.get_type()));
    }
    return key;
}

Control *TableWidget::_make_element_header(int64_t p_index, int64_t p_size, bool p_transposed)
{
    HBoxContainer *header = memnew(HBoxContainer);
    Label *index = memnew(Label);
    index->set_text("[" + String::num_int64(p_index) + "]");
    header->add_child(index);
    if (!is_read_only())
    {
        Button *before = make_button(p_transposed ? U"◀" : U"▲", p_transposed ? String(U"Vers la gauche") : String("Monter"));
        before->set_disabled(p_index == 0);
        before->connect("pressed", callable_mp(static_cast<CollectionWidget *>(this), &TableWidget::_on_move).bind(p_index, p_index - 1));
        Button *after = make_button(p_transposed ? U"▶" : U"▼", p_transposed ? String(U"Vers la droite") : String("Descendre"));
        after->set_disabled(p_index == p_size - 1);
        after->connect("pressed", callable_mp(static_cast<CollectionWidget *>(this), &TableWidget::_on_move).bind(p_index, p_index + 1));
        Button *remove = make_button(U"✕", U"Supprimer l'élément");
        remove->connect("pressed", callable_mp(static_cast<CollectionWidget *>(this), &TableWidget::_on_remove).bind(p_index));
        header->add_child(before);
        header->add_child(after);
        header->add_child(remove);
    }
    return header;
}

Control *TableWidget::_make_cell(int64_t p_index, const Variant &p_element, const String &p_column, const PackedInt64Array &p_chain)
{
    Ref<ValueAccessor> accessor = get_accessor();
    String index_text = "[" + String::num_int64(p_index) + "]";
    Ref<ValueAccessor> cell;
    PackedInt64Array chain = p_chain;
    Object *object = p_element.get_validated_object();
    if (p_column == VALUE_COLUMN)
    {
        // Élément lui-même (valeur simple, ou objet nul : « Nouveau… »).
        if (object || p_element.get_type() == Variant::DICTIONARY)
        {
            return nullptr;
        }
        cell = accessor->child(p_index, element_info(element_decl, index_text), index_text);
    }
    else if (object)
    {
        if (get_ancestors().has((int64_t)object->get_instance_id()))
        {
            Label *cycle = memnew(Label);
            cycle->set_text(U"↺");
            return cycle;
        }
        cell.instantiate();
        cell->setup(object, p_column);
        if (cell->get_property_info().is_empty())
        {
            return nullptr;
        }
        chain.push_back((int64_t)object->get_instance_id());
    }
    else if (p_element.get_type() == Variant::DICTIONARY)
    {
        Dictionary dictionary = p_element;
        Variant key;
        Array keys = dictionary.keys();
        bool found = false;
        for (int k = 0; k < keys.size() && !found; k++)
        {
            if (PropertyWidget::stringify(keys[k]) == p_column)
            {
                key = keys[k];
                found = true;
            }
        }
        if (!found)
        {
            return nullptr;
        }
        Dictionary value_info;
        value_info["name"] = p_column;
        value_info["type"] = dictionary[key].get_type();
        cell = accessor->child(p_index, element_info(element_decl, index_text), index_text)->child(key, value_info, p_column);
    }
    else if (p_element.get_type() == Variant::NIL && p_column == String())
    {
        cell = accessor->child(p_index, element_info(element_decl, index_text), index_text);
    }
    else
    {
        return nullptr;
    }
    Ref<DataEntry> entry = make_default_entry(cell->get_property_info());
    Ref<WidgetSettings> compact;
    compact.instantiate();
    compact->set_show_label(false);
    entry->set_settings(compact);
    PropertyWidget *widget = build_child(cell, entry, chain);
    if (!widget)
    {
        Label *depth = memnew(Label);
        depth->set_text(U"…");
        return depth;
    }
    widget->set_h_size_flags(SIZE_EXPAND_FILL);
    return widget;
}

void TableWidget::fill(const Variant &p_value)
{
    if (p_value.get_type() != Variant::ARRAY)
    {
        CollectionWidget::fill(p_value); // null (« Créer le tableau ») ou type inattendu
        return;
    }
    Array all = p_value;
    int64_t size = all.size();
    int64_t shown = MIN(size, (int64_t)get_max_displayed());
    Array elements = all.slice(0, shown);
    Ref<TableWidgetSettings> table_settings = get_settings();
    bool transposed = table_settings.is_valid() && table_settings->get_transpose();
    PackedStringArray columns = columns_for(elements, table_settings.is_valid() ? table_settings->get_table_columns() : PackedStringArray());
    PackedInt64Array chain = get_ancestors();

    auto header_label = [](const String &p_text) {
        Label *label = memnew(Label);
        label->set_text(p_text);
        label->set_modulate(Color(1, 1, 1, 0.75));
        return label;
    };
    auto column_title = [](const String &p_column) { return p_column == VALUE_COLUMN ? String("valeur") : PropertyWidget::display_name(p_column); };
    auto cell_or_dash = [&](int64_t p_index, const String &p_column) -> Control * {
        Variant element = elements[p_index];
        Control *cell = _make_cell(p_index, element, p_column, chain);
        // Élément nul : « Nouveau… » (widget de l'élément) dans la première colonne.
        if (!cell && element.get_type() == Variant::NIL && p_column == columns[0])
        {
            cell = _make_cell(p_index, element, String(), chain);
        }
        if (!cell)
        {
            Label *dash = memnew(Label);
            dash->set_text(U"—");
            dash->set_modulate(Color(1, 1, 1, 0.4));
            dash->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
            cell = dash;
        }
        return cell;
    };

    GridContainer *grid = memnew(GridContainer);
    grid->set_h_size_flags(SIZE_EXPAND_FILL);
    if (!transposed)
    {
        // Une ligne par élément, une colonne par propriété.
        grid->set_columns(columns.size() + 1);
        grid->add_child(memnew(Control));
        for (const String &column : columns)
        {
            grid->add_child(header_label(column_title(column)));
        }
        for (int64_t i = 0; i < shown; i++)
        {
            grid->add_child(_make_element_header(i, size, false));
            for (const String &column : columns)
            {
                grid->add_child(cell_or_dash(i, column));
            }
        }
    }
    else
    {
        // Une ligne par propriété, une colonne par élément.
        grid->set_columns(shown + 1);
        grid->add_child(memnew(Control));
        for (int64_t i = 0; i < shown; i++)
        {
            grid->add_child(_make_element_header(i, size, true));
        }
        for (const String &column : columns)
        {
            grid->add_child(header_label(column_title(column)));
            for (int64_t i = 0; i < shown; i++)
            {
                grid->add_child(cell_or_dash(i, column));
            }
        }
    }
    add_item(grid, String());
    if (size > shown)
    {
        add_note(U"… " + String::num_int64(size - shown) + U" autres éléments");
    }
    if (!is_read_only())
    {
        Button *add = make_button(U"+ Ajouter un élément");
        add->connect("pressed", callable_mp(static_cast<CollectionWidget *>(this), &TableWidget::_on_add));
        add_footer(add);
    }
}
