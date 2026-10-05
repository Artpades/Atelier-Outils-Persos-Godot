#ifndef SIGNAL_LINK_HPP
#define SIGNAL_LINK_HPP

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
// Source de la valeur d'un paramètre (port de SignalArgument C#, généralisé). Partagé par les liens de
// signaux (Phase 3) et les gizmos (Phase 4) : resolve(contexte, arguments du signal).
//  - CONSTANT : valeur Variant. Un objet doit être une Resource (sérialisable) : un nœud est refusé
//    (utiliser PROPERTY avec son chemin).
//  - PROPERTY : objet (object_resource, sinon NodePath object_path relatif au contexte ; vide = le contexte)
//    + chemin de propriété property_path ("position", "position:x"...), lu avec get_indexed.
//  - SIGNAL_ARG : argument signal_index du signal.
//  - METHOD : même objet que PROPERTY ; valeur de retour de la méthode method, appelée sans argument.
class ArgumentBinding : public Resource
{
    GDCLASS(ArgumentBinding, Resource)

  public:
    // Valeurs enregistrées dans les .tres : jamais renumérotées.
    enum Source
    {
        SOURCE_CONSTANT = 0,
        SOURCE_PROPERTY = 1,
        SOURCE_SIGNAL_ARG = 2,
        SOURCE_METHOD = 3,
    };

  private:
    Source source = SOURCE_CONSTANT;
    Variant constant;
    NodePath object_path;
    Ref<Resource> object_resource;
    NodePath property_path;
    StringName method;
    int signal_index = 0;
    // Description du paramètre alimenté (type, hint, hint_string, class_name) : la constante est alors
    // présentée avec ce type (inspecteur Godot, ProceduralEditor) au lieu d'un Variant libre.
    Dictionary expected_info;

  protected:
    static void _bind_methods();
    void _validate_property(PropertyInfo &p_property) const;

  public:
    // Message d'erreur si p_value ne peut pas être enregistrée comme constante (vide si elle le peut).
    static String constant_error(const Variant &p_value);
    // Objet de base pour un chemin relatif quand le contexte n'est pas un nœud : scène courante.
    static Node *fallback_base_node();
    // Membres de p_object utilisables comme source p_source (PROPERTY ou METHOD) pour un paramètre décrit
    // par p_expected (type, class_name) : [{name, type, class_name, const}]. Méthodes : sans argument
    // obligatoire, avec une valeur de retour compatible, publiques (pas de « _ »). « const » n'est connu que
    // pour les méthodes natives (toujours faux pour une méthode de script).
    static TypedArray<Dictionary> compatible_members(Object *p_object, Source p_source, const Dictionary &p_expected);
    // Vrai si la méthode est déclarée const (classes natives uniquement).
    static bool is_method_const(Object *p_object, const StringName &p_method);

    void set_source(Source p_source);
    Source get_source() const;
    void set_constant(const Variant &p_value);
    Variant get_constant() const;
    void set_object_path(const NodePath &p_path);
    NodePath get_object_path() const;
    void set_object_resource(const Ref<Resource> &p_resource);
    Ref<Resource> get_object_resource() const;
    void set_property_path(const NodePath &p_path);
    NodePath get_property_path() const;
    void set_method(const StringName &p_method);
    StringName get_method() const;
    void set_signal_index(int p_index);
    int get_signal_index() const;
    void set_expected_info(const Dictionary &p_info);
    Dictionary get_expected_info() const;

    Object *resolve_object(Object *p_context) const;
    Variant resolve(Object *p_context, const Array &p_signal_args) const;
};

// Lien « mode ressource » d'un signal vers une méthode (port de SignalConnection C#) : enregistré dans le
// registre (DataEntry::links), appliqué par connect_to / retiré par disconnect_from (idempotents).
// Correspondance des paramètres libre (un ArgumentBinding par paramètre), via un Callable personnalisé.
class SignalLink : public Resource
{
    GDCLASS(SignalLink, Resource)

  private:
    bool enabled = true;
    NodePath target_path; // relatif à l'émetteur (vide : l'émetteur lui-même)
    Ref<Resource> target_resource;
    StringName method;
    TypedArray<ArgumentBinding> arguments;

    // État d'exécution (non enregistré) : connexions actives, par émetteur.
    Dictionary live; // ObjectID (int) de l'émetteur -> [signal, Callable]
    bool invoking = false;

  protected:
    static void _bind_methods();

  public:
    void set_enabled(bool p_enabled);
    bool get_enabled() const;
    void set_target_path(const NodePath &p_path);
    NodePath get_target_path() const;
    void set_target_resource(const Ref<Resource> &p_resource);
    Ref<Resource> get_target_resource() const;
    void set_method(const StringName &p_method);
    StringName get_method() const;
    void set_arguments(const TypedArray<ArgumentBinding> &p_arguments);
    TypedArray<ArgumentBinding> get_arguments() const;

    // Objet appelé : target_resource, sinon le nœud à target_path (relatif à l'émetteur).
    Object *resolve_target(Object *p_source) const;

    void connect_to(Object *p_source, const StringName &p_signal);
    void disconnect_from(Object *p_source);
    bool is_connected_to(Object *p_source) const;
    // Appelé par le Callable du lien à chaque émission.
    void invoke(Object *p_source, const Array &p_signal_args);
};
} // namespace godot

VARIANT_ENUM_CAST(ArgumentBinding::Source);

#endif // SIGNAL_LINK_HPP
