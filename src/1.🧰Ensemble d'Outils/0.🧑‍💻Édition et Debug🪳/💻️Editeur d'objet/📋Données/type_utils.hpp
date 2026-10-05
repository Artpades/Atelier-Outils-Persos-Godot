#ifndef TYPE_UTILS_HPP
#define TYPE_UTILS_HPP

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot
{
// Outils de typage partagés (C++ uniquement).
class TypeUtils
{
  public:
    TypeUtils() = delete; // Classe purement statique

    // Vrai si p_class (classe native OU class_name de script) est p_base ou en hérite.
    // Les class_name ne sont pas dans ClassDB : leur héritage est suivi via la liste des classes globales.
    static bool class_inherits(const String &p_class, const String &p_base);

    // Un argument de signal (p_arg) peut-il alimenter un paramètre de méthode (p_param) ? (descriptions au
    // format get_signal_list / get_method_list). NIL = tout ; mêmes types ; int → float ; objets : héritage.
    static bool is_compatible(const Dictionary &p_arg, const Dictionary &p_param);
};
} // namespace godot

#endif // TYPE_UTILS_HPP
