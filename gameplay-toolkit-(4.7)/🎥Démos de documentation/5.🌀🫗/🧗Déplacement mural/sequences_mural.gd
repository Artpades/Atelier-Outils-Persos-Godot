extends "res://🎥Démos de documentation/5.🌀🫗/sequences_mouvement.gd"

## 🧗 Plans filmés de la page Déplacement mural (mouvement/deplacement-mural.html) : WallMovement2D / 3D.


func titre_module() -> String:
	return "🧗 Déplacement mural · WallMovement2D / 3D"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-WallMovement2D": _classe_2d,
		"classe-WallMovement3D": _classe_3d,
		"WallMovement2D.slide_speed": _slide_speed,
		"WallMovement2D.jump_away_velocity": _jump_away,
		"WallMovement2D.can_climb": _can_climb,
		"WallMovement2D.require_input_toward_wall": _require_input,
		"ex-escalade": _ex_escalade,
		"ex-capacite": _ex_capacite,
		"ex-anim": _ex_anim,
		"ex-auto": _ex_auto,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Un puits étroit remonté en enchaînant des sauts muraux, puis une glissade lente le long d'une paroi (<code>debug_mode</code> : normale du mur, état).",
		"classe-WallMovement2D": "Contre un mur hors du sol : glissade ralentie en poussant vers le mur, saut mural loin du mur.",
		"classe-WallMovement3D": "Le même comportement en 3D, entre deux murs.",
		"WallMovement2D.slide_speed": "<code>slide_speed</code> 40, 110 (défaut) et 300 px/s : vitesse de chute en glissade.",
		"WallMovement2D.jump_away_velocity": "Saut mural : <code>jump_away_velocity</code> 150, 320 (défaut) et 550, avec <code>jump_up_velocity</code> 420.",
		"WallMovement2D.can_climb": "<code>can_climb</code> : le mur se grimpe (haut / bas) à <code>climb_speed</code> au lieu de glisser.",
		"WallMovement2D.require_input_toward_wall": "true (défaut) : la glissade exige de pousser vers le mur ; false : tout contact freine la chute.",
		"ex-escalade": "<code>can_climb</code> activé seulement dans la zone grimpable (Area2D verte) : ailleurs, le personnage glisse.",
		"ex-capacite": "Sans WallMovement2D, pas de saut mural ; <code>debloquer_saut_mural()</code> l'ajoute et recrée les contextes.",
		"ex-anim": "<code>is_climbing()</code> / <code>is_sliding()</code> choisissent l'animation, <code>get_wall_normal()</code> met le dos au mur.",
		"ex-auto": "<code>require_input_toward_wall = false</code> et <code>slide_speed = 60</code> : un mur « collant ».",
	}


func schemas() -> Dictionary:
	return {
		"reglages": ["reglages.svg", "Aligner le contrôle aérien : <code>air_speed</code> = <code>max_speed</code> du sol, <code>air_acceleration</code> ≈ <code>acceleration</code> × <code>air_control</code> ; <code>stick_velocity</code> plaque légèrement le corps contre le mur pendant la glissade et l'escalade."],
	}


func reutilisations() -> Dictionary:
	var meme_3d := "Même réglage qu'en 2D, en m/s (voir la vidéo 2D)."
	var r := {
		"WallMovement2D.jump_up_velocity": "WallMovement2D.jump_away_velocity",
		"WallMovement2D.climb_speed": "WallMovement2D.can_climb",
		"WallMovement2D.air_speed": "reglages",
		"WallMovement2D.air_acceleration": "reglages",
		"WallMovement2D.stick_velocity": "reglages",
		"WallMovement2D.is_climbing": "ex-anim",
		"WallMovement2D.is_sliding": "ex-anim",
		"WallMovement2D.is_touching_wall": "ex-anim",
		"WallMovement2D.wall_jumped": "mode-emploi",
		"WallMovement3D.is_climbing": "ex-anim",
		"WallMovement3D.is_sliding": "ex-anim",
		"WallMovement3D.is_touching_wall": "ex-anim",
		"WallMovement3D.wall_jumped": "classe-WallMovement3D",
	}
	for membre in ["slide_speed", "jump_away_velocity", "jump_up_velocity", "can_climb", "climb_speed", "require_input_toward_wall", "air_speed", "air_acceleration", "stick_velocity"]:
		r["WallMovement3D." + membre] = [r.get("WallMovement2D." + membre, "WallMovement2D." + membre), meme_3d]
	return r


func _grimpeur(vue: Node, position: Vector2, reglages_mur: Dictionary = {}, couleur: Color = VARIANTES[0]) -> CharacterBody2D:
	return heros_2d(vue, position, {"Sol": "GroundMovement2D", "Mur": "WallMovement2D", "Directeur": "MovementDirector"}, {"Mur": reglages_mur}, couleur)


## Colonnes avec un mur à droite de chaque personnage ; renvoie les directeurs.
func _colonnes_mur(propriete: String, valeurs: Array, textes: Array, communs: Dictionary = {}) -> Array:
	var vue := monde_2d()
	arene(vue)
	var directeurs := []
	var n := valeurs.size()
	var pas := 1000.0 / n
	for i in n:
		var x := -500.0 + pas * i
		bloc(vue, Rect2(x + pas - 60, -400, 30, 600))
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		var corps := _grimpeur(vue, Vector2(x + pas - 100, -150), reglages, VARIANTES[i % 4])
		directeurs.append(corps.get_node("Directeur"))
		etiquette(Vector2(576 + x + pas / 2.0, 545), textes[i], 16, VARIANTES[i % 4])
	debug(vue)
	return directeurs


## Remontée réactive d'un puits pendant `duree` : pousse vers un mur, saute dès le contact (saut mural),
## puis pousse vers le mur opposé ; au sol, saute. `corps` : CharacterBody2D ou CharacterBody3D.
func _remonter(d: MovementDirector, corps: Node, duree: float) -> void:
	var sens := 1.0
	var attente := 0.0
	var t := 0.0
	while t < duree:
		await get_tree().physics_frame
		var delta := get_physics_process_delta_time()
		t += delta
		attente -= delta
		d.move(Vector2(sens, 0))
		# Le saut mural n'est exécuté que si le contexte « mur » est déjà actif (bascule à l'image du contact)
		var au_mur: bool = d.get_current_context() == &"mur" and corps.is_on_wall()
		if attente <= 0.0 and (au_mur or corps.is_on_floor()):
			if au_mur:
				var normale = corps.get_wall_normal()
				sens = signf(normale.x)
			d.jump()
			attente = 0.25
			get_tree().create_timer(0.18).timeout.connect(d.release_jump)
	d.move(Vector2.ZERO)


func _tous(cibles: Array, etapes: Array) -> void:
	for i in cibles.size() - 1:
		piloter(cibles[i], etapes)
	await piloter(cibles[cibles.size() - 1], etapes)


## Replace les corps des directeurs à leur point de départ (en haut).
func _replacer(directeurs: Array, y: float = -150.0) -> void:
	for d in directeurs:
		var corps: CharacterBody2D = d.get_body()
		corps.position.y = y
		corps.velocity = Vector2.ZERO


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : sauts muraux et glissade")
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.7)
	vue.get_meta("camera").position = Vector2(0, -60)
	arene(vue)
	bloc(vue, Rect2(-110, -900, 30, 1100))
	bloc(vue, Rect2(80, -900, 30, 1100))
	var heros := _grimpeur(vue, Vector2(0, 150))
	var camera: Camera2D = vue.get_meta("camera")
	get_tree().process_frame.connect(func():
		if is_instance_valid(camera):
			camera.position.y = lerpf(camera.position.y, minf(heros.position.y, -60.0), 0.1))
	debug(vue)
	var sauts := etiquette(Vector2(576, 140), "", 18, ACCENT)
	var nombre := [0]
	heros.get_node("Mur").wall_jumped.connect(func():
		nombre[0] += 1
		sauts.text = "wall_jumped × %d" % nombre[0])
	arbre(Vector2(40, 140), PackedStringArray(["CharacterBody2D", "  MovementDirector", "  GroundMovement2D", "  WallMovement2D"]))
	code("$MovementDirector.move(Vector2(Input.get_axis(\"ui_left\", \"ui_right\"), ...))\nif Input.is_action_just_pressed(\"ui_accept\"): $MovementDirector.jump()")
	var d: MovementDirector = heros.get_node("Directeur")
	legende("Remonter le puits : saut, puis saut mural d'une paroi à l'autre")
	await _remonter(d, heros, 4.0)
	legende("Glissade lente en poussant vers la paroi")
	await piloter(d, [[2.5, Vector2.LEFT], [1.0, Vector2.ZERO]])


# 💻 API

func _classe_2d() -> void:
	sujet("WallMovement2D : glissade et saut mural")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(200, -400, 30, 520))
	var heros := _grimpeur(vue, Vector2(-100, 150))
	debug(vue)
	var d: MovementDirector = heros.get_node("Directeur")
	code("$MovementDirector.move(Vector2.RIGHT)   # pousser vers le mur\n$MovementDirector.jump()   # saut mural")
	for i in 2:
		legende("Saut contre le mur, en poussant vers lui : glissade ralentie")
		await piloter(d, [[0.4, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [1.6, Vector2.RIGHT]])
		legende("Saut : rebond loin du mur")
		await piloter(d, [[0.6, Vector2.ZERO, "saut_long"], [1.0, Vector2.ZERO]])


func _classe_3d() -> void:
	sujet("WallMovement3D : entre deux murs")
	var vue := monde_3d()
	vue.get_camera_3d().position = Vector3(0, 5, 13)
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(20, 1, 10))
	bloc_3d(vue, Vector3(-2.2, 4, 0), Vector3(0.4, 9, 4), Formes.DECOR)
	bloc_3d(vue, Vector3(2.2, 4, 0), Vector3(0.4, 9, 4), Formes.DECOR)
	var heros := heros_3d(vue, Vector3(0, 1, 0), {"Sol": "GroundMovement3D", "Mur": "WallMovement3D", "Directeur": "MovementDirector"})
	debug(vue)
	var d: MovementDirector = heros.get_node("Directeur")
	code("WallMovement3D : glissade, saut mural (m/s)")
	await _remonter(d, heros, 4.0)
	await piloter(d, [[2.0, Vector2.LEFT], [0.6, Vector2.ZERO]])


func _slide_speed() -> void:
	sujet("slide_speed : vitesse de glissade")
	code("$WallMovement2D.slide_speed = 110.0")
	var d := _colonnes_mur("slide_speed", [40.0, 110.0, 300.0], ["40", "110 (défaut)", "300"])
	for i in 2:
		_replacer(d)
		await _tous(d, [[3.0, Vector2.RIGHT], [0.4, Vector2.ZERO]])


func _jump_away() -> void:
	sujet("jump_away_velocity et jump_up_velocity : le saut mural")
	code("$WallMovement2D.jump_away_velocity = 320.0\n$WallMovement2D.jump_up_velocity = 420.0")
	var d := _colonnes_mur("jump_away_velocity", [150.0, 320.0, 550.0], ["150", "320 (défaut)", "550"])
	for i in 3:
		_replacer(d)
		await _tous(d, [[0.6, Vector2.RIGHT], [1.4, Vector2.ZERO, "saut_long"]])


func _can_climb() -> void:
	sujet("can_climb et climb_speed : grimper")
	code("$WallMovement2D.can_climb = true\n$WallMovement2D.climb_speed = 120.0")
	var d := _colonnes_mur("can_climb", [false, true], ["can_climb = false", "true (climb_speed 120)"])
	_replacer(d, 100.0)
	await _tous(d, [[0.4, Vector2.RIGHT, "saut_long"], [2.0, Vector2(1, -1)], [1.0, Vector2(1, 1)], [1.0, Vector2(1, -1)], [0.5, Vector2.ZERO]])


func _require_input() -> void:
	sujet("require_input_toward_wall")
	code("$WallMovement2D.require_input_toward_wall = true")
	var d := _colonnes_mur("require_input_toward_wall", [true, false], ["true (défaut) : il faut pousser", "false : tout contact freine"])
	for i in 2:
		_replacer(d)
		await _tous(d, [[0.4, Vector2.RIGHT], [2.4, Vector2.ZERO]])


# 📌 Exemples

func _ex_escalade() -> void:
	sujet("Exemple : un mur grimpable seulement par endroits")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(200, -400, 30, 600))
	var zone := Area2D.new()
	zone.position = Vector2(200, -100)
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = Vector2(80, 220)
	zone.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(Vector2(80, 220))
	dessin.color = Color(0.36, 0.81, 0.48, 0.3)
	zone.add_child(dessin)
	vue.add_child(zone)
	var heros := _grimpeur(vue, Vector2(0, 150))
	zone.body_entered.connect(func(corps: Node2D):
		var mur := corps.get_node_or_null("Mur") as WallMovement2D
		if mur:
			mur.can_climb = true)
	zone.body_exited.connect(func(corps: Node2D):
		var mur := corps.get_node_or_null("Mur") as WallMovement2D
		if mur:
			mur.can_climb = false)
	debug(vue)
	code("func _on_zone_escalade_body_entered(corps):\n\tmur.can_climb = true\nfunc _on_zone_escalade_body_exited(corps):\n\tmur.can_climb = false")
	var d: MovementDirector = heros.get_node("Directeur")
	await piloter(d, [[0.6, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [1.0, Vector2(1, -1)], [2.0, Vector2(1, -1)], [1.4, Vector2(1, 1)], [0.6, Vector2.ZERO]])


func _ex_capacite() -> void:
	sujet("Exemple : débloquer le saut mural en cours de partie")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(200, -400, 30, 600))
	var heros := heros_2d(vue, Vector2(0, 150), {"Sol": "GroundMovement2D", "Directeur": "MovementDirector"})
	debug(vue)
	var d: MovementDirector = heros.get_node("Directeur")
	legende("Sans WallMovement2D : le personnage tombe le long du mur")
	await piloter(d, [[0.3, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [1.0, Vector2.RIGHT], [0.5, Vector2.ZERO, "saut_long"], [0.6, Vector2.ZERO]])
	code("var mur := WallMovement2D.new()\n$Joueur.add_child(mur)\n$Joueur/MovementDirector.create_default_contexts()")
	legende("debloquer_saut_mural()")
	var mur := WallMovement2D.new()
	mur.name = "WallMovement2D"
	heros.add_child(mur)
	d.create_default_contexts()
	debug(vue)
	heros.position.x = 0
	await piloter(d, [[0.3, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [1.2, Vector2.RIGHT], [0.8, Vector2.ZERO, "saut_long"], [0.6, Vector2.ZERO]])


func _ex_anim() -> void:
	sujet("Exemple : la bonne animation (glissade, escalade)")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(200, -400, 30, 600))
	var heros := _grimpeur(vue, Vector2(0, 150))
	var mur: WallMovement2D = heros.get_node("Mur")
	var etat := etiquette(Vector2(576, 140), "", 20, ACCENT)
	get_tree().process_frame.connect(func():
		if not is_instance_valid(etat):
			return
		var anim := "—"
		if mur.active:
			anim = "escalade" if mur.is_climbing() else ("glissade" if mur.is_sliding() else "mur")
		etat.text = "animation : %s · is_touching_wall() = %s · normale = %s" % [anim, mur.is_touching_wall(), heros.get_wall_normal() if heros.is_on_wall() else Vector2.ZERO]
		if mur.active and heros.is_on_wall():
			heros.get_node("Visuel").scale.x = -1.0 if heros.get_wall_normal().x < 0.0 else 1.0)
	code("if mur.is_climbing(): $AnimationPlayer.play(\"escalade\")\nelif mur.is_sliding(): $AnimationPlayer.play(\"glissade\")\n$Sprite2D.flip_h = get_wall_normal().x > 0.0")
	var d: MovementDirector = heros.get_node("Directeur")
	await piloter(d, [[0.5, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [1.5, Vector2.RIGHT]])
	mur.can_climb = true
	await piloter(d, [[1.5, Vector2(1, -1)], [0.6, Vector2.ZERO, "saut_long"], [1.0, Vector2.ZERO]])


func _ex_auto() -> void:
	sujet("Exemple : glisser sans pousser vers le mur")
	code("$WallMovement2D.require_input_toward_wall = false\n$WallMovement2D.slide_speed = 60.0")
	var d := _colonnes_mur("slide_speed", [110.0, 60.0], ["réglages par défaut", "mur collant"], {})
	d[1].get_body().get_node("Mur").require_input_toward_wall = false
	for i in 2:
		_replacer(d)
		await _tous(d, [[0.4, Vector2.RIGHT], [2.6, Vector2.ZERO]])
