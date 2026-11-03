extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## 🌻 Plans filmés de la page Ciblage d'objet (effets/ciblage-d-objet.html) : FXTargeter.


func titre_module() -> String:
	return "🌻 Ciblage d'objet · FXTargeter"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-FXTargeter": _classe,
		"FXTargeter.add_lerp": _add_lerp,
		"FXTargeter.lerp_speed": _lerp_speed,
		"FXTargeter.nodes_to_target": _nodes_to_target,
		"FXTargeter.enable_camera_3d_depth_control": _depth_control,
		"FXTargeter.depth_margin": _depth_margin,
		"ex-camera-groupe": _ex_camera_groupe,
		"ex-yeux": _ex_yeux,
		"ex-projecteur": _ex_projecteur,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "La tourelle vise le joueur pendant qu'il est dans la zone de détection (<code>play()</code>), puis garde son orientation (<code>stop()</code>).",
		"classe-FXTargeter": "En 2D, l'axe +X de la cible pointe vers les nœuds visés ; en 3D, l'axe -Z d'une caméra, qui recule aussi pour les cadrer.",
		"FXTargeter.add_lerp": "Sans interpolation la tourelle est toujours alignée ; avec, elle a un temps de réaction.",
		"FXTargeter.lerp_speed": "<code>lerp_speed</code> 1, 3 et 10 (avec <code>add_lerp</code>) : vitesse à laquelle l'écart est rattrapé.",
		"FXTargeter.nodes_to_target": "Deux nœuds visés : la tourelle vise la moyenne de leurs positions (croix).",
		"FXTargeter.enable_camera_3d_depth_control": "Le groupe s'écarte : avec le contrôle de distance la caméra recule pour le garder dans le champ, sans elle elle ne fait que tourner.",
		"FXTargeter.depth_margin": "<code>depth_margin</code> 1, 1,5 et 2,5 : la marge de cadrage autour du groupe.",
		"ex-camera-groupe": "Trois combattants qui se dispersent puis se regroupent : la caméra tourne vers leur centre et avance ou recule.",
		"ex-yeux": "Deux yeux visent un Marker2D placé sous la souris.",
		"ex-projecteur": "Une SpotLight3D suit le joueur : son axe -Z est celui orienté par FXTargeter.",
	}


func reutilisations() -> Dictionary:
	return {
		"FXTargeter.min_distance": ["FXTargeter.enable_camera_3d_depth_control", "La distance de la caméra au centre du groupe reste entre <code>min_distance</code> et <code>max_distance</code>."],
		"FXTargeter.max_distance": ["FXTargeter.enable_camera_3d_depth_control", "La distance de la caméra au centre du groupe reste entre <code>min_distance</code> et <code>max_distance</code>."],
	}


## Tourelle 2D (canon le long de +X) centrée sur `position`.
func _tourelle(parent: Node, position: Vector2, couleur: Color, nom: String = "") -> Node2D:
	var tourelle := Node2D.new()
	tourelle.position = position
	if nom:
		tourelle.name = nom
	var canon := Polygon2D.new()
	canon.polygon = PackedVector2Array([Vector2(0, -8), Vector2(60, -8), Vector2(60, 8), Vector2(0, 8)])
	canon.color = couleur.darkened(0.2)
	tourelle.add_child(canon)
	tourelle.draw.connect(func(): tourelle.draw_circle(Vector2.ZERO, 26, couleur, true, -1.0, true))
	parent.add_child(tourelle)
	return tourelle


## Ciblage de `cible` vers les chemins `chemins` (relatifs au PlayerFX).
func _viser(cible: Node, chemins: Array, reglages: Dictionary = {}) -> PlayerFX:
	var ciblage: FXTargeter = effet("FXTargeter", reglages)
	var liste: Array[NodePath] = []
	for chemin in chemins:
		liste.append(NodePath(chemin))
	ciblage.nodes_to_target = liste
	return joueur_fx(cible, ciblage)


## Personnage 2D (rectangle bleu) nommé `nom` dans `parent`.
func _joueur(parent: Node, position: Vector2, nom: String = "Joueur", couleur: Color = VARIANTES[0]) -> Node2D:
	var joueur := Formes.carre(parent, position, couleur, 34)
	joueur.name = nom
	return joueur


## Déplace `noeud` en boucle sur un cercle (centre, rayon, période) pendant `duree` secondes.
func _ronde(noeuds: Array, centre: Vector2, rayon: Vector2, periode: float, duree: float) -> void:
	var t := 0.0
	while t < duree:
		await images()
		t += get_process_delta_time()
		for i in noeuds.size():
			var angle := TAU * t / periode + i * TAU / noeuds.size()
			noeuds[i].position = centre + Vector2(cos(angle) * rayon.x, sin(angle) * rayon.y)


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : une tourelle 2D qui vise le joueur")
	arbre(Vector2(40, 140), PackedStringArray(["Niveau", "  Tourelle (Sprite2D, canon vers +X)", "    PlayerFX  (FXTargeter → ../../Joueur)", "  Joueur"]))
	var tourelle := _tourelle(scene, Vector2(576, 340), ACCENT, "Tourelle")
	var zone := Node2D.new()
	zone.position = tourelle.position
	zone.draw.connect(func(): zone.draw_arc(Vector2.ZERO, 220, 0, TAU, 64, Color(1, 1, 1, 0.25), 2.0))
	scene.add_child(zone)
	etiquette(Vector2(250, 540), "Zone de détection (rayon 220)", 14)
	var joueur := _joueur(scene, Vector2(150, 260))
	var joueur_fx_noeud := _viser(tourelle, ["../../Joueur"], {"add_lerp": true, "lerp_speed": 3.0})
	code("func _on_detection_body_entered(_corps):\n\t$PlayerFX.play()\nfunc _on_detection_body_exited(_corps):\n\t$PlayerFX.stop()")
	var dedans := false
	var t := 0.0
	while t < 11.0:
		await images()
		t += get_process_delta_time()
		joueur.position = Vector2(576, 340) + Vector2(cos(t * 0.75) * 400, sin(t * 1.1) * 180)
		var proche := joueur.position.distance_to(tourelle.position) < 220
		if proche != dedans:
			dedans = proche
			if dedans:
				joueur_fx_noeud.play()
				legende("Joueur détecté : play(), la tourelle le suit (add_lerp, lerp_speed 3)")
			else:
				joueur_fx_noeud.stop()
				legende("Joueur hors de portée : stop(), la tourelle garde son orientation")


# 💻 API

func _classe() -> void:
	sujet("FXTargeter : orienter vers un ou plusieurs nœuds")
	var gauche := Node2D.new()
	scene.add_child(gauche)
	var tourelle := _tourelle(gauche, Vector2(300, 330), ACCENT)
	var joueur := _joueur(gauche, Vector2(300, 200))
	joueur.name = "Cible"
	tourelle.name = "Tourelle"
	_viser(tourelle, ["../../Cible"]).play()
	etiquette(Vector2(300, 520), "2D : l'axe +X vise la cible", 16)

	var vue := vue_3d(Rect2(560, 150, 520, 330))
	cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(30, 0.2, 30))
	var combattants: Array[Node3D] = []
	for i in 3:
		var c := Formes.sphere(vue, Vector3(i * 2 - 2, 0.5, -2), VARIANTES[i], 0.5)
		c.name = "Combattant%d" % i
		combattants.append(c)
	var camera := vue.get_camera_3d()
	_viser(camera, ["../../Combattant0", "../../Combattant1", "../../Combattant2"], {"add_lerp": true, "lerp_speed": 2.0, "min_distance": 4.0}).play()
	etiquette(Vector2(820, 520), "3D : la caméra tourne et recule pour cadrer", 16)
	code("ciblage.nodes_to_target = [NodePath(\"../../Cible\")]\n$PlayerFX.play()")
	var t := 0.0
	while t < 8.0:
		await images()
		t += get_process_delta_time()
		joueur.position = Vector2(300, 330) + Vector2(cos(t) * 170, sin(t * 1.3) * 130)
		var ecart := 1.5 + sin(t * 0.8) * 1.3
		for i in 3:
			combattants[i].position = Vector3((i - 1) * ecart * 2.0 + sin(t) * 1.5, 0.5, -2 + cos(t * 0.7 + i) * ecart)


func _deux_tourelles(textes: Array, reglages: Array, duree: float = 9.0) -> void:
	var n := reglages.size()
	var tourelles: Array[Node2D] = []
	var cible := _joueur(scene, Vector2(576, 180), "Cible")
	for i in n:
		var x := 1152.0 / (n + 1) * (i + 1)
		var tourelle := _tourelle(scene, Vector2(x, 420), VARIANTES[i])
		tourelle.name = "T%d" % i
		etiquette(Vector2(x, 480), textes[i], 17, VARIANTES[i])
		_viser(tourelle, ["../../Cible"], reglages[i]).play()
		tourelles.append(tourelle)
	var t := 0.0
	while t < duree:
		await images()
		t += get_process_delta_time()
		# Sauts brusques de la cible : on voit le temps de réaction
		var phase := int(t / 1.5) % 4
		var vise: Vector2 = [Vector2(200, 180), Vector2(950, 160), Vector2(576, 260), Vector2(300, 300)][phase]
		cible.position = cible.position.lerp(vise, minf(1.0, get_process_delta_time() * 12.0))


func _add_lerp() -> void:
	sujet("add_lerp : rotation progressive")
	code("ciblage.add_lerp = true")
	await _deux_tourelles(["add_lerp = false (défaut)", "add_lerp = true"], [{}, {"add_lerp": true, "lerp_speed": 3.0}])


func _lerp_speed() -> void:
	sujet("lerp_speed : rapidité de l'interpolation")
	code("ciblage.add_lerp = true\nciblage.lerp_speed = 3.0")
	await _deux_tourelles(["lerp_speed = 1", "lerp_speed = 3", "lerp_speed = 10"],
		[{"add_lerp": true, "lerp_speed": 1.0}, {"add_lerp": true, "lerp_speed": 3.0}, {"add_lerp": true, "lerp_speed": 10.0}])


func _nodes_to_target() -> void:
	sujet("nodes_to_target : vers la moyenne de plusieurs nœuds")
	var a := _joueur(scene, Vector2(300, 200), "A", VARIANTES[0])
	var b := _joueur(scene, Vector2(850, 220), "B", VARIANTES[2])
	var tourelle := _tourelle(scene, Vector2(576, 470), ACCENT, "Tourelle")
	_viser(tourelle, ["../../A", "../../B"]).play()
	var croix := Node2D.new()
	croix.draw.connect(func():
		croix.draw_line(Vector2(-12, -12), Vector2(12, 12), Color.WHITE, 2.0)
		croix.draw_line(Vector2(-12, 12), Vector2(12, -12), Color.WHITE, 2.0))
	scene.add_child(croix)
	code("ciblage.nodes_to_target = [NodePath(\"../../A\"), NodePath(\"../../B\")]")
	legende("La croix : moyenne des positions de A et B")
	var t := 0.0
	while t < 8.0:
		await images()
		t += get_process_delta_time()
		a.position = Vector2(300 + sin(t * 0.9) * 200, 270 + cos(t * 1.4) * 80)
		b.position = Vector2(850 + cos(t * 0.7) * 200, 270 + sin(t * 1.1) * 90)
		croix.position = (a.position + b.position) / 2.0


## Deux vues 3D d'un même groupe qui s'écarte, caméras réglées par `reglages[i]`.
func _vues_groupe(textes: Array, reglages: Array, duree: float = 9.0) -> void:
	var n := reglages.size()
	var largeur := (1040.0 - (n - 1) * 16.0) / n
	var groupes: Array = []
	for i in n:
		var cadre := Rect2(56 + i * (largeur + 16), 140, largeur, largeur * 0.62)
		var vue := vue_3d(cadre)
		cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(60, 0.2, 60))
		var membres: Array[Node3D] = []
		for j in 3:
			var m := Formes.sphere(vue, Vector3.ZERO, VARIANTES[j], 0.5)
			m.name = "M%d" % j
			membres.append(m)
		groupes.append(membres)
		var camera := vue.get_camera_3d()
		camera.position = Vector3(0, 4, 9)
		_viser(camera, ["../../M0", "../../M1", "../../M2"], reglages[i]).play()
		etiquette(Vector2(cadre.get_center().x, cadre.end.y + 24), textes[i], 17, VARIANTES[i])
	var t := 0.0
	while t < duree:
		await images()
		t += get_process_delta_time()
		var ecart := 1.0 + (1.0 - cos(t * 0.8)) * 3.0
		for membres in groupes:
			for j in 3:
				membres[j].position = Vector3((j - 1) * ecart, 0.5, sin(j * 2.0) * ecart * 0.5)


func _depth_control() -> void:
	sujet("enable_camera_3d_depth_control, min_distance, max_distance")
	code("ciblage.enable_camera_3d_depth_control = true\nciblage.min_distance = 5.0\nciblage.max_distance = 50.0")
	legende("Le groupe s'écarte puis se resserre")
	await _vues_groupe(["contrôle de distance : true (défaut)", "false : rotation seule"],
		[{"add_lerp": true, "lerp_speed": 3.0}, {"add_lerp": true, "lerp_speed": 3.0, "enable_camera_3d_depth_control": false}])


func _depth_margin() -> void:
	sujet("depth_margin : marge de cadrage")
	code("ciblage.depth_margin = 1.5")
	await _vues_groupe(["depth_margin = 1", "1,5 (défaut)", "2,5"], [{"add_lerp": true, "depth_margin": 1.0, "min_distance": 1.0},
		{"add_lerp": true, "depth_margin": 1.5, "min_distance": 1.0}, {"add_lerp": true, "depth_margin": 2.5, "min_distance": 1.0}])


# 📌 Exemples

func _ex_camera_groupe() -> void:
	sujet("Exemple : cadrer plusieurs combattants (caméra 3D)")
	var vue := vue_3d(Rect2(76, 150, 1000, 380))
	cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(80, 0.2, 80))
	var noms := ["Joueur1", "Joueur2", "Boss"]
	var combattants: Array[Node3D] = []
	for i in 3:
		var c := Formes.sphere(vue, Vector3.ZERO, VARIANTES[i], 0.6 if i < 2 else 1.0)
		c.name = noms[i]
		combattants.append(c)
	var camera := vue.get_camera_3d()
	camera.position = Vector3(0, 5, 12)
	_viser(camera, ["../../Joueur1", "../../Joueur2", "../../Boss"], {"add_lerp": true, "lerp_speed": 2.0, "min_distance": 8.0, "max_distance": 30.0, "depth_margin": 1.8}).play()
	code("cadrage.nodes_to_target = [Joueur1, Joueur2, Boss]\ncadrage.add_lerp = true · lerp_speed = 2\ncadrage.min_distance = 8 · max_distance = 30\ncadrage.depth_margin = 1.8")
	var t := 0.0
	while t < 10.0:
		await images()
		t += get_process_delta_time()
		var ecart := 2.0 + (1.0 - cos(t * 0.7)) * 5.0
		combattants[0].position = Vector3(-ecart, 0.6, sin(t) * 2.0)
		combattants[1].position = Vector3(ecart * 0.6, 0.6, cos(t * 1.3) * ecart * 0.5)
		combattants[2].position = Vector3(sin(t * 0.5) * 3.0, 1.0, -ecart * 0.8)


func _ex_yeux() -> void:
	sujet("Exemple : des yeux qui suivent la souris")
	var visage := Node2D.new()
	visage.position = Vector2(576, 330)
	visage.draw.connect(func(): visage.draw_circle(Vector2.ZERO, 150, Color("ffd166")))
	scene.add_child(visage)
	var cible := Marker2D.new()
	cible.name = "CibleSouris"
	visage.add_child(cible)
	for i in 2:
		var oeil := Node2D.new()
		oeil.name = ["OeilGauche", "OeilDroit"][i]
		oeil.position = Vector2(-55 + i * 110, -30)
		oeil.draw.connect(func():
			oeil.draw_circle(Vector2.ZERO, 32, Color.WHITE)
			oeil.draw_circle(Vector2(16, 0), 13, Color("1b1f2a")))
		visage.add_child(oeil)
		_viser(oeil, ["../../CibleSouris"]).play()
	var fleche := pointeur()
	code("func _process(_delta):\n\t$CibleSouris.global_position = get_global_mouse_position()")
	for point in [Vector2(200, 160), Vector2(950, 140), Vector2(900, 560), Vector2(250, 540), Vector2(576, 330), Vector2(576, 120)]:
		var depart := fleche.position
		var t := 0.0
		while t < 0.9:
			await images()
			t += get_process_delta_time()
			fleche.position = depart.lerp(point, smoothstep(0.0, 1.0, t / 0.9))
			cible.global_position = fleche.position


func _ex_projecteur() -> void:
	sujet("Exemple : un projecteur 3D qui suit le joueur")
	var vue := vue_3d(Rect2(76, 150, 1000, 380))
	cube(vue, Vector3(0, -0.1, 0), Color("cfd6e6"), Vector3(30, 0.2, 30))
	for enfant in vue.get_children():
		if enfant is DirectionalLight3D:
			enfant.light_energy = 0.08
		if enfant is WorldEnvironment:
			enfant.environment.ambient_light_color = Color(0.15, 0.15, 0.2)
	var joueur := Formes.sphere(vue, Vector3.ZERO, VARIANTES[0], 0.5)
	joueur.name = "Joueur"
	var projecteur := SpotLight3D.new()
	projecteur.name = "Projecteur"
	projecteur.position = Vector3(0, 6, -3)
	projecteur.spot_range = 20.0
	projecteur.spot_angle = 14.0
	projecteur.light_energy = 6.0
	vue.add_child(projecteur)
	Formes.sphere(vue, projecteur.position, Color(1, 1, 0.8), 0.2)
	_viser(projecteur, ["../../Joueur"], {"add_lerp": true}).play()
	code("suivi_lumiere.nodes_to_target = [NodePath(\"../../Joueur\")]\nsuivi_lumiere.add_lerp = true\n$Projecteur/PlayerFX.play()")
	var t := 0.0
	while t < 9.0:
		await images()
		t += get_process_delta_time()
		joueur.position = Vector3(sin(t * 0.8) * 4.0, 0.5, cos(t * 1.2) * 2.0 + 0.5)
