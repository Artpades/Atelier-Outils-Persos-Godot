#ifndef INPUT_CONTEXT_HPP
#define INPUT_CONTEXT_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot
{
/// 🗂️ Groupe d'actions d'entrée (menu, gameplay, pause...) que l'on active ou désactive d'un bloc.
/// Son nom sert aussi d'intertitre dans PlayerInputUI.
class InputContext : public Node
{
    GDCLASS(InputContext, Node)

  public:
    void set_context_name(const String &p_name) { context_name = p_name; }
    String get_context_name() const { return context_name; }

    /// Recopie p_enabled dans action_enabled de chaque InputAction enfant direct.
    void set_context_enabled(bool p_enabled);
    bool is_context_enabled() const { return context_enabled; }

  protected:
    static void _bind_methods();

  private:
    String context_name = String(U"[Insérer le nom du contexte ici]");
    bool context_enabled = true;
};
} // namespace godot

#endif // INPUT_CONTEXT_HPP
