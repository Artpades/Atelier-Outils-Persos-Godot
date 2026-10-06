#ifndef BEHAVIOR_EXPRESSION_HPP
#define BEHAVIOR_EXPRESSION_HPP

#include "blackboard.hpp"

#include <godot_cpp/classes/expression.hpp>
#include <godot_cpp/classes/node.hpp>

namespace godot
{
/// Évaluation des expressions des conditions et des valeurs (classe C++ interne, non enregistrée).
/// Entrées disponibles dans l'expression :
///   - chaque clé du tableau noir qui est un identifiant valide (ex. « vie < 20 ») ;
///   - « agent » (le nœud piloté) et « blackboard » (la ressource) ;
///   - les méthodes de l'agent, appelables directement (base de l'expression), ex. « is_on_floor() ».
/// L'expression analysée est mise en cache tant que le texte et la liste des clés ne changent pas.
class BehaviorExpression
{
  public:
    /// r_ok vaut false si l'expression est vide, ne s'analyse pas ou échoue à l'exécution.
    Variant evaluate(const String &p_source, const Ref<Blackboard> &p_blackboard, Node *p_agent, bool &r_ok);
    /// Mot réservé des expressions (and, or, not, in, true, false, null, self, PI, TAU, INF, NAN) :
    /// inutilisable comme nom de variable.
    static bool is_reserved_word(const String &p_name);
    /// Dernière erreur (analyse ou exécution), vide s'il n'y en a pas.
    String get_error() const { return error; }

  private:
    String source;
    PackedStringArray names;
    Ref<Expression> expression;
    bool parsed = false;
    String error;
    String reported_error; // pour ne signaler chaque erreur qu'une fois dans la console
};
} // namespace godot

#endif // BEHAVIOR_EXPRESSION_HPP
