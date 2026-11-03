extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## 💥 Plans filmés de la page Pop de texte, d'image et d'objets (effets/pop-de-texte-d-image-et-d-objets.html).


func titre_module() -> String:
	return "💥 Pop · PopSpawner, FXPop, PopStyle"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-PopSpawner": _classe_spawner,
		"classe-FXPop": _classe_fxpop,
		"classe-PopStyle": _classe_style,
		"PopSpawner.pop_scene": _pop_scene,
		"PopSpawner.max_pops": _max_pops,
		"PopSpawner.clear": _clear,
		"PopSpawner.unit_scale_3d": _unit_scale,
		"FXPop.count": _fxpop_count,
		"PopStyle.direction_degrees": _direction,
		"PopStyle.spread_degrees": _spread,
		"PopStyle.speed_max": _vitesse,
		"PopStyle.gravity": _gravite,
		"PopStyle.damping": _damping,
		"PopStyle.lifetime": _lifetime,
		"PopStyle.fade_out_duration": _fade_out,
		"PopStyle.pop_scale": _pop_scale,
		"PopStyle.end_scale": _end_scale,
		"PopStyle.scale_curve": _scale_curve,
		"PopStyle.use_floor": _use_floor,
		"PopStyle.bounce": _bounce,
		"PopStyle.floor_friction": _friction,
		"PopStyle.spin_max": _spin,
		"PopStyle.spawn_radius": _spawn_radius,
		"PopStyle.font_size": _font_size,
		"PopStyle.outline_size": _outline,
		"PopStyle.negative_color": _couleurs,
		"PopStyle.icon_size": _icon_size,
		"PopStyle.always_on_top_3d": _on_top,
		"PopStyle.z_index": _z_index,
		"ex-pieces": _ex_pieces,
		"ex-fx": _ex_fx,
		"ex-3d-ecran": _ex_3d_ecran,
		"ex-position": _ex_position,
		"ex-interface": _ex_interface,
		"ex-popped": _ex_popped,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Un PopSpawner sous l'ennemi (<code>offset</code> au-dessus de sa tête) : <code>pop_number(-montant)</code> à chaque coup.",
		"classe-PopSpawner": "Texte, nombre, image ou scène : chaque appel crée un pop qui jaillit de la cible puis se libère seul.",
		"classe-FXPop": "FXPop : un effet qui fait jaillir des pops à chaque jeu, ici enchaîné à une secousse.",
		"classe-PopStyle": "Trois styles pour les mêmes appels : éclat léger, chiffres lourds, bulles qui s'envolent.",
		"PopSpawner.pop_scene": "<code>pop_scene()</code> : une scène entière (ici une étoile animée) jaillit comme un pop.",
		"PopSpawner.max_pops": "Rafale de pops : avec <code>max_pops = 8</code>, les plus anciens sont libérés d'un coup.",
		"PopSpawner.clear": "<code>get_active_count()</code> en direct, puis <code>clear()</code> libère tous les pops vivants.",
		"PopSpawner.unit_scale_3d": "Pops 3D avec <code>unit_scale_3d</code> 0,01 (défaut) et 0,02 : deux fois plus grands et rapides.",
		"FXPop.count": "<code>count</code> 1, 5 et 12 : le nombre de pops par jeu de l'effet.",
		"PopStyle.direction_degrees": "<code>direction_degrees</code> 0 (haut), 90 (droite) et 180 (bas).",
		"PopStyle.spread_degrees": "<code>spread_degrees</code> 0, 40 (défaut) et 360 : la dispersion autour de la direction.",
		"PopStyle.speed_max": "Vitesses de départ 50-100, 150-250 (défaut) et 400-600 px/s.",
		"PopStyle.gravity": "<code>gravity</code> 600 (défaut), 0 et -300 (les pops s'envolent).",
		"PopStyle.damping": "<code>damping</code> 0 (défaut) et 4 : le mouvement freine.",
		"PopStyle.lifetime": "<code>lifetime</code> 0,5, 1 (défaut) et 2 s.",
		"PopStyle.fade_out_duration": "<code>fade_out_duration</code> 0, 0,3 (défaut) et 1 s : fondu et réduction de fin de vie.",
		"PopStyle.pop_scale": "<code>pop_scale</code> 1, 1,4 (défaut) et 2,2 : le « pop » d'apparition (de <code>start_scale</code> à <code>pop_scale</code>, puis 1, sur <code>pop_in_duration</code>).",
		"PopStyle.end_scale": "<code>end_scale</code> 0,2, 0,6 (défaut) et 1,5 : l'échelle atteinte en fin de vie.",
		"PopStyle.scale_curve": "Une Curve de rebond remplace l'apparition et la réduction : échelle selon l'âge du pop.",
		"PopStyle.use_floor": "<code>use_floor</code> : un sol à <code>floor_offset</code> sous le point de départ, sur lequel les pops rebondissent.",
		"PopStyle.bounce": "<code>bounce</code> 0, 0,5 (défaut) et 0,9.",
		"PopStyle.floor_friction": "<code>floor_friction</code> 0,1 et 0,95 : la vitesse horizontale gardée à chaque rebond.",
		"PopStyle.spin_max": "Rotation tirée entre <code>spin_min</code> et <code>spin_max</code> : 0 puis -360 à 360 °/s.",
		"PopStyle.spawn_radius": "<code>spawn_radius</code> 0 et 60 px : le disque d'où partent les pops.",
		"PopStyle.font_size": "<code>font_size</code> 16, 32 (défaut) et 56 (police par défaut, ou <code>font</code>).",
		"PopStyle.outline_size": "<code>outline_size</code> 0, 8 (défaut) et 16, en <code>outline_color</code> noir puis violet.",
		"PopStyle.negative_color": "<code>pop_number</code> : positifs en <code>positive_color</code>, négatifs en <code>negative_color</code> ; <code>pop_text</code> sans couleur : <code>text_color</code>.",
		"PopStyle.icon_size": "<code>icon_size</code> 24, 32 (défaut) et 56 px, et <code>spacing</code> entre l'image et le texte.",
		"PopStyle.always_on_top_3d": "Derrière un mur : <code>always_on_top_3d</code> true (défaut) garde le pop lisible, false le cache.",
		"PopStyle.z_index": "<code>z_index</code> 100 (défaut) au-dessus du décor, -10 derrière.",
		"ex-pieces": "Coffre ouvert : six pièces jaillissent en gerbe, rebondissent au sol puis disparaissent.",
		"ex-fx": "Collecte : le cœur rétrécit et un « +1 » avec son icône jaillit (FXPop enchaîné à FXChangeSize).",
		"ex-3d-ecran": "Ennemis 3D à différentes distances : <code>screen_space_3d</code> garde les dégâts à taille constante.",
		"ex-position": "<code>pop(..., at = point)</code> : le pop part du point d'impact, « CRITIQUE ! » en orange.",
		"ex-interface": "Gain d'or : le pop part du compteur du HUD (CanvasLayer), au-dessus du jeu.",
		"ex-popped": "<code>popped</code> : chaque pop reçoit une teinte et un contour épais, et un son (coupé ici).",
	}


func schemas() -> Dictionary:
	return {
		"courbes": ["courbes.svg", "Échelle (<code>get_scale_at</code>) et opacité (<code>get_alpha_at</code>) d'un pop selon son âge, avec le style par défaut : apparition de <code>start_scale</code> à <code>pop_scale</code> puis 1, fondu et réduction vers <code>end_scale</code> pendant <code>fade_out_duration</code>."],
	}


func reutilisations() -> Dictionary:
	return {
		"PopSpawner.offset": "mode-emploi",
		"PopSpawner.pop_number": "mode-emploi",
		"PopSpawner.style": "classe-PopStyle",
		"PopSpawner.target": "classe-PopSpawner",
		"PopSpawner.pop_text": "classe-PopSpawner",
		"PopSpawner.pop_texture": "ex-pieces",
		"PopSpawner.pop": "ex-position",
		"PopSpawner.container": "ex-interface",
		"PopSpawner.screen_space_3d": "ex-3d-ecran",
		"PopSpawner.get_active_count": "PopSpawner.clear",
		"PopSpawner.get_active_pops": "PopSpawner.clear",
		"PopSpawner.popped": "ex-popped",
		"FXPop.text": "classe-FXPop",
		"FXPop.texture": "ex-fx",
		"FXPop.color": "classe-FXPop",
		"FXPop.offset": "classe-FXPop",
		"FXPop.scene": "PopSpawner.pop_scene",
		"FXPop.style": "classe-PopStyle",
		"FXPop.screen_space_3d": "ex-3d-ecran",
		"FXPop.unit_scale_3d": "PopSpawner.unit_scale_3d",
		"PopStyle.speed_min": "PopStyle.speed_max",
		"PopStyle.spin_min": "PopStyle.spin_max",
		"PopStyle.pop_in_duration": "PopStyle.pop_scale",
		"PopStyle.start_scale": "PopStyle.pop_scale",
		"PopStyle.floor_offset": "PopStyle.use_floor",
		"PopStyle.positive_color": "PopStyle.negative_color",
		"PopStyle.text_color": "PopStyle.negative_color",
		"PopStyle.outline_color": "PopStyle.outline_size",
		"PopStyle.font": "PopStyle.font_size",
		"PopStyle.spacing": "PopStyle.icon_size",
		"PopStyle.get_alpha_at": "courbes",
		"PopStyle.get_scale_at": "courbes",
	}


func _style(reglages: Dictionary = {}) -> PopStyle:
	var style := PopStyle.new()
	for propriete in reglages:
		style.set(propriete, reglages[propriete])
	return style


func _spawner(cible: Node, reglages: Dictionary = {}, decalage: Vector3 = Vector3(0, -50, 0)) -> PopSpawner:
	var spawner := PopSpawner.new()
	spawner.style = _style(reglages)
	spawner.offset = decalage
	cible.add_child(spawner)
	return spawner


func _piece() -> Texture2D:
	return Formes.pixel_art(PackedStringArray(["..jjjj..", ".jjooojj", "jjoojjoj", "jojjjjoj", "jojjjjoj", "jjoojjoj", ".jjooojj", "..jjjj.."]),
		{"j": Color("ffd166"), "o": Color("f2a33a")}, 4)


func _coeur() -> Texture2D:
	return Formes.pixel_art(PackedStringArray([".rr.rr.", "rrrrrrr", "rrrrrrr", ".rrrrr.", "..rrr..", "...r..."]), {"r": Color("ef5f5f")}, 4)


## Colonnes de cibles, chacune avec un PopSpawner réglé par `communs` + {propriete: valeurs[i]} ;
## `appel(spawner)` crée les pops, répété `fois` fois toutes les `pause` secondes.
func _colonnes_pop(propriete: String, valeurs: Array, textes: Array, communs: Dictionary, appel: Callable,
		fois: int = 6, pause: float = 0.7, hauteur: float = 330.0) -> void:
	var pas := TAILLE.x / (valeurs.size() + 1)
	var spawners: Array[PopSpawner] = []
	for i in valeurs.size():
		var x := pas * (i + 1)
		var cible := carre(Vector2(x, hauteur), VARIANTES[i % 4].darkened(0.2), 50)
		etiquette(Vector2(x, 560), textes[i], 18, VARIANTES[i % 4])
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		spawners.append(_spawner(cible, reglages, Vector3(0, -30, 0)))
	for n in fois:
		for spawner in spawners:
			appel.call(spawner)
		await secondes(pause)
	await secondes(1.0)


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : des chiffres de dégâts")
	arbre(Vector2(40, 140), PackedStringArray(["Ennemi", "  PopSpawner  (offset = (0, -40, 0))"]))
	var ennemi := carre(Vector2(700, 360), MAUVAIS.lightened(0.2), 90)
	var heros := carre(Vector2(420, 360), VARIANTES[0], 60)
	var spawner := _spawner(ennemi, {}, Vector3(0, -40, 0))
	code("func subir_degats(montant: int) -> void:\n\tvie -= montant\n\t$PopSpawner.pop_number(-montant)")
	for montant in [12, 7, 23, 9, 15]:
		await deplacer(heros, Vector2(610, 360), 0.15)
		legende("subir_degats(%d) : « -%d » en negative_color" % [montant, montant])
		spawner.pop_number(-montant)
		await deplacer(heros, Vector2(420, 360), 0.3)
		await secondes(0.4)
	await secondes(0.8)


# 💻 API

func _classe_spawner() -> void:
	sujet("PopSpawner : faire jaillir texte, nombres, images et scènes")
	var cible := carre(Vector2(576, 380), VARIANTES[2], 70)
	var spawner := _spawner(cible)
	var appels := [["pop_text(\"Paf !\")", func(): spawner.pop_text("Paf !")], ["pop_number(+25)", func(): spawner.pop_number(25)],
		["pop_number(-8)", func(): spawner.pop_number(-8)], ["pop_texture(piece)", func(): spawner.pop_texture(_piece())],
		["pop(\"x3\", coeur)", func(): spawner.pop("x3", _coeur())], ["pop_text(\"Critique\", Color.ORANGE)", func(): spawner.pop_text("Critique", Color.ORANGE)]]
	for appel in appels:
		code("$PopSpawner." + appel[0])
		appel[1].call()
		await secondes(0.9)
	await secondes(0.8)


func _classe_fxpop() -> void:
	sujet("FXPop : des pops à chaque jeu d'un effet")
	var cible := carre(Vector2(576, 380), Color.WHITE, 80)
	var secousse := effet("FXShake", {"duration": 0.3, "max_amplitude": 8.0})
	var pop: FXPop = effet("FXPop", {"text": "Bonk", "color": Color.ORANGE, "offset": Vector3(0, -50, 0)})
	secousse.effects_on_play_fx = [pop]
	var joueur := joueur_fx(cible, secousse)
	code("pop.text = \"Bonk\"\npop.color = Color.ORANGE\nsecousse.effects_on_play_fx = [pop]\n$PlayerFX.play()")
	for i in 5:
		joueur.play()
		await secondes(0.9)
	await secondes(0.6)


func _classe_style() -> void:
	sujet("PopStyle : mouvement et apparence")
	var styles := [{"speed_min": 300.0, "speed_max": 400.0, "spread_degrees": 120.0, "font_size": 22, "lifetime": 0.6},
		{"gravity": 1400.0, "speed_min": 450.0, "speed_max": 500.0, "spread_degrees": 20.0, "font_size": 44, "negative_color": Color("ff3b3b")},
		{"gravity": -200.0, "speed_min": 40.0, "speed_max": 80.0, "spread_degrees": 60.0, "font_size": 26, "text_color": Color("9fd8ff"), "lifetime": 2.0}]
	var noms := ["Éclat léger", "Dégâts lourds", "Bulles qui s'envolent"]
	var spawners: Array[PopSpawner] = []
	for i in 3:
		var cible := carre(Vector2(288 + i * 288, 380), VARIANTES[i].darkened(0.2), 50)
		etiquette(Vector2(288 + i * 288, 560), noms[i], 18, VARIANTES[i])
		spawners.append(_spawner(cible, styles[i]))
	code("var style := PopStyle.new()\nstyle.gravity = 1400.0\nstyle.font_size = 44\n$PopSpawner.style = style")
	for n in 7:
		spawners[0].pop_text("✦")
		spawners[1].pop_number(-randi_range(20, 99))
		spawners[2].pop_text("o")
		await secondes(0.6)
	await secondes(1.0)


func _pop_scene() -> void:
	sujet("pop_scene() : une scène comme pop")
	var etoile := Node2D.new()
	var dessin := Polygon2D.new()
	var points := PackedVector2Array()
	for i in 10:
		var r := 22.0 if i % 2 == 0 else 9.0
		points.append(Vector2.from_angle(i * TAU / 10.0 - PI / 2.0) * r)
	dessin.polygon = points
	dessin.color = Color("ffd166")
	dessin.name = "Etoile"
	etoile.add_child(dessin)
	dessin.owner = etoile
	var scene_etoile := PackedScene.new()
	scene_etoile.pack(etoile)
	var cible := carre(Vector2(576, 380), VARIANTES[2], 70)
	var spawner := _spawner(cible, {"spin_min": -360.0, "spin_max": 360.0, "spread_degrees": 90.0})
	code("$PopSpawner.pop_scene(preload(\"res://etoile.tscn\"))")
	for i in 8:
		spawner.pop_scene(scene_etoile)
		await secondes(0.5)
	await secondes(1.0)


func _max_pops() -> void:
	sujet("max_pops : nombre maximal de pops vivants")
	var cibles := []
	for i in 2:
		var cible := carre(Vector2(380 + i * 400, 400), VARIANTES[i].darkened(0.2), 50)
		var spawner := _spawner(cible, {"lifetime": 3.0, "gravity": 100.0, "spread_degrees": 160.0})
		spawner.max_pops = [64, 8][i]
		etiquette(Vector2(380 + i * 400, 560), ["max_pops = 64 (défaut)", "max_pops = 8"][i], 18, VARIANTES[i])
		cibles.append(spawner)
	var compte := etiquette(Vector2(576, 150), "", 18, ACCENT)
	code("$PopSpawner.max_pops = 8")
	for n in 40:
		for spawner in cibles:
			spawner.pop_text(str(n))
		compte.text = "get_active_count() : %d   ·   %d" % [cibles[0].get_active_count(), cibles[1].get_active_count()]
		await secondes(0.1)
	await secondes(1.5)


func _clear() -> void:
	sujet("get_active_count(), get_active_pops(), clear()")
	var cible := carre(Vector2(576, 420), VARIANTES[0].darkened(0.2), 50)
	var spawner := _spawner(cible, {"lifetime": 4.0, "gravity": 60.0, "spread_degrees": 200.0, "speed_min": 60.0, "speed_max": 180.0})
	var compte := etiquette(Vector2(576, 150), "", 20, ACCENT)
	get_tree().process_frame.connect(func():
		if is_instance_valid(compte):
			compte.text = "get_active_count() = %d" % spawner.get_active_count())
	for tour in 2:
		code("$PopSpawner.pop_number(...)")
		for n in 15:
			spawner.pop_number(randi_range(1, 50))
			await secondes(0.1)
		await secondes(0.8)
		code("$PopSpawner.clear()")
		legende("clear() : tous les pops vivants sont libérés")
		spawner.clear()
		await secondes(1.2)
		legende("")


func _unit_scale() -> void:
	sujet("unit_scale_3d : taille des pops en 3D")
	var vue := vue_3d(Rect2(76, 140, 1000, 390))
	cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(16, 0.2, 10))
	var spawners: Array[PopSpawner] = []
	for i in 2:
		var ennemi := Formes.cube(vue, Vector3(-2.2 + i * 4.4, 0.6, 0), VARIANTES[i], Vector3(1, 1.2, 1))
		var spawner := _spawner(ennemi, {}, Vector3(0, 1.0, 0))
		spawner.unit_scale_3d = [0.01, 0.02][i]
		spawners.append(spawner)
	etiquette(Vector2(330, 555), "unit_scale_3d = 0,01 (défaut)", 17, VARIANTES[0])
	etiquette(Vector2(820, 555), "unit_scale_3d = 0,02", 17, VARIANTES[1])
	code("$PopSpawner.unit_scale_3d = 0.02   # 100 px = 2 m")
	for n in 7:
		for spawner in spawners:
			spawner.pop_number(-randi_range(5, 40))
		await secondes(0.7)
	await secondes(1.0)


func _fxpop_count() -> void:
	sujet("FXPop.count : nombre de pops par jeu")
	var joueurs: Array[PlayerFX] = []
	for i in 3:
		var x := 288 + i * 288
		var cible := carre(Vector2(x, 400), VARIANTES[i].darkened(0.2), 50)
		etiquette(Vector2(x, 560), "count = %d" % [1, 5, 12][i], 18, VARIANTES[i])
		joueurs.append(joueur_fx(cible, effet("FXPop", {"texture": _piece(), "count": [1, 5, 12][i], "style": _style({"spread_degrees": 120.0, "speed_min": 200.0, "speed_max": 380.0})})))
	code("pop.count = 5")
	await jouer_ensemble(joueurs, 4, 1.3)


func _direction() -> void:
	code("style.direction_degrees = 90.0")
	sujet("direction_degrees : direction de départ")
	await _colonnes_pop("direction_degrees", [0.0, 90.0, 180.0], ["0 : haut (défaut)", "90 : droite", "180 : bas"], {"spread_degrees": 20.0, "gravity": 0.0},
		func(s): s.pop_text("•"), 10, 0.3)


func _spread() -> void:
	sujet("spread_degrees : dispersion")
	code("style.spread_degrees = 40.0")
	await _colonnes_pop("spread_degrees", [0.0, 40.0, 360.0], ["0", "40 (défaut)", "360"], {"gravity": 200.0}, func(s): s.pop_text("•"), 14, 0.2)


func _vitesse() -> void:
	sujet("speed_min et speed_max : vitesse de départ")
	code("style.speed_min = 150.0\nstyle.speed_max = 250.0")
	var valeurs := [100.0, 250.0, 600.0]
	var pas := TAILLE.x / 4.0
	var spawners: Array[PopSpawner] = []
	for i in 3:
		var cible := carre(Vector2(pas * (i + 1), 420), VARIANTES[i].darkened(0.2), 50)
		etiquette(Vector2(pas * (i + 1), 560), ["50 - 100", "150 - 250 (défaut)", "400 - 600"][i], 18, VARIANTES[i])
		spawners.append(_spawner(cible, {"speed_min": valeurs[i] * 0.6 if i != 1 else 150.0, "speed_max": valeurs[i]}))
	for n in 8:
		for spawner in spawners:
			spawner.pop_text("•")
		await secondes(0.5)
	await secondes(1.0)


func _gravite() -> void:
	sujet("gravity : accélération vers le bas")
	code("style.gravity = 600.0")
	await _colonnes_pop("gravity", [600.0, 0.0, -300.0], ["600 (défaut)", "0", "-300"], {"lifetime": 1.6}, func(s): s.pop_text("•"), 8, 0.5, 380.0)


func _damping() -> void:
	sujet("damping : freinage du mouvement")
	code("style.damping = 4.0")
	await _colonnes_pop("damping", [0.0, 4.0], ["0 (défaut)", "4"], {"gravity": 0.0, "spread_degrees": 360.0, "speed_min": 250.0, "speed_max": 300.0, "lifetime": 1.5},
		func(s): s.pop_text("•"), 10, 0.4)


func _lifetime() -> void:
	sujet("lifetime : durée de vie")
	code("style.lifetime = 1.0")
	await _colonnes_pop("lifetime", [0.5, 1.0, 2.0], ["0,5 s", "1 s (défaut)", "2 s"], {"gravity": 100.0}, func(s): s.pop_number(randi_range(1, 99)), 6, 0.8)


func _fade_out() -> void:
	sujet("fade_out_duration : fondu de fin de vie")
	code("style.fade_out_duration = 0.3")
	await _colonnes_pop("fade_out_duration", [0.0, 0.3, 1.0], ["0", "0,3 (défaut)", "1"], {"gravity": 100.0, "lifetime": 1.4}, func(s): s.pop_text("Pop"), 5, 1.0)


func _pop_scale() -> void:
	sujet("start_scale, pop_scale, pop_in_duration : l'apparition")
	code("style.start_scale = 0.2\nstyle.pop_scale = 1.4\nstyle.pop_in_duration = 0.18")
	await _colonnes_pop("pop_scale", [1.0, 1.4, 2.2], ["1", "1,4 (défaut)", "2,2"], {"gravity": 0.0, "speed_min": 30.0, "speed_max": 40.0, "pop_in_duration": 0.4},
		func(s): s.pop_text("POP"), 5, 1.0)


func _end_scale() -> void:
	sujet("end_scale : échelle en fin de vie")
	code("style.end_scale = 0.6")
	await _colonnes_pop("end_scale", [0.2, 0.6, 1.5], ["0,2", "0,6 (défaut)", "1,5"], {"gravity": 0.0, "speed_min": 30.0, "speed_max": 40.0, "fade_out_duration": 0.8, "lifetime": 1.5},
		func(s): s.pop_text("Fin"), 4, 1.3)


func _scale_curve() -> void:
	sujet("scale_curve : échelle selon l'âge (Curve)")
	var courbe := Curve.new()
	courbe.add_point(Vector2(0, 0))
	courbe.add_point(Vector2(0.15, 1.6))
	courbe.add_point(Vector2(0.3, 0.8))
	courbe.add_point(Vector2(0.45, 1.2))
	courbe.add_point(Vector2(0.8, 1.0))
	courbe.add_point(Vector2(1, 0))
	courbe.max_value = 2.0
	code("style.scale_curve = preload(\"res://rebond.tres\")  # Curve")
	await _colonnes_pop("scale_curve", [null, courbe], ["sans courbe (défaut)", "courbe de rebond"], {"gravity": 0.0, "speed_min": 30.0, "speed_max": 40.0, "lifetime": 1.5},
		func(s): s.pop_text("Boing"), 4, 1.4)


func _use_floor() -> void:
	sujet("use_floor et floor_offset : un sol de rebond")
	code("style.use_floor = true\nstyle.floor_offset = 40.0")
	await _colonnes_pop("use_floor", [false, true], ["use_floor = false", "true, floor_offset = 40"], {"lifetime": 2.0, "spread_degrees": 90.0, "speed_min": 250.0, "speed_max": 350.0},
		func(s): s.pop_texture(_piece()), 6, 0.6)


func _bounce() -> void:
	sujet("bounce : restitution au rebond")
	code("style.bounce = 0.5")
	await _colonnes_pop("bounce", [0.0, 0.5, 0.9], ["0", "0,5 (défaut)", "0,9"], {"use_floor": true, "lifetime": 2.2, "spread_degrees": 30.0, "speed_min": 350.0, "speed_max": 380.0},
		func(s): s.pop_texture(_piece()), 4, 1.0)


func _friction() -> void:
	sujet("floor_friction : glissement au sol")
	code("style.floor_friction = 0.6")
	await _colonnes_pop("floor_friction", [0.1, 0.95], ["0,1", "0,95"], {"use_floor": true, "bounce": 0.3, "lifetime": 2.2, "direction_degrees": 60.0, "spread_degrees": 10.0, "speed_min": 350.0, "speed_max": 380.0},
		func(s): s.pop_texture(_piece()), 4, 1.0)


func _spin() -> void:
	sujet("spin_min et spin_max : rotation")
	code("style.spin_min = -360.0\nstyle.spin_max = 360.0")
	await _colonnes_pop("spin_max", [0.0, 360.0], ["0 et 0 (défaut)", "-360 à 360 °/s"], {"spin_min": -360.0, "lifetime": 1.5}, func(s):
		if s.style.spin_max == 0.0:
			s.style.spin_min = 0.0
		s.pop_texture(_piece()), 8, 0.5)


func _spawn_radius() -> void:
	sujet("spawn_radius : disque de départ")
	code("style.spawn_radius = 60.0")
	await _colonnes_pop("spawn_radius", [0.0, 60.0], ["0 (défaut)", "60 px"], {"gravity": 0.0, "speed_min": 20.0, "speed_max": 30.0, "spread_degrees": 360.0}, func(s): s.pop_text("•"), 12, 0.25)


func _font_size() -> void:
	sujet("font_size et font : le texte")
	code("style.font_size = 32\nstyle.font = null   # police par défaut")
	await _colonnes_pop("font_size", [16, 32, 56], ["16", "32 (défaut)", "56"], {}, func(s): s.pop_number(randi_range(1, 99)), 6, 0.7)


func _outline() -> void:
	sujet("outline_size et outline_color : le contour")
	code("style.outline_size = 8\nstyle.outline_color = Color.BLACK")
	await _colonnes_pop("outline_size", [0, 8, 16], ["0", "8 (défaut)", "16, contour violet"], {"font_size": 40}, func(s):
		if s.style.outline_size == 16:
			s.style.outline_color = Color("6b2fa3")
		s.pop_text("Wow"), 5, 0.9)


func _couleurs() -> void:
	sujet("positive_color, negative_color, text_color")
	var cible := carre(Vector2(576, 400), VARIANTES[2].darkened(0.2), 60)
	var spawner := _spawner(cible)
	for appel in [["pop_number(15)  # positive_color", func(): spawner.pop_number(15)], ["pop_number(-9)  # negative_color", func(): spawner.pop_number(-9)],
			["pop_text(\"Esquive\")  # text_color", func(): spawner.pop_text("Esquive")], ["pop_number(0)  # positive_color", func(): spawner.pop_number(0)]]:
		code("$PopSpawner." + appel[0])
		for n in 2:
			appel[1].call()
			await secondes(0.6)
	await secondes(0.8)


func _icon_size() -> void:
	sujet("icon_size et spacing : l'image")
	code("style.icon_size = Vector2(32, 32)\nstyle.spacing = 4.0")
	await _colonnes_pop("icon_size", [Vector2(24, 24), Vector2(32, 32), Vector2(56, 56)], ["24", "32 (défaut)", "56, spacing 16"], {}, func(s):
		if s.style.icon_size.x > 50:
			s.style.spacing = 16.0
		s.pop("+5", _piece()), 5, 0.9)


func _on_top() -> void:
	sujet("always_on_top_3d : lisible derrière le décor")
	var vue := vue_3d(Rect2(76, 140, 1000, 390))
	cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(16, 0.2, 10))
	var spawners: Array[PopSpawner] = []
	for i in 2:
		var ennemi := Formes.cube(vue, Vector3(-2.2 + i * 4.4, 0.6, -1.5), VARIANTES[i], Vector3(1, 1.2, 1))
		Formes.cube(vue, Vector3(-2.2 + i * 4.4, 1.5, 0.3), Formes.DECOR, Vector3(2.4, 3.0, 0.3))
		spawners.append(_spawner(ennemi, {"always_on_top_3d": i == 0, "gravity": 0.0}, Vector3(0, 1.0, 0)))
	etiquette(Vector2(330, 555), "always_on_top_3d = true (défaut)", 17, VARIANTES[0])
	etiquette(Vector2(820, 555), "false", 17, VARIANTES[1])
	code("style.always_on_top_3d = true")
	for n in 7:
		for spawner in spawners:
			spawner.pop_number(-randi_range(5, 40))
		await secondes(0.7)
	await secondes(1.0)


func _z_index() -> void:
	sujet("z_index : ordre de dessin des pops 2D")
	var spawners: Array[PopSpawner] = []
	for i in 2:
		var cible := carre(Vector2(380 + i * 400, 430), VARIANTES[i].darkened(0.2), 50)
		var mur := carre(Vector2(380 + i * 400, 300), Formes.DECOR, 10)
		mur.scale = Vector2(22, 12)
		spawners.append(_spawner(cible, {"z_index": [100, -10][i]}))
		etiquette(Vector2(380 + i * 400, 560), ["z_index = 100 (défaut)", "z_index = -10"][i], 18, VARIANTES[i])
	code("style.z_index = 100")
	for n in 7:
		for spawner in spawners:
			spawner.pop_number(randi_range(1, 99))
		await secondes(0.6)
	await secondes(1.0)


# 📌 Exemples

func _ex_pieces() -> void:
	sujet("Exemple : des pièces qui jaillissent d'un coffre")
	var sol := Polygon2D.new()
	sol.polygon = PackedVector2Array([Vector2(0, 440), Vector2(1152, 440), Vector2(1152, 648), Vector2(0, 648)])
	sol.color = Formes.SOL
	scene.add_child(sol)
	var coffre := carre(Vector2(576, 410), Color("8a5a3c"), 60)
	var spawner := _spawner(coffre, {"spread_degrees": 100.0, "speed_min": 250.0, "speed_max": 400.0, "use_floor": true, "floor_offset": 20.0,
		"bounce": 0.4, "lifetime": 1.6, "icon_size": Vector2(24, 24)}, Vector3(0, -10, 0))
	code("style.use_floor = true · bounce = 0.4\nfor i in 6:\n\t$PopSpawner.pop_texture(preload(\"res://icones/piece.png\"))")
	for tour in 3:
		await secondes(0.6)
		legende("ouvrir_coffre()")
		for i in 6:
			spawner.pop_texture(_piece())
		await secondes(1.6)


func _ex_fx() -> void:
	sujet("Exemple : un « +1 » à la collecte, sans code de pop")
	for tour in 3:
		var coeur := Sprite2D.new()
		coeur.texture = _coeur()
		coeur.scale = Vector2(2, 2)
		coeur.position = Vector2(576, 330)
		scene.add_child(coeur)
		var plus_un: FXPop = effet("FXPop", {"text": "+1", "texture": _coeur(), "offset": Vector3(0, -30, 0)})
		var disparition := effet("FXChangeSize", {"final_scale": 0.0, "original_value": 2.0, "duration": 0.25})
		disparition.effects_on_play_fx = [plus_un]
		var joueur := joueur_fx(coeur, disparition)
		code("plus_un.text = \"+1\"\nplus_un.texture = preload(\"res://icones/coeur.png\")\ndisparition.effects_on_play_fx = [plus_un]\n$PlayerFX.play()")
		await secondes(0.8)
		var heros := carre(Vector2(300, 330), VARIANTES[0], 50)
		await deplacer(heros, Vector2(540, 330), 0.5)
		joueur.play()
		await secondes(1.4)
		heros.queue_free()
		coeur.queue_free()


func _ex_3d_ecran() -> void:
	sujet("Exemple : dégâts à taille constante en 3D")
	var vue := vue_3d(Rect2(76, 140, 1000, 390))
	cube(vue, Vector3(0, -0.1, -6), Formes.SOL, Vector3(24, 0.2, 30))
	var spawners: Array[PopSpawner] = []
	for i in 3:
		var ennemi := Formes.cube(vue, Vector3(-3 + i * 3, 0.6, -i * 7.0), MAUVAIS.lightened(0.2), Vector3(1, 1.2, 1))
		var spawner := _spawner(ennemi, {}, Vector3(0, 1.2, 0))
		spawner.screen_space_3d = true
		spawners.append(spawner)
	code("# Ennemi3D └─ PopSpawner (screen_space_3d = true, offset = (0, 2, 0))\n$PopSpawner.pop_number(-degats)")
	legende("Proche ou lointain, le chiffre garde la même taille à l'écran")
	for n in 8:
		for spawner in spawners:
			spawner.pop_number(-randi_range(5, 40))
		await secondes(0.7)
	await secondes(1.0)


func _ex_position() -> void:
	sujet("Exemple : un pop au point d'impact")
	var mur := carre(Vector2(760, 330), Formes.DECOR, 10)
	mur.scale = Vector2(6, 34)
	var spawner := _spawner(mur)
	code("$PopSpawner.pop(texte, null, null, Color.ORANGE if critique else Color(0, 0, 0, 0), point)")
	for i in 7:
		var point := Vector2(730, 200 + randf() * 260)
		var balle := disque(Vector2(150, point.y), ACCENT, 8)
		await deplacer(balle, point, 0.3)
		balle.queue_free()
		var critique := i % 3 == 2
		spawner.pop("CRITIQUE !" if critique else "Paf", null, null, Color.ORANGE if critique else Color(0, 0, 0, 0), point)
		await secondes(0.4)


func _ex_interface() -> void:
	sujet("Exemple : un pop dans l'interface (HUD)")
	var hud := CanvasLayer.new()
	hud.layer = 5
	scene.add_child(hud)
	var compteur := Label.new()
	compteur.text = "Or : 0"
	compteur.position = Vector2(860, 130)
	compteur.add_theme_font_size_override("font_size", 28)
	hud.add_child(compteur)
	var spawner := PopSpawner.new()
	spawner.style = _style({"positive_color": Color("ffd166"), "font_size": 26})
	compteur.add_child(spawner)
	var total := 0
	code("# HUD (CanvasLayer) / CompteurOr (Label) / PopSpawner\n$HUD/CompteurOr/PopSpawner.pop_number(montant)")
	for i in 6:
		var piece := Sprite2D.new()
		piece.texture = _piece()
		piece.position = Vector2(200 + i * 90, 450)
		scene.add_child(piece)
		var heros := carre(Vector2(140 + i * 90, 450), VARIANTES[0], 40)
		await deplacer(heros, piece.position, 0.3)
		piece.queue_free()
		heros.queue_free()
		var montant := randi_range(5, 20)
		total += montant
		compteur.text = "Or : %d" % total
		spawner.pop_number(montant)
		await secondes(0.5)
	await secondes(1.0)


func _ex_popped() -> void:
	sujet("Exemple : personnaliser chaque pop (signal popped)")
	var cible := carre(Vector2(576, 400), VARIANTES[1].darkened(0.2), 60)
	var spawner := _spawner(cible, {"font_size": 40})
	spawner.popped.connect(func(pop: Node):
		if pop is Node2D:
			pop.modulate = Color(1.0, 0.9, 0.6)
		var texte := pop.get_node_or_null("Text")
		if texte is Label and texte.label_settings:
			texte.label_settings.outline_size = 12)
	code("$PopSpawner.popped.connect(func(pop: Node):\n\t$SonPop.play()\n\tpop.modulate = Color(1.0, 0.9, 0.6)\n\tpop.get_node(\"Text\").label_settings.outline_size = 12)")
	for i in 7:
		spawner.pop_text(["Pif", "Paf", "Pouf"][i % 3])
		await secondes(0.7)
	await secondes(1.0)
