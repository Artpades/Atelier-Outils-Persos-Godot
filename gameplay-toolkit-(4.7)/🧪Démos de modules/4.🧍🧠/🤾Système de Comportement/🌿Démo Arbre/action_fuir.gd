extends BTAction

## Fuir : s'éloigne du danger tant qu'il est à moins de distance_sure (RUNNING), puis SUCCESS.

@export var distance_sure := 200.0


func _on_enter() -> void:
	get_agent().couleur = Color(1, 0.4, 0.4)
	get_agent().vitesse = 190.0


func _on_exit() -> void:
	get_agent().vitesse = 120.0


func _tick(_delta: float) -> int:
	var agent = get_agent()
	var eloignement: Vector2 = (agent.position - agent.danger_position).normalized()
	agent.destination = (agent.position + eloignement * 80.0).clamp(Vector2(20, 20), Vector2(692, 230))
	return RUNNING if get_blackboard().get_value("danger") < distance_sure else SUCCESS
