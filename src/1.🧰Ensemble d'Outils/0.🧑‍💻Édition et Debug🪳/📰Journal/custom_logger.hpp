#ifndef CUSTOM_LOGGER_HPP
#define CUSTOM_LOGGER_HPP

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot
{
// 📰Journal : affiche du texte détaillé (classe + icône + message) dans un RichTextLabel de sortie,
// ou dans la console Godot si aucune sortie n'est définie.
class CustomLogger : public Object
{
    GDCLASS(CustomLogger, Object)

  private:
    // ObjectID plutôt qu'un pointeur : le label peut être libéré sans prévenir le logger.
    inline static ObjectID output_id;

  protected:
    static void _bind_methods();

  public:
    static void log(const String &class_name, const String &icon_path, const String &custom_message = String(U"[Message non spécifié !]"));

    static void set_output(RichTextLabel *label);
    static RichTextLabel *get_output();
};
} // namespace godot

#endif // CUSTOM_LOGGER_HPP
