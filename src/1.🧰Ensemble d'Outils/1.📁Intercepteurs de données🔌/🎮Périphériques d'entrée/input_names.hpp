#ifndef INPUT_NAMES_HPP
#define INPUT_NAMES_HPP

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot
{
/// 🏷️ Noms lisibles des touches / boutons / axes, et chaînes d'indice PROPERTY_HINT_ENUM correspondantes.
/// godot-cpp n'expose pas les énumérations globales (Key, JoyButton...) comme des types d'export : sans ces
/// indices, l'inspecteur n'afficherait qu'un entier.
namespace InputNames
{
/// Icônes du module, pour CustomLogger.
inline constexpr const char32_t *ICON_ACTION = U"res://🖼️Editor Icons/🎮🔌/InputAction Icon.png";
inline constexpr const char32_t *ICON_PLAYER = U"res://🖼️Editor Icons/🎮🔌/InputManager Icon.png";
inline constexpr const char32_t *ICON_REBINDER = U"res://🖼️Editor Icons/🎮🔌/InputMapBinding Icon.png";
inline constexpr const char32_t *ICON_UI = U"res://🖼️Editor Icons/🎮🔌/ButtonAssigned Icon.png";

String key(Key p_key);
String mouse_button(MouseButton p_button);
String joy_button(JoyButton p_button);
String joy_axis(JoyAxis p_axis);

/// Toutes les touches connues du moteur (« Nom:valeur,... »), calculé une seule fois.
String key_hint();
String mouse_button_hint();
String joy_button_hint();
String joy_axis_hint();
} // namespace InputNames
} // namespace godot

#endif // INPUT_NAMES_HPP
