#include "widget_property_modifier_builder.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🧩Générateur de Widget d'éditions/Callback pour les type/property_callback.hpp"
#include "godot_cpp/classes/box_container.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/print_string.hpp"

using namespace godot;
WidgetPropertyModifierBuilder::WidgetPropertyModifierBuilder() {}
WidgetPropertyModifierBuilder::~WidgetPropertyModifierBuilder() {}

BoxContainer *WidgetPropertyModifierBuilder::create_visual_base()
{
    BoxContainer *container = memnew(BoxContainer);
    container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    ScrollContainer *scroll_container = memnew(ScrollContainer);
    scroll_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    container->add_child(scroll_container);
    return container;
}
Label *WidgetPropertyModifierBuilder::create_label_name_widget(Control *base_visual, const String &name)
{
    // Créer un label pour afficher le nom de la propriétée
    Label *label = memnew(Label);
    label->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    label->set_text(name);
    if (base_visual != nullptr)
    {
        base_visual->add_child(label);
    }
    return label;
}
CheckBox *WidgetPropertyModifierBuilder::create_bool_widget(Object *obj, const String &property_name, bool vertical, Control *control_for_adding_widget)
{
    // Si l'objet est null ou que le nom de propriétée est vide
    if (obj == nullptr || property_name.is_empty())
    {
        // Retourner et dire que cette fonction a échoué car il n'y a pas de propriété à modifier
        print_line_rich("[color=red]Erreur : Impossible de créer un CheckBox car l'objet ou le nom de propriété est vide[/color]");
        return nullptr;
    }

    CheckBox *checkbox = memnew(CheckBox);

    // Créer l'objet callback qui capture le contexte
    Ref<PropertyCallback> callback = memnew(PropertyCallback);
    callback->setup(obj, property_name);

    // Sauvegarde la référence dans les métadonnées du node pour maintenir le RefCounted en vie
    checkbox->set_meta("_property_callback", callback);

    // Connecter le signal "toggled" à la méthode du callback
    checkbox->connect("toggled", Callable(callback.ptr(), "on_toggled"));

    // Ajouter le widget au contrôleur parent
    if (control_for_adding_widget != nullptr)
    {
        create_label_name_widget(control_for_adding_widget, property_name);
        control_for_adding_widget->add_child(checkbox);
    }

    return checkbox;
}

void WidgetPropertyModifierBuilder::_bind_methods()
{
    ClassDB::bind_static_method("WidgetPropertyModifierBuilder", D_METHOD("create_base_visual"), static_cast<BoxContainer *(*)()>(&WidgetPropertyModifierBuilder::create_visual_base));
    ClassDB::bind_static_method("WidgetPropertyModifierBuilder", D_METHOD("create_label_name_widget", "obj", "property_name", "control_for_adding_widget"),
                                static_cast<Label *(*)(Control *, const String &)>(&WidgetPropertyModifierBuilder::create_label_name_widget));
    ClassDB::bind_static_method("WidgetPropertyModifierBuilder", D_METHOD("create_bool_widget", "obj", "property_name", "control_for_adding_widget"),
                                static_cast<CheckBox *(*)(Object *, const String &, bool, Control *)>(&WidgetPropertyModifierBuilder::create_bool_widget), DEFVAL(false), DEFVAL(nullptr));
}