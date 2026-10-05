#ifndef RICH_TOOLTIP_HPP
#define RICH_TOOLTIP_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧩Widgets/property_widget.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
// Infobulle enrichie d'un ProceduralEditor (nœud interne) : Godot n'affiche pas de BBCode dans l'infobulle
// d'un contrôle natif. Au survol prolongé d'un widget de l'éditeur qui a des infobulles
// (PropertyWidget::get_tooltip_texts), une fenêtre affiche leur texte BBCode, les modèles {…} étant résolus
// sur la cible de l'éditeur au moment de l'affichage. Fonctionne dans l'éditeur et en jeu.
class RichTooltip : public Node
{
    GDCLASS(RichTooltip, Node)

  private:
    ObjectID root_id;    // Control dont les widgets sont suivis (le ProceduralEditor)
    ObjectID context_id; // objet sur lequel les modèles sont résolus
    ObjectID hovered_id;
    double hover_time = 0.0;
    double delay = 0.5;
    PopupPanel *popup = nullptr; // enfants possédés
    RichTextLabel *label = nullptr;

    PropertyWidget *_widget_under_mouse() const;

  protected:
    static void _bind_methods();

  public:
    RichTooltip();
    void _process(double p_delta) override;

    void setup(Control *p_root);
    void set_context(Object *p_context);
    void set_delay(double p_delay);
    double get_delay() const;
    // Texte BBCode final des infobulles de p_widget (séparées par une ligne vide).
    String build_text(PropertyWidget *p_widget) const;
    // Affiche l'infobulle de p_widget à la position de la souris (aussi utilisé par les tests).
    void show_for(PropertyWidget *p_widget);
    void hide_tooltip();
    bool is_shown() const;
    String get_shown_text() const;
};
} // namespace godot

#endif // RICH_TOOLTIP_HPP
