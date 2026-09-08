// property_callback.hpp
#ifndef PROPERTY_BOOLEAN_CALLBACK_HPP
#define PROPERTY_BOOLEAN_CALLBACK_HPP

#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/core/class_db.hpp"

// Vu qu'on peut pas faire de fonction lambda avec le godot_cpp
// On va créer des classes qui vont servir à faire des callback

using namespace godot;

class PropertyBoolCallback : public RefCounted
{
    GDCLASS(PropertyBoolCallback, RefCounted)

  private:
    Object *target_obj = nullptr;
    String property_name;

  public:
    static void _bind_methods() { ClassDB::bind_method(D_METHOD("on_toggled", "pressed"), &PropertyBoolCallback::on_toggled); }

    void setup(Object *obj, const String &prop)
    {
        target_obj = obj;
        property_name = prop;
    }

    void on_toggled(bool pressed)
    {
        if (target_obj != nullptr)
        {
            target_obj->set(property_name, Variant(pressed));
        }
    }
};

#endif