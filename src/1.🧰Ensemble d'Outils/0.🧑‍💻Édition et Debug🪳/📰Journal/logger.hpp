#ifndef CUSTOM_LOGGER_HPP
#define CUSTOM_LOGGER_HPP
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot
{
// Journale Console
// Une GDExtension ne peut pas ajouter de fonction dans @GlobalScope : log_in est donc une méthode statique,
// appelable depuis n'importe quel script sans instance : CustomLogger.log_in("message")
class CustomLogger : public Object
{
    GDCLASS(CustomLogger, Object)

  protected:
    static void _bind_methods();

  public:
    // Commande de log
    static void log_in(const String &p_message);
};

} // namespace godot

#endif
