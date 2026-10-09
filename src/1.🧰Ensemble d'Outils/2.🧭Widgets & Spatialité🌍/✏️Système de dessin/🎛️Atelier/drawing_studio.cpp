#include "drawing_studio.hpp"

#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_flow_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/classes/v_split_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <initializer_list>

using namespace godot;

static const char32_t *const TOOL_NAMES[DrawingCanvas::TOOL_MAX] = {
    U"Pinceau (B)", U"Gomme (E)", U"Remplir (G)", U"Pipette (I)", U"Rectangle (M)", U"Ellipse (M)", U"Lasso (L)", U"Baguette (W)", U"Main (H)",
};

static const char32_t *const TOOL_TOOLTIPS[DrawingCanvas::TOOL_MAX] = {
    U"Pinceau : Alt + clic pour prélever une couleur",
    U"Gomme (aussi : bout gomme du stylet)",
    U"Pot de peinture : remplit la zone de couleur semblable",
    U"Pipette : prélève la couleur affichée",
    U"Sélection rectangulaire (Maj : ajouter, Alt : soustraire, clic : désélectionner)",
    U"Sélection elliptique (Maj : ajouter, Alt : soustraire, clic : désélectionner)",
    U"Lasso : sélection à main levée",
    U"Baguette magique : sélectionne la zone de couleur semblable",
    U"Main : déplacer la vue (aussi : Espace ou clic du milieu)",
};

static const char32_t *const BLEND_NAMES[DrawingLayer::LAYER_BLEND_MAX] = {
    U"Normal", U"Produit",      U"Écran",      U"Incrustation",    U"Obscurcir",    U"Éclaircir", U"Densité couleur -", U"Densité couleur +",
    U"Ajout",  U"Soustraction", U"Différence", U"Lumière tamisée", U"Lumière crue",
};

static PackedStringArray to_strings(const char32_t *const *p_items, int p_count)
{
    PackedStringArray strings;
    for (int i = 0; i < p_count; i++)
    {
        strings.push_back(String(p_items[i]));
    }
    return strings;
}

static PackedStringArray to_strings(std::initializer_list<const char32_t *> p_items)
{
    return to_strings(p_items.begin(), (int)p_items.size());
}

DrawingStudio::DrawingStudio() { _build(); }

void DrawingStudio::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_READY:
        _ensure_brushes();
        if (canvas->get_document().is_null())
        {
            new_document(Vector2i(1280, 720));
        }
        else
        {
            _connect_document();
        }
        _refresh_colors();
        set_process_internal(true);
        break;
    case NOTIFICATION_INTERNAL_PROCESS:
        if (tool_options_dirty)
        {
            _rebuild_tool_options();
        }
        if (layers_dirty)
        {
            _rebuild_layers();
        }
        if (size_slider)
        {
            const Ref<DrawingBrush> brush = _edited_brush();
            if (brush.is_valid() && !Math::is_equal_approx(size_slider->get_value(), double(brush->get_size())))
            {
                size_slider->set_value_no_signal(brush->get_size());
            }
        }
        break;
    case NOTIFICATION_DRAW:
        draw_style_box(get_theme_stylebox("panel", "PanelContainer"), Rect2(Vector2(), get_size()));
        break;
    default:
        break;
    }
}

Vector2 DrawingStudio::_get_minimum_size() const { return root ? root->get_combined_minimum_size() : Vector2(); }

void DrawingStudio::_gui_input(const Ref<InputEvent> &p_event)
{
    // Raccourcis de fichier (les touches non utilisées par la toile remontent jusqu'ici).
    Ref<InputEventKey> key = p_event;
    if (key.is_null() || !key->is_pressed() || key->is_echo() || !key->is_command_or_control_pressed() || !show_file_buttons)
    {
        return;
    }
    switch (key->get_keycode())
    {
    case KEY_N:
        _on_new_pressed();
        break;
    case KEY_O:
        _on_open_pressed();
        break;
    case KEY_S:
        key->is_shift_pressed() ? _on_save_as_pressed() : (void)_on_save_pressed();
        break;
    default:
        return;
    }
    accept_event();
}

// 🧱 Construction

void DrawingStudio::_build()
{
    canvas = memnew(DrawingCanvas);
    canvas->set_name("Canvas");
    canvas->set_h_size_flags(SIZE_EXPAND_FILL);
    canvas->set_v_size_flags(SIZE_EXPAND_FILL);
    canvas->set_custom_minimum_size(Vector2(120, 120));

    root = memnew(VBoxContainer);
    root->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
    root->add_theme_constant_override("separation", 4);
    add_child(root, false, INTERNAL_MODE_FRONT);

    _build_top_bar();

    HBoxContainer *body = memnew(HBoxContainer);
    body->set_v_size_flags(SIZE_EXPAND_FILL);
    root->add_child(body);
    _build_left_panel(body);
    body->add_child(canvas);
    _build_right_panel(body);

    status_label = memnew(Label);
    status_label->set_text(U"Prêt");
    status_label->set_modulate(Color(1, 1, 1, 0.75f));
    status_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
    root->add_child(status_label);

    _build_dialogs();

    canvas->connect("tool_changed", callable_mp(this, &DrawingStudio::_on_tool_changed));
    canvas->connect("document_changed", callable_mp(this, &DrawingStudio::_on_document_changed));
    canvas->connect("view_changed", callable_mp(this, &DrawingStudio::_on_view_changed));
    canvas->connect("primary_color_changed", callable_mp(this, &DrawingStudio::_on_primary_color_changed));
    canvas->connect("secondary_color_changed", callable_mp(this, &DrawingStudio::_on_primary_color_changed));
    canvas->connect("stroke_rejected", callable_mp(this, &DrawingStudio::_on_stroke_rejected));
}

Button *DrawingStudio::_add_button(Container *p_parent, const String &p_text, const String &p_tooltip, const Callable &p_callback)
{
    Button *button = memnew(Button);
    button->set_text(p_text);
    button->set_tooltip_text(p_tooltip);
    button->set_focus_mode(FOCUS_NONE); // la toile garde le focus (raccourcis clavier)
    button->connect("pressed", p_callback);
    p_parent->add_child(button);
    return button;
}

HSlider *DrawingStudio::_add_slider(
    Container *p_parent, const String &p_label, double p_min, double p_max, double p_step, double p_value, const Callable &p_callback, bool p_exponential, const String &p_suffix)
{
    HBoxContainer *row = memnew(HBoxContainer);
    p_parent->add_child(row);
    Label *label = memnew(Label);
    label->set_text(p_label);
    label->set_custom_minimum_size(Vector2(80, 0));
    label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
    row->add_child(label);
    HSlider *slider = memnew(HSlider);
    slider->set_min(p_min);
    slider->set_max(p_max);
    slider->set_step(p_step);
    slider->set_exp_ratio(p_exponential);
    slider->set_h_size_flags(SIZE_EXPAND_FILL);
    slider->set_v_size_flags(SIZE_SHRINK_CENTER);
    slider->set_focus_mode(FOCUS_NONE);
    slider->set_value_no_signal(p_value);
    row->add_child(slider);
    SpinBox *spin = memnew(SpinBox);
    spin->set_min(p_min);
    spin->set_max(p_max);
    spin->set_step(p_step);
    spin->set_exp_ratio(p_exponential);
    spin->set_suffix(p_suffix);
    spin->set_custom_minimum_size(Vector2(78, 0));
    spin->set_select_all_on_focus(true);
    slider->share(spin); // même valeur : la boîte adopte celle du curseur
    row->add_child(spin);
    slider->connect("value_changed", p_callback);
    return slider;
}

CheckBox *DrawingStudio::_add_check(Container *p_parent, const String &p_text, bool p_pressed, const Callable &p_callback)
{
    CheckBox *check = memnew(CheckBox);
    check->set_text(p_text);
    check->set_pressed_no_signal(p_pressed);
    check->set_focus_mode(FOCUS_NONE);
    check->connect("toggled", p_callback);
    p_parent->add_child(check);
    return check;
}

OptionButton *DrawingStudio::_add_options(Container *p_parent, const String &p_label, const PackedStringArray &p_items, int p_selected, const Callable &p_callback)
{
    HBoxContainer *row = memnew(HBoxContainer);
    p_parent->add_child(row);
    if (!p_label.is_empty())
    {
        Label *label = memnew(Label);
        label->set_text(p_label);
        label->set_custom_minimum_size(Vector2(92, 0));
        row->add_child(label);
    }
    OptionButton *options = memnew(OptionButton);
    for (const String &item : p_items)
    {
        options->add_item(item);
    }
    options->select(p_selected);
    options->set_h_size_flags(SIZE_EXPAND_FILL);
    options->set_focus_mode(FOCUS_NONE);
    options->set_fit_to_longest_item(false);
    options->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
    options->connect("item_selected", p_callback);
    row->add_child(options);
    return options;
}

static void add_section(Container *p_parent, const String &p_text)
{
    p_parent->add_child(memnew(HSeparator));
    Label *label = memnew(Label);
    label->set_text(p_text);
    label->set_modulate(Color(1, 1, 1, 0.8f));
    p_parent->add_child(label);
}

/// Panneau latéral de largeur fixe. p_scroll : défile en entier (sinon, c'est au contenu de prévoir un défilement).
static VBoxContainer *add_panel(HBoxContainer *p_parent, float p_width, bool p_scroll)
{
    MarginContainer *margins = memnew(MarginContainer);
    margins->set_custom_minimum_size(Vector2(p_width, 0));
    for (const char *side : {"margin_left", "margin_right", "margin_top", "margin_bottom"})
    {
        margins->add_theme_constant_override(side, 4);
    }
    p_parent->add_child(margins);
    VBoxContainer *panel = memnew(VBoxContainer);
    panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    if (!p_scroll)
    {
        margins->add_child(panel);
        return panel;
    }
    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    margins->add_child(scroll);
    scroll->add_child(panel);
    return panel;
}

void DrawingStudio::_build_top_bar()
{
    HFlowContainer *bar = memnew(HFlowContainer); // passe à la ligne si l'atelier est étroit
    root->add_child(bar);

    file_buttons = memnew(HBoxContainer);
    bar->add_child(file_buttons);
    _add_button(file_buttons, U"Nouveau", U"Nouveau dessin (Ctrl+N)", callable_mp(this, &DrawingStudio::_on_new_pressed));
    _add_button(file_buttons, U"Ouvrir", U"Ouvrir une image ou un document de dessin (Ctrl+O)", callable_mp(this, &DrawingStudio::_on_open_pressed));
    _add_button(file_buttons, U"Enregistrer", U"Enregistrer (Ctrl+S)", callable_mp(this, &DrawingStudio::_on_save_pressed));
    _add_button(file_buttons, U"Enregistrer sous", U"Enregistrer sous : image (.png, .jpg, .webp, calques fusionnés) ou document avec ses calques (.tres, .res) (Ctrl+Maj+S)",
                callable_mp(this, &DrawingStudio::_on_save_as_pressed));
    file_buttons->add_child(memnew(VSeparator));

    undo_button = _add_button(bar, U"Annuler", U"Annuler (Ctrl+Z)", callable_mp(this, &DrawingStudio::_on_undo));
    redo_button = _add_button(bar, U"Rétablir", U"Rétablir (Ctrl+Y)", callable_mp(this, &DrawingStudio::_on_redo));
    bar->add_child(memnew(VSeparator));

    _add_button(bar, U"Ajuster", U"Afficher tout le dessin (Ctrl+0)", callable_mp(canvas, &DrawingCanvas::fit_to_view));
    _add_button(bar, U"100 %", U"Taille réelle (Ctrl+1)", callable_mp(canvas, &DrawingCanvas::actual_size));
    _add_button(bar, U"⟲ 15°", U"Tourner la vue (-)", callable_mp(this, &DrawingStudio::_on_rotate).bind(-15.0f));
    _add_button(bar, U"⟳ 15°", U"Tourner la vue (=)", callable_mp(this, &DrawingStudio::_on_rotate).bind(15.0f));
    _add_button(bar, U"0°", U"Annuler la rotation et le miroir de la vue", callable_mp(canvas, &DrawingCanvas::reset_rotation));
    flip_button = memnew(Button);
    flip_button->set_text(U"Miroir");
    flip_button->set_tooltip_text(U"Retourner la vue horizontalement (le dessin n'est pas modifié)");
    flip_button->set_toggle_mode(true);
    flip_button->set_focus_mode(FOCUS_NONE);
    flip_button->connect("toggled", callable_mp(this, &DrawingStudio::_on_flip));
    bar->add_child(flip_button);

    zoom_label = memnew(Label);
    zoom_label->set_h_size_flags(SIZE_EXPAND_FILL);
    zoom_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
    zoom_label->set_v_size_flags(SIZE_SHRINK_CENTER);
    bar->add_child(zoom_label);
}

void DrawingStudio::_build_left_panel(HBoxContainer *p_parent)
{
    VBoxContainer *panel = add_panel(p_parent, 220, false);

    GridContainer *grid = memnew(GridContainer);
    grid->set_columns(2);
    panel->add_child(grid);
    tool_group.instantiate();
    for (int i = 0; i < DrawingCanvas::TOOL_MAX; i++)
    {
        Button *button = memnew(Button);
        button->set_text(TOOL_NAMES[i]);
        button->set_tooltip_text(TOOL_TOOLTIPS[i]);
        button->set_toggle_mode(true);
        button->set_button_group(tool_group);
        button->set_focus_mode(FOCUS_NONE);
        button->set_h_size_flags(SIZE_EXPAND_FILL);
        button->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
        button->connect("pressed", callable_mp(this, &DrawingStudio::_on_tool_pressed).bind(i));
        grid->add_child(button);
        tool_buttons[i] = button;
    }
    tool_buttons[canvas->get_tool()]->set_pressed_no_signal(true);

    panel->add_child(memnew(HSeparator));
    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    panel->add_child(scroll);
    tool_options = memnew(VBoxContainer);
    tool_options->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->add_child(tool_options);
}

void DrawingStudio::_build_right_panel(HBoxContainer *p_parent)
{
    VBoxContainer *panel = add_panel(p_parent, 250, false);
    // Couleurs en haut, calques en bas, séparés par une poignée réglable ; chaque partie défile de son côté.
    VSplitContainer *split = memnew(VSplitContainer);
    split->set_v_size_flags(SIZE_EXPAND_FILL);
    split->set_split_offset(-120);
    panel->add_child(split);
    ScrollContainer *colors_scroll = memnew(ScrollContainer);
    colors_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    colors_scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    colors_scroll->set_custom_minimum_size(Vector2(0, 80));
    split->add_child(colors_scroll);
    VBoxContainer *colors = memnew(VBoxContainer);
    colors->set_h_size_flags(SIZE_EXPAND_FILL);
    colors_scroll->add_child(colors);
    VBoxContainer *layers = memnew(VBoxContainer);
    layers->set_v_size_flags(SIZE_EXPAND_FILL);
    layers->set_custom_minimum_size(Vector2(0, 220));
    split->add_child(layers);

    // 🎨 Couleurs
    HBoxContainer *swatches = memnew(HBoxContainer);
    colors->add_child(swatches);
    primary_swatch = memnew(ColorRect);
    primary_swatch->set_custom_minimum_size(Vector2(30, 30));
    primary_swatch->set_tooltip_text(U"Couleur principale");
    swatches->add_child(primary_swatch);
    secondary_swatch = memnew(ColorRect);
    secondary_swatch->set_custom_minimum_size(Vector2(22, 22));
    secondary_swatch->set_v_size_flags(SIZE_SHRINK_END);
    secondary_swatch->set_tooltip_text(U"Couleur secondaire");
    swatches->add_child(secondary_swatch);
    _add_button(swatches, U"Échanger (X)", U"Échanger les couleurs principale et secondaire", callable_mp(canvas, &DrawingCanvas::swap_colors));

    color_picker = memnew(ColorPicker);
    color_picker->set_edit_alpha(true);
    color_picker->set_modes_visible(false);
    color_picker->set_sampler_visible(false);
    color_picker->set_presets_visible(true);
    color_picker->set_picker_shape(ColorPicker::SHAPE_OKHSL_CIRCLE); // plus compact que le rectangle
    color_picker->connect("color_changed", callable_mp(this, &DrawingStudio::_on_picker_color_changed));
    colors->add_child(color_picker);

    // 🗂️ Calques
    panel = layers;
    Label *title = memnew(Label);
    title->set_text(U"Calques");
    title->set_modulate(Color(1, 1, 1, 0.8f));
    panel->add_child(title);
    PackedStringArray blend_names = to_strings(BLEND_NAMES, DrawingLayer::LAYER_BLEND_MAX);
    layer_blend = _add_options(panel, U"Fusion", blend_names, 0, callable_mp(this, &DrawingStudio::_on_layer_blend));
    layer_opacity = _add_slider(panel, U"Opacité", 0, 100, 1, 100, callable_mp(this, &DrawingStudio::_on_layer_opacity), false, U" %");
    HBoxContainer *flags = memnew(HBoxContainer);
    panel->add_child(flags);
    layer_alpha_lock = _add_check(flags, U"Transp.", false, callable_mp(this, &DrawingStudio::_on_layer_flag).bind(StringName("alpha_locked")));
    layer_alpha_lock->set_tooltip_text(U"Verrou de transparence : on ne peint que sur les pixels déjà peints");
    layer_clip = _add_check(flags, U"Écrêter", false, callable_mp(this, &DrawingStudio::_on_layer_flag).bind(StringName("clipped")));
    layer_clip->set_tooltip_text(U"Écrêter au calque inférieur : visible seulement là où le calque du dessous est peint");
    layer_lock = _add_check(flags, U"Verrou", false, callable_mp(this, &DrawingStudio::_on_layer_flag).bind(StringName("locked")));
    layer_lock->set_tooltip_text(U"Verrouiller le calque (aucune modification)");
    layer_name = memnew(LineEdit);
    layer_name->set_placeholder(U"Nom du calque");
    layer_name->connect("text_submitted", callable_mp(this, &DrawingStudio::_on_layer_name_submitted));
    panel->add_child(layer_name);

    ScrollContainer *list_scroll = memnew(ScrollContainer);
    list_scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    list_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    panel->add_child(list_scroll);
    layer_list = memnew(VBoxContainer);
    layer_list->set_h_size_flags(SIZE_EXPAND_FILL);
    list_scroll->add_child(layer_list);

    GridContainer *actions = memnew(GridContainer);
    actions->set_columns(3);
    panel->add_child(actions);
    _add_button(actions, U"Nouveau", U"Nouveau calque au-dessus du calque courant", callable_mp(this, &DrawingStudio::_on_add_layer));
    _add_button(actions, U"Dupliquer", U"Dupliquer le calque courant", callable_mp(this, &DrawingStudio::_on_duplicate_layer));
    layer_remove = _add_button(actions, U"Supprimer", U"Supprimer le calque courant", callable_mp(this, &DrawingStudio::_on_remove_layer));
    _add_button(actions, U"Monter", U"Monter le calque", callable_mp(this, &DrawingStudio::_on_move_layer).bind(1));
    _add_button(actions, U"Descendre", U"Descendre le calque", callable_mp(this, &DrawingStudio::_on_move_layer).bind(-1));
    layer_merge = _add_button(actions, U"Fusionner", U"Fusionner avec le calque inférieur", callable_mp(this, &DrawingStudio::_on_merge_down));
    _add_button(actions, U"Effacer", U"Effacer tout le calque", callable_mp(this, &DrawingStudio::_on_clear_layer));
    for (int i = 0; i < actions->get_child_count(); i++)
    {
        Object::cast_to<Control>(actions->get_child(i))->set_h_size_flags(SIZE_EXPAND_FILL);
    }
}

void DrawingStudio::_build_dialogs()
{
    open_dialog = memnew(FileDialog);
    open_dialog->set_title(U"Ouvrir");
    open_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    open_dialog->add_filter("*.png, *.jpg, *.jpeg, *.webp, *.tres, *.res", U"Images et documents de dessin");
    open_dialog->add_filter("*.png, *.jpg, *.jpeg, *.webp", U"Images");
    open_dialog->add_filter("*.tres, *.res", U"Documents de dessin");
    open_dialog->connect("file_selected", callable_mp(this, &DrawingStudio::_on_open_selected));
    add_child(open_dialog, false, INTERNAL_MODE_BACK);

    save_dialog = memnew(FileDialog);
    save_dialog->set_title(U"Enregistrer sous");
    save_dialog->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
    save_dialog->add_filter("*.png", U"Image PNG (calques fusionnés)");
    save_dialog->add_filter("*.tres", U"Document de dessin, texte (calques conservés)");
    save_dialog->add_filter("*.res", U"Document de dessin, binaire (calques conservés)");
    save_dialog->add_filter("*.webp", U"Image WebP sans perte");
    save_dialog->add_filter("*.jpg, *.jpeg", U"Image JPEG");
    save_dialog->connect("file_selected", callable_mp(this, &DrawingStudio::_on_save_selected));
    add_child(save_dialog, false, INTERNAL_MODE_BACK);

    new_dialog = memnew(ConfirmationDialog);
    new_dialog->set_title(U"Nouveau dessin");
    VBoxContainer *content = memnew(VBoxContainer);
    new_dialog->add_child(content);
    for (SpinBox **spin : {&new_width, &new_height})
    {
        HBoxContainer *row = memnew(HBoxContainer);
        content->add_child(row);
        Label *label = memnew(Label);
        label->set_text(spin == &new_width ? U"Largeur" : U"Hauteur");
        label->set_custom_minimum_size(Vector2(80, 0));
        row->add_child(label);
        *spin = memnew(SpinBox);
        (*spin)->set_min(1);
        (*spin)->set_max(8192);
        (*spin)->set_value(spin == &new_width ? 1280 : 720);
        (*spin)->set_suffix("px");
        (*spin)->set_h_size_flags(SIZE_EXPAND_FILL);
        row->add_child(*spin);
    }
    new_transparent = memnew(CheckBox);
    new_transparent->set_text(U"Fond transparent");
    content->add_child(new_transparent);
    new_dialog->connect("confirmed", callable_mp(this, &DrawingStudio::_on_new_confirmed));
    add_child(new_dialog, false, INTERNAL_MODE_BACK);
    set_file_access(file_access);
}

// 📄 Document

void DrawingStudio::set_document(const Ref<DrawingDocument> &p_document) { edit_document(p_document, String()); }

void DrawingStudio::_connect_document()
{
    const Ref<DrawingDocument> document = canvas->get_document();
    if (document.is_null())
    {
        return;
    }
    const Callable layers = callable_mp(this, &DrawingStudio::_on_layers_changed);
    if (!document->is_connected("layers_changed", layers))
    {
        document->connect("layers_changed", layers);
        document->connect("current_layer_changed", callable_mp(this, &DrawingStudio::_on_layers_changed).unbind(1));
        document->connect("history_changed", callable_mp(this, &DrawingStudio::_on_history_changed));
    }
    layers_dirty = true;
    _refresh_history();
    _refresh_zoom();
}

void DrawingStudio::_disconnect_document()
{
    const Ref<DrawingDocument> document = canvas->get_document();
    if (document.is_null())
    {
        return;
    }
    const Callable layers = callable_mp(this, &DrawingStudio::_on_layers_changed);
    if (document->is_connected("layers_changed", layers))
    {
        document->disconnect("layers_changed", layers);
        document->disconnect("current_layer_changed", callable_mp(this, &DrawingStudio::_on_layers_changed).unbind(1));
        document->disconnect("history_changed", callable_mp(this, &DrawingStudio::_on_history_changed));
    }
}

void DrawingStudio::_on_document_changed() { _connect_document(); }

void DrawingStudio::new_document(const Vector2i &p_size, const Color &p_background)
{
    edit_document(DrawingDocument::create(p_size, p_background), String());
    show_message(String(U"Nouveau dessin ") + String::num_int64(p_size.x) + U" × " + String::num_int64(p_size.y));
}

void DrawingStudio::edit_document(const Ref<DrawingDocument> &p_document, const String &p_path)
{
    _disconnect_document();
    current_path = p_path;
    canvas->set_document(p_document);
    _connect_document();
}

Error DrawingStudio::open_file(const String &p_path)
{
    const String extension = p_path.get_extension().to_lower();
    Ref<DrawingDocument> document;
    if (extension == "tres" || extension == "res")
    {
        document = ResourceLoader::get_singleton()->load(p_path);
        if (document.is_null())
        {
            show_message(String(U"Pas un document de dessin : ") + p_path);
            return ERR_FILE_UNRECOGNIZED;
        }
    }
    else
    {
        Ref<Image> image = Image::load_from_file(p_path);
        if (image.is_null() || image->is_empty())
        {
            show_message(String(U"Impossible d'ouvrir : ") + p_path);
            return ERR_FILE_CANT_OPEN;
        }
        document = DrawingDocument::create_from_image(image);
    }
    edit_document(document, p_path);
    show_message(String(U"Ouvert : ") + p_path);
    emit_signal("document_opened", p_path);
    return OK;
}

Error DrawingStudio::save()
{
    if (current_path.is_empty())
    {
        _on_save_as_pressed();
        return ERR_UNAVAILABLE;
    }
    return save_as(current_path);
}

Error DrawingStudio::save_as(const String &p_path)
{
    const Ref<DrawingDocument> document = canvas->get_document();
    ERR_FAIL_COND_V(document.is_null(), ERR_UNCONFIGURED);
    if (document->is_stroking())
    {
        document->end_stroke();
    }
    const String extension = p_path.get_extension().to_lower();
    Error error = OK;
    bool flattened = true;
    if (extension == "tres" || extension == "res")
    {
        error = ResourceSaver::get_singleton()->save(document, p_path, ResourceSaver::FLAG_CHANGE_PATH);
        flattened = false;
    }
    else if (extension == "jpg" || extension == "jpeg")
    {
        error = document->get_composite_image(true)->save_jpg(p_path, 0.92f);
    }
    else if (extension == "webp")
    {
        error = document->get_composite_image(true)->save_webp(p_path, false);
    }
    else if (extension == "png")
    {
        error = document->export_png(p_path, true);
    }
    else
    {
        show_message(String(U"Extension non prise en charge : ") + extension);
        return ERR_FILE_UNRECOGNIZED;
    }
    if (error != OK)
    {
        show_message(String(U"Échec de l'enregistrement (erreur ") + String::num_int64(error) + U") : " + p_path);
        return error;
    }
    current_path = p_path;
    show_message(String(flattened && document->get_layer_count() > 1 ? U"Enregistré (calques fusionnés) : " : U"Enregistré : ") + p_path);
    emit_signal("document_saved", p_path);
    return OK;
}

void DrawingStudio::show_message(const String &p_message)
{
    if (status_label)
    {
        status_label->set_text(p_message);
    }
}

// ⚙️ Réglages

void DrawingStudio::set_brushes(const TypedArray<DrawingBrush> &p_brushes)
{
    brushes = p_brushes;
    _ensure_brushes();
}

void DrawingStudio::set_erasers(const TypedArray<DrawingBrush> &p_erasers)
{
    erasers = p_erasers;
    _ensure_brushes();
}

void DrawingStudio::set_show_file_buttons(bool p_show)
{
    show_file_buttons = p_show;
    file_buttons->set_visible(p_show);
}

void DrawingStudio::set_file_access(int p_access)
{
    file_access = CLAMP(p_access, 0, 2);
    open_dialog->set_access(FileDialog::Access(file_access));
    save_dialog->set_access(FileDialog::Access(file_access));
}

TypedArray<DrawingBrush> DrawingStudio::_brush_list(bool p_eraser) const
{
    if (p_eraser)
    {
        return erasers.is_empty() ? builtin_erasers : erasers;
    }
    return brushes.is_empty() ? builtin_brushes : brushes;
}

Ref<DrawingBrush> DrawingStudio::_edited_brush() const { return canvas->get_active_brush(); }

void DrawingStudio::_ensure_brushes()
{
    if (builtin_brushes.is_empty())
    {
        for (DrawingBrush::Preset preset :
             {DrawingBrush::PRESET_PEN, DrawingBrush::PRESET_PENCIL, DrawingBrush::PRESET_MARKER, DrawingBrush::PRESET_AIRBRUSH, DrawingBrush::PRESET_WATERCOLOR, DrawingBrush::PRESET_INK_VELOCITY})
        {
            builtin_brushes.push_back(DrawingBrush::create_preset(preset));
        }
        builtin_erasers.push_back(DrawingBrush::create_preset(DrawingBrush::PRESET_HARD_ERASER));
        builtin_erasers.push_back(DrawingBrush::create_preset(DrawingBrush::PRESET_SOFT_ERASER));
        grain_textures.push_back(Array::make(String(U"Aucun"), Variant()));
        grain_textures.push_back(Array::make(String(U"Papier fin"), DrawingBrush::create_grain_texture(128, 7, 1.6f)));
        grain_textures.push_back(Array::make(String(U"Papier rugueux"), DrawingBrush::create_grain_texture(64, 21, 2.6f)));
        grain_textures.push_back(Array::make(String(U"Toile"), DrawingBrush::create_grain_texture(256, 3, 1.2f)));
    }
    const TypedArray<DrawingBrush> brush_list = _brush_list(false);
    const TypedArray<DrawingBrush> eraser_list = _brush_list(true);
    if ((canvas->get_brush().is_null() || !brush_list.has(canvas->get_brush())) && !brush_list.is_empty())
    {
        canvas->set_brush(brush_list[0]);
    }
    if ((canvas->get_eraser().is_null() || !eraser_list.has(canvas->get_eraser())) && !eraser_list.is_empty())
    {
        canvas->set_eraser(eraser_list[0]);
    }
    tool_options_dirty = true;
}

// 🛠️ Réglages de l'outil

void DrawingStudio::_rebuild_tool_options()
{
    tool_options_dirty = false;
    size_slider = nullptr;
    for (int i = tool_options->get_child_count() - 1; i >= 0; i--)
    {
        Node *child = tool_options->get_child(i);
        tool_options->remove_child(child);
        child->queue_free();
    }
    const DrawingCanvas::Tool tool = canvas->get_tool();
    switch (tool)
    {
    case DrawingCanvas::TOOL_BRUSH:
    case DrawingCanvas::TOOL_ERASER:
        _build_brush_options(tool == DrawingCanvas::TOOL_ERASER);
        break;
    case DrawingCanvas::TOOL_FILL:
    {
        _add_slider(tool_options, U"Tolérance", 0, 100, 1, canvas->get_fill_tolerance() * 100.0, callable_mp(this, &DrawingStudio::_on_canvas_property).bind(StringName("fill_tolerance"), 100.0),
                    false, U" %");
        HSlider *gap = _add_slider(tool_options, U"Fermer trous", 0, 64, 1, canvas->get_fill_gap_closing(),
                                   callable_mp(this, &DrawingStudio::_on_canvas_property).bind(StringName("fill_gap_closing"), 1.0), false, U" px");
        gap->set_tooltip_text(U"Ne fuit pas par les trous des traits plus étroits que cette valeur");
        HSlider *expand =
            _add_slider(tool_options, U"Étendre", -16, 16, 1, canvas->get_fill_expand(), callable_mp(this, &DrawingStudio::_on_canvas_property).bind(StringName("fill_expand"), 1.0), false, U" px");
        expand->set_tooltip_text(U"Agrandit la zone remplie (glisse sous les traits)");
        _add_options(tool_options, U"Référence", to_strings({U"Calque courant", U"Tous les calques"}), canvas->get_fill_reference(),
                     callable_mp(this, &DrawingStudio::_on_canvas_option).bind(StringName("fill_reference")));
        _add_check(tool_options, U"Zone contiguë seulement", canvas->get_fill_contiguous(), callable_mp(this, &DrawingStudio::_on_canvas_bool).bind(StringName("fill_contiguous")));
        break;
    }
    case DrawingCanvas::TOOL_EYEDROPPER:
        _add_check(tool_options, U"Couleur affichée (tous les calques)", canvas->get_eyedropper_all_layers(),
                   callable_mp(this, &DrawingStudio::_on_canvas_bool).bind(StringName("eyedropper_all_layers")));
        break;
    case DrawingCanvas::TOOL_SELECT_RECT:
    case DrawingCanvas::TOOL_SELECT_ELLIPSE:
    case DrawingCanvas::TOOL_SELECT_LASSO:
    case DrawingCanvas::TOOL_SELECT_WAND:
    {
        _add_options(tool_options, U"Mode", to_strings({U"Remplacer", U"Ajouter (Maj)", U"Soustraire (Alt)", U"Intersection"}), canvas->get_selection_mode(),
                     callable_mp(this, &DrawingStudio::_on_canvas_option).bind(StringName("selection_mode")));
        if (tool == DrawingCanvas::TOOL_SELECT_WAND)
        {
            _add_slider(tool_options, U"Tolérance", 0, 100, 1, canvas->get_wand_tolerance() * 100.0, callable_mp(this, &DrawingStudio::_on_canvas_property).bind(StringName("wand_tolerance"), 100.0),
                        false, U" %");
            _add_options(tool_options, U"Référence", to_strings({U"Calque courant", U"Tous les calques"}), canvas->get_fill_reference(),
                         callable_mp(this, &DrawingStudio::_on_canvas_option).bind(StringName("fill_reference")));
        }
        add_section(tool_options, U"Sélection");
        GridContainer *grid = memnew(GridContainer);
        grid->set_columns(2);
        tool_options->add_child(grid);
        const Ref<DrawingDocument> document = canvas->get_document();
        if (document.is_valid())
        {
            _add_button(grid, U"Tout", U"Tout sélectionner (Ctrl+A)", callable_mp(document.ptr(), &DrawingDocument::select_all));
            _add_button(grid, U"Rien", U"Désélectionner (Ctrl+D)", callable_mp(document.ptr(), &DrawingDocument::deselect));
            _add_button(grid, U"Inverser", U"Inverser la sélection (Ctrl+Maj+I)", callable_mp(document.ptr(), &DrawingDocument::invert_selection));
            _add_button(grid, U"Effacer", U"Effacer le contenu de la sélection (Suppr)", callable_mp(document.ptr(), &DrawingDocument::clear_selection_content));
            _add_button(grid, U"Remplir", U"Remplir la sélection avec la couleur principale", callable_mp(document.ptr(), &DrawingDocument::fill_selection).bind(canvas->get_primary_color()));
        }
        break;
    }
    case DrawingCanvas::TOOL_PAN:
    default:
    {
        Label *help = memnew(Label);
        help->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
        help->set_text(U"Glisser pour déplacer la vue.\nMolette : zoom ; Alt + molette : rotation.\nEspace + glisser : déplacer ; Ctrl + Espace : "
                       U"zoom ; Maj + Espace : rotation.\nTactile : deux doigts.");
        tool_options->add_child(help);
        break;
    }
    }
}

void DrawingStudio::_build_brush_options(bool p_eraser)
{
    const TypedArray<DrawingBrush> list = _brush_list(p_eraser);
    const Ref<DrawingBrush> brush = _edited_brush();
    PackedStringArray names;
    int selected = -1;
    for (int64_t i = 0; i < list.size(); i++)
    {
        const Ref<DrawingBrush> preset = list[i];
        names.push_back(preset->get_name().is_empty() ? String(U"Pinceau ") + String::num_int64(i + 1) : preset->get_name());
        if (preset == brush)
        {
            selected = int(i);
        }
    }
    if (selected < 0)
    {
        names.push_back(brush->get_name().is_empty() ? String(U"Personnalisé") : brush->get_name());
        selected = names.size() - 1;
    }
    _add_options(tool_options, U"Préréglage", names, selected, callable_mp(this, &DrawingStudio::_on_preset_selected).bind(p_eraser));

    const Callable property = callable_mp(this, &DrawingStudio::_on_brush_property);
    size_slider = _add_slider(tool_options, U"Taille", 0.5, 500, 0.5, brush->get_size(), property.bind(StringName("size"), 1.0), true, U" px");
    _add_slider(tool_options, U"Opacité", 0, 100, 1, brush->get_opacity() * 100.0, property.bind(StringName("opacity"), 100.0), false, U" %");
    HSlider *flow = _add_slider(tool_options, U"Densité", 1, 100, 1, brush->get_flow() * 100.0, property.bind(StringName("flow"), 100.0), false, U" %");
    flow->set_tooltip_text(U"Vitesse d'accumulation des tampons d'un même trait (faible : aérographe)");
    if (!brush->has_tip_texture())
    {
        _add_slider(tool_options, U"Dureté", 0, 100, 1, brush->get_hardness() * 100.0, property.bind(StringName("hardness"), 100.0), false, U" %");
    }
    _add_slider(tool_options, U"Espacement", 1, 200, 1, brush->get_spacing() * 100.0, property.bind(StringName("spacing"), 100.0), false, U" %");
    HSlider *stabilization = _add_slider(tool_options, U"Stabilisation", 0, 100, 1, brush->get_stabilization(), property.bind(StringName("stabilization"), 1.0));
    stabilization->set_tooltip_text(U"Lisse le trait (corrige les tremblements, au prix d'un léger retard)");
    _add_slider(tool_options, U"Angle", -180, 180, 1, brush->get_angle(), property.bind(StringName("angle"), 1.0), false, U"°");
    _add_slider(tool_options, U"Rondeur", 5, 100, 1, brush->get_roundness() * 100.0, property.bind(StringName("roundness"), 100.0), false, U" %");
    _add_check(tool_options, U"Orienter selon le trait", brush->get_follow_direction(), callable_mp(this, &DrawingStudio::_on_brush_bool).bind(StringName("follow_direction")));
    if (!p_eraser)
    {
        _add_options(tool_options, U"Mode", to_strings({U"Normal", U"Derrière", U"Gomme"}), brush->get_blend_mode(), callable_mp(this, &DrawingStudio::_on_brush_blend));
    }

    // 📈 Dynamique : pression et vitesse, pour la taille et pour l'opacité.
    const Callable flag = callable_mp(this, &DrawingStudio::_on_brush_flag);
    add_section(tool_options, U"Taille selon");
    HBoxContainer *size_row = memnew(HBoxContainer);
    tool_options->add_child(size_row);
    _add_check(size_row, U"Pression", brush->get_size_dynamics() & DrawingBrush::DYNAMIC_PRESSURE, flag.bind(StringName("size_dynamics"), int(DrawingBrush::DYNAMIC_PRESSURE)));
    _add_check(size_row, U"Vitesse", brush->get_size_dynamics() & DrawingBrush::DYNAMIC_VELOCITY, flag.bind(StringName("size_dynamics"), int(DrawingBrush::DYNAMIC_VELOCITY)));
    _add_slider(tool_options, U"Taille min", 0, 100, 1, brush->get_size_min() * 100.0, property.bind(StringName("size_min"), 100.0), false, U" %");
    add_section(tool_options, U"Opacité selon");
    HBoxContainer *opacity_row = memnew(HBoxContainer);
    tool_options->add_child(opacity_row);
    _add_check(opacity_row, U"Pression", brush->get_opacity_dynamics() & DrawingBrush::DYNAMIC_PRESSURE, flag.bind(StringName("opacity_dynamics"), int(DrawingBrush::DYNAMIC_PRESSURE)));
    _add_check(opacity_row, U"Vitesse", brush->get_opacity_dynamics() & DrawingBrush::DYNAMIC_VELOCITY, flag.bind(StringName("opacity_dynamics"), int(DrawingBrush::DYNAMIC_VELOCITY)));
    _add_slider(tool_options, U"Opacité min", 0, 100, 1, brush->get_opacity_min() * 100.0, property.bind(StringName("opacity_min"), 100.0), false, U" %");
    HSlider *velocity = _add_slider(tool_options, U"Vitesse max", 100, 20000, 10, brush->get_velocity_max(), property.bind(StringName("velocity_max"), 1.0), true, U" px/s");
    velocity->set_tooltip_text(U"Vitesse (pixels de l'écran par seconde) à laquelle l'effet de la vitesse est complet");

    // 🧻 Grain et ✨ shader
    add_section(tool_options, U"Texture");
    PackedStringArray grain_names;
    int grain_selected = -1;
    for (int64_t i = 0; i < grain_textures.size(); i++)
    {
        const Array entry = grain_textures[i];
        grain_names.push_back(entry[0]);
        if (Ref<Texture2D>(entry[1]) == brush->get_texture())
        {
            grain_selected = int(i);
        }
    }
    if (grain_selected < 0)
    {
        grain_names.push_back(U"Personnalisée");
        grain_selected = grain_names.size() - 1;
    }
    _add_options(tool_options, U"Grain", grain_names, grain_selected, callable_mp(this, &DrawingStudio::_on_grain_selected));
    if (brush->get_texture().is_valid())
    {
        _add_slider(tool_options, U"Force", 0, 100, 1, brush->get_texture_strength() * 100.0, property.bind(StringName("texture_strength"), 100.0), false, U" %");
        _add_slider(tool_options, U"Échelle", 0.1, 10, 0.05, brush->get_texture_scale(), property.bind(StringName("texture_scale"), 1.0), true);
    }
    if (brush->get_material().is_valid())
    {
        Label *shader = memnew(Label);
        const String name = brush->get_material()->get_name().is_empty() ? brush->get_material()->get_path().get_file() : brush->get_material()->get_name();
        shader->set_text(String(U"Shader : ") + (name.is_empty() ? String(U"intégré") : name));
        shader->set_tooltip_text(U"Le trait est rendu à travers ce ShaderMaterial (réglable dans l'inspecteur du pinceau)");
        tool_options->add_child(shader);
    }
}

void DrawingStudio::_on_tool_pressed(int p_tool) { canvas->set_tool(DrawingCanvas::Tool(p_tool)); }

void DrawingStudio::_on_tool_changed(int p_tool)
{
    if (p_tool >= 0 && p_tool < DrawingCanvas::TOOL_MAX)
    {
        tool_buttons[p_tool]->set_pressed_no_signal(true);
    }
    tool_options_dirty = true;
}

void DrawingStudio::_on_preset_selected(int p_index, bool p_eraser)
{
    const TypedArray<DrawingBrush> list = _brush_list(p_eraser);
    if (p_index < 0 || p_index >= list.size())
    {
        return;
    }
    if (p_eraser)
    {
        canvas->set_eraser(list[p_index]);
    }
    else
    {
        canvas->set_brush(list[p_index]);
    }
    tool_options_dirty = true;
}

void DrawingStudio::_on_brush_property(double p_value, const StringName &p_property, double p_scale)
{
    const Ref<DrawingBrush> brush = _edited_brush();
    if (brush.is_valid())
    {
        brush->set(p_property, p_value / p_scale);
    }
}

void DrawingStudio::_on_brush_flag(bool p_pressed, const StringName &p_property, int p_flag)
{
    const Ref<DrawingBrush> brush = _edited_brush();
    if (brush.is_null())
    {
        return;
    }
    const int flags = brush->get(p_property);
    brush->set(p_property, p_pressed ? (flags | p_flag) : (flags & ~p_flag));
}

void DrawingStudio::_on_brush_bool(bool p_pressed, const StringName &p_property)
{
    const Ref<DrawingBrush> brush = _edited_brush();
    if (brush.is_valid())
    {
        brush->set(p_property, p_pressed);
    }
}

void DrawingStudio::_on_brush_blend(int p_index)
{
    const Ref<DrawingBrush> brush = _edited_brush();
    if (brush.is_valid())
    {
        brush->set_blend_mode(DrawingBrush::BrushBlend(p_index));
    }
}

void DrawingStudio::_on_grain_selected(int p_index)
{
    const Ref<DrawingBrush> brush = _edited_brush();
    if (brush.is_null() || p_index < 0 || p_index >= grain_textures.size())
    {
        return; // « Personnalisée » : la texture actuelle est gardée
    }
    const Array entry = grain_textures[p_index];
    brush->set_texture(entry[1]);
    tool_options_dirty = true; // réglages de force et d'échelle affichés ou non
}

void DrawingStudio::_on_canvas_property(double p_value, const StringName &p_property, double p_scale) { canvas->set(p_property, p_value / p_scale); }

void DrawingStudio::_on_canvas_bool(bool p_pressed, const StringName &p_property) { canvas->set(p_property, p_pressed); }

void DrawingStudio::_on_canvas_option(int p_index, const StringName &p_property) { canvas->set(p_property, p_index); }

// 🗂️ Calques

void DrawingStudio::_rebuild_layers()
{
    layers_dirty = false;
    for (int i = layer_list->get_child_count() - 1; i >= 0; i--)
    {
        Node *child = layer_list->get_child(i);
        layer_list->remove_child(child);
        child->queue_free();
    }
    const Ref<DrawingDocument> document = canvas->get_document();
    if (document.is_null())
    {
        return;
    }
    const int current = document->get_current_layer();
    // Du calque le plus haut au plus bas, comme dans la palette de calques de CSP.
    for (int i = document->get_layer_count() - 1; i >= 0; i--)
    {
        const Ref<DrawingLayer> layer = document->get_layer(i);
        HBoxContainer *row = memnew(HBoxContainer);
        layer_list->add_child(row);
        CheckBox *visible = memnew(CheckBox);
        visible->set_pressed_no_signal(layer->is_visible());
        visible->set_tooltip_text(U"Afficher / masquer");
        visible->set_focus_mode(FOCUS_NONE);
        visible->connect("toggled", callable_mp(this, &DrawingStudio::_on_layer_visibility).bind(i));
        row->add_child(visible);
        Button *name = memnew(Button);
        String text = layer->is_clipped() ? String(U"  ↳ ") : String();
        text += layer->get_layer_name();
        if (layer->is_locked())
        {
            text += U"  [verrouillé]";
        }
        name->set_text(text);
        name->set_tooltip_text(String(BLEND_NAMES[layer->get_blend_mode()]) + U" · " + String::num_int64(int64_t(Math::round(layer->get_opacity() * 100.0f))) + U" %");
        name->set_toggle_mode(true);
        name->set_pressed_no_signal(i == current);
        name->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
        name->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
        name->set_h_size_flags(SIZE_EXPAND_FILL);
        name->set_focus_mode(FOCUS_NONE);
        name->connect("pressed", callable_mp(this, &DrawingStudio::_on_layer_selected).bind(i));
        row->add_child(name);
        Label *opacity = memnew(Label);
        opacity->set_text(String::num_int64(int64_t(Math::round(layer->get_opacity() * 100.0f))) + U" %");
        opacity->set_modulate(Color(1, 1, 1, 0.6f));
        row->add_child(opacity);
    }

    const Ref<DrawingLayer> layer = document->get_current();
    if (layer.is_valid())
    {
        layer_blend->select(layer->get_blend_mode());
        layer_opacity->set_value_no_signal(layer->get_opacity() * 100.0f);
        layer_alpha_lock->set_pressed_no_signal(layer->is_alpha_locked());
        layer_clip->set_pressed_no_signal(layer->is_clipped());
        layer_lock->set_pressed_no_signal(layer->is_locked());
        if (!layer_name->has_focus())
        {
            layer_name->set_text(layer->get_layer_name());
        }
    }
    layer_remove->set_disabled(document->get_layer_count() <= 1);
    layer_merge->set_disabled(current <= 0);
}

void DrawingStudio::_on_layers_changed() { layers_dirty = true; }

void DrawingStudio::_on_layer_selected(int p_index)
{
    canvas->get_document()->set_current_layer(p_index);
    layers_dirty = true; // les autres boutons se relâchent
}

void DrawingStudio::_on_layer_visibility(bool p_visible, int p_index) { canvas->get_document()->set_layer_property(p_index, "visible", p_visible); }

void DrawingStudio::_on_layer_name_submitted(const String &p_text)
{
    const Ref<DrawingDocument> document = canvas->get_document();
    if (!p_text.strip_edges().is_empty())
    {
        document->set_layer_property(document->get_current_layer(), "layer_name", p_text.strip_edges());
    }
    layer_name->release_focus();
    canvas->grab_focus();
}

void DrawingStudio::_on_layer_blend(int p_index)
{
    const Ref<DrawingDocument> document = canvas->get_document();
    document->set_layer_property(document->get_current_layer(), "blend_mode", p_index);
}

void DrawingStudio::_on_layer_opacity(double p_value)
{
    const Ref<DrawingDocument> document = canvas->get_document();
    document->set_layer_property(document->get_current_layer(), "opacity", p_value / 100.0);
}

void DrawingStudio::_on_layer_flag(bool p_pressed, const StringName &p_property)
{
    const Ref<DrawingDocument> document = canvas->get_document();
    document->set_layer_property(document->get_current_layer(), p_property, p_pressed);
}

void DrawingStudio::_on_add_layer() { canvas->get_document()->add_layer(); }

void DrawingStudio::_on_duplicate_layer()
{
    const Ref<DrawingDocument> document = canvas->get_document();
    document->duplicate_layer(document->get_current_layer());
}

void DrawingStudio::_on_remove_layer()
{
    const Ref<DrawingDocument> document = canvas->get_document();
    if (document->get_layer_count() > 1)
    {
        document->remove_layer(document->get_current_layer());
    }
}

void DrawingStudio::_on_move_layer(int p_direction)
{
    const Ref<DrawingDocument> document = canvas->get_document();
    document->move_layer(document->get_current_layer(), document->get_current_layer() + p_direction);
}

void DrawingStudio::_on_merge_down()
{
    const Ref<DrawingDocument> document = canvas->get_document();
    if (!document->merge_down(document->get_current_layer()))
    {
        show_message(U"Fusion impossible (calque le plus bas, ou calque inférieur verrouillé).");
    }
}

void DrawingStudio::_on_clear_layer()
{
    const Ref<DrawingDocument> document = canvas->get_document();
    document->clear_layer(document->get_current_layer());
}

// ↩️ Historique, vue, couleurs

void DrawingStudio::_refresh_history()
{
    const Ref<DrawingDocument> document = canvas->get_document();
    const bool can_undo = document.is_valid() && document->can_undo();
    const bool can_redo = document.is_valid() && document->can_redo();
    undo_button->set_disabled(!can_undo);
    redo_button->set_disabled(!can_redo);
    undo_button->set_tooltip_text(can_undo ? String(U"Annuler : ") + document->get_undo_name() + U" (Ctrl+Z)" : String(U"Rien à annuler"));
    redo_button->set_tooltip_text(can_redo ? String(U"Rétablir : ") + document->get_redo_name() + U" (Ctrl+Y)" : String(U"Rien à rétablir"));
}

void DrawingStudio::_on_history_changed() { _refresh_history(); }

void DrawingStudio::_on_undo()
{
    canvas->get_document()->undo();
    tool_options_dirty = true; // un réglage de calque a pu changer
}

void DrawingStudio::_on_redo() { canvas->get_document()->redo(); }

void DrawingStudio::_refresh_zoom()
{
    String text = String::num_int64(int64_t(Math::round(canvas->get_zoom() * 100.0f))) + U" %";
    if (!Math::is_zero_approx(canvas->get_view_rotation()))
    {
        text += U" · " + String::num_int64(int64_t(Math::round(canvas->get_view_rotation()))) + U"°";
    }
    if (canvas->get_flip_h())
    {
        text += U" · miroir";
    }
    const Ref<DrawingDocument> document = canvas->get_document();
    if (document.is_valid())
    {
        text += U"   " + String::num_int64(document->get_size().x) + U" × " + String::num_int64(document->get_size().y) + U" px";
    }
    zoom_label->set_text(text);
    flip_button->set_pressed_no_signal(canvas->get_flip_h());
}

void DrawingStudio::_on_view_changed() { _refresh_zoom(); }

void DrawingStudio::_on_rotate(float p_degrees) { canvas->rotate_at(p_degrees, canvas->get_size() * 0.5f); }

void DrawingStudio::_on_flip(bool p_pressed) { canvas->set_flip_h(p_pressed); }

void DrawingStudio::_refresh_colors()
{
    updating = true;
    color_picker->set_pick_color(canvas->get_primary_color());
    updating = false;
    primary_swatch->set_color(canvas->get_primary_color());
    secondary_swatch->set_color(canvas->get_secondary_color());
}

void DrawingStudio::_on_primary_color_changed(const Color & /*p_color*/)
{
    _refresh_colors();
    if (canvas->get_tool() >= DrawingCanvas::TOOL_SELECT_RECT && canvas->get_tool() <= DrawingCanvas::TOOL_SELECT_WAND)
    {
        tool_options_dirty = true; // bouton « Remplir » lié à la couleur
    }
}

void DrawingStudio::_on_picker_color_changed(const Color &p_color)
{
    if (!updating)
    {
        canvas->set_primary_color(p_color);
    }
}

void DrawingStudio::_on_stroke_rejected() { show_message(U"Impossible de peindre : le calque courant est masqué ou verrouillé."); }

// 📁 Fichiers

void DrawingStudio::_on_new_pressed()
{
    const Ref<DrawingDocument> document = canvas->get_document();
    if (document.is_valid())
    {
        new_width->set_value(document->get_size().x);
        new_height->set_value(document->get_size().y);
    }
    new_dialog->popup_centered(Vector2(320, 0));
}

void DrawingStudio::_on_new_confirmed() { new_document(Vector2i(int(new_width->get_value()), int(new_height->get_value())), new_transparent->is_pressed() ? Color(1, 1, 1, 0) : Color(1, 1, 1, 1)); }

void DrawingStudio::_on_open_pressed() { open_dialog->popup_centered_ratio(0.6f); }

void DrawingStudio::_on_save_pressed() { save(); }

void DrawingStudio::_on_save_as_pressed()
{
    if (!current_path.is_empty())
    {
        save_dialog->set_current_path(current_path);
    }
    save_dialog->popup_centered_ratio(0.6f);
}

void DrawingStudio::_on_open_selected(const String &p_path) { open_file(p_path); }

void DrawingStudio::_on_save_selected(const String &p_path) { save_as(p_path); }

// 🔗 Liaisons

void DrawingStudio::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_canvas"), &DrawingStudio::get_canvas);
    ClassDB::bind_method(D_METHOD("set_document", "document"), &DrawingStudio::set_document);
    ClassDB::bind_method(D_METHOD("get_document"), &DrawingStudio::get_document);
    ClassDB::bind_method(D_METHOD("set_brushes", "brushes"), &DrawingStudio::set_brushes);
    ClassDB::bind_method(D_METHOD("get_brushes"), &DrawingStudio::get_brushes);
    ClassDB::bind_method(D_METHOD("set_erasers", "erasers"), &DrawingStudio::set_erasers);
    ClassDB::bind_method(D_METHOD("get_erasers"), &DrawingStudio::get_erasers);
    ClassDB::bind_method(D_METHOD("set_show_file_buttons", "show"), &DrawingStudio::set_show_file_buttons);
    ClassDB::bind_method(D_METHOD("get_show_file_buttons"), &DrawingStudio::get_show_file_buttons);
    ClassDB::bind_method(D_METHOD("set_file_access", "access"), &DrawingStudio::set_file_access);
    ClassDB::bind_method(D_METHOD("get_file_access"), &DrawingStudio::get_file_access);
    ClassDB::bind_method(D_METHOD("get_current_path"), &DrawingStudio::get_current_path);
    ClassDB::bind_method(D_METHOD("new_document", "size", "background"), &DrawingStudio::new_document, DEFVAL(Color(1, 1, 1, 1)));
    ClassDB::bind_method(D_METHOD("open_file", "path"), &DrawingStudio::open_file);
    ClassDB::bind_method(D_METHOD("edit_document", "document", "path"), &DrawingStudio::edit_document, DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("save"), &DrawingStudio::save);
    ClassDB::bind_method(D_METHOD("save_as", "path"), &DrawingStudio::save_as);
    ClassDB::bind_method(D_METHOD("show_message", "message"), &DrawingStudio::show_message);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "document", PROPERTY_HINT_RESOURCE_TYPE, "DrawingDocument", PROPERTY_USAGE_NONE), "set_document", "get_document");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "brushes", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":DrawingBrush"),
                 "set_brushes", "get_brushes");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "erasers", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":DrawingBrush"),
                 "set_erasers", "get_erasers");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_file_buttons"), "set_show_file_buttons", "get_show_file_buttons");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "file_access", PROPERTY_HINT_ENUM, "Resources,User Data,File System"), "set_file_access", "get_file_access");

    ADD_SIGNAL(MethodInfo("document_opened", PropertyInfo(Variant::STRING, "path")));
    ADD_SIGNAL(MethodInfo("document_saved", PropertyInfo(Variant::STRING, "path")));
}
