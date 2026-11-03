extends "res://🎥Démos de documentation/5.🌀🫗/sequences_mouvement.gd"

## 🦶 Plans filmés de la page Déplacement au sol (mouvement/deplacement-au-sol.html) : GroundMovement2D / 3D.


func titre_module() -> String:
	return "🦶 Déplacement au sol · GroundMovement2D / 3D"


func plans() -> Dictionary:
	return {
		"mode-emploi-tuto-2d": _tuto_2d,
		"mode-emploi-tuto-3d": _tuto_3d,
		"classe-GroundMovement2D": _classe_2d,
		"classe-GroundMovement3D": _classe_3d,
		"GroundMovement2D.max_speed": _max_speed,
		"GroundMovement2D.acceleration": _acceleration,
		"GroundMovement2D.air_control": _air_control,
		"GroundMovement2D.jump_velocity": _jump_velocity,
		"GroundMovement2D.max_jumps": _max_jumps,
		"GroundMovement2D.coyote_time": _coyote_time,
		"GroundMovement2D.jump_cut": _jump_cut,
		"GroundMovement2D.gravity_scale": _gravity_scale,
		"GroundMovement2D.max_fall_speed": _max_fall_speed,
		"GroundMovement2D.run_multiplier": _run_multiplier,
		"GroundMovement2D.visual_path": _visual_path,
		"GroundMovement3D.camera_relative": _camera_relative,
		"GroundMovement3D.turn_speed": _turn_speed,
		"ex-topdown": _ex_topdown,
		"ex-glace": _ex_glace,
		"ex-effets": _ex_effets,
		"ex-ia3d": _ex_ia3d,
		"ex-gravite": _ex_gravite,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi-tuto-2d": "Double saut (<code>max_jumps = 2</code>), saut juste après un rebord (temps de grâce) et petit saut relâché tôt, avec <code>debug_mode</code>.",
		"mode-emploi-tuto-3d": "GroundMovement3D avec <code>camera_relative</code> : haut = vers l'avant de la caméra ; seul le modèle (<code>visual_path</code>) tourne.",
		"classe-GroundMovement2D": "Deux modes : plateforme (gravité, sauts) et vue de dessus (déplacement libre en x et y).",
		"classe-GroundMovement3D": "Marche dans le plan XZ, sauts selon Y, modèle tourné vers la direction de marche.",
		"GroundMovement2D.max_speed": "<code>max_speed</code> 100, 200 (défaut) et 400 px/s.",
		"GroundMovement2D.acceleration": "<code>acceleration</code> / <code>deceleration</code> 300 / 150 (glissant), 1500 / 2000 (défaut) et 6000 / 8000 (instantané).",
		"GroundMovement2D.air_control": "Saut vers la droite puis demi-tour en l'air : <code>air_control</code> 0, 0,6 (défaut) et 1.",
		"GroundMovement2D.jump_velocity": "<code>jump_velocity</code> 300, 420 (défaut) et 600 px/s.",
		"GroundMovement2D.max_jumps": "Trois appuis sur saut : <code>max_jumps</code> 1, 2 et 3 (0 désactive le saut).",
		"GroundMovement2D.coyote_time": "Saut 0,1 s après avoir quitté le rebord : refusé avec <code>coyote_time = 0</code>, accepté avec 0,15.",
		"GroundMovement2D.jump_cut": "Saut relâché tôt : <code>jump_cut</code> 1 (aucune coupure), 0,5 (défaut) et 0,2.",
		"GroundMovement2D.gravity_scale": "<code>gravity_scale</code> 0,5, 1 et 2 : même saut, gravité différente.",
		"GroundMovement2D.max_fall_speed": "Chute depuis très haut : <code>max_fall_speed</code> 300 et 1500 (défaut).",
		"GroundMovement2D.run_multiplier": "En course : <code>run_multiplier</code> 1,2, 1,6 (défaut) et 2,5.",
		"GroundMovement2D.visual_path": "<code>visual_path</code> vers le sprite : il est retourné selon la direction ; <code>get_facing()</code> (flèche) suit.",
		"GroundMovement3D.camera_relative": "La caméra tourne : avec <code>camera_relative</code>, « avant » suit la caméra ; sans, c'est toujours -Z du monde.",
		"GroundMovement3D.turn_speed": "<code>turn_speed</code> 3, 12 (défaut) et 40 : la vitesse à laquelle le modèle se tourne vers sa marche.",
		"ex-topdown": "Vue de dessus (<code>MODE_TOP_DOWN</code>) : déplacement libre, course avec Maj.",
		"ex-glace": "Sur la zone gelée (Area2D), accélération 300 et freinage 150 : le personnage glisse.",
		"ex-effets": "<code>jumped</code> et <code>landed</code> : poussière à l'atterrissage, antenne écrasée (Jiggle2D).",
		"ex-ia3d": "Un PNJ 3D suit le chemin de son NavigationAgent3D autour d'un obstacle : <code>move(Vector2(x, z))</code>.",
		"ex-gravite": "<code>gravity_scale *= -1</code> et <code>up_direction</code> inversée : le personnage marche au plafond.",
	}


func schemas() -> Dictionary:
	return {
		"figure-2": ["fleches_3d.svg", "Les flèches de <code>debug_mode</code> en 3D : intention convertie en direction du monde (jaune, <code>get_world_input()</code>), regard (rose, <code>get_facing()</code>), normale du sol (rouge), vitesse (vert)."],
	}


func reutilisations() -> Dictionary:
	var meme_3d := "Même réglage que GroundMovement2D, en m/s (voir la vidéo 2D)."
	var r := {
		"GroundMovement2D.deceleration": "GroundMovement2D.acceleration",
		"GroundMovement2D.mode": "classe-GroundMovement2D",
		"GroundMovement2D.get_facing": "GroundMovement2D.visual_path",
		"GroundMovement2D.is_on_floor": "mode-emploi-tuto-2d",
		"GroundMovement2D.jumped": "ex-effets",
		"GroundMovement2D.landed": "ex-effets",
		"GroundMovement3D.rotate_toward_movement": "GroundMovement3D.turn_speed",
		"GroundMovement3D.visual_path": "mode-emploi-tuto-3d",
		"GroundMovement3D.get_facing": "figure-2",
		"GroundMovement3D.get_world_input": "figure-2",
		"GroundMovement3D.is_on_floor": "classe-GroundMovement3D",
		"GroundMovement3D.jumped": "classe-GroundMovement3D",
		"GroundMovement3D.landed": "classe-GroundMovement3D",
	}
	for membre in ["max_speed", "acceleration", "deceleration", "air_control", "jump_velocity", "max_jumps", "coyote_time", "jump_cut", "gravity_scale", "max_fall_speed", "run_multiplier"]:
		r["GroundMovement3D." + membre] = ["GroundMovement2D." + (membre if membre != "deceleration" else "acceleration"), meme_3d]
	return r


## Colonnes : un personnage par valeur, chacun sur son sol ; renvoie les composants (même ordre).
func _colonnes_sol(propriete: String, valeurs: Array, textes: Array, communs: Dictionary = {}, vue: SubViewport = null) -> Array:
	if vue == null:
		vue = monde_2d()
		arene(vue)
	var composants := []
	var n := valeurs.size()
	var pas := 1000.0 / n
	for i in n:
		var x := -500.0 + pas * (i + 0.5)
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		var corps := heros_2d(vue, Vector2(x, 150), {"Sol": "GroundMovement2D"}, {"Sol": reglages}, VARIANTES[i % 4])
		composants.append(corps.get_node("Sol"))
		etiquette(Vector2(76 + 500 + x, 540), textes[i], 17, VARIANTES[i % 4])
	return composants


## Couloirs : un personnage par valeur, sur des plateformes superposées (de haut en bas) ; renvoie les composants.
func _couloirs(propriete: String, valeurs: Array, textes: Array, communs: Dictionary = {}) -> Array:
	var vue := monde_2d()
	arene(vue, 2000.0)
	var composants := []
	for i in valeurs.size():
		var y := -110.0 + i * 110.0
		if i < valeurs.size() - 1:
			bloc(vue, Rect2(-1000, y + 24, 2000, 16))
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		var corps := heros_2d(vue, Vector2(-480, y), {"Sol": "GroundMovement2D"}, {"Sol": reglages}, VARIANTES[i % 4])
		composants.append(corps.get_node("Sol"))
		etiquette(Vector2(1000, 110 + 220 + y - 50 - 28), textes[i], 16, VARIANTES[i % 4])
	return composants


## Joue les étapes de pilotage sur tous les composants en même temps.
func _tous(composants: Array, etapes: Array) -> void:
	for i in composants.size() - 1:
		piloter(composants[i], etapes)
	await piloter(composants[composants.size() - 1], etapes)


# ⚙️ Mode d'emploi

func _tuto_2d() -> void:
	sujet("A. Personnage de plateforme 2D")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(-480, 60, 260, 24))
	bloc(vue, Rect2(150, 40, 240, 24))
	var heros := heros_2d(vue, Vector2(-450, 0), {"Sol": "GroundMovement2D"}, {"Sol": {"max_jumps": 2}})
	debug(vue)
	code("g.move(Vector2(Input.get_axis(\"ui_left\", \"ui_right\"), 0))\nif Input.is_action_just_pressed(\"ui_accept\"): g.jump()\nelif Input.is_action_just_released(\"ui_accept\"): g.release_jump()")
	var sol: GroundMovement2D = heros.get_node("Sol")
	await secondes(0.4)
	legende("Double saut (max_jumps = 2)")
	await piloter(sol, [[0.5, Vector2.RIGHT], [0.35, Vector2.RIGHT, "saut_long"], [0.6, Vector2.RIGHT, "saut_long"], [0.6, Vector2.RIGHT]])
	legende("Saut juste après le rebord : temps de grâce (coyote_time)")
	await piloter(sol, [[0.9, Vector2.RIGHT], [0.6, Vector2.RIGHT, "saut_long"], [0.5, Vector2.ZERO]])
	legende("Petit saut : relâché tôt (jump_cut)")
	await piloter(sol, [[0.3, Vector2.LEFT], [0.5, Vector2.LEFT, "saut_court"], [0.5, Vector2.LEFT], [0.5, Vector2.LEFT, "saut_court"], [0.8, Vector2.ZERO]])


func _tuto_3d() -> void:
	sujet("B. Personnage 3D à la troisième personne")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(30, 1, 30))
	for p in [Vector3(-4, 0.5, -3), Vector3(3, 0.5, -5), Vector3(5, 0.5, 2)]:
		bloc_3d(vue, p, Vector3(1.5, 1, 1.5), Formes.DECOR)
	var heros := heros_3d(vue, Vector3(0, 1, 0), {"Sol": "GroundMovement3D"}, {"Sol": {"camera_relative": true, "visual_path": NodePath("../Visuel")}})
	debug(vue)
	code("g.move(Input.get_vector(\"ui_left\", \"ui_right\", \"ui_up\", \"ui_down\"))  # haut = avant\nif Input.is_action_just_pressed(\"ui_accept\"): g.jump()")
	var sol: GroundMovement3D = heros.get_node("Sol")
	await piloter(sol, [[1.2, Vector2.UP], [1.0, Vector2.RIGHT], [0.6, Vector2.DOWN, "saut_long"], [1.2, Vector2.DOWN], [1.0, Vector2.LEFT], [0.8, Vector2(1, -1)], [0.5, Vector2.ZERO]])


# 💻 API

func _classe_2d() -> void:
	sujet("GroundMovement2D : plateforme ou vue de dessus")
	var gauche := vue_2d(Rect2(56, 130, 512, 380))
	gauche.get_meta("camera").position.y = 50
	arene(gauche, 500)
	var plateforme := heros_2d(gauche, Vector2(-150, 150), {"Sol": "GroundMovement2D"})
	etiquette(Vector2(312, 535), "MODE_PLATFORMER (défaut)", 17, VARIANTES[0])
	var droite := vue_2d(Rect2(584, 130, 512, 380))
	for rect in [Rect2(-260, -190, 520, 20), Rect2(-260, 170, 520, 20), Rect2(-260, -190, 20, 380), Rect2(240, -190, 20, 380)]:
		bloc(droite, rect)
	var dessus := heros_2d(droite, Vector2(-100, 0), {"Sol": "GroundMovement2D"}, {"Sol": {"mode": GroundMovement2D.MODE_TOP_DOWN}}, VARIANTES[1])
	etiquette(Vector2(840, 535), "MODE_TOP_DOWN", 17, VARIANTES[1])
	debug(scene)
	code("$GroundMovement2D.mode = GroundMovement2D.MODE_TOP_DOWN")
	var etapes := [[0.8, Vector2.RIGHT], [0.6, Vector2(0, -1), "saut_long"], [0.8, Vector2(0, 1)], [0.8, Vector2(-1, -1)], [0.8, Vector2.LEFT, "saut_long"], [0.5, Vector2.ZERO]]
	piloter(plateforme.get_node("Sol"), etapes)
	await piloter(dessus.get_node("Sol"), etapes)


func _classe_3d() -> void:
	sujet("GroundMovement3D : marche, sauts, rotation du modèle")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(30, 1, 30))
	bloc_3d(vue, Vector3(3, 0.4, -2), Vector3(3, 0.8, 3), Formes.DECOR)
	var heros := heros_3d(vue, Vector3(-3, 1, 1), {"Sol": "GroundMovement3D"}, {"Sol": {"visual_path": NodePath("../Visuel"), "max_jumps": 2}})
	var sol: GroundMovement3D = heros.get_node("Sol")
	var signaux := etiquette(Vector2(576, 140), "", 18, ACCENT)
	sol.jumped.connect(func(): signaux.text = "jumped")
	sol.landed.connect(func(): signaux.text = "landed")
	debug(vue)
	code("$GroundMovement3D.move(Vector2(x, z))\n$GroundMovement3D.jump()")
	await piloter(sol, [[1.0, Vector2.RIGHT], [0.7, Vector2(1, -0.5), "saut_long"], [0.8, Vector2(0, -1)], [0.6, Vector2.LEFT, "saut_long"], [0.4, Vector2.LEFT, "saut_long"], [1.0, Vector2(-1, 1)], [0.6, Vector2.ZERO]])


func _max_speed() -> void:
	sujet("max_speed : vitesse de marche")
	code("$GroundMovement2D.max_speed = 200.0")
	var c := _couloirs("max_speed", [100.0, 200.0, 400.0], ["100", "200 (défaut)", "400"])
	await _tous(c, [[3.5, Vector2.RIGHT], [1.0, Vector2.ZERO]])


func _acceleration() -> void:
	sujet("acceleration et deceleration : démarrage et freinage")
	code("$GroundMovement2D.acceleration = 1500.0\n$GroundMovement2D.deceleration = 2000.0")
	var c := _couloirs("acceleration", [300.0, 1500.0, 6000.0], ["300 / 150", "1500 / 2000 (défaut)", "6000 / 8000"])
	c[0].deceleration = 150.0
	c[2].deceleration = 8000.0
	await _tous(c, [[1.2, Vector2.RIGHT], [1.0, Vector2.ZERO], [1.0, Vector2.LEFT], [1.2, Vector2.ZERO]])


func _air_control() -> void:
	sujet("air_control : maîtrise de la trajectoire en l'air")
	code("$GroundMovement2D.air_control = 0.6")
	var c := _colonnes_sol("air_control", [0.0, 0.6, 1.0], ["0", "0,6 (défaut)", "1"])
	for i in 2:
		await _tous(c, [[0.25, Vector2.RIGHT], [0.15, Vector2.RIGHT, "saut_long"], [0.6, Vector2.LEFT], [0.6, Vector2.ZERO]])


func _jump_velocity() -> void:
	sujet("jump_velocity : force du saut")
	code("$GroundMovement2D.jump_velocity = 420.0")
	var c := _colonnes_sol("jump_velocity", [300.0, 420.0, 600.0], ["300", "420 (défaut)", "600"])
	for i in 3:
		await _tous(c, [[1.2, Vector2.ZERO, "saut_long"], [0.3, Vector2.ZERO]])


func _max_jumps() -> void:
	sujet("max_jumps : sauts avant de retoucher le sol")
	code("$GroundMovement2D.max_jumps = 2")
	var c := _colonnes_sol("max_jumps", [1, 2, 3], ["1 (défaut)", "2 : double saut", "3"])
	for i in 2:
		await _tous(c, [[0.35, Vector2.ZERO, "saut_long"], [0.35, Vector2.ZERO, "saut_long"], [0.35, Vector2.ZERO, "saut_long"], [1.4, Vector2.ZERO]])


func _coyote_time() -> void:
	sujet("coyote_time : temps de grâce après un rebord")
	code("$GroundMovement2D.coyote_time = 0.1")
	var vue := monde_2d()
	arene(vue)
	var c := []
	for i in 2:
		var x := -380.0 + i * 500.0
		bloc(vue, Rect2(x - 120, 60, 200, 24))
		var corps := heros_2d(vue, Vector2(x - 80, 20), {"Sol": "GroundMovement2D"}, {"Sol": {"coyote_time": [0.0, 0.15][i]}}, VARIANTES[i * 2])
		c.append(corps.get_node("Sol"))
		etiquette(Vector2(576 + x, 540), ["coyote_time = 0", "coyote_time = 0,15"][i], 17, VARIANTES[i * 2])
	for essai in 2:
		for j in 2:
			var corps: Node2D = c[j].get_body()
			corps.position = Vector2(-380.0 + j * 500.0 - 80, 20)
			corps.velocity = Vector2.ZERO
		await secondes(0.4)
		legende("Course vers le bord, saut 0,1 s après l'avoir quitté")
		for composant in c:
			composant.move(Vector2.RIGHT)
		var parti := [false, false]
		var t := 0.0
		while t < 2.0:
			await get_tree().physics_frame
			t += get_physics_process_delta_time()
			for j in 2:
				if not parti[j] and c[j].get_body().position.x > -380.0 + j * 500.0 + 80 + 14:
					parti[j] = true
					var composant: GroundMovement2D = c[j]
					get_tree().create_timer(0.1).timeout.connect(func(): composant.jump())
		for composant in c:
			composant.release_jump()
			composant.move(Vector2.ZERO)
		await secondes(0.5)


func _jump_cut() -> void:
	sujet("jump_cut : hauteur de saut variable")
	code("$GroundMovement2D.jump_cut = 0.5\n$GroundMovement2D.release_jump()   # relâché tôt")
	var c := _colonnes_sol("jump_cut", [1.0, 0.5, 0.2], ["1 : aucune coupure", "0,5 (défaut)", "0,2"])
	for i in 3:
		await _tous(c, [[1.2, Vector2.ZERO, "saut_court"]])


func _gravity_scale() -> void:
	sujet("gravity_scale : multiplicateur de gravité")
	code("$GroundMovement2D.gravity_scale = 1.0")
	var c := _colonnes_sol("gravity_scale", [0.5, 1.0, 2.0], ["0,5", "1 (défaut)", "2"])
	for i in 2:
		await _tous(c, [[1.8, Vector2.ZERO, "saut_long"]])


func _max_fall_speed() -> void:
	sujet("max_fall_speed : vitesse de chute maximale")
	code("$GroundMovement2D.max_fall_speed = 1500.0")
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.45)
	vue.get_meta("camera").position.y = -200
	arene(vue)
	var c := []
	for i in 2:
		var corps := heros_2d(vue, Vector2(-250 + i * 500, -650), {"Sol": "GroundMovement2D"}, {"Sol": {"max_fall_speed": [300.0, 1500.0][i]}}, VARIANTES[i * 2])
		c.append(corps)
		etiquette(Vector2(576 - 110 + i * 220, 540), ["300", "1500 (défaut)"][i], 17, VARIANTES[i * 2])
	for essai in 2:
		for i in 2:
			c[i].position.y = -650
			c[i].velocity = Vector2.ZERO
		await secondes(3.0)


func _run_multiplier() -> void:
	sujet("run_multiplier : vitesse de course")
	code("$GroundMovement2D.run_multiplier = 1.6\n$GroundMovement2D.set_running(true)")
	var c := _couloirs("run_multiplier", [1.2, 1.6, 2.5], ["1,2", "1,6 (défaut)", "2,5"])
	for composant in c:
		composant.set_running(true)
	await _tous(c, [[3.0, Vector2.RIGHT], [1.0, Vector2.ZERO]])


func _visual_path() -> void:
	sujet("visual_path et get_facing()")
	var vue := monde_2d()
	arene(vue)
	var heros := heros_2d(vue, Vector2(-100, 150), {"Sol": "GroundMovement2D"}, {"Sol": {"visual_path": NodePath("../Visuel")}})
	var sol: GroundMovement2D = heros.get_node("Sol")
	var fleche := Line2D.new()
	fleche.width = 4
	fleche.default_color = Color("ff6fd8")
	vue.add_child(fleche)
	var regard := etiquette(Vector2(576, 140), "", 18, Color("ff6fd8"))
	get_tree().process_frame.connect(func():
		if is_instance_valid(fleche):
			var f := sol.get_facing()
			fleche.points = PackedVector2Array([heros.position + Vector2(0, -40), heros.position + Vector2(0, -40) + f * 40])
			regard.text = "get_facing() = %s · Visuel.scale.x = %s" % [f, heros.get_node("Visuel").scale.x])
	code("$GroundMovement2D.visual_path = NodePath(\"../Sprite2D\")")
	await piloter(sol, [[1.2, Vector2.RIGHT], [0.4, Vector2.ZERO], [1.4, Vector2.LEFT], [0.4, Vector2.ZERO], [1.0, Vector2.RIGHT], [0.5, Vector2.ZERO]])


func _camera_relative() -> void:
	sujet("camera_relative : « avant » selon la caméra")
	var vues := []
	for i in 2:
		var cadre := Rect2(56 + i * 528, 140, 512, 330)
		var vue := vue_3d(cadre)
		bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(40, 1, 40))
		for x in range(-3, 4):
			Formes.cube(vue, Vector3(x * 4, 0.3, -12), Formes.DECOR, Vector3(1, 0.6, 1))
		var heros := heros_3d(vue, Vector3(0, 1, 0), {"Sol": "GroundMovement3D"}, {"Sol": {"camera_relative": i == 0, "visual_path": NodePath("../Visuel")}}, VARIANTES[i])
		var pivot := Node3D.new()
		vue.add_child(pivot)
		var camera := vue.get_camera_3d()
		camera.reparent(pivot)
		camera.position = Vector3(0, 5, 9)
		camera.rotation_degrees.x = -25
		vues.append([heros, pivot])
		etiquette(Vector2(cadre.get_center().x, cadre.end.y + 24), ["camera_relative = true", "false"][i], 17, VARIANTES[i])
	code("$GroundMovement3D.camera_relative = true\n$GroundMovement3D.move(Vector2(0, -1))   # « avant »")
	legende("Commande : toujours « avant » ; la caméra tourne autour du personnage")
	for paire in vues:
		paire[0].get_node("Sol").move(Vector2(0, -1))
	var t := 0.0
	while t < 8.0:
		await images()
		t += get_process_delta_time()
		for paire in vues:
			paire[1].position = paire[0].position
			paire[1].rotation.y = t * 0.6
			if paire[0].position.length() > 8.0:
				paire[0].position = Vector3(0, 1, 0)


func _turn_speed() -> void:
	sujet("rotate_toward_movement et turn_speed")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(30, 1, 30))
	var c := []
	for i in 3:
		var heros := heros_3d(vue, Vector3(-4 + i * 4, 1, 0), {"Sol": "GroundMovement3D"}, {"Sol": {"turn_speed": [3.0, 12.0, 40.0][i], "visual_path": NodePath("../Visuel"), "max_speed": 2.0}}, VARIANTES[i])
		c.append(heros.get_node("Sol"))
	for i in 3:
		etiquette(Vector2(250 + i * 326, 540), ["turn_speed = 3", "12 (défaut)", "40"][i], 17, VARIANTES[i])
	code("$GroundMovement3D.rotate_toward_movement = true\n$GroundMovement3D.turn_speed = 12.0")
	await _tous(c, [[0.7, Vector2.LEFT], [0.7, Vector2.RIGHT], [0.7, Vector2.UP], [0.7, Vector2.DOWN], [0.7, Vector2.LEFT], [0.6, Vector2.ZERO]])


# 📌 Exemples

func _ex_topdown() -> void:
	sujet("Exemple : un jeu en vue de dessus")
	var vue := vue_2d(Rect2(76, 110, 1000, 440))
	for rect in [Rect2(-480, -210, 960, 20), Rect2(-480, 190, 960, 20), Rect2(-480, -210, 20, 420), Rect2(460, -210, 20, 420), Rect2(-100, -60, 60, 120), Rect2(150, 40, 120, 40)]:
		bloc(vue, rect)
	var heros := heros_2d(vue, Vector2(-300, 0), {"Sol": "GroundMovement2D"}, {"Sol": {"mode": GroundMovement2D.MODE_TOP_DOWN, "max_speed": 150.0}})
	debug(vue)
	code("$GroundMovement2D.mode = GroundMovement2D.MODE_TOP_DOWN\n$GroundMovement2D.max_speed = 150.0\n$GroundMovement2D.set_running(Input.is_key_pressed(KEY_SHIFT))")
	await piloter(heros.get_node("Sol"), [[1.2, Vector2(1, -1)], [1.0, Vector2.RIGHT, "course"], [1.0, Vector2(0, 1)], [1.2, Vector2(-1, 0.3), "course"], [1.0, Vector2.UP], [0.5, Vector2.ZERO]])


func _ex_glace() -> void:
	sujet("Exemple : un sol glissant (glace)")
	var vue := monde_2d()
	arene(vue)
	var glace := Area2D.new()
	glace.position = Vector2(150, 170)
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = Vector2(500, 60)
	glace.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = PackedVector2Array([Vector2(-250, 30), Vector2(250, 30), Vector2(250, 42), Vector2(-250, 42)])
	dessin.color = Color("bfe6ff")
	glace.add_child(dessin)
	vue.add_child(glace)
	var heros := heros_2d(vue, Vector2(-450, 150), {"GroundMovement2D": "GroundMovement2D"})
	glace.body_entered.connect(func(corps: Node2D):
		var g := corps.get_node_or_null("GroundMovement2D") as GroundMovement2D
		if g:
			g.acceleration = 300.0
			g.deceleration = 150.0)
	glace.body_exited.connect(func(corps: Node2D):
		var g := corps.get_node_or_null("GroundMovement2D") as GroundMovement2D
		if g:
			g.acceleration = 1500.0
			g.deceleration = 2000.0)
	debug(vue)
	code("func _on_glace_body_entered(corps):\n\tg.acceleration = 300.0\n\tg.deceleration = 150.0")
	await piloter(heros.get_node("GroundMovement2D"), [[1.6, Vector2.RIGHT], [1.0, Vector2.ZERO], [0.8, Vector2.LEFT], [1.2, Vector2.ZERO], [1.6, Vector2.LEFT], [0.6, Vector2.ZERO]])


func _ex_effets() -> void:
	sujet("Exemple : effets au saut et à l'atterrissage")
	var vue := monde_2d()
	arene(vue)
	var heros := heros_2d(vue, Vector2(-300, 150), {"GroundMovement2D": "GroundMovement2D"})
	var antenne := Jiggle2D.new()
	antenne.position = Vector2(0, -30)
	var pompon := Polygon2D.new()
	pompon.polygon = Formes.rectangle(Vector2(12, 12))
	pompon.position = Vector2(0, -22)
	pompon.color = ACCENT
	antenne.add_child(pompon)
	heros.add_child(antenne)
	var sol: GroundMovement2D = heros.get_node("GroundMovement2D")
	var journal := etiquette(Vector2(576, 140), "", 20, ACCENT)
	sol.jumped.connect(func():
		journal.text = "jumped"
		antenne.impulse(Vector2(0, 250)))
	sol.landed.connect(func():
		journal.text = "landed : poussière"
		antenne.impulse(Vector2(0, -350))
		for i in 6:
			var grain := Formes.disque(vue, heros.position + Vector2(randf_range(-20, 20), 22), Color(0.8, 0.8, 0.75, 0.8), 5)
			var animation := grain.create_tween().set_parallel()
			animation.tween_property(grain, "position", grain.position + Vector2(randf_range(-40, 40), -randf_range(5, 25)), 0.5)
			animation.tween_property(grain, "modulate:a", 0.0, 0.5)
			animation.chain().tween_callback(grain.queue_free))
	code("$GroundMovement2D.jumped.connect($SonSaut.play)\n$GroundMovement2D.landed.connect(func():\n\t$Poussiere.restart()\n\t$Jiggle2D.impulse(Vector2(0, 300)))")
	for i in 4:
		await piloter(sol, [[0.6, Vector2.RIGHT if i % 2 == 0 else Vector2.LEFT, "saut_long"], [0.8, Vector2.ZERO]])


func _ex_ia3d() -> void:
	sujet("Exemple : un PNJ 3D qui suit un chemin (NavigationAgent3D)")
	var vue := monde_3d()
	vue.get_camera_3d().position = Vector3(0, 14, 12)
	vue.get_camera_3d().rotation_degrees.x = -50
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(16, 1, 16))
	bloc_3d(vue, Vector3(0, 0.75, 0), Vector3(4, 1.5, 4), Formes.DECOR)
	# Maillage de navigation : quatre bandes autour de l'obstacle central (arêtes partagées)
	var maillage := NavigationMesh.new()
	maillage.vertices = PackedVector3Array([Vector3(-8, 0, -8), Vector3(8, 0, -8), Vector3(8, 0, -2), Vector3(2, 0, -2), Vector3(-2, 0, -2), Vector3(-8, 0, -2),
		Vector3(-8, 0, 2), Vector3(-2, 0, 2), Vector3(2, 0, 2), Vector3(8, 0, 2), Vector3(8, 0, 8), Vector3(-8, 0, 8)])
	for polygone in [[0, 1, 2, 3, 4, 5], [5, 4, 7, 6], [3, 2, 9, 8], [6, 7, 8, 9, 10, 11]]:
		maillage.add_polygon(PackedInt32Array(polygone))
	var region := NavigationRegion3D.new()
	region.navigation_mesh = maillage
	vue.add_child(region)
	var pnj := heros_3d(vue, Vector3(-6, 1, 6), {"GroundMovement3D": "GroundMovement3D"}, {"GroundMovement3D": {"visual_path": NodePath("../Visuel")}}, MAUVAIS.lightened(0.2))
	var agent := NavigationAgent3D.new()
	agent.path_desired_distance = 0.6
	agent.target_desired_distance = 0.6
	agent.debug_enabled = true
	pnj.add_child(agent)
	var sol: GroundMovement3D = pnj.get_node("GroundMovement3D")
	code("var vers := agent.get_next_path_position() - global_position\n$GroundMovement3D.move(Vector2(vers.x, vers.z).normalized())")
	await get_tree().physics_frame
	for cible in [Vector3(6, 0, -6), Vector3(-6, 0, -6), Vector3(6, 0, 6)]:
		agent.target_position = cible
		var marque := Formes.sphere(vue, cible + Vector3(0, 0.3, 0), BON, 0.3)
		var t := 0.0
		while t < 5.0 and not agent.is_navigation_finished():
			await get_tree().physics_frame
			t += get_physics_process_delta_time()
			var vers := agent.get_next_path_position() - pnj.global_position
			sol.move(Vector2(vers.x, vers.z).normalized())
		sol.move(Vector2.ZERO)
		marque.queue_free()
		await secondes(0.3)


func _ex_gravite() -> void:
	sujet("Exemple : inverser la gravité")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(-800, -180, 1600, 30))
	var heros := heros_2d(vue, Vector2(-300, 150), {"GroundMovement2D": "GroundMovement2D"})
	var sol: GroundMovement2D = heros.get_node("GroundMovement2D")
	debug(vue)
	code("func inverser() -> void:\n\t$GroundMovement2D.gravity_scale *= -1.0\n\tup_direction = -up_direction\n\t$Sprite2D.flip_v = not $Sprite2D.flip_v")
	for i in 3:
		await piloter(sol, [[1.0, Vector2.RIGHT if i % 2 == 0 else Vector2.LEFT]])
		legende("inverser()")
		sol.gravity_scale *= -1.0
		heros.up_direction = -heros.up_direction
		heros.get_node("Visuel").scale.y *= -1
		await piloter(sol, [[1.2, Vector2.RIGHT if i % 2 == 0 else Vector2.LEFT]])
