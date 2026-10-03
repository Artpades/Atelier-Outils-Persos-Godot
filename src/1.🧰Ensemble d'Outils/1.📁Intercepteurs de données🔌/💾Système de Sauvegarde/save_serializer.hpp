#ifndef SAVE_SERIALIZER_HPP
#define SAVE_SERIALIZER_HPP

#include "save_data.hpp"

#include <godot_cpp/classes/object.hpp>

namespace godot
{
/// 🧾 Sérialise et désérialise des SaveData en JSON (clés "system_id" et "data").
class SaveSerializer : public Object
{
    GDCLASS(SaveSerializer, Object)

  protected:
    static void _bind_methods();

  public:
    static String serialize(const Ref<SaveData> &p_save_data);
    /// Retourne un SaveData nul si le JSON est invalide ou n'est pas un dictionnaire.
    static Ref<SaveData> deserialize(const String &p_json);
};
} // namespace godot

#endif // SAVE_SERIALIZER_HPP