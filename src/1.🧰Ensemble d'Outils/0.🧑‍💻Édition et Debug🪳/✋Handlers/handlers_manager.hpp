#ifndef HANDLERS_MANAGER_HPP
#define HANDLERS_MANAGER_HPP

#include <godot_cpp/classes/ref_counted.hpp>

namespace godot
{
class HandlersManager : public RefCounted
{
    GDCLASS(HandlersManager, RefCounted);

  protected:
    static void _bind_methods();

  public:
    HandlersManager();
    ~HandlersManager();
};
} // namespace godot

#endif