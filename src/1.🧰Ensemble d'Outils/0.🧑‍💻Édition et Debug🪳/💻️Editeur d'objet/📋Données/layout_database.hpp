#ifndef LAYOUT_DATABASE_HPP
#define LAYOUT_DATABASE_HPP

#include "data_registry.hpp"
#include "group_registry.hpp"

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🎯Gizmos/gizmo_registry.hpp"
#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/🔧Réglages/interface_style.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
// Interface d'un TYPE d'objet : quoi afficher (DataRegistry), comment le regrouper (GroupRegistry), quoi
// dessiner (GizmoRegistry). Le type est une classe native ou un class_name de script.
class TypeLayout : public Resource
{
    GDCLASS(TypeLayout, Resource)

  private:
    String type_name;
    bool include_subclasses = true;
    int priority = 0;
    Ref<DataRegistry> data_registry;
    Ref<GroupRegistry> group_registry;
    Ref<GizmoRegistry> gizmo_registry;
    Ref<InterfaceStyle> style; // style global de l'interface du type

  protected:
    static void _bind_methods();

  public:
    void set_type_name(const String &p_type);
    String get_type_name() const;
    void set_include_subclasses(bool p_include);
    bool get_include_subclasses() const;
    void set_priority(int p_priority);
    int get_priority() const;
    void set_data_registry(const Ref<DataRegistry> &p_registry);
    Ref<DataRegistry> get_data_registry() const;
    void set_group_registry(const Ref<GroupRegistry> &p_registry);
    Ref<GroupRegistry> get_group_registry() const;
    void set_gizmo_registry(const Ref<GizmoRegistry> &p_registry);
    Ref<GizmoRegistry> get_gizmo_registry() const;
    void set_style(const Ref<InterfaceStyle> &p_style);
    Ref<InterfaceStyle> get_style() const;

    // Crée les registres absents (les trois existent ensuite).
    void ensure_registries();
};

// Base des interfaces par type (fichier .tres désigné par le réglage de projet
// gameplay_toolkit/object_editor/layout_database). Résolution de l'interface d'un objet :
//   1. surcharge de l'instance : métadonnée « object_editor_layout » (un TypeLayout) ;
//   2. le type le plus spécifique : chaîne des scripts (class_name), puis chaîne native ; un TypeLayout
//      d'une classe parente ne s'applique que si include_subclasses ;
//   3. à spécificité égale, la plus grande priorité ;
//   4. sinon : aucune (interface générale par défaut).
class LayoutDatabase : public Resource
{
    GDCLASS(LayoutDatabase, Resource)

  private:
    TypedArray<TypeLayout> layouts;

  protected:
    static void _bind_methods();

  public:
    static constexpr const char *SETTING_PATH = "gameplay_toolkit/object_editor/layout_database";
    static constexpr const char *DEFAULT_PATH = "res://object_editor_layouts.tres";
    static constexpr const char *INSTANCE_META = "object_editor_layout";

    // Déclare le réglage de projet (au chargement du module).
    static void register_project_setting();
    static String get_project_path();
    // Base du projet (nulle si le fichier n'existe pas encore). Le cache de ResourceLoader garantit qu'une
    // base en cours d'édition est la même instance.
    static Ref<LayoutDatabase> load_project_database();

    // Types de p_object du plus spécifique au plus général : class_name des scripts, puis classes natives.
    static PackedStringArray type_chain(Object *p_object);

    void set_layouts(const TypedArray<TypeLayout> &p_layouts);
    TypedArray<TypeLayout> get_layouts() const;

    Ref<TypeLayout> find_layout(Object *p_object) const;
    Ref<TypeLayout> find_by_type(const String &p_type_name) const;
    // TypeLayout du type (créé, avec ses registres, s'il n'existe pas).
    Ref<TypeLayout> ensure_type(const String &p_type_name);
};
} // namespace godot

#endif // LAYOUT_DATABASE_HPP
