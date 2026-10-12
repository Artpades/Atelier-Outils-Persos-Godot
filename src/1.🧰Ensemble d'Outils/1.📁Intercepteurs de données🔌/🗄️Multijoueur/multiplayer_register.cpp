#include "🌐Session/network_session.hpp"
#include "📡Découverte/lan_discovery.hpp"
#include "🔐Chiffrement/secure_channel.hpp"
#include "🚪Accès/server_access_policy.hpp"
#include "🌍Internet/port_forwarder.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;

// "static" : ces fonctions restent privées à cette bibliothèque (pas de conflit avec register.cpp).
static void initialize(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return; // appelé aussi au niveau EDITOR : les classes ne doivent être enregistrées qu'une fois
    }

    GDREGISTER_CLASS(SecureChannel); // @icon res://🖼️Editor Icons/💾🔐/EncryptionKey Icon.png
    GDREGISTER_CLASS(LanDiscovery); // @icon res://🖼️Editor Icons/🎮🔌/LanDiscovery Icon.png
    GDREGISTER_CLASS(NetworkSession); // @icon res://🖼️Editor Icons/🎮🔌/NetworkSession Icon.png
    GDREGISTER_CLASS(ServerAccessPolicy); // @icon res://🖼️Editor Icons/🎮🔌/ServerAccessPolicy Icon.png
    GDREGISTER_CLASS(PortForwarder); // @icon res://🖼️Editor Icons/🎮🔌/PortForwarder Icon.png

    godot::print_line_rich(U"[color=dark_green]🗄️Module Multijoueur chargé avec succès ![/color]");
}

static void uninitialize(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::print_line_rich(U"[color=dark_red]🗄️Module Multijoueur déchargé avec succès ![/color]");
}

extern "C"
{
    GDExtensionBool GDE_EXPORT multiplayer_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize);
        init_obj.register_terminator(uninitialize);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
