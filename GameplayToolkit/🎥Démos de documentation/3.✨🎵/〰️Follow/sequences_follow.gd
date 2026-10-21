extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## 〰️ Plans filmés de la page Follow (effets/follow.html) : FXFollower.


func titre_module() -> String:
	return "〰️ Follow · FXFollower"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-FXFollower": _classe,
		"FXFollower.follow_speed": _follow_speed,
		"FXFollower.use_interpolation": _use_interpolation,
		"FXFollower.auto_zoom_2d": _auto_zoom,
		"FXFollower.min_zoom": _min_max_zoom,
		"FXFollower.zoom_margin": _zoom_margin,
		"ex-familier": _ex_familier,
		"ex-ajout-joueur": _ex_ajout_joueur,
		"ex-3d": _ex_3d,
		"ex-cinematique": _ex_cinematique,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "La Camera2D reste entre les deux joueurs et dézoome quand ils s'éloignent. En surimpression : leur rectangle englobant et la marge <code>zoom_margin</code>.",
		"classe-FXFollower": "La cible rejoint le centre des nœuds suivis ; une Camera2D règle aussi son zoom pour les garder à l'écran.",
		"FXFollower.follow_speed": "Trois suiveurs à <code>follow_speed</code> 1, 5 et 20 derrière la même cible.",
		"FXFollower.use_interpolation": "true : la cible rejoint le centre en douceur · false : elle y est placée à chaque image.",
		"FXFollower.auto_zoom_2d": "Avec le zoom automatique, la caméra dézoome pour garder les deux joueurs ; sans, ils sortent de l'écran.",
		"FXFollower.min_zoom": "Joueurs très écartés : <code>min_zoom</code> borne le dézoom (ils sortent alors de l'écran) ; proches : <code>max_zoom</code> borne le rapprochement.",
		"FXFollower.zoom_margin": "<code>zoom_margin</code> 30 et 200 px : l'espace gardé autour des joueurs.",
		"ex-familier": "Le familier suit un Marker2D du joueur avec <code>follow_speed = 3</code> : un suivi élastique.",
		"ex-ajout-joueur": "Un troisième joueur rejoint la partie : on l'ajoute à <code>targets</code> et la caméra en tient compte.",
		"ex-3d": "Un pivot suit le groupe (FXFollower) et la caméra, enfant du pivot, le vise (FXTargeter).",
		"ex-cinematique": "<code>stop()</code> fige la caméra pendant la cinématique, <code>play()</code> la ramène en douceur vers les joueurs.",
	}


func reutilisations() -> Dictionary:
	return {
		"FXFollower.targets": "mode-emploi",
		"FXFollower.max_zoom": "FXFollower.min_zoom",
	}


func _suivi(cible: Node, chemins: Array, reglages: Dictionary = {}) -> PlayerFX:
	var suivi: FXFollower = effet("FXFollower", reglages)
	var liste: Array[NodePath] = []
	for chemin in chemins:
		liste.append(NodePath(chemin))
	suivi.targets = liste
	return joueur_fx(cible, suivi)


## Décor de niveau étendu (repères au sol) dans une vue.
func _decor(vue: Node) -> void:
	for i in range(-12, 13):
		var bloc := Polygon2D.new()
		bloc.polygon = Formes.rectangle(Vector2(60, 30 + (i * 37) % 70))
		bloc.position = Vector2(i * 160, 140)
		bloc.color = Formes.DECOR
		vue.add_child(bloc)
	var sol := Polygon2D.new()
	sol.polygon = PackedVector2Array([Vector2(-3000, 160), Vector2(3000, 160), Vector2(3000, 900), Vector2(-3000, 900)])
	sol.color = Formes.SOL
	vue.add_child(sol)


func _joueur(vue: Node, nom: String, couleur: Color) -> Node2D:
	var j := Formes.carre(vue, Vector2.ZERO, couleur, 36)
	j.name = nom
	return j


## Rectangle englobant des `noeuds` et sa marge, dessinés dans la vue.
func _surimpression(vue: Node, noeuds: Array, marge: float) -> void:
	var dessin := Node2D.new()
	dessin.z_index = 10
	vue.add_child(dessin)
	dessin.draw.connect(func():
		var boite := Rect2(noeuds[0].global_position, Vector2.ZERO)
		for n in noeuds:
			boite = boite.expand(n.global_position)
		dessin.draw_rect(boite, Color(1, 1, 1, 0.7), false, 2.0)
		dessin.draw_rect(boite.grow(marge), Color(0.95, 0.64, 0.23, 0.8), false, 2.0))
	get_tree().process_frame.connect(dessin.queue_redraw)


func _bouger_joueurs(a: Node2D, b: Node2D, duree: float, amplitude: float = 600.0) -> void:
	var t := 0.0
	while t < duree:
		await images()
		t += get_process_delta_time()
		var ecart := 80.0 + (1.0 - cos(t * 0.9)) * amplitude / 2.0
		a.position = Vector2(-ecart + sin(t * 1.3) * 60, 100 + sin(t * 2.0) * 30)
		b.position = Vector2(ecart + cos(t) * 80, 100 + cos(t * 1.7) * 40)


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : une Camera2D entre deux joueurs")
	var vue := vue_2d(Rect2(306, 110, 800, 440))
	_decor(vue)
	var a := _joueur(vue, "Joueur1", VARIANTES[0])
	var b := _joueur(vue, "Joueur2", VARIANTES[2])
	var camera: Camera2D = vue.get_meta("camera")
	camera.name = "Camera2D"
	_surimpression(vue, [a, b], 100.0)
	_suivi(camera, ["../../Joueur1", "../../Joueur2"]).play()
	arbre(Vector2(30, 140), PackedStringArray(["Niveau", "  Camera2D", "    PlayerFX", "  Joueur1", "  Joueur2"]))
	etiquette(Vector2(150, 300), "targets = [../../Joueur1,\n                 ../../Joueur2]", 14, ACCENT)
	code("func _ready() -> void:\n\t$Camera2D/PlayerFX.play()")
	legende("Blanc : rectangle des joueurs · orange : avec zoom_margin (100 px)")
	await _bouger_joueurs(a, b, 11.0, 900.0)


# 💻 API

func _classe() -> void:
	sujet("FXFollower : suivre le centre d'un ou plusieurs nœuds")
	var vue := vue_2d(Rect2(76, 110, 1000, 420))
	_decor(vue)
	var a := _joueur(vue, "A", VARIANTES[0])
	var b := _joueur(vue, "B", VARIANTES[2])
	var camera: Camera2D = vue.get_meta("camera")
	_suivi(camera, ["../../A", "../../B"]).play()
	var drone := Formes.disque(vue, Vector2.ZERO, ACCENT, 14)
	drone.name = "Drone"
	_suivi(drone, ["../../A", "../../B"], {"follow_speed": 2.0}).play()
	code("suivi.targets = [NodePath(\"../../A\"), NodePath(\"../../B\")]\n$PlayerFX.play()")
	legende("La caméra (et le drone orange, plus lent) suivent le milieu de A et B")
	await _bouger_joueurs(a, b, 9.0, 600.0)


func _follow_speed() -> void:
	sujet("follow_speed : rapidité du suivi")
	var meneur := carre(Vector2(200, 300), Color.WHITE, 30)
	meneur.name = "Meneur"
	for i in 3:
		var suiveur := disque(Vector2(200, 300), VARIANTES[i], 14)
		suiveur.name = "S%d" % i
		_suivi(suiveur, ["../../Meneur"], {"follow_speed": [1.0, 5.0, 20.0][i]}).play()
	for i in 3:
		etiquette(Vector2(140 + i * 160, 560), ["1", "5 (défaut)", "20"][i], 18, VARIANTES[i])
	code("suivi.follow_speed = 5.0")
	var t := 0.0
	while t < 9.0:
		await images()
		t += get_process_delta_time()
		var phase := int(t / 1.5) % 4
		var vise: Vector2 = [Vector2(250, 220), Vector2(900, 200), Vector2(850, 450), Vector2(300, 430)][phase]
		meneur.position = meneur.position.lerp(vise, minf(1.0, get_process_delta_time() * 6.0))


func _use_interpolation() -> void:
	sujet("use_interpolation : rejoindre en douceur ou instantanément")
	var meneur := carre(Vector2(576, 300), Color.WHITE, 30)
	meneur.name = "Meneur"
	for i in 2:
		var suiveur := disque(Vector2(576, 300), VARIANTES[i], 16)
		suiveur.name = "S%d" % i
		_suivi(suiveur, ["../../Meneur"], {"use_interpolation": i == 0, "follow_speed": 3.0}).play()
		etiquette(Vector2(400 + i * 350, 560), ["use_interpolation = true (défaut)", "false"][i], 18, VARIANTES[i])
	code("suivi.use_interpolation = false")
	var t := 0.0
	while t < 8.0:
		await images()
		t += get_process_delta_time()
		meneur.position = Vector2(576, 320) + Vector2(cos(t * 1.4) * 380, sin(t * 2.1) * 170)


func _deux_vues(textes: Array, reglages: Array, amplitude: float = 900.0, duree: float = 10.0) -> void:
	var joueurs := []
	for i in 2:
		var cadre := Rect2(56 + i * 528, 140, 512, 330)
		var vue := vue_2d(cadre)
		_decor(vue)
		var a := _joueur(vue, "Joueur1", VARIANTES[0])
		var b := _joueur(vue, "Joueur2", VARIANTES[2])
		var r: Dictionary = reglages[i]
		_surimpression(vue, [a, b], r.get("zoom_margin", 100.0))
		_suivi(vue.get_meta("camera"), ["../../Joueur1", "../../Joueur2"], r).play()
		etiquette(Vector2(cadre.get_center().x, cadre.end.y + 24), textes[i], 17, VARIANTES[i])
		joueurs.append([a, b])
	var t := 0.0
	while t < duree:
		await images()
		t += get_process_delta_time()
		var ecart := 80.0 + (1.0 - cos(t * 0.8)) * amplitude / 2.0
		for paire in joueurs:
			paire[0].position = Vector2(-ecart, 100 + sin(t * 2.0) * 30)
			paire[1].position = Vector2(ecart, 100 + cos(t * 1.7) * 40)


func _auto_zoom() -> void:
	sujet("auto_zoom_2d : zoom automatique de la Camera2D")
	code("suivi.auto_zoom_2d = true")
	await _deux_vues(["auto_zoom_2d = true (défaut)", "false"], [{}, {"auto_zoom_2d": false}])


func _min_max_zoom() -> void:
	sujet("min_zoom et max_zoom : bornes du zoom automatique")
	code("suivi.min_zoom = Vector2(0.5, 0.5)\nsuivi.max_zoom = Vector2(2, 2)")
	await _deux_vues(["min 0,5 / max 2 (défaut)", "min 0,8 / max 1,2"], [{}, {"min_zoom": Vector2(0.8, 0.8), "max_zoom": Vector2(1.2, 1.2)}], 1400.0)


func _zoom_margin() -> void:
	sujet("zoom_margin : espace gardé autour des joueurs")
	code("suivi.zoom_margin = 100.0")
	await _deux_vues(["zoom_margin = 30", "zoom_margin = 200"], [{"zoom_margin": 30.0}, {"zoom_margin": 200.0}])


# 📌 Exemples

func _ex_familier() -> void:
	sujet("Exemple : un familier qui suit avec du retard")
	var joueur := carre(Vector2(300, 350), VARIANTES[0], 40)
	joueur.name = "Joueur"
	var point := Marker2D.new()
	point.name = "PointFamilier"
	point.position = Vector2(-40, -30)
	joueur.add_child(point)
	var familier := disque(Vector2(260, 320), ACCENT, 14)
	familier.name = "Familier"
	_suivi(familier, ["../../Joueur/PointFamilier"], {"follow_speed": 3.0}).play()
	code("suivi.targets = [NodePath(\"../../Joueur/PointFamilier\")]\nsuivi.follow_speed = 3.0")
	var t := 0.0
	while t < 9.0:
		await images()
		t += get_process_delta_time()
		joueur.position = Vector2(576 + sin(t * 0.9) * 380, 380 - absf(sin(t * 3.0)) * 90)


func _ex_ajout_joueur() -> void:
	sujet("Exemple : un joueur rejoint la partie")
	var vue := vue_2d(Rect2(76, 110, 1000, 420))
	_decor(vue)
	var a := _joueur(vue, "Joueur1", VARIANTES[0])
	var b := _joueur(vue, "Joueur2", VARIANTES[2])
	var camera: Camera2D = vue.get_meta("camera")
	var joueur := _suivi(camera, ["../../Joueur1", "../../Joueur2"])
	joueur.play()
	var suivi: FXFollower = joueur.special_effects[0]
	var c: Node2D = null
	var noeuds: Array = [a, b]
	_surimpression(vue, noeuds, 100.0)
	code("var liste := suivi.targets\nliste.append($Camera2D/PlayerFX.get_path_to(joueur))\nsuivi.targets = liste")
	var t := 0.0
	while t < 10.0:
		await images()
		t += get_process_delta_time()
		a.position = Vector2(-200 + sin(t) * 60, 100)
		b.position = Vector2(150 + cos(t * 1.2) * 60, 100)
		if t > 3.0 and c == null:
			c = _joueur(vue, "Joueur3", VARIANTES[3])
			var liste := suivi.targets
			liste.append(joueur.get_path_to(c))
			suivi.targets = liste
			noeuds.append(c)
			legende("ajouter_joueur(Joueur3)")
		if c:
			c.position = Vector2(200 + (t - 3.0) * 120, 60)


func _ex_3d() -> void:
	sujet("Exemple : suivi et visée en 3D")
	var vue := vue_3d(Rect2(76, 140, 1000, 380))
	cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(80, 0.2, 80))
	for i in 20:
		cube(vue, Vector3((i % 5) * 6 - 12, 0.5, (i / 5) * 6 - 12), Formes.DECOR, Vector3(0.8, 1, 0.8))
	var joueurs: Array[Node3D] = []
	for i in 2:
		var j := Formes.sphere(vue, Vector3.ZERO, VARIANTES[i * 2], 0.5)
		j.name = "J%d" % i
		joueurs.append(j)
	var pivot := Node3D.new()
	pivot.name = "Pivot"
	vue.add_child(pivot)
	var camera := vue.get_camera_3d()
	camera.reparent(pivot)
	camera.position = Vector3(0, 6, 10)
	_suivi(pivot, ["../../J0", "../../J1"], {"follow_speed": 3.0}).play()
	var visee: FXTargeter = effet("FXTargeter", {"enable_camera_3d_depth_control": false, "add_lerp": true})
	visee.nodes_to_target = [NodePath("../../../J0"), NodePath("../../../J1")]
	joueur_fx(camera, visee).play()
	code("# Pivot ─ PlayerFX (FXFollower : joueurs)\n# └─ Camera3D (0, 6, 10) ─ PlayerFX (FXTargeter)")
	var t := 0.0
	while t < 10.0:
		await images()
		t += get_process_delta_time()
		joueurs[0].position = Vector3(sin(t * 0.5) * 8.0, 0.5, cos(t * 0.4) * 6.0)
		joueurs[1].position = joueurs[0].position + Vector3(cos(t) * 2.5, 0, sin(t) * 2.5)


func _ex_cinematique() -> void:
	sujet("Exemple : figer le suivi pendant une cinématique")
	var vue := vue_2d(Rect2(76, 110, 1000, 420))
	_decor(vue)
	var a := _joueur(vue, "Joueur1", VARIANTES[0])
	var b := _joueur(vue, "Joueur2", VARIANTES[2])
	var boss := Formes.carre(vue, Vector2(1300, 60), MAUVAIS, 110)
	var camera: Camera2D = vue.get_meta("camera")
	var joueur := _suivi(camera, ["../../Joueur1", "../../Joueur2"])
	joueur.play()
	code("$Camera2D/PlayerFX.stop()\n$AnimationPlayer.play(\"intro_boss\")\nawait $AnimationPlayer.animation_finished\n$Camera2D/PlayerFX.play()")
	a.position = Vector2(-120, 100)
	b.position = Vector2(120, 100)
	legende("Jeu : la caméra suit les joueurs")
	await secondes(1.5)
	legende("debut_cinematique() : stop(), l'animation emmène la caméra vers le boss")
	joueur.stop()
	var animation := camera.create_tween()
	animation.tween_property(camera, "position", Vector2(1300, 40), 1.5).set_trans(Tween.TRANS_SINE)
	animation.tween_interval(1.0)
	await animation.finished
	legende("Fin : play(), retour en douceur vers les joueurs")
	joueur.play()
	await secondes(3.0)
	boss.name = "Boss"
