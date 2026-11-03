extends "res://🎥Démos de documentation/5.🌀🫗/apercu_mouvement.gd"

## 🦶 Aperçu interactif de GroundMovement2D : plateformes, double saut, temps de grâce, hauteur variable,
## vitesse, accélération, contrôle en l'air, gravité, et la vue de dessus.


func titre() -> String:
	return "Déplacement au sol (GroundMovement2D)"


func aide() -> String:
	return "Clavier : flèches, Espace (saut, maintenir = plus haut), Maj (course). En vue de dessus : les quatre flèches."


func construire_niveau() -> void:
	arene()
	bloc(Rect2(-480, 60, 220, 24))
	bloc(Rect2(-120, -20, 200, 24))
	bloc(Rect2(220, 80, 200, 24))
	bloc(Rect2(480, 140, 60, 60))


func reglages() -> void:
	var sol: GroundMovement2D = heros.get_node("Sol")
	sol.visual_path = NodePath("../Visuel")
	panneau.ajouter_options("mode", PackedStringArray(["MODE_PLATFORMER", "MODE_TOP_DOWN"]), 0, func(i: int): sol.mode = i; _maj_code())
	for reglage in [["max_speed", 50.0, 600.0, 10.0], ["acceleration", 100.0, 8000.0, 100.0], ["deceleration", 100.0, 8000.0, 100.0],
			["air_control", 0.0, 1.0, 0.05], ["jump_velocity", 100.0, 900.0, 10.0], ["max_jumps", 0.0, 4.0, 1.0], ["coyote_time", 0.0, 0.5, 0.01],
			["jump_cut", 0.0, 1.0, 0.05], ["gravity_scale", -1.0, 3.0, 0.1], ["run_multiplier", 1.0, 3.0, 0.1]]:
		var nom: String = reglage[0]
		panneau.ajouter_curseur(nom, reglage[1], reglage[2], reglage[3], sol.get(nom), func(v: float):
			sol.set(nom, int(v) if nom == "max_jumps" else v)
			if nom == "gravity_scale":
				heros.up_direction = Vector2.UP if v >= 0.0 else Vector2.DOWN
			_maj_code())
	_maj_code()


func _maj_code() -> void:
	var sol: GroundMovement2D = heros.get_node("Sol")
	afficher_code("$GroundMovement2D.mode = %d · max_speed = %d · max_jumps = %d\n$GroundMovement2D.jump_velocity = %d · jump_cut = %s · coyote_time = %s\n$GroundMovement2D.air_control = %s · gravity_scale = %s" % [
		sol.mode, sol.max_speed, sol.max_jumps, sol.jump_velocity, snappedf(sol.jump_cut, 0.01), snappedf(sol.coyote_time, 0.01), snappedf(sol.air_control, 0.01), snappedf(sol.gravity_scale, 0.1)])
