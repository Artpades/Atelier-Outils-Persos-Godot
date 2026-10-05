#ifndef GIZMO_REGISTRY_HPP
#define GIZMO_REGISTRY_HPP

#include "1.🧰Ensemble d'Outils/0.🧑‍💻Édition et Debug🪳/💻️Editeur d'objet/📋Données/signal_link.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot
{
// Catalogue des fonctions de dessin : méthodes statiques draw_* de Gizmos2D / Gizmos3D lues dans ClassDB
// (aucune liste écrite en dur). Une fonction est désignée par "Classe.méthode" (ex: "Gizmos3D.draw_3D_cube").
// Le premier paramètre (RID du gizmo) est toujours fourni par le GizmoHost : les « paramètres utilisateur »
// commencent au deuxième.
class GizmoCatalog
{
  public:
    GizmoCatalog() = delete; // Classe purement statique

    static PackedStringArray functions();
    static Dictionary method_info(const String &p_function);
    static String class_of(const String &p_function);
    static String method_of(const String &p_function);
    static bool is_3d(const String &p_function);
    static int user_param_count(const String &p_function);
    static Dictionary user_param(const String &p_function, int p_index);
    // Valeur par défaut d'un paramètre utilisateur (default_args, sinon défaut du type).
    static Variant user_default(const String &p_function, int p_index);
};

// Un dessin automatique (port de GizmoEntry C#) : fonction du catalogue + un ArgumentBinding par paramètre
// utilisateur (constante ou propriété de la cible), visibilité et politique de redessin.
class GizmoEntry : public Resource
{
    GDCLASS(GizmoEntry, Resource)

  public:
    // Valeurs enregistrées dans les .tres : jamais renumérotées.
    enum Visibility
    {
        VISIBILITY_EVERYWHERE = 0,
        VISIBILITY_EDITOR = 1,
        VISIBILITY_GAME = 2,
    };
    enum Redraw
    {
        REDRAW_ON_CHANGE = 0,   // quand une source (constante, propriété) change
        REDRAW_EVERY_FRAME = 1, // à chaque image
        REDRAW_ONCE = 2,        // une seule fois (à la création du gizmo)
    };

  private:
    bool enabled = true;
    String function;
    TypedArray<ArgumentBinding> arguments; // déclaré APRÈS function : au chargement, set_function ne l'écrase pas
    Visibility visibility = VISIBILITY_EVERYWHERE;
    Redraw redraw = REDRAW_ON_CHANGE;

  protected:
    static void _bind_methods();
    void _validate_property(PropertyInfo &p_property) const;

  public:
    // Arguments neufs pour une fonction : constantes par défaut, typées comme les paramètres.
    static TypedArray<ArgumentBinding> make_default_arguments(const String &p_function);

    void set_enabled(bool p_enabled);
    bool get_enabled() const;
    // Changer de fonction recrée les arguments (valeurs par défaut de la nouvelle fonction).
    void set_function(const String &p_function);
    String get_function() const;
    // Fonction ET arguments en une fois (annulation d'un changement de fonction, sans régénérer les arguments).
    void restore(const String &p_function, const TypedArray<ArgumentBinding> &p_arguments);
    void set_arguments(const TypedArray<ArgumentBinding> &p_arguments);
    TypedArray<ArgumentBinding> get_arguments() const;
    void set_visibility(Visibility p_visibility);
    Visibility get_visibility() const;
    void set_redraw(Redraw p_redraw);
    Redraw get_redraw() const;

    // Vrai si l'entrée doit être dessinée dans le contexte courant (éditeur ou jeu).
    bool is_shown_now() const;
};

// Liste des dessins d'un objet (port de GizmoRegistry C#).
class GizmoRegistry : public Resource
{
    GDCLASS(GizmoRegistry, Resource)

  private:
    TypedArray<GizmoEntry> entries;

  protected:
    static void _bind_methods();

  public:
    void set_entries(const TypedArray<GizmoEntry> &p_entries);
    TypedArray<GizmoEntry> get_entries() const;
};
} // namespace godot

VARIANT_ENUM_CAST(GizmoEntry::Visibility);
VARIANT_ENUM_CAST(GizmoEntry::Redraw);

#endif // GIZMO_REGISTRY_HPP
