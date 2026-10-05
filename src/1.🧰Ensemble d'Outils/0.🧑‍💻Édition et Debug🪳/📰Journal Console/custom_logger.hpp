#ifndef CUSTOM_LOGGER_HPP
#define CUSTOM_LOGGER_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/print_string.hpp>

namespace godot
{
// Affiche dans la console : [hh:mm:ss] Appelant [icône] : message (BBCode accepté).
// object_caller : Node (son nom), Object (sa classe) ou String. Optionnel, comme icon_path.
// Définie dans le header (comme print_line) pour être utilisable depuis tous les modules.
inline void log_it(const String &message, const Variant &object_caller = Variant(), const String &icon_path = "")
{
    Time *time = Time::get_singleton();
    // Récupération de la date et de l'heure actuelles grâce à Time
    Dictionary date = time->get_time_dict_from_system();

    // Analyser le type du Variant object_caller pour obtenir le nom de la classe

    String class_name; // Pour récupérer le nom de la classe

    if (object_caller.get_type() == Variant::OBJECT) // Si c'est un objet
    {
        // Récupérer l'objet à partir du Variant
        Object *obj = object_caller;
        if (obj) // Si l'objet est valide, récupérer le nom de la classe
        {
            // Dans le cas où l'objet est un Node, on peut utiliser son name pour
            if (Node *node = Object::cast_to<Node>(obj))
            {
                class_name = node->get_name(); // Récupérer le nom du Node
            }
            else
            {
                class_name = obj->get_class(); // Sinon, récupérer le nom de la classe
            }
        }
    }
    // Si c'est une chaîne de caractères
    else if (object_caller.get_type() == Variant::STRING || object_caller.get_type() == Variant::STRING_NAME)
    {
        class_name = object_caller;
    }
    // Récupérer les valeurs de la date
    int hour = date["hour"];
    int minute = date["minute"];
    int second = date["second"];

    // Formater avec 2 chiffres (remplissage automatique par un zéro)
    String time_str = vformat("%02d:%02d:%02d", hour, minute, second);

    String log_message = "[color=gray][" + time_str + "][/color] ";
    if (!class_name.is_empty())
    {
        log_message += "[b]" + class_name + "[/b] ";
    }
    if (!icon_path.is_empty())
    {
        log_message += "[img]" + icon_path + "[/img] ";
    }
    // Séparateur dès qu'il y a un appelant OU une icône (manquait avec un appelant sans icône)
    if (!class_name.is_empty() || !icon_path.is_empty())
    {
        log_message += ": ";
    }
    log_message += message;

    print_line_rich(log_message);
}

// Version exposée à GDScript : L.log_it(...)
class L : public Object
{
    GDCLASS(L, Object);

  protected:
    static void _bind_methods();

  public:
    static void log_it(const String &message, const Variant &object_caller = Variant(), const String &icon_path = "") { godot::log_it(message, object_caller, icon_path); }
};
} // namespace godot

#endif
