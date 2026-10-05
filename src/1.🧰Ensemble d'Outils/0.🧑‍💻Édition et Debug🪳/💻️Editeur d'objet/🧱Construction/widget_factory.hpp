#ifndef WIDGET_FACTORY_HPP
#define WIDGET_FACTORY_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/data_entry.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/property_widget.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot
{
// Fabrique de widgets : traduit une DataEntry en PropertyWidget (non encore configuré par setup()).
// Choix du widget : réglages de l'entrée (widget_choice) > indice de la propriété (via le DataKind :
// Enum, Flags...) > widget par défaut du type (WidgetTable).
// Extensible : set_widget_class() remplace le widget intégré d'un choix par une autre classe
// (native ou class_name) héritant de PropertyWidget, pour toutes les entrées de cet éditeur.
class WidgetFactory : public RefCounted
{
    GDCLASS(WidgetFactory, RefCounted)

  private:
    Dictionary class_overrides; // WidgetChoice (int) -> StringName

  protected:
    static void _bind_methods();

  public:
    // Instancie une classe héritant de PropertyWidget : native (ClassDB) ou script global (class_name).
    // Retourne nullptr (avec message) si la classe est introuvable ou n'hérite pas de PropertyWidget.
    static PropertyWidget *instantiate_widget_class(const StringName &p_class_name);

    void set_widget_class(DataEntry::WidgetChoice p_choice, const StringName &p_class_name);
    StringName get_widget_class(DataEntry::WidgetChoice p_choice) const;

    // Crée le widget d'une entrée (jamais nullptr : repli sur un PropertyWidget en lecture seule).
    PropertyWidget *create(const Ref<DataEntry> &p_entry) const;
};
} // namespace godot

#endif // WIDGET_FACTORY_HPP
