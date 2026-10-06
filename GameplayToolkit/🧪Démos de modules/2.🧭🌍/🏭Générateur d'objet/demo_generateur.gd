extends Node2D

## 🏭 Démo du module générateur d'objet : Spawner et PoolSpawner côte à côte.
##
## - Au lancement, exécute la suite de tests automatiques du module (suite_tests_generateur.gd).
## - À gauche, un Spawner instancie (puis détruit) des objets ; à droite, un PoolSpawner les recycle :
##   comparer le nombre d'instances allouées. Les modes de sélection des scènes et des positions se
##   changent en direct. Les positions sont les marqueurs enfants de chaque spawner (repli automatique).

const Panneau := preload("../panneau_demo.gd")
const SuiteTests := preload("suite_tests_generateur.gd")
const MODES := ["Séquentiel", "Aléatoire", "Toutes"]

@onready var panneau: Panneau = $Interface/Panneau
@onready var spawner: Spawner = $Spawner
@onready var pool: PoolSpawner = $PoolSpawner
@onready var objets_spawner: Node2D = $ObjetsSpawner
@onready var objets_pool: Node2D = $ObjetsPool

var _stats: Label
var _detruits := 0
var _despawns := 0


func _ready() -> void:
	objets_spawner.child_exiting_tree.connect(func(_n): _detruits += 1)
	pool.despawned.connect(func(_instance): _despawns += 1)
	spawner.spawned.connect(func(): panneau.journaliser("📡 Spawner.spawned"))
	pool.spawned.connect(func(): panneau.journaliser("📡 PoolSpawner.spawned"))

	_ajouter_commandes("🏭 Spawner (instancie / détruit)", spawner)
	_ajouter_commandes("♻️ PoolSpawner (recycle)", pool)
	panneau.ajouter_bouton("Tout rendre au pool", _tout_rendre)

	panneau.ajouter_section("📊 Instances")
	_stats = panneau.ajouter_etiquette()

	panneau.ajouter_section("🧪 Tests")
	panneau.ajouter_bouton("Relancer les tests automatiques", _lancer_tests)
	_lancer_tests()


func _process(_delta: float) -> void:
	var entree: SpawnPoolEntry = pool.entries[0]
	var disponibles := PackedStringArray()
	for scene in entree.scenes:
		disponibles.append("%s %d" % [scene.resource_path.get_file().get_basename().trim_prefix("objet_"), pool.get_available_count(scene)])
	_stats.text = "Spawner : %d actives, %d détruites depuis le début\nPoolSpawner : %d actives / %d allouées au total, %d despawns\nDisponibles dans le pool : %s" % [
		objets_spawner.get_child_count(), _detruits,
		objets_pool.get_children().filter(func(n): return n.visible).size(), objets_pool.get_child_count(), _despawns,
		", ".join(disponibles)]


func _draw() -> void:
	# Marqueurs de position et nom de chaque spawner (les Marker2D sont invisibles en jeu)
	var police := ThemeDB.fallback_font
	for zone in [spawner, pool]:
		var marqueurs: Array = zone.get_children().filter(func(n): return n is Marker2D)
		for marqueur in marqueurs:
			draw_circle(marqueur.global_position, 8.0, Color(1, 1, 1, 0.5))
		draw_string(police, marqueurs[0].global_position + Vector2(-10, 60), zone.name, HORIZONTAL_ALIGNMENT_LEFT, -1, 22, Color(1, 1, 1, 0.6))


func _ajouter_commandes(titre: String, cible: Spawner) -> void:
	var entree: SpawnEntry = cible.entries[0]
	panneau.ajouter_section(titre)
	panneau.ajouter_options("Scènes", MODES, entree.scenes_selection_mode, func(index: int): entree.scenes_selection_mode = index as SpawnEntry.SelectionMode)
	panneau.ajouter_options("Positions", MODES, entree.positions_selection_mode, func(index: int): entree.positions_selection_mode = index as SpawnEntry.SelectionMode)
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Spawn", cible.spawn, rangee)
	panneau.ajouter_bouton("Rafale ×10", _rafale.bind(cible), rangee)


func _rafale(cible: Spawner) -> void:
	for i in 10:
		cible.spawn()
		await get_tree().create_timer(0.05).timeout


func _tout_rendre() -> void:
	for instance in objets_pool.get_children():
		if instance.visible:
			pool.despawn(instance)


func _lancer_tests() -> void:
	panneau.lancer_tests("Tests du module générateur d'objet", SuiteTests, self)
