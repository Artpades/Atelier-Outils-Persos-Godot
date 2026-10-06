#include "🧩Commun/movement_component.hpp"
#include "🦶Déplacement au sol d'objet/ground_movement.hpp"
#include "🌬️Recule/knockback.hpp"
#include "🧲Zone de force/force_zone.hpp"
#include "🌊Déplacement Aquatique/water.hpp"
#include "🧗Déplacement Mural/wall_movement.hpp"
#include "🪂Physique Aérodynamique/aerodynamic_movement.hpp"
#include "🫨Physique Trémousse/jiggle.hpp"
#include "🪢Physique de Corde/rope.hpp"
#include "🎬Orchestrateur de mouvement/movement_director.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_movement_physics(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }

    // Ordre : une classe parente avant ses classes filles.
    GDREGISTER_VIRTUAL_CLASS(MovementComponent); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png

    // 🦶 Déplacement au sol d'objet
    GDREGISTER_CLASS(GroundMovement2D); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png
    GDREGISTER_CLASS(GroundMovement3D); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png

    // 🌬️ Recul
    GDREGISTER_CLASS(Knockback2D); // @icon res://🖼️Editor Icons/🌀🫗/RecoilCharacter Icon.png
    GDREGISTER_CLASS(Knockback3D); // @icon res://🖼️Editor Icons/🌀🫗/RecoilCharacter Icon.png

    // 🧲 Zones de force
    GDREGISTER_CLASS(ForceZone2D); // @icon res://🖼️Editor Icons/🌀🫗/RecoilPhysics Icon.png
    GDREGISTER_CLASS(ForceZone3D); // @icon res://🖼️Editor Icons/🌀🫗/RecoilPhysics Icon.png

    // 🪂 Physique aérodynamique
    GDREGISTER_CLASS(AerodynamicMovement2D); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png
    GDREGISTER_CLASS(AerodynamicMovement3D); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png

    // 🌊 Déplacement aquatique
    GDREGISTER_CLASS(WaterArea2D);    // @icon res://🖼️Editor Icons/🌀🫗/RigidBodyMovement.png
    GDREGISTER_CLASS(WaterArea3D);    // @icon res://🖼️Editor Icons/🌀🫗/RigidBodyMovement.png
    GDREGISTER_CLASS(SwimMovement2D); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png
    GDREGISTER_CLASS(SwimMovement3D); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png

    // 🧗 Déplacement mural
    GDREGISTER_CLASS(WallMovement2D); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png
    GDREGISTER_CLASS(WallMovement3D); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png

    // 🫨 Physique trémousse
    GDREGISTER_CLASS(Jiggle2D); // @icon res://🖼️Editor Icons/🌀🫗/Recoil Icon.png
    GDREGISTER_CLASS(Jiggle3D); // @icon res://🖼️Editor Icons/🌀🫗/Recoil Icon.png

    // 🪢 Physique de corde
    GDREGISTER_CLASS(Rope2D);         // @icon res://🖼️Editor Icons/🌀🫗/RigidBodyMovement.png
    GDREGISTER_CLASS(Rope3D);         // @icon res://🖼️Editor Icons/🌀🫗/RigidBodyMovement.png
    GDREGISTER_CLASS(RopeMovement2D); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png
    GDREGISTER_CLASS(RopeMovement3D); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png

    // 🎬 Orchestrateur
    GDREGISTER_CLASS(MovementContext);  // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png
    GDREGISTER_CLASS(MovementDirector); // @icon res://🖼️Editor Icons/🌀🫗/CharacterMovement.png

    godot::print_line_rich(U"[color=dark_green]🌀Module Mouvement & Physique chargé avec succès ![/color]");
}

static void uninitialize_movement_physics(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]🌀Module Mouvement & Physique déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT movement_physics_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_movement_physics);
        init_obj.register_terminator(uninitialize_movement_physics);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
