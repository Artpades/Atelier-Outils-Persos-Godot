# Atelier Outils Persos Godot
 Un projet qui répertorie les outils pour créer des jeux et autre

## Modules

Chaque dossier de `src/` qui contient un `module.json` est compilé en une **GDExtension séparée** :
`GameplayToolkit/bin/<id>/gameplaytoolkit_<id>.gdextension` + ses bibliothèques.
Pour choisir ses outils dans un projet, il suffit de copier (ou supprimer) les dossiers `bin/<id>/` voulus.

```sh
scons                                  # tous les modules
scons modules=input,edition_debug,save # seulement certains modules
```

Un module possède tout son dossier, sous-dossiers compris. Les outils d'édition et de debug (Gizmos, Journal,
Générateur de Gestionnaire, Générateur de Widget d'éditions, Fenêtre d'édition, Hiérarchie de scène, Handlers)
forment ainsi un seul module, `edition_debug`.

| Module | Dépendances requises | Dépendances optionnelles |
|---|---|---|
| `edition_debug`, `gauge`, `exemples` | — | — |
| `camera` | `edition_debug` (ManagerBuilder) | — |
| `save` | — | `edition_debug` (Journal) |
| `input` | `edition_debug` (InspectorBinding), `save` — pour `PlayerInputUI` uniquement | `edition_debug` (dessin de debug, Journal) |

- **Dépendance requise absente** : la classe concernée reste disponible mais se désactive (avertissement dans
  l'éditeur, erreur explicite en jeu, `get_singleton()` retourne `null`).
- **Dépendance optionnelle absente** : l'objet fonctionne en mode dégradé (logs dans la console, pas de dessin de debug).

Les modules ne s'appellent jamais directement en C++ : ils passent par la ClassDB du moteur via les
en-têtes `*_api.hpp` de chaque module fournisseur et `src/commun/module_dependencies.hpp`.
Pour ajouter un module : un dossier avec `module.json` (id, dépendances, classes/icônes) et un
`register_types.cpp` qui se termine par `GAMEPLAYTOOLKIT_MODULE(<id>, U"<nom>")`.
