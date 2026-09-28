#ifndef CUSTOM_LOGGER_API_HPP
#define CUSTOM_LOGGER_API_HPP

#include "commun/module_dependencies.hpp"

#include <godot_cpp/core/print_string.hpp>

namespace godot::custom_logger_api
{
// 📰 Accès au Journal depuis les AUTRES modules (dépendance optionnelle).
// Sans le module Journal, le message part dans la console Godot : l'appelant fonctionne quand même.

inline void log(const String &p_class_name, const String &p_icon_path, const String &p_message)
{
    if (modules::is_loaded("CustomLogger"))
    {
        modules::call_static("CustomLogger", "log", p_class_name, p_icon_path, p_message);
        return;
    }
    print_line_rich("[b]" + p_class_name + "[/b] : " + p_message);
}
} // namespace godot::custom_logger_api

#endif // CUSTOM_LOGGER_API_HPP
