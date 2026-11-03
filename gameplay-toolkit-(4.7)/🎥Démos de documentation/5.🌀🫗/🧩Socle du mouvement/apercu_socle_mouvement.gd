extends "res://🎥Démos de documentation/5.🌀🫗/apercu_mouvement.gd"

## 🧩 Aperçu interactif du Socle du mouvement : un héros avec un GroundMovement2D, piloté au clavier ;
## réglages de la base (jump_buffer_time, active) et de la course.


func titre() -> String:
	return "Socle du mouvement"


func aide() -> String:
	return "Cliquer dans la page, puis : flèches pour marcher, Espace pour sauter (maintenir = plus haut), Maj pour courir."


func construire_niveau() -> void:
	arene()
	bloc(Rect2(-120, 100, 200, 24))
	bloc(Rect2(160, 20, 160, 24))
	bloc(Rect2(380, 120, 80, 80))


func reglages() -> void:
	var sol: MovementComponent = heros.get_node("Sol")
	panneau.ajouter_case("Sol.active", true, func(v: bool): sol.active = v; _maj_code())
	panneau.ajouter_curseur("jump_buffer_time", 0.0, 0.5, 0.01, sol.jump_buffer_time, func(v: float): sol.jump_buffer_time = v; _maj_code())
	panneau.ajouter_curseur("max_jumps", 0.0, 3.0, 1.0, 1.0, func(v: float): sol.set("max_jumps", int(v)); _maj_code())
	_maj_code()


func _maj_code() -> void:
	var sol: MovementComponent = heros.get_node("Sol")
	afficher_code("sol.move(Vector2(Input.get_axis(\"ui_left\", \"ui_right\"), 0))\nif Input.is_action_just_pressed(\"ui_accept\"): sol.jump()\nsol.active = %s · jump_buffer_time = %s" % [sol.active, snappedf(sol.jump_buffer_time, 0.01)])
