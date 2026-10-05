#ifndef INTERFACE_CONFIG_WINDOW_HPP
#define INTERFACE_CONFIG_WINDOW_HPP

#include "data_matrix_editor.hpp"
#include "group_matrix_editor.hpp"
#include "behavior_list_editor.hpp"
#include "interface_style_editor.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🧱Construction/procedural_editor.hpp"

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot
{
// Fenêtre « Configurer l'interface » (éditeur uniquement, port d'InterfaceConfigWindowEditor C#) :
// édite le DataRegistry d'un ProceduralEditor de la scène éditée, pour la cible qu'il déclare
// (target_path / target_resource).
//  - gauche : barre de scan (propriétés / méthodes / signaux / méthodes privées, périmètre, actualiser) +
//    onglets Données (DataMatrixEditor) et Groupes (GroupMatrixEditor) ;
//  - droite : aperçu en direct (ProceduralEditor branché sur la vraie cible, annulation de l'éditeur).
// Les registres (données, groupes) restent ceux du ProceduralEditor (enregistrés avec la scène, marquée comme modifiée).
// Une seule fenêtre ouverte à la fois (open_for remplace la précédente).
class InterfaceConfigWindow : public Window
{
    GDCLASS(InterfaceConfigWindow, Window)

  private:
    static ObjectID current_window_id; // ObjectID : destruction triviale, pas de souci au déchargement

    ObjectID host_id;
    Ref<DataRegistry> registry;
    Ref<GroupRegistry> groups;

    // Enfants créés et possédés par la fenêtre.
    Label *target_label = nullptr;
    CheckBox *scan_properties = nullptr;
    CheckBox *scan_methods = nullptr;
    CheckBox *scan_signals = nullptr;
    CheckBox *private_methods = nullptr;
    OptionButton *scope = nullptr;
    DataMatrixEditor *matrix = nullptr;
    GroupMatrixEditor *group_matrix = nullptr;
    InterfaceStyleEditor *style_editor = nullptr;
    BehaviorListEditor *behavior_list = nullptr;
    ProceduralEditor *preview = nullptr;

    void _build_ui();
    ProceduralEditor *_get_host() const;
    void _on_scan_toggled(bool p_on);
    void _on_scope_selected(int64_t p_index);
    void _on_registry_changed();
    void _on_style_changed();
    void _on_close_requested();

  protected:
    static void _bind_methods();

  public:
    // Ouvre (ou remplace) la fenêtre de configuration pour p_host.
    static void open_for(ProceduralEditor *p_host);
    static void close_current();

    // Relit la cible déclarée, resynchronise le registre et reconstruit tableau + aperçu.
    void resync();
};
} // namespace godot

#endif // INTERFACE_CONFIG_WINDOW_HPP
