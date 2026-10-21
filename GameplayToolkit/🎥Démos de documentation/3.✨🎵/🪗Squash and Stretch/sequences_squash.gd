extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## 🪗 Plans filmés de la page Squash and stretch (effets/squash-and-stretch.html) : FXStretchAndSquash.


func titre_module() -> String:
	return "🪗 Squash and stretch · FXStretchAndSquash"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-FXStretchAndSquash": _classe,
		"FXStretchAndSquash.orient_to_velocity": _orient,
		"FXStretchAndSquash.max_stretch": _max_stretch,
		"FXStretchAndSquash.max_speed": _max_speed,
		"FXStretchAndSquash.enable_impact_squash": _impact_squash,
		"FXStretchAndSquash.impact_threshold": _impact_threshold,
		"FXStretchAndSquash.max_impact_squash": _max_impact_squash,
		"FXStretchAndSquash.impact_recovery_speed": _impact_recovery,
		"FXStretchAndSquash.smooth_speed": _smooth_speed,
		"ex-projectile": _ex_projectile,
		"ex-3d": _ex_3d,
		"ex-ui": _ex_ui,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "PlayerFX sous le sprite du personnage : il s'allonge en sautant et s'écrase à l'atterrissage, la collision reste intacte.",
		"classe-FXStretchAndSquash": "Étirement dans le sens du mouvement, écrasement à l'impact, en 2D comme en 3D.",
		"FXStretchAndSquash.orient_to_velocity": "false : étirement vertical / horizontal · true : la balle tourne et s'allonge dans l'axe de sa trajectoire.",
		"FXStretchAndSquash.max_stretch": "0,2, 0,5 (défaut) et 1 : l'allongement maximal.",
		"FXStretchAndSquash.max_speed": "300, 600 (défaut) et 1200 px/s : la vitesse à laquelle l'étirement est maximal.",
		"FXStretchAndSquash.enable_impact_squash": "Sans puis avec l'écrasement à l'impact.",
		"FXStretchAndSquash.impact_threshold": "100, 300 (défaut) et 900 px/s : la perte de vitesse qui compte comme un choc.",
		"FXStretchAndSquash.max_impact_squash": "0,2, 0,4 (défaut) et 0,8 : l'écrasement maximal d'un impact.",
		"FXStretchAndSquash.impact_recovery_speed": "4, 12 (défaut) et 30 : la rapidité avec laquelle l'écrasement se résorbe.",
		"FXStretchAndSquash.smooth_speed": "4, 15 (défaut) et 40 : déformation molle ou nerveuse.",
		"ex-projectile": "Des flèches lancées en cloche, orientées et étirées dans l'axe de leur vol.",
		"ex-3d": "En 3D, vitesses en m/s : <code>max_speed = 12</code>, <code>impact_threshold = 6</code>.",
		"ex-ui": "Un panneau qui glisse (Tween Back) se déforme selon sa vitesse, puis <code>stop()</code> restaure son échelle.",
	}


## Balle qui rebondit sous gravité (Node2D) ; renvoie le nœud déplacé.
func _balle(parent: Node, position: Vector2, couleur: Color, sol: float, rayon: float = 26.0, horizontal: float = 0.0) -> Node2D:
	var balle := Node2D.new()
	balle.position = position
	balle.draw.connect(func():
		balle.draw_circle(Vector2.ZERO, rayon, couleur, true, -1.0, true)
		balle.draw_circle(Vector2(rayon * 0.35, -rayon * 0.3), rayon * 0.22, Color(1, 1, 1, 0.6)))
	parent.add_child(balle)
	var vitesse := [Vector2(horizontal, 0)]
	var gauche := position.x - 200.0
	var droite := position.x + 200.0
	get_tree().process_frame.connect(func():
		if not is_instance_valid(balle):
			return
		var delta := get_process_delta_time()
		vitesse[0].y += 1400.0 * delta
		balle.position += vitesse[0] * delta
		if balle.position.y > sol - rayon:
			balle.position.y = sol - rayon
			vitesse[0].y = -850.0
		if horizontal != 0.0 and (balle.position.x < gauche or balle.position.x > droite):
			vitesse[0].x = -vitesse[0].x)
	return balle


## Colonnes de balles comparant les valeurs d'une propriété (effet joué en continu).
func _colonnes_balles(propriete: String, valeurs: Array, textes: Array, communs: Dictionary = {}, horizontal: float = 0.0) -> void:
	var pas := TAILLE.x / (valeurs.size() + 1)
	for i in valeurs.size():
		var x := pas * (i + 1)
		var sol := Line2D.new()
		sol.points = PackedVector2Array([Vector2(x - pas / 2.0 + 10, 470), Vector2(x + pas / 2.0 - 10, 470)])
		sol.width = 3
		sol.default_color = Formes.DECOR
		scene.add_child(sol)
		var balle := _balle(scene, Vector2(x, 160 + i * 25), VARIANTES[i % 4], 470.0, 26.0, horizontal)
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		joueur_fx(balle, effet("FXStretchAndSquash", reglages)).play()
		etiquette(Vector2(x, 510), textes[i], 18, VARIANTES[i % 4])
	await secondes(7.0)


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : un personnage qui s'étire et s'écrase")
	arbre(Vector2(40, 140), PackedStringArray(["Joueur (CharacterBody2D)", "  CollisionShape2D", "  Sprite2D", "    PlayerFX  (FXStretchAndSquash)"]))
	var sol := Polygon2D.new()
	sol.polygon = PackedVector2Array([Vector2(0, 470), Vector2(1152, 470), Vector2(1152, 648), Vector2(0, 648)])
	sol.color = Formes.SOL
	scene.add_child(sol)
	var corps := Node2D.new()
	corps.position = Vector2(300, 470)
	scene.add_child(corps)
	var collision := Line2D.new()
	collision.points = Formes.rectangle(Vector2(50, 70))
	collision.closed = true
	collision.width = 2
	collision.default_color = Color(0.4, 1, 0.6, 0.8)
	collision.position = Vector2(0, -35)
	corps.add_child(collision)
	var sprite := Node2D.new()
	sprite.position = Vector2(0, -35)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(Vector2(50, 70))
	dessin.color = VARIANTES[0]
	sprite.add_child(dessin)
	corps.add_child(sprite)
	joueur_fx(sprite, effet("FXStretchAndSquash", {"max_speed": 900.0, "impact_threshold": 450.0})).play()
	code("func _ready() -> void:\n\t$Sprite2D/PlayerFX.play()")
	legende("Vert : la forme de collision (intacte) · bleu : le sprite déformé")
	var vitesse := Vector2(220, 0)
	var t := 0.0
	var saut := 0.0
	while t < 10.0:
		await images()
		var delta := get_process_delta_time()
		t += delta
		saut -= delta
		vitesse.y += 1800.0 * delta
		if corps.position.y >= 470 and saut <= 0.0:
			vitesse.y = -900.0
			saut = 0.9
		corps.position += vitesse * delta
		if corps.position.y > 470:
			corps.position.y = 470
			vitesse.y = 0
		if corps.position.x > 900 or corps.position.x < 250:
			vitesse.x = -vitesse.x


# 💻 API

func _classe() -> void:
	sujet("FXStretchAndSquash : 2D et 3D")
	var sol := Line2D.new()
	sol.points = PackedVector2Array([Vector2(80, 470), Vector2(500, 470)])
	sol.width = 3
	sol.default_color = Formes.DECOR
	scene.add_child(sol)
	var balle := _balle(scene, Vector2(290, 200), VARIANTES[1], 470.0, 30.0, 160.0)
	joueur_fx(balle, effet("FXStretchAndSquash")).play()
	etiquette(Vector2(290, 510), "2D (px/s)", 17)
	var vue := vue_3d(Rect2(600, 160, 480, 340))
	cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(10, 0.2, 10))
	var boule := Formes.sphere(vue, Vector3(0, 2, 0), VARIANTES[2], 0.5)
	joueur_fx(boule, effet("FXStretchAndSquash", {"max_speed": 10.0, "impact_threshold": 5.0, "max_stretch": 0.4})).play()
	etiquette(Vector2(840, 530), "3D (m/s)", 17)
	code("$Sprite2D/PlayerFX.play()  # effet continu, sans durée")
	var vitesse := 0.0
	var t := 0.0
	while t < 8.0:
		await images()
		var delta := get_process_delta_time()
		t += delta
		vitesse -= 22.0 * delta
		boule.position.y += vitesse * delta
		if boule.position.y < 0.5:
			boule.position.y = 0.5
			vitesse = 9.5


func _orient() -> void:
	sujet("orient_to_velocity : s'étirer dans l'axe de la trajectoire")
	code("etirement.orient_to_velocity = true")
	await _colonnes_balles("orient_to_velocity", [false, true], ["false (défaut)", "true"], {}, 220.0)


func _max_stretch() -> void:
	sujet("max_stretch : allongement maximal")
	code("etirement.max_stretch = 0.5")
	await _colonnes_balles("max_stretch", [0.2, 0.5, 1.0], ["0,2", "0,5 (défaut)", "1"])


func _max_speed() -> void:
	sujet("max_speed : vitesse d'étirement maximal")
	code("etirement.max_speed = 600.0")
	await _colonnes_balles("max_speed", [300.0, 600.0, 1200.0], ["300", "600 (défaut)", "1200"])


func _impact_squash() -> void:
	sujet("enable_impact_squash : écrasement à l'impact")
	code("etirement.enable_impact_squash = true")
	await _colonnes_balles("enable_impact_squash", [false, true], ["false", "true (défaut)"])


func _impact_threshold() -> void:
	sujet("impact_threshold : ce qui compte comme un choc")
	code("etirement.impact_threshold = 300.0")
	await _colonnes_balles("impact_threshold", [100.0, 300.0, 900.0], ["100", "300 (défaut)", "900 : jamais"])


func _max_impact_squash() -> void:
	sujet("max_impact_squash : écrasement maximal")
	code("etirement.max_impact_squash = 0.4")
	await _colonnes_balles("max_impact_squash", [0.2, 0.4, 0.8], ["0,2", "0,4 (défaut)", "0,8"])


func _impact_recovery() -> void:
	sujet("impact_recovery_speed : retour après l'écrasement")
	code("etirement.impact_recovery_speed = 12.0")
	await _colonnes_balles("impact_recovery_speed", [4.0, 12.0, 30.0], ["4", "12 (défaut)", "30"])


func _smooth_speed() -> void:
	sujet("smooth_speed : rapidité de la déformation")
	code("etirement.smooth_speed = 15.0")
	await _colonnes_balles("smooth_speed", [4.0, 15.0, 40.0], ["4", "15 (défaut)", "40"])


# 📌 Exemples

func _ex_projectile() -> void:
	sujet("Exemple : des flèches étirées dans l'axe de leur vol")
	code("etirement.orient_to_velocity = true\netirement.max_speed = 900.0\netirement.max_stretch = 0.8\netirement.enable_impact_squash = false")
	for tir in 5:
		var fleche := Node2D.new()
		fleche.position = Vector2(120, 470)
		var corps := Polygon2D.new()
		corps.polygon = PackedVector2Array([Vector2(-22, -3), Vector2(14, -3), Vector2(14, -8), Vector2(26, 0), Vector2(14, 8), Vector2(14, 3), Vector2(-22, 3)])
		corps.color = Color("cfd6e6")
		fleche.add_child(corps)
		scene.add_child(fleche)
		joueur_fx(fleche, effet("FXStretchAndSquash", {"orient_to_velocity": true, "max_speed": 900.0, "max_stretch": 0.8, "enable_impact_squash": false})).play()
		var vitesse := Vector2(650 + tir * 60, -820 + tir * 40)
		var t := 0.0
		while t < 1.3 and fleche.position.y <= 480:
			await images()
			var delta := get_process_delta_time()
			t += delta
			vitesse.y += 1100.0 * delta
			fleche.position += vitesse * delta
		await secondes(0.2)


func _ex_3d() -> void:
	sujet("Exemple : en 3D (vitesses en m/s)")
	var vue := vue_3d(Rect2(76, 140, 1000, 390))
	cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(20, 0.2, 20))
	var modele := Formes.cube(vue, Vector3(-3, 0.5, 0), VARIANTES[0], Vector3(0.8, 1.0, 0.8))
	joueur_fx(modele, effet("FXStretchAndSquash", {"max_speed": 12.0, "impact_threshold": 6.0, "max_stretch": 0.3})).play()
	code("souplesse.max_speed = 12.0         # m/s\nsouplesse.impact_threshold = 6.0\nsouplesse.max_stretch = 0.3")
	var vitesse := Vector3(2.0, 0, 0)
	var t := 0.0
	while t < 9.0:
		await images()
		var delta := get_process_delta_time()
		t += delta
		vitesse.y -= 25.0 * delta
		modele.position += vitesse * delta
		if modele.position.y < 0.5:
			modele.position.y = 0.5
			vitesse.y = 10.0
		if absf(modele.position.x) > 3.5:
			vitesse.x = -vitesse.x


func _ex_ui() -> void:
	sujet("Exemple : une interface « élastique »")
	var panneau := Panel.new()
	panneau.size = Vector2(300, 200)
	panneau.position = Vector2(-320, 220)
	panneau.pivot_offset = panneau.size / 2.0
	scene.add_child(panneau)
	var titre := Label.new()
	titre.text = "Inventaire"
	titre.position = Vector2(20, 16)
	panneau.add_child(titre)
	var joueur := joueur_fx(panneau, effet("FXStretchAndSquash", {"max_speed": 2000.0, "max_stretch": 0.4}))
	code("$Panneau/PlayerFX.play()\ntween.tween_property($Panneau, \"position:x\", 40.0, 0.4)\nawait tween.finished\n$Panneau/PlayerFX.stop()")
	for i in 3:
		joueur.play()
		var tween := panneau.create_tween().set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
		tween.tween_property(panneau, "position:x", 420.0, 0.5)
		await tween.finished
		await secondes(0.3)
		joueur.stop()
		await secondes(0.7)
		joueur.play()
		var retour := panneau.create_tween().set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_IN)
		retour.tween_property(panneau, "position:x", -320.0, 0.4)
		await retour.finished
		joueur.stop()
		await secondes(0.3)
