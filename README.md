# Atelier Outils Persos Godot
 Un projet qui répertorie les outils pour créer des jeux et autre

## Modules

Chaque dossier de `src/` qui contient un `module.json` est compilé en une **GDExtension séparée** :
`GameplayToolkit/bin/<id>/gameplaytoolkit_<id>.gdextension` + ses bibliothèques.
Pour choisir ses outils dans un projet, il suffit de copier (ou supprimer) les dossiers `bin/<id>/` voulus.

```sh
scons                                  # tous les modules
scons modules=input,gizmos,save        # seulement certains modules
```

| Module | Dépendances requises | Dépendances optionnelles |
|---|---|---|
| `gizmos`, `journal`, `manager_builder`, `window_editor`, `widgets`, `gauge`, `exemples` | — | — |
| `handlers` | `manager_builder`, `gizmos` | — |
| `camera` | `manager_builder` | — |
| `scene_hierarchy` | — | `journal` |
| `save` | — | `journal` |
| `input` | `widgets`, `save` (pour `PlayerInputUI` uniquement) | `gizmos` (dessin de debug), `journal` |

- **Dépendance requise absente** : la classe concernée reste disponible mais se désactive (avertissement dans
  l'éditeur, erreur explicite en jeu, `get_singleton()` retourne `null`).
- **Dépendance optionnelle absente** : l'objet fonctionne en mode dégradé (logs dans la console, pas de dessin de debug).

Les modules ne s'appellent jamais directement en C++ : ils passent par la ClassDB du moteur via les
en-têtes `*_api.hpp` de chaque module fournisseur et `src/commun/module_dependencies.hpp`.
Pour ajouter un module : un dossier avec `module.json` (id, dépendances, classes/icônes) et un
`register_types.cpp` qui se termine par `GAMEPLAYTOOLKIT_MODULE(<id>, U"<nom>")`.
