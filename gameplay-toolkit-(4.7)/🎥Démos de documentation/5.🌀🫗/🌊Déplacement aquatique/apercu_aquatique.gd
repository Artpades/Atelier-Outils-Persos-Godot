extends "res://🎥Démos de documentation/5.🌀🫗/apercu_mouvement.gd"

## 🌊 Aperçu interactif : un héros qui marche et nage (SwimMovement2D), un bassin (WaterArea2D) où l'on lâche
## des caisses ; style de nage, vitesses, flottaison et courant.

var _eau: WaterArea2D


func titre() -> String:
	return "Déplacement aquatique"


func aide() -> String:
	return "Flèches pour nager (haut / bas : remonter / plonger), Espace : coup de nage ou sortie près de la surface."


func composants() -> Dictionary:
	return {"Sol": "GroundMovement2D", "Nage": "SwimMovement2D"}


func construire_niveau() -> void:
	arene()
	bloc(Rect2(-200, 200, 650, 200), Formes.SOL.darkened(0.25))
	_eau = eau(Rect2(-200, -40, 650, 240))
	bloc(Rect2(-200, 200, 650, 20))


func reglages() -> void:
	var nage: SwimMovement2D = heros.get_node("Nage")
	nage.visual_path = NodePath("../Visuel")
	panneau.ajouter_options("style", PackedStringArray(["STYLE_SURFACE", "STYLE_DIRECTIONAL", "STYLE_FISH"]), 1, func(i: int): nage.style = i; _maj_code())
	for reglage in [["swim_speed", 40.0, 500.0, 10.0], ["turn_speed", 0.5, 20.0, 0.5], ["stroke_velocity", 0.0, 500.0, 10.0], ["jump_out_velocity", 100.0, 800.0, 10.0]]:
		var nom: String = reglage[0]
		panneau.ajouter_curseur(nom, reglage[1], reglage[2], reglage[3], nage.get(nom), func(v: float): nage.set(nom, v); _maj_code())
	panneau.ajouter_section("Eau")
	panneau.ajouter_curseur("buoyancy (caisses)", 0.0, 5.0, 0.1, _eau.buoyancy, func(v: float): _eau.buoyancy = v; _maj_code())
	panneau.ajouter_curseur("current.x", -300.0, 300.0, 10.0, 0.0, func(v: float): _eau.current = Vector2(v, 0); _maj_code())
	panneau.ajouter_bouton("Lâcher une caisse", _jouer)
	_maj_code()


func _jouer() -> void:
	var corps := RigidBody2D.new()
	corps.position = Vector2(randf_range(-120, 380), -250)
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = Vector2(36, 36)
	corps.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(Vector2(36, 36))
	dessin.color = Color("a0714f")
	corps.add_child(dessin)
	vue.add_child(corps)


func _maj_code() -> void:
	var nage: SwimMovement2D = heros.get_node("Nage")
	afficher_code("$SwimMovement2D.style = %d · swim_speed = %d · turn_speed = %s\n$WaterArea2D.buoyancy = %s · current = %s" % [nage.style, nage.swim_speed, nage.turn_speed, _eau.buoyancy, _eau.current])
