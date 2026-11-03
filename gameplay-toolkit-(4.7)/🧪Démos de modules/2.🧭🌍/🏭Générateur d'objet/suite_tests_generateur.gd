extends "../suite_tests_base.gd"

## Suite de tests automatiques du module générateur d'objet : SpawnEntry, SpawnPoolEntry, Spawner, PoolSpawner.
##
## Les scènes instanciées sont construites en mémoire (PackedScene.pack) et portent une méta « id » pour
## reconnaître la scène d'origine de chaque instance.
## Avertissements attendus dans la console : aucune entrée, position introuvable, scène nulle, pool épuisé,
## objet étranger au pool, double despawn.

## Script des objets de pool : compte les appels à on_spawn_from_pool.
var _script_objet_pool: GDScript


func _executer() -> void:
	_script_objet_pool = GDScript.new()
	_script_objet_pool.source_code = "extends Node2D\nvar appels := 0\nfunc on_spawn_from_pool() -> void:\n\tappels += 1\n"
	_script_objet_pool.reload()

	_tester_entrees()
	await _tester_spawner_sans_entree()
	await _tester_scenes_sequentielles()
	await _tester_scenes_toutes()
	await _tester_scenes_aleatoires()
	await _tester_positions_repli()
	await _tester_positions_sequentielles()
	await _tester_positions_toutes()
	await _tester_transform_3d()
	await _tester_entrees_invalides()
	await _tester_pool_prechauffage()
	await _tester_pool_cycle()
	await _tester_pool_epuise()
	await _tester_pool_objet_etranger()
	await _tester_pool_instance_liberee()
	await _tester_pool_entree_simple()
	await _tester_pool_liberation()


# 📋 Ressources

func _tester_entrees() -> void:
	var entree := SpawnEntry.new()
	_verifier("SpawnEntry : listes vides par défaut", entree.positions.is_empty() and entree.scenes.is_empty())
	_verifier("SpawnEntry : modes séquentiels par défaut",
		entree.positions_selection_mode == SpawnEntry.SELECTION_SEQUENTIAL and entree.scenes_selection_mode == SpawnEntry.SELECTION_SEQUENTIAL)

	var entree_pool := SpawnPoolEntry.new()
	_verifier("SpawnPoolEntry : hérite de SpawnEntry", entree_pool is SpawnEntry)
	_verifier("SpawnPoolEntry : 10 instances pré-allouées par défaut", entree_pool.initial_pool_size_per_scene == 10)

	var copie: SpawnPoolEntry = _aller_retour(entree_pool)
	_verifier("SpawnPoolEntry : sérialisation (duplicate)", copie != null and copie.initial_pool_size_per_scene == 10)


# 🏭 Spawner

func _tester_spawner_sans_entree() -> void:
	var spawner := Spawner.new()
	_ajouter(spawner)
	var espion := _espionner(spawner.spawned)
	spawner.spawn() # avertissement attendu
	_verifier("Spawner : sans entrée, rien n'est émis ni instancié", espion.n == 0)
	spawner.queue_free()
	await _frames()


func _tester_scenes_sequentielles() -> void:
	var entree := _entree([_scene("A"), _scene("B")], SpawnEntry.SELECTION_SEQUENTIAL)
	var montage := _monter(Spawner.new(), entree, [Vector2(10, 20)])
	var espion := _espionner(montage.spawner.spawned)
	for i in 3:
		montage.spawner.spawn()
	_verifier("Spawner séquentiel : A, B, A", _ids(montage.conteneur) == ["A", "B", "A"], str(_ids(montage.conteneur)))
	_verifier("Spawner : signal spawned à chaque spawn", espion.n == 3)
	_verifier("Spawner : instances ajoutées sous parent_node", montage.conteneur.get_child_count() == 3)
	await _demonter(montage)


func _tester_scenes_toutes() -> void:
	var entree := _entree([_scene("A"), _scene("B"), _scene("C")], SpawnEntry.SELECTION_ALL)
	var montage := _monter(Spawner.new(), entree, [Vector2(10, 20)])
	montage.spawner.spawn()
	_verifier("Spawner « toutes » : chaque scène instanciée", _ids(montage.conteneur) == ["A", "B", "C"], str(_ids(montage.conteneur)))
	_verifier("Spawner « toutes » : position unique réutilisée (modulo)",
		montage.conteneur.get_children().all(func(n): return n.global_position == Vector2(10, 20)))
	await _demonter(montage)


func _tester_scenes_aleatoires() -> void:
	var entree := _entree([_scene("A"), _scene("B")], SpawnEntry.SELECTION_RANDOM)
	var montage := _monter(Spawner.new(), entree, [Vector2.ZERO])
	for i in 30:
		montage.spawner.spawn()
	var ids := _ids(montage.conteneur)
	_verifier("Spawner aléatoire : une instance par spawn, parmi les scènes", ids.size() == 30 and ids.all(func(id): return id in ["A", "B"]))
	_avertir_si("Spawner aléatoire : les deux scènes apparaissent", "A" in ids and "B" in ids, "une seule scène sur 30 tirages")
	await _demonter(montage)


func _tester_positions_repli() -> void:
	var entree := _entree([_scene("A")], SpawnEntry.SELECTION_SEQUENTIAL)
	var montage := _monter(Spawner.new(), entree, [Vector2(5, 5), Vector2(50, 50)])
	montage.spawner.spawn()
	montage.spawner.spawn()
	var positions: Array = montage.conteneur.get_children().map(func(n): return n.global_position)
	_verifier("Spawner : sans positions, les enfants 2D/3D servent de positions", positions == [Vector2(5, 5), Vector2(50, 50)], str(positions))
	_verifier("Spawner : la ressource partagée n'est pas modifiée par le repli", entree.positions.is_empty())
	await _demonter(montage)


func _tester_positions_sequentielles() -> void:
	var entree := _entree([_scene("A")], SpawnEntry.SELECTION_SEQUENTIAL)
	entree.positions = [NodePath("M1"), NodePath("M0")]
	var montage := _monter(Spawner.new(), entree, [Vector2(0, 0), Vector2(100, 0)])
	for i in 3:
		montage.spawner.spawn()
	var positions: Array = montage.conteneur.get_children().map(func(n): return n.global_position.x)
	_verifier("Spawner : positions séquentielles dans l'ordre de la liste (M1, M0, M1)", positions == [100.0, 0.0, 100.0], str(positions))
	await _demonter(montage)


func _tester_positions_toutes() -> void:
	var entree := _entree([_scene("A"), _scene("B"), _scene("C")], SpawnEntry.SELECTION_ALL)
	entree.positions_selection_mode = SpawnEntry.SELECTION_ALL
	var montage := _monter(Spawner.new(), entree, [Vector2(0, 0), Vector2(100, 0)])
	montage.spawner.spawn()
	var positions: Array = montage.conteneur.get_children().map(func(n): return n.global_position.x)
	_verifier("Spawner « toutes / toutes » : scène i à la position i modulo le nombre de positions", positions == [0.0, 100.0, 0.0], str(positions))
	await _demonter(montage)


func _tester_transform_3d() -> void:
	var racine := Node3D.new()
	racine.set_meta("id", "3D")
	var entree := _entree([_empaqueter(racine)], SpawnEntry.SELECTION_SEQUENTIAL)

	var conteneur := Node3D.new()
	conteneur.position = Vector3(10, 0, 0) # parent décalé : la transform globale doit quand même correspondre
	_ajouter(conteneur)
	var spawner := Spawner.new()
	spawner.parent_node = conteneur
	spawner.entries = [entree]
	var marqueur := Marker3D.new()
	marqueur.position = Vector3(1, 2, 3)
	marqueur.rotation = Vector3(0, 1, 0)
	spawner.add_child(marqueur)
	_ajouter(spawner)

	spawner.spawn()
	var instance: Node3D = conteneur.get_child(0)
	_verifier("Spawner 3D : transform globale du marqueur copiée",
		instance.global_transform.is_equal_approx(marqueur.global_transform), "%s ≠ %s" % [instance.global_transform, marqueur.global_transform])
	spawner.queue_free()
	conteneur.queue_free()
	await _frames()


func _tester_entrees_invalides() -> void:
	var entree := _entree([_scene("A")], SpawnEntry.SELECTION_SEQUENTIAL)
	entree.positions = [NodePath("Inexistant")]
	var montage := _monter(Spawner.new(), entree, [])
	montage.spawner.spawn() # avertissement attendu
	_verifier("Spawner : position introuvable → aucune instance, sans plantage", montage.conteneur.get_child_count() == 0)
	await _demonter(montage)

	var avec_nulle := _entree([null, _scene("A")], SpawnEntry.SELECTION_ALL)
	montage = _monter(Spawner.new(), avec_nulle, [Vector2.ZERO])
	montage.spawner.spawn() # avertissement attendu
	_verifier("Spawner : scène nulle ignorée, les autres instanciées", _ids(montage.conteneur) == ["A"], str(_ids(montage.conteneur)))
	await _demonter(montage)

	var spawner := Spawner.new()
	spawner.entries = [null]
	_ajouter(spawner)
	spawner.spawn() # avertissement attendu
	_verifier("Spawner : entrée nulle ignorée sans plantage", true)
	spawner.queue_free()

	var hors_arbre := Spawner.new()
	hors_arbre.spawn() # erreur attendue
	_verifier("Spawner : spawn hors de l'arbre refusé sans plantage", true)
	hors_arbre.free()
	await _frames()


# ♻️ PoolSpawner

func _tester_pool_prechauffage() -> void:
	var montage := _monter(PoolSpawner.new(), _entree_pool([_scene_pool("P")], 3), [Vector2(30, 40)])
	await _frames() # ajout différé des instances pré-allouées
	var instances: Array = montage.conteneur.get_children()
	_verifier("PoolSpawner : pré-allocation de initial_pool_size_per_scene instances", instances.size() == 3 and montage.spawner.get_available_count(_derniere_scene) == 3,
		"%d enfants, %d disponibles" % [instances.size(), montage.spawner.get_available_count(_derniere_scene)])
	_verifier("PoolSpawner : instances pré-allouées masquées et désactivées",
		instances.all(func(n): return not n.visible and n.process_mode == Node.PROCESS_MODE_DISABLED))
	_verifier("PoolSpawner : on_spawn_from_pool pas encore appelé", instances.all(func(n): return n.appels == 0))
	await _demonter(montage)


func _tester_pool_cycle() -> void:
	var scene := _scene_pool("P")
	var montage := _monter(PoolSpawner.new(), _entree_pool([scene], 2), [Vector2(30, 40)])
	await _frames()
	var pool: PoolSpawner = montage.spawner

	pool.spawn()
	var actives := _visibles(montage.conteneur)
	_verifier("PoolSpawner spawn : une instance prise dans le pool", pool.get_available_count(scene) == 1 and actives.size() == 1)
	_verifier("PoolSpawner spawn : pas de nouvelle allocation", montage.conteneur.get_child_count() == 2)
	var instance: Node2D = actives[0]
	_verifier("PoolSpawner spawn : instance activée (visible, process_mode INHERIT)", instance.visible and instance.process_mode == Node.PROCESS_MODE_INHERIT)
	_verifier("PoolSpawner spawn : placée sur le marqueur", instance.global_position == Vector2(30, 40))
	_verifier("PoolSpawner spawn : on_spawn_from_pool appelé", instance.appels == 1)

	var espion := _espionner(pool.despawned)
	pool.despawn(instance)
	_verifier("PoolSpawner despawn : instance rendue au pool", pool.get_available_count(scene) == 2)
	_verifier("PoolSpawner despawn : instance masquée et désactivée, pas détruite",
		not instance.visible and instance.process_mode == Node.PROCESS_MODE_DISABLED and not instance.is_queued_for_deletion())
	_verifier("PoolSpawner despawn : signal despawned avec l'instance", espion.n == 1 and espion.args[0] == instance)

	pool.despawn(instance) # avertissement attendu
	_verifier("PoolSpawner : double despawn refusé (pas de doublon dans le pool)", pool.get_available_count(scene) == 2 and espion.n == 1)

	pool.spawn()
	_verifier("PoolSpawner : l'instance rendue est réutilisée", instance.visible and instance.appels == 2)
	await _demonter(montage)


func _tester_pool_epuise() -> void:
	var scene := _scene_pool("P")
	var montage := _monter(PoolSpawner.new(), _entree_pool([scene], 1), [Vector2.ZERO])
	await _frames()
	for i in 3:
		montage.spawner.spawn() # avertissements attendus : pool épuisé
	_verifier("PoolSpawner épuisé : agrandissement dynamique", _visibles(montage.conteneur).size() == 3 and montage.conteneur.get_child_count() == 3)
	_verifier("PoolSpawner épuisé : aucune instance disponible", montage.spawner.get_available_count(scene) == 0)

	for instance in montage.conteneur.get_children():
		montage.spawner.despawn(instance)
	_verifier("PoolSpawner : les instances allouées dynamiquement reviennent aussi au pool", montage.spawner.get_available_count(scene) == 3)
	await _demonter(montage)


func _tester_pool_objet_etranger() -> void:
	var montage := _monter(PoolSpawner.new(), _entree_pool([_scene_pool("P")], 1), [Vector2.ZERO])
	await _frames()
	var etranger: Node2D = _ajouter(Node2D.new())
	montage.spawner.despawn(etranger) # avertissement attendu
	_verifier("PoolSpawner : despawn d'un objet étranger → destruction standard", etranger.is_queued_for_deletion())
	montage.spawner.despawn(null) # erreur attendue
	_verifier("PoolSpawner : despawn(null) sans plantage", true)
	await _demonter(montage)


func _tester_pool_instance_liberee() -> void:
	var scene := _scene_pool("P")
	var montage := _monter(PoolSpawner.new(), _entree_pool([scene], 2), [Vector2.ZERO])
	await _frames()
	# Une instance du pool est libérée par un autre code : le pool doit l'ignorer.
	montage.conteneur.get_child(montage.conteneur.get_child_count() - 1).free()
	montage.spawner.spawn()
	montage.spawner.spawn()
	var actives := _visibles(montage.conteneur)
	_verifier("PoolSpawner : instance libérée de l'extérieur ignorée, spawns valides", actives.size() == 2 and actives.all(func(n): return is_instance_valid(n)))
	await _demonter(montage)


func _tester_pool_entree_simple() -> void:
	var scene := _scene_pool("P")
	var montage := _monter(PoolSpawner.new(), _entree([scene], SpawnEntry.SELECTION_SEQUENTIAL), [Vector2.ZERO])
	await _frames()
	_verifier("PoolSpawner + SpawnEntry simple : aucune pré-allocation", montage.conteneur.get_child_count() == 0 and montage.spawner.get_available_count(scene) == 0)
	montage.spawner.spawn() # avertissement attendu : pool épuisé
	_verifier("PoolSpawner + SpawnEntry simple : allocation à la demande", _visibles(montage.conteneur).size() == 1)
	await _demonter(montage)


func _tester_pool_liberation() -> void:
	var montage := _monter(PoolSpawner.new(), _entree_pool([_scene_pool("P")], 3), [Vector2.ZERO])
	await _frames()
	montage.spawner.spawn()
	montage.spawner.free()
	await _frames()
	_verifier("PoolSpawner libéré : instances inactives libérées, l'instance active reste en jeu",
		montage.conteneur.get_child_count() == 1 and montage.conteneur.get_child(0).visible, "%d enfants restants" % montage.conteneur.get_child_count())
	montage.conteneur.queue_free()
	await _frames()


# 🛠️ Utilitaires

var _derniere_scene: PackedScene


func _scene(id: String) -> PackedScene:
	var racine := Node2D.new()
	racine.set_meta("id", id)
	return _empaqueter(racine)


func _scene_pool(id: String) -> PackedScene:
	var racine := Node2D.new()
	racine.set_script(_script_objet_pool)
	racine.set_meta("id", id)
	_derniere_scene = _empaqueter(racine)
	return _derniere_scene


func _entree(scenes: Array, mode: SpawnEntry.SelectionMode) -> SpawnEntry:
	var entree := SpawnEntry.new()
	entree.scenes = scenes
	entree.scenes_selection_mode = mode
	return entree


func _entree_pool(scenes: Array, taille: int) -> SpawnPoolEntry:
	var entree := SpawnPoolEntry.new()
	entree.scenes = scenes
	entree.initial_pool_size_per_scene = taille
	return entree


## Monte un spawner avec une entrée, des marqueurs enfants (M0, M1...) et un conteneur comme parent_node.
func _monter(spawner: Spawner, entree: SpawnEntry, positions: Array) -> Dictionary:
	var conteneur := Node2D.new()
	_ajouter(conteneur)
	spawner.parent_node = conteneur
	spawner.entries = [entree]
	for i in positions.size():
		var marqueur := Marker2D.new()
		marqueur.name = "M%d" % i
		marqueur.position = positions[i]
		spawner.add_child(marqueur)
	_ajouter(spawner)
	return {"spawner": spawner, "conteneur": conteneur}


func _demonter(montage: Dictionary) -> void:
	montage.spawner.queue_free()
	montage.conteneur.queue_free()
	await _frames()


func _ids(conteneur: Node) -> Array:
	return conteneur.get_children().map(func(n): return n.get_meta("id"))


func _visibles(conteneur: Node) -> Array:
	return conteneur.get_children().filter(func(n): return n.visible)


func _aller_retour(ressource: Resource) -> Resource:
	return ressource.duplicate(true)
