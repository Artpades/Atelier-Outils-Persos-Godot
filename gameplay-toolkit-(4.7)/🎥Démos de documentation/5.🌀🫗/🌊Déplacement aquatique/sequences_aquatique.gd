extends "res://🎥Démos de documentation/5.🌀🫗/sequences_mouvement.gd"

## 🌊 Plans filmés de la page Déplacement aquatique (mouvement/deplacement-aquatique.html) :
## WaterArea2D / 3D et SwimMovement2D / 3D.


func titre_module() -> String:
	return "🌊 Aquatique · WaterArea, SwimMovement"


func plans() -> Dictionary:
	return {
		"mode-emploi-tuto-eau": _tuto_eau,
		"mode-emploi-tuto-nage": _tuto_nage,
		"classe-WaterArea2D": _classe_eau_2d,
		"classe-WaterArea3D": _classe_eau_3d,
		"classe-SwimMovement2D": _classe_nage_2d,
		"classe-SwimMovement3D": _classe_nage_3d,
		"WaterArea2D.buoyancy": _buoyancy,
		"WaterArea2D.linear_drag": _linear_drag,
		"WaterArea2D.angular_drag": _angular_drag,
		"WaterArea2D.debug_mode": _eau_debug,
		"SwimMovement2D.swim_speed": _swim_speed,
		"SwimMovement2D.turn_speed": _turn_speed,
		"SwimMovement2D.stroke_velocity": _stroke,
		"SwimMovement2D.jump_out_velocity": _jump_out,
		"SwimMovement2D.align_to_heading": _align,
		"SwimMovement2D.cruise_ratio": _poisson,
		"SwimMovement2D.buoyancy": _flottaison,
		"ex-styles": _ex_styles,
		"ex-oxygene": _ex_oxygene,
		"ex-plouf": _ex_plouf,
		"ex-3d": _ex_3d,
		"ex-riviere": _ex_riviere,
		"ex-niveau": _ex_niveau,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi-tuto-eau": "Des caisses (RigidBody2D) tombent dans la WaterArea2D, s'enfoncent, remontent et dérivent avec le courant (80, 0) ; <code>debug_mode</code> montre la surface et le courant.",
		"mode-emploi-tuto-nage": "Le héros marche, plonge (contexte « eau » : nage Directionnelle, corps incliné), remonte et saute hors de l'eau.",
		"classe-WaterArea2D": "Les corps rigides flottent, freinent et suivent le courant ; les personnages y nagent avec un SwimMovement2D.",
		"classe-WaterArea3D": "Le même bassin en 3D : caisses qui flottent et dérivent.",
		"classe-SwimMovement2D": "Nage 2D : direction poussée, élan, inclinaison du visuel, coups de nage et sortie de l'eau.",
		"classe-SwimMovement3D": "Nage 3D : avancer, plonger avec la commande verticale, remonter, sortir.",
		"WaterArea2D.buoyancy": "<code>buoyancy</code> 0,5 (coule), 2 (défaut, mi-immergée) et 4 (flotte haut).",
		"WaterArea2D.linear_drag": "<code>linear_drag</code> 0,5, 3 (défaut) et 8 : la caisse entre dans l'eau plus ou moins freinée.",
		"WaterArea2D.angular_drag": "Caisses lancées en tournant : <code>angular_drag</code> 0 et 3 (défaut).",
		"WaterArea2D.debug_mode": "<code>debug_mode</code> : contour, ligne de surface, flèche de courant, nombre de corps.",
		"SwimMovement2D.swim_speed": "<code>swim_speed</code> 80, 160 (défaut) et 320 px/s.",
		"SwimMovement2D.turn_speed": "Demi-tours sous l'eau : <code>turn_speed</code> 1,5, 5 (défaut) et 15.",
		"SwimMovement2D.stroke_velocity": "Coups de nage répétés (saut sous l'eau) : <code>stroke_velocity</code> 160 et <code>stroke_cooldown</code> 0,35 s entre deux coups.",
		"SwimMovement2D.jump_out_velocity": "Saut près de la surface : <code>jump_out_velocity</code> 250, 380 (défaut) et 600 ; <code>surface_tolerance</code> = profondeur maximale pour sortir.",
		"SwimMovement2D.align_to_heading": "<code>align_to_heading</code> true : le visuel s'incline vers la direction de nage ; false : il reste droit.",
		"SwimMovement2D.cruise_ratio": "Style Poisson : nage toujours vers l'avant (vitesse de croisière <code>cruise_ratio</code>), ondulation du corps (<code>undulation_*</code>).",
		"SwimMovement2D.buoyancy": "Style Flottaison : remontée automatique à <code>float_depth</code> sous la surface, freinage <code>drag</code>.",
		"ex-styles": "Les trois styles côte à côte, mêmes commandes : Directionnel, Poisson, Flottaison.",
		"ex-oxygene": "La jauge d'oxygène se vide tant que le contexte est « eau », se remplit dehors.",
		"ex-plouf": "<code>jumped_out</code> : éclaboussure en sortant de l'eau ; <code>stroked</code> : bulles à chaque coup de nage.",
		"ex-3d": "Nage 3D avec <code>camera_relative</code> : l'avant de la caméra (baissée) fait plonger.",
		"ex-riviere": "Une rivière (<code>current = (250, 0)</code>, <code>linear_drag = 1</code>) emporte caisses et nageur.",
		"ex-niveau": "<code>monter_eau()</code> agrandit le rectangle vers le haut : la surface monte, les caisses aussi.",
	}


func schemas() -> Dictionary:
	return {
		"surface": ["surface.svg", "Surface et profondeur : <code>get_surface_y()</code> est le bord haut du rectangle (ou <code>surface_offset</code>), <code>get_depth(point)</code> la distance sous la surface ; un corps rigide est immergé au-delà de <code>float_depth</code>."],
	}


func reutilisations() -> Dictionary:
	var meme_3d := "Même réglage qu'en 2D, en mètres (voir la vidéo 2D)."
	var r := {
		"WaterArea2D.current": "mode-emploi-tuto-eau",
		"WaterArea2D.float_depth": "surface",
		"WaterArea2D.surface_offset": "surface",
		"WaterArea2D.get_surface_y": "surface",
		"WaterArea2D.get_depth": "surface",
		"WaterArea2D.find_containing": "surface",
		"SwimMovement2D.style": "ex-styles",
		"SwimMovement2D.acceleration": "SwimMovement2D.swim_speed",
		"SwimMovement2D.run_multiplier": "SwimMovement2D.swim_speed",
		"SwimMovement2D.stroke_cooldown": "SwimMovement2D.stroke_velocity",
		"SwimMovement2D.surface_tolerance": "SwimMovement2D.jump_out_velocity",
		"SwimMovement2D.visual_path": "SwimMovement2D.align_to_heading",
		"SwimMovement2D.undulation_amplitude": "SwimMovement2D.cruise_ratio",
		"SwimMovement2D.undulation_frequency": "SwimMovement2D.cruise_ratio",
		"SwimMovement2D.float_depth": "SwimMovement2D.buoyancy",
		"SwimMovement2D.drag": "SwimMovement2D.buoyancy",
		"SwimMovement2D.get_heading": "classe-SwimMovement2D",
		"SwimMovement2D.get_speed": "classe-SwimMovement2D",
		"SwimMovement2D.get_water": "surface",
		"SwimMovement2D.jumped_out": "ex-plouf",
		"SwimMovement2D.stroked": "ex-plouf",
		"SwimMovement3D.camera_relative": "ex-3d",
		"SwimMovement3D.jumped_out": "ex-plouf",
		"SwimMovement3D.stroked": "ex-plouf",
		"SwimMovement3D.get_heading": "classe-SwimMovement3D",
		"SwimMovement3D.get_speed": "classe-SwimMovement3D",
		"SwimMovement3D.get_water": "surface",
	}
	for membre in ["buoyancy", "linear_drag", "angular_drag", "current", "debug_mode", "float_depth", "surface_offset"]:
		r["WaterArea3D." + membre] = [r.get("WaterArea2D." + membre, "WaterArea2D." + membre), meme_3d]
	for membre in ["get_surface_y", "get_depth", "find_containing"]:
		r["WaterArea3D." + membre] = "surface"
	for membre in ["style", "swim_speed", "acceleration", "run_multiplier", "turn_speed", "stroke_velocity", "stroke_cooldown", "jump_out_velocity",
			"surface_tolerance", "align_to_heading", "visual_path", "cruise_ratio", "undulation_amplitude", "undulation_frequency", "buoyancy", "float_depth", "drag"]:
		var source = r.get("SwimMovement2D." + membre, "SwimMovement2D." + membre)
		r["SwimMovement3D." + membre] = [source, meme_3d]
	return r


## Caisse (RigidBody2D) de côté `cote`, lâchée en `position`.
func caisse(parent: Node, position: Vector2, couleur: Color = Color("a0714f"), cote: float = 40.0) -> RigidBody2D:
	var corps := RigidBody2D.new()
	corps.position = position
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = Vector2(cote, cote)
	corps.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(Vector2(cote, cote))
	dessin.color = couleur
	corps.add_child(dessin)
	var diagonale := Line2D.new()
	diagonale.points = PackedVector2Array([Vector2(-cote / 2.0, -cote / 2.0), Vector2(cote / 2.0, cote / 2.0)])
	diagonale.width = 3
	diagonale.default_color = couleur.darkened(0.3)
	corps.add_child(diagonale)
	parent.add_child(corps)
	return corps


## Niveau type : sol à gauche, bassin profond au centre-droite.
func _bassin(vue: Node, reglages: Dictionary = {}) -> WaterArea2D:
	arene(vue)
	bloc(vue, Rect2(-200, 200, 650, 200), Formes.SOL.darkened(0.25))
	var zone := eau(vue, Rect2(-200, -40, 650, 240), reglages)
	bloc(vue, Rect2(-200, 200, 650, 20))
	return zone


func _nageur(vue: Node, position: Vector2, style: int = SwimMovement2D.STYLE_DIRECTIONAL, reglages_nage: Dictionary = {}, couleur: Color = VARIANTES[0]) -> CharacterBody2D:
	var r := reglages_nage.duplicate()
	r["style"] = style
	r["visual_path"] = NodePath("../Visuel")
	return heros_2d(vue, position, {"Sol": "GroundMovement2D", "Nage": "SwimMovement2D", "Directeur": "MovementDirector"}, {"Nage": r}, couleur)


## Colonnes de bassins (un nageur par valeur), mêmes commandes pour tous.
func _bassins(propriete: String, valeurs: Array, textes: Array, communs: Dictionary = {}, style: int = SwimMovement2D.STYLE_DIRECTIONAL) -> Array:
	var vue := monde_2d()
	arene(vue)
	var directeurs := []
	var n := valeurs.size()
	var pas := 1000.0 / n
	for i in n:
		var x := -500.0 + pas * i
		eau(vue, Rect2(x + 10, -120, pas - 20, 320))
		if i > 0:
			bloc(vue, Rect2(x - 5, -140, 10, 340))
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		var corps := _nageur(vue, Vector2(x + pas / 2.0, 0), style, reglages, VARIANTES[i % 4])
		directeurs.append(corps.get_node("Directeur"))
		etiquette(Vector2(576 + x + pas / 2.0, 545), textes[i], 16, VARIANTES[i % 4])
	return directeurs


func _tous(cibles: Array, etapes: Array) -> void:
	for i in cibles.size() - 1:
		piloter(cibles[i], etapes)
	await piloter(cibles[cibles.size() - 1], etapes)


## Colonnes de bassins où l'on lâche une caisse par valeur de `propriete` de la WaterArea2D.
func _caisses_colonnes(propriete: String, valeurs: Array, textes: Array, hauteur_chute: float = -150.0, spin: float = 0.0) -> void:
	var vue := monde_2d()
	arene(vue)
	var n := valeurs.size()
	var pas := 1000.0 / n
	var depart := []
	for i in n:
		var x := -500.0 + pas * i
		eau(vue, Rect2(x + 10, 0, pas - 20, 200), {propriete: valeurs[i]})
		if i > 0:
			bloc(vue, Rect2(x - 5, -40, 10, 240))
		etiquette(Vector2(576 + x + pas / 2.0, 545), textes[i], 16, VARIANTES[i % 4])
		depart.append(Vector2(x + pas / 2.0, hauteur_chute))
	debug(vue)
	for essai in 2:
		var caisses := []
		for i in n:
			var c := caisse(vue, depart[i], VARIANTES[i % 4].darkened(0.2))
			c.angular_velocity = spin
			caisses.append(c)
		await secondes(4.0)
		for c in caisses:
			c.queue_free()


# ⚙️ Mode d'emploi

func _tuto_eau() -> void:
	sujet("A. Un bassin où les caisses flottent")
	var vue := monde_2d()
	arene(vue)
	var zone := eau(vue, Rect2(-400, -20, 800, 220), {"current": Vector2(80, 0)})
	debug(vue)
	code("$WaterArea2D.current = Vector2(80, 0)\n$WaterArea2D.debug_mode = true")
	for i in 5:
		caisse(vue, Vector2(-330 + i * 70, -200 - i * 40), Color("a0714f").lightened(i * 0.08))
		await secondes(0.5)
	await secondes(5.0)
	zone.name = "Eau"


func _tuto_nage() -> void:
	sujet("B. Un personnage qui nage")
	var vue := monde_2d()
	_bassin(vue)
	var heros := _nageur(vue, Vector2(-500, 150))
	debug(vue)
	arbre(Vector2(40, 140), PackedStringArray(["CharacterBody2D", "  MovementDirector", "  GroundMovement2D", "  SwimMovement2D"]))
	code("$MovementDirector.move(Input.get_vector(\"ui_left\", \"ui_right\", \"ui_up\", \"ui_down\"))\nif Input.is_action_just_pressed(\"ui_accept\"): $MovementDirector.jump()")
	var d: MovementDirector = heros.get_node("Directeur")
	await piloter(d, [[1.0, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [1.0, Vector2(1, 1)], [0.6, Vector2(1, 0), "saut"], [1.2, Vector2(-1, 0.3)],
		[1.4, Vector2(0.4, -1)], [0.6, Vector2(-1, -1), "saut_long"], [1.0, Vector2.LEFT], [0.5, Vector2.ZERO]])


# 💻 API

func _classe_eau_2d() -> void:
	sujet("WaterArea2D : flotter, freiner, emporter")
	var vue := monde_2d()
	_bassin(vue, {"current": Vector2(40, 0)})
	var heros := _nageur(vue, Vector2(-500, 150))
	debug(vue)
	code("$WaterArea2D.buoyancy = 2.0\n$WaterArea2D.current = Vector2(40, 0)")
	for i in 3:
		caisse(vue, Vector2(-100 + i * 150, -250 - i * 60))
	await piloter(heros.get_node("Directeur"), [[1.0, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [2.0, Vector2(1, 0.5)], [1.4, Vector2(0, -1)], [1.0, Vector2.ZERO]])


func _classe_eau_3d() -> void:
	sujet("WaterArea3D : le même bassin en 3D")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -2.5, 0), Vector3(14, 1, 10))
	for p in [Vector3(-7, -1, 0), Vector3(7, -1, 0), Vector3(0, -1, -5), Vector3(0, -1, 5)]:
		bloc_3d(vue, p, Vector3(0.4 if absf(p.x) > 1 else 14, 3, 10 if absf(p.x) > 1 else 0.4), Formes.SOL)
	var zone := WaterArea3D.new()
	zone.position = Vector3(0, -1, 0)
	zone.current = Vector3(0.8, 0, 0)
	var forme := CollisionShape3D.new()
	forme.shape = BoxShape3D.new()
	forme.shape.size = Vector3(14, 2.4, 10)
	zone.add_child(forme)
	Formes.cube(zone, Vector3.ZERO, Color(0.2, 0.45, 0.9, 0.45), Vector3(14, 2.4, 10))
	vue.add_child(zone)
	debug(vue)
	code("$WaterArea3D.buoyancy = 2.0\n$WaterArea3D.current = Vector3(0.8, 0, 0)")
	for i in 5:
		var corps := RigidBody3D.new()
		corps.position = Vector3(-4 + i * 1.6, 3 + i * 0.8, randf_range(-2, 2))
		var f := CollisionShape3D.new()
		f.shape = BoxShape3D.new()
		f.shape.size = Vector3.ONE * 0.8
		corps.add_child(f)
		Formes.cube(corps, Vector3.ZERO, Color("a0714f").lightened(i * 0.06), Vector3.ONE * 0.8)
		vue.add_child(corps)
		await secondes(0.4)
	await secondes(5.0)


func _classe_nage_2d() -> void:
	sujet("SwimMovement2D : nage directionnelle")
	var vue := monde_2d()
	_bassin(vue)
	var heros := _nageur(vue, Vector2(100, 60))
	var nage: SwimMovement2D = heros.get_node("Nage")
	debug(vue)
	var etat := etiquette(Vector2(576, 140), "", 17, ACCENT)
	get_tree().process_frame.connect(func():
		if is_instance_valid(etat):
			etat.text = "get_heading() = %s · get_speed() = %d" % [nage.get_heading().snapped(Vector2(0.01, 0.01)), nage.get_speed()])
	code("$MovementDirector.move(direction)  # haut / bas : remonter / plonger\n$MovementDirector.jump()   # coup de nage, ou sortie près de la surface")
	await piloter(heros.get_node("Directeur"), [[1.2, Vector2(1, 1)], [0.4, Vector2(1, 0), "saut"], [1.2, Vector2(-1, 0)], [0.4, Vector2(-1, -0.5), "saut"],
		[1.4, Vector2(0, -1)], [0.6, Vector2(-1, -1), "saut_long"], [1.2, Vector2.LEFT], [0.5, Vector2.ZERO]])


func _classe_nage_3d() -> void:
	sujet("SwimMovement3D : nage en 3D")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -3.5, 0), Vector3(14, 1, 10))
	var zone := WaterArea3D.new()
	zone.position = Vector3(0, -1.5, 0)
	var forme := CollisionShape3D.new()
	forme.shape = BoxShape3D.new()
	forme.shape.size = Vector3(14, 3.6, 10)
	zone.add_child(forme)
	Formes.cube(zone, Vector3.ZERO, Color(0.2, 0.45, 0.9, 0.35), Vector3(14, 3.6, 10))
	vue.add_child(zone)
	var heros := heros_3d(vue, Vector3(-4, -1, 0), {"Sol": "GroundMovement3D", "Nage": "SwimMovement3D", "Directeur": "MovementDirector"}, {"Nage": {"visual_path": NodePath("../Visuel")}})
	debug(vue)
	var d: MovementDirector = heros.get_node("Directeur")
	code("$MovementDirector.move(Vector2(x, z))\n$MovementDirector.set_vertical_input(-1)  # plonger")
	d.set_vertical_input(-1.0)
	await piloter(d, [[1.4, Vector2(1, 0)]])
	d.set_vertical_input(0.0)
	await piloter(d, [[1.4, Vector2(0, -1)], [1.4, Vector2(-1, 0)]])
	d.set_vertical_input(1.0)
	await piloter(d, [[1.6, Vector2(0, 1)], [0.5, Vector2.ZERO]])
	d.set_vertical_input(0.0)


func _buoyancy() -> void:
	sujet("WaterArea2D.buoyancy : poussée d'Archimède")
	code("$WaterArea2D.buoyancy = 2.0")
	await _caisses_colonnes("buoyancy", [0.5, 2.0, 4.0], ["0,5 : coule", "2 (défaut)", "4 : flotte haut"])


func _linear_drag() -> void:
	sujet("WaterArea2D.linear_drag : freinage dans l'eau")
	code("$WaterArea2D.linear_drag = 3.0")
	await _caisses_colonnes("linear_drag", [0.5, 3.0, 8.0], ["0,5", "3 (défaut)", "8"], -300.0)


func _angular_drag() -> void:
	sujet("WaterArea2D.angular_drag : freinage de la rotation")
	code("$WaterArea2D.angular_drag = 3.0")
	await _caisses_colonnes("angular_drag", [0.0, 3.0], ["0", "3 (défaut)"], -150.0, 12.0)


func _eau_debug() -> void:
	sujet("WaterArea2D.debug_mode")
	var vue := monde_2d()
	arene(vue)
	var zone := eau(vue, Rect2(-400, -20, 800, 220), {"current": Vector2(-60, 0)})
	for i in 3:
		caisse(vue, Vector2(-200 + i * 200, -150))
	code("$WaterArea2D.debug_mode = true")
	legende("debug_mode = false")
	await secondes(2.0)
	zone.debug_mode = true
	legende("debug_mode = true : surface, courant, corps détectés")
	await secondes(4.0)


func _swim_speed() -> void:
	sujet("SwimMovement2D.swim_speed et acceleration")
	code("$SwimMovement2D.swim_speed = 160.0\n$SwimMovement2D.acceleration = 500.0")
	var d := _bassins("swim_speed", [80.0, 160.0, 320.0], ["80", "160 (défaut)", "320"])
	await _tous(d, [[1.4, Vector2(0.4, 1)], [1.4, Vector2(-0.4, -1)], [1.4, Vector2(0.4, 1)], [0.5, Vector2.ZERO]])


func _turn_speed() -> void:
	sujet("SwimMovement2D.turn_speed : rapidité des virages")
	code("$SwimMovement2D.turn_speed = 5.0")
	var d := _bassins("turn_speed", [1.5, 5.0, 15.0], ["1,5", "5 (défaut)", "15"])
	await _tous(d, [[0.8, Vector2(0, 1)], [0.8, Vector2(0, -1)], [0.8, Vector2(0, 1)], [0.8, Vector2(0, -1)], [0.5, Vector2.ZERO]])


func _stroke() -> void:
	sujet("stroke_velocity et stroke_cooldown : coups de nage")
	code("$SwimMovement2D.stroke_velocity = 160.0\n$SwimMovement2D.stroke_cooldown = 0.35")
	var d := _bassins("stroke_velocity", [0.0, 160.0, 400.0], ["0", "160 (défaut)", "400"])
	for i in 6:
		await _tous(d, [[0.4, Vector2(0, 1) if i < 3 else Vector2(0, -1), "saut"]])
	await secondes(0.8)


func _jump_out() -> void:
	sujet("jump_out_velocity et surface_tolerance : sortir de l'eau")
	code("$SwimMovement2D.jump_out_velocity = 380.0\n$SwimMovement2D.surface_tolerance = 14.0")
	var d := _bassins("jump_out_velocity", [250.0, 380.0, 600.0], ["250", "380 (défaut)", "600"])
	for i in 2:
		await _tous(d, [[1.6, Vector2(0, -1)], [1.2, Vector2(0, -1), "saut_long"], [1.0, Vector2.ZERO]])


func _align() -> void:
	sujet("align_to_heading et visual_path")
	code("$SwimMovement2D.align_to_heading = true\n$SwimMovement2D.visual_path = NodePath(\"../Sprite2D\")")
	var d := _bassins("align_to_heading", [true, false], ["true (défaut)", "false"])
	await _tous(d, [[1.0, Vector2(1, 1)], [1.0, Vector2(-1, 0)], [1.0, Vector2(0, -1)], [1.0, Vector2(1, -0.5)], [0.5, Vector2.ZERO]])


func _poisson() -> void:
	sujet("Style Poisson : cruise_ratio, undulation_amplitude, undulation_frequency")
	code("$SwimMovement2D.style = SwimMovement2D.STYLE_FISH\n$SwimMovement2D.cruise_ratio = 0.4")
	var d := _bassins("undulation_amplitude", [0.1, 0.25, 0.6], ["ondulation 0,1", "0,25 (défaut)", "0,6"], {}, SwimMovement2D.STYLE_FISH)
	await _tous(d, [[1.5, Vector2(0, 1)], [1.5, Vector2(0, -1)], [1.5, Vector2.ZERO]])


func _flottaison() -> void:
	sujet("Style Flottaison : buoyancy, float_depth, drag")
	code("$SwimMovement2D.style = SwimMovement2D.STYLE_SURFACE\n$SwimMovement2D.buoyancy = 600.0\n$SwimMovement2D.float_depth = 10.0")
	var d := _bassins("float_depth", [10.0, 40.0, 90.0], ["float_depth 10", "40", "90"], {}, SwimMovement2D.STYLE_SURFACE)
	await _tous(d, [[1.2, Vector2(0, 1)], [1.8, Vector2.ZERO], [1.0, Vector2(0, 1)], [1.6, Vector2.ZERO]])


# 📌 Exemples

func _ex_styles() -> void:
	sujet("Exemple : les trois styles de nage")
	var vue := monde_2d()
	arene(vue)
	var d := []
	var noms := ["STYLE_DIRECTIONAL", "STYLE_FISH", "STYLE_SURFACE"]
	for i in 3:
		var x := -500.0 + i * 333.3
		eau(vue, Rect2(x + 10, -120, 313, 320))
		if i > 0:
			bloc(vue, Rect2(x - 5, -140, 10, 340))
		var corps := _nageur(vue, Vector2(x + 166, 0), [SwimMovement2D.STYLE_DIRECTIONAL, SwimMovement2D.STYLE_FISH, SwimMovement2D.STYLE_SURFACE][i], {}, VARIANTES[i])
		d.append(corps.get_node("Directeur"))
		etiquette(Vector2(576 + x + 166, 545), noms[i], 16, VARIANTES[i])
	code("$SwimMovement2D.style = SwimMovement2D.STYLE_DIRECTIONAL")
	await _tous(d, [[1.2, Vector2(1, 1)], [0.4, Vector2(-1, 0), "saut"], [1.2, Vector2(-1, 0)], [1.2, Vector2(0, -1)], [1.0, Vector2.ZERO]])


func _ex_oxygene() -> void:
	sujet("Exemple : une jauge d'oxygène")
	var vue := monde_2d()
	_bassin(vue)
	var heros := _nageur(vue, Vector2(-500, 150))
	var d: MovementDirector = heros.get_node("Directeur")
	var barre := ProgressBar.new()
	barre.position = Vector2(426, 130)
	barre.size = Vector2(300, 24)
	barre.value = 100
	scene.add_child(barre)
	etiquette(Vector2(576, 110), "Oxygène", 16, Color("9fd8ff"))
	get_tree().physics_frame.connect(func():
		if not is_instance_valid(barre):
			return
		if d.get_current_context() == &"eau":
			barre.value -= 10.0 * get_physics_process_delta_time()
		else:
			barre.value = 100.0)
	code("if $MovementDirector.get_current_context() == &\"eau\":\n\toxygene.add(-10.0 * delta)\nelse:\n\toxygene.fill()")
	await piloter(d, [[1.0, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [2.5, Vector2(0.5, 1)], [1.5, Vector2(-0.2, -1)], [0.6, Vector2(-1, -1), "saut_long"], [1.2, Vector2.LEFT]])


func _ex_plouf() -> void:
	sujet("Exemple : éclaboussure et bulles")
	var vue := monde_2d()
	_bassin(vue)
	var heros := _nageur(vue, Vector2(-500, 150))
	var nage: SwimMovement2D = heros.get_node("Nage")
	var effet := func(position: Vector2, couleur: Color, nombre: int, vers_le_haut: bool) -> void:
		for i in nombre:
			var goutte := Formes.disque(vue, position + Vector2(randf_range(-15, 15), 0), couleur, 5)
			var arrivee := goutte.position + Vector2(randf_range(-40, 40), -randf_range(30, 70) if vers_le_haut else -randf_range(20, 40))
			var animation := goutte.create_tween().set_parallel()
			animation.tween_property(goutte, "position", arrivee, 0.6)
			animation.tween_property(goutte, "modulate:a", 0.0, 0.6)
			animation.chain().tween_callback(goutte.queue_free)
	nage.jumped_out.connect(func(): effet.call(heros.position, Color(0.75, 0.9, 1.0), 10, true))
	nage.stroked.connect(func(): effet.call(heros.position, Color(1, 1, 1, 0.7), 4, false))
	code("$SwimMovement2D.jumped_out.connect(func(): $Eclaboussure.restart())\n$SwimMovement2D.stroked.connect(func(): $SonNage.play())")
	await piloter(heros.get_node("Directeur"), [[1.0, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [1.0, Vector2(1, 1)], [0.4, Vector2(1, 0), "saut"], [0.4, Vector2(0, 0), "saut"],
		[0.4, Vector2(-1, 0), "saut"], [1.4, Vector2(0, -1)], [0.6, Vector2(-1, -1), "saut_long"], [1.0, Vector2.LEFT]])


func _ex_3d() -> void:
	sujet("Exemple : nager vers où regarde la caméra (3D)")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -5.5, 0), Vector3(20, 1, 16))
	var zone := WaterArea3D.new()
	zone.position = Vector3(0, -2.5, 0)
	var forme := CollisionShape3D.new()
	forme.shape = BoxShape3D.new()
	forme.shape.size = Vector3(20, 5.6, 16)
	zone.add_child(forme)
	Formes.cube(zone, Vector3.ZERO, Color(0.2, 0.45, 0.9, 0.3), Vector3(20, 5.6, 16))
	vue.add_child(zone)
	var heros := heros_3d(vue, Vector3(0, -1, 4), {"Sol": "GroundMovement3D", "Nage": "SwimMovement3D", "Directeur": "MovementDirector"},
		{"Nage": {"camera_relative": true, "visual_path": NodePath("../Visuel")}})
	var camera := vue.get_camera_3d()
	camera.position = Vector3(0, 2, 10)
	debug(vue)
	code("$SwimMovement3D.camera_relative = true   # avant caméra baissée = plonger\n$MovementDirector.move(Input.get_vector(\"gauche\", \"droite\", \"avant\", \"arriere\"))")
	var d: MovementDirector = heros.get_node("Directeur")
	for etape in [[-10.0, "Caméra presque horizontale : « avant » avance"], [-45.0, "Caméra baissée : « avant » plonge"]]:
		camera.rotation_degrees.x = etape[0]
		legende(etape[1])
		await piloter(d, [[2.0, Vector2(0, -1)], [0.4, Vector2.ZERO]])
	d.set_vertical_input(1.0)
	await piloter(d, [[1.6, Vector2(0, 1)]])
	d.set_vertical_input(0.0)


func _ex_riviere() -> void:
	sujet("Exemple : une rivière qui emporte tout")
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.7)
	arene(vue, 2400.0)
	var zone := eau(vue, Rect2(-1200, 0, 2400, 200), {"current": Vector2(250, 0), "linear_drag": 1.0})
	var heros := _nageur(vue, Vector2(-600, 60), SwimMovement2D.STYLE_DIRECTIONAL, {"drag": 0.5})
	debug(vue)
	code("$Riviere.current = Vector2(250, 0)\n$Riviere.linear_drag = 1.0\n$Joueur/SwimMovement2D.drag = 0.5")
	for i in 4:
		caisse(vue, Vector2(-800 + i * 120, -100))
	var t := 0.0
	while t < 7.0:
		await get_tree().physics_frame
		t += get_physics_process_delta_time()
		for corps in vue.get_children():
			if corps is PhysicsBody2D and not corps is StaticBody2D and corps.position.x > 1150:
				corps.position.x = -1100
	zone.name = "Riviere"


func _ex_niveau() -> void:
	sujet("Exemple : faire monter le niveau de l'eau")
	var vue := monde_2d()
	arene(vue)
	var zone := eau(vue, Rect2(-400, 140, 800, 60))
	var forme: CollisionShape2D = zone.get_child(0)
	var dessin: Polygon2D = zone.get_child(1)
	debug(vue)
	for i in 4:
		caisse(vue, Vector2(-300 + i * 200, 120))
	code("var forme := $Eau/CollisionShape2D.shape as RectangleShape2D\nvar bas := position.y + forme.size.y / 2.0\nforme.size.y += hauteur\nposition.y = bas - forme.size.y / 2.0")
	await secondes(1.0)
	legende("monter_eau(4) à chaque image physique")
	var t := 0.0
	while t < 5.0:
		await get_tree().physics_frame
		t += get_physics_process_delta_time()
		var rectangle := forme.shape as RectangleShape2D
		if rectangle.size.y < 300:
			var bas := forme.position.y + rectangle.size.y / 2.0
			rectangle.size.y += 1.5
			forme.position.y = bas - rectangle.size.y / 2.0
			dessin.polygon = Formes.rectangle(rectangle.size)
			dessin.position.y = forme.position.y
	await secondes(1.0)
