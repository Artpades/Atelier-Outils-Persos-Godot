extends "res://🎥Démos de documentation/5.🌀🫗/sequences_mouvement.gd"

## 🪂 Plans filmés de la page Physique aérodynamique (mouvement/physique-aerodynamique.html).


func titre_module() -> String:
	return "🪂 Aérodynamique · AerodynamicMovement2D / 3D"


func plans() -> Dictionary:
	return {
		"mode-emploi-tuto-plane": _tuto_plane,
		"mode-emploi-tuto-feuille": _tuto_feuille,
		"classe-AerodynamicMovement2D": _classe_2d,
		"classe-AerodynamicMovement3D": _classe_3d,
		"AerodynamicMovement2D.glide_fall_speed": _glide_fall,
		"AerodynamicMovement2D.glide_speed": _glide_speed,
		"AerodynamicMovement2D.wind_influence": _wind,
		"AerodynamicMovement2D.drag_coefficient": _drag,
		"AerodynamicMovement2D.lift_coefficient": _lift,
		"AerodynamicMovement2D.flutter_frequency": _flutter,
		"ex-battements": _ex_battements,
		"ex-vol": _ex_vol,
		"ex-parapente": _ex_parapente,
		"ex-confettis": _ex_confettis,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi-tuto-plane": "Saut maintenu : dès que le personnage redescend, le contexte « plané » s'active ; relâcher le fait retomber.",
		"mode-emploi-tuto-feuille": "Des feuilles (RigidBody2D légers) avec traînée, portance et <code>flutter_frequency</code> : chute en zigzag.",
		"classe-AerodynamicMovement2D": "Un personnage qui plane et des feuilles qui volettent : le même composant, sur un personnage ou un corps rigide.",
		"classe-AerodynamicMovement3D": "En 3D : un personnage qui plane depuis une tour, et des confettis.",
		"AerodynamicMovement2D.glide_fall_speed": "<code>glide_fall_speed</code> 30, 70 (défaut) et 200 px/s ; <code>glide_gravity_scale</code> ralentit la chute avant d'atteindre cette vitesse.",
		"AerodynamicMovement2D.glide_speed": "<code>glide_speed</code> 120, 260 (défaut) et 450 px/s, rejointe à <code>glide_acceleration</code>.",
		"AerodynamicMovement2D.wind_influence": "Dans un courant ascendant : <code>wind_influence</code> 0,5, 2 (défaut) et 4.",
		"AerodynamicMovement2D.drag_coefficient": "Feuilles avec <code>drag_coefficient</code> 0,002 (défaut), 0,02 et 0,08 : la vitesse limite de chute.",
		"AerodynamicMovement2D.lift_coefficient": "<code>lift_coefficient</code> 0, 0,01 et 0,03 (avec <code>flutter_frequency</code> 1,5) : la portance fait osciller la trajectoire.",
		"AerodynamicMovement2D.flutter_frequency": "<code>flutter_frequency</code> 0, 1,5 et 4 Hz : la portance change de sens, la feuille volette.",
		"ex-battements": "Contexte « air » (In Air) : jusqu'à deux battements d'ailes (<code>max_flaps = 2</code>) en l'air, signal <code>flapped</code>.",
		"ex-vol": "Mode Fly : <code>set_context(&\"vol\")</code> pour voler librement, <code>clear_forced_context()</code> pour atterrir.",
		"ex-parapente": "Parapente : <code>wind_influence = 4</code> et un courant ascendant (ForceZone2D) qui le fait monter.",
		"ex-confettis": "Des confettis 3D (RigidBody3D) avec un AerodynamicMovement3D chacun : traînée, portance et fréquence de battement aléatoire.",
	}


func reutilisations() -> Dictionary:
	var meme_3d := "Même réglage qu'en 2D, en m/s (voir la vidéo 2D)."
	var r := {
		"AerodynamicMovement2D.glide_gravity_scale": "AerodynamicMovement2D.glide_fall_speed",
		"AerodynamicMovement2D.glide_acceleration": "AerodynamicMovement2D.glide_speed",
		"AerodynamicMovement2D.mode": "ex-vol",
		"AerodynamicMovement2D.fly_speed": "ex-vol",
		"AerodynamicMovement2D.fly_acceleration": "ex-vol",
		"AerodynamicMovement2D.fly_drag": "ex-vol",
		"AerodynamicMovement2D.max_flaps": "ex-battements",
		"AerodynamicMovement2D.flap_velocity": "ex-battements",
		"AerodynamicMovement2D.get_flaps_left": "ex-battements",
		"AerodynamicMovement2D.flapped": "ex-battements",
		"AerodynamicMovement3D.camera_relative": "classe-AerodynamicMovement3D",
		"AerodynamicMovement3D.get_flaps_left": "ex-battements",
		"AerodynamicMovement3D.flapped": "ex-battements",
	}
	for membre in ["drag_coefficient", "flap_velocity", "flutter_frequency", "fly_acceleration", "fly_drag", "fly_speed", "glide_acceleration", "glide_fall_speed",
			"glide_gravity_scale", "glide_speed", "lift_coefficient", "max_flaps", "mode", "wind_influence"]:
		r["AerodynamicMovement3D." + membre] = [r.get("AerodynamicMovement2D." + membre, "AerodynamicMovement2D." + membre), meme_3d]
	return r


## Feuille : RigidBody2D léger avec un AerodynamicMovement2D réglé par `reglages`.
func feuille(parent: Node, position: Vector2, reglages: Dictionary, couleur: Color = Color("7bc96f")) -> RigidBody2D:
	var corps := RigidBody2D.new()
	corps.position = position
	corps.mass = 0.05
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = Vector2(22, 8)
	corps.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = PackedVector2Array([Vector2(-12, 0), Vector2(-4, -5), Vector2(8, -4), Vector2(12, 0), Vector2(6, 4), Vector2(-6, 4)])
	dessin.color = couleur
	corps.add_child(dessin)
	var aero := AerodynamicMovement2D.new()
	for propriete in reglages:
		aero.set(propriete, reglages[propriete])
	corps.add_child(aero)
	parent.add_child(corps)
	return corps


func _planeur(vue: Node, position: Vector2, reglages_aero: Dictionary = {}, couleur: Color = VARIANTES[0]) -> CharacterBody2D:
	return heros_2d(vue, position, {"Sol": "GroundMovement2D", "Aero": "AerodynamicMovement2D", "Directeur": "MovementDirector"}, {"Aero": reglages_aero}, couleur)


## Colonnes : une tour par valeur, le planeur saute de la tour en maintenant le saut.
func _colonnes_plane(propriete: String, valeurs: Array, textes: Array) -> Array:
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.75)
	vue.get_meta("camera").position.y = -40
	arene(vue, 1400.0)
	var directeurs := []
	var n := valeurs.size()
	for i in n:
		var y := -230.0 + i * 0.0
		var x := -560.0 + i * 1120.0 / n
		bloc(vue, Rect2(x, y, 90, 430), Formes.DECOR)
		var corps := _planeur(vue, Vector2(x + 40, y - 30), {propriete: valeurs[i]}, VARIANTES[i % 4])
		directeurs.append(corps.get_node("Directeur"))
		etiquette(Vector2(576 + (x + 120) * 0.75, 545), textes[i], 16, VARIANTES[i % 4])
	debug(vue)
	return directeurs


func _tous(cibles: Array, etapes: Array) -> void:
	for i in cibles.size() - 1:
		piloter(cibles[i], etapes)
	await piloter(cibles[cibles.size() - 1], etapes)


## Colonnes de feuilles (trois par valeur) qui tombent du haut.
func _feuilles_colonnes(propriete: String, valeurs: Array, textes: Array, communs: Dictionary) -> void:
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.7)
	vue.get_meta("camera").position.y = -60
	arene(vue, 1500.0)
	var n := valeurs.size()
	for i in n:
		etiquette(Vector2(576 + (-700.0 + (i + 0.5) * 1400.0 / n) * 0.7, 545), textes[i], 16, VARIANTES[i % 4])
	for vague in 2:
		for i in n:
			var reglages := communs.duplicate()
			reglages[propriete] = valeurs[i]
			for j in 3:
				feuille(vue, Vector2(-700.0 + (i + 0.5) * 1400.0 / n + (j - 1) * 60, -420 - j * 40), reglages, VARIANTES[i % 4].lerp(Color("7bc96f"), 0.4))
		await secondes(4.0)


# ⚙️ Mode d'emploi

func _tuto_plane() -> void:
	sujet("A. Planer en maintenant le saut")
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.75)
	vue.get_meta("camera").position.y = -40
	arene(vue, 1400.0)
	bloc(vue, Rect2(-560, -200, 120, 400), Formes.DECOR)
	var heros := _planeur(vue, Vector2(-520, -240))
	debug(vue)
	var contexte := etiquette(Vector2(576, 140), "", 18, ACCENT)
	var d: MovementDirector = heros.get_node("Directeur")
	get_tree().process_frame.connect(func():
		if is_instance_valid(contexte):
			contexte.text = "contexte : %s" % d.get_current_context())
	code("if Input.is_action_just_pressed(\"ui_accept\"): $MovementDirector.jump()\nelif Input.is_action_just_released(\"ui_accept\"): $MovementDirector.release_jump()")
	legende("Saut maintenu : plané dès la descente")
	await piloter(d, [[3.5, Vector2.RIGHT, "saut_long"]])
	legende("Relâché : chute normale")
	await piloter(d, [[1.0, Vector2.RIGHT], [0.6, Vector2.ZERO]])


func _tuto_feuille() -> void:
	sujet("B. Une feuille qui tombe en zigzag")
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.7)
	vue.get_meta("camera").position.y = -60
	arene(vue, 1500.0)
	code("aero.drag_coefficient = 0.02\naero.lift_coefficient = 0.01\naero.flutter_frequency = 1.5")
	for i in 7:
		feuille(vue, Vector2(-500 + i * 160, -420 - (i % 3) * 50), {"drag_coefficient": 0.02, "lift_coefficient": 0.01, "flutter_frequency": 1.5})
		await secondes(0.4)
	await secondes(5.0)


# 💻 API

func _classe_2d() -> void:
	sujet("AerodynamicMovement2D : planer, voler, voleter")
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.75)
	vue.get_meta("camera").position.y = -40
	arene(vue, 1400.0)
	bloc(vue, Rect2(-560, -200, 120, 400), Formes.DECOR)
	var heros := _planeur(vue, Vector2(-520, -240))
	for i in 4:
		feuille(vue, Vector2(200 + i * 90, -420 - i * 50), {"drag_coefficient": 0.02, "lift_coefficient": 0.01, "flutter_frequency": 1.5})
	debug(vue)
	code("# CharacterBody2D : plané (contexte « plané »)\n# RigidBody2D : traînée et portance passives")
	await piloter(heros.get_node("Directeur"), [[4.0, Vector2.RIGHT, "saut_long"], [1.0, Vector2.ZERO]])


func _classe_3d() -> void:
	sujet("AerodynamicMovement3D : planer en 3D")
	var vue := monde_3d()
	vue.get_camera_3d().position = Vector3(2, 9, 16)
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(40, 1, 20))
	bloc_3d(vue, Vector3(-8, 3, 0), Vector3(2, 7, 2), Formes.DECOR)
	var heros := heros_3d(vue, Vector3(-8, 7.5, 0), {"Sol": "GroundMovement3D", "Aero": "AerodynamicMovement3D", "Directeur": "MovementDirector"}, {"Sol": {"visual_path": NodePath("../Visuel")}})
	debug(vue)
	code("AerodynamicMovement3D : glide_fall_speed 1,5 m/s, glide_speed 7 m/s")
	await piloter(heros.get_node("Directeur"), [[4.5, Vector2.RIGHT, "saut_long"], [0.8, Vector2.ZERO]])


func _glide_fall() -> void:
	sujet("glide_fall_speed et glide_gravity_scale : la descente")
	code("$AerodynamicMovement2D.glide_fall_speed = 70.0\n$AerodynamicMovement2D.glide_gravity_scale = 0.35")
	var d := _colonnes_plane("glide_fall_speed", [30.0, 70.0, 200.0], ["30", "70 (défaut)", "200"])
	await _tous(d, [[3.6, Vector2.RIGHT, "saut_long"], [0.8, Vector2.ZERO]])


func _glide_speed() -> void:
	sujet("glide_speed et glide_acceleration : vitesse horizontale")
	code("$AerodynamicMovement2D.glide_speed = 260.0\n$AerodynamicMovement2D.glide_acceleration = 500.0")
	var d := _colonnes_plane("glide_speed", [120.0, 260.0, 450.0], ["120", "260 (défaut)", "450"])
	await _tous(d, [[3.6, Vector2.RIGHT, "saut_long"], [0.8, Vector2.ZERO]])


func _wind() -> void:
	sujet("wind_influence : sensibilité au vent")
	code("$AerodynamicMovement2D.wind_influence = 2.0")
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.75)
	vue.get_meta("camera").position.y = -40
	arene(vue, 1400.0)
	var d := []
	for i in 3:
		var x := -560.0 + i * 1120.0 / 3.0
		bloc(vue, Rect2(x, -230, 90, 430), Formes.DECOR)
		zone_force(vue, Rect2(x + 110, -500, 200, 700), {"direction": Vector2.UP, "strength": 160.0})
		var corps := _planeur(vue, Vector2(x + 40, -260), {"wind_influence": [0.5, 2.0, 4.0][i]}, VARIANTES[i])
		d.append(corps.get_node("Directeur"))
		etiquette(Vector2(576 + (x + 160) * 0.75, 545), ["0,5", "2 (défaut)", "4"][i], 16, VARIANTES[i])
	debug(vue)
	await _tous(d, [[0.5, Vector2.RIGHT, "saut_long"], [3.5, Vector2.ZERO, "saut_long"], [0.5, Vector2.ZERO]])


func _drag() -> void:
	sujet("drag_coefficient : traînée d'un corps rigide")
	code("aero.drag_coefficient = 0.002")
	await _feuilles_colonnes("drag_coefficient", [0.002, 0.02, 0.08], ["0,002 (défaut)", "0,02", "0,08"], {})


func _lift() -> void:
	sujet("lift_coefficient : portance")
	code("aero.lift_coefficient = 0.01\naero.flutter_frequency = 1.5")
	await _feuilles_colonnes("lift_coefficient", [0.0, 0.01, 0.03], ["0 (défaut)", "0,01", "0,03"], {"drag_coefficient": 0.02, "flutter_frequency": 1.5})


func _flutter() -> void:
	sujet("flutter_frequency : voleter")
	code("aero.flutter_frequency = 1.5")
	await _feuilles_colonnes("flutter_frequency", [0.0, 1.5, 4.0], ["0 (défaut)", "1,5 Hz", "4 Hz"], {"drag_coefficient": 0.02, "lift_coefficient": 0.015})


# 📌 Exemples

func _ex_battements() -> void:
	sujet("Exemple : battre des ailes en l'air")
	var vue := monde_2d()
	arene(vue)
	var heros := heros_2d(vue, Vector2(-450, 150), {"Sol": "GroundMovement2D", "Aero": "AerodynamicMovement2D"}, {"Aero": {"max_flaps": 2}})
	var sol := MovementContext.new()
	sol.context_name = &"sol"
	sol.components = [NodePath("../Sol")]
	var air := MovementContext.new()
	air.context_name = &"air"
	air.condition = MovementContext.CONDITION_IN_AIR
	air.priority = 10
	air.components = [NodePath("../Aero")]
	var d := MovementDirector.new()
	d.auto_configure = false
	d.contexts = [sol, air]
	heros.add_child(d)
	var aero: AerodynamicMovement2D = heros.get_node("Aero")
	var journal := etiquette(Vector2(576, 140), "", 18, ACCENT)
	aero.flapped.connect(func(): journal.text = "flapped · get_flaps_left() = %d" % aero.get_flaps_left())
	debug(vue)
	code("$AerodynamicMovement2D.max_flaps = 2\nair.condition = MovementContext.CONDITION_IN_AIR\n$AerodynamicMovement2D.flapped.connect(...)")
	for i in 2:
		await piloter(d, [[0.4, Vector2.RIGHT if i == 0 else Vector2.LEFT, "saut_long"], [0.4, Vector2.RIGHT if i == 0 else Vector2.LEFT, "saut_long"],
			[0.4, Vector2.RIGHT if i == 0 else Vector2.LEFT, "saut_long"], [0.4, Vector2.RIGHT if i == 0 else Vector2.LEFT, "saut_long"], [1.6, Vector2.ZERO]])


func _ex_vol() -> void:
	sujet("Exemple : voler librement (fée, jetpack)")
	var vue := monde_2d()
	arene(vue)
	var heros := _planeur(vue, Vector2(-450, 150), {"mode": AerodynamicMovement2D.MODE_FLY})
	var d: MovementDirector = heros.get_node("Directeur")
	debug(vue)
	var contexte := etiquette(Vector2(576, 140), "", 18, ACCENT)
	get_tree().process_frame.connect(func():
		if is_instance_valid(contexte):
			contexte.text = "contexte : %s" % d.get_current_context())
	code("if event.is_action_pressed(\"voler\"): $MovementDirector.set_context(&\"vol\")\nelif event.is_action_pressed(\"atterrir\"): $MovementDirector.clear_forced_context()")
	await piloter(d, [[0.8, Vector2.RIGHT]])
	legende("« voler » : set_context(&\"vol\") — fly_speed, fly_acceleration, fly_drag")
	d.set_context(&"vol")
	await piloter(d, [[1.2, Vector2(1, -1)], [1.0, Vector2(1, 0.3)], [1.0, Vector2(-1, -0.5)], [0.8, Vector2.ZERO]])
	legende("« atterrir » : clear_forced_context()")
	d.clear_forced_context()
	await piloter(d, [[1.5, Vector2.LEFT], [0.5, Vector2.ZERO]])


func _ex_parapente() -> void:
	sujet("Exemple : un parapente sensible aux courants")
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.75)
	vue.get_meta("camera").position.y = -40
	arene(vue, 1400.0)
	bloc(vue, Rect2(-560, -200, 120, 400), Formes.DECOR)
	zone_force(vue, Rect2(-200, -500, 260, 700), {"direction": Vector2.UP, "strength": 300.0})
	var heros := _planeur(vue, Vector2(-520, -240), {"wind_influence": 4.0, "glide_fall_speed": 40.0})
	debug(vue)
	code("$AerodynamicMovement2D.wind_influence = 4.0\n$AerodynamicMovement2D.glide_fall_speed = 40.0\n$CourantAscendant.strength = 300.0")
	await piloter(heros.get_node("Directeur"), [[1.6, Vector2.RIGHT, "saut_long"], [2.4, Vector2(0.1, 0), "saut_long"], [2.0, Vector2.RIGHT, "saut_long"], [0.6, Vector2.ZERO]])


func _ex_confettis() -> void:
	sujet("Exemple : des confettis en 3D")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(30, 1, 20))
	code("aero.drag_coefficient = 0.8\naero.lift_coefficient = 0.3\naero.flutter_frequency = randf_range(1.0, 3.0)\nc.add_child(aero)")
	for vague in 3:
		for i in 25:
			var c := RigidBody3D.new()
			c.mass = 0.01
			var forme := CollisionShape3D.new()
			forme.shape = BoxShape3D.new()
			forme.shape.size = Vector3(0.25, 0.02, 0.18)
			c.add_child(forme)
			Formes.cube(c, Vector3.ZERO, Color.from_hsv(randf(), 0.7, 1.0), Vector3(0.25, 0.02, 0.18))
			var aero := AerodynamicMovement3D.new()
			aero.drag_coefficient = 0.8
			aero.lift_coefficient = 0.3
			aero.flutter_frequency = randf_range(1.0, 3.0)
			c.add_child(aero)
			vue.add_child(c)
			c.global_position = Vector3(randf_range(-4, 4), 8 + randf_range(0, 2), randf_range(-2, 2))
			c.angular_velocity = Vector3(randf_range(-5, 5), randf_range(-5, 5), randf_range(-5, 5))
		await secondes(2.2)
	await secondes(2.0)
