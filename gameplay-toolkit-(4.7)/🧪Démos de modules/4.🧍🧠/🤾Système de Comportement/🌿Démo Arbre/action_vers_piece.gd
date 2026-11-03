extends BTAction

## Aller vers la pièce la plus proche : RUNNING en chemin, SUCCESS une fois dessus, FAILURE s'il n'y en a plus.


func _on_enter() -> void:
	get_agent().couleur = Color(1, 0.85, 0.3)


func _tick(_delta: float) -> int:
	var agent = get_agent()
	if agent.pieces.is_empty():
		return FAILURE
	agent.destination = agent.piece_la_plus_proche()
	return SUCCESS if agent.position.distance_to(agent.destination) < 4.0 else RUNNING
