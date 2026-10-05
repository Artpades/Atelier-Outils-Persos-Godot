# Contexte
GDExtension C++ (godot-cpp) pour Godot 4.7.2, build SCons. Module d'outils d'édition et de debug.
Objectif : générer automatiquement des interfaces d'édition d'objets (propriétés, méthodes, signaux, gizmos)
pour ~80 modules, utilisables dans l'éditeur ET en jeu (fenêtres de debug).

# Référence de conception : implémentation C# existante
Une version C# (namespace GameplayToolkit.DebugEdition) existe : DataRegistry, DataEntry, GroupRegistry, GroupEntry,
WidgetTable (DataKind, WidgetChoice, GroupContainerType), PropertyBinding, ValueProxy, ConfiguredInterfaceBuilder,
EntryGeneratorFactory, PropertyWidgetGenerator, PropertyWidgetRegistry, IInspectorLayoutHost.
Emplacement : [À COMPLÉTER].
- Porte ses CONCEPTS en C++ (synchronisation registre ↔ membres avec IsMissing, groupes auto depuis @export_group,
  table des widgets autorisés par type, anti-écho NotifyWritten, comparaison par hash de contenu).
- Ne porte PAS ses défauts, listés dans chaque phase (ExtraData non typé, un Node de polling par widget, pas d'undo,
  registre lié à une instance et non à un type).
- Classes référencées mais absentes (SignalConnection, SignalConnector, GizmoRegistry, EffectRegistry, InspectorBuilder,
  WidgetUtils, PackedArrays, DynamicInspectorPanel, InspectorWindowEditor) : si elles sont dans le dépôt, lis-les ;
  sinon, ne devine pas leur contenu.

# Règles de vérification (obligatoires)
- Avant d'utiliser une classe/méthode/enum Godot, vérifie son existence dans gen/extension_api.json ou les headers générés
  de godot-cpp. N'invente aucune API. Si elle n'existe pas : arrête-toi et signale-le.
- Exigence ambiguë → pose la question, ne devine pas.
- Une étape n'est terminée qu'après `scons` (target=editor ET target=template_debug) sans erreur ni nouveau warning,
  et un test décrit (ce qui a été testé ET ce qui ne l'a pas été).

# Conventions
- Logs : godot::log_it(message, caller, icon_path) (custom_logger.hpp). Commentaires en français.
- Conserver les chemins d'include existants (dossiers avec emojis).
- Runtime : enregistré au niveau MODULE_INITIALIZATION_LEVEL_SCENE. Éditeur (EditorPlugin, EditorUndoRedoManager,
  EditorSelection, fenêtres d'édition de registres) : niveau EDITOR, fichiers séparés.
- Code runtime appelant une API éditeur (comme Gizmos::_get_scenario) : uniquement derrière
  Engine::get_singleton()->is_editor_hint() ET un test de nullité du singleton.
- Jamais de pointeur brut vers un Object non possédé : ObjectID + ObjectDB::get_instance(), vérifié à chaque accès
  (même principe que ManagerBuilder).
- Toute ressource RenderingServer (RID) a un propriétaire unique qui la libère dans NOTIFICATION_PREDELETE.
- Configuration typée : pas de Dictionary « ExtraData » à clés libres. Chaque widget a sa Resource de réglages
  avec des propriétés exportées (fautes de frappe impossibles, validées par l'inspecteur).

# Pièges Godot à ne jamais oublier
- Packed*Array, Vector*, Rect*, AABB, Basis, Transform*, Quaternion, Plane, Projection, Color sont copiés à la lecture :
  modifier un sous-élément = reconstruire la valeur puis set() sur la propriété racine.
- Object n'a aucun signal « valeur modifiée » ; property_list_changed concerne la liste des propriétés, pas les valeurs.
- Les classes GDScript `class_name` ne sont pas dans ClassDB (ClassDB::instantiate, class_get_method_list échouent) :
  héritage de script via Script::get_global_name() / get_base_script().
- Callable : bind() ajoute après les arguments d'appel, unbind(n) retire les n derniers.
  Forme utilisée : Callable(cible, methode).bind(c0, c1...).unbind(n) → appel(a0..a(S-n-1), c0, c1...).
  Un Callable custom n'est pas sérialisé par CONNECT_PERSIST.
- Gizmos (gizmos.hpp) : CHAQUE draw_* efface le gizmo avant de dessiner (canvas_item_clear en 2D, mesh_clear via
  _get_or_create_mesh_rid en 3D). Donc 1 RID = 1 forme. Les registres 3D sont statiques et propres à chaque bibliothèque :
  créer, dessiner et libérer un gizmo depuis la même bibliothèque.

# Architecture cible
- WidgetTable (port) : DataKind, WidgetChoice, widgets autorisés par type, widget par défaut.
- DataEntry / DataRegistry (port) : un membre = une entrée ; synchronisation avec les listes de propriétés/méthodes/signaux.
- GroupEntry / GroupRegistry (port) : groupes définis par l'utilisateur, conteneur VBox / HBox / Grid (colonnes) / Panel.
- PropertyWidget (base) + WidgetFactory (registre extensible, priorité réglages > hint > défaut).
- ValueAccessor (port de ValueProxy) : accès par chemin (propriété racine + sous-chemin) avec réécriture des types valeur.
- SyncController : UN contrôleur par éditeur qui interroge les widgets visibles (remplace un Node de polling par widget),
  garde la logique anti-écho NotifyWritten.
- UndoBackend : EditorUndoRedoManager (éditeur) / UndoRedo (jeu).
- ArgumentBinding : source = constante | chemin de propriété | argument de signal. Partagé par signaux et gizmos.
- SignalLink : deux modes (natif CONNECT_PERSIST / Resource maison).
- GizmoEntry / GizmoRegistry / GizmoHost.
- LayoutDatabase : associe un type (classe native ou script) à ses registres ; surcharge possible par instance.

# Phases (une à la fois, validation entre chaque) : voir PHASES.md
0 corrections · 1 noyau · 2 collections, groupes, méthodes · 3 signaux · 4 gizmos · 5 registres et ouverture auto.
