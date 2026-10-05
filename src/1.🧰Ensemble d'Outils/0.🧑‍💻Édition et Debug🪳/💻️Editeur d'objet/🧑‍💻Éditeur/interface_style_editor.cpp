#include "interface_style_editor.hpp"
#include "editor_undo_backend.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/container_widgets.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧱Construction/procedural_editor.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_resource_picker.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

namespace
{
// Identifiant de la source « Valeur du matériau » (uniform non lié) dans la liste des sources. Pas -1 :
// pour un OptionButton, -1 signifie « identifiant = index ».
constexpr int SOURCE_MATERIAL = 100;

Label *make_label(const String &p_text, float p_alpha = 1.0f)
{
    Label *label = memnew(Label);
    label->set_text(p_text);
    label->set_modulate(Color(1, 1, 1, p_alpha));
    return label;
}

String type_name_of(const Dictionary &p_info)
{
    int type = p_info.get("type", 0);
    String class_name = p_info.get("class_name", "");
    if (type == Variant::OBJECT && !class_name.is_empty())
    {
        return class_name;
    }
    return type == Variant::NIL ? String("Variant") : Variant::get_type_name((Variant::Type)type);
}
} // namespace

void InterfaceStyleEditor::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("refresh"), &InterfaceStyleEditor::refresh);
    ADD_SIGNAL(MethodInfo("style_changed"));
}

InterfaceStyleEditor::InterfaceStyleEditor()
{
    set_h_size_flags(SIZE_EXPAND_FILL);
    backend = Ref<UndoBackend>(memnew(EditorUndoBackend));
}

void InterfaceStyleEditor::_notification(int p_what)
{
    if (p_what == NOTIFICATION_PREDELETE)
    {
        _watch(Ref<InterfaceStyle>());
    }
}

void InterfaceStyleEditor::set_owner(Object *p_owner, const StringName &p_property)
{
    owner_id = p_owner ? ObjectID(p_owner->get_instance_id()) : ObjectID();
    property = p_property;
    refresh();
}

void InterfaceStyleEditor::set_context_object(Object *p_object)
{
    context_id = p_object ? ObjectID(p_object->get_instance_id()) : ObjectID();
    _queue_refresh();
}

void InterfaceStyleEditor::set_context_type(const String &p_type)
{
    context_type = p_type;
    _queue_refresh();
}

Ref<InterfaceStyle> InterfaceStyleEditor::_style() const
{
    Object *owner = ObjectDB::get_instance(owner_id);
    return owner ? Ref<InterfaceStyle>(owner->get(property)) : Ref<InterfaceStyle>();
}

void InterfaceStyleEditor::_watch(const Ref<InterfaceStyle> &p_style)
{
    Callable callback = callable_mp(this, &InterfaceStyleEditor::_on_style_changed);
    if (watched.is_valid() && watched->is_connected("changed", callback))
    {
        watched->disconnect("changed", callback);
    }
    watched = p_style;
    if (watched.is_valid())
    {
        watched->connect("changed", callback);
    }
}

void InterfaceStyleEditor::_queue_refresh()
{
    if (!refresh_queued)
    {
        refresh_queued = true;
        callable_mp(this, &InterfaceStyleEditor::refresh).call_deferred();
    }
}

void InterfaceStyleEditor::_commit(const String &p_name, const Array &p_do, const Array &p_undo)
{
    backend->commit_calls(p_name, p_do, p_undo);
    emit_signal("style_changed");
    _queue_refresh();
}

void InterfaceStyleEditor::_commit_set(Object *p_object, const StringName &p_property, const Variant &p_value)
{
    _commit(U"Style : " + String(p_property), Array::make(Array::make(p_object, "set", p_property, p_value)),
            Array::make(Array::make(p_object, "set", p_property, p_object->get(p_property))));
}

Object *InterfaceStyleEditor::_resolve_context(Variant &r_sample) const
{
    if (Object *context = ObjectDB::get_instance(context_id))
    {
        return context;
    }
    if (!context_type.is_empty())
    {
        r_sample = ObjectWidget::instantiate_class(context_type);
    }
    return r_sample.get_validated_object();
}

void InterfaceStyleEditor::refresh()
{
    refresh_queued = false;
    TypedArray<Node> children = get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        remove_child(child);
        child->queue_free();
    }
    Object *owner = ObjectDB::get_instance(owner_id);
    if (!owner)
    {
        _watch(Ref<InterfaceStyle>());
        return;
    }

    // Le style : nouveau, chargé (partagé entre plusieurs éléments) ou retiré.
    Ref<InterfaceStyle> style = _style();
    _watch(style);
    last_material = style.is_valid() ? style->get_material() : Ref<Material>();
    last_parameter_count = style.is_valid() ? (int)style->get_shader_parameters().size() : 0;
    HBoxContainer *head = memnew(HBoxContainer);
    add_child(head);
    head->add_child(make_label("Style"));
    EditorResourcePicker *style_picker = memnew(EditorResourcePicker);
    style_picker->set_base_type("InterfaceStyle");
    style_picker->set_edited_resource(style);
    style_picker->set_h_size_flags(SIZE_EXPAND_FILL);
    style_picker->connect("resource_changed", callable_mp(this, &InterfaceStyleEditor::_on_style_resource_changed));
    head->add_child(style_picker);
    if (style.is_null())
    {
        add_child(make_label(U"Aucun style : « Nouveau InterfaceStyle » dans le sélecteur, ou charger un style partagé (.tres).", 0.6f));
        return;
    }

    // Thème, fond, matériau : ressources (clic : édition dans l'inspecteur de Godot).
    const char *resources[][3] = {{"theme", "Theme", "Thème"}, {"background", "StyleBox", "Fond"}, {"material", "ShaderMaterial,CanvasItemMaterial", "Matériau"}};
    for (const auto &resource : resources)
    {
        HBoxContainer *row = memnew(HBoxContainer);
        add_child(row);
        Label *name = make_label(String::utf8(resource[2]));
        name->set_custom_minimum_size(Vector2(120, 0));
        row->add_child(name);
        EditorResourcePicker *picker = memnew(EditorResourcePicker);
        picker->set_base_type(resource[1]);
        picker->set_edited_resource(Ref<Resource>(style->get(resource[0])));
        picker->set_h_size_flags(SIZE_EXPAND_FILL);
        picker->connect("resource_changed", callable_mp(this, &InterfaceStyleEditor::_on_resource_changed).bind(String(resource[0])));
        picker->connect("resource_selected", callable_mp(this, &InterfaceStyleEditor::_on_resource_selected));
        row->add_child(picker);
    }

    // Variation, propagation, teintes : ProceduralEditor sur le style.
    Ref<DataRegistry> registry;
    registry.instantiate();
    registry->set_scope(DataEntry::SCOPE_CLASS_ONLY);
    registry->sync(style.ptr());
    TypedArray<DataEntry> entries = registry->get_entries();
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<DataEntry> entry = entries[i];
        String name = entry->get_data_name();
        entry->set_enabled(name == "theme_type_variation" || name == "propagate_material" || name == "modulate" || name == "self_modulate");
    }
    ProceduralEditor *fields = memnew(ProceduralEditor);
    fields->set_scroll_enabled(false);
    fields->set_styles_enabled(false);
    fields->set_use_layouts(false);
    fields->set_undo_backend(backend);
    fields->set_registry(registry);
    fields->set_target(style.ptr());
    add_child(fields);

    // Uniforms du shader.
    TypedArray<Dictionary> uniforms = style->get_shader_uniforms();
    if (uniforms.is_empty())
    {
        Ref<ShaderMaterial> shader_material = style->get_material();
        add_child(make_label(shader_material.is_valid() ? String(U"Matériau sans shader ou sans uniform.") : String(U"Paramètres de shader : choisir un ShaderMaterial."), 0.6f));
        return;
    }
    add_child(make_label(U"Paramètres du shader"));
    Variant sample;
    Object *context = _resolve_context(sample);
    for (int u = 0; u < uniforms.size(); u++)
    {
        Dictionary uniform = uniforms[u];
        String uniform_name = uniform.get("name", "");
        int uniform_type = uniform.get("type", 0);
        if (uniform_name.is_empty() || uniform_type == Variant::RID)
        {
            continue;
        }
        Ref<ShaderParameterBinding> parameter = style->find_parameter(uniform_name);
        Ref<ArgumentBinding> binding = parameter.is_valid() ? parameter->get_binding() : Ref<ArgumentBinding>();
        int current = binding.is_valid() ? (int)binding->get_source() : SOURCE_MATERIAL;

        HBoxContainer *row = memnew(HBoxContainer);
        add_child(row);
        Label *name = make_label(uniform_name + " : " + type_name_of(uniform));
        name->set_custom_minimum_size(Vector2(160, 0));
        row->add_child(name);
        OptionButton *source = memnew(OptionButton);
        source->add_item(U"Valeur du matériau", SOURCE_MATERIAL);
        source->add_item("Valeur manuelle", ArgumentBinding::SOURCE_CONSTANT);
        source->add_item(U"Propriété", ArgumentBinding::SOURCE_PROPERTY);
        source->add_item("Fonction", ArgumentBinding::SOURCE_METHOD);
        source->select(MAX(0, source->get_item_index(current)));
        source->connect("item_selected", callable_mp(this, &InterfaceStyleEditor::_on_source_selected).bind(uniform_name, (uint64_t)source->get_instance_id()));
        row->add_child(source);
        if (binding.is_null())
        {
            continue;
        }

        bool from_member = current == ArgumentBinding::SOURCE_PROPERTY || current == ArgumentBinding::SOURCE_METHOD;
        Object *source_object = nullptr;
        if (from_member)
        {
            Node *context_node = Object::cast_to<Node>(context);
            source_object = binding->get_object_resource().is_valid()            ? (Object *)binding->get_object_resource().ptr()
                            : binding->get_object_path().is_empty()              ? context
                            : (context_node && context_node->is_inside_tree()) ? (Object *)context_node->get_node_or_null(binding->get_object_path())
                                                                               : nullptr;
            OptionButton *member = memnew(OptionButton);
            member->set_fit_to_longest_item(false);
            member->set_custom_minimum_size(Vector2(180, 0));
            member->add_item(current == ArgumentBinding::SOURCE_METHOD ? U"‹ fonction compatible ›" : U"‹ propriété compatible ›");
            member->set_item_metadata(0, "");
            String selected_name = current == ArgumentBinding::SOURCE_METHOD ? String(binding->get_method()) : String(binding->get_property_path());
            TypedArray<Dictionary> members = ArgumentBinding::compatible_members(source_object, (ArgumentBinding::Source)current, uniform);
            for (int m = 0; m < members.size(); m++)
            {
                Dictionary info = members[m];
                String member_name = info.get("name", "");
                String suffix = (current == ArgumentBinding::SOURCE_METHOD && !bool(info.get("const", false))) ? String(U"  ⚠") : String();
                member->add_item(member_name + (current == ArgumentBinding::SOURCE_METHOD ? String("()") : String()) + " : " + type_name_of(info) + suffix);
                member->set_item_metadata(m + 1, member_name);
                if (member_name == selected_name)
                {
                    member->select(m + 1);
                }
            }
            member->set_disabled(members.is_empty());
            member->set_tooltip_text(!source_object ? String(U"Objet introuvable dans l'éditeur : saisir le nom à la main")
                                                    : U"Membres de " + source_object->get_class() + U" compatibles avec " + type_name_of(uniform));
            member->connect("item_selected", callable_mp(this, &InterfaceStyleEditor::_on_member_selected).bind(uniform_name, (uint64_t)member->get_instance_id()));
            row->add_child(member);
        }

        // Valeur manuelle (avec le widget du type de l'uniform), ou objet + nom saisis à la main.
        Ref<DataRegistry> value_registry;
        value_registry.instantiate();
        value_registry->set_scope(DataEntry::SCOPE_CLASS_ONLY);
        value_registry->sync(binding.ptr());
        TypedArray<DataEntry> value_entries = value_registry->get_entries();
        for (int v = 0; v < value_entries.size(); v++)
        {
            Ref<DataEntry> value_entry = value_entries[v];
            String field = value_entry->get_data_name();
            bool shown = current == ArgumentBinding::SOURCE_PROPERTY ? (field == "object_path" || field == "property_path")
                         : current == ArgumentBinding::SOURCE_METHOD ? (field == "object_path" || field == "method")
                                                                     : field == "constant";
            value_entry->set_enabled(shown);
            if (shown)
            {
                Ref<WidgetSettings> label_settings;
                label_settings.instantiate();
                label_settings->set_show_label(from_member);
                value_entry->set_settings(label_settings);
            }
        }
        ProceduralEditor *value_editor = memnew(ProceduralEditor);
        value_editor->set_scroll_enabled(false);
        value_editor->set_styles_enabled(false);
        value_editor->set_use_layouts(false);
        value_editor->set_h_size_flags(SIZE_EXPAND_FILL);
        value_editor->set_undo_backend(backend);
        value_editor->set_registry(value_registry);
        value_editor->set_target(binding.ptr());
        row->add_child(value_editor);

        if (current == ArgumentBinding::SOURCE_METHOD && binding->get_method() != StringName() &&
            (!source_object || !ArgumentBinding::is_method_const(source_object, binding->get_method())))
        {
            Label *warning = make_label(U"⚠ « " + String(binding->get_method()) +
                                        U"() » n'est pas déclarée const (ou c'est une méthode de script) : elle est appelée à chaque relecture, "
                                        U"attention aux effets de bord.");
            warning->set_modulate(Color(1, 0.75, 0.3));
            warning->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
            add_child(warning);
        }
    }
    if (Object *sample_object = sample.get_validated_object(); sample_object && !Object::cast_to<RefCounted>(sample_object))
    {
        memdelete(sample_object);
    }
}

void InterfaceStyleEditor::_on_style_resource_changed(const Ref<Resource> &p_resource)
{
    if (Object *owner = ObjectDB::get_instance(owner_id))
    {
        _commit_set(owner, property, p_resource);
    }
}

void InterfaceStyleEditor::_on_resource_changed(const Ref<Resource> &p_resource, const String &p_property)
{
    if (Ref<InterfaceStyle> style = _style(); style.is_valid())
    {
        _commit_set(style.ptr(), p_property, p_resource);
    }
}

void InterfaceStyleEditor::_on_resource_selected(const Ref<Resource> &p_resource, bool p_inspect)
{
    (void)p_inspect;
    if (EditorInterface *editor = EditorInterface::get_singleton(); editor && p_resource.is_valid())
    {
        editor->edit_resource(p_resource);
    }
}

void InterfaceStyleEditor::_on_style_changed()
{
    // Modification du style (ici ou ailleurs, annulation comprise) : l'aperçu se reconstruit ; la liste des
    // uniforms seulement si le matériau ou les liens ont changé (pas pendant la saisie d'une teinte).
    emit_signal("style_changed");
    Ref<InterfaceStyle> style = _style();
    if (style.is_valid() && (style->get_material() != last_material || (int)style->get_shader_parameters().size() != last_parameter_count))
    {
        _queue_refresh();
    }
}

void InterfaceStyleEditor::_on_source_selected(int64_t p_item, const String &p_uniform, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    Ref<InterfaceStyle> style = _style();
    if (!option || style.is_null())
    {
        return;
    }
    int source = option->get_item_id(p_item);
    Ref<ShaderParameterBinding> parameter = style->find_parameter(p_uniform);
    TypedArray<ShaderParameterBinding> before = style->get_shader_parameters();
    if (source == SOURCE_MATERIAL)
    {
        if (parameter.is_valid())
        {
            TypedArray<ShaderParameterBinding> after = before.duplicate();
            after.erase(parameter);
            _commit(U"Style : uniform « " + p_uniform + U" » non lié", Array::make(Array::make(style, "set", "shader_parameters", after)),
                    Array::make(Array::make(style, "set", "shader_parameters", before)));
        }
        return;
    }
    if (parameter.is_valid())
    {
        _commit_set(parameter->get_binding().ptr(), "source", source);
        return;
    }
    // Nouveau lien : la valeur manuelle part de la valeur actuelle du matériau, avec le type de l'uniform.
    Dictionary expected;
    TypedArray<Dictionary> uniforms = style->get_shader_uniforms();
    for (int i = 0; i < uniforms.size(); i++)
    {
        if (String(Dictionary(uniforms[i]).get("name", "")) == p_uniform)
        {
            expected = uniforms[i];
        }
    }
    parameter.instantiate();
    parameter->set_parameter(p_uniform);
    Ref<ArgumentBinding> binding = parameter->get_binding();
    binding->set_expected_info(expected);
    binding->set_source((ArgumentBinding::Source)source);
    Ref<ShaderMaterial> shader_material = style->get_material();
    if (shader_material.is_valid())
    {
        binding->set_constant(shader_material->get_shader_parameter(p_uniform));
    }
    TypedArray<ShaderParameterBinding> after = before.duplicate();
    after.push_back(parameter);
    _commit(U"Style : uniform « " + p_uniform + U" » lié", Array::make(Array::make(style, "set", "shader_parameters", after)),
            Array::make(Array::make(style, "set", "shader_parameters", before)));
}

void InterfaceStyleEditor::_on_member_selected(int64_t p_item, const String &p_uniform, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    Ref<InterfaceStyle> style = _style();
    Ref<ShaderParameterBinding> parameter = style.is_valid() ? style->find_parameter(p_uniform) : Ref<ShaderParameterBinding>();
    if (!option || parameter.is_null())
    {
        return;
    }
    String name = option->get_item_metadata(p_item);
    if (name.is_empty())
    {
        return;
    }
    Ref<ArgumentBinding> binding = parameter->get_binding();
    if (binding->get_source() == ArgumentBinding::SOURCE_METHOD)
    {
        _commit_set(binding.ptr(), "method", StringName(name));
    }
    else
    {
        _commit_set(binding.ptr(), "property_path", NodePath(name));
    }
}
