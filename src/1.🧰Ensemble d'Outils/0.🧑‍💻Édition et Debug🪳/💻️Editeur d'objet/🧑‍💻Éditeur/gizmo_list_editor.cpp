#include "gizmo_list_editor.hpp"
#include "editor_undo_backend.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/container_widgets.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧱Construction/procedural_editor.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

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

Label *make_label(const String &p_text, float p_alpha = 1.0f)
{
    Label *label = memnew(Label);
    label->set_text(p_text);
    label->set_modulate(Color(1, 1, 1, p_alpha));
    return label;
}

String member_type_name(const Dictionary &p_member)
{
    int type = p_member.get("type", 0);
    String class_name = p_member.get("class_name", "");
    if (type == Variant::OBJECT && !class_name.is_empty())
    {
        return class_name;
    }
    return type == Variant::NIL ? String("Variant") : Variant::get_type_name((Variant::Type)type);
}
} // namespace

void GizmoListEditor::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("refresh"), &GizmoListEditor::refresh);
    ClassDB::bind_method(D_METHOD("set_context_object", "object"), &GizmoListEditor::set_context_object);
    ClassDB::bind_method(D_METHOD("get_context_object"), &GizmoListEditor::get_context_object);
    ClassDB::bind_method(D_METHOD("set_context_type", "type"), &GizmoListEditor::set_context_type);
    ClassDB::bind_method(D_METHOD("get_context_type"), &GizmoListEditor::get_context_type);
    ADD_SIGNAL(MethodInfo("registry_changed"));
}

GizmoListEditor::GizmoListEditor()
{
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_h_size_flags(SIZE_EXPAND_FILL);
    backend = Ref<UndoBackend>(memnew(EditorUndoBackend));

    Button *add = make_button("+ Gizmo", U"Ajouter un dessin automatique");
    add->set_h_size_flags(SIZE_SHRINK_BEGIN);
    add->connect("pressed", callable_mp(this, &GizmoListEditor::_on_add));
    add_child(add);

    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    add_child(scroll);
    list = memnew(VBoxContainer);
    list->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->add_child(list);
}

void GizmoListEditor::_notification(int p_what)
{
    if (p_what == NOTIFICATION_PREDELETE)
    {
        _unwatch_all();
    }
}

void GizmoListEditor::set_registry(const Ref<GizmoRegistry> &p_registry)
{
    registry = p_registry;
    refresh();
}

Ref<GizmoRegistry> GizmoListEditor::get_registry() const { return registry; }

void GizmoListEditor::set_context_object(Object *p_object)
{
    context_id = p_object ? ObjectID(p_object->get_instance_id()) : ObjectID();
    _queue_refresh();
}
Object *GizmoListEditor::get_context_object() const { return ObjectDB::get_instance(context_id); }
void GizmoListEditor::set_context_type(const String &p_type)
{
    context_type = p_type;
    _queue_refresh();
}
String GizmoListEditor::get_context_type() const { return context_type; }

Ref<GizmoEntry> GizmoListEditor::_entry(int64_t p_index) const
{
    TypedArray<GizmoEntry> entries = registry.is_valid() ? registry->get_entries() : TypedArray<GizmoEntry>();
    return (p_index >= 0 && p_index < entries.size()) ? Ref<GizmoEntry>(entries[p_index]) : Ref<GizmoEntry>();
}

// Le registre et ses entrées signalent leurs changements (y compris une annulation dans l'éditeur) :
// la liste est alors reconstruite (une seule fois par image).
void GizmoListEditor::_watch_all()
{
    _unwatch_all();
    if (registry.is_null())
    {
        return;
    }
    Callable callback = callable_mp(this, &GizmoListEditor::_queue_refresh);
    watched.push_back(registry);
    TypedArray<GizmoEntry> entries = registry->get_entries();
    for (int i = 0; i < entries.size(); i++)
    {
        if (Ref<GizmoEntry>(entries[i]).is_valid())
        {
            watched.push_back(entries[i]);
        }
    }
    for (int i = 0; i < watched.size(); i++)
    {
        Ref<Resource> resource = watched[i];
        if (!resource->is_connected("changed", callback))
        {
            resource->connect("changed", callback);
        }
    }
}

void GizmoListEditor::_unwatch_all()
{
    Callable callback = callable_mp(this, &GizmoListEditor::_queue_refresh);
    for (int i = 0; i < watched.size(); i++)
    {
        Ref<Resource> resource = watched[i];
        if (resource.is_valid() && resource->is_connected("changed", callback))
        {
            resource->disconnect("changed", callback);
        }
    }
    watched.clear();
}

void GizmoListEditor::_queue_refresh()
{
    if (!refresh_queued)
    {
        refresh_queued = true;
        callable_mp(this, &GizmoListEditor::refresh).call_deferred();
    }
}

void GizmoListEditor::_commit(const String &p_name, const Array &p_do, const Array &p_undo)
{
    backend->commit_calls(p_name, p_do, p_undo);
    emit_signal("registry_changed");
    _queue_refresh();
}

void GizmoListEditor::_commit_set(Object *p_object, const StringName &p_property, const Variant &p_value)
{
    _commit(U"Gizmos : " + String(p_property), Array::make(Array::make(p_object, "set", p_property, p_value)),
            Array::make(Array::make(p_object, "set", p_property, p_object->get(p_property))));
}

void GizmoListEditor::refresh()
{
    refresh_queued = false;
    TypedArray<Node> children = list->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        list->remove_child(child);
        child->queue_free();
    }
    _watch_all();
    if (registry.is_null())
    {
        return;
    }

    // Objet de contexte (membres compatibles) : objet réel, sinon instance d'exemple libérée en fin de méthode.
    Object *context = ObjectDB::get_instance(context_id);
    Variant sample;
    if (!context && !context_type.is_empty())
    {
        sample = ObjectWidget::instantiate_class(context_type);
        context = sample.get_validated_object();
    }

    PackedStringArray functions = GizmoCatalog::functions();
    TypedArray<GizmoEntry> entries = registry->get_entries();
    if (entries.is_empty())
    {
        list->add_child(make_label(U"Aucun gizmo : « + Gizmo » pour ajouter un dessin automatique.", 0.6f));
    }
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<GizmoEntry> entry = entries[i];
        if (entry.is_null())
        {
            continue;
        }
        PanelContainer *panel = memnew(PanelContainer);
        VBoxContainer *box = memnew(VBoxContainer);
        panel->add_child(box);
        list->add_child(panel);

        // Ligne 1 : activé, fonction, visibilité, redessin, ordre, suppression.
        HBoxContainer *row = memnew(HBoxContainer);
        box->add_child(row);
        CheckBox *enabled = memnew(CheckBox);
        enabled->set_pressed_no_signal(entry->get_enabled());
        enabled->set_tooltip_text(U"Gizmo actif");
        enabled->connect("toggled", callable_mp(this, &GizmoListEditor::_on_enabled_toggled).bind(i));
        row->add_child(enabled);

        OptionButton *function = memnew(OptionButton);
        function->set_h_size_flags(SIZE_EXPAND_FILL);
        function->add_item(U"‹ choisir une fonction ›");
        function->set_item_metadata(0, "");
        for (int f = 0; f < functions.size(); f++)
        {
            function->add_item(functions[f]);
            function->set_item_metadata(f + 1, functions[f]);
            if (functions[f] == entry->get_function())
            {
                function->select(f + 1);
            }
        }
        function->connect("item_selected", callable_mp(this, &GizmoListEditor::_on_function_selected).bind(i, (uint64_t)function->get_instance_id()));
        row->add_child(function);

        OptionButton *visibility = memnew(OptionButton);
        visibility->add_item(U"Éditeur et jeu", GizmoEntry::VISIBILITY_EVERYWHERE);
        visibility->add_item(U"Éditeur", GizmoEntry::VISIBILITY_EDITOR);
        visibility->add_item("Jeu", GizmoEntry::VISIBILITY_GAME);
        visibility->select(visibility->get_item_index(entry->get_visibility()));
        visibility->connect("item_selected", callable_mp(this, &GizmoListEditor::_on_visibility_selected).bind(i));
        row->add_child(visibility);

        OptionButton *redraw = memnew(OptionButton);
        redraw->add_item("Sur changement", GizmoEntry::REDRAW_ON_CHANGE);
        redraw->add_item("Chaque image", GizmoEntry::REDRAW_EVERY_FRAME);
        redraw->add_item("Une fois", GizmoEntry::REDRAW_ONCE);
        redraw->select(redraw->get_item_index(entry->get_redraw()));
        redraw->connect("item_selected", callable_mp(this, &GizmoListEditor::_on_redraw_selected).bind(i));
        row->add_child(redraw);

        Button *up = make_button(U"▲", "Monter");
        up->set_disabled(i == 0);
        up->connect("pressed", callable_mp(this, &GizmoListEditor::_on_move).bind(i, i - 1));
        row->add_child(up);
        Button *down = make_button(U"▼", "Descendre");
        down->set_disabled(i == entries.size() - 1);
        down->connect("pressed", callable_mp(this, &GizmoListEditor::_on_move).bind(i, i + 1));
        row->add_child(down);
        Button *remove = make_button(U"✕", "Supprimer le gizmo");
        remove->connect("pressed", callable_mp(this, &GizmoListEditor::_on_remove).bind(i));
        row->add_child(remove);

        // Paramètres (le RID du gizmo est fourni par l'hôte).
        TypedArray<ArgumentBinding> arguments = entry->get_arguments();
        for (int p = 0; p < arguments.size(); p++)
        {
            Ref<ArgumentBinding> binding = arguments[p];
            if (binding.is_null())
            {
                continue;
            }
            Dictionary param = GizmoCatalog::user_param(entry->get_function(), p);
            HBoxContainer *param_row = memnew(HBoxContainer);
            box->add_child(param_row);
            Label *name = make_label(String(param.get("name", "")));
            name->set_custom_minimum_size(Vector2(140, 0));
            param_row->add_child(name);

            int param_type = param.get("type", 0);
            if (param_type == Variant::RID)
            {
                param_row->add_child(make_label(U"RID : fourni par l'hôte du gizmo", 0.6f));
                continue;
            }

            ArgumentBinding::Source current = binding->get_source();
            bool from_member = current == ArgumentBinding::SOURCE_PROPERTY || current == ArgumentBinding::SOURCE_METHOD;
            OptionButton *source = memnew(OptionButton);
            source->add_item("Valeur manuelle", ArgumentBinding::SOURCE_CONSTANT);
            source->add_item(U"Propriété", ArgumentBinding::SOURCE_PROPERTY);
            source->add_item("Fonction", ArgumentBinding::SOURCE_METHOD);
            source->select(MAX(0, source->get_item_index(current)));
            source->connect("item_selected", callable_mp(this, &GizmoListEditor::_on_source_selected).bind(i, p, (uint64_t)source->get_instance_id()));
            param_row->add_child(source);

            // Objet lu : ressource, sinon le contexte (chemin vide), sinon le nœud au chemin si le contexte est dans l'arbre.
            Object *source_object = nullptr;
            if (from_member)
            {
                Node *context_node = Object::cast_to<Node>(context);
                source_object = binding->get_object_resource().is_valid() ? (Object *)binding->get_object_resource().ptr()
                                : binding->get_object_path().is_empty()      ? context
                                : (context_node && context_node->is_inside_tree()) ? (Object *)context_node->get_node_or_null(binding->get_object_path())
                                                                                   : nullptr;
                // Liste des membres compatibles avec le type du paramètre ; saisie libre dans le champ à côté.
                OptionButton *member = memnew(OptionButton);
                member->set_fit_to_longest_item(false);
                member->set_custom_minimum_size(Vector2(180, 0));
                member->add_item(current == ArgumentBinding::SOURCE_METHOD ? U"‹ fonction compatible ›" : U"‹ propriété compatible ›");
                member->set_item_metadata(0, "");
                String selected_name = current == ArgumentBinding::SOURCE_METHOD ? String(binding->get_method()) : String(binding->get_property_path());
                TypedArray<Dictionary> members = ArgumentBinding::compatible_members(source_object, current, param);
                for (int m = 0; m < members.size(); m++)
                {
                    Dictionary info = members[m];
                    String member_name = info.get("name", "");
                    String suffix = (current == ArgumentBinding::SOURCE_METHOD && !bool(info.get("const", false))) ? String(U"  ⚠") : String();
                    member->add_item(member_name + (current == ArgumentBinding::SOURCE_METHOD ? String("()") : String()) + " : " + member_type_name(info) + suffix);
                    member->set_item_metadata(m + 1, member_name);
                    if (member_name == selected_name)
                    {
                        member->select(m + 1);
                    }
                }
                member->set_disabled(members.is_empty());
                member->set_tooltip_text(!source_object      ? String(U"Objet introuvable dans l'éditeur : saisir le nom à la main")
                                         : members.is_empty() ? U"Aucun membre de " + source_object->get_class() + U" compatible avec " + member_type_name(param)
                                                              : U"Membres de " + source_object->get_class() + U" compatibles avec " + member_type_name(param));
                member->connect("item_selected", callable_mp(this, &GizmoListEditor::_on_member_selected).bind(i, p, (uint64_t)member->get_instance_id()));
                param_row->add_child(member);
            }

            // Valeur : ProceduralEditor sur l'ArgumentBinding, limité aux propriétés utiles à la source
            // (la constante est présentée avec le type du paramètre grâce à expected_info).
            Ref<DataRegistry> value_registry;
            value_registry.instantiate();
            value_registry->set_scope(DataEntry::SCOPE_CLASS_ONLY);
            value_registry->sync(binding.ptr());
            TypedArray<DataEntry> value_entries = value_registry->get_entries();
            for (int v = 0; v < value_entries.size(); v++)
            {
                Ref<DataEntry> value_entry = value_entries[v];
                String property = value_entry->get_data_name();
                bool shown = current == ArgumentBinding::SOURCE_PROPERTY ? (property == "object_path" || property == "property_path")
                             : current == ArgumentBinding::SOURCE_METHOD ? (property == "object_path" || property == "method")
                                                                         : property == "constant";
                value_entry->set_enabled(shown);
                if (shown)
                {
                    Ref<WidgetSettings> no_label;
                    no_label.instantiate();
                    no_label->set_show_label(from_member);
                    value_entry->set_settings(no_label);
                }
            }
            ProceduralEditor *value_editor = memnew(ProceduralEditor);
            value_editor->set_scroll_enabled(false);
            value_editor->set_h_size_flags(SIZE_EXPAND_FILL);
            value_editor->set_undo_backend(backend);
            value_editor->set_registry(value_registry);
            value_editor->set_target(binding.ptr());
            param_row->add_child(value_editor);

            // Fonction appelée à chaque image (« Sur changement » relit aussi les valeurs à chaque image).
            if (current == ArgumentBinding::SOURCE_METHOD && binding->get_method() != StringName() && entry->get_redraw() != GizmoEntry::REDRAW_ONCE &&
                (!source_object || !ArgumentBinding::is_method_const(source_object, binding->get_method())))
            {
                Label *warning = make_label(U"⚠ « " + String(binding->get_method()) +
                                            U"() » n'est pas déclarée const (ou c'est une méthode de script) : elle est appelée à chaque image, "
                                            U"attention aux effets de bord.");
                warning->set_modulate(Color(1, 0.75, 0.3));
                warning->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
                box->add_child(warning);
            }
        }
    }

    // Instance d'exemple : un RefCounted est libéré avec le Variant, un autre objet (Node...) explicitement.
    if (Object *sample_object = sample.get_validated_object(); sample_object && !Object::cast_to<RefCounted>(sample_object))
    {
        memdelete(sample_object);
    }
}

void GizmoListEditor::_on_add()
{
    if (registry.is_null())
    {
        return;
    }
    Ref<GizmoEntry> entry;
    entry.instantiate();
    PackedStringArray functions = GizmoCatalog::functions();
    if (!functions.is_empty())
    {
        entry->set_function(functions[0]);
    }
    TypedArray<GizmoEntry> entries = registry->get_entries().duplicate();
    entries.push_back(entry);
    _commit_set(registry.ptr(), "entries", entries);
}

void GizmoListEditor::_on_move(int64_t p_from, int64_t p_to)
{
    TypedArray<GizmoEntry> entries = registry->get_entries().duplicate();
    if (p_from < 0 || p_to < 0 || p_from >= entries.size() || p_to >= entries.size())
    {
        return;
    }
    Variant moved = entries[p_from];
    entries[p_from] = entries[p_to];
    entries[p_to] = moved;
    _commit_set(registry.ptr(), "entries", entries);
}

void GizmoListEditor::_on_remove(int64_t p_index)
{
    TypedArray<GizmoEntry> entries = registry->get_entries().duplicate();
    if (p_index < 0 || p_index >= entries.size())
    {
        return;
    }
    entries.remove_at(p_index);
    _commit_set(registry.ptr(), "entries", entries);
}

void GizmoListEditor::_on_enabled_toggled(bool p_on, int64_t p_index)
{
    if (Ref<GizmoEntry> entry = _entry(p_index); entry.is_valid())
    {
        _commit_set(entry.ptr(), "enabled", p_on);
    }
}

void GizmoListEditor::_on_function_selected(int64_t p_item, int64_t p_index, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    Ref<GizmoEntry> entry = _entry(p_index);
    if (!option || entry.is_null())
    {
        return;
    }
    String function = option->get_item_metadata(p_item);
    if (function.is_empty() || function == entry->get_function())
    {
        return;
    }
    // Changer de fonction recrée les arguments. L'annulation restaure fonction ET arguments en UN appel :
    // les opérations d'annulation sont exécutées en ordre inverse, deux « set » successifs perdraient les arguments.
    _commit(U"Gizmos : fonction", Array::make(Array::make(entry, "set", "function", function)),
            Array::make(Array::make(entry, "restore", entry->get_function(), entry->get_arguments())));
}

void GizmoListEditor::_on_visibility_selected(int64_t p_item, int64_t p_index)
{
    if (Ref<GizmoEntry> entry = _entry(p_index); entry.is_valid())
    {
        _commit_set(entry.ptr(), "visibility", (int)p_item); // ids = valeurs de l'énumération, dans l'ordre
    }
}

void GizmoListEditor::_on_redraw_selected(int64_t p_item, int64_t p_index)
{
    if (Ref<GizmoEntry> entry = _entry(p_index); entry.is_valid())
    {
        _commit_set(entry.ptr(), "redraw", (int)p_item);
    }
}

void GizmoListEditor::_on_source_selected(int64_t p_item, int64_t p_index, int64_t p_param, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    Ref<GizmoEntry> entry = _entry(p_index);
    TypedArray<ArgumentBinding> arguments = entry.is_valid() ? entry->get_arguments() : TypedArray<ArgumentBinding>();
    if (!option || p_param < 0 || p_param >= arguments.size())
    {
        return;
    }
    Ref<ArgumentBinding> binding = arguments[p_param];
    _commit_set(binding.ptr(), "source", option->get_item_id(p_item));
}

void GizmoListEditor::_on_member_selected(int64_t p_item, int64_t p_index, int64_t p_param, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    Ref<GizmoEntry> entry = _entry(p_index);
    TypedArray<ArgumentBinding> arguments = entry.is_valid() ? entry->get_arguments() : TypedArray<ArgumentBinding>();
    if (!option || p_param < 0 || p_param >= arguments.size())
    {
        return;
    }
    Ref<ArgumentBinding> binding = arguments[p_param];
    String name = option->get_item_metadata(p_item);
    if (binding.is_null() || name.is_empty())
    {
        return;
    }
    if (binding->get_source() == ArgumentBinding::SOURCE_METHOD)
    {
        _commit_set(binding.ptr(), "method", StringName(name));
    }
    else
    {
        _commit_set(binding.ptr(), "property_path", NodePath(name));
    }
}
