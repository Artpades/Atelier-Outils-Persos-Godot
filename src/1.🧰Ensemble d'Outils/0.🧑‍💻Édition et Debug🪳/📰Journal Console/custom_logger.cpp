#include "custom_logger.hpp"

#include <godot_cpp/core/class_db.hpp>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;
void L::log(const String &message, const Variant &object_caller, const String &icon_path)
{
    Time *time = Time::get_singleton();
    // Récupération de la date et de l'heure actuelles grâce à Time
    Dictionary date = time->get_time_dict_from_system();

    // Analyser le type du Variant object_caller pour obtenir le nom de la classe

    String class_name; // Pour récupérer le nom de la classe

    if (object_caller.get_type() == Variant::OBJECT) // Si c'est un objet
    {
        // Récupérer l'objet à partir du Variant
        Object *obj = Object::cast_to<Object>(object_caller);
        if (obj) // Si l'objet est valide, récupérer le nom de la classe
        {
            // Dans le cas où l'objet est un Node, on peut utiliser son name pour
            if (obj->is_class("Node"))
            {
                Node *node = Object::cast_to<Node>(obj);
                class_name = node->get_name(); // Récupérer le nom du Node
            }
            else
            {
                class_name = obj->get_class(); // Sinon, récupérer le nom de la classe
            }
        }
    }
    // Si c'est une chaîne de caractères
    else if (object_caller.get_type() == Variant::STRING)
    {
        class_name = object_caller;
    }
    // Récupérer les valeurs de la date
    int hour = date["hour"];
    int minute = date["minute"];
    int second = date["second"];

    // Formoter avec 2 chiffres (remplissage automatique par un zéro)
    String time_str = vformat("%02d:%02d:%02d", hour, minute, second);

    String log_message = "[color=gray][" + time_str + "][/color] ";
    if (!class_name.is_empty())
    {
        log_message += "[b]" + class_name + "[/b]";
    }
    if (!icon_path.is_empty())
    {
        log_message += "[img]" + icon_path + "[/img] : ";
    }
    log_message += message;

    print_line_rich(log_message);
}
void L::_bind_methods() { ClassDB::bind_static_method("L", D_METHOD("log", "message", "object_caller", "icon_path"), &L::log); }