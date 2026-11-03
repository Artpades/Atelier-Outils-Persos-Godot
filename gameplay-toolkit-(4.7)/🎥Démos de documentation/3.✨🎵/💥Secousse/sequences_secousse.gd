extends "res://🎥Démos de documentation/socle/sequences.gd"

## 💥 Plans filmés de la page Secousse (Documentation/modules/effets/secousse.html).

const Traceur := preload("res://🎥Démos de documentation/socle/traceur.gd")


func titre_module() -> String:
	return "💥 Secousse · FXShake"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-FXShake": _classe,
		"FXShake.duration": _duration,
		"FXShake.frequency": _frequency,
		"FXShake.max_amplitude": _max_amplitude,
		"FXShake.decrease_over_time": _decrease_over_time,
		"ex-proportionnel": _ex_proportionnel,
		"ex-ui": _ex_ui,
		"ex-3d": _ex_3d,
		"ex-continu": _ex_continu,
		"ex-mobile": _ex_mobile,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Les trois étapes dans le moteur : PlayerFX sous la Camera2D, réglages, puis <code>play()</code> à chaque explosion.",
		"classe-FXShake": "La même secousse sur un Node2D, un Control et un Node3D (caméra 3D à 0,15 m).",
		"FXShake.duration": "0,25 s, 0,75 s, 1,5 s et 0 (sans fin : arrêtée par <code>stop()</code>), joués en même temps.",
		"FXShake.frequency": "4, 15 et 40 Hz, à amplitude constante : roulis lent ou vibration nerveuse.",
		"FXShake.max_amplitude": "4, 12 et 30 px : l'écart maximal au début de la secousse.",
		"FXShake.decrease_over_time": "true : l'amplitude s'éteint progressivement · false : pleine amplitude puis arrêt net.",
		"ex-proportionnel": "Trois explosions de plus en plus proches : l'amplitude passe de 2,9 à 18,4.",
		"ex-ui": "Deux mots de passe refusés : le champ tremble 0,3 s à chaque refus.",
		"ex-3d": "Impacts à 0,15 m, puis la valeur par défaut (10 m) pour comparer.",
		"ex-continu": "Moteur allumé puis éteint deux fois : la vibration dure exactement tant qu'il tourne.",
		"ex-mobile": "En haut, la secousse fige le corps puis le ramène en arrière ; en bas, seul le sprite tremble.",
	}


func schemas() -> Dictionary:
	return {
		"FXShake": ["cycle.svg", "Cycle d'une secousse : le décalage suit un bruit continu dont l'enveloppe décroît jusqu'à <code>duration</code>, puis la position d'origine est rendue."],
	}


## Ajoute à `cible` un PlayerFX jouant un FXShake réglé par `reglages` ({propriété: valeur}).
func _secousse(cible: Node, reglages: Dictionary = {}) -> PlayerFX:
	var effet := FXShake.new()
	for propriete in reglages:
		effet.set(propriete, reglages[propriete])
	var joueur := PlayerFX.new()
	joueur.special_effects = [effet]
	cible.add_child(joueur)
	return joueur


## Traceur du décalage horizontal de `cible` par rapport à sa position actuelle, centré sous `position`.
func _traceur(position: Vector2, cible: Node, couleur: Color, echelle: float = 1.5) -> Traceur:
	var traceur := Traceur.new()
	var origine: Vector2 = cible.position
	traceur.mesure = func() -> float: return cible.position.x - origine.x
	traceur.couleur = couleur
	traceur.largeur = 220.0
	traceur.echelle = echelle
	traceur.position = position - Vector2(110, 0)
	scene.add_child(traceur)
	return traceur


## Petit niveau 2D (sol, plateformes, personnage) dans une vue de jeu ; renvoie la vue.
func _niveau(cadre: Rect2) -> SubViewport:
	var vue := vue_2d(cadre)
	var sol := Polygon2D.new()
	sol.polygon = PackedVector2Array([Vector2(-900, 120), Vector2(900, 120), Vector2(900, 600), Vector2(-900, 600)])
	sol.color = Color("3a4256")
	vue.add_child(sol)
	for bloc in [Rect2(-260, 40, 120, 24), Rect2(80, -20, 160, 24), Rect2(-60, 70, 70, 50)]:
		var forme := Polygon2D.new()
		forme.polygon = PackedVector2Array([bloc.position, Vector2(bloc.end.x, bloc.position.y), bloc.end, Vector2(bloc.position.x, bloc.end.y)])
		forme.color = Color("56607a")
		vue.add_child(forme)
	var joueur := Polygon2D.new()
	joueur.polygon = PackedVector2Array([Vector2(-14, -40), Vector2(14, -40), Vector2(14, 0), Vector2(-14, 0)])
	joueur.color = VARIANTES[0]
	joueur.position = Vector2(-180, 120)
	vue.add_child(joueur)
	return vue


## Explosion (disque qui grandit et s'efface) dans `parent` à `position`.
func _explosion(parent: Node, position: Vector2, rayon: float = 60.0) -> void:
	var disque := Node2D.new()
	disque.position = position
	disque.draw.connect(func(): disque.draw_circle(Vector2.ZERO, rayon, Color(1, 0.75, 0.3)))
	parent.add_child(disque)
	disque.scale = Vector2.ONE * 0.2
	var animation := disque.create_tween().set_parallel()
	animation.tween_property(disque, "scale", Vector2.ONE, 0.25).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	animation.tween_property(disque, "modulate:a", 0.0, 0.45)
	animation.chain().tween_callback(disque.queue_free)


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi")
	var vue := _niveau(Rect2(420, 110, 680, 400))
	var camera: Camera2D = vue.get_meta("camera")
	arbre(Vector2(40, 150), PackedStringArray(["Niveau", "  Camera2D", "    PlayerFX", "  Joueur"]))
	etiquette(Vector2(200, 300), "special_effects = [FXShake]", 16, ACCENT)

	legende("1. Un PlayerFX sous la Camera2D, avec un FXShake dans special_effects")
	await secondes(2.5)
	var joueur := _secousse(camera, {"duration": 0.4, "max_amplitude": 12.0, "frequency": 15.0})
	legende("2. Régler la secousse dans l'inspecteur")
	code("duration = 0.4\nmax_amplitude = 12\nfrequency = 15")
	await secondes(2.5)
	legende("3. Déclencher depuis le jeu : toute la vue tremble, puis la caméra revient en place")
	code("func _on_explosion() -> void:\n\t$Camera2D/PlayerFX.play()")
	for position in [Vector2(120, 60), Vector2(-60, 90), Vector2(220, 30)]:
		await secondes(0.8)
		_explosion(vue, position)
		joueur.play()
		await secondes(0.6)


# 💻 API

func _classe() -> void:
	sujet("FXShake : Node2D, Control ou Node3D")
	legende("FXShake secoue la position de sa cible, puis la remet exactement en place")
	var carre_2d := carre(Vector2(220, 300), VARIANTES[0], 110)
	etiquette(Vector2(220, 420), "Node2D")

	var bouton := Button.new()
	bouton.text = "Valider"
	bouton.add_theme_font_size_override("font_size", 24)
	bouton.position = Vector2(500, 270)
	bouton.size = Vector2(150, 60)
	scene.add_child(bouton)
	etiquette(Vector2(575, 420), "Control")

	var vue := vue_3d(Rect2(790, 200, 300, 200))
	var cube_3d := cube(vue, Vector3(0, 0.6, 0), VARIANTES[1], Vector3.ONE * 1.4)
	cube(vue, Vector3(0, -0.2, 0), Color("3a4256"), Vector3(6, 0.2, 6))
	etiquette(Vector2(940, 420), "Node3D")

	var joueurs := [
		_secousse(carre_2d, {"max_amplitude": 14.0}),
		_secousse(bouton, {"max_amplitude": 10.0}),
		_secousse(cube_3d, {"max_amplitude": 0.15}),
	]
	code("$Cible/PlayerFX.play()")
	for i in 4:
		await secondes(0.7)
		for joueur in joueurs:
			joueur.play()
		await secondes(0.8)


## Colonnes de carrés comparant les `valeurs` de `propriete` (autres réglages : `communs`) ;
## renvoie les PlayerFX, tous joués ensemble par _jouer_colonnes.
func _colonnes(propriete: String, valeurs: Array, textes: Array, communs: Dictionary) -> Array[PlayerFX]:
	var joueurs: Array[PlayerFX] = []
	var pas := TAILLE.x / (valeurs.size() + 1)
	for i in valeurs.size():
		var x := pas * (i + 1)
		var cible := carre(Vector2(x, 250), VARIANTES[i], 80)
		etiquette(Vector2(x, 330), textes[i], 18, VARIANTES[i])
		_traceur(Vector2(x, 430), cible, VARIANTES[i])
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		joueurs.append(_secousse(cible, reglages))
	return joueurs


func _jouer_colonnes(joueurs: Array[PlayerFX], repetitions: int, pause: float) -> void:
	for i in repetitions:
		await secondes(0.5)
		for joueur in joueurs:
			joueur.play()
		await secondes(pause)


func _duration() -> void:
	sujet("duration : durée de la secousse (s)")
	legende("Courbe : décalage horizontal de chaque carré. duration = 0 : jusqu'à stop()")
	var joueurs := _colonnes("duration", [0.25, 0.75, 1.5, 0.0], ["0,25 s", "0,75 s", "1,5 s", "0 (sans fin)"], {"max_amplitude": 20.0})
	code("shake.duration = 0.75")
	await _jouer_colonnes(joueurs, 1, 2.4)
	code("$Carre/PlayerFX.stop()  # duration = 0")
	joueurs[3].stop()
	await secondes(1.0)
	code("shake.duration = 0.75")
	await _jouer_colonnes(joueurs, 1, 2.4)
	joueurs[3].stop()
	await secondes(1.0)


func _frequency() -> void:
	sujet("frequency : vitesse des oscillations (Hz)")
	legende("Basse : roulis lourd · haute : vibration nerveuse")
	var joueurs := _colonnes("frequency", [4.0, 15.0, 40.0], ["4 Hz", "15 Hz", "40 Hz"],
		{"max_amplitude": 20.0, "duration": 2.2, "decrease_over_time": false})
	code("shake.frequency = 15.0")
	await _jouer_colonnes(joueurs, 2, 2.6)


func _max_amplitude() -> void:
	sujet("max_amplitude : écart maximal (pixels en 2D, mètres en 3D)")
	legende("L'écart au début de la secousse, avant la décroissance")
	var joueurs := _colonnes("max_amplitude", [4.0, 12.0, 30.0], ["4 px", "12 px", "30 px"], {"duration": 1.2})
	code("shake.max_amplitude = 12.0")
	await _jouer_colonnes(joueurs, 3, 1.6)


func _decrease_over_time() -> void:
	sujet("decrease_over_time : décroissance jusqu'à l'arrêt")
	legende("true : l'amplitude baisse jusqu'à 0 · false : pleine amplitude puis arrêt net")
	var joueurs := _colonnes("decrease_over_time", [true, false], ["true", "false"], {"duration": 1.5, "max_amplitude": 25.0})
	code("shake.decrease_over_time = true")
	await _jouer_colonnes(joueurs, 3, 2.0)


# 📌 Exemples

func _ex_proportionnel() -> void:
	sujet("Exemple : plus l'explosion est proche, plus ça secoue")
	var vue := _niveau(Rect2(76, 170, 1000, 380))
	var camera: Camera2D = vue.get_meta("camera")
	camera.zoom = Vector2.ONE * 0.62
	var joueur := _secousse(camera, {"duration": 0.45})
	var secousse: FXShake = joueur.special_effects[0]
	code("var distance := $Camera2D.global_position.distance_to(position_explosion)\nsecousse.max_amplitude = remap(clampf(distance, 0.0, 800.0), 0.0, 800.0, 20.0, 2.0)")
	for position in [Vector2(760, 40), Vector2(380, 80), Vector2(40, 60)]:
		await secondes(1.0)
		var distance := camera.global_position.distance_to(position)
		secousse.max_amplitude = remap(clampf(distance, 0.0, 800.0), 0.0, 800.0, 20.0, 2.0)
		legende("distance %d px → max_amplitude %.1f" % [distance, secousse.max_amplitude])
		_explosion(vue, position, 70.0)
		joueur.play()
		await secondes(1.0)


func _ex_ui() -> void:
	sujet("Exemple : un champ de saisie refusé tremble")
	var champ := LineEdit.new()
	champ.placeholder_text = "Mot de passe"
	champ.secret = true
	champ.add_theme_font_size_override("font_size", 24)
	champ.position = Vector2(406, 250)
	champ.size = Vector2(340, 52)
	scene.add_child(champ)
	var message := etiquette(Vector2(576, 340), "", 20, MAUVAIS)
	var joueur := _secousse(champ, {"duration": 0.3, "max_amplitude": 8.0, "frequency": 25.0})
	code("$ChampMotDePasse/PlayerFX.play()")
	legende("Le LineEdit est hors conteneur : sinon le conteneur le replacerait aussitôt")
	for tentative in ["soleil", "azerty1"]:
		champ.text = ""
		message.text = ""
		await secondes(0.6)
		for lettre in tentative:
			champ.text += lettre
			await secondes(0.09)
		await secondes(0.35)
		message.text = "Mot de passe refusé"
		joueur.play()
		await secondes(1.2)


func _ex_3d() -> void:
	sujet("Exemple : secouer une caméra 3D (amplitude en mètres)")
	var vue := vue_3d(Rect2(76, 110, 1000, 400))
	var camera := vue.get_camera_3d()
	cube(vue, Vector3(0, -0.1, 0), Color("3a4256"), Vector3(14, 0.2, 10))
	for x in [-3.0, -1.5, 1.8, 3.4]:
		cube(vue, Vector3(x, 0.4, -1.5 - absf(x) * 0.3), Color("56607a"), Vector3(0.8, 0.8, 0.8))
	var bloc := cube(vue, Vector3(0, 6, 0), VARIANTES[1], Vector3(1.4, 1.4, 1.4))
	var joueur := _secousse(camera, {"max_amplitude": 0.15, "duration": 0.5})
	var secousse: FXShake = joueur.special_effects[0]
	for amplitude in [0.15, 0.15, 10.0]:
		secousse.max_amplitude = amplitude
		code("tremblement.max_amplitude = %s\n$Camera3D/PlayerFX.play()" % amplitude)
		legende("max_amplitude = 0,15 m : un impact lourd" if amplitude < 1.0 else "La valeur par défaut (10) vaut 10 mètres en 3D : beaucoup trop !")
		bloc.position.y = 6.0
		await secondes(0.6)
		var chute := bloc.create_tween()
		chute.tween_property(bloc, "position:y", 0.7, 0.45).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		await chute.finished
		joueur.play()
		await secondes(1.3)


func _ex_continu() -> void:
	sujet("Exemple : vibrer tant qu'une condition dure (duration = 0)")
	var moteur := carre(Vector2(400, 280), Color("6b7590"), 140)
	var helice := Polygon2D.new()
	helice.polygon = PackedVector2Array([Vector2(-50, -8), Vector2(50, -8), Vector2(50, 8), Vector2(-50, 8)])
	helice.color = Color("cfd6e6")
	moteur.add_child(helice)
	var etat := etiquette(Vector2(400, 400), "Moteur : éteint", 20)
	var traceur := _traceur(Vector2(820, 280), moteur, ACCENT, 12.0)
	etiquette(Vector2(820, 350), "décalage × 12", 14)
	var joueur := _secousse(moteur, {"duration": 0.0, "max_amplitude": 1.5})
	var allume := false
	var rotation_helice := func(delta: float) -> void:
		if allume:
			helice.rotation += delta * 18.0
	get_tree().process_frame.connect(func(): rotation_helice.call(get_process_delta_time()))
	for i in 2:
		await secondes(1.0)
		allume = true
		etat.text = "Moteur : allumé"
		code("set_moteur_allume(true)  # PlayerFX.play()")
		joueur.play()
		await secondes(2.2)
		allume = false
		etat.text = "Moteur : éteint"
		code("set_moteur_allume(false)  # PlayerFX.stop()")
		joueur.stop()
	traceur.mesure = Callable()
	await secondes(0.8)


func _ex_mobile() -> void:
	sujet("Exemple : secouer un personnage qui se déplace")
	var lignes := [
		{"y": 200.0, "texte": "PlayerFX sous le corps : la secousse impose la position", "bon": false},
		{"y": 420.0, "texte": "PlayerFX sous le Sprite2D : le corps continue d'avancer", "bon": true},
	]
	var corps: Array[Node2D] = []
	var joueurs: Array[PlayerFX] = []
	for ligne in lignes:
		var y: float = ligne["y"]
		etiquette(Vector2(576, y - 80), ("✓ " if ligne["bon"] else "✗ ") + ligne["texte"], 18, BON if ligne["bon"] else MAUVAIS)
		var piste := Line2D.new()
		piste.points = PackedVector2Array([Vector2(80, y + 45), Vector2(1072, y + 45)])
		piste.width = 2
		piste.default_color = Color(1, 1, 1, 0.15)
		scene.add_child(piste)
		var corps_ligne := Node2D.new()
		corps_ligne.position = Vector2(140, y)
		scene.add_child(corps_ligne)
		var sprite := carre(Vector2.ZERO, VARIANTES[0] if ligne["bon"] else VARIANTES[2], 70)
		sprite.reparent(corps_ligne, false)
		corps.append(corps_ligne)
		joueurs.append(_secousse(sprite if ligne["bon"] else corps_ligne, {"duration": 0.5, "max_amplitude": 10.0}))
	code("func toucher() -> void:\n\t$Sprite2D/PlayerFX.play()")
	legende("Touché toutes les 1,2 s, en avançant à 110 px/s")
	var temps := 0.0
	var prochain_coup := 0.6
	while temps < 8.0:
		var delta := get_process_delta_time()
		await images()
		temps += delta
		for c in corps:
			c.position.x += 110.0 * delta
		if temps >= prochain_coup:
			prochain_coup += 1.2
			for joueur in joueurs:
				joueur.play()
