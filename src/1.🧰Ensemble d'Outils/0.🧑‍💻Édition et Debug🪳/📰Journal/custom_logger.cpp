#include "custom_logger.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;

void CustomLogger::log(const String &class_name, const String &icon_path, const String &custom_message)
{
    RichTextLabel *label = get_output();
    if (label)
    {
        String icon = icon_path.is_empty() ? String() : " [img]" + icon_path + "[/img]";
        label->append_text(class_name + icon + " : " + custom_message + "\n");
        return;
    }

    // La console de l'éditeur ne gère pas la balise [img].
    print_line_rich("[b]" + class_name + "[/b] : " + custom_message);
}

void CustomLogger::set_output(RichTextLabel *label)
{
    output_id = label ? ObjectID(label->get_instance_id()) : ObjectID();
}

RichTextLabel *CustomLogger::get_output()
{
    if (output_id.is_null())
    {
        return nullptr;
    }
    return Object::cast_to<RichTextLabel>(ObjectDB::get_instance(output_id));
}

void CustomLogger::_bind_methods()
{
    ClassDB::bind_static_method("CustomLogger",
                                D_METHOD("log", "class_name", "icon_path", "custom_message"),
                                &CustomLogger::log,
                                DEFVAL(String(U"[Message non spécifié !]")));
    ClassDB::bind_static_method("CustomLogger", D_METHOD("set_output", "label"), &CustomLogger::set_output);
    ClassDB::bind_static_method("CustomLogger", D_METHOD("get_output"), &CustomLogger::get_output);
}
