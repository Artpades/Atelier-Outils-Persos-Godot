#ifndef MANAGER_BUILDER_HPP
#define MANAGER_BUILDER_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot
{

// ManagerBuilder
//
// Portage C++/GDExtension du ManagerBuilder GDScript/C#, adapté sur deux points
// suite aux décisions prises avec l'utilisateur (voir échange) :
//
// 1. Pas de recherche par nom dans l'arbre de scène (find_child). Le script GDScript
//    d'origine cherchait un nœud nommé "TypeName" alors que le nœud créé était nommé
//    "TypeName (Singleton)" : la recherche ne pouvait donc jamais retrouver l'instance
//    déjà créée. Ici, un registre statique StringName -> ObjectID tient lieu de source
//    de vérité ; aucune recherche par nom n'est faite.
//
// 2. Uniquement des classes natives C++ enregistrées dans ClassDB (pas de Script ni de
//    PackedScene) : ce module gère des singletons natifs du framework, pas des scripts
//    chargés dynamiquement.
//
// Le registre stocke des ObjectID (pas des Node*) : un Node* brut resterait "pendant"
// (dangling) si le singleton était libéré (queue_free/memdelete) sans repasser par
// unregister_instance(). La résolution passe systématiquement par
// UtilityFunctions::instance_from_id(), qui retourne un Variant nul si l'objet n'existe
// plus — donc jamais de pointeur invalide déréférencé silencieusement.
class ManagerBuilder : public RefCounted
{
    GDCLASS(ManagerBuilder, RefCounted)

  private:
    // Registre statique partagé par tout le projet : un singleton par nom de classe native.
    static HashMap<StringName, ObjectID> &_get_registry();

    // Résout une entrée du registre en Node* valide, ou nullptr si absente/périmée.
    // Si l'entrée est périmée (objet détruit sans désenregistrement), elle est retirée.
    static Node *_resolve(const StringName &p_class_name);

  protected:
    static void _bind_methods();

  public:
    ManagerBuilder();
    ~ManagerBuilder();

    // Retourne le singleton existant pour p_class_name, ou en crée un nouveau via
    // ClassDB::instantiate(p_class_name) si aucune instance valide n'est enregistrée.
    // p_class_name doit être une classe native dérivant de Node, enregistrée dans ClassDB
    // (moteur, ce module ou un autre module GDExtension). Les classes GDScript `class_name`
    // ne sont PAS dans ClassDB : elles sont refusées (cf. point 2 ci-dessus).
    // Le nœud créé est nommé "p_class_name (Singleton)" et ajouté en différé (call_deferred)
    // sous la racine du SceneTree pour survivre aux changements de scène.
    // Retourne nullptr si : pas de SceneTree actif, classe inexistante/non instanciable,
    // ou classe existante mais ne dérivant pas de Node.
    static Node *make_getter(const StringName &p_class_name);

    // À appeler dans _enter_tree() ou _ready() du manager concerné, avec son propre nom
    // de classe et lui-même comme p_caller.
    // - Si aucune instance n'est encore enregistrée pour p_class_name : p_caller devient
    //   l'instance canonique (enregistrée dans le registre) et est retourné.
    // - Si une instance valide existe déjà et que ce n'est pas p_caller : p_caller est
    //   détruit (queue_free) et l'instance existante est retournée.
    static Node *init_instance(const StringName &p_class_name, Node *p_caller);

    // À appeler dans _exit_tree() du manager (ou avant sa destruction explicite) si son
    // cycle de vie ne se termine pas par la destruction naturelle du nœud, afin d'éviter
    // qu'une future résolution retombe sur une entrée périmée avant sa purge automatique.
    // Ne retire l'entrée que si elle correspond bien à p_caller (pas d'écrasement d'un
    // autre singleton enregistré entre-temps sous le même nom).
    static void unregister_instance(const StringName &p_class_name, Node *p_caller);
};

} // namespace godot

#endif // MANAGER_BUILDER_HPP