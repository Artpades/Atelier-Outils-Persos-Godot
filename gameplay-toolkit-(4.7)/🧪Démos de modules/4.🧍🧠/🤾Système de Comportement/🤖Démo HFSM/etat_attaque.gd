extends State

## Attaque : s'arrête et clignote ; compte les coups dans le tableau noir.

var _temps := 0.0


func _on_enter() -> void:
	get_agent().vitesse = 0.0
	_temps = 0.0


func _on_update(delta: float) -> void:
	_temps += delta
	get_agent().couleur = Color(1, 0.25, 0.25) if fmod(_temps, 0.3) < 0.15 else Color(1, 0.6, 0.6)
	if _temps >= 0.6:
		_temps = 0.0
		get_blackboard().set_value("coups", get_blackboard().get_value("coups", 0) + 1)
