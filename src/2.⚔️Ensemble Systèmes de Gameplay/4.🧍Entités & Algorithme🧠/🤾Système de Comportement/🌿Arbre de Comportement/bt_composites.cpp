#include "bt_composites.hpp"

#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

PackedStringArray BTComposite::_get_configuration_warnings() const
{
    PackedStringArray warnings = BTNode::_get_configuration_warnings();
    if (get_bt_child_count() == 0)
    {
        warnings.push_back(U"Un composite a besoin d'au moins un BTNode enfant.");
    }
    return warnings;
}

// ➡️❓ Séquence / sélecteur

void BTOrderedComposite::_native_enter()
{
    current = 0;
    int count = get_bt_child_count();
    order.resize(count);
    for (int i = 0; i < count; i++)
    {
        order[i] = i;
    }
    if (random_order)
    {
        // Mélange de Fisher-Yates
        for (int i = count - 1; i > 0; i--)
        {
            int j = int(UtilityFunctions::randi_range(0, i));
            SWAP(order[i], order[j]);
        }
    }
}

BTNode::Status BTOrderedComposite::_native_tick(double p_delta)
{
    TypedArray<BTNode> children = get_bt_children();
    if (int(order.size()) != children.size())
    {
        _native_enter(); // enfants ajoutés ou retirés pendant l'exécution
    }
    const Status stop = _stop_status();
    for (int i = reactive ? 0 : current; i < int(order.size()); i++)
    {
        BTNode *child = Object::cast_to<BTNode>(children[order[i]]);
        Status status = child->execute(p_delta);
        if (status == RUNNING || status == stop)
        {
            // Un enfant plus loin a pu rester en cours (mode reactive) : il est interrompu.
            for (int j = i + 1; j < int(order.size()); j++)
            {
                BTNode *other = Object::cast_to<BTNode>(children[order[j]]);
                if (other->is_running())
                {
                    other->abort();
                }
            }
            current = i;
            return status;
        }
    }
    current = 0;
    return stop == FAILURE ? SUCCESS : FAILURE;
}

String BTOrderedComposite::get_debug_info() const
{
    PackedStringArray flags;
    if (reactive)
    {
        flags.push_back(U"réactif");
    }
    if (random_order)
    {
        flags.push_back(U"aléatoire");
    }
    if (is_running())
    {
        flags.push_back(String::num_int64(current + 1) + "/" + String::num_int64(order.size()));
    }
    return String(" · ").join(flags);
}

void BTOrderedComposite::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_reactive", "reactive"), &BTOrderedComposite::set_reactive);
    ClassDB::bind_method(D_METHOD("is_reactive"), &BTOrderedComposite::is_reactive);
    ClassDB::bind_method(D_METHOD("set_random_order", "random"), &BTOrderedComposite::set_random_order);
    ClassDB::bind_method(D_METHOD("is_random_order"), &BTOrderedComposite::is_random_order);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reactive"), "set_reactive", "is_reactive");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "random_order"), "set_random_order", "is_random_order");
}

// ⇉ Parallèle

void BTParallel::_native_enter()
{
    results.resize(get_bt_child_count());
    for (uint32_t i = 0; i < results.size(); i++)
    {
        results[i] = -1;
    }
    successes = 0;
    failures = 0;
}

BTNode::Status BTParallel::_native_tick(double p_delta)
{
    TypedArray<BTNode> children = get_bt_children();
    if (int(results.size()) != children.size())
    {
        _native_enter();
    }
    const int count = children.size();
    const int needed_successes = success_threshold == 0 ? count : MIN(success_threshold, count);
    const int needed_failures = failure_threshold == 0 ? count : MIN(failure_threshold, count);
    for (int i = 0; i < count; i++)
    {
        if (results[i] != -1)
        {
            continue; // déjà terminé pendant cette exécution
        }
        Status status = Object::cast_to<BTNode>(children[i])->execute(p_delta);
        if (status != RUNNING)
        {
            results[i] = status;
            (status == SUCCESS ? successes : failures)++;
        }
    }
    Status result = RUNNING;
    if (successes >= needed_successes)
    {
        result = SUCCESS;
    }
    else if (failures >= needed_failures || successes + failures >= count)
    {
        result = FAILURE; // seuil d'échecs atteint, ou tout est terminé sans atteindre celui des réussites
    }
    if (result != RUNNING)
    {
        _abort_running_children();
    }
    return result;
}

String BTParallel::get_debug_info() const
{
    if (!is_running())
    {
        return String();
    }
    return U"✔" + String::num_int64(successes) + U"  ✘" + String::num_int64(failures);
}

void BTParallel::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_success_threshold", "threshold"), &BTParallel::set_success_threshold);
    ClassDB::bind_method(D_METHOD("get_success_threshold"), &BTParallel::get_success_threshold);
    ClassDB::bind_method(D_METHOD("set_failure_threshold", "threshold"), &BTParallel::set_failure_threshold);
    ClassDB::bind_method(D_METHOD("get_failure_threshold"), &BTParallel::get_failure_threshold);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "success_threshold", PROPERTY_HINT_RANGE, "0,16,1,or_greater"), "set_success_threshold", "get_success_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "failure_threshold", PROPERTY_HINT_RANGE, "0,16,1,or_greater"), "set_failure_threshold", "get_failure_threshold");
}
