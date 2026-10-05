#include "widget_factory.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/widget_table.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/container_widgets.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/scalar_widgets.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/signal_widget.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/special_widgets.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/struct_widget.hpp"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

void WidgetFactory::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_widget_class", "choice", "class_name"), &WidgetFactory::set_widget_class);
    ClassDB::bind_method(D_METHOD("get_widget_class", "choice"), &WidgetFactory::get_widget_class);
}

PropertyWidget *WidgetFactory::instantiate_widget_class(const StringName &p_class_name)
{
    const StringName base = "PropertyWidget";
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();

    // 1. Classe native (moteur ou GDExtension) : ClassDB.
    if (cdb->class_exists(p_class_name))
    {
        if (!cdb->is_parent_class(p_class_name, base) || !cdb->can_instantiate(p_class_name))
        {
            log_it(U"[color=red]Widget « " + String(p_class_name) + U" » : classe non instanciable ou n'héritant pas de PropertyWidget[/color]", "WidgetFactory");
            return nullptr;
        }
        Object *obj = cdb->instantiate(p_class_name); // Node : pas de RefCounted, le Variant temporaire ne le libère pas
        return Object::cast_to<PropertyWidget>(obj);
    }

    // 2. Script global (class_name) : absent de ClassDB, retrouvé via la liste des classes globales.
    TypedArray<Dictionary> globals = ProjectSettings::get_singleton()->get_global_class_list();
    for (int i = 0; i < globals.size(); i++)
    {
        Dictionary global = globals[i];
        if (StringName(global.get("class", "")) != p_class_name)
        {
            continue;
        }
        Ref<Script> script = ResourceLoader::get_singleton()->load(global.get("path", ""));
        if (script.is_null() || !cdb->is_parent_class(script->get_instance_base_type(), base))
        {
            log_it(U"[color=red]Widget « " + String(p_class_name) + U" » : script introuvable ou n'héritant pas de PropertyWidget[/color]", "WidgetFactory");
            return nullptr;
        }
        Object *obj = script->call("new");
        PropertyWidget *widget = Object::cast_to<PropertyWidget>(obj);
        if (!widget && obj)
        {
            memdelete(obj);
        }
        return widget;
    }

    log_it(U"[color=red]Widget « " + String(p_class_name) + U" » : classe inconnue[/color]", "WidgetFactory");
    return nullptr;
}

void WidgetFactory::set_widget_class(DataEntry::WidgetChoice p_choice, const StringName &p_class_name)
{
    if (String(p_class_name).is_empty())
    {
        class_overrides.erase((int)p_choice);
    }
    else
    {
        class_overrides[(int)p_choice] = p_class_name;
    }
}

StringName WidgetFactory::get_widget_class(DataEntry::WidgetChoice p_choice) const { return class_overrides.get((int)p_choice, StringName()); }

PropertyWidget *WidgetFactory::create(const Ref<DataEntry> &p_entry) const
{
    if (p_entry.is_null())
    {
        return memnew(PropertyWidget);
    }
    DataEntry::WidgetChoice choice = p_entry->resolved_choice();

    // Classe imposée : widget personnalisé de l'entrée, ou remplacement enregistré dans la fabrique.
    StringName class_name = choice == DataEntry::WIDGET_CUSTOM ? p_entry->get_custom_widget_class() : get_widget_class(choice);
    if (!String(class_name).is_empty())
    {
        if (PropertyWidget *widget = instantiate_widget_class(class_name))
        {
            return widget;
        }
    }
    else if (choice == DataEntry::WIDGET_CUSTOM)
    {
        log_it(U"[color=orange]« " + String(p_entry->get_data_name()) + U" » : widget Custom sans custom_widget_class, affichage en lecture seule[/color]", "WidgetFactory");
    }

    switch (choice)
    {
    case DataEntry::WIDGET_CHECK_BOX:
    case DataEntry::WIDGET_CHECK_BUTTON:
        return memnew(BoolWidget);
    case DataEntry::WIDGET_SPIN_BOX:
    case DataEntry::WIDGET_H_SLIDER:
    case DataEntry::WIDGET_V_SLIDER:
    case DataEntry::WIDGET_PROGRESS_BAR:
    case DataEntry::WIDGET_TEXTURE_PROGRESS_BAR:
        return memnew(NumberWidget);
    case DataEntry::WIDGET_LINE_EDIT:
    case DataEntry::WIDGET_TEXT_EDIT:
        return memnew(TextWidget);
    case DataEntry::WIDGET_OPTION_BUTTON:
    case DataEntry::WIDGET_CHECK_BUTTON_LIST:
    case DataEntry::WIDGET_CHECK_BOX_LIST:
        return memnew(EnumWidget);
    case DataEntry::WIDGET_COLOR_PICKER_BUTTON:
    case DataEntry::WIDGET_COLOR_RECT_CHANNELS:
        return memnew(ColorWidget);
    case DataEntry::WIDGET_STRUCT_SPIN_BOXES:
        return memnew(StructWidget);
    case DataEntry::WIDGET_BOX_CONTAINER:
    case DataEntry::WIDGET_GRID_CONTAINER:
    case DataEntry::WIDGET_FLOW_CONTAINER:
    case DataEntry::WIDGET_TAB_CONTAINER:
        // Objet ou NodePath : dépliage de l'objet / du nœud ciblé ; sinon collection.
        if (p_entry->get_kind() == DataEntry::KIND_OBJECT || p_entry->get_kind() == DataEntry::KIND_NODE_PATH)
        {
            return memnew(ObjectWidget);
        }
        return memnew(CollectionWidget);
    case DataEntry::WIDGET_PATH_BUTTON:
        return memnew(PathWidget);
    case DataEntry::WIDGET_VARIANT_SELECTOR:
        return memnew(VariantWidget);
    case DataEntry::WIDGET_BUTTON:
        if (p_entry->get_member() == DataEntry::MEMBER_METHOD)
        {
            return memnew(MethodWidget);
        }
        if (p_entry->get_member() == DataEntry::MEMBER_SIGNAL)
        {
            return memnew(SignalWidget);
        }
        return memnew(PropertyWidget);
    default:
        // LABEL : lecture seule.
        return memnew(PropertyWidget);
    }
}
