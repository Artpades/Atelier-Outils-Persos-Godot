extends State

## Étourdi : immobile et gris. La transition vers Patrouille (min_time_in_state = 2 s) le réveille.


func _on_enter() -> void:
	get_agent().vitesse = 0.0
	get_agent().couleur = Color(0.55, 0.55, 0.6)
