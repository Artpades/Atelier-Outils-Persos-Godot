extends State

## Patrouille : va de point en point le long du chemin de ronde.

@export var points: Array[Vector2] = [Vector2(120, 80), Vector2(560, 80), Vector2(560, 280), Vector2(120, 280)]
var _index := 0


func _on_enter() -> void:
	get_agent().vitesse = 90.0
	get_agent().couleur = Color(0.35, 0.75, 1.0)
	get_agent().destination = points[_index]


func _on_update(_delta: float) -> void:
	var agent = get_agent() # Node2D du garde (non typé : propriétés de son script)
	if agent.position.distance_to(points[_index]) < 4.0:
		_index = (_index + 1) % points.size()
		agent.destination = points[_index]
