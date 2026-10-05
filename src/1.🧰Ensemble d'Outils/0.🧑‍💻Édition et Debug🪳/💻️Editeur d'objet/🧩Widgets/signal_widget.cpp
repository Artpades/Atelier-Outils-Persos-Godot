#include "signal_widget.hpp"
#include "special_widgets.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/data_registry.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/type_utils.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/spin_box.hpp>
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
EditorInterface *editor_interface() { return Engine::get_singleton()->is_editor_hint() ? EditorInterface::get_singleton() : nullptr; }

Button *make_button(const String &p_text, const String &p_tooltip = String())
{
    Button *button = memnew(Button);
    button->set_text(p_text);
    button->set_tooltip_text(p_tooltip);
    return button;
}

Label *make_label(const String &p_text, const Color &p_modulate = Color(1, 1, 1, 1))
{
    Label *label = memnew(Label);
    label->set_text(p_text);
    label->set_modulate(p_modulate);
    return label;
}

String arg_name(const Dictionary &p_arg) { return p_arg.get("name", ""); }

// Valeur par défaut d'un paramètre : default_args (alignés sur les derniers paramètres), sinon défaut du type.
Variant default_for(const Dictionary &p_method_info, int p_index)
{
    Array params = p_method_info.get("args", Array());
    Array defaults = p_method_info.get("default_args", Array());
    int first_default = params.size() - defaults.size();
    if (p_index >= first_default && p_index - first_default < defaults.size())
    {
        return defaults[p_index - first_default];
    }
    int type = Dictionary(params[p_index]).get("type", 0);
    return type == Variant::NIL || type == Variant::OBJECT ? Variant() : UtilityFunctions::type_convert(Variant(), type);
}

// Arguments neufs (constantes par défaut) pour une méthode.
TypedArray<ArgumentBinding> default_arguments(const Dictionary &p_method_info)
{
    TypedArray<ArgumentBinding> arguments;
    Array params = p_method_info.get("args", Array());
    for (int i = 0; i < params.size(); i++)
    {
        Ref<ArgumentBinding> binding;
        binding.instantiate();
        binding->set_constant(default_for(p_method_info, i));
        arguments.push_back(binding);
    }
    return arguments;
}

String describe_binding(const Ref<ArgumentBinding> &p_binding)
{
    if (p_binding.is_null())
    {
        return "null";
    }
    return String::num_int64(p_binding->get_source()) + "/" + UtilityFunctions::var_to_str(p_binding->get_constant()) + "/" + String(p_binding->get_object_path()) + "/" +
           String(p_binding->get_property_path()) + "/" + String(p_binding->get_method()) + "/" + String::num_int64(p_binding->get_signal_index());
}
} // namespace

// =====================================================================================================
// Outils statiques
// =====================================================================================================

bool SignalWidget::is_native_eligible(Object *p_source, Object *p_target)
{
    EditorInterface *editor = editor_interface();
    Node *root = editor ? editor->get_edited_scene_root() : nullptr;
    if (!root)
    {
        return false;
    }
    auto in_scene = [root](Object *p_object)
    {
        Node *node = Object::cast_to<Node>(p_object);
        return node && (node == root || node->get_owner() == root);
    };
    return in_scene(p_source) && in_scene(p_target);
}

Callable SignalWidget::make_native_callable(Object *p_target, const StringName &p_method, const Array &p_constants, int p_unbinds)
{
    Callable callable(p_target, p_method);
    if (!p_constants.is_empty())
    {
        callable = callable.bindv(p_constants);
    }
    if (p_unbinds > 0)
    {
        callable = callable.unbind(p_unbinds); // retire les DERNIERS arguments du signal, puis bind ajoute les constantes
    }
    return callable;
}

int SignalWidget::max_native_prefix(const Array &p_signal_args, const Array &p_method_args)
{
    int k = 0;
    while (k < p_signal_args.size() && k < p_method_args.size() && TypeUtils::is_compatible(p_signal_args[k], p_method_args[k]))
    {
        k++;
    }
    return k;
}

// =====================================================================================================
// Accès
// =====================================================================================================

Object *SignalWidget::_source() const { return get_accessor().is_valid() ? get_accessor()->get_target() : nullptr; }
StringName SignalWidget::_signal() const { return get_accessor().is_valid() ? get_accessor()->get_property() : StringName(); }
Array SignalWidget::_signal_args() const { return get_info().get("args", Array()); }
bool SignalWidget::_native_mode() const { return is_native_eligible(_source(), _source()); }

Dictionary SignalWidget::_method_info(Object *p_target, const StringName &p_method) const
{
    if (!p_target || String(p_method).is_empty())
    {
        return Dictionary();
    }
    TypedArray<Dictionary> methods = p_target->get_method_list();
    for (int i = 0; i < methods.size(); i++)
    {
        Dictionary info = methods[i];
        if (StringName(info.get("name", "")) == p_method)
        {
            return info;
        }
    }
    return Dictionary();
}

void SignalWidget::_adapt_arguments(const Ref<SignalLink> &p_link, const Dictionary &p_method_info, bool p_native, int p_k) const
{
    Array params = p_method_info.get("args", Array());
    TypedArray<ArgumentBinding> arguments = p_link->get_arguments();
    bool changed = arguments.size() != params.size();
    while (arguments.size() > params.size())
    {
        arguments.remove_at(arguments.size() - 1);
    }
    while (arguments.size() < params.size())
    {
        Ref<ArgumentBinding> binding;
        binding.instantiate();
        binding->set_constant(default_for(p_method_info, arguments.size()));
        arguments.push_back(binding);
    }
    if (p_native)
    {
        // Natif : les k premiers paramètres viennent du signal, dans l'ordre ; les autres sont des constantes.
        for (int i = 0; i < arguments.size(); i++)
        {
            Ref<ArgumentBinding> binding = arguments[i];
            if (i < p_k)
            {
                binding->set_source(ArgumentBinding::SOURCE_SIGNAL_ARG);
                binding->set_signal_index(i);
            }
            else if (binding->get_source() != ArgumentBinding::SOURCE_CONSTANT)
            {
                binding->set_source(ArgumentBinding::SOURCE_CONSTANT);
                binding->set_constant(default_for(p_method_info, i));
            }
        }
    }
    if (changed)
    {
        p_link->set_arguments(arguments);
    }
}

Ref<UndoBackend> SignalWidget::_backend() const { return get_context().is_valid() ? get_context()->get_undo_backend() : Ref<UndoBackend>(); }

void SignalWidget::_commit(const String &p_name, const Array &p_do, const Array &p_undo)
{
    Ref<UndoBackend> backend = _backend();
    if (backend.is_valid())
    {
        backend->commit_calls(p_name, p_do, p_undo);
    }
    else
    {
        // Sans historique : exécution directe.
        for (int i = 0; i < p_do.size(); i++)
        {
            Array call = p_do[i];
            Object *object = call[0];
            if (object)
            {
                object->callv(call[1], call.slice(2));
            }
        }
    }
    _reapply_resource_links();
    _rebuild_deferred();
}

void SignalWidget::_reapply_resource_links()
{
    if (get_context().is_valid())
    {
        get_context()->links_changed();
    }
}

void SignalWidget::_commit_sets(const Array &p_sets)
{
    Array do_calls;
    Array undo_calls;
    for (int i = 0; i < p_sets.size(); i++)
    {
        Array set = p_sets[i];
        Object *object = set[0];
        if (!object)
        {
            continue;
        }
        Array do_call;
        do_call.push_back(object);
        do_call.push_back("set");
        do_call.push_back(set[1]);
        do_call.push_back(set[2]);
        do_calls.push_back(do_call);
        Array undo_call;
        undo_call.push_back(object);
        undo_call.push_back("set");
        undo_call.push_back(set[1]);
        undo_call.push_back(object->get(set[1]));
        undo_calls.push_front(undo_call);
    }
    _commit(U"Lien du signal « " + String(_signal()) + U" »", do_calls, undo_calls);
}

// =====================================================================================================
// Construction
// =====================================================================================================

Control *SignalWidget::build_editor()
{
    VBoxContainer *root = memnew(VBoxContainer);

    HBoxContainer *header = memnew(HBoxContainer);
    root->add_child(header);
    Label *title = make_label(U"📡 " + MethodWidget::signature(get_info()));
    title->set_h_size_flags(SIZE_EXPAND_FILL);
    title->set_clip_text(true);
    header->add_child(title);
    Button *emit = make_button(U"▶ Émettre", U"Émet le signal avec les valeurs par défaut de ses arguments");
    emit->connect("pressed", callable_mp(this, &SignalWidget::_on_emit));
    header->add_child(emit);
    Button *add = make_button("+ Lien", U"Ajouter un appel de méthode à ce signal");
    add->set_disabled(is_read_only());
    add->connect("pressed", callable_mp(this, &SignalWidget::_on_add));
    header->add_child(add);

    root->add_child(make_label(_native_mode() ? String(U"Mode natif : connexions enregistrées dans la scène (visibles dans l'onglet Nœud)")
                                              : String(U"Mode ressource : liens enregistrés dans le registre et appliqués en jeu"),
                               Color(1, 1, 1, 0.6)));

    links_box = memnew(VBoxContainer);
    links_box->set_h_size_flags(SIZE_EXPAND_FILL);
    root->add_child(links_box);
    return root;
}

Variant SignalWidget::read_value() const
{
    Object *source = _source();
    if (!source)
    {
        return Variant();
    }
    String fingerprint;
    StringName signal = _signal();
    if (_native_mode() && source->has_signal(signal))
    {
        TypedArray<Dictionary> connections = source->get_signal_connection_list(signal);
        for (int i = 0; i < connections.size(); i++)
        {
            Dictionary connection = connections[i];
            Callable callable = connection.get("callable", Callable());
            if (!(int(connection.get("flags", 0)) & Object::CONNECT_PERSIST))
            {
                continue;
            }
            fingerprint += "N:" + String::num_uint64((uint64_t)callable.get_object_id()) + ":" + String(callable.get_method()) + ":" +
                           UtilityFunctions::var_to_str(callable.get_bound_arguments()) + ":" + String::num_int64(callable.get_unbound_arguments_count()) + ";";
        }
    }
    fingerprint += "D:" + String::num_int64(native_drafts.size()) + ";";
    Ref<DataEntry> data_entry = get_entry();
    TypedArray<SignalLink> links = data_entry.is_valid() ? data_entry->get_links() : TypedArray<SignalLink>();
    for (int i = 0; i < links.size(); i++)
    {
        Ref<SignalLink> link = links[i];
        if (link.is_null())
        {
            continue;
        }
        fingerprint += "R:" + String::num_int64(link->get_enabled()) + ":" + String(link->get_target_path()) + ":" +
                       (link->get_target_resource().is_valid() ? link->get_target_resource()->get_path() : String()) + ":" + String(link->get_method());
        TypedArray<ArgumentBinding> arguments = link->get_arguments();
        for (int a = 0; a < arguments.size(); a++)
        {
            fingerprint += "|" + describe_binding(arguments[a]);
        }
        fingerprint += ";";
    }
    return fingerprint;
}

void SignalWidget::pull_value(const Variant &p_value)
{
    if (shown_fingerprint.get_type() == Variant::NIL || !shown_fingerprint.hash_compare(p_value))
    {
        shown_fingerprint = p_value;
        _rebuild();
    }
}

void SignalWidget::_rebuild_deferred() { callable_mp(this, &SignalWidget::_rebuild).call_deferred(); }

void SignalWidget::_collect_items()
{
    items.clear();
    Object *source = _source();
    if (!source)
    {
        return;
    }
    StringName signal = _signal();
    int signal_count = _signal_args().size();

    if (_native_mode() && source->has_signal(signal))
    {
        Node *source_node = Object::cast_to<Node>(source);
        TypedArray<Dictionary> connections = source->get_signal_connection_list(signal);
        for (int i = 0; i < connections.size(); i++)
        {
            Dictionary connection = connections[i];
            Callable callable = connection.get("callable", Callable());
            Node *target = Object::cast_to<Node>(callable.get_object());
            if (!(int(connection.get("flags", 0)) & Object::CONNECT_PERSIST) || !is_native_eligible(source, target))
            {
                continue;
            }
            // Relecture du lien natif : k arguments du signal transmis, puis les constantes liées (bind).
            int k = MAX(0, signal_count - (int)callable.get_unbound_arguments_count());
            Array bound = callable.get_bound_arguments();
            Ref<SignalLink> link;
            link.instantiate();
            link->set_target_path(source_node->get_path_to(target));
            link->set_method(callable.get_method());
            TypedArray<ArgumentBinding> arguments;
            for (int a = 0; a < k; a++)
            {
                Ref<ArgumentBinding> binding;
                binding.instantiate();
                binding->set_source(ArgumentBinding::SOURCE_SIGNAL_ARG);
                binding->set_signal_index(a);
                arguments.push_back(binding);
            }
            for (int b = 0; b < bound.size(); b++)
            {
                Ref<ArgumentBinding> binding;
                binding.instantiate();
                binding->set_constant(bound[b]);
                arguments.push_back(binding);
            }
            link->set_arguments(arguments);

            Dictionary item;
            item["link"] = link;
            item["native"] = true;
            item["callable"] = callable;
            item["k"] = k;
            items.push_back(item);
        }
        for (int d = 0; d < native_drafts.size(); d++)
        {
            Dictionary item;
            item["link"] = native_drafts[d];
            item["native"] = true;
            item["k"] = 0;
            items.push_back(item);
        }
    }

    Ref<DataEntry> data_entry = get_entry();
    TypedArray<SignalLink> links = data_entry.is_valid() ? data_entry->get_links() : TypedArray<SignalLink>();
    for (int i = 0; i < links.size(); i++)
    {
        if (Ref<SignalLink>(links[i]).is_valid())
        {
            Dictionary item;
            item["link"] = links[i];
            item["native"] = false;
            items.push_back(item);
        }
    }
}

void SignalWidget::_rebuild()
{
    if (!links_box)
    {
        return;
    }
    TypedArray<Node> children = links_box->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        links_box->remove_child(child);
        child->queue_free();
    }
    _collect_items();
    for (int i = 0; i < items.size(); i++)
    {
        links_box->add_child(_build_item(i));
    }
    if (items.is_empty())
    {
        links_box->add_child(make_label(U"Aucun lien : « + Lien » pour appeler une méthode à chaque émission.", Color(1, 1, 1, 0.6)));
    }
}

Control *SignalWidget::_build_item(int p_index)
{
    Dictionary item = items[p_index];
    Ref<SignalLink> link = item["link"];
    bool native = item["native"];
    bool read_only = is_read_only();
    Object *source = _source();
    Object *target = link->resolve_target(source);

    PanelContainer *panel = memnew(PanelContainer);
    panel->set_h_size_flags(SIZE_EXPAND_FILL);
    VBoxContainer *box = memnew(VBoxContainer);
    panel->add_child(box);

    // Ligne 1 : mode, activation (ressource), cible, état, sélecteurs, suppression.
    HBoxContainer *row = memnew(HBoxContainer);
    box->add_child(row);
    Label *tag = make_label(native ? String("Natif") : String("Ressource"), Color(1, 1, 1, 0.7));
    tag->set_tooltip_text(native ? String(U"Connexion de la scène (CONNECT_PERSIST) : correspondance « k premiers paramètres = k premiers arguments »")
                                 : String(U"Lien du registre : correspondance libre des paramètres"));
    row->add_child(tag);
    if (!native)
    {
        CheckBox *enabled = memnew(CheckBox);
        enabled->set_pressed_no_signal(link->get_enabled());
        enabled->set_tooltip_text("Lien actif");
        enabled->set_disabled(read_only);
        enabled->connect("toggled", callable_mp(this, &SignalWidget::_on_enabled_toggled).bind(p_index));
        row->add_child(enabled);
    }
    LineEdit *path = memnew(LineEdit);
    path->set_h_size_flags(SIZE_EXPAND_FILL);
    path->set_placeholder(U"(émetteur)");
    path->set_tooltip_text(U"Objet appelé : chemin relatif à l'émetteur (vide : l'émetteur lui-même)");
    if (link->get_target_resource().is_valid())
    {
        path->set_text(link->get_target_resource()->get_path());
        path->set_editable(false);
    }
    else
    {
        path->set_text(String(link->get_target_path()));
        path->set_editable(!read_only);
        path->connect("text_submitted", callable_mp(this, &SignalWidget::_on_target_submitted).bind(p_index));
    }
    row->add_child(path);
    Node *target_node = Object::cast_to<Node>(target);
    row->add_child(make_label(target ? U"✔ " + (target_node ? String(target_node->get_name()) : target->get_class()) : String(U"✘ introuvable"),
                              target ? Color(0.6, 1, 0.6) : Color(1, 0.6, 0.6)));
    Button *pick = make_button(U"…", U"Choisir le nœud appelé");
    pick->set_disabled(read_only);
    pick->connect("pressed", callable_mp(this, &SignalWidget::_on_pick_node).bind(p_index));
    row->add_child(pick);
    if (!native && editor_interface())
    {
        Button *pick_resource = make_button(U"Ressource…", U"Appeler une méthode d'une ressource (fichier)");
        pick_resource->set_disabled(read_only);
        pick_resource->connect("pressed", callable_mp(this, &SignalWidget::_on_pick_resource).bind(p_index));
        row->add_child(pick_resource);
    }
    Button *remove = make_button(U"✕", "Supprimer le lien");
    remove->set_disabled(read_only);
    remove->connect("pressed", callable_mp(this, &SignalWidget::_on_remove).bind(p_index));
    row->add_child(remove);

    if (!target)
    {
        box->add_child(make_label(U"Choisir un objet valide pour lister ses méthodes.", Color(1, 1, 1, 0.6)));
        return panel;
    }

    // Ligne 2 : méthode.
    HBoxContainer *method_row = memnew(HBoxContainer);
    box->add_child(method_row);
    method_row->add_child(make_label(U"Méthode"));
    OptionButton *methods = memnew(OptionButton);
    methods->set_h_size_flags(SIZE_EXPAND_FILL);
    methods->set_disabled(read_only);
    methods->add_item(U"‹ choisir une méthode ›");
    methods->set_item_metadata(0, "");
    TypedArray<Dictionary> listed = DataRegistry::get_members(target, DataEntry::MEMBER_METHOD, DataEntry::SCOPE_AUTO);
    bool found = false;
    for (int i = 0; i < listed.size(); i++)
    {
        Dictionary info = listed[i];
        String name = info.get("name", "");
        if (name.begins_with("_") && StringName(name) != link->get_method())
        {
            continue;
        }
        methods->add_item(MethodWidget::signature(info));
        methods->set_item_metadata(methods->get_item_count() - 1, name);
        if (StringName(name) == link->get_method())
        {
            methods->select(methods->get_item_count() - 1);
            found = true;
        }
    }
    Dictionary method_info = _method_info(target, link->get_method());
    if (!found && !String(link->get_method()).is_empty())
    {
        // Méthode héritée (hors liste courte) ou introuvable.
        methods->add_item(method_info.is_empty() ? String(link->get_method()) + U"  (introuvable)" : MethodWidget::signature(method_info));
        methods->set_item_metadata(methods->get_item_count() - 1, String(link->get_method()));
        methods->select(methods->get_item_count() - 1);
    }
    methods->connect("item_selected", callable_mp(this, &SignalWidget::_on_method_selected).bind(p_index, (uint64_t)methods->get_instance_id()));
    method_row->add_child(methods);
    if (editor_interface())
    {
        Button *pick_method = make_button(U"…", U"Toutes les méthodes de l'objet (sélecteur de l'éditeur)");
        pick_method->set_disabled(read_only);
        pick_method->connect("pressed", callable_mp(this, &SignalWidget::_on_pick_method).bind(p_index));
        method_row->add_child(pick_method);
    }

    if (method_info.is_empty())
    {
        return panel;
    }
    if (native)
    {
        _build_native_parameters(box, p_index, method_info);
    }
    else
    {
        _build_resource_parameters(box, p_index, method_info);
    }
    return panel;
}

PropertyWidget *SignalWidget::_build_constant(const Ref<ArgumentBinding> &p_binding, const Dictionary &p_param, bool p_undoable)
{
    int type = p_param.get("type", 0);
    Dictionary info;
    info["name"] = arg_name(p_param);
    info["type"] = type;
    info["hint"] = p_param.get("hint", 0);
    info["hint_string"] = p_param.get("hint_string", "");
    info["class_name"] = p_param.get("class_name", "");
    info["usage"] = (int)PROPERTY_USAGE_DEFAULT | (type == Variant::NIL ? (int)PROPERTY_USAGE_NIL_IS_VARIANT : 0);

    Ref<ValueAccessor> accessor;
    accessor.instantiate();
    accessor->setup(p_binding.ptr(), "constant");
    accessor->set_property_info(info);
    accessor->set_display_name(arg_name(p_param));
    accessor->set_undoable(p_undoable); // natif : constante locale, validée par reconnexion (une action)
    return build_child(accessor, make_default_entry(info), get_ancestors());
}

void SignalWidget::_build_native_parameters(VBoxContainer *p_box, int p_index, const Dictionary &p_method_info)
{
    Dictionary item = items[p_index];
    Ref<SignalLink> link = item["link"];
    Array params = p_method_info.get("args", Array());
    Array signal_args = _signal_args();
    int max_k = max_native_prefix(signal_args, params);
    int k = MIN((int)item.get("k", 0), max_k);
    item["k"] = k;
    _adapt_arguments(link, p_method_info, true, k);

    HBoxContainer *k_row = memnew(HBoxContainer);
    p_box->add_child(k_row);
    k_row->add_child(make_label(U"Paramètres reçus du signal (dans l'ordre)"));
    SpinBox *k_spin = memnew(SpinBox);
    k_spin->set_min(0);
    k_spin->set_max(max_k);
    k_spin->set_value_no_signal(k);
    k_spin->set_editable(!is_read_only() && max_k > 0);
    k_spin->connect("value_changed", callable_mp(this, &SignalWidget::_on_k_changed).bind(p_index));
    k_row->add_child(k_spin);
    Label *explanation = make_label(U"Mode natif (bind / unbind) : seuls les k PREMIERS paramètres peuvent venir du signal, dans le même ordre, "
                                    U"et seulement si leurs types sont compatibles ; les autres sont des constantes. Une correspondance libre "
                                    U"n'existe qu'en mode ressource (émetteur ou cible hors de la scène éditée, ou en jeu).",
                                    Color(1, 1, 1, 0.6));
    explanation->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    p_box->add_child(explanation);

    TypedArray<ArgumentBinding> arguments = link->get_arguments();
    for (int p = 0; p < params.size(); p++)
    {
        Dictionary param = params[p];
        if (p < k)
        {
            p_box->add_child(make_label(U"✓ " + arg_name(param) + U" : fourni par le signal (argument " + String::num_int64(p) + U" « " + arg_name(signal_args[p]) + U" »)"));
            continue;
        }
        PropertyWidget *widget = _build_constant(arguments[p], param, false);
        if (widget)
        {
            widget->connect("value_changed", callable_mp(this, &SignalWidget::_on_native_constant_changed).bind(p_index));
            p_box->add_child(widget);
        }
    }
}

void SignalWidget::_build_resource_parameters(VBoxContainer *p_box, int p_index, const Dictionary &p_method_info)
{
    Dictionary item = items[p_index];
    Ref<SignalLink> link = item["link"];
    _adapt_arguments(link, p_method_info, false, 0);
    Array params = p_method_info.get("args", Array());
    Array signal_args = _signal_args();
    TypedArray<ArgumentBinding> arguments = link->get_arguments();
    bool read_only = is_read_only();

    for (int p = 0; p < params.size(); p++)
    {
        Dictionary param = params[p];
        Ref<ArgumentBinding> binding = arguments[p];
        HBoxContainer *row = memnew(HBoxContainer);
        p_box->add_child(row);

        // Source : constante, propriété, ou un argument COMPATIBLE du signal (n'importe lequel : correspondance libre).
        OptionButton *source = memnew(OptionButton);
        source->set_disabled(read_only);
        source->add_item("Constante", 0);
        source->add_item(U"Propriété", 1);
        for (int s = 0; s < signal_args.size(); s++)
        {
            if (TypeUtils::is_compatible(signal_args[s], param))
            {
                source->add_item("Signal : " + arg_name(signal_args[s]), 100 + s);
            }
        }
        int selected_id = binding->get_source() == ArgumentBinding::SOURCE_PROPERTY     ? 1
                          : binding->get_source() == ArgumentBinding::SOURCE_SIGNAL_ARG ? 100 + binding->get_signal_index()
                                                                                        : 0;
        if (source->get_item_index(selected_id) < 0)
        {
            source->add_item(U"Signal : argument " + String::num_int64(binding->get_signal_index()) + U" (incompatible)", selected_id);
        }
        source->select(source->get_item_index(selected_id));
        source->connect("item_selected", callable_mp(this, &SignalWidget::_on_source_selected).bind(p_index, p, (uint64_t)source->get_instance_id()));
        row->add_child(source);

        switch (binding->get_source())
        {
        case ArgumentBinding::SOURCE_SIGNAL_ARG:
            row->add_child(make_label(U"✓ " + arg_name(param) + U" : fourni par le signal"));
            break;
        case ArgumentBinding::SOURCE_PROPERTY:
        {
            row->add_child(make_label(arg_name(param)));
            LineEdit *object_path = memnew(LineEdit);
            object_path->set_placeholder(U"(émetteur)");
            object_path->set_tooltip_text(U"Objet lu : chemin relatif à l'émetteur (vide : l'émetteur)");
            object_path->set_text(String(binding->get_object_path()));
            object_path->set_editable(!read_only);
            object_path->set_h_size_flags(SIZE_EXPAND_FILL);
            object_path->connect("text_submitted", callable_mp(this, &SignalWidget::_on_property_path_submitted).bind(p_index, p, true));
            row->add_child(object_path);
            LineEdit *property_path = memnew(LineEdit);
            property_path->set_placeholder(U"propriété (ex: position:x)");
            property_path->set_text(String(binding->get_property_path()));
            property_path->set_editable(!read_only);
            property_path->set_h_size_flags(SIZE_EXPAND_FILL);
            property_path->connect("text_submitted", callable_mp(this, &SignalWidget::_on_property_path_submitted).bind(p_index, p, false));
            row->add_child(property_path);
            break;
        }
        case ArgumentBinding::SOURCE_CONSTANT:
        default:
        {
            PropertyWidget *widget = _build_constant(binding, param, true);
            if (widget)
            {
                widget->set_h_size_flags(SIZE_EXPAND_FILL);
                row->add_child(widget);
            }
            break;
        }
        }
    }
}

// =====================================================================================================
// Actions
// =====================================================================================================

void SignalWidget::_commit_native(int64_t p_index)
{
    if (p_index < 0 || p_index >= items.size())
    {
        return;
    }
    Dictionary item = items[p_index];
    Ref<SignalLink> link = item["link"];
    Object *source = _source();
    StringName signal = _signal();
    Object *target = link->resolve_target(source);
    Dictionary method_info = _method_info(target, link->get_method());
    int k = item.get("k", 0);
    Variant old = item.get("callable", Variant());
    bool old_connected = old.get_type() == Variant::CALLABLE && source && source->is_connected(signal, old);
    bool complete = !method_info.is_empty() && is_native_eligible(source, target);

    Array do_calls;
    Array undo_calls;
    if (old_connected)
    {
        do_calls.push_back(Array::make(source, "disconnect", signal, old));
    }
    if (complete)
    {
        Array params = method_info.get("args", Array());
        TypedArray<ArgumentBinding> arguments = link->get_arguments();
        Array constants;
        for (int p = k; p < params.size() && p < arguments.size(); p++)
        {
            Variant constant = Ref<ArgumentBinding>(arguments[p])->get_constant();
            String error = ArgumentBinding::constant_error(constant);
            if (!error.is_empty())
            {
                log_it(U"[color=orange]" + error + U"[/color]", this);
                return;
            }
            constants.push_back(constant);
        }
        Callable callable = make_native_callable(target, link->get_method(), constants, _signal_args().size() - k);
        do_calls.push_back(Array::make(source, "connect", signal, callable, (int)Object::CONNECT_PERSIST));
        undo_calls.push_back(Array::make(source, "disconnect", signal, callable));
        native_drafts.erase(link);
    }
    else if (old_connected && !native_drafts.has(link))
    {
        native_drafts.push_back(link); // devenu incomplet : on garde le brouillon à l'écran
    }
    if (old_connected)
    {
        undo_calls.push_back(Array::make(source, "connect", signal, old, (int)Object::CONNECT_PERSIST));
    }
    if (do_calls.is_empty())
    {
        _rebuild_deferred();
        return;
    }
    _commit(U"Lien natif « " + String(signal) + U" » → " + String(link->get_method()), do_calls, undo_calls);
}

void SignalWidget::_on_emit()
{
    Object *source = _source();
    if (!source)
    {
        return;
    }
    Array arguments;
    arguments.push_back(_signal());
    Array signal_args = _signal_args();
    for (int i = 0; i < signal_args.size(); i++)
    {
        int type = Dictionary(signal_args[i]).get("type", 0);
        arguments.push_back(type == Variant::NIL || type == Variant::OBJECT ? Variant() : UtilityFunctions::type_convert(Variant(), type));
    }
    source->callv("emit_signal", arguments);
}

void SignalWidget::_on_add()
{
    Ref<SignalLink> link;
    link.instantiate(); // cible par défaut : l'émetteur lui-même
    if (_native_mode())
    {
        // Brouillon natif : connecté dès qu'une méthode est choisie.
        native_drafts.push_back(link);
        _rebuild();
        return;
    }
    Ref<DataEntry> data_entry = get_entry();
    if (data_entry.is_null())
    {
        return;
    }
    TypedArray<SignalLink> links = data_entry->get_links().duplicate();
    links.push_back(link);
    _commit_sets(Array::make(Array::make(data_entry, "links", links)));
}

void SignalWidget::_on_remove(int64_t p_index)
{
    Dictionary item = items[p_index];
    Ref<SignalLink> link = item["link"];
    if (bool(item["native"]))
    {
        if (native_drafts.has(link))
        {
            native_drafts.erase(link);
            _rebuild_deferred();
            return;
        }
        Object *source = _source();
        Variant old = item.get("callable", Variant());
        if (source && old.get_type() == Variant::CALLABLE && source->is_connected(_signal(), old))
        {
            _commit(U"Supprimer le lien natif « " + String(_signal()) + U" »", Array::make(Array::make(source, "disconnect", _signal(), old)),
                    Array::make(Array::make(source, "connect", _signal(), old, (int)Object::CONNECT_PERSIST)));
        }
        return;
    }
    Ref<DataEntry> data_entry = get_entry();
    TypedArray<SignalLink> links = data_entry->get_links().duplicate();
    links.erase(link);
    link->disconnect_from(_source());
    _commit_sets(Array::make(Array::make(data_entry, "links", links)));
}

void SignalWidget::_on_enabled_toggled(bool p_on, int64_t p_index)
{
    Ref<SignalLink> link = Dictionary(items[p_index])["link"];
    _commit_sets(Array::make(Array::make(link, "enabled", p_on)));
}

void SignalWidget::_on_target_submitted(const String &p_text, int64_t p_index)
{
    Dictionary item = items[p_index];
    Ref<SignalLink> link = item["link"];
    NodePath path = p_text;
    if (bool(item["native"]))
    {
        Ref<SignalLink> probe;
        probe.instantiate();
        probe->set_target_path(path);
        if (!is_native_eligible(_source(), probe->resolve_target(_source())))
        {
            log_it(U"[color=orange]Lien natif : la cible doit être un nœud de la scène éditée[/color]", this);
            _rebuild_deferred();
            return;
        }
        link->set_target_path(path); // lien natif : modèle local, validé par reconnexion
        _commit_native(p_index);
        return;
    }
    _commit_sets(Array::make(Array::make(link, "target_path", path), Array::make(link, "target_resource", Variant())));
}

void SignalWidget::_on_pick_node(int64_t p_index)
{
    if (EditorInterface *editor = editor_interface())
    {
        editor->popup_node_selector(callable_mp(this, &SignalWidget::_on_node_picked).bind(p_index));
        return;
    }
    NodeSelectorDialog::open(this, ArgumentBinding::fallback_base_node(), PackedStringArray(), callable_mp(this, &SignalWidget::_on_runtime_node_picked).bind(p_index));
}

void SignalWidget::_on_node_picked(const NodePath &p_path, int64_t p_index)
{
    EditorInterface *editor = editor_interface();
    Node *root = editor ? editor->get_edited_scene_root() : nullptr;
    if (!p_path.is_empty() && root)
    {
        _apply_target_node(root->get_node_or_null(p_path), p_index); // chemin relatif à la racine de la scène éditée
    }
}

void SignalWidget::_on_runtime_node_picked(Node *p_node, int64_t p_index) { _apply_target_node(p_node, p_index); }

void SignalWidget::_apply_target_node(Node *p_node, int64_t p_index)
{
    if (!p_node)
    {
        return;
    }
    Node *source = Object::cast_to<Node>(_source());
    _on_target_submitted(String(source && source->is_inside_tree() ? source->get_path_to(p_node) : p_node->get_path()), p_index);
}

void SignalWidget::_on_pick_resource(int64_t p_index)
{
    if (EditorInterface *editor = editor_interface())
    {
        TypedArray<StringName> bases;
        bases.push_back(StringName("Resource"));
        editor->popup_quick_open(callable_mp(this, &SignalWidget::_on_resource_picked).bind(p_index), bases);
    }
}

void SignalWidget::_on_resource_picked(const String &p_path, int64_t p_index)
{
    if (p_path.is_empty())
    {
        return;
    }
    Ref<Resource> resource = ResourceLoader::get_singleton()->load(p_path);
    if (resource.is_null())
    {
        return;
    }
    Ref<SignalLink> link = Dictionary(items[p_index])["link"];
    _commit_sets(Array::make(Array::make(link, "target_resource", resource), Array::make(link, "target_path", NodePath())));
}

void SignalWidget::_on_method_selected(int64_t p_item, int64_t p_index, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    if (!option)
    {
        return;
    }
    String method = option->get_item_metadata(p_item);
    if (!method.is_empty())
    {
        _set_method(method, p_index);
    }
}

void SignalWidget::_on_pick_method(int64_t p_index)
{
    Ref<SignalLink> link = Dictionary(items[p_index])["link"];
    Object *target = link->resolve_target(_source());
    if (EditorInterface *editor = editor_interface())
    {
        if (target)
        {
            editor->popup_method_selector(target, callable_mp(this, &SignalWidget::_on_method_picked).bind(p_index), String(link->get_method()));
        }
    }
}

void SignalWidget::_on_method_picked(const String &p_method, int64_t p_index)
{
    if (!p_method.is_empty())
    {
        _set_method(p_method, p_index);
    }
}

void SignalWidget::_set_method(const StringName &p_method, int64_t p_index)
{
    Dictionary item = items[p_index];
    Ref<SignalLink> link = item["link"];
    Dictionary method_info = _method_info(link->resolve_target(_source()), p_method);
    if (bool(item["native"]))
    {
        link->set_method(p_method);
        link->set_arguments(default_arguments(method_info));
        item["k"] = max_native_prefix(_signal_args(), method_info.get("args", Array())); // par défaut : tout ce qui peut venir du signal
        _commit_native(p_index);
        return;
    }
    _commit_sets(Array::make(Array::make(link, "method", p_method), Array::make(link, "arguments", default_arguments(method_info))));
}

void SignalWidget::_on_k_changed(double p_value, int64_t p_index)
{
    Dictionary item = items[p_index];
    item["k"] = (int)p_value;
    Ref<SignalLink> link = item["link"];
    _adapt_arguments(link, _method_info(link->resolve_target(_source()), link->get_method()), true, (int)p_value);
    _commit_native(p_index);
}

void SignalWidget::_on_native_constant_changed(const Variant &p_value, bool p_committing, int64_t p_index)
{
    (void)p_value;
    if (p_committing)
    {
        // Différé : l'écriture de la constante (ProceduralEditor) est faite avant la reconnexion.
        callable_mp(this, &SignalWidget::_commit_native).call_deferred(p_index);
    }
}

void SignalWidget::_on_source_selected(int64_t p_item, int64_t p_index, int64_t p_param, uint64_t p_option_id)
{
    OptionButton *option = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_option_id));
    Ref<SignalLink> link = Dictionary(items[p_index])["link"];
    TypedArray<ArgumentBinding> arguments = link->get_arguments();
    if (!option || p_param < 0 || p_param >= arguments.size())
    {
        return;
    }
    Ref<ArgumentBinding> binding = arguments[p_param];
    int id = option->get_item_id(p_item);
    if (id >= 100)
    {
        _commit_sets(Array::make(Array::make(binding, "source", ArgumentBinding::SOURCE_SIGNAL_ARG), Array::make(binding, "signal_index", id - 100)));
    }
    else
    {
        _commit_sets(Array::make(Array::make(binding, "source", id == 1 ? ArgumentBinding::SOURCE_PROPERTY : ArgumentBinding::SOURCE_CONSTANT)));
    }
}

void SignalWidget::_on_property_path_submitted(const String &p_text, int64_t p_index, int64_t p_param, bool p_object_part)
{
    Ref<SignalLink> link = Dictionary(items[p_index])["link"];
    TypedArray<ArgumentBinding> arguments = link->get_arguments();
    if (p_param < 0 || p_param >= arguments.size())
    {
        return;
    }
    Ref<ArgumentBinding> binding = arguments[p_param];
    _commit_sets(Array::make(Array::make(binding, p_object_part ? "object_path" : "property_path", NodePath(p_text))));
}
