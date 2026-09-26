#ifndef INSPECTOR_CORE_HPP
#define INSPECTOR_CORE_HPP

#include <functional>
#include <utility>

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/callable_custom.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot
{
/// 🪝 Callable qui exécute une fonction C++ quelconque (lambda avec capture).
/// godot-cpp ne sait pas binder une lambda avec ClassDB, mais il accepte un CallableCustom :
/// on peut donc connecter une lambda à n'importe quel signal, sans créer une classe « callback » par type.
/// `owner` : objet auquel la connexion est rattachée. S'il est libéré, Godot n'appelle plus la lambda.
class LambdaCallable : public CallableCustom
{
  public:
    using Function = std::function<Variant(const Variant **, int)>;

    LambdaCallable(Object *p_owner, Function p_function)
        : owner_id(p_owner ? ObjectID(p_owner->get_instance_id()) : ObjectID()), function(std::move(p_function))
    {
    }

    uint32_t hash() const override { return (uint32_t)(uintptr_t)this; }
    String get_as_text() const override { return "LambdaCallable"; }
    CompareEqualFunc get_compare_equal_func() const override { return &compare_equal; }
    CompareLessFunc get_compare_less_func() const override { return &compare_less; }
    bool is_valid() const override { return owner_id.is_null() || ObjectDB::get_instance(owner_id) != nullptr; }
    ObjectID get_object() const override { return owner_id; }

    int get_argument_count(bool &r_is_valid) const override
    {
        r_is_valid = false; // nombre d'arguments libre : la lambda reçoit ce que le signal envoie
        return 0;
    }

    void call(const Variant **p_arguments, int p_argcount, Variant &r_return_value, GDExtensionCallError &r_call_error) const override
    {
        r_return_value = function(p_arguments, p_argcount);
        r_call_error.error = GDEXTENSION_CALL_OK;
    }

  private:
    static bool compare_equal(const CallableCustom *p_a, const CallableCustom *p_b) { return p_a == p_b; }
    static bool compare_less(const CallableCustom *p_a, const CallableCustom *p_b) { return p_a < p_b; }

    ObjectID owner_id;
    Function function;
};

/// Lambda sans argument (ex : signal "pressed").
template <typename F> Callable lambda_callable(Object *p_owner, F p_function)
{
    // Lambda construite hors de memnew : la macro ne l'accepte pas en C++17.
    LambdaCallable::Function function = [p_function](const Variant **, int) {
        p_function();
        return Variant();
    };
    return Callable(memnew(LambdaCallable(p_owner, function)));
}

/// Lambda qui reçoit le premier argument du signal (ex : "toggled", "value_changed").
template <typename F> Callable lambda_callable_1(Object *p_owner, F p_function)
{
    LambdaCallable::Function function = [p_function](const Variant **p_args, int p_count) {
        p_function(p_count > 0 ? *p_args[0] : Variant());
        return Variant();
    };
    return Callable(memnew(LambdaCallable(p_owner, function)));
}

/// 🔌 Accès à une valeur éditable : une propriété d'objet, un élément de tableau, un argument de méthode...
/// Les widgets ne connaissent que cette interface : un même générateur sert donc pour tous ces cas
/// (équivalent C++ du ValueProxy de la version C#).
struct ValueAccessor
{
    /// Lit la valeur courante.
    std::function<Variant()> get;
    /// Écrit la valeur (et signale la modification si besoin).
    std::function<void(const Variant &)> set;
    /// Faux si la source a disparu (objet libéré...). Absent : toujours valide.
    std::function<bool()> is_alive;
    /// Description au format de get_property_list() : name, type, hint, hint_string, usage, class_name.
    Dictionary info;
    /// Libellé affiché à gauche du widget.
    String label;
    /// Objet de départ pour résoudre un NodePath (0 : racine de la scène).
    ObjectID context;

    bool alive() const { return !is_alive || is_alive(); }
    Variant::Type type() const { return (Variant::Type)(int)info.get("type", (int)Variant::NIL); }
    int hint() const { return (int)info.get("hint", 0); }
    String hint_string() const { return info.get("hint_string", ""); }
    int64_t usage() const { return (int64_t)info.get("usage", (int64_t)PROPERTY_USAGE_DEFAULT); }
    bool read_only() const { return (usage() & PROPERTY_USAGE_READ_ONLY) != 0; }
};

} // namespace godot

#endif // INSPECTOR_CORE_HPP
