#ifndef INSPECTOR_WIDGETS_HPP
#define INSPECTOR_WIDGETS_HPP

#include "inspector_core.hpp"

#include <godot_cpp/classes/container.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
/// 🏗️ Génération procédurale des widgets d'édition.
/// • Propriétés : un widget par type (booléen, nombre, texte, enum/flags, couleur, vecteurs, NodePath, objet,
///   Array, Dictionary, Variant), lié à la valeur en mémoire dans les deux sens (voir InspectorBinding).
/// • Méthodes : un bouton d'appel + un champ éditable par argument.
/// • Signaux : bouton d'émission, liste des connexions (✕ pour retirer) et ajout d'une connexion vers la méthode
///   d'un autre objet. Dans l'éditeur, les connexions sont créées avec CONNECT_PERSIST : elles sont enregistrées
///   dans la scène, comme celles de l'onglet « Nœud ».
namespace InspectorWidgets
{
enum class Scope
{
    AUTO,        ///< Membres du script s'il y en a un ; sinon propriétés complètes, méthodes/signaux de la classe elle-même.
    SCRIPT_ONLY, ///< Uniquement les membres déclarés par le script.
    ALL          ///< Tous les membres, y compris ceux hérités du moteur (très long pour un Node).
};

enum class MemberKind
{
    PROPERTY,
    METHOD,
    SIGNAL
};

struct Options
{
    Scope scope = Scope::AUTO;
    bool properties = true;
    bool methods = true;
    bool signals = true;
    int depth = 0; ///< Profondeur de récursion (objets imbriqués).
};

/// Au-delà, un objet imbriqué n'est plus déplié (protège des hiérarchies très profondes).
constexpr int MAX_DEPTH = 8;

/// Vide `p_container` puis le remplit avec l'interface de `p_target`.
void populate(Container *p_container, Object *p_target, const Options &p_options);

/// Supprime tous les enfants de `p_node`.
void clear(Node *p_node);

/// Liste les descriptions de membres d'un objet selon le périmètre.
TypedArray<Dictionary> get_members(Object *p_target, MemberKind p_kind, Scope p_scope);

/// Accès à la propriété `p_info["name"]` de `p_object`.
ValueAccessor property_accessor(Object *p_object, const Dictionary &p_info);

/// Widget d'édition d'une valeur, choisi selon son type et son indice (hint).
Control *build_value_widget(const ValueAccessor &p_accessor, int p_depth = 0);

/// Bouton d'appel d'une méthode, avec un widget par argument.
Control *build_method_widget(Object *p_object, const Dictionary &p_method_info);

/// Émission, liste et ajout des connexions d'un signal.
Control *build_signal_widget(Object *p_object, const Dictionary &p_signal_info);

/// Racine de scène : scène éditée (éditeur), sinon scène courante, sinon racine de l'arbre.
Node *find_scene_root();

/// Signale une modification : `changed` pour une Resource, scène marquée « non enregistrée » dans l'éditeur.
void notify_edited(Object *p_object);

/// Valeur « vide » raisonnable pour un type (arguments par défaut, nouveaux éléments).
Variant default_for(Variant::Type p_type);

/// Signature lisible : `heal(amount: int, source: Node)`.
String signature(const Dictionary &p_info);
} // namespace InspectorWidgets

} // namespace godot

#endif // INSPECTOR_WIDGETS_HPP
