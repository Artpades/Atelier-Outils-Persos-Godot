#include "inspector_binding.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

InspectorBinding *InspectorBinding::attach(Control *p_owner, const ValueAccessor &p_accessor, Pull p_pull, Key p_key)
{
    InspectorBinding *binding = memnew(InspectorBinding);
    binding->set_name("InspectorBinding");
    binding->accessor = p_accessor;
    binding->pull = std::move(p_pull);
    binding->key = p_key ? std::move(p_key) : Key(&InspectorBinding::default_key);
    p_owner->add_child(binding);
    return binding;
}

uint32_t InspectorBinding::default_key(const Variant &p_value)
{
    return p_value.hash() * 31u + (uint32_t)p_value.get_type();
}

void InspectorBinding::_ready()
{
    set_process(true);
    refresh(true);
}

void InspectorBinding::_process(double p_delta)
{
    elapsed += p_delta;
    if (elapsed < DEFAULT_POLL_INTERVAL)
    {
        return;
    }
    elapsed = 0.0;
    refresh(false);
}

void InspectorBinding::refresh(bool p_force)
{
    if (!accessor.alive())
    {
        set_process(false);
        return;
    }

    Variant value = accessor.get();
    uint32_t k = key(value);
    if (!p_force && has_last && k == last_key)
    {
        return;
    }
    last_key = k;
    has_last = true;
    do_pull(value);
}

void InspectorBinding::commit(const Variant &p_value)
{
    if (pulling || !accessor.alive() || !accessor.set)
    {
        return;
    }
    accessor.set(p_value);

    // Relecture : mémorise la valeur pour ne pas la renvoyer au widget, et corrige l'affichage
    // si l'objet a gardé ou transformé la valeur.
    Variant actual = accessor.get();
    last_key = key(actual);
    has_last = true;

    bool same = actual.get_type() == p_value.get_type() && actual.hash_compare(p_value);
    bool numeric = (actual.get_type() == Variant::INT || actual.get_type() == Variant::FLOAT) &&
                   (p_value.get_type() == Variant::INT || p_value.get_type() == Variant::FLOAT);
    if (numeric)
    {
        double a = actual, b = p_value;
        same = Math::abs(a - b) <= 1e-4 * MAX(1.0, Math::abs(a));
    }
    if (!same)
    {
        do_pull(actual);
    }
}

void InspectorBinding::do_pull(const Variant &p_value)
{
    pulling = true;
    pull(p_value);
    pulling = false;
}

void InspectorBinding::refresh_all(Node *p_root)
{
    if (p_root == nullptr)
    {
        return;
    }
    if (InspectorBinding *binding = Object::cast_to<InspectorBinding>(p_root))
    {
        binding->refresh(true);
    }
    for (int i = 0; i < p_root->get_child_count(); i++)
    {
        refresh_all(p_root->get_child(i));
    }
}

void InspectorBinding::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("refresh", "force"), &InspectorBinding::refresh, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("commit", "value"), &InspectorBinding::commit);
}
