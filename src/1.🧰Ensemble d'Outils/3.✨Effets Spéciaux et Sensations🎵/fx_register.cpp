#include "effect.hpp"
#include "player_fx.hpp"

#include "〰️Follow/fx_follower.hpp"
#include "⏯️Ralentissement-Arrêt temporaire du temps (HitStop, Slow Motion)/time_manipulator_fx.hpp"
#include "🌻Ciblage d'objet/fx_targeter.hpp"
#include "💥Secousse/fx_shake.hpp"
#include "🔊Effet joueur de son/fx_audio.hpp"
#include "🦣🫘Agrandissement/Rétrécissement/fx_change_size.hpp"
#include "🪗Squash and Stretch/fx_stretch_and_squash.hpp"

#include "👆Changeur du visuel du curseur/cursor_changer.hpp"
#include "👆Changeur du visuel du curseur/cursor_style.hpp"
#include "🌈Fondu Couleur/Texture/fx_fade.hpp"
#include "🌈Fondu Couleur/Texture/screen_fader.hpp"
#include "💥Pop de Texte-d'image-d'objets/fx_pop.hpp"
#include "💥Pop de Texte-d'image-d'objets/pop_instance.hpp"
#include "💥Pop de Texte-d'image-d'objets/pop_spawner.hpp"
#include "💥Pop de Texte-d'image-d'objets/pop_style.hpp"
#include "🔁Music Looper/music_looper.hpp"
#include "🗨️Infobulle Personnalisable/custom_tooltip.hpp"
#include "🗨️Infobulle Personnalisable/tooltip_content.hpp"
#include "🛟Floating Dock/floating_dock.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize_fx(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : la classe ne doit être enregistrée qu'une fois
    }

    // ✨ Socle : une classe parente avant ses classes filles.
    GDREGISTER_CLASS(Effect);   // @icon res://🖼️Editor Icons/✨🎵/Fancy UI Icon.png
    GDREGISTER_CLASS(PlayerFX); // @icon res://🖼️Editor Icons/✨🎵/Fancy UI Icon.png

    // ✨ Effets
    GDREGISTER_CLASS(FXStretchAndSquash); // @icon res://🖼️Editor Icons/✨🎵/Squash And Stretch Icon.png
    GDREGISTER_CLASS(FXChangeSize);       // @icon res://🖼️Editor Icons/✨🎵/Tween Player icon.png
    GDREGISTER_CLASS(FXShake);            // @icon res://🖼️Editor Icons/✨🎵/Shake FX Icon.png
    GDREGISTER_CLASS(FXTargeter);         // @icon res://🖼️Editor Icons/✨🎵/Targeter Icon.png
    GDREGISTER_CLASS(FXFollower);         // @icon res://🖼️Editor Icons/✨🎵/Follower FX Icon.png
    GDREGISTER_CLASS(FXAudio);            // @icon res://🖼️Editor Icons/✨🎵/Fx Audio icon.png
    GDREGISTER_CLASS(FXFade);
    GDREGISTER_CLASS(FXPop);

    // 🌈 Fondu de l'écran · 🛟 Dock · 🔁 Musique
    GDREGISTER_CLASS(ScreenFader);
    GDREGISTER_CLASS(FloatingDock);
    GDREGISTER_CLASS(MusicLooper); // @icon res://🖼️Editor Icons/✨🎵/MusicManager.png

    // 💥 Pops : les instances sont créées par PopSpawner / FXPop uniquement (classes internes).
    GDREGISTER_CLASS(PopStyle);
    GDREGISTER_CLASS(PopSpawner);
    GDREGISTER_INTERNAL_CLASS(PopInstance2D);
    GDREGISTER_INTERNAL_CLASS(PopInstance3D);

    // Ressources de l'infobulle et du curseur
    GDREGISTER_CLASS(TooltipContent);
    GDREGISTER_CLASS(CursorStyle);

    // Classes « runtime » : n'agissent pas dans l'éditeur (vitesse du jeu, curseur, infobulles de la scène éditée).
    GDREGISTER_RUNTIME_CLASS(TimeManipulatorFX); // @icon res://🖼️Editor Icons/✨🎵/TimeManipulator FX Icon.png
    GDREGISTER_RUNTIME_CLASS(CustomTooltip);
    GDREGISTER_RUNTIME_CLASS(CursorChanger);

    godot::print_line_rich(U"[color=dark_green]✨Module d'effets spéciaux chargé avec succès ![/color]");
}

static void uninitialize_fx(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]✨Module d'effets spéciaux déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT fx_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_fx);
        init_obj.register_terminator(uninitialize_fx);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
