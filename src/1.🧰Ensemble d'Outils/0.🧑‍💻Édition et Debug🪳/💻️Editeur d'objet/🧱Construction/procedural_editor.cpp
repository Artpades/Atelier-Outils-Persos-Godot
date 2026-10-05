#include "procedural_editor.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/preview_widget.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔌Accès/runtime_undo_backend.hpp"

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

// --- ProceduralEditorContext ---

void ProceduralEditorContext::set_editor(ProceduralEditor *p_editor) { editor_id = p_editor ? ObjectID(p_editor->get_instance_id()) : ObjectID(); }

PropertyWidget *ProceduralEditorContext::build_widget(const Ref<ValueAccessor> &p_accessor, const Ref<DataEntry> &p_entry, int p_depth, const PackedInt64Array &p_ancestors)
{
    ProceduralEditor *editor = Object::cast_to<ProceduralEditor>(ObjectDB::get_instance(editor_id));
    return editor ? editor->build_widget(p_accessor, p_entry, p_depth, p_ancestors) : nullptr;
}

int ProceduralEditorContext::get_max_depth() const
{
    ProceduralEditor *editor = Object::cast_to<ProceduralEditor>(ObjectDB::get_instance(editor_id));
    return editor ? editor->get_max_depth() : 0;
}

void ProceduralEditorContext::links_changed()
{
    if (ProceduralEditor *editor = Object::cast_to<ProceduralEditor>(ObjectDB::get_instance(editor_id)))
    {
        editor->reapply_links();
    }
}

Ref<LayoutDatabase> ProceduralEditorContext::get_layout_database() const
{
    ProceduralEditor *editor = Object::cast_to<ProceduralEditor>(ObjectDB::get_instance(editor_id));
    return editor && editor->get_use_layouts() ? editor->get_effective_layout_database() : Ref<LayoutDatabase>();
}

Ref<UndoBackend> ProceduralEditorContext::get_undo_backend() const
{
    ProceduralEditor *editor = Object::cast_to<ProceduralEditor>(ObjectDB::get_instance(editor_id));
    return editor ? editor->get_undo_backend() : Ref<UndoBackend>();
}

// --- ProceduralEditor ---

void ProceduralEditor::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_target", "target"), &ProceduralEditor::set_target);
    ClassDB::bind_method(D_METHOD("get_target"), &ProceduralEditor::get_target);
    ClassDB::bind_method(D_METHOD("set_target_path", "path"), &ProceduralEditor::set_target_path);
    ClassDB::bind_method(D_METHOD("get_target_path"), &ProceduralEditor::get_target_path);
    ClassDB::bind_method(D_METHOD("set_target_resource", "resource"), &ProceduralEditor::set_target_resource);
    ClassDB::bind_method(D_METHOD("get_target_resource"), &ProceduralEditor::get_target_resource);
    ClassDB::bind_method(D_METHOD("resolve_declared_target"), &ProceduralEditor::resolve_declared_target);
    ClassDB::bind_method(D_METHOD("set_registry", "registry"), &ProceduralEditor::set_registry);
    ClassDB::bind_method(D_METHOD("get_registry"), &ProceduralEditor::get_registry);
    ClassDB::bind_method(D_METHOD("set_group_registry", "groups"), &ProceduralEditor::set_group_registry);
    ClassDB::bind_method(D_METHOD("get_group_registry"), &ProceduralEditor::get_group_registry);
    ClassDB::bind_method(D_METHOD("clear_registries"), &ProceduralEditor::clear_registries);
    ClassDB::bind_method(D_METHOD("set_layout_database", "database"), &ProceduralEditor::set_layout_database);
    ClassDB::bind_method(D_METHOD("get_layout_database"), &ProceduralEditor::get_layout_database);
    ClassDB::bind_method(D_METHOD("get_effective_layout_database"), &ProceduralEditor::get_effective_layout_database);
    ClassDB::bind_method(D_METHOD("set_use_layouts", "use"), &ProceduralEditor::set_use_layouts);
    ClassDB::bind_method(D_METHOD("get_use_layouts"), &ProceduralEditor::get_use_layouts);
    ClassDB::bind_method(D_METHOD("set_draw_gizmos", "draw"), &ProceduralEditor::set_draw_gizmos);
    ClassDB::bind_method(D_METHOD("get_draw_gizmos"), &ProceduralEditor::get_draw_gizmos);
    ClassDB::bind_method(D_METHOD("set_style", "style"), &ProceduralEditor::set_style);
    ClassDB::bind_method(D_METHOD("get_style"), &ProceduralEditor::get_style);
    ClassDB::bind_method(D_METHOD("set_styles_enabled", "enabled"), &ProceduralEditor::set_styles_enabled);
    ClassDB::bind_method(D_METHOD("get_styles_enabled"), &ProceduralEditor::get_styles_enabled);
    ClassDB::bind_method(D_METHOD("get_effective_style"), &ProceduralEditor::get_effective_style);
    ClassDB::bind_method(D_METHOD("get_styled_count"), &ProceduralEditor::get_styled_count);
    ClassDB::bind_method(D_METHOD("evaluate_behaviors"), &ProceduralEditor::evaluate_behaviors);
    ClassDB::bind_method(D_METHOD("get_rich_tooltip"), &ProceduralEditor::get_rich_tooltip);
    ClassDB::bind_method(D_METHOD("get_active_layout"), &ProceduralEditor::get_active_layout);
    ClassDB::bind_method(D_METHOD("get_active_registry"), &ProceduralEditor::get_active_registry);
    ClassDB::bind_method(D_METHOD("get_active_group_registry"), &ProceduralEditor::get_active_group_registry);
    ClassDB::bind_method(D_METHOD("get_gizmo_host"), &ProceduralEditor::get_gizmo_host);
    ClassDB::bind_method(D_METHOD("set_max_depth", "depth"), &ProceduralEditor::set_max_depth);
    ClassDB::bind_method(D_METHOD("get_max_depth"), &ProceduralEditor::get_max_depth);
    ClassDB::bind_method(D_METHOD("set_undo_backend", "backend"), &ProceduralEditor::set_undo_backend);
    ClassDB::bind_method(D_METHOD("get_undo_backend"), &ProceduralEditor::get_undo_backend);
    ClassDB::bind_method(D_METHOD("set_factory", "factory"), &ProceduralEditor::set_factory);
    ClassDB::bind_method(D_METHOD("get_factory"), &ProceduralEditor::get_factory);
    ClassDB::bind_method(D_METHOD("set_scroll_enabled", "enabled"), &ProceduralEditor::set_scroll_enabled);
    ClassDB::bind_method(D_METHOD("is_scroll_enabled"), &ProceduralEditor::is_scroll_enabled);
    ClassDB::bind_method(D_METHOD("set_sync_interval", "interval"), &ProceduralEditor::set_sync_interval);
    ClassDB::bind_method(D_METHOD("get_sync_interval"), &ProceduralEditor::get_sync_interval);
    ClassDB::bind_method(D_METHOD("rebuild"), &ProceduralEditor::rebuild);
    ClassDB::bind_method(D_METHOD("get_widget", "property"), &ProceduralEditor::get_widget);
    ClassDB::bind_method(D_METHOD("undo"), &ProceduralEditor::undo);
    ClassDB::bind_method(D_METHOD("redo"), &ProceduralEditor::redo);
    ClassDB::bind_method(D_METHOD("reapply_links"), &ProceduralEditor::reapply_links);

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_path"), "set_target_path", "get_target_path");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target_resource", PROPERTY_HINT_RESOURCE_TYPE, "Resource"), "set_target_resource", "get_target_resource");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "registry", PROPERTY_HINT_RESOURCE_TYPE, "DataRegistry"), "set_registry", "get_registry");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "group_registry", PROPERTY_HINT_RESOURCE_TYPE, "GroupRegistry"), "set_group_registry", "get_group_registry");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_depth", PROPERTY_HINT_RANGE, "1,16,1"), "set_max_depth", "get_max_depth");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layout_database", PROPERTY_HINT_RESOURCE_TYPE, "LayoutDatabase"), "set_layout_database", "get_layout_database");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_layouts"), "set_use_layouts", "get_use_layouts");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_gizmos"), "set_draw_gizmos", "get_draw_gizmos");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "style", PROPERTY_HINT_RESOURCE_TYPE, "InterfaceStyle"), "set_style", "get_style");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "styles_enabled"), "set_styles_enabled", "get_styles_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sync_interval", PROPERTY_HINT_RANGE, "0,5,0.01,suffix:s"), "set_sync_interval", "get_sync_interval");
}

ProceduralEditor::ProceduralEditor()
{
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    context.instantiate();
    context->set_editor(this);

    status = memnew(Label);
    status->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    status->set_modulate(Color(1, 1, 1, 0.7));
    add_child(status, false, INTERNAL_MODE_FRONT);

    scroll = memnew(ScrollContainer);
    scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(scroll, false, INTERNAL_MODE_FRONT);

    content = memnew(VBoxContainer);
    content->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->add_child(content);

    sync = memnew(SyncController);
    sync->connect("target_lost", callable_mp(this, &ProceduralEditor::_on_target_lost));
    sync->connect("synced", callable_mp(this, &ProceduralEditor::_evaluate_behaviors));
    add_child(sync, false, INTERNAL_MODE_BACK);

    rich_tooltip = memnew(RichTooltip);
    rich_tooltip->setup(this);
    add_child(rich_tooltip, false, INTERNAL_MODE_BACK);

    _show_status("Aucun objet");
}

void ProceduralEditor::_ready()
{
    set_process_input(true);
    // En jeu, la cible déclarée est appliquée automatiquement. Dans l'éditeur, un ProceduralEditor de la scène
    // éditée ne se branche pas sur sa cible (pas d'édition depuis le canevas) : c'est la fenêtre de configuration
    // qui affiche l'aperçu.
    if (!Engine::get_singleton()->is_editor_hint() && !get_target())
    {
        if (Object *declared = resolve_declared_target())
        {
            set_target(declared);
        }
    }
}

void ProceduralEditor::set_target_path(const NodePath &p_path) { target_path = p_path; }
NodePath ProceduralEditor::get_target_path() const { return target_path; }
void ProceduralEditor::set_target_resource(const Ref<Resource> &p_resource) { target_resource = p_resource; }
Ref<Resource> ProceduralEditor::get_target_resource() const { return target_resource; }

Object *ProceduralEditor::resolve_declared_target() const
{
    if (target_resource.is_valid())
    {
        return target_resource.ptr();
    }
    if (!target_path.is_empty() && is_inside_tree())
    {
        return get_node_or_null(target_path);
    }
    return nullptr;
}

PropertyWidget *ProceduralEditor::_widget_of(Node *p_control) const
{
    for (Node *node = p_control; node && node != this; node = node->get_parent())
    {
        if (PropertyWidget *widget = Object::cast_to<PropertyWidget>(node))
        {
            return widget;
        }
    }
    return nullptr;
}

void ProceduralEditor::_input(const Ref<InputEvent> &p_event)
{
    Ref<InputEventKey> key = p_event;
    if (key.is_null() || !key->is_pressed() || key->is_echo() || !key->is_command_or_control_pressed())
    {
        return;
    }
    bool is_undo = key->get_keycode() == KEY_Z && !key->is_shift_pressed();
    bool is_redo = (key->get_keycode() == KEY_Z && key->is_shift_pressed()) || key->get_keycode() == KEY_Y;
    if ((!is_undo && !is_redo) || !is_visible_in_tree())
    {
        return;
    }
    // Raccourci retransmis à l'éditeur par EditorUndoBackend : ne jamais le réintercepter (récursion infinie).
    // Sans cible (ex: ProceduralEditor de la scène éditée, non branché dans l'éditeur), rien à annuler ici.
    if (key->has_meta(FORWARDED_SHORTCUT_META) || !get_target())
    {
        return;
    }
    Window *window = get_window();
    if (!window || !window->has_focus())
    {
        return;
    }

    // Focus sur une autre interface de la fenêtre : son Ctrl+Z ne nous concerne pas.
    Control *focus = window->gui_get_focus_owner();
    if (focus && !is_ancestor_of(focus))
    {
        return;
    }
    // Saisie de texte en cours (non validée) : on laisse au champ son annulation de texte.
    PropertyWidget *focused_widget = focus ? _widget_of(focus) : nullptr;
    if (focused_widget && focused_widget->has_uncommitted_edit())
    {
        return;
    }

    if (is_redo)
    {
        redo();
    }
    else
    {
        undo();
    }
    // Consommé même sans rien à annuler : le champ focalisé ne doit pas l'interpréter.
    window->set_input_as_handled();
}

void ProceduralEditor::_resolve_registries(Object *p_target)
{
    active_layout.unref();
    if (registry.is_valid() || !p_target)
    {
        // Registres donnés à l'éditeur (ou pas de cible) : ils priment.
        if (registry.is_valid() && group_registry.is_null())
        {
            group_registry.instantiate();
        }
        active_registry = registry;
        active_groups = group_registry;
        return;
    }
    Ref<LayoutDatabase> database = use_layouts ? get_effective_layout_database() : Ref<LayoutDatabase>();
    Ref<TypeLayout> layout = database.is_valid() ? database->find_layout(p_target) : Ref<TypeLayout>();
    if (layout.is_valid())
    {
        layout->ensure_registries();
        active_layout = layout;
        active_registry = layout->get_data_registry();
        active_groups = layout->get_group_registry();
        return;
    }
    // Interface générale : registres automatiques (conservés par l'éditeur, comme avant).
    registry.instantiate();
    if (group_registry.is_null())
    {
        group_registry.instantiate();
    }
    active_registry = registry;
    active_groups = group_registry;
}

void ProceduralEditor::_update_gizmos()
{
    Ref<GizmoRegistry> gizmos = (draw_gizmos && active_layout.is_valid()) ? active_layout->get_gizmo_registry() : Ref<GizmoRegistry>();
    if (gizmos.is_null())
    {
        if (gizmo_host)
        {
            gizmo_host->set_registry(Ref<GizmoRegistry>());
            gizmo_host->set_target_object(nullptr);
        }
        return;
    }
    if (!gizmo_host)
    {
        gizmo_host = memnew(GizmoHost);
        add_child(gizmo_host, false, INTERNAL_MODE_BACK);
    }
    gizmo_host->set_registry(gizmos);
    gizmo_host->set_target_object(get_target()); // dessin dans le viewport de la cible
}

void ProceduralEditor::set_target(Object *p_target)
{
    target_id = p_target ? ObjectID(p_target->get_instance_id()) : ObjectID();
    _resolve_registries(p_target);
    if (p_target && active_registry.is_valid())
    {
        active_registry->sync(p_target, active_groups);
    }
    rebuild();
    reapply_links();
    _update_gizmos();
}

Object *ProceduralEditor::get_target() const { return ObjectDB::get_instance(target_id); }

void ProceduralEditor::set_registry(const Ref<DataRegistry> &p_registry)
{
    registry = p_registry;
    Object *target = get_target();
    _resolve_registries(target);
    if (target && active_registry.is_valid())
    {
        active_registry->sync(target, active_groups);
    }
    rebuild();
    reapply_links();
    _update_gizmos();
}

Ref<DataRegistry> ProceduralEditor::get_registry() const { return registry; }

void ProceduralEditor::set_group_registry(const Ref<GroupRegistry> &p_groups)
{
    group_registry = p_groups;
    if (registry.is_valid() || !get_target())
    {
        active_groups = group_registry;
    }
    rebuild();
}

void ProceduralEditor::clear_registries()
{
    registry.unref();
    group_registry.unref();
}

void ProceduralEditor::set_layout_database(const Ref<LayoutDatabase> &p_database) { layout_database = p_database; }
Ref<LayoutDatabase> ProceduralEditor::get_layout_database() const { return layout_database; }
Ref<LayoutDatabase> ProceduralEditor::get_effective_layout_database() const { return layout_database.is_valid() ? layout_database : LayoutDatabase::load_project_database(); }
void ProceduralEditor::set_use_layouts(bool p_use) { use_layouts = p_use; }
bool ProceduralEditor::get_use_layouts() const { return use_layouts; }
void ProceduralEditor::set_draw_gizmos(bool p_draw)
{
    draw_gizmos = p_draw;
    _update_gizmos();
}
bool ProceduralEditor::get_draw_gizmos() const { return draw_gizmos; }

void ProceduralEditor::set_style(const Ref<InterfaceStyle> &p_style)
{
    style = p_style;
    if (is_inside_tree())
    {
        rebuild();
    }
}
Ref<InterfaceStyle> ProceduralEditor::get_style() const { return style; }
void ProceduralEditor::set_styles_enabled(bool p_enabled)
{
    styles_enabled = p_enabled;
    if (is_inside_tree())
    {
        rebuild();
    }
}
bool ProceduralEditor::get_styles_enabled() const { return styles_enabled; }

Ref<InterfaceStyle> ProceduralEditor::get_effective_style() const
{
    if (!styles_enabled)
    {
        return Ref<InterfaceStyle>();
    }
    if (style.is_valid())
    {
        return style;
    }
    if (active_layout.is_valid() && active_layout->get_style().is_valid())
    {
        return active_layout->get_style();
    }
    return InterfaceStyle::load_project_default();
}

int ProceduralEditor::get_styled_count() const { return sync->get_styled_count(); }

RichTooltip *ProceduralEditor::get_rich_tooltip() const { return rich_tooltip; }

void ProceduralEditor::evaluate_behaviors() { _evaluate_behaviors(); }

Control *ProceduralEditor::_behavior_target(const String &p_name, CanvasItem **r_style_item, Ref<InterfaceStyle> *r_base_style) const
{
    // Donnée : son widget de premier niveau (style de base : celui des réglages s'il vise le widget entier).
    if (top_widgets.has(p_name))
    {
        PropertyWidget *widget = Object::cast_to<PropertyWidget>(ObjectDB::get_instance(uint64_t(int64_t(top_widgets[p_name]))));
        if (widget)
        {
            *r_style_item = widget;
            Ref<WidgetSettings> settings = widget->get_entry().is_valid() ? widget->get_entry()->get_settings() : Ref<WidgetSettings>();
            *r_base_style = (settings.is_valid() && settings->get_style_target() == WidgetSettings::STYLE_WIDGET) ? settings->get_style() : Ref<InterfaceStyle>();
        }
        return widget;
    }
    // Groupe : nœud extérieur (visibilité), racine (style, de base celui du groupe).
    if (group_nodes.has(p_name))
    {
        Array nodes = group_nodes[p_name];
        Control *outer = Object::cast_to<Control>(ObjectDB::get_instance(uint64_t(int64_t(nodes[0]))));
        *r_style_item = Object::cast_to<CanvasItem>(ObjectDB::get_instance(uint64_t(int64_t(nodes[1]))));
        Ref<GroupEntry> group = active_groups.is_valid() ? active_groups->find(p_name) : Ref<GroupEntry>();
        *r_base_style = (styles_enabled && group.is_valid()) ? group->get_style() : Ref<InterfaceStyle>();
        return outer;
    }
    return nullptr;
}

void ProceduralEditor::_evaluate_behaviors()
{
    Object *target = get_target();
    TypedArray<WidgetBehavior> behaviors = active_registry.is_valid() ? active_registry->get_behaviors() : TypedArray<WidgetBehavior>();
    if (!target || behaviors.is_empty())
    {
        return;
    }
    // États combinés par cible (toutes les cibles citées, même si aucun comportement n'y est actif).
    HashMap<String, BehaviorState> states;
    PackedStringArray order;
    for (int b = 0; b < behaviors.size(); b++)
    {
        Ref<WidgetBehavior> behavior = behaviors[b];
        if (behavior.is_null() || !behavior->get_enabled())
        {
            continue;
        }
        bool active = behavior->evaluate(target);
        PackedStringArray targets = behavior->get_targets();
        for (const String &name : targets)
        {
            if (!states.has(name))
            {
                states.insert(name, BehaviorState());
                order.push_back(name);
            }
            behavior->contribute(states[name], active);
            // Comportement de script : appelé quand son état change (ou après une reconstruction).
            if (behavior->has_script_apply())
            {
                String key = String::num_int64((int64_t)behavior->get_instance_id()) + ":" + name;
                if (!script_behavior_states.has(key) || bool(script_behavior_states[key]) != active)
                {
                    script_behavior_states[key] = active;
                    CanvasItem *unused_item = nullptr;
                    Ref<InterfaceStyle> unused_style;
                    behavior->apply_script(_behavior_target(name, &unused_item, &unused_style), active);
                }
            }
        }
    }

    for (const String &name : order)
    {
        const BehaviorState &state = states[name];
        CanvasItem *style_item = nullptr;
        Ref<InterfaceStyle> base_style;
        Control *control = _behavior_target(name, &style_item, &base_style);
        if (!control)
        {
            continue;
        }
        // Signature : rien n'est réappliqué tant que l'état ne change pas.
        String styles_key;
        for (const Ref<InterfaceStyle> &style : state.styles)
        {
            styles_key += String::num_int64((int64_t)style->get_instance_id()) + ",";
        }
        String signature = String::num_int64(state.visible) + "|" + String::num_int64(state.editable) + "|" + styles_key + "|" + String("\n").join(state.tooltips) + "|" +
                           (state.icon.is_valid() ? String::num_int64((int64_t)state.icon->get_instance_id()) : String());
        if (behavior_signatures.has(name) && String(behavior_signatures[name]) == signature)
        {
            continue;
        }
        behavior_signatures[name] = signature;

        // Page d'un groupe en onglets : c'est l'onglet qui est masqué.
        if (TabContainer *tabs = Object::cast_to<TabContainer>(control->get_parent()))
        {
            tabs->set_tab_hidden(control->get_index(), !state.visible);
        }
        else
        {
            control->set_visible(state.visible);
        }
        PropertyWidget *widget = Object::cast_to<PropertyWidget>(control);
        TypedArray<Node> locked_widgets;
        if (widget)
        {
            locked_widgets.push_back(widget);
        }
        else
        {
            locked_widgets = control->find_children("*", "PropertyWidget", true, false);
        }

        // Styles conditionnels (déverrouillés le temps de la restauration : l'atténuation suit la teinte).
        if (style_item && (!state.styles.is_empty() || behavior_styled.has(name)))
        {
            for (int i = 0; i < locked_widgets.size(); i++)
            {
                Object::cast_to<PropertyWidget>(locked_widgets[i])->set_locked(false);
            }
            InterfaceStyle::restore(style_item, base_style);
            for (const Ref<InterfaceStyle> &style : state.styles)
            {
                style->apply_to(style_item);
                sync->register_styled(style_item, style);
            }
            if (state.styles.is_empty())
            {
                behavior_styled.erase(name);
            }
            else
            {
                behavior_styled[name] = true;
            }
        }
        for (int i = 0; i < locked_widgets.size(); i++)
        {
            Object::cast_to<PropertyWidget>(locked_widgets[i])->set_locked(!state.editable);
        }
        if (widget)
        {
            widget->set_icon_override(state.icon);
            PackedStringArray tooltips;
            if (widget->get_entry().is_valid() && !widget->get_entry()->get_tooltip().is_empty())
            {
                tooltips.push_back(widget->get_entry()->get_tooltip());
            }
            tooltips.append_array(state.tooltips);
            widget->set_tooltip_texts(tooltips);
        }
    }
}

void ProceduralEditor::_apply_global_style()
{
    // Seul ce que l'éditeur a lui-même posé est retiré (un modulate réglé dans la scène est conservé sinon).
    if (global_style_applied)
    {
        InterfaceStyle::clear_from(this);
        global_style_applied = false;
    }
    Ref<InterfaceStyle> effective = get_effective_style();
    global_background = effective.is_valid() ? effective->get_background() : Ref<StyleBox>();
    queue_redraw();
    if (effective.is_null())
    {
        return;
    }
    effective->apply_to(this);
    global_style_applied = true;
    sync->register_styled(this, effective);
}
Ref<TypeLayout> ProceduralEditor::get_active_layout() const { return active_layout; }
Ref<DataRegistry> ProceduralEditor::get_active_registry() const { return active_registry; }
Ref<GroupRegistry> ProceduralEditor::get_active_group_registry() const { return active_groups; }
GizmoHost *ProceduralEditor::get_gizmo_host() const { return gizmo_host; }

Ref<GroupRegistry> ProceduralEditor::get_group_registry() const { return group_registry; }

void ProceduralEditor::set_max_depth(int p_depth) { max_depth = CLAMP(p_depth, 1, 16); }
int ProceduralEditor::get_max_depth() const { return max_depth; }

PropertyWidget *ProceduralEditor::build_widget(const Ref<ValueAccessor> &p_accessor, const Ref<DataEntry> &p_entry, int p_depth, const PackedInt64Array &p_ancestors)
{
    if (p_depth > max_depth || p_accessor.is_null() || p_entry.is_null())
    {
        return nullptr;
    }
    if (factory.is_null())
    {
        factory.instantiate();
    }
    PropertyWidget *widget = factory->create(p_entry);
    widget->set_context(context, p_depth, p_ancestors);
    widget->set_entry(p_entry);
    widget->setup(p_accessor, p_entry->get_settings(), p_entry->resolved_choice(), p_entry->get_is_vertical());
    widget->connect("value_changed", callable_mp(this, &ProceduralEditor::_on_value_changed).bind((uint64_t)widget->get_instance_id()));
    widget->connect("edit_discarded", callable_mp(this, &ProceduralEditor::_on_edit_discarded).bind((uint64_t)widget->get_instance_id()));
    sync->register_widget(widget);
    if (!p_entry->get_tooltip().is_empty())
    {
        PackedStringArray tooltips;
        tooltips.push_back(p_entry->get_tooltip());
        widget->set_tooltip_texts(tooltips);
    }
    Ref<WidgetSettings> settings = p_entry->get_settings();
    if (styles_enabled && settings.is_valid())
    {
        sync->register_styled(widget->get_styled_item(), settings->get_style()); // uniforms liés seulement
    }
    if (widget->is_value_widget() && p_accessor->is_valid())
    {
        Variant value = widget->read_value();
        widget->set_last_value(value);
        widget->pull(value);
    }
    return widget;
}

void ProceduralEditor::_notification(int p_what)
{
    if (p_what == NOTIFICATION_DRAW && global_background.is_valid())
    {
        draw_style_box(global_background, Rect2(Vector2(), get_size()));
    }
    if (p_what == NOTIFICATION_PREDELETE)
    {
        // Les liens connectés par cet éditeur ne doivent pas lui survivre : sans cible, reapply_links ne fait
        // que les retirer.
        target_id = ObjectID();
        reapply_links();
    }
}

void ProceduralEditor::reapply_links()
{
    for (int i = 0; i < applied_links.size(); i++)
    {
        Array applied = applied_links[i];
        Ref<SignalLink> link = applied[0];
        Object *source = ObjectDB::get_instance(uint64_t(int64_t(applied[1])));
        if (link.is_valid() && source)
        {
            link->disconnect_from(source);
        }
    }
    applied_links.clear();

    Object *target = get_target();
    if (!target || active_registry.is_null() || Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    // Nœud dont l'interface vient de la base par type : ses liens sont appliqués par le LayoutLinker (node_added).
    if (active_layout.is_valid() && Object::cast_to<Node>(target))
    {
        return;
    }
    TypedArray<DataEntry> entries = active_registry->get_entries();
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<DataEntry> entry = entries[i];
        if (entry.is_null() || entry->get_member() != DataEntry::MEMBER_SIGNAL || entry->get_is_missing())
        {
            continue;
        }
        TypedArray<SignalLink> links = entry->get_links();
        for (int l = 0; l < links.size(); l++)
        {
            Ref<SignalLink> link = links[l];
            if (link.is_null())
            {
                continue;
            }
            link->connect_to(target, entry->get_data_name());
            applied_links.push_back(Array::make(link, (int64_t)target->get_instance_id()));
        }
    }
}

Dictionary ProceduralEditor::_find_signal(Object *p_target, const StringName &p_signal)
{
    TypedArray<Dictionary> signals = p_target ? p_target->get_signal_list() : TypedArray<Dictionary>();
    for (int i = 0; i < signals.size(); i++)
    {
        Dictionary info = signals[i];
        if (StringName(info.get("name", "")) == p_signal)
        {
            return info;
        }
    }
    return Dictionary();
}

Dictionary ProceduralEditor::_find_method(Object *p_target, const StringName &p_method)
{
    TypedArray<Dictionary> methods = p_target ? p_target->get_method_list() : TypedArray<Dictionary>();
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

Container *ProceduralEditor::_make_group_container(const Ref<GroupEntry> &p_group, VBoxContainer *&r_root, Container *p_parent)
{
    TabContainer *parent_tabs = Object::cast_to<TabContainer>(p_parent);
    r_root = memnew(VBoxContainer);
    r_root->set_h_size_flags(SIZE_EXPAND_FILL);
    if (!p_group->get_group_name().is_empty() && !parent_tabs) // page d'onglet : le titre est celui de l'onglet
    {
        HBoxContainer *title_row = memnew(HBoxContainer);
        if (p_group->get_icon().is_valid())
        {
            TextureRect *icon = memnew(TextureRect);
            icon->set_texture(p_group->get_icon());
            icon->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
            icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
            icon->set_custom_minimum_size(Vector2(16, 16));
            title_row->add_child(icon);
        }
        Label *title = memnew(Label);
        title->set_text(p_group->get_group_name());
        title->set_modulate(Color(1, 1, 1, 0.75));
        title_row->add_child(title);
        r_root->add_child(title_row);
    }

    Container *box = nullptr;
    if (p_group->get_container_type() == GroupEntry::CONTAINER_TABS)
    {
        box = memnew(TabContainer); // pages : les groupes de child_groups
    }
    else if (p_group->get_container_type() == GroupEntry::CONTAINER_GRID)
    {
        GridContainer *grid = memnew(GridContainer);
        grid->set_columns(MAX(1, p_group->get_columns()));
        box = grid;
    }
    else
    {
        BoxContainer *boxed = memnew(BoxContainer);
        boxed->set_vertical(p_group->get_is_vertical());
        box = boxed;
    }
    box->set_h_size_flags(SIZE_EXPAND_FILL);

    if (p_group->get_container_type() == GroupEntry::CONTAINER_PANEL)
    {
        PanelContainer *panel = memnew(PanelContainer);
        panel->set_h_size_flags(SIZE_EXPAND_FILL);
        MarginContainer *margin = memnew(MarginContainer);
        const char *sides[] = {"margin_left", "margin_top", "margin_right", "margin_bottom"};
        for (const char *side : sides)
        {
            margin->add_theme_constant_override(side, 6);
        }
        margin->add_child(box);
        panel->add_child(margin);
        r_root->add_child(panel);
    }
    else
    {
        r_root->add_child(box);
    }

    // Style du groupe : sur sa racine (titre + conteneur) ; fond dans un PanelContainer qui l'entoure.
    Ref<InterfaceStyle> group_style = styles_enabled ? p_group->get_style() : Ref<InterfaceStyle>();
    Control *outer = r_root;
    if (group_style.is_valid())
    {
        if (group_style->get_background().is_valid())
        {
            PanelContainer *frame = memnew(PanelContainer);
            frame->set_h_size_flags(SIZE_EXPAND_FILL);
            frame->add_theme_stylebox_override("panel", group_style->get_background());
            frame->add_child(r_root);
            outer = frame;
        }
        group_style->apply_to(r_root);
        sync->register_styled(r_root, group_style);
    }
    if (parent_tabs)
    {
        outer->set_meta("group_name", p_group->get_group_name());
        parent_tabs->add_child(outer);
        int tab = outer->get_index();
        parent_tabs->set_tab_title(tab, p_group->get_group_name());
        parent_tabs->set_tab_icon(tab, p_group->get_icon());
    }
    else
    {
        content->add_child(outer);
    }
    group_nodes[p_group->get_group_name()] = Array::make((int64_t)outer->get_instance_id(), (int64_t)r_root->get_instance_id());
    return box;
}

void ProceduralEditor::set_undo_backend(const Ref<UndoBackend> &p_backend) { undo_backend = p_backend; }

Ref<UndoBackend> ProceduralEditor::get_undo_backend()
{
    if (undo_backend.is_null())
    {
        undo_backend = Ref<UndoBackend>(memnew(RuntimeUndoBackend));
    }
    return undo_backend;
}

void ProceduralEditor::set_factory(const Ref<WidgetFactory> &p_factory)
{
    factory = p_factory;
    rebuild();
}

Ref<WidgetFactory> ProceduralEditor::get_factory() const { return factory; }

void ProceduralEditor::set_scroll_enabled(bool p_enabled)
{
    scroll->set_vertical_scroll_mode(p_enabled ? ScrollContainer::SCROLL_MODE_AUTO : ScrollContainer::SCROLL_MODE_DISABLED);
    set_v_size_flags(p_enabled ? SIZE_EXPAND_FILL : SIZE_FILL);
}
bool ProceduralEditor::is_scroll_enabled() const { return scroll->get_vertical_scroll_mode() != ScrollContainer::SCROLL_MODE_DISABLED; }

void ProceduralEditor::set_sync_interval(double p_interval) { sync->set_interval(p_interval); }
double ProceduralEditor::get_sync_interval() const { return sync->get_interval(); }

void ProceduralEditor::_clear_widgets()
{
    sync->clear();
    TypedArray<Node> children = content->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        if (child)
        {
            content->remove_child(child);
            child->queue_free();
        }
    }
}

void ProceduralEditor::_show_status(const String &p_text)
{
    status->set_text(p_text);
    status->set_visible(!p_text.is_empty());
}

void ProceduralEditor::rebuild()
{
    _clear_widgets();
    top_widgets.clear();
    Object *target = get_target();
    sync->set_target(target);
    rich_tooltip->set_context(target);
    rich_tooltip->hide_tooltip();
    group_nodes.clear();
    behavior_signatures.clear();
    behavior_styled.clear();
    script_behavior_states.clear();
    _apply_global_style();
    if (!target || active_registry.is_null())
    {
        _show_status("Aucun objet");
        return;
    }
    _show_status("");

    PackedInt64Array ancestors;
    ancestors.push_back((int64_t)target->get_instance_id());

    // Groupes construits : ObjectID du GroupEntry -> [conteneur, racine].
    Dictionary built_groups;
    TypedArray<DataEntry> entries = active_registry->get_entries();
    for (int i = 0; i < entries.size(); i++)
    {
        Ref<DataEntry> entry = entries[i];
        if (entry.is_null() || !entry->get_enabled() || entry->get_is_missing())
        {
            continue;
        }

        Ref<ValueAccessor> accessor;
        accessor.instantiate();
        accessor->setup(target, entry->get_data_name());
        if (entry->get_member() == DataEntry::MEMBER_METHOD)
        {
            accessor->set_property_info(_find_method(target, entry->get_data_name()));
        }
        else if (entry->get_member() == DataEntry::MEMBER_SIGNAL)
        {
            accessor->set_property_info(_find_signal(target, entry->get_data_name()));
        }
        else if (entry->get_member() == DataEntry::MEMBER_PREVIEW)
        {
            accessor->set_property_info(PreviewWidget::make_info(entry->get_data_name()));
        }
        if (accessor->get_property_info().is_empty())
        {
            continue;
        }

        PropertyWidget *widget = build_widget(accessor, entry, 0, ancestors);
        if (!widget)
        {
            continue;
        }
        top_widgets[String(entry->get_data_name())] = (int64_t)widget->get_instance_id();

        // Un groupe s'affiche à la place de sa PREMIÈRE donnée et rassemble toutes ses données.
        Ref<GroupEntry> group = active_groups.is_valid() ? active_groups->group_of(entry->get_data_name()) : Ref<GroupEntry>();
        if (group.is_null())
        {
            content->add_child(widget);
            continue;
        }
        int64_t key = (int64_t)group->get_instance_id();
        if (!built_groups.has(key))
        {
            // Groupe d'un groupe en onglets : le groupe en onglets est construit d'abord (à cette place), puis
            // ce groupe devient l'une de ses pages.
            Container *parent = nullptr;
            Ref<GroupEntry> tabs = active_groups->tabs_of(group->get_group_name());
            if (tabs.is_valid() && tabs != group)
            {
                int64_t tabs_key = (int64_t)tabs->get_instance_id();
                if (!built_groups.has(tabs_key))
                {
                    VBoxContainer *tabs_root = nullptr;
                    Container *tabs_box = _make_group_container(tabs, tabs_root);
                    built_groups[tabs_key] = Array::make((int64_t)tabs_box->get_instance_id(), (int64_t)tabs_root->get_instance_id(), tabs);
                }
                parent = Object::cast_to<Container>(ObjectDB::get_instance(uint64_t(int64_t(Array(built_groups[tabs_key])[0]))));
            }
            VBoxContainer *root = nullptr;
            Container *box = _make_group_container(group, root, parent);
            Array slot;
            slot.push_back((int64_t)box->get_instance_id());
            slot.push_back((int64_t)root->get_instance_id());
            slot.push_back(group);
            built_groups[key] = slot;
        }
        Array slot = built_groups[key];
        Container *box = Object::cast_to<Container>(ObjectDB::get_instance(uint64_t(int64_t(slot[0]))));
        box->add_child(widget);
    }

    // Ordre des données dans chaque groupe (data_names) + signalement des noms qui ne désignent aucune donnée affichable.
    Array keys = built_groups.keys();
    for (int g = 0; g < keys.size(); g++)
    {
        Array slot = built_groups[keys[g]];
        Container *box = Object::cast_to<Container>(ObjectDB::get_instance(uint64_t(int64_t(slot[0]))));
        VBoxContainer *root = Object::cast_to<VBoxContainer>(ObjectDB::get_instance(uint64_t(int64_t(slot[1]))));
        Ref<GroupEntry> group = slot[2];
        // Groupe en onglets : pages dans l'ordre de child_groups (titres et icônes remis après déplacement).
        if (TabContainer *tabs = Object::cast_to<TabContainer>(box))
        {
            PackedStringArray pages = group->get_child_groups();
            int position = 0;
            for (const String &page_name : pages)
            {
                if (group_nodes.has(page_name))
                {
                    Node *page = Object::cast_to<Node>(ObjectDB::get_instance(uint64_t(int64_t(Array(group_nodes[page_name])[0]))));
                    if (page && page->get_parent() == tabs)
                    {
                        tabs->move_child(page, position++);
                    }
                }
            }
            for (int t = 0; t < tabs->get_tab_count(); t++)
            {
                Node *page = tabs->get_tab_control(t);
                String page_name = page ? String(page->get_meta("group_name", String())) : String();
                Ref<GroupEntry> page_group = active_groups->find(page_name);
                tabs->set_tab_title(t, page_name);
                tabs->set_tab_icon(t, page_group.is_valid() ? page_group->get_icon() : Ref<Texture2D>());
            }
            continue;
        }
        PackedStringArray names = group->get_data_names();
        int position = 0;
        for (int n = 0; n < names.size(); n++)
        {
            if (top_widgets.has(names[n]))
            {
                Node *widget = Object::cast_to<Node>(ObjectDB::get_instance(uint64_t(int64_t(top_widgets[names[n]]))));
                if (widget && widget->get_parent() == box)
                {
                    box->move_child(widget, position++);
                }
                continue;
            }
            Ref<DataEntry> named = active_registry->find(names[n]);
            if (named.is_null())
            {
                named = active_registry->find(names[n], DataEntry::MEMBER_METHOD);
            }
            if (named.is_null())
            {
                named = active_registry->find(names[n], DataEntry::MEMBER_PREVIEW);
            }
            if (named.is_null() || named->get_is_missing())
            {
                Label *warning = memnew(Label);
                warning->set_text(U"⚠ « " + names[n] + U" » : donnée manquante");
                warning->set_modulate(Color(1, 0.8, 0.4));
                root->add_child(warning);
            }
        }
    }

    _evaluate_behaviors();
}

PropertyWidget *ProceduralEditor::get_widget(const StringName &p_property) const
{
    String name = p_property;
    return top_widgets.has(name) ? Object::cast_to<PropertyWidget>(ObjectDB::get_instance(uint64_t(int64_t(top_widgets[name])))) : nullptr;
}

void ProceduralEditor::_on_value_changed(const Variant &p_value, bool p_committing, uint64_t p_widget_id)
{
    PropertyWidget *widget = Object::cast_to<PropertyWidget>(ObjectDB::get_instance(p_widget_id));
    if (!widget)
    {
        return;
    }
    get_undo_backend()->edit_property(widget->get_accessor(), p_value, p_committing);
    sync->notify_written(widget, p_value, p_committing);
}

void ProceduralEditor::_on_edit_discarded(uint64_t p_widget_id)
{
    if (PropertyWidget *widget = Object::cast_to<PropertyWidget>(ObjectDB::get_instance(p_widget_id)))
    {
        get_undo_backend()->discard_edit(widget->get_accessor());
    }
}

void ProceduralEditor::_on_target_lost()
{
    _clear_widgets();
    _show_status(U"Objet libéré");
}

bool ProceduralEditor::undo()
{
    Ref<UndoBackend> backend = get_undo_backend();
    bool done = backend->undo();
    if (done)
    {
        reapply_links();
        sync->refresh_all(false); // affichage immédiat, sans attendre le prochain intervalle
    }
    return done;
}

bool ProceduralEditor::redo()
{
    Ref<UndoBackend> backend = get_undo_backend();
    bool done = backend->redo();
    if (done)
    {
        reapply_links();
        sync->refresh_all(false);
    }
    return done;
}
