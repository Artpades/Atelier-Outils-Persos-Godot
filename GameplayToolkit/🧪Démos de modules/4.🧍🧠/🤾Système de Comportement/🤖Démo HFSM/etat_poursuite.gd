extends State

## Poursuite : fonce vers l'intrus.


func _on_enter() -> void:
	get_agent().vitesse = 170.0
	get_agent().couleur = Color(1.0, 0.7, 0.2)


func _on_update(_delta: float) -> void:
	get_agent().destination = get_agent().intrus
