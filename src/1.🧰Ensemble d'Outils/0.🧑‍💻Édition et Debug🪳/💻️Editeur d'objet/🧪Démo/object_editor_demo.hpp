#ifndef OBJECT_EDITOR_DEMO_HPP
#define OBJECT_EDITOR_DEMO_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot
{
/// Ressource de démonstration (sous-objet) : un Transform3D, un entier et une référence vers une autre
/// ressource du même type (permet de tester l'anti-cycle en la faisant pointer sur elle-même).
class ObjectEditorDemoResource : public Resource
{
    GDCLASS(ObjectEditorDemoResource, Resource)

  private:
    Transform3D transform;
    int64_t valeur = 1;
    Ref<Resource> autre;

  protected:
    static void _bind_methods();

  public:
    void set_transform(const Transform3D &p_transform);
    Transform3D get_transform() const;
    void set_valeur(int64_t p_valeur);
    int64_t get_valeur() const;
    void set_autre(const Ref<Resource> &p_autre);
    Ref<Resource> get_autre() const;
};

/// Classe de démonstration / de test de l'éditeur d'objet : une propriété de chaque type pris en charge
/// en Phase 1 (déclarées dynamiquement via _get_property_list, valeurs dans un Dictionary).
/// Cas particuliers :
///  - "valeur_bornee" : le setter borne la valeur entre 0 et 10 (test de NotifyWritten) ;
///  - "lecture_seule" : PROPERTY_USAGE_READ_ONLY (widget non éditable) ;
///  - "compteur" : modifié de l'extérieur (increment_counter(), ou chaque 0,5 s si "auto_increment").
class ObjectEditorDemo : public Node
{
    GDCLASS(ObjectEditorDemo, Node)

  private:
    Dictionary values;
    double elapsed = 0.0;

  protected:
    static void _bind_methods();
    bool _set(const StringName &p_name, const Variant &p_value);
    bool _get(const StringName &p_name, Variant &r_value) const;
    void _get_property_list(List<PropertyInfo> *p_list) const;

  public:
    ObjectEditorDemo();
    void _process(double p_delta) override;

    void increment_counter();
    /// Méthode à deux arguments avec valeurs par défaut (test du widget de méthode).
    String combine(int64_t p_count = 3, const String &p_text = "x") const;

    /// Signal valeur_changee(nouvelle: int, ancienne: int), émis par emettre().
    void emettre(int64_t p_nouvelle);
    /// Cible de liens de signaux : mémorise l'appel dans "dernier_appel" ("a|b|c").
    void recevoir(int64_t p_a, const String &p_b, double p_c);
};
} // namespace godot

#endif // OBJECT_EDITOR_DEMO_HPP
