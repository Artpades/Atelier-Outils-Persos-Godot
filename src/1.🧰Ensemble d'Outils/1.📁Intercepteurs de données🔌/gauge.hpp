#ifndef GAUGE_HPP
#define GAUGE_HPP

#include <godot_cpp/classes/resource.hpp>
namespace godot
{
class Gauge : public Resource
{
  private:
    float current_value;

  protected:
    static void _bind_methods();

  public:
    Gauge();
    ~Gauge();

    virtual float get_current_value() = 0;
    virtual void set_current_value(float value) = 0;
};
} // namespace godot
#endif