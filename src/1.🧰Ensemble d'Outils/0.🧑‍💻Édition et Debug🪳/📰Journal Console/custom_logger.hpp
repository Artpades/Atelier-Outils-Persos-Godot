#ifndef CUSTOM_LOGGER_HPP
#define CUSTOM_LOGGER_HPP

#include <godot_cpp/classes/object.hpp>

namespace godot
{
class L : public Object
{
    GDCLASS(L, Object);

  protected:
    static void _bind_methods();

  public:
    static void log_it(const String &message, const Variant &object_caller = nullptr, const String &icon_path = "");
};
} // namespace godot

#endif