#include "editor_test.hpp"

#include "📰Journal Console/custom_logger.hpp"
#include "💻️Editeur d'objet/🧑‍💻Éditeur/object_interface_panel.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

void EditorTest::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("creer_fenetre_personnalisation"), &EditorTest::creer_fenetre_personnalisation);
    ClassDB::bind_method(D_METHOD("get_interface_panel"), &EditorTest::get_interface_panel);
}

ObjectInterfacePanel *EditorTest::get_interface_panel() const { return Object::cast_to<ObjectInterfacePanel>(ObjectDB::get_instance(interface_panel_id)); }

void EditorTest::_enter_tree()
{
    Button *btn = memnew(Button);
    btn->set_text(U"Ouvrir fenêtre de personnalisation");
    btn->connect("pressed", Callable(this, "creer_fenetre_personnalisation"));

    add_control_to_container(CONTAINER_TOOLBAR, btn);
    button_id = btn->get_instance_id();

    log_it(U"Plugin de test chargé", this);
}

void EditorTest::_exit_tree()
{
    // Le bouton peut déjà avoir été libéré par l'éditeur (fermeture) : on le résout avant d'y toucher.
    if (Button *btn = Object::cast_to<Button>(ObjectDB::get_instance(button_id)))
    {
        remove_control_from_container(CONTAINER_TOOLBAR, btn);
        btn->queue_free();
    }
    button_id = ObjectID();

    // Dernière référence : la WindowEditor libère sa fenêtre (NOTIFICATION_PREDELETE).
    window.unref();
}

void EditorTest::creer_fenetre_personnalisation()
{
    if (window.is_null())
        window.instantiate();
    window->open_window();

    // Panneau (aperçu + configuration de l'interface du type) : créé une fois, recréé si la fenêtre (et donc
    // lui) a été libérée.
    ObjectInterfacePanel *panel = Object::cast_to<ObjectInterfacePanel>(ObjectDB::get_instance(interface_panel_id));
    if (!panel)
    {
        panel = memnew(ObjectInterfacePanel);
        panel->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
        window->add_control_to_window(panel);
        interface_panel_id = ObjectID(panel->get_instance_id());
    }

    // Cible : premier nœud sélectionné, sinon la racine de la scène éditée.
    EditorInterface *editor_interface = EditorInterface::get_singleton();
    Node *target = nullptr;
    if (editor_interface)
    {
        TypedArray<Node> selected = editor_interface->get_selection()->get_selected_nodes();
        target = selected.is_empty() ? editor_interface->get_edited_scene_root() : Object::cast_to<Node>(selected[0]);
    }
    // Interface du type de la cible (base du projet), modifiable et enregistrable.
    panel->set_target(target);
}
