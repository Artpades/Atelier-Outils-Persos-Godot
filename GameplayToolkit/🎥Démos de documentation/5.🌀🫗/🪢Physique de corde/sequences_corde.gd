extends "res://🎥Démos de documentation/5.🌀🫗/sequences_mouvement.gd"

## 🪢 Plans filmés de la page Physique de corde (mouvement/physique-de-corde.html).


func titre_module() -> String:
	return "🪢 Corde · Rope2D / 3D, RopeMovement2D / 3D"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-Rope2D": _classe_rope_2d,
		"classe-Rope3D": _classe_rope_3d,
		"classe-RopeMovement2D": _classe_mouvement_2d,
		"classe-RopeMovement3D": _classe_mouvement_3d,
		"Rope2D.rope_length": _rope_length,
		"Rope2D.point_count": _point_count,
		"Rope2D.damping": _damping,
		"Rope2D.gravity": _gravity,
		"Rope2D.iterations": _iterations,
		"Rope2D.attached_weight": _attached_weight,
		"Rope2D.collide": _collide,
		"Rope2D.width": _width,
		"Rope2D.debug_mode": _rope_debug,
		"RopeMovement2D.swing_force": _swing_force,
		"RopeMovement2D.jump_up_velocity": _jump_up,
		"RopeMovement2D.climb_speed": _climb_speed,
		"ex-auto": _ex_auto,
		"ex-pont": _ex_pont,
		"ex-lanterne": _ex_lanterne,
		"ex-proche": _ex_proche,
		"ex-visuel3d": _ex_visuel3d,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Saut, <code>try_grab()</code> sur la liane, montée, balancement gauche / droite, puis saut vers la plateforme (<code>debug_mode</code> de la corde).",
		"classe-Rope2D": "Cordes simulées (Verlet) : une liane qui pend, un câble tendu entre deux ancrages, une corde secouée.",
		"classe-Rope3D": "La même simulation en 3D : une corde pendue qu'on balance, un câble entre deux poteaux.",
		"classe-RopeMovement2D": "Accroché à la corde : haut / bas pour grimper, gauche / droite pour se balancer, saut pour lâcher avec l'élan.",
		"classe-RopeMovement3D": "En 3D : accroché, le personnage grimpe et se balance dans le plan de la commande.",
		"Rope2D.rope_length": "<code>rope_length</code> 100, 200 (défaut) et 350 px.",
		"Rope2D.point_count": "<code>point_count</code> 4, 16 (défaut) et 48 : la souplesse de la corde.",
		"Rope2D.damping": "Même impulsion : <code>damping</code> 0, 0,01 (défaut) et 0,1.",
		"Rope2D.gravity": "<code>gravity</code> (0, 980) (défaut), (500, 600) (vent) et (0, -400) (flotte vers le haut).",
		"Rope2D.iterations": "Une lourde charge au bout : <code>iterations</code> 1, 12 (défaut) et 50 ; peu de passes = la corde s'étire.",
		"Rope2D.attached_weight": "Une lanterne accrochée : <code>attached_weight</code> 0, 1,5 (défaut) et 6.",
		"Rope2D.collide": "<code>collide</code> false : la corde traverse le rocher · true : elle se pose dessus.",
		"Rope2D.width": "<code>width</code> et <code>color</code> : l'apparence du trait.",
		"Rope2D.debug_mode": "<code>debug_mode</code> : points, origine, longueur et nombre de points.",
		"RopeMovement2D.swing_force": "<code>swing_force</code> 500, 2000 (défaut) et 5000 : l'amplitude du balancement.",
		"RopeMovement2D.jump_up_velocity": "Lâcher au même moment : <code>jump_up_velocity</code> 0, 320 (défaut) et 600.",
		"RopeMovement2D.climb_speed": "<code>climb_speed</code> 50, 110 (défaut) et 250 px/s.",
		"ex-auto": "En l'air, <code>try_grab()</code> à chaque image : le personnage attrape la corde en sautant dessus (sauf pendant <code>regrab_delay</code>).",
		"ex-pont": "Un câble (<code>end_anchor_path</code>) de 420 px entre deux poteaux, <code>collide</code> : il pend et se pose sur le décor.",
		"ex-lanterne": "<code>attach_body</code> : une lanterne au bout de la corde ; <code>apply_impulse_at</code> : un coup de vent la balance.",
		"ex-proche": "<code>Rope2D.find_nearest_info()</code> : l'aide « Attraper » s'affiche quand une corde est à portée.",
		"ex-visuel3d": "Les points d'une Rope3D (<code>get_points()</code>) donnés à une Path3D + CSGPolygon3D : une corde épaisse.",
	}


func schemas() -> Dictionary:
	return {
		"index": ["index.svg", "Les positions le long d'une corde sont des index fractionnaires : 0 = premier point, <code>point_count - 1</code> = dernier, 2,5 = à mi-chemin entre les points 2 et 3."],
	}


func reutilisations() -> Dictionary:
	var meme_3d := "Même réglage qu'en 2D, en mètres (voir la vidéo 2D)."
	var r := {
		"Rope2D.color": "Rope2D.width",
		"Rope2D.collision_mask": "Rope2D.collide",
		"Rope2D.start_anchor_path": "ex-pont",
		"Rope2D.end_anchor_path": "ex-pont",
		"Rope2D.apply_impulse_at": "ex-lanterne",
		"Rope2D.attach_body": "ex-lanterne",
		"Rope2D.body_attached": "mode-emploi",
		"Rope2D.body_detached": "mode-emploi",
		"Rope2D.find_nearest_info": "ex-proche",
		"Rope2D.get_points": "ex-visuel3d",
		"RopeMovement2D.try_grab": "mode-emploi",
		"RopeMovement2D.release": "RopeMovement2D.jump_up_velocity",
		"RopeMovement2D.is_attached": "ex-proche",
		"RopeMovement2D.get_rope": "classe-RopeMovement2D",
		"RopeMovement2D.grab_distance": "ex-auto",
		"RopeMovement2D.regrab_delay": "ex-auto",
		"RopeMovement2D.grabbed": "mode-emploi",
		"RopeMovement2D.released": "mode-emploi",
	}
	for methode in ["detach_body", "find_rope_of", "get_attached_position", "get_closest_position", "get_position_at", "get_segment_length", "get_velocity_at",
			"has_body", "reset_points", "set_attached_position"]:
		r["Rope2D." + methode] = "index"
		r["Rope3D." + methode] = "index"
	for membre in ["attached_weight", "collide", "collision_mask", "color", "damping", "debug_mode", "end_anchor_path", "gravity", "iterations", "point_count",
			"rope_length", "start_anchor_path", "apply_impulse_at", "attach_body", "body_attached", "body_detached", "get_points"]:
		r["Rope3D." + membre] = [r.get("Rope2D." + membre, "Rope2D." + membre), meme_3d]
	for membre in ["climb_speed", "grab_distance", "jump_up_velocity", "regrab_delay", "swing_force", "try_grab", "release", "is_attached", "get_rope", "grabbed", "released"]:
		r["RopeMovement3D." + membre] = [r.get("RopeMovement2D." + membre, "RopeMovement2D." + membre), meme_3d]
	return r


func corde(parent: Node, position: Vector2, reglages: Dictionary = {}) -> Rope2D:
	var c := Rope2D.new()
	c.position = position
	for propriete in reglages:
		c.set(propriete, reglages[propriete])
	parent.add_child(c)
	var attache := Polygon2D.new()
	attache.polygon = Formes.rectangle(Vector2(18, 10))
	attache.color = Formes.DECOR
	c.add_child(attache)
	return c


func _acrobate(vue: Node, position: Vector2, reglages_corde: Dictionary = {}, couleur: Color = VARIANTES[0]) -> CharacterBody2D:
	return heros_2d(vue, position, {"Sol": "GroundMovement2D", "Corde": "RopeMovement2D", "Directeur": "MovementDirector"}, {"Corde": reglages_corde}, couleur)


## Saute vers la corde et l'attrape dès qu'elle est à portée (try_grab à chaque image en l'air).
func _attraper(heros: CharacterBody2D, direction: float = 1.0, duree: float = 1.2) -> void:
	var d: MovementDirector = heros.get_node("Directeur")
	var mouvement: RopeMovement2D = heros.get_node("Corde")
	d.move(Vector2(direction, 0))
	d.jump()
	var t := 0.0
	while t < duree and not mouvement.is_attached():
		await get_tree().physics_frame
		t += get_physics_process_delta_time()
		if t > 0.1:
			mouvement.try_grab()
	d.release_jump()
	d.move(Vector2.ZERO)


## Balance la corde : gauche / droite alternés, demi-période `demi`.
func _balancer(d: MovementDirector, fois: int, demi: float = 0.55) -> void:
	for i in fois:
		await piloter(d, [[demi, Vector2.RIGHT if i % 2 == 0 else Vector2.LEFT]])


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : attraper une liane, grimper, se balancer")
	var vue := monde_2d()
	arene(vue, 1600.0)
	bloc(vue, Rect2(-800, -230, 1600, 30))
	bloc(vue, Rect2(300, 40, 300, 24))
	var liane := corde(vue, Vector2(0, -200), {"rope_length": 300.0, "debug_mode": true})
	var heros := _acrobate(vue, Vector2(-300, 150))
	debug(vue)
	var journal := etiquette(Vector2(576, 140), "", 18, ACCENT)
	liane.body_attached.connect(func(_c): journal.text = "body_attached")
	liane.body_detached.connect(func(_c): journal.text = "body_detached")
	code("if Input.is_action_just_pressed(\"attraper\"): $RopeMovement2D.try_grab()\nif Input.is_action_just_pressed(\"ui_accept\"): $MovementDirector.jump()   # lâche")
	var d: MovementDirector = heros.get_node("Directeur")
	await piloter(d, [[0.9, Vector2.RIGHT]])
	legende("try_grab() en sautant vers la liane")
	await _attraper(heros)
	legende("Haut : grimper")
	await piloter(d, [[1.0, Vector2.UP]])
	legende("Gauche / droite : se balancer")
	await _balancer(d, 5)
	legende("Saut : lâcher avec l'élan, vers la plateforme")
	await piloter(d, [[0.2, Vector2.RIGHT, "saut_long"], [1.5, Vector2.RIGHT], [0.4, Vector2.ZERO]])


# 💻 API

func _classe_rope_2d() -> void:
	sujet("Rope2D : une corde simulée")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(-800, -230, 1600, 30))
	var liane := corde(vue, Vector2(-350, -200), {"rope_length": 250.0})
	var poteau_g := bloc(vue, Rect2(-60, 0, 20, 200), Formes.DECOR)
	var poteau_d := bloc(vue, Rect2(220, 0, 20, 200), Formes.DECOR)
	var cable := corde(vue, Vector2(-50, 0), {"rope_length": 330.0, "point_count": 24})
	cable.end_anchor_path = cable.get_path_to(poteau_d)
	var secouee := corde(vue, Vector2(400, -200), {"rope_length": 200.0})
	code("var corde := Rope2D.new()\ncorde.rope_length = 200.0\ncorde.end_anchor_path = NodePath(\"../PoteauDroit\")")
	for i in 4:
		await secondes(1.2)
		secouee.apply_impulse_at(secouee.point_count - 1, Vector2(500 if i % 2 == 0 else -500, 0))
		liane.apply_impulse_at(liane.point_count - 1, Vector2(250, 0))
	poteau_g.name = "PoteauGauche"
	await secondes(1.0)


func _corde_3d(parent: Node, position: Vector3, reglages: Dictionary = {}) -> Rope3D:
	var c := Rope3D.new()
	c.position = position
	for propriete in reglages:
		c.set(propriete, reglages[propriete])
	parent.add_child(c)
	return c


func _classe_rope_3d() -> void:
	sujet("Rope3D : une corde en 3D")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(20, 1, 10))
	bloc_3d(vue, Vector3(-3, 3, 0), Vector3(0.3, 7, 0.3), Formes.DECOR)
	var pendue := _corde_3d(vue, Vector3(-3, 6.5, 0), {"rope_length": 4.0, "debug_mode": true})
	var gauche := bloc_3d(vue, Vector3(1, 1.5, 0), Vector3(0.3, 3, 0.3), Formes.DECOR)
	var droite := bloc_3d(vue, Vector3(5, 1.5, 0), Vector3(0.3, 3, 0.3), Formes.DECOR)
	var cable := _corde_3d(vue, Vector3(1, 3, 0), {"rope_length": 4.6, "point_count": 20})
	cable.end_anchor_path = cable.get_path_to(droite)
	code("var corde := Rope3D.new()\ncorde.rope_length = 4.0   # mètres")
	for i in 4:
		await secondes(1.2)
		pendue.apply_impulse_at(pendue.point_count - 1, Vector3(4 if i % 2 == 0 else -2, 0, 3))
		cable.apply_impulse_at(10, Vector3(0, 3, 2))
	gauche.name = "PoteauGauche"
	await secondes(1.0)


func _classe_mouvement_2d() -> void:
	sujet("RopeMovement2D : grimper, se balancer, sauter")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(-800, -230, 1600, 30))
	corde(vue, Vector2(-100, -200), {"rope_length": 320.0})
	var heros := _acrobate(vue, Vector2(-350, 150))
	var mouvement: RopeMovement2D = heros.get_node("Corde")
	debug(vue)
	var etat := etiquette(Vector2(576, 140), "", 18, ACCENT)
	get_tree().process_frame.connect(func():
		if is_instance_valid(etat):
			etat.text = "is_attached() = %s · get_rope() = %s" % [mouvement.is_attached(), mouvement.get_rope().name if mouvement.get_rope() else "null"])
	code("$MovementDirector.move(Vector2(x, y))   # y : grimper, x : balancer\n$MovementDirector.jump()   # lâcher")
	var d: MovementDirector = heros.get_node("Directeur")
	await piloter(d, [[0.8, Vector2.RIGHT]])
	await _attraper(heros)
	await piloter(d, [[0.8, Vector2.UP], [0.6, Vector2.DOWN]])
	await _balancer(d, 4)
	await piloter(d, [[0.2, Vector2.RIGHT, "saut_long"], [1.4, Vector2.RIGHT]])


func _classe_mouvement_3d() -> void:
	sujet("RopeMovement3D : la corde en 3D")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(20, 1, 10))
	bloc_3d(vue, Vector3(0, 7.2, 0), Vector3(3, 0.3, 3), Formes.DECOR)
	var c := _corde_3d(vue, Vector3(0, 7, 0), {"rope_length": 5.0})
	var heros := heros_3d(vue, Vector3(-1.2, 1, 0), {"Sol": "GroundMovement3D", "Corde": "RopeMovement3D", "Directeur": "MovementDirector"}, {"Corde": {"grab_distance": 1.5}})
	debug(vue)
	code("$RopeMovement3D.try_grab()\n$MovementDirector.move(Vector2(x, z))   # balancer\n$MovementDirector.set_vertical_input(1)   # grimper")
	var d: MovementDirector = heros.get_node("Directeur")
	var mouvement: RopeMovement3D = heros.get_node("Corde")
	d.move(Vector2(1, 0))
	d.jump()
	var t := 0.0
	while t < 1.2 and not mouvement.is_attached():
		await get_tree().physics_frame
		t += get_physics_process_delta_time()
		mouvement.try_grab()
	d.release_jump()
	d.set_vertical_input(1.0)
	await piloter(d, [[0.8, Vector2.ZERO]])
	d.set_vertical_input(0.0)
	for i in 4:
		await piloter(d, [[0.6, Vector2(1, 0) if i % 2 == 0 else Vector2(-1, 0)]])
	await piloter(d, [[0.2, Vector2(1, 0), "saut_long"], [1.2, Vector2.ZERO]])
	c.name = "Corde"


## Colonnes de cordes pendues (une par valeur de `propriete`), secouées ensemble.
func _cordes_colonnes(propriete: String, valeurs: Array, textes: Array, communs: Dictionary = {}, impulsion: Vector2 = Vector2(450, 0)) -> Array:
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(-800, -230, 1600, 30))
	var cordes := []
	var n := valeurs.size()
	for i in n:
		var x := -500.0 + (i + 0.5) * 1000.0 / n
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		cordes.append(corde(vue, Vector2(x, -200), reglages))
		etiquette(Vector2(576 + x, 545), textes[i], 16, VARIANTES[i % 4])
	return cordes


func _secouer(cordes: Array, fois: int = 3, impulsion: Vector2 = Vector2(450, 0), pause: float = 1.6) -> void:
	for i in fois:
		await secondes(0.5)
		for c in cordes:
			c.apply_impulse_at(c.point_count - 1, impulsion if i % 2 == 0 else -impulsion)
		await secondes(pause)


func _rope_length() -> void:
	sujet("rope_length : longueur au repos")
	code("$Corde.rope_length = 200.0")
	await _secouer(_cordes_colonnes("rope_length", [100.0, 200.0, 350.0], ["100", "200 (défaut)", "350"]))


func _point_count() -> void:
	sujet("point_count : nombre de points simulés")
	code("$Corde.point_count = 16")
	var cordes := _cordes_colonnes("point_count", [4, 16, 48], ["4", "16 (défaut)", "48"], {"rope_length": 300.0})
	for c in cordes:
		c.debug_mode = true
	await _secouer(cordes)


func _damping() -> void:
	sujet("damping : amortissement")
	code("$Corde.damping = 0.01")
	await _secouer(_cordes_colonnes("damping", [0.0, 0.01, 0.1], ["0", "0,01 (défaut)", "0,1"], {"rope_length": 300.0}), 1, Vector2(600, 0), 5.0)


func _gravity() -> void:
	sujet("gravity : gravité propre à la corde")
	code("$Corde.gravity = Vector2(0, 980)")
	_cordes_colonnes("gravity", [Vector2(0, 980), Vector2(500, 600), Vector2(0, -400)], ["(0, 980) (défaut)", "(500, 600)", "(0, -400)"], {"rope_length": 200.0})
	await secondes(5.0)


func _iterations() -> void:
	sujet("iterations : rigidité (passes de correction)")
	code("$Corde.iterations = 12")
	var cordes := _cordes_colonnes("iterations", [1, 12, 50], ["1", "12 (défaut)", "50"], {"rope_length": 220.0, "attached_weight": 8.0})
	var vue: Node = cordes[0].get_parent()
	for c in cordes:
		var poids := Formes.carre(vue, Vector2.ZERO, ACCENT, 26)
		c.attach_body(poids, c.point_count - 1)
	await _secouer(cordes, 2)


func _attached_weight() -> void:
	sujet("attached_weight : poids d'un corps accroché")
	code("$Corde.attached_weight = 1.5")
	var cordes := _cordes_colonnes("attached_weight", [0.0, 1.5, 6.0], ["0", "1,5 (défaut)", "6"], {"rope_length": 260.0})
	var vue: Node = cordes[0].get_parent()
	for c in cordes:
		var lanterne := Formes.carre(vue, Vector2.ZERO, Color("ffd166"), 24)
		c.attach_body(lanterne, c.point_count * 0.6)
	await _secouer(cordes, 2)


func _collide() -> void:
	sujet("collide et collision_mask : collisions avec le décor")
	code("$Corde.collide = true\n$Corde.collision_mask = 1")
	var cordes := _cordes_colonnes("collide", [false, true], ["collide = false", "collide = true"], {"rope_length": 330.0})
	var vue: Node = cordes[0].get_parent()
	for c in cordes:
		bloc(vue, Rect2(c.position.x - 120, 30, 110, 70), Formes.DECOR)
	await _secouer(cordes, 3, Vector2(-500, 0))


func _width() -> void:
	sujet("width et color : apparence")
	code("$Corde.width = 3.0\n$Corde.color = Color(0.8, 0.65, 0.4)")
	await _secouer(_cordes_colonnes("width", [1.0, 3.0, 10.0], ["width 1", "3 (défaut)", "10, rouge"], {"rope_length": 250.0}).map(func(c):
		if c.width > 5:
			c.color = Color("ef5f5f")
		return c), 2)


func _rope_debug() -> void:
	sujet("debug_mode de la corde")
	code("$Corde.debug_mode = true")
	var cordes := _cordes_colonnes("debug_mode", [false, true], ["debug_mode = false", "true"], {"rope_length": 260.0})
	await _secouer(cordes, 2)


## Colonnes : un acrobate accroché à sa corde par valeur de `propriete` du RopeMovement2D.
func _acrobates(propriete: String, valeurs: Array, textes: Array) -> Array:
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(-800, -230, 1600, 30))
	var heros := []
	var n := valeurs.size()
	for i in n:
		var x := -500.0 + (i + 0.5) * 1000.0 / n
		var c := corde(vue, Vector2(x, -200), {"rope_length": 280.0})
		var h := _acrobate(vue, Vector2(x, 60), {propriete: valeurs[i]}, VARIANTES[i % 4])
		heros.append(h)
		etiquette(Vector2(576 + x, 545), textes[i], 16, VARIANTES[i % 4])
		c.name = "Corde%d" % i
	await images(3)
	for h in heros:
		h.get_node("Corde").try_grab()
	return heros


func _tous(cibles: Array, etapes: Array) -> void:
	for i in cibles.size() - 1:
		piloter(cibles[i], etapes)
	await piloter(cibles[cibles.size() - 1], etapes)


func _swing_force() -> void:
	sujet("swing_force : intensité du balancement")
	code("$RopeMovement2D.swing_force = 2000.0")
	var heros := await _acrobates("swing_force", [500.0, 2000.0, 5000.0], ["500", "2000 (défaut)", "5000"])
	var d := heros.map(func(h): return h.get_node("Directeur"))
	for i in 8:
		await _tous(d, [[0.55, Vector2.RIGHT if i % 2 == 0 else Vector2.LEFT]])


func _jump_up() -> void:
	sujet("jump_up_velocity et release() : lâcher la corde")
	code("$RopeMovement2D.jump_up_velocity = 320.0\n$RopeMovement2D.release(true)   # lâcher en sautant")
	var heros := await _acrobates("jump_up_velocity", [0.0, 320.0, 600.0], ["0", "320 (défaut)", "600"])
	var d := heros.map(func(h): return h.get_node("Directeur"))
	for i in 4:
		await _tous(d, [[0.55, Vector2.RIGHT if i % 2 == 0 else Vector2.LEFT]])
	await _tous(d, [[0.2, Vector2.ZERO, "saut_long"], [1.6, Vector2.ZERO]])


func _climb_speed() -> void:
	sujet("climb_speed : vitesse de montée")
	code("$RopeMovement2D.climb_speed = 110.0")
	var heros := await _acrobates("climb_speed", [50.0, 110.0, 250.0], ["50", "110 (défaut)", "250"])
	var d := heros.map(func(h): return h.get_node("Directeur"))
	await _tous(d, [[2.0, Vector2.UP], [2.0, Vector2.DOWN], [0.5, Vector2.ZERO]])


# 📌 Exemples

func _ex_auto() -> void:
	sujet("Exemple : attraper automatiquement une corde en sautant")
	var vue := monde_2d()
	arene(vue, 1600.0)
	bloc(vue, Rect2(-800, -230, 1600, 30))
	for x in [-150.0, 150.0, 450.0]:
		corde(vue, Vector2(x, -200), {"rope_length": 260.0})
	var heros := _acrobate(vue, Vector2(-450, 150))
	var sol: GroundMovement2D = heros.get_node("Sol")
	var mouvement: RopeMovement2D = heros.get_node("Corde")
	var d: MovementDirector = heros.get_node("Directeur")
	debug(vue)
	get_tree().physics_frame.connect(func():
		if is_instance_valid(heros) and not sol.is_on_floor() and not mouvement.is_attached():
			mouvement.try_grab())
	code("if not $GroundMovement2D.is_on_floor() and not $RopeMovement2D.is_attached():\n\t$RopeMovement2D.try_grab()")
	await piloter(d, [[0.6, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [0.6, Vector2.RIGHT], [0.6, Vector2.LEFT], [0.2, Vector2.RIGHT, "saut_long"],
		[0.6, Vector2.RIGHT], [0.6, Vector2.LEFT], [0.2, Vector2.RIGHT, "saut_long"], [0.6, Vector2.RIGHT], [0.6, Vector2.LEFT], [0.2, Vector2.RIGHT, "saut_long"], [1.0, Vector2.RIGHT]])


func _ex_pont() -> void:
	sujet("Exemple : un pont suspendu (câble tendu)")
	var vue := monde_2d()
	arene(vue)
	var gauche := bloc(vue, Rect2(-260, -60, 30, 260), Formes.DECOR)
	var droite := bloc(vue, Rect2(150, -60, 30, 260), Formes.DECOR)
	bloc(vue, Rect2(-120, 60, 140, 140), Formes.SOL)
	var cable := corde(vue, Vector2(-230, -50), {"rope_length": 420.0, "point_count": 24, "collide": true, "width": 5.0})
	cable.end_anchor_path = cable.get_path_to(droite)
	code("cable.rope_length = 420.0\ncable.point_count = 24\ncable.end_anchor_path = NodePath(\"../PoteauDroit\")\ncable.collide = true")
	await secondes(1.5)
	for i in 3:
		cable.apply_impulse_at(12, Vector2(0, -500))
		await secondes(1.5)
	gauche.name = "PoteauGauche"


func _ex_lanterne() -> void:
	sujet("Exemple : une lanterne au bout d'une corde")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(-800, -230, 1600, 30))
	var c := corde(vue, Vector2(0, -200), {"rope_length": 260.0, "attached_weight": 3.0})
	var lanterne := Node2D.new()
	lanterne.draw.connect(func():
		lanterne.draw_rect(Rect2(-14, -6, 28, 34), Color("8a5a3c"))
		lanterne.draw_circle(Vector2(0, 12), 10, Color("ffd166")))
	vue.add_child(lanterne)
	c.attach_body(lanterne, c.point_count - 1)
	code("corde.attached_weight = 3.0\ncorde.attach_body($Lanterne, corde.point_count - 1)\n$Corde.apply_impulse_at($Corde.point_count - 1, Vector2(300, 0))")
	for i in 4:
		await secondes(1.2)
		legende("_on_coup_de_vent()")
		c.apply_impulse_at(c.point_count - 1, Vector2(300 if i % 2 == 0 else -300, 0))
	await secondes(1.0)


func _ex_proche() -> void:
	sujet("Exemple : une aide quand une corde est à portée")
	var vue := monde_2d()
	arene(vue, 1600.0)
	bloc(vue, Rect2(-800, -230, 1600, 30))
	corde(vue, Vector2(0, -200), {"rope_length": 300.0})
	var heros := _acrobate(vue, Vector2(-450, 150))
	var mouvement: RopeMovement2D = heros.get_node("Corde")
	var aide := Label.new()
	aide.text = "[E] Attraper"
	aide.add_theme_color_override("font_color", ACCENT)
	aide.position = Vector2(-30, -70)
	heros.add_child(aide)
	get_tree().process_frame.connect(func():
		if is_instance_valid(aide):
			var info = Rope2D.find_nearest_info(heros.global_position, 40.0)
			aide.visible = info != null and not mouvement.is_attached())
	code("var info = Rope2D.find_nearest_info($Joueur.global_position, 40.0)\n$AideAttraper.visible = info != null and not $Joueur/RopeMovement2D.is_attached()")
	var d: MovementDirector = heros.get_node("Directeur")
	await piloter(d, [[1.3, Vector2.RIGHT], [0.6, Vector2.ZERO], [0.6, Vector2.RIGHT]])
	await _attraper(heros, 0.0, 0.6)
	await piloter(d, [[1.0, Vector2.ZERO], [0.2, Vector2.RIGHT, "saut_long"], [1.4, Vector2.RIGHT]])


func _ex_visuel3d() -> void:
	sujet("Exemple : une corde épaisse en 3D")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(20, 1, 10))
	bloc_3d(vue, Vector3(0, 6.6, 0), Vector3(3, 0.3, 3), Formes.DECOR)
	var c := _corde_3d(vue, Vector3(0, 6.4, 0), {"rope_length": 4.5})
	var chemin := Path3D.new()
	vue.add_child(chemin)
	var tube := CSGPolygon3D.new()
	tube.mode = CSGPolygon3D.MODE_PATH
	var cercle := PackedVector2Array()
	for i in 8:
		cercle.append(Vector2.from_angle(i * TAU / 8.0) * 0.08)
	tube.polygon = cercle
	tube.path_node = tube.get_path_to(chemin)
	tube.material = Formes.matiere(Color("c9a46b"))
	vue.add_child(tube)
	tube.path_node = tube.get_path_to(chemin)
	code("var courbe := Curve3D.new()\nfor p in $Corde.get_points():\n\tcourbe.add_point($Chemin.to_local(p))\n$Chemin.curve = courbe   # Path3D + CSGPolygon3D")
	get_tree().process_frame.connect(func():
		if is_instance_valid(chemin):
			var courbe := Curve3D.new()
			for p in c.get_points():
				courbe.add_point(chemin.to_local(p))
			chemin.curve = courbe)
	for i in 5:
		await secondes(1.2)
		c.apply_impulse_at(c.point_count - 1, Vector3(3 if i % 2 == 0 else -3, 0, 2))
	await secondes(0.8)
