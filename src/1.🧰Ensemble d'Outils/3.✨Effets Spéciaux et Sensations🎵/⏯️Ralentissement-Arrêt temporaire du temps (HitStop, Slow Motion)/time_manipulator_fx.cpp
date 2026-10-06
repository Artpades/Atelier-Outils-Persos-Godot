#include "time_manipulator_fx.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void TimeManipulatorFX::_ready()
{
    // Le jeu commence au temps normal
    Engine::get_singleton()->set_time_scale(1.0);
    target_time_scale = 1.0;
    last_ticks_usec = Time::get_singleton()->get_ticks_usec();
}

void TimeManipulatorFX::_notification(int p_what)
{
    if (p_what == NOTIFICATION_EXIT_TREE)
    {
        // Sans ce nœud, plus personne ne ramènerait le jeu à sa vitesse normale
        hit_stopping = false;
        Engine::get_singleton()->set_time_scale(1.0);
    }
}

void TimeManipulatorFX::_process(double /*p_delta*/)
{
    // Temps réel écoulé : le delta de _process est lui-même multiplié par time_scale
    const uint64_t now = Time::get_singleton()->get_ticks_usec();
    const double real_delta = (now - last_ticks_usec) / 1000000.0;
    last_ticks_usec = now;

    // Pendant un HitStop, le temps ne s'interpole pas
    if (hit_stopping)
    {
        return;
    }

    Engine *engine = Engine::get_singleton();
    const double current = engine->get_time_scale();
    if (Math::is_equal_approx(current, target_time_scale))
    {
        return;
    }

    double next = Math::lerp(current, target_time_scale, Math::min(recovery_speed * real_delta, 1.0));
    // Le lerp n'atteint jamais exactement la cible
    if (Math::abs(next - target_time_scale) < 0.01)
    {
        next = target_time_scale;
    }
    engine->set_time_scale(next);
}

// 🎯 API publique

void TimeManipulatorFX::set_time_scale(double p_target_scale) { target_time_scale = Math::max(0.0, p_target_scale); }

void TimeManipulatorFX::trigger_hit_stop(double p_duration, double p_scale)
{
    // Évite de superposer plusieurs HitStops de manière chaotique
    if (hit_stopping || !is_inside_tree())
    {
        return;
    }

    hit_stopping = true;
    scale_before_hit_stop = target_time_scale; // vitesse désirée d'avant le HitStop
    Engine::get_singleton()->set_time_scale(Math::max(0.0, p_scale));
    emit_signal("hit_stop_started");

    // Minuteur qui ignore time_scale (sinon il ne s'écoulerait jamais à 0) et la pause
    Ref<SceneTreeTimer> timer = get_tree()->create_timer(p_duration, true, false, true);
    timer->connect("timeout", callable_mp(this, &TimeManipulatorFX::_on_hit_stop_finished));
}

void TimeManipulatorFX::_on_hit_stop_finished()
{
    if (!hit_stopping)
    {
        return; // nœud sorti de l'arbre entre-temps : vitesse déjà restaurée
    }
    target_time_scale = scale_before_hit_stop;
    Engine::get_singleton()->set_time_scale(scale_before_hit_stop);
    hit_stopping = false;
    last_ticks_usec = Time::get_singleton()->get_ticks_usec();
    emit_signal("hit_stop_finished");
}

// 🔗 Liaisons

void TimeManipulatorFX::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_time_scale", "target_scale"), &TimeManipulatorFX::set_time_scale);
    ClassDB::bind_method(D_METHOD("trigger_hit_stop", "duration", "scale"), &TimeManipulatorFX::trigger_hit_stop, DEFVAL(0.0));
    ClassDB::bind_method(D_METHOD("is_hit_stopping"), &TimeManipulatorFX::is_hit_stopping);
    ClassDB::bind_method(D_METHOD("set_recovery_speed", "value"), &TimeManipulatorFX::set_recovery_speed);
    ClassDB::bind_method(D_METHOD("get_recovery_speed"), &TimeManipulatorFX::get_recovery_speed);
    ClassDB::bind_method(D_METHOD("set_target_time_scale", "value"), &TimeManipulatorFX::set_target_time_scale);
    ClassDB::bind_method(D_METHOD("get_target_time_scale"), &TimeManipulatorFX::get_target_time_scale);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "recovery_speed", PROPERTY_HINT_RANGE, "0,50,0.1,or_greater"), "set_recovery_speed", "get_recovery_speed");
    // Non enregistrée : _ready la remet à 1
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "target_time_scale", PROPERTY_HINT_RANGE, "0,4,0.01,or_greater", PROPERTY_USAGE_EDITOR), "set_target_time_scale", "get_target_time_scale");

    ADD_SIGNAL(MethodInfo("hit_stop_started"));
    ADD_SIGNAL(MethodInfo("hit_stop_finished"));
}
