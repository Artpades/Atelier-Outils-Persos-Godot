#include "input_names.hpp"

#include <godot_cpp/classes/os.hpp>

using namespace godot;

namespace
{
const char *MOUSE_BUTTON_NAMES[] = {"Aucun", "Gauche", "Droit", "Milieu", "Molette haut", "Molette bas", "Molette gauche", "Molette droite", "Extra 1", "Extra 2"};

const char *JOY_BUTTON_NAMES[] = {"A", "B", "X", "Y", "Back", "Guide", "Start", "Stick gauche", "Stick droit", "Gâchette haute gauche", "Gâchette haute droite", "Croix haut", "Croix bas",
                                  "Croix gauche", "Croix droite", "Misc 1", "Palette 1", "Palette 2", "Palette 3", "Palette 4", "Pavé tactile"};

const char *JOY_AXIS_NAMES[] = {"Stick gauche X", "Stick gauche Y", "Stick droit X", "Stick droit Y", "Gâchette gauche", "Gâchette droite"};

template <size_t N> String name_or_number(const char *(&p_names)[N], int p_index)
{
    if (p_index >= 0 && p_index < (int)N)
    {
        return String::utf8(p_names[p_index]);
    }
    return itos(p_index);
}

template <size_t N> String hint_from(const char *(&p_names)[N], int p_first_value, const String &p_prefix)
{
    String hint = p_prefix;
    for (size_t i = 0; i < N; i++)
    {
        if (!hint.is_empty())
        {
            hint += ",";
        }
        hint += String::utf8(p_names[i]) + ":" + itos(p_first_value + (int)i);
    }
    return hint;
}
} // namespace

String InputNames::key(Key p_key)
{
    if (p_key == KEY_NONE)
    {
        return String::utf8("Aucune");
    }
    return OS::get_singleton()->get_keycode_string(p_key);
}

String InputNames::mouse_button(MouseButton p_button) { return name_or_number(MOUSE_BUTTON_NAMES, (int)p_button); }

String InputNames::joy_button(JoyButton p_button)
{
    if (p_button == JOY_BUTTON_INVALID)
    {
        return String::utf8("Aucun");
    }
    return name_or_number(JOY_BUTTON_NAMES, (int)p_button);
}

String InputNames::joy_axis(JoyAxis p_axis)
{
    if (p_axis == JOY_AXIS_INVALID)
    {
        return String::utf8("Aucun");
    }
    return name_or_number(JOY_AXIS_NAMES, (int)p_axis);
}

String InputNames::key_hint()
{
    static String hint;
    if (!hint.is_empty())
    {
        return hint;
    }

    OS *os = OS::get_singleton();
    hint = "Aucune:0";
    auto add_if_named = [&](int64_t p_code) {
        String name = os->get_keycode_string((Key)p_code);
        // L'aller-retour élimine les codes sans nom propre (le moteur renvoie alors un caractère quelconque).
        if (name.is_empty() || name.contains(",") || name.contains(":") || os->find_keycode_from_string(name) != (Key)p_code)
        {
            return;
        }
        hint += "," + name + ":" + itos(p_code);
    };

    // Touches spéciales (Échap, flèches, F1...) : listées une à une, car interroger un code spécial inexistant
    // fait produire au moteur un caractère Unicode invalide (et une erreur dans la console).
    static const Key SPECIAL_KEYS[] = {
        KEY_ESCAPE,      KEY_TAB,       KEY_BACKTAB,    KEY_BACKSPACE,      KEY_ENTER,       KEY_KP_ENTER,   KEY_INSERT,     KEY_DELETE,     KEY_PAUSE,      KEY_PRINT,
        KEY_SYSREQ,      KEY_CLEAR,     KEY_HOME,       KEY_END,            KEY_LEFT,        KEY_UP,         KEY_RIGHT,      KEY_DOWN,       KEY_PAGEUP,     KEY_PAGEDOWN,
        KEY_SHIFT,       KEY_CTRL,      KEY_META,       KEY_ALT,            KEY_CAPSLOCK,    KEY_NUMLOCK,    KEY_SCROLLLOCK, KEY_F1,         KEY_F2,         KEY_F3,
        KEY_F4,          KEY_F5,        KEY_F6,         KEY_F7,             KEY_F8,          KEY_F9,         KEY_F10,        KEY_F11,        KEY_F12,        KEY_F13,
        KEY_F14,         KEY_F15,       KEY_F16,        KEY_F17,            KEY_F18,         KEY_F19,        KEY_F20,        KEY_F21,        KEY_F22,        KEY_F23,
        KEY_F24,         KEY_F25,       KEY_F26,        KEY_F27,            KEY_F28,         KEY_F29,        KEY_F30,        KEY_F31,        KEY_F32,        KEY_F33,
        KEY_F34,         KEY_F35,       KEY_KP_MULTIPLY, KEY_KP_DIVIDE,     KEY_KP_SUBTRACT, KEY_KP_PERIOD,  KEY_KP_ADD,     KEY_KP_0,       KEY_KP_1,       KEY_KP_2,
        KEY_KP_3,        KEY_KP_4,      KEY_KP_5,       KEY_KP_6,           KEY_KP_7,        KEY_KP_8,       KEY_KP_9,       KEY_MENU,       KEY_HYPER,      KEY_HELP,
        KEY_BACK,        KEY_FORWARD,   KEY_STOP,       KEY_REFRESH,        KEY_VOLUMEDOWN,  KEY_VOLUMEMUTE, KEY_VOLUMEUP,   KEY_MEDIAPLAY,  KEY_MEDIASTOP,  KEY_MEDIAPREVIOUS,
        KEY_MEDIANEXT,   KEY_MEDIARECORD, KEY_HOMEPAGE, KEY_FAVORITES,      KEY_SEARCH,      KEY_STANDBY,    KEY_OPENURL,    KEY_LAUNCHMAIL, KEY_LAUNCHMEDIA, KEY_LAUNCH0,
        KEY_LAUNCH1,     KEY_LAUNCH2,   KEY_LAUNCH3,    KEY_LAUNCH4,        KEY_LAUNCH5,     KEY_LAUNCH6,    KEY_LAUNCH7,    KEY_LAUNCH8,    KEY_LAUNCH9,    KEY_LAUNCHA,
        KEY_LAUNCHB,     KEY_LAUNCHC,   KEY_LAUNCHD,    KEY_LAUNCHE,        KEY_LAUNCHF,     KEY_GLOBE,      KEY_KEYBOARD,   KEY_JIS_EISU,   KEY_JIS_KANA,
    };
    for (Key code : SPECIAL_KEYS)
    {
        add_if_named(code);
    }
    // Caractères imprimables : Key n'utilise que les majuscules, les minuscules 'a'..'z' n'existent pas.
    for (int64_t code = 32; code < 256; code++)
    {
        if (code >= 'a' && code <= 'z')
        {
            continue;
        }
        add_if_named(code);
    }
    return hint;
}

String InputNames::mouse_button_hint() { return hint_from(MOUSE_BUTTON_NAMES, 0, String()); }

String InputNames::joy_button_hint() { return hint_from(JOY_BUTTON_NAMES, 0, String::utf8("Aucun:-1")); }

String InputNames::joy_axis_hint() { return hint_from(JOY_AXIS_NAMES, 0, String::utf8("Aucun:-1")); }
