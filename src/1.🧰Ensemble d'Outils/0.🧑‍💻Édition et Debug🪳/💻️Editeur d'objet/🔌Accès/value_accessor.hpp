#ifndef VALUE_ACCESSOR_HPP
#define VALUE_ACCESSOR_HPP

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot
{
/// Accès à une valeur d'un objet (port de ValueProxy C#) : objet racine (ObjectID) + propriété racine
/// + chemin (index d'Array / de Packed*Array, clé de Dictionary).
///  - read() descend le chemin depuis la valeur de la propriété racine ;
///  - write() reconstruit la racine (copie à chaque niveau du chemin, la valeur d'origine n'est jamais
///    modifiée en place) puis fait UN set() sur la propriété racine. Obligatoire pour les types copiés à
///    la lecture (Packed*Array, structures) ; pour Array/Dictionary (références), la copie évite de modifier
///    une collection partagée et garantit que le setter est appelé.
/// Les annulations portent donc toujours sur la propriété racine (voir UndoBackend).
class ValueAccessor : public RefCounted
{
    GDCLASS(ValueAccessor, RefCounted)

  private:
    ObjectID target_id;
    StringName property;
    Array path;               // clés successives depuis la valeur racine
    Dictionary property_info; // description de la valeur pointée (type, hint, hint_string, usage, class_name)
    String display_name;      // libellé (vide : nom de la propriété)
    bool undoable = true;     // false : écriture directe, hors historique (ex: arguments d'une méthode)

    static Variant _with_value(const Variant &p_container, const Array &p_path, int p_index, const Variant &p_value);

  protected:
    static void _bind_methods();

  public:
    /// Recherche la description de p_property dans p_target->get_property_list() (dictionnaire vide si absente).
    static Dictionary find_property_info(Object *p_target, const StringName &p_property);
    /// Copie superficielle d'une collection (Array, Dictionary, Packed*Array) ; les autres types sont déjà des copies.
    static Variant copy_collection(const Variant &p_value);

    void setup(Object *p_target, const StringName &p_property);
    /// Accès à un élément de la valeur pointée (index ou clé), décrit par p_info.
    Ref<ValueAccessor> child(const Variant &p_key, const Dictionary &p_info, const String &p_display_name) const;
    /// Même valeur, autre description (ex: Variant affiché avec le widget du type choisi).
    Ref<ValueAccessor> with_info(const Dictionary &p_info) const;

    /// Objet cible, ou nullptr s'il a été libéré.
    Object *get_target() const;
    bool is_valid() const;
    StringName get_property() const;
    Array get_path() const;
    bool is_root() const;
    Dictionary get_property_info() const;
    void set_property_info(const Dictionary &p_info);
    String get_display_name() const;
    void set_display_name(const String &p_name);
    bool is_read_only() const;
    bool is_undoable() const;
    void set_undoable(bool p_undoable);

    Variant read_root() const;
    Variant read() const;
    /// Écrit la valeur pointée (via la propriété racine) ; une Resource émet "changed".
    void write(const Variant &p_value);
};
} // namespace godot

#endif // VALUE_ACCESSOR_HPP
