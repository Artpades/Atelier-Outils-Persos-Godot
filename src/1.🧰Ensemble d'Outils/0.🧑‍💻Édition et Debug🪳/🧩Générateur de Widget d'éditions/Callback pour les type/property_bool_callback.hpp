// property_callback.hpp
#ifndef PROPERTY_BOOLEAN_CALLBACK_HPP
#define PROPERTY_BOOLEAN_CALLBACK_HPP

#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/print_string.hpp"

// Vu qu'on peut pas faire de fonction lambda avec le godot_cpp
// On va créer des classes qui vont servir à faire des callback

using namespace godot;

// Renommer WidgetPropertyCallback
class PropertyBoolCallback : public RefCounted
{
    GDCLASS(PropertyBoolCallback, RefCounted)

  private:
    Object *target_obj = nullptr;
    String property_name;

  public:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("setup", "obj", "prop"), &PropertyBoolCallback::setup);
        ClassDB::bind_method(D_METHOD("on_toggled", "pressed"), &PropertyBoolCallback::on_toggled);
    }

    void setup(Object *obj, const String &prop)
    {
        target_obj = obj;
        property_name = prop;
        print_line("Setup Callback avec : ", obj, " et ", prop);
    }

    void on_toggled(bool pressed)
    {
        if (target_obj != nullptr)
        {
            target_obj->set(property_name, Variant(pressed));
            print_line("Définition de : ", target_obj, " et ", property_name, " à ", Variant(pressed));
        }
        else
        {
            print_line("Objet cible manquant pour la propriété : ", property_name);
        }
    }
};

#endif