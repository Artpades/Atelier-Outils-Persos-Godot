extends "res://🎥Démos de documentation/5.🌀🫗/apercu_mouvement.gd"

## 🧗 Aperçu interactif de WallMovement2D : un puits pour enchaîner les sauts muraux, une paroi pour glisser
## ou grimper.


func titre() -> String:
	return "Déplacement mural"


func aide() -> String:
	return "Sauter contre un mur en poussant vers lui : glissade ; Espace : saut mural. Avec can_climb : haut / bas pour grimper."


func composants() -> Dictionary:
	return {"Sol": "GroundMovement2D", "Mur": "WallMovement2D"}


func construire_niveau() -> void:
	arene()
	bloc(Rect2(-110, -500, 30, 700))
	bloc(Rect2(80, -500, 30, 560))
	bloc(Rect2(380, -300, 30, 500))
	depart = Vector2(-300, 120)


func reglages() -> void:
	var mur: WallMovement2D = heros.get_node("Mur")
	for reglage in [["slide_speed", 0.0, 400.0, 10.0], ["jump_away_velocity", 0.0, 700.0, 10.0], ["jump_up_velocity", 0.0, 800.0, 10.0], ["climb_speed", 20.0, 400.0, 10.0]]:
		var nom: String = reglage[0]
		panneau.ajouter_curseur(nom, reglage[1], reglage[2], reglage[3], mur.get(nom), func(v: float): mur.set(nom, v); _maj_code())
	panneau.ajouter_case("can_climb", false, func(v: bool): mur.can_climb = v; _maj_code())
	panneau.ajouter_case("require_input_toward_wall", true, func(v: bool): mur.require_input_toward_wall = v; _maj_code())
	mur.wall_jumped.connect(func(): journaliser("wall_jumped"))
	_maj_code()


func _process(_delta: float) -> void:
	var mur: WallMovement2D = heros.get_node("Mur")
	afficher_code("$WallMovement2D.slide_speed = %d · can_climb = %s\nis_sliding() = %s · is_climbing() = %s · is_touching_wall() = %s" % [mur.slide_speed, mur.can_climb, mur.is_sliding(), mur.is_climbing(), mur.is_touching_wall()])


func _maj_code() -> void:
	pass
