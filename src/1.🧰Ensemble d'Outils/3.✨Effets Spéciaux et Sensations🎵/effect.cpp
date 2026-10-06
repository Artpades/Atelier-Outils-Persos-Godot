#include "effect.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace
{
/// Marque une opération en cours le temps d'une portée (voir Effect::Operation).
struct OperationScope
{
    uint32_t &operations;
    uint32_t flag;
    OperationScope(uint32_t &r_operations, uint32_t p_flag) : operations(r_operations), flag(p_flag) { operations |= flag; }
    ~OperationScope() { operations &= ~flag; }
};
} // namespace

Effect::Effect()
{
    // Chaque instance de scène reçoit sa copie : l'état de l'effet n'est pas partagé entre deux cibles.
    set_local_to_scene(true);
}

// 🎬 Cycle de vie

void Effect::play_fx(Node *p_target)
{
    if (running_operations & OPERATION_PLAY)
    {
        return;
    }
    OperationScope scope(running_operations, OPERATION_PLAY);

    restarting = activated;
    activated = true;
    _play(p_target);
    restarting = false;
    emit_signal("fx_played");

    for (int64_t i = 0; i < effects_on_play_fx.size(); i++)
    {
        Ref<Effect> effect = effects_on_play_fx[i];
        if (effect.is_valid())
        {
            effect->play_fx(p_target);
        }
    }
}

void Effect::process_fx(double p_delta, Node *p_target)
{
    if (!activated || (running_operations & OPERATION_PROCESS))
    {
        return;
    }
    OperationScope scope(running_operations, OPERATION_PROCESS);

    _process(p_delta, p_target);
    for (int64_t i = 0; i < effects_on_play_fx.size(); i++)
    {
        Ref<Effect> effect = effects_on_play_fx[i];
        if (effect.is_valid())
        {
            effect->process_fx(p_delta, p_target);
        }
    }
}

void Effect::physics_process_fx(double p_delta, Node *p_target)
{
    if (!activated || (running_operations & OPERATION_PHYSICS_PROCESS))
    {
        return;
    }
    OperationScope scope(running_operations, OPERATION_PHYSICS_PROCESS);

    _physics_process(p_delta, p_target);
    for (int64_t i = 0; i < effects_on_play_fx.size(); i++)
    {
        Ref<Effect> effect = effects_on_play_fx[i];
        if (effect.is_valid())
        {
            effect->physics_process_fx(p_delta, p_target);
        }
    }
}

void Effect::stop_fx(Node *p_target)
{
    if (!activated || (running_operations & OPERATION_STOP))
    {
        return;
    }
    OperationScope scope(running_operations, OPERATION_STOP);

    _stop(p_target);

    // On arrête les effets en cours d'exécution
    for (int64_t i = 0; i < effects_on_play_fx.size(); i++)
    {
        Ref<Effect> effect = effects_on_play_fx[i];
        if (effect.is_valid())
        {
            effect->stop_fx(p_target);
        }
    }

    // On déclenche les effets dédiés à l'arrêt
    for (int64_t i = 0; i < effects_on_stop_fx.size(); i++)
    {
        Ref<Effect> effect = effects_on_stop_fx[i];
        if (effect.is_valid())
        {
            effect->play_fx(p_target);
        }
    }

    activated = false;
    emit_signal("fx_stopped");
}

// Implémentations par défaut : celles d'un script dérivé, s'il y en a un.
void Effect::_play(Node *p_target) { GDVIRTUAL_CALL(_play_fx, p_target); }
void Effect::_process(double p_delta, Node *p_target) { GDVIRTUAL_CALL(_process_fx, p_delta, p_target); }
void Effect::_physics_process(double p_delta, Node *p_target) { GDVIRTUAL_CALL(_physics_process_fx, p_delta, p_target); }
void Effect::_stop(Node *p_target) { GDVIRTUAL_CALL(_stop_fx, p_target); }

// 🔗 Effets enchaînés et PlayerFX

void Effect::set_effects_on_play_fx(const TypedArray<Effect> &p_effects)
{
    effects_on_play_fx = p_effects;
    set_player_fx(get_player_fx()); // les nouveaux effets enchaînés connaissent le PlayerFX
}

void Effect::set_effects_on_stop_fx(const TypedArray<Effect> &p_effects)
{
    effects_on_stop_fx = p_effects;
    set_player_fx(get_player_fx());
}

void Effect::set_player_fx(Node *p_player)
{
    if (running_operations & OPERATION_LINK)
    {
        return;
    }
    OperationScope scope(running_operations, OPERATION_LINK);

    player_fx_id = p_player ? ObjectID(p_player->get_instance_id()) : ObjectID();
    for (const TypedArray<Effect> *list : {&effects_on_play_fx, &effects_on_stop_fx})
    {
        for (int64_t i = 0; i < list->size(); i++)
        {
            Ref<Effect> effect = (*list)[i];
            if (effect.is_valid())
            {
                effect->set_player_fx(p_player);
            }
        }
    }
}

Node *Effect::get_player_fx() const { return Object::cast_to<Node>(ObjectDB::get_instance(player_fx_id)); }

Node *Effect::_get_node_from_player(const NodePath &p_path) const
{
    Node *player = get_player_fx();
    if (!player || p_path.is_empty())
    {
        return nullptr;
    }
    return player->get_node_or_null(p_path);
}

// 🔗 Liaisons

void Effect::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("play_fx", "target"), &Effect::play_fx, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("process_fx", "delta", "target"), &Effect::process_fx, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("physics_process_fx", "delta", "target"), &Effect::physics_process_fx, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("stop_fx", "target"), &Effect::stop_fx, DEFVAL(Variant()));
    ClassDB::bind_method(D_METHOD("is_activated"), &Effect::is_activated);
    ClassDB::bind_method(D_METHOD("set_effects_on_play_fx", "effects"), &Effect::set_effects_on_play_fx);
    ClassDB::bind_method(D_METHOD("get_effects_on_play_fx"), &Effect::get_effects_on_play_fx);
    ClassDB::bind_method(D_METHOD("set_effects_on_stop_fx", "effects"), &Effect::set_effects_on_stop_fx);
    ClassDB::bind_method(D_METHOD("get_effects_on_stop_fx"), &Effect::get_effects_on_stop_fx);
    ClassDB::bind_method(D_METHOD("set_player_fx", "player"), &Effect::set_player_fx);
    ClassDB::bind_method(D_METHOD("get_player_fx"), &Effect::get_player_fx);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "effects_on_play_fx", PROPERTY_HINT_ARRAY_TYPE, "Effect"), "set_effects_on_play_fx", "get_effects_on_play_fx");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "effects_on_stop_fx", PROPERTY_HINT_ARRAY_TYPE, "Effect"), "set_effects_on_stop_fx", "get_effects_on_stop_fx");

    ADD_SIGNAL(MethodInfo("fx_played"));
    ADD_SIGNAL(MethodInfo("fx_stopped"));

    GDVIRTUAL_BIND(_play_fx, "target");
    GDVIRTUAL_BIND(_process_fx, "delta", "target");
    GDVIRTUAL_BIND(_physics_process_fx, "delta", "target");
    GDVIRTUAL_BIND(_stop_fx, "target");
}
