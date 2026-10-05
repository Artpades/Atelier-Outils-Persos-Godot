#include "preview_source_editor.hpp"
#include "editor_undo_backend.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/preview_widget.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧱Construction/procedural_editor.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧱Construction/widget_factory.hpp"

#include <godot_cpp/classes/editor_resource_picker.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

void PreviewSourceEditor::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("refresh"), &PreviewSourceEditor::refresh);
    ClassDB::bind_static_method("PreviewSourceEditor", D_METHOD("source_hint_of", "class_name"), &PreviewSourceEditor::source_hint_of);
    ADD_SIGNAL(MethodInfo("source_changed"));
}

PreviewSourceEditor::PreviewSourceEditor() { backend = Ref<UndoBackend>(memnew(EditorUndoBackend)); }

Dictionary PreviewSourceEditor::source_hint_of(const StringName &p_class)
{
    PropertyWidget *widget = WidgetFactory::instantiate_widget_class(p_class);
    PreviewWidget *preview = Object::cast_to<PreviewWidget>(widget);
    Dictionary result = preview ? preview->get_source_hint() : Dictionary();
    if (widget)
    {
        memdelete(widget); // jamais ajoutée à l'arbre
    }
    return result;
}

void PreviewSourceEditor::setup(const Ref<PreviewSettings> &p_settings, Object *p_context)
{
    settings = p_settings;
    context_id = p_context ? ObjectID(p_context->get_instance_id()) : ObjectID();
    hint = settings.is_valid() ? source_hint_of(settings->get_preview_class()) : Dictionary();
    refresh();
}

void PreviewSourceEditor::_commit_set(Object *p_object, const StringName &p_property, const Variant &p_value)
{
    backend->commit_calls(U"Aperçu : " + String(p_property), Array::make(Array::make(p_object, "set", p_property, p_value)),
                          Array::make(Array::make(p_object, "set", p_property, p_object->get(p_property))));
    emit_signal("source_changed");
    callable_mp(this, &PreviewSourceEditor::refresh).call_deferred();
}

void PreviewSourceEditor::refresh()
{
    TypedArray<Node> children = get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        remove_child(child);
        child->queue_free();
    }
    if (settings.is_null())
    {
        return;
    }
    Ref<ArgumentBinding> source = settings->get_source();
    bool displayed = source->get_source() == ArgumentBinding::SOURCE_PROPERTY && source->get_property_path().is_empty() && !property_pending;
    int mode = displayed ? MODE_DISPLAYED : (int)source->get_source();
    int hint_type = hint.get("type", 0);
    String hint_class = hint.get("class_name", "");

    HBoxContainer *row = memnew(HBoxContainer);
    add_child(row);
    Label *title = memnew(Label);
    title->set_text("Source" + (hint_class.is_empty() ? String() : U" (" + hint_class + ")"));
    row->add_child(title);
    OptionButton *modes = memnew(OptionButton);
    modes->add_item(U"Objet affiché", MODE_DISPLAYED);
    modes->add_item(U"Propriété", ArgumentBinding::SOURCE_PROPERTY);
    modes->add_item("Fonction", ArgumentBinding::SOURCE_METHOD);
    modes->add_item("Valeur manuelle", ArgumentBinding::SOURCE_CONSTANT);
    modes->select(MAX(0, modes->get_item_index(mode)));
    modes->connect("item_selected", callable_mp(this, &PreviewSourceEditor::_on_mode_selected).bind((uint64_t)modes->get_instance_id()));
    row->add_child(modes);

    Object *context = ObjectDB::get_instance(context_id);
    if (mode == ArgumentBinding::SOURCE_PROPERTY || mode == ArgumentBinding::SOURCE_METHOD)
    {
        Node *context_node = Object::cast_to<Node>(context);
        Object *object = source->get_object_path().is_empty() ? context
                         : (context_node && context_node->is_inside_tree()) ? (Object *)context_node->get_node_or_null(source->get_object_path())
                                                                            : nullptr;
        OptionButton *member = memnew(OptionButton);
        member->set_fit_to_longest_item(false);
        member->set_custom_minimum_size(Vector2(200, 0));
        member->add_item(mode == ArgumentBinding::SOURCE_METHOD ? U"‹ fonction compatible ›" : U"‹ propriété compatible ›");
        member->set_item_metadata(0, "");
        String selected_name = mode == ArgumentBinding::SOURCE_METHOD ? String(source->get_method()) : String(source->get_property_path());
        TypedArray<Dictionary> members = ArgumentBinding::compatible_members(object, (ArgumentBinding::Source)mode, hint);
        for (int m = 0; m < members.size(); m++)
        {
            String name = Dictionary(members[m]).get("name", "");
            member->add_item(name + (mode == ArgumentBinding::SOURCE_METHOD ? String("()") : String()));
            member->set_item_metadata(m + 1, name);
            if (name == selected_name)
            {
                member->select(m + 1);
            }
        }
        member->set_disabled(members.is_empty());
        member->connect("item_selected", callable_mp(this, &PreviewSourceEditor::_on_member_selected).bind((uint64_t)member->get_instance_id()));
        row->add_child(member);
    }
    if (mode == ArgumentBinding::SOURCE_CONSTANT && hint_type == Variant::OBJECT)
    {
        EditorResourcePicker *picker = memnew(EditorResourcePicker);
        picker->set_base_type(hint_class.is_empty() ? String("Resource") : hint_class);
        picker->set_edited_resource(Ref<Resource>(source->get_constant()));
        picker->set_h_size_flags(SIZE_EXPAND_FILL);
        picker->connect("resource_changed", callable_mp(this, &PreviewSourceEditor::_on_constant_resource));
        row->add_child(picker);
        return;
    }

    // Saisie libre : chemin de l'objet (nœud enfant...), nom du membre, ou constante typée.
    Ref<DataRegistry> registry;
    registry.instantiate();
    registry->set_scope(DataEntry::SCOPE_CLASS_ONLY);
    registry->sync(source.ptr());
    TypedArray<DataEntry> entries = registry->get_entries();
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<DataEntry> entry = entries[i];
        String field = entry->get_data_name();
        bool shown = mode == MODE_DISPLAYED                       ? field == "object_path"
                     : mode == ArgumentBinding::SOURCE_PROPERTY ? (field == "object_path" || field == "property_path")
                     : mode == ArgumentBinding::SOURCE_METHOD   ? (field == "object_path" || field == "method")
                                                                : field == "constant";
        entry->set_enabled(shown);
    }
    if (mode == ArgumentBinding::SOURCE_CONSTANT && source->get_expected_info() != hint && !hint.is_empty())
    {
        source->set_expected_info(hint); // constante présentée avec le type attendu
    }
    ProceduralEditor *fields = memnew(ProceduralEditor);
    fields->set_scroll_enabled(false);
    fields->set_styles_enabled(false);
    fields->set_use_layouts(false);
    fields->set_h_size_flags(SIZE_EXPAND_FILL);
    fields->set_undo_backend(backend);
    fields->set_registry(registry);
    fields->set_target(source.ptr());
    add_child(fields);
}

void PreviewSourceEditor::_on_mode_selected(int64_t p_item, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    if (!option || settings.is_null())
    {
        return;
    }
    Ref<ArgumentBinding> source = settings->get_source();
    int mode = option->get_item_id(p_item);
    property_pending = mode == ArgumentBinding::SOURCE_PROPERTY && source->get_property_path().is_empty();
    Array redo;
    Array undo;
    if (mode == MODE_DISPLAYED)
    {
        redo.push_back(Array::make(source, "set", "source", (int)ArgumentBinding::SOURCE_PROPERTY));
        redo.push_back(Array::make(source, "set", "property_path", NodePath()));
    }
    else
    {
        redo.push_back(Array::make(source, "set", "source", mode));
    }
    // Annulation en ordre inverse : chemin puis source.
    undo.push_back(Array::make(source, "set", "property_path", source->get_property_path()));
    undo.push_back(Array::make(source, "set", "source", (int)source->get_source()));
    backend->commit_calls(U"Aperçu : source", redo, undo);
    emit_signal("source_changed");
    callable_mp(this, &PreviewSourceEditor::refresh).call_deferred();
}

void PreviewSourceEditor::_on_member_selected(int64_t p_item, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    if (!option || settings.is_null())
    {
        return;
    }
    String name = option->get_item_metadata(p_item);
    if (name.is_empty())
    {
        return;
    }
    Ref<ArgumentBinding> source = settings->get_source();
    property_pending = false;
    if (source->get_source() == ArgumentBinding::SOURCE_METHOD)
    {
        _commit_set(source.ptr(), "method", StringName(name));
    }
    else
    {
        _commit_set(source.ptr(), "property_path", NodePath(name));
    }
}

void PreviewSourceEditor::_on_constant_resource(const Ref<Resource> &p_resource)
{
    if (settings.is_valid())
    {
        _commit_set(settings->get_source().ptr(), "constant", p_resource);
    }
}
