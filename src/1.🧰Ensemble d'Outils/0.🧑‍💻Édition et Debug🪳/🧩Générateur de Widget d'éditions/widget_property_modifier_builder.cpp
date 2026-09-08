#include "widget_property_modifier_builder.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/🧩Générateur de Widget d'éditions/Callback pour les type/property_bool_callback.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/print_string.hpp"

using namespace godot;
WidgetPropertyModifierBuilder::WidgetPropertyModifierBuilder() {}
WidgetPropertyModifierBuilder::~WidgetPropertyModifierBuilder() {}

BoxContainer *WidgetPropertyModifierBuilder::create_base_visual() { return memnew(BoxContainer); }

CheckBox *WidgetPropertyModifierBuilder::create_bool_check_box(Object *obj, const String &property_name, Control *control_for_adding_widget)
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
    Ref<PropertyBoolCallback> callback = memnew(PropertyBoolCallback);
    callback->setup(obj, property_name);

    // Connecter le signal "toggled" à la méthode du callback
    checkbox->connect("toggled", Callable(callback.ptr(), "on_toggled"));

    // Ajouter le widget au contrôleur parent
    if (control_for_adding_widget != nullptr)
    {
        control_for_adding_widget->add_child(checkbox);
    }

    return checkbox;
}

void WidgetPropertyModifierBuilder::_bind_methods()
{
    ClassDB::bind_static_method("WidgetPropertyModifierBuilder", D_METHOD("create_base_visual"), static_cast<BoxContainer *(*)()>(&WidgetPropertyModifierBuilder::create_base_visual));

    ClassDB::bind_static_method("WidgetPropertyModifierBuilder", D_METHOD("create_bool_check_box", "obj", "property_name", "control_for_adding_widget"),
                                static_cast<CheckBox *(*)(Object *, const String &, Control *)>(&WidgetPropertyModifierBuilder::create_bool_check_box), DEFVAL(nullptr));
}