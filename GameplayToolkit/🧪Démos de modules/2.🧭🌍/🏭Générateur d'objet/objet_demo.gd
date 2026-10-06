extends Node2D

## Objet de la démo du générateur : jaillit, retombe en tournant et s'efface au bout de DUREE_DE_VIE.
##
## - Créé par le Spawner : détruit (queue_free) en fin de vie.
## - Créé par le PoolSpawner : réinitialisé par on_spawn_from_pool() à chaque sortie du pool, et rendu au
##   pool (despawn) en fin de vie. Inactif dans le pool, son _process ne tourne pas (process_mode DISABLED).

const DUREE_DE_VIE := 2.5
const GRAVITE := 900.0

var _vitesse := Vector2.ZERO
var _vie := DUREE_DE_VIE
## PoolSpawner d'origine (nul si l'objet vient d'un Spawner simple)
var _pool: PoolSpawner


func _ready() -> void:
	_reinitialiser()


## Appelée par le PoolSpawner à chaque réactivation.
func on_spawn_from_pool() -> void:
	_pool = get_tree().get_first_node_in_group(&"pool_spawner_demo")
	_reinitialiser()


func _reinitialiser() -> void:
	_vie = DUREE_DE_VIE
	_vitesse = Vector2(randf_range(-160.0, 160.0), randf_range(-620.0, -380.0))
	rotation = 0.0
	modulate.a = 1.0


func _process(delta: float) -> void:
	_vitesse.y += GRAVITE * delta
	position += _vitesse * delta
	rotation += 3.0 * delta
	_vie -= delta
	modulate.a = clampf(_vie / 0.5, 0.0, 1.0)

	if _vie <= 0.0:
		if _pool:
			_pool.despawn(self)
		else:
			queue_free()
