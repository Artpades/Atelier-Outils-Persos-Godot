extends BTAction

## Se reposer : immobile, regagne de l'énergie jusqu'à 100 (RUNNING), puis SUCCESS.


func _on_enter() -> void:
	get_agent().couleur = Color(0.5, 0.6, 1.0)
	get_agent().destination = get_agent().position


func _tick(delta: float) -> int:
	var tableau := get_blackboard()
	tableau.set_value("energie", minf(100.0, tableau.get_value("energie") + 35.0 * delta))
	return SUCCESS if tableau.get_value("energie") >= 100.0 else RUNNING
