#ifndef INSPECTOR_BINDING_HPP
#define INSPECTOR_BINDING_HPP

#include "inspector_core.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/node.hpp>

namespace godot
{
/// 🔗 Liaison de données bidirectionnelle entre un widget et une valeur (ValueAccessor).
/// Nœud invisible enfant du widget : il sonde la valeur à intervalle régulier et appelle `pull` quand elle
/// change de l'extérieur (Godot n'a pas de signal générique « propriété modifiée »).
/// Sens inverse : le widget appelle `commit()`, qui écrit la valeur puis corrige l'affichage si l'objet l'a
/// transformée (clamp, setter qui refuse...).
class InspectorBinding : public Node
{
    GDCLASS(InspectorBinding, Node)

  public:
    using Pull = std::function<void(const Variant &)>;
    /// Clé de comparaison : deux valeurs de même clé sont considérées identiques.
    using Key = std::function<uint32_t(const Variant &)>;

    static constexpr double DEFAULT_POLL_INTERVAL = 0.1;

    /// Crée la liaison, enfant de `p_owner`. La première synchronisation a lieu à l'entrée dans l'arbre,
    /// donc après que le générateur a connecté les signaux du widget.
    static InspectorBinding *attach(Control *p_owner, const ValueAccessor &p_accessor, Pull p_pull, Key p_key = nullptr);

    /// Clé par défaut : type + hash du contenu (compare aussi le contenu des Array / Dictionary).
    static uint32_t default_key(const Variant &p_value);

    void _ready() override;
    void _process(double p_delta) override;

    /// Relit la valeur et met le widget à jour si elle a changé (ou si `p_force`).
    void refresh(bool p_force = false);

    /// Écrit `p_value` depuis un signal du widget. Ignoré pendant un `pull` (évite la boucle d'écho).
    void commit(const Variant &p_value);

    /// Vrai pendant que la liaison pousse une valeur vers le widget.
    bool is_pulling() const { return pulling; }

    const ValueAccessor &get_accessor() const { return accessor; }

    /// Force la relecture de toutes les liaisons sous `p_root`.
    static void refresh_all(Node *p_root);

  protected:
    static void _bind_methods();

  private:
    void do_pull(const Variant &p_value);

    ValueAccessor accessor;
    Pull pull;
    Key key;
    uint32_t last_key = 0;
    bool has_last = false;
    bool pulling = false;
    double elapsed = 0.0;
};

} // namespace godot

#endif // INSPECTOR_BINDING_HPP
