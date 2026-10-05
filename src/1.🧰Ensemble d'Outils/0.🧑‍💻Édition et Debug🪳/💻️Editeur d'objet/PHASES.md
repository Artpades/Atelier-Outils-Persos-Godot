# Prompts par phase (à donner un par un à Claude Code)

Chaque prompt commence implicitement par : « Lis CLAUDE.md. Avant de coder, rends un plan : fichiers/classes,
niveau d'enregistrement, API Godot utilisées avec leur vérification dans extension_api.json, questions bloquantes.
Attends ma validation. »

---

## Phase 0 — Corrections du code existant

1. tool_editor_register.cpp : EditorTest est créé par memnew sans être ajouté à l'arbre ni libéré (fuite, _enter_tree
   jamais appelé). Remplace par l'enregistrement standard d'un EditorPlugin de cette version de godot-cpp (vérifie l'API).
2. WindowEditor : Window* → ObjectID, gérer la libération de la fenêtre, supprimer on_window_close (doublon de close_window).
   La fenêtre doit fonctionner en jeu et dans l'éditeur.
3. Gizmos2D : normaliser les noms liés à GDScript sur ceux de Gizmos et de Gizmos3D
   (create_2D_gizmos → create_2D_gizmo, clear_2D_gizmos → clear_2D_gizmo, free_gizmos → free_2D_gizmo).
   Avant : recherche tous les usages GDScript/C# dans le projet et liste-les ; c'est un changement cassant.
4. manager_builder.hpp : le commentaire de make_getter dit qu'une classe GDScript `class_name` est acceptée.
   C'est faux (pas dans ClassDB) et contredit le point 2 du même en-tête : corrige le commentaire.

---

## Phase 1 — Noyau

Porter depuis le C# : WidgetTable (DataKind, WidgetChoice, AllowedFor, DefaultFor, KindOf), DataEntry, DataRegistry.Sync
(scan propriétés/méthodes/signaux, filtrage CATEGORY/GROUP/SUBGROUP/EDITOR, entrées IsMissing jamais supprimées).

Écarts imposés par rapport au C# :
- ExtraData → une Resource de réglages par widget (ex. RangeWidgetSettings : min, max, step, allow_greater,
  allow_lesser, suffix ; GridSettings : columns ; TextEditSettings : height...).
- PropertyBinding (un Node + _process par widget) → SyncController unique par éditeur, intervalle configurable,
  widgets visibles seulement ; conserver NotifyWritten (valeur refusée/transformée par un setter → widget corrigé + warning)
  et la comparaison par type + hash de contenu.
- Toute écriture passe par UndoBackend (une seule action par édition : value_changed(value, committing),
  committing=false pendant un glissement ou une frappe, true à la validation).

Widgets de cette phase :
- bool : CheckBox, CheckButton, Label.
- int/float : SpinBox, HSlider, VSlider (éditables) ; ProgressBar, TextureProgressBar, Label (lecture seule).
  min/max/step/allow_greater/allow_lesser via l'API Range ; PROPERTY_HINT_RANGE ("min,max,step,or_greater,or_less")
  sert de défaut quand les réglages ne précisent rien.
- String/StringName : LineEdit, TextEdit, Label.
- PROPERTY_HINT_ENUM (int ou String, format "Nom:valeur") : OptionButton, ou liste de CheckButton/CheckBox dans un ButtonGroup.
- PROPERTY_HINT_FLAGS : liste de CheckBox sans ButtonGroup (masque de bits).
- Color : ColorPickerButton, ou SpinBox par canal + ColorRect d'aperçu.
- Structures (Vector2/2i/3/3i/4/4i, Rect2/2i, AABB, Plane, Quaternion, Basis, Transform2D/3D, Projection) : SpinBox par composante.
- RID : Label (get_id()).

Tests : classe de démonstration avec une propriété de chaque type ; édition en jeu et dans l'éditeur ;
vérifier get() après modification, undo/redo, modification externe reflétée, setter qui clamp la valeur,
libération de l'objet cible fenêtre ouverte sans crash.

---

## Phase 2 — Collections, Variant, méthodes, groupes

- Object, NodePath, Array, Dictionary, Packed*Array : génération récursive via ValueAccessor (chemin depuis la propriété
  racine ; réécriture de la racine pour les types valeur). Conteneur au choix : VBoxContainer, HBoxContainer,
  GridContainer (colonnes), FlowContainer, TabContainer. Dépliage paresseux, profondeur max, anti-cycle par ObjectID.
- Array/Dictionary typés : analyser le hint_string. Ajout/suppression/réordonnancement d'éléments. Object nul :
  affecter null ou instancier une classe compatible (natif via ClassDB, script via la liste des classes globales).
- Sélecteur de chemin : éditeur → EditorInterface::popup_node_selector / popup_property_selector ;
  jeu → sélecteur maison (arbre de la scène courante).
- Variant : OptionButton des types assignables ; au changement, reconstruire le widget du type choisi.
- Méthodes : bouton au nom de la méthode ; si arguments, widgets générés par la factory (défauts depuis default_args),
  appel par callv, résultat affiché si non void. Exclure les méthodes commençant par « _ » (comme le C#), option pour les inclure.
- Groupes (port de GroupEntry/GroupRegistry) : définis par l'utilisateur, conteneur VBox / HBox / Grid (colonnes) / Panel.
  Règles conservées : une donnée appartient à un seul groupe ; le groupe s'affiche à la place de sa première donnée ;
  groupes créés automatiquement depuis @export_group / @export_subgroup pour les NOUVEAUX membres.
  Signaler dans l'UI les noms de groupe qui pointent vers une donnée IsMissing (au lieu de les ignorer en silence).

Tests : modifier un élément d'un PackedVector3Array et un champ de Transform3D imbriqué dans une Resource → get() cohérent ;
objet auto-référencé → pas de boucle ; groupe Grid à 3 colonnes ; méthode à 2 arguments appelée avec ses défauts.

---

## Phase 3 — ArgumentBinding et signaux

ArgumentBinding (Resource) : source = CONSTANT (Variant + widget) | PROPERTY (chemin depuis un objet) | SIGNAL_ARG (index).

Widget de signal (façon UnityEvent) : liste de liens ; chaque lien = objet cible, méthode, paramètres.
Pour chaque paramètre : Label « ✓ fourni par le signal » si l'argument du signal au même index est compatible, sinon widget constante.
Compatibilité : NIL = tout ; mêmes types ; int→float accepté ; Object via ClassDB::is_parent_class ou chaîne de scripts.

Deux modes, choisis automatiquement et affichés dans l'UI :
- NATIF : source et cible sont des nœuds de la même scène éditée (possédés par la racine de la scène éditée).
  connect(signal, Callable(cible, m).bind(constantes...).unbind(S - k), CONNECT_PERSIST) via UndoBackend.
  Seul le motif « k premiers paramètres = k premiers arguments du signal, le reste en constantes » est exprimable :
  l'UI doit rendre impossibles les autres correspondances dans ce mode et expliquer pourquoi.
  Test obligatoire : sauvegarder, ouvrir le .tscn en texte, vérifier binds ET unbinds, rouvrir la scène.
- RESSOURCE : tous les autres cas (Resource source ou cible, autre scène, jeu). Liens stockés dans le registre
  (comme DataEntry.Connections en C#), appliqués au chargement et retirés proprement (idempotent).
  Dans ce mode la correspondance arbitraire est permise via un Callable custom (vérifier CallableCustom dans godot-cpp ;
  sinon un objet relais avec méthode vararg).
- Constantes de type Object dans un lien persistant : refuser les objets non sérialisables et le signaler.

Sélection de la cible : éditeur → popup_node_selector / popup_method_selector, Resources via sélecteur de fichier ;
jeu → sélecteurs maison.

---

## Phase 4 — Automatisation des gizmos

- GizmoEntry (Resource) : fonction (choisie parmi les méthodes statiques draw_* de Gizmos2D/Gizmos3D via
  ClassDB::class_get_method_list), arguments = Array[ArgumentBinding] (le premier argument RID est fourni par l'hôte,
  jamais par l'utilisateur ; défauts depuis default_args), activé, visibilité (éditeur / jeu / les deux),
  politique de redessin (chaque frame / sur changement des sources / une fois).
- GizmoHost (Node) : crée UN RID par GizmoEntry (règle 1 RID = 1 forme), appelle via ClassDB::class_call_static,
  libère tout dans NOTIFICATION_PREDELETE et à exit_tree, recrée à enter_tree, recrée si le viewport change.
  Aucun appel à free_* en dehors de l'hôte.
- 2D dans l'éditeur : create_2D_gizmo n'a pas de repli éditeur (contrairement au 3D). Déterminer et tester quel viewport
  passer (EditorInterface::get_editor_viewport_2d() ?) ; si ça ne s'affiche pas, rapporter au lieu de bricoler.
- Tests : ajout/suppression d'entrées, suppression du nœud hôte, changement de scène, fermeture de l'éditeur ;
  vérifier l'absence de RID fuités (compteur interne de l'hôte à zéro + messages de fuite à la sortie en build debug).

---

## Phase 5 — Registres et ouverture automatique

- LayoutDatabase (Resource .tres) : liste de TypeLayout { type (classe native ou nom global de script), inclure les sous-classes,
  priorité, DataRegistry, GroupRegistry, GizmoRegistry }.
  Résolution : surcharge par instance (comme IInspectorLayoutHost en C#) > type le plus spécifique (chaîne de scripts puis
  chaîne native) > priorité > interface générale par défaut.
- Interface par gestionnaire : un TypeLayout par classe de manager (ManagerBuilder) ; le nœud renvoyé par make_getter est ajouté
  en différé : l'éditeur doit gérer une cible pas encore dans l'arbre.
- Fenêtre d'édition des registres dans l'éditeur, construite avec le ProceduralEditor lui-même quand c'est possible.
- Ouverture automatique : nœuds → EditorSelection::selection_changed ; Resources → signal edited_object_changed de
  EditorInterface::get_inspector(). Une fenêtre unique réutilisée, avec épinglage et option pour désactiver l'ouverture auto.
- Jeu : API open_editor_for(Object) + fenêtre de debug ; vérifier que les .tres des registres sont inclus dans l'export.
