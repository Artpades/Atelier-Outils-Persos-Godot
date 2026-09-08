#include "manager_builder.hpp"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

ManagerBuilder::ManagerBuilder() {}
ManagerBuilder::~ManagerBuilder() {}

void ManagerBuilder::_bind_methods()
{
    ClassDB::bind_static_method("ManagerBuilder", D_METHOD("make_getter", "class_name"), &ManagerBuilder::make_getter);
    ClassDB::bind_static_method("ManagerBuilder", D_METHOD("init_instance", "class_name", "caller"), &ManagerBuilder::init_instance);
    ClassDB::bind_static_method("ManagerBuilder", D_METHOD("unregister_instance", "class_name", "caller"), &ManagerBuilder::unregister_instance);
}

HashMap<StringName, ObjectID> &ManagerBuilder::_get_registry()
{
    static HashMap<StringName, ObjectID> registry;
    return registry;
}

Node *ManagerBuilder::_resolve(const StringName &p_class_name)
{
    HashMap<StringName, ObjectID>::Iterator it = _get_registry().find(p_class_name);
    if (!it)
    {
        return nullptr;
    }

    // instance_from_id() passe par ObjectDB en interne : renvoie un Variant nul (donc
    // Object* == nullptr après cast) si l'objet a été détruit, sans jamais déréférencer
    // un pointeur périmé.
    Object *obj = Object::cast_to<Object>(UtilityFunctions::instance_from_id((int64_t)it->value));
    Node *node = Object::cast_to<Node>(obj);
    if (!node)
    {
        // Entrée périmée : on la retire pour ne pas la re-tester à chaque appel.
        _get_registry().erase(p_class_name);
        return nullptr;
    }
    return node;
}

Node *ManagerBuilder::make_getter(const StringName &p_class_name)
{
    Node *existing = _resolve(p_class_name);
    if (existing)
    {
        return existing;
    }

    SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (!tree)
    {
        print_line_rich("[color=red]ManagerBuilder::make_getter — Aucun SceneTree actif, impossible de créer '" + String(p_class_name) + "'[/color]");
        return nullptr;
    }

    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    if (!cdb || !cdb->class_exists(p_class_name) || !cdb->can_instantiate(p_class_name))
    {
        print_line_rich("[color=red]ManagerBuilder::make_getter — Classe native '" + String(p_class_name) + "' inexistante ou non instanciable dans ClassDB[/color]");
        return nullptr;
    }

    Object *new_obj = cdb->instantiate(p_class_name);
    Node *new_node = Object::cast_to<Node>(new_obj);
    if (!new_node)
    {
        print_line_rich("[color=red]ManagerBuilder::make_getter — '" + String(p_class_name) + "' ne dérive pas de Node, singleton refusé[/color]");
        if (new_obj)
        {
            memdelete(new_obj);
        }
        return nullptr;
    }

    new_node->set_name(String(p_class_name) + " (Singleton)");
    _get_registry()[p_class_name] = new_node->get_instance_id();

    // Ajout différé pour éviter de perturber l'arbre pendant une transition de scène en cours.
    tree->get_root()->call_deferred("add_child", new_node);

    return new_node;
}

Node *ManagerBuilder::init_instance(const StringName &p_class_name, Node *p_caller)
{
    if (!p_caller)
    {
        return nullptr;
    }

    Node *existing = _resolve(p_class_name);
    if (existing && existing != p_caller)
    {
        // Doublon : l'appelant n'est pas l'instance canonique, on le détruit.
        p_caller->queue_free();
        return existing;
    }

    // Aucune instance valide enregistrée, ou p_caller est déjà l'instance enregistrée :
    // p_caller devient/reste l'instance canonique.
    _get_registry()[p_class_name] = p_caller->get_instance_id();
    return p_caller;
}

void ManagerBuilder::unregister_instance(const StringName &p_class_name, Node *p_caller)
{
    if (!p_caller)
    {
        return;
    }

    HashMap<StringName, ObjectID>::Iterator it = _get_registry().find(p_class_name);
    if (it && (uint64_t)it->value == (uint64_t)p_caller->get_instance_id())
    {
        _get_registry().erase(p_class_name);
    }
}