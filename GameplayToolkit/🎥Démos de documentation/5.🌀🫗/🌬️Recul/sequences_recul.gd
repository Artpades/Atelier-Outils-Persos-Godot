extends "res://🎥Démos de documentation/5.🌀🫗/sequences_mouvement.gd"

## 🌬️ Plans filmés de la page Recul (mouvement/recul.html) : Knockback2D / 3D.


func titre_module() -> String:
	return "🌬️ Recul · Knockback2D / 3D"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-Knockback2D": _classe_2d,
		"classe-Knockback3D": _classe_3d,
		"Knockback2D.base_force": _base_force,
		"Knockback2D.duration": _duration,
		"Knockback2D.friction": _friction,
		"Knockback2D.upward_bias": _upward_bias,
		"Knockback2D.rigid_mode": _rigid_mode,
		"Knockback2D.add_torque": _add_torque,
		"Knockback2D.enabled": _enabled,
		"Knockback2D.debug_mode": _debug_mode,
		"ex-explosion": _ex_explosion,
		"ex-invincible": _ex_invincible,
		"ex-effets": _ex_effets,
		"ex-topdown": _ex_topdown,
		"ex-seul": _ex_seul,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Au contact de l'ennemi : <code>apply_knockback_from(position_ennemi)</code>, projeté en arrière et un peu vers le haut, glissade, puis reprise du contrôle.",
		"classe-Knockback2D": "Un personnage et une caisse (RigidBody2D) repoussés par le même type de nœud.",
		"classe-Knockback3D": "En 3D : un personnage et des caisses repoussés par un coup.",
		"Knockback2D.base_force": "<code>base_force</code> 200, 450 (défaut) et 900 px/s.",
		"Knockback2D.duration": "<code>duration</code> 0,1, 0,3 (défaut) et 0,8 s de commandes bloquées (le personnage pousse vers l'ennemi).",
		"Knockback2D.friction": "<code>friction</code> 300, 900 (défaut) et 3000 : glissade longue ou arrêt sec.",
		"Knockback2D.upward_bias": "<code>upward_bias</code> 0, 0,35 (défaut) et 1 : la part « vers le haut ».",
		"Knockback2D.rigid_mode": "Corps rigides : RIGID_IMPULSE (instantané) et RIGID_CONTINUOUS (force pendant <code>duration</code>).",
		"Knockback2D.add_torque": "<code>add_torque</code> false et true (<code>torque_factor</code> 0,02) : la caisse tourne.",
		"Knockback2D.enabled": "<code>enabled = false</code> : le coup est ignoré (invincibilité, poids lourd).",
		"Knockback2D.debug_mode": "<code>debug_mode</code> : flèche de vitesse, cercle du temps restant (<code>get_lock_remaining()</code>).",
		"ex-explosion": "Explosion : tout ce qui est dans le rayon est repoussé, plus fort près du centre.",
		"ex-invincible": "<code>knocked_back</code> coupe la zone de dégâts, <code>recovered</code> la rétablit : pas de double coup.",
		"ex-effets": "Recul + flash rouge + secousse (PlayerFX), arrêtés à <code>recovered</code>.",
		"ex-topdown": "Vue de dessus : <code>upward_bias = 0</code>, <code>friction = 1500</code>.",
		"ex-seul": "Un PNJ sans composant de mouvement : le Knockback2D le déplace lui-même (<code>gravity_affect</code>).",
	}


func reutilisations() -> Dictionary:
	var meme_3d := "Même réglage qu'en 2D, en m/s (voir la vidéo 2D)."
	var r := {
		"Knockback2D.torque_factor": "Knockback2D.add_torque",
		"Knockback2D.gravity_affect": "ex-seul",
		"Knockback2D.apply_knockback": "ex-seul",
		"Knockback2D.apply_knockback_from": "mode-emploi",
		"Knockback2D.is_knocked": "ex-invincible",
		"Knockback2D.get_lock_remaining": "Knockback2D.debug_mode",
		"Knockback2D.active_on": "classe-Knockback2D",
		"Knockback2D.knocked_back": "ex-invincible",
		"Knockback2D.recovered": "ex-invincible",
		"Knockback3D.apply_knockback_from": "classe-Knockback3D",
	}
	for membre in ["add_torque", "base_force", "debug_mode", "duration", "enabled", "friction", "gravity_affect", "rigid_mode", "torque_factor", "upward_bias",
			"active_on", "apply_knockback", "get_lock_remaining", "is_knocked", "knocked_back", "recovered"]:
		r["Knockback3D." + membre] = [r.get("Knockback2D." + membre, "Knockback2D." + membre), meme_3d]
	return r


func _victime(vue: Node, position: Vector2, reglages: Dictionary = {}, couleur: Color = VARIANTES[0]) -> CharacterBody2D:
	return heros_2d(vue, position, {"Sol": "GroundMovement2D", "Recul": "Knockback2D"}, {"Recul": reglages}, couleur)


func _caisse(vue: Node, position: Vector2, reglages: Dictionary = {}, couleur: Color = Color("a0714f")) -> RigidBody2D:
	var corps := RigidBody2D.new()
	corps.position = position
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = Vector2(44, 44)
	corps.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(Vector2(44, 44))
	dessin.color = couleur
	corps.add_child(dessin)
	var recul := Knockback2D.new()
	recul.name = "Recul"
	for propriete in reglages:
		recul.set(propriete, reglages[propriete])
	corps.add_child(recul)
	vue.add_child(corps)
	return corps


func _ennemi(vue: Node, position: Vector2) -> Node2D:
	var e := Formes.carre(vue, position, MAUVAIS, 46)
	var pointes := Polygon2D.new()
	pointes.polygon = PackedVector2Array([Vector2(-23, -23), Vector2(-15, -36), Vector2(-7, -23), Vector2(1, -36), Vector2(9, -23), Vector2(17, -36), Vector2(23, -23)])
	pointes.color = MAUVAIS.darkened(0.3)
	e.add_child(pointes)
	return e


## Lignes : une victime par valeur, sur des plateformes superposées, frappée par un ennemi à sa droite.
func _lignes(propriete: String, valeurs: Array, textes: Array, communs: Dictionary = {}, pousser: bool = false) -> void:
	var vue := monde_2d()
	arene(vue, 2000.0)
	var victimes := []
	for i in valeurs.size():
		var y := -110.0 + i * 110.0
		if i < valeurs.size() - 1:
			bloc(vue, Rect2(-1000, y + 24, 2000, 16))
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		victimes.append(_victime(vue, Vector2(200, y), reglages, VARIANTES[i % 4]))
		_ennemi(vue, Vector2(260, y))
		etiquette(Vector2(1000, 280 + y - 28), textes[i], 16, VARIANTES[i % 4])
	debug(vue)
	for coup in 2:
		for v in victimes:
			v.position.x = 200
			v.velocity = Vector2.ZERO
		await secondes(0.6)
		for v in victimes:
			v.get_node("Recul").apply_knockback_from(v.position + Vector2(60, 0))
			if pousser:
				v.get_node("Sol").move(Vector2.RIGHT)
		await secondes(2.2)
		for v in victimes:
			v.get_node("Sol").move(Vector2.ZERO)


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : repousser le joueur touché")
	var vue := monde_2d()
	arene(vue)
	var heros := _victime(vue, Vector2(-300, 150))
	var ennemi := _ennemi(vue, Vector2(150, 177))
	debug(vue)
	code("func _on_hitbox_body_entered(corps):\n\tvar recul := corps.get_node_or_null(\"Knockback2D\") as Knockback2D\n\tif recul and not recul.is_knocked():\n\t\trecul.apply_knockback_from(global_position)")
	var sol: GroundMovement2D = heros.get_node("Sol")
	var recul: Knockback2D = heros.get_node("Recul")
	for i in 3:
		sol.move(Vector2.RIGHT)
		var t := 0.0
		while t < 3.0:
			await get_tree().physics_frame
			t += get_physics_process_delta_time()
			if absf(heros.position.x - ennemi.position.x) < 40 and not recul.is_knocked():
				legende("Touché : recul à l'opposé de l'ennemi, commandes bloquées %s s" % recul.duration)
				recul.apply_knockback_from(ennemi.global_position)
				break
		sol.move(Vector2.ZERO)
		await secondes(1.0)
		legende("Contrôle repris")


# 💻 API

func _classe_2d() -> void:
	sujet("Knockback2D : personnages et corps rigides")
	var vue := monde_2d()
	arene(vue)
	var heros := _victime(vue, Vector2(-150, 150))
	var caisse := _caisse(vue, Vector2(150, 170))
	var etat := etiquette(Vector2(576, 140), "", 17, ACCENT)
	get_tree().process_frame.connect(func():
		if is_instance_valid(etat):
			etat.text = "Knockback2D.active_on(heros) = %s" % ("Recul" if Knockback2D.active_on(heros) else "null"))
	debug(vue)
	code("$Knockback2D.apply_knockback_from(position_attaquant)")
	for i in 3:
		await secondes(0.8)
		var centre := Vector2(0, 170)
		explosion(vue, centre, 50.0)
		heros.get_node("Recul").apply_knockback_from(centre)
		caisse.get_node("Recul").apply_knockback_from(centre)
		await secondes(1.4)
		heros.position = Vector2(-150, 150)
		heros.velocity = Vector2.ZERO
		caisse.position = Vector2(150, 170)
		caisse.linear_velocity = Vector2.ZERO
		caisse.angular_velocity = 0.0
		caisse.rotation = 0.0


func _classe_3d() -> void:
	sujet("Knockback3D : recul en 3D")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(30, 1, 20))
	var heros := heros_3d(vue, Vector3(-2, 1, 0), {"Sol": "GroundMovement3D", "Recul": "Knockback3D"})
	var caisses: Array[RigidBody3D] = []
	for i in 3:
		var c := RigidBody3D.new()
		c.position = Vector3(2 + i * 0.4, 0.5 + i * 1.05, 0)
		var f := CollisionShape3D.new()
		f.shape = BoxShape3D.new()
		f.shape.size = Vector3.ONE
		c.add_child(f)
		Formes.cube(c, Vector3.ZERO, Color("a0714f"), Vector3.ONE)
		var recul := Knockback3D.new()
		recul.name = "Recul"
		c.add_child(recul)
		vue.add_child(c)
		caisses.append(c)
	debug(vue)
	code("$Knockback3D.apply_knockback_from(point)   # plan horizontal + upward_bias")
	await secondes(1.0)
	var centre := Vector3(0, 0.5, 0)
	Formes.sphere(vue, centre, ACCENT, 0.4)
	heros.get_node("Recul").apply_knockback_from(centre)
	for c in caisses:
		c.get_node("Recul").apply_knockback_from(centre)
	await secondes(3.0)


func _base_force() -> void:
	sujet("base_force : force du recul")
	code("$Knockback2D.base_force = 450.0")
	await _lignes("base_force", [200.0, 450.0, 900.0], ["200", "450 (défaut)", "900"])


func _duration() -> void:
	sujet("duration : commandes bloquées")
	code("$Knockback2D.duration = 0.3")
	await _lignes("duration", [0.1, 0.3, 0.8], ["0,1", "0,3 (défaut)", "0,8"], {}, true)


func _friction() -> void:
	sujet("friction : freinage pendant le recul")
	code("$Knockback2D.friction = 900.0")
	await _lignes("friction", [300.0, 900.0, 3000.0], ["300", "900 (défaut)", "3000"], {"duration": 0.6})


func _upward_bias() -> void:
	sujet("upward_bias : part vers le haut")
	code("$Knockback2D.upward_bias = 0.35")
	await _lignes("upward_bias", [0.0, 0.35, 1.0], ["0", "0,35 (défaut)", "1"])


func _rigid_mode() -> void:
	sujet("rigid_mode : impulsion ou force continue")
	var vue := monde_2d()
	arene(vue)
	var caisses := [_caisse(vue, Vector2(-250, 170), {"rigid_mode": Knockback2D.RIGID_IMPULSE, "add_torque": false}),
		_caisse(vue, Vector2(250, 170), {"rigid_mode": Knockback2D.RIGID_CONTINUOUS, "duration": 0.6, "add_torque": false}, VARIANTES[1].darkened(0.3))]
	etiquette(Vector2(376, 520), "RIGID_IMPULSE (défaut)", 16, VARIANTES[0])
	etiquette(Vector2(826, 520), "RIGID_CONTINUOUS (duration 0,6)", 16, VARIANTES[1])
	debug(vue)
	code("$Knockback2D.rigid_mode = Knockback2D.RIGID_CONTINUOUS")
	for i in 2:
		for j in 2:
			caisses[j].position = Vector2(-250 + j * 500, 170)
			caisses[j].linear_velocity = Vector2.ZERO
		await secondes(0.6)
		for c in caisses:
			c.get_node("Recul").apply_knockback(Vector2.RIGHT)
		await secondes(2.4)


func _add_torque() -> void:
	sujet("add_torque et torque_factor : rotation des corps rigides")
	var vue := monde_2d()
	arene(vue)
	var caisses := [_caisse(vue, Vector2(-250, 170), {"add_torque": false}), _caisse(vue, Vector2(250, 170), {"add_torque": true}, VARIANTES[1].darkened(0.3))]
	etiquette(Vector2(376, 520), "add_torque = false", 16, VARIANTES[0])
	etiquette(Vector2(826, 520), "add_torque = true (torque_factor 0,02)", 16, VARIANTES[1])
	code("$Knockback2D.add_torque = true\n$Knockback2D.torque_factor = 0.02")
	for i in 2:
		for j in 2:
			caisses[j].position = Vector2(-250 + j * 500, 170)
			caisses[j].linear_velocity = Vector2.ZERO
			caisses[j].angular_velocity = 0.0
			caisses[j].rotation = 0.0
		await secondes(0.6)
		for c in caisses:
			c.get_node("Recul").apply_knockback(Vector2(1, -0.3), 1.5)
		await secondes(2.4)


func _enabled() -> void:
	sujet("enabled : ignorer les reculs")
	code("$Knockback2D.enabled = false   # invincible")
	await _lignes("enabled", [true, false], ["enabled = true", "enabled = false"])


func _debug_mode() -> void:
	sujet("debug_mode et get_lock_remaining()")
	var vue := monde_2d()
	arene(vue)
	var heros := _victime(vue, Vector2(0, 150), {"duration": 0.8, "debug_mode": true})
	var recul: Knockback2D = heros.get_node("Recul")
	var reste := etiquette(Vector2(576, 140), "", 18, ACCENT)
	get_tree().process_frame.connect(func():
		if is_instance_valid(reste):
			reste.text = "get_lock_remaining() = %.2f s" % recul.get_lock_remaining())
	code("$Knockback2D.debug_mode = true")
	for i in 3:
		await secondes(0.6)
		recul.apply_knockback(Vector2.LEFT if i % 2 == 0 else Vector2.RIGHT)
		await secondes(1.4)


# 📌 Exemples

func _ex_explosion() -> void:
	sujet("Exemple : repousser tout autour d'une explosion")
	var vue := monde_2d()
	arene(vue)
	var repoussables := []
	for i in 4:
		repoussables.append(_victime(vue, Vector2(-330 + i * 90, 150), {}, VARIANTES[i]))
	for i in 4:
		repoussables.append(_caisse(vue, Vector2(60 + i * 90, 170)))
	var cercle := Node2D.new()
	vue.add_child(cercle)
	code("for corps in get_tree().get_nodes_in_group(\"repoussables\"):\n\tvar d := corps.global_position.distance_to(centre)\n\tif recul and d < rayon:\n\t\trecul.apply_knockback_from(centre, 2.0 * (1.0 - d / rayon))")
	for centre in [Vector2(-150, 180), Vector2(200, 180)]:
		await secondes(0.8)
		cercle.position = centre
		cercle.draw.connect(func(): cercle.draw_arc(Vector2.ZERO, 300, 0, TAU, 48, Color(1, 0.7, 0.3, 0.6), 2.0), CONNECT_ONE_SHOT)
		cercle.queue_redraw()
		explosion(vue, centre, 60.0)
		for corps in repoussables:
			var d: float = corps.global_position.distance_to(centre)
			if d < 300.0:
				corps.get_node("Recul").apply_knockback_from(centre, 2.0 * (1.0 - d / 300.0))
		await secondes(2.4)


func _ex_invincible() -> void:
	sujet("Exemple : invincible pendant le recul")
	var vue := monde_2d()
	arene(vue)
	var heros := _victime(vue, Vector2(-300, 150), {"duration": 0.8})
	var ennemi := _ennemi(vue, Vector2(100, 177))
	var recul: Knockback2D = heros.get_node("Recul")
	var zone := etiquette(Vector2(576, 140), "Hurtbox.monitoring = true", 18, BON)
	recul.knocked_back.connect(func(v: Vector2):
		zone.text = "knocked_back(%s) → Hurtbox.monitoring = false" % v.round()
		zone.add_theme_color_override("font_color", MAUVAIS))
	recul.recovered.connect(func():
		zone.text = "recovered → Hurtbox.monitoring = true"
		zone.add_theme_color_override("font_color", BON))
	debug(vue)
	code("$Knockback2D.knocked_back.connect(func(_v): $Hurtbox.set_deferred(\"monitoring\", false))\n$Knockback2D.recovered.connect(func(): $Hurtbox.set_deferred(\"monitoring\", true))")
	var sol: GroundMovement2D = heros.get_node("Sol")
	for i in 3:
		sol.move(Vector2.RIGHT)
		var t := 0.0
		while t < 3.0:
			await get_tree().physics_frame
			t += get_physics_process_delta_time()
			if absf(heros.position.x - ennemi.position.x) < 40 and not recul.is_knocked():
				recul.apply_knockback_from(ennemi.global_position)
				break
		await secondes(1.2)
	sol.move(Vector2.ZERO)


func _ex_effets() -> void:
	sujet("Exemple : recul, flash et secousse")
	var vue := monde_2d()
	arene(vue)
	var heros := _victime(vue, Vector2(-100, 150), {"duration": 0.5})
	var visuel: Node2D = heros.get_node("Visuel")
	var effets := PlayerFX.new()
	var flash := FXFade.new()
	flash.color = MAUVAIS
	flash.duration = 0.08
	var secousse := FXShake.new()
	secousse.duration = 0.0
	secousse.max_amplitude = 4.0
	effets.special_effects = [flash, secousse]
	visuel.add_child(effets)
	var recul: Knockback2D = heros.get_node("Recul")
	code("$Knockback2D.apply_knockback_from(attaquant)\n$PlayerFX.play()   # flash + secousse\nawait $Knockback2D.recovered\n$PlayerFX.stop()")
	for i in 4:
		await secondes(0.7)
		var attaquant := heros.position + Vector2(50 if i % 2 == 0 else -50, 0)
		explosion(vue, attaquant, 25.0)
		recul.apply_knockback_from(attaquant)
		effets.play()
		await recul.recovered
		effets.stop()


func _ex_topdown() -> void:
	sujet("Exemple : le recul en vue de dessus")
	var vue := vue_2d(Rect2(76, 110, 1000, 440))
	for rect in [Rect2(-480, -210, 960, 20), Rect2(-480, 190, 960, 20), Rect2(-480, -210, 20, 420), Rect2(460, -210, 20, 420)]:
		bloc(vue, rect)
	var heros := heros_2d(vue, Vector2(0, 0), {"Sol": "GroundMovement2D", "Recul": "Knockback2D"}, {"Sol": {"mode": GroundMovement2D.MODE_TOP_DOWN}, "Recul": {"upward_bias": 0.0, "friction": 1500.0}})
	var recul: Knockback2D = heros.get_node("Recul")
	debug(vue)
	code("$Knockback2D.upward_bias = 0.0\n$Knockback2D.friction = 1500.0")
	for direction in [Vector2.LEFT, Vector2(1, 1), Vector2.UP, Vector2(1, -0.5), Vector2(-1, 1)]:
		await secondes(0.6)
		var attaquant: Vector2 = heros.position - direction * 50
		explosion(vue, attaquant, 25.0)
		recul.apply_knockback_from(attaquant, 1.5)
		await secondes(0.8)


func _ex_seul() -> void:
	sujet("Exemple : un PNJ sans composant de mouvement")
	var vue := monde_2d()
	arene(vue)
	var pnj := CharacterBody2D.new()
	pnj.position = Vector2(150, 150)
	var forme := CollisionShape2D.new()
	forme.shape = CapsuleShape2D.new()
	forme.shape.radius = 14.0
	forme.shape.height = 48.0
	pnj.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(Vector2(28, 48))
	dessin.color = VARIANTES[2]
	pnj.add_child(dessin)
	var recul := Knockback2D.new()
	recul.gravity_affect = true
	pnj.add_child(recul)
	vue.add_child(pnj)
	debug(vue)
	code("$PNJ/Knockback2D.gravity_affect = true\n$PNJ/Knockback2D.apply_knockback(Vector2.LEFT, 1.5)")
	for i in 3:
		await secondes(0.8)
		recul.apply_knockback(Vector2.LEFT if i % 2 == 0 else Vector2.RIGHT, 1.5)
		await secondes(1.6)
