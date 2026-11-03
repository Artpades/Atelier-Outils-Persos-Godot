extends BTAction

## Errer : choisit un point au hasard dans la zone et s'y rend (RUNNING), puis SUCCESS.


func _on_enter() -> void:
	var agent = get_agent()
	agent.couleur = Color(0.45, 0.85, 0.45)
	agent.destination = Vector2(randf_range(40, 670), randf_range(30, 220))


func _tick(_delta: float) -> int:
	var agent = get_agent()
	return SUCCESS if agent.position.distance_to(agent.destination) < 4.0 else RUNNING
