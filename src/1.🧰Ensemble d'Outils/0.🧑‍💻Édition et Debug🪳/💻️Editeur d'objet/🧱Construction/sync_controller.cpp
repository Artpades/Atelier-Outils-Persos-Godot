#include "sync_controller.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/📰Journal Console/custom_logger.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/object.hpp>

using namespace godot;

void SyncController::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_interval", "interval"), &SyncController::set_interval);
    ClassDB::bind_method(D_METHOD("get_interval"), &SyncController::get_interval);
    ClassDB::bind_method(D_METHOD("refresh_all", "force"), &SyncController::refresh_all);
    ClassDB::bind_static_method("SyncController", D_METHOD("same_value", "a", "b"), &SyncController::same_value);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "interval", PROPERTY_HINT_RANGE, "0,5,0.01,suffix:s"), "set_interval", "get_interval");

    // La cible a été libérée : les widgets ne sont plus valides.
    ADD_SIGNAL(MethodInfo("target_lost"));
}

SyncController::SyncController()
{
    // Les fenêtres de debug doivent rester à jour même quand le jeu est en pause.
    set_process_mode(PROCESS_MODE_ALWAYS);
}

bool SyncController::same_value(const Variant &p_a, const Variant &p_b)
{
    Variant::Type ta = p_a.get_type();
    Variant::Type tb = p_b.get_type();
    bool a_number = ta == Variant::INT || ta == Variant::FLOAT;
    bool b_number = tb == Variant::INT || tb == Variant::FLOAT;
    if (a_number && b_number)
    {
        double a = p_a;
        double b = p_b;
        return Math::abs(a - b) <= 1e-4 * MAX(1.0, Math::abs(b));
    }
    return ta == tb && p_a.hash_compare(p_b);
}

void SyncController::set_interval(double p_interval) { interval = MAX(p_interval, 0.0); }
double SyncController::get_interval() const { return interval; }

void SyncController::set_target(Object *p_target)
{
    target_id = p_target ? ObjectID(p_target->get_instance_id()) : ObjectID();
    target_lost = false;
    set_process(p_target != nullptr);
}

void SyncController::register_widget(PropertyWidget *p_widget)
{
    if (p_widget && p_widget->is_value_widget())
    {
        widgets.push_back(ObjectID(p_widget->get_instance_id()));
    }
}

void SyncController::clear() { widgets.clear(); }

void SyncController::_refresh_widget(PropertyWidget *p_widget, bool p_force)
{
    Ref<ValueAccessor> accessor = p_widget->get_accessor();
    if (accessor.is_null() || !accessor->is_valid())
    {
        return;
    }
    Variant value = p_widget->read_value();
    // Un widget n'affiche que les valeurs de son type déclaré : pendant qu'un VariantWidget change de type,
    // son widget enfant (encore à l'ancien type) ne doit pas recevoir la nouvelle valeur.
    Variant::Type declared = (Variant::Type)(int)p_widget->get_info().get("type", 0);
    Variant::Type actual = value.get_type();
    bool numbers = (declared == Variant::INT || declared == Variant::FLOAT) && (actual == Variant::INT || actual == Variant::FLOAT);
    bool texts = (declared == Variant::STRING || declared == Variant::STRING_NAME) && (actual == Variant::STRING || actual == Variant::STRING_NAME);
    if (declared != Variant::NIL && actual != Variant::NIL && declared != actual && !numbers && !texts)
    {
        return;
    }
    if (!p_force && p_widget->has_last_value() && same_value(p_widget->get_last_value(), value))
    {
        return;
    }
    p_widget->set_last_value(value);
    p_widget->pull(value);
}

void SyncController::_process(double p_delta)
{
    elapsed += p_delta;
    if (elapsed < interval)
    {
        return;
    }
    elapsed = 0.0;

    if (!ObjectDB::get_instance(target_id))
    {
        if (!target_lost)
        {
            target_lost = true;
            widgets.clear();
            set_process(false);
            emit_signal("target_lost");
        }
        return;
    }

    for (int i = widgets.size() - 1; i >= 0; i--)
    {
        PropertyWidget *widget = Object::cast_to<PropertyWidget>(ObjectDB::get_instance(widgets[i]));
        if (!widget)
        {
            widgets.remove_at(i); // widget libéré (reconstruction de l'interface)
            continue;
        }
        if (!widget->is_visible_in_tree() || widget->has_edit_focus())
        {
            continue;
        }
        _refresh_widget(widget, false);
    }
}

void SyncController::refresh_all(bool p_force)
{
    for (int i = 0; i < widgets.size(); i++)
    {
        if (PropertyWidget *widget = Object::cast_to<PropertyWidget>(ObjectDB::get_instance(widgets[i])))
        {
            _refresh_widget(widget, p_force);
        }
    }
}

void SyncController::notify_written(PropertyWidget *p_widget, const Variant &p_sent, bool p_committing)
{
    if (!p_widget)
    {
        return;
    }
    Ref<ValueAccessor> accessor = p_widget->get_accessor();
    if (accessor.is_null() || !accessor->is_valid())
    {
        return;
    }

    Variant actual = accessor->read();
    p_widget->set_last_value(actual); // la valeur écrite ne doit pas revenir au widget comme un changement externe

    // Pendant une édition (glissement, frappe), on ne touche pas au widget : la correction se fait à la validation.
    if (!p_committing || same_value(p_sent, actual))
    {
        return;
    }

    // Setter qui refuse (valeur conservée) ou transforme (clamp, arrondi) : le widget revient à la valeur réelle.
    log_it(U"[color=orange]« " + String(p_widget->get_property_name()) + U" » : la valeur écrite (" + PropertyWidget::stringify(p_sent) +
               U") n'a pas été prise telle quelle ; valeur relue : " + PropertyWidget::stringify(actual) + "[/color]",
           accessor->get_target());
    p_widget->pull(actual);
}
