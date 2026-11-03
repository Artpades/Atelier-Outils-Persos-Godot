extends "res://🎥Démos de documentation/5.🌀🫗/sequences_mouvement.gd"

## 🧲 Plans filmés de la page Zone de force (mouvement/zone-de-force.html) : ForceZone2D / 3D.


func titre_module() -> String:
	return "🧲 Zone de force · ForceZone2D / 3D"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-ForceZone2D": _classe_2d,
		"classe-ForceZone3D": _classe_3d,
		"ForceZone2D.mode": _mode,
		"ForceZone2D.strength": _strength,
		"ForceZone2D.local_direction": _local_direction,
		"ForceZone2D.falloff": _falloff,
		"ForceZone2D.affect_characters": _affect,
		"ex-aimant": _ex_aimant,
		"ex-rafales": _ex_rafales,
		"ex-planeur": _ex_planeur,
		"ex-boue": _ex_boue,
		"ex-mesure": _ex_mesure,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Un ventilateur (ForceZone2D, direction (0, -1), strength 1800) soulève le personnage et les caisses ; <code>debug_mode</code> montre le champ.",
		"classe-ForceZone2D": "Une zone de vent latéral : les caisses sont poussées directement, le personnage via son composant de mouvement.",
		"classe-ForceZone3D": "Un couloir de vent 3D qui emporte les cubes.",
		"ForceZone2D.mode": "Les quatre modes : DIRECTIONAL (vent), ATTRACT (aimant), VORTEX (tourbillon), DRAG (freinage), avec des caisses.",
		"ForceZone2D.strength": "Ventilateurs à <code>strength</code> 600, 1200 et 1800 : seule une accélération supérieure à la gravité (980) soulève le personnage.",
		"ForceZone2D.local_direction": "Un ventilateur qui tourne : avec <code>local_direction</code>, le vent suit sa rotation.",
		"ForceZone2D.falloff": "Attraction avec <code>falloff</code> NONE, LINEAR et INVERSE_SQUARE (<code>falloff_radius</code> 200).",
		"ForceZone2D.affect_characters": "<code>affect_characters</code> / <code>affect_rigid_bodies</code> : à gauche le vent pousse tout, à droite seulement les caisses.",
		"ex-aimant": "Un aimant (ATTRACT, <code>affect_characters = false</code>) enfant du joueur : les pièces sont attirées, pas le joueur.",
		"ex-rafales": "<code>turbulence = 0,8</code> : l'intensité varie ; puis des rafales franches avec <code>enabled</code>.",
		"ex-planeur": "Un courant modeste (400) soulève fortement un planeur (<code>wind_influence = 3</code>) et peu un marcheur.",
		"ex-boue": "Une flaque de boue (DRAG, strength 4) : les caisses qui la traversent ralentissent.",
		"ex-mesure": "<code>ForceZone2D.get_acceleration_on($Joueur)</code> oriente une écharpe selon le vent subi.",
	}


func reutilisations() -> Dictionary:
	var meme_3d := "Même réglage qu'en 2D, en m/s² (voir la vidéo 2D)."
	var r := {
		"ForceZone2D.direction": "ForceZone2D.local_direction",
		"ForceZone2D.falloff_radius": "ForceZone2D.falloff",
		"ForceZone2D.affect_rigid_bodies": "ForceZone2D.affect_characters",
		"ForceZone2D.turbulence": "ex-rafales",
		"ForceZone2D.turbulence_speed": "ex-rafales",
		"ForceZone2D.enabled": "ex-rafales",
		"ForceZone2D.debug_mode": "mode-emploi",
		"ForceZone2D.compute_acceleration": "ex-mesure",
		"ForceZone2D.get_acceleration_on": "ex-mesure",
	}
	for membre in ["affect_characters", "affect_rigid_bodies", "debug_mode", "direction", "enabled", "falloff", "falloff_radius", "local_direction", "mode",
			"strength", "turbulence", "turbulence_speed", "compute_acceleration", "get_acceleration_on"]:
		r["ForceZone3D." + membre] = [r.get("ForceZone2D." + membre, "ForceZone2D." + membre), meme_3d]
	return r


func _caisse(vue: Node, position: Vector2, cote: float = 30.0, couleur: Color = Color("a0714f")) -> RigidBody2D:
	var corps := RigidBody2D.new()
	corps.position = position
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = Vector2(cote, cote)
	corps.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(Vector2(cote, cote))
	dessin.color = couleur
	corps.add_child(dessin)
	vue.add_child(corps)
	return corps


func _ventilateur(vue: Node, position: Vector2) -> void:
	var socle := Polygon2D.new()
	socle.polygon = PackedVector2Array([Vector2(-50, 0), Vector2(50, 0), Vector2(40, -14), Vector2(-40, -14)])
	socle.position = position
	socle.color = Color("6b7590")
	vue.add_child(socle)


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : un ventilateur qui soulève")
	var vue := monde_2d()
	arene(vue)
	_ventilateur(vue, Vector2(100, 200))
	zone_force(vue, Rect2(30, -350, 140, 550), {"direction": Vector2(0, -1), "strength": 1800.0, "debug_mode": true})
	var heros := heros_2d(vue, Vector2(-350, 150), {"Sol": "GroundMovement2D"})
	code("$ForceZone2D.mode = ForceZone2D.MODE_DIRECTIONAL\n$ForceZone2D.direction = Vector2(0, -1)\n$ForceZone2D.strength = 1800.0")
	for i in 4:
		_caisse(vue, Vector2(220 + i * 40, -200 - i * 40))
	await piloter(heros.get_node("Sol"), [[2.2, Vector2.RIGHT], [1.4, Vector2.ZERO], [1.2, Vector2.RIGHT], [1.4, Vector2.LEFT], [0.6, Vector2.ZERO]])


# 💻 API

func _classe_2d() -> void:
	sujet("ForceZone2D : une force dans une zone")
	var vue := monde_2d()
	arene(vue)
	zone_force(vue, Rect2(-150, -200, 400, 400), {"direction": Vector2.RIGHT, "strength": 500.0, "debug_mode": true})
	var heros := heros_2d(vue, Vector2(-400, 150), {"Sol": "GroundMovement2D"})
	for i in 4:
		_caisse(vue, Vector2(-100 + i * 50, -150))
	code("vent.direction = Vector2.RIGHT\nvent.strength = 500.0")
	await piloter(heros.get_node("Sol"), [[1.5, Vector2.RIGHT], [1.0, Vector2.ZERO], [1.6, Vector2.LEFT], [0.6, Vector2.ZERO, "saut_long"], [1.2, Vector2.ZERO]])


func _classe_3d() -> void:
	sujet("ForceZone3D : un couloir de vent")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(30, 1, 12))
	var zone := ForceZone3D.new()
	zone.direction = Vector3(1, 0, 0)
	zone.strength = 12.0
	zone.debug_mode = true
	zone.position = Vector3(0, 1.5, 0)
	var forme := CollisionShape3D.new()
	forme.shape = BoxShape3D.new()
	forme.shape.size = Vector3(10, 3, 4)
	zone.add_child(forme)
	vue.add_child(zone)
	code("$ForceZone3D.direction = Vector3(1, 0, 0)\n$ForceZone3D.strength = 12.0")
	for i in 6:
		var c := RigidBody3D.new()
		c.position = Vector3(-6 + randf_range(-0.5, 0.5), 2 + i, randf_range(-1, 1))
		var f := CollisionShape3D.new()
		f.shape = BoxShape3D.new()
		f.shape.size = Vector3.ONE * 0.6
		c.add_child(f)
		Formes.cube(c, Vector3.ZERO, Color("a0714f").lightened(i * 0.05), Vector3.ONE * 0.6)
		vue.add_child(c)
		await secondes(0.5)
	await secondes(3.0)


func _mode() -> void:
	sujet("mode : quatre types de force")
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.8)
	arene(vue, 1400)
	var modes := [ForceZone2D.MODE_DIRECTIONAL, ForceZone2D.MODE_ATTRACT, ForceZone2D.MODE_VORTEX, ForceZone2D.MODE_DRAG]
	var noms := ["DIRECTIONAL", "ATTRACT", "VORTEX", "DRAG"]
	for i in 4:
		var x := -570.0 + i * 300.0
		var r := {"mode": modes[i], "debug_mode": true, "strength": [700.0, 2500.0, 1500.0, 4.0][i], "direction": Vector2(1, -1)}
		zone_force(vue, Rect2(x, -250, 240, 300), r)
		etiquette(Vector2(576 + (x + 120) * 0.8, 545), noms[i], 16, VARIANTES[i])
		for j in 3:
			_caisse(vue, Vector2(x + 60 + j * 60, -300 - j * 30), 24, VARIANTES[i].darkened(0.2))
	code("$ForceZone2D.mode = ForceZone2D.MODE_ATTRACT")
	await secondes(6.0)


func _strength() -> void:
	sujet("strength : intensité de l'accélération")
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.8)
	arene(vue, 1400)
	var heros := []
	for i in 3:
		var x := -450.0 + i * 400.0
		_ventilateur(vue, Vector2(x, 200))
		zone_force(vue, Rect2(x - 70, -350, 140, 550), {"direction": Vector2.UP, "strength": [600.0, 1200.0, 1800.0][i]})
		heros.append(heros_2d(vue, Vector2(x, -50), {"Sol": "GroundMovement2D"}, {}, VARIANTES[i]))
		etiquette(Vector2(576 + x * 0.8, 545), ["600 : ralentit la chute", "1200 : soulève", "1800"][i], 16, VARIANTES[i])
	code("$ForceZone2D.strength = 1200.0   # > gravité (980) : soulève")
	await secondes(6.0)


func _local_direction() -> void:
	sujet("direction et local_direction : un ventilateur qui tourne")
	var vue := monde_2d()
	arene(vue)
	var zone := zone_force(vue, Rect2(-180, -180, 360, 360), {"direction": Vector2.RIGHT, "strength": 700.0, "local_direction": true, "debug_mode": true})
	for i in 8:
		_caisse(vue, Vector2(-120 + (i % 4) * 70, -100 + (i / 4) * 70))
	code("$Ventilateur.direction = Vector2.RIGHT\n$Ventilateur.local_direction = true   # suit la rotation")
	var t := 0.0
	while t < 7.0:
		await get_tree().physics_frame
		t += get_physics_process_delta_time()
		zone.rotation = t * 0.9


func _falloff() -> void:
	sujet("falloff et falloff_radius : atténuation")
	var vue := monde_2d(Rect2(76, 110, 1000, 440), 0.8)
	arene(vue, 1400)
	for i in 3:
		var x := -450.0 + i * 450.0
		zone_force(vue, Rect2(x - 200, -300, 400, 450), {"mode": ForceZone2D.MODE_ATTRACT, "strength": 1500.0, "falloff": i, "falloff_radius": 200.0, "debug_mode": true})
		etiquette(Vector2(576 + x * 0.8, 545), ["FALLOFF_NONE", "FALLOFF_LINEAR", "FALLOFF_INVERSE_SQUARE"][i], 16, VARIANTES[i])
		for j in 4:
			_caisse(vue, Vector2(x - 150 + j * 100, -280), 22, VARIANTES[i].darkened(0.2))
	code("aimant.falloff = ForceZone2D.FALLOFF_LINEAR\naimant.falloff_radius = 200.0")
	await secondes(6.0)


func _affect() -> void:
	sujet("affect_characters et affect_rigid_bodies")
	var vue := monde_2d()
	arene(vue)
	for i in 2:
		var x := -250.0 + i * 500.0
		zone_force(vue, Rect2(x - 200, -200, 400, 400), {"direction": Vector2.RIGHT, "strength": 600.0, "affect_characters": i == 0})
		heros_2d(vue, Vector2(x - 120, 150), {"Sol": "GroundMovement2D"}, {}, VARIANTES[i])
		for j in 3:
			_caisse(vue, Vector2(x - 60 + j * 40, -150))
		etiquette(Vector2(576 + x, 545), ["tout est poussé", "affect_characters = false"][i], 16, VARIANTES[i])
	code("$Vent.affect_characters = false   # seulement les corps rigides")
	await secondes(5.0)


# 📌 Exemples

func _ex_aimant() -> void:
	sujet("Exemple : un aimant à pièces")
	var vue := monde_2d()
	arene(vue)
	var heros := heros_2d(vue, Vector2(-450, 150), {"Sol": "GroundMovement2D"})
	var aimant := ForceZone2D.new()
	var forme := CollisionShape2D.new()
	forme.shape = CircleShape2D.new()
	forme.shape.radius = 150.0
	aimant.add_child(forme)
	aimant.mode = ForceZone2D.MODE_ATTRACT
	aimant.strength = 2500.0
	aimant.affect_characters = false
	aimant.debug_mode = true
	heros.add_child(aimant)
	for i in 8:
		var piece := _caisse(vue, Vector2(-250 + i * 80, 170), 16, Color("ffd166"))
		piece.gravity_scale = 0.4
	code("aimant.mode = ForceZone2D.MODE_ATTRACT\naimant.strength = 2500.0\naimant.affect_characters = false\nadd_child(aimant)   # enfant du joueur")
	await piloter(heros.get_node("Sol"), [[3.5, Vector2.RIGHT], [0.8, Vector2.ZERO], [2.0, Vector2.LEFT]])


func _ex_rafales() -> void:
	sujet("Exemple : un vent par rafales")
	var vue := monde_2d()
	arene(vue)
	var vent := zone_force(vue, Rect2(-500, -200, 1000, 400), {"direction": Vector2.RIGHT, "strength": 350.0, "turbulence": 0.8, "turbulence_speed": 2.0, "debug_mode": true})
	var heros := heros_2d(vue, Vector2(0, 150), {"Sol": "GroundMovement2D"})
	for i in 4:
		_caisse(vue, Vector2(-300 + i * 50, 170))
	code("$Vent.turbulence = 0.8\n$Vent.turbulence_speed = 2.0")
	legende("turbulence : l'intensité varie de 0,2 à 1,8 fois")
	heros.get_node("Sol").move(Vector2.LEFT)
	await secondes(3.5)
	code("func _on_timer_timeout() -> void:\n\t$Vent.enabled = not $Vent.enabled")
	legende("Rafales franches : enabled basculé par un minuteur")
	vent.turbulence = 0.0
	for i in 6:
		vent.enabled = not vent.enabled
		await secondes(0.6)
	heros.get_node("Sol").move(Vector2.ZERO)


func _ex_planeur() -> void:
	sujet("Exemple : un courant ascendant pour un planeur")
	var vue := monde_2d()
	arene(vue)
	zone_force(vue, Rect2(-80, -400, 260, 600), {"direction": Vector2.UP, "strength": 400.0, "debug_mode": true})
	var marcheur := heros_2d(vue, Vector2(-450, 150), {"Sol": "GroundMovement2D"}, {}, VARIANTES[1])
	var planeur := heros_2d(vue, Vector2(-350, 150), {"Sol": "GroundMovement2D", "Aero": "AerodynamicMovement2D", "Directeur": "MovementDirector"}, {"Aero": {"wind_influence": 3.0}})
	code("$Courant.direction = Vector2.UP\n$Courant.strength = 400.0\n$Joueur/AerodynamicMovement2D.wind_influence = 3.0")
	piloter(marcheur.get_node("Sol"), [[1.5, Vector2.RIGHT], [0.4, Vector2.RIGHT, "saut_long"], [3.0, Vector2.ZERO]])
	await piloter(planeur.get_node("Directeur"), [[1.2, Vector2.RIGHT], [4.0, Vector2(0.15, 0), "saut_long"], [1.0, Vector2.ZERO]])


func _ex_boue() -> void:
	sujet("Exemple : une flaque de boue qui ralentit")
	var vue := monde_2d()
	arene(vue)
	var boue := zone_force(vue, Rect2(-100, 120, 300, 80), {"mode": ForceZone2D.MODE_DRAG, "strength": 4.0})
	boue.get_child(1).color = Color(0.45, 0.32, 0.2, 0.6)
	code("$Boue.mode = ForceZone2D.MODE_DRAG\n$Boue.strength = 4.0")
	for vague in 3:
		for i in 3:
			var c := _caisse(vue, Vector2(-450, 150 - i * 40))
			c.linear_velocity = Vector2(600 + i * 80, 0)
		await secondes(2.2)


func _ex_mesure() -> void:
	sujet("Exemple : lire la force subie")
	var vue := monde_2d()
	arene(vue)
	zone_force(vue, Rect2(-100, -300, 300, 500), {"direction": Vector2(1, -0.5), "strength": 500.0})
	var heros := heros_2d(vue, Vector2(-400, 150), {"Sol": "GroundMovement2D"})
	var echarpe := Line2D.new()
	echarpe.points = PackedVector2Array([Vector2.ZERO, Vector2(-40, 0)])
	echarpe.width = 6
	echarpe.default_color = MAUVAIS
	echarpe.position = Vector2(0, -14)
	heros.add_child(echarpe)
	var valeur := etiquette(Vector2(576, 140), "", 18, ACCENT)
	get_tree().physics_frame.connect(func():
		if is_instance_valid(echarpe):
			var vent := ForceZone2D.get_acceleration_on(heros)
			echarpe.rotation = vent.angle() + PI if vent.length() > 10.0 else 0.0
			valeur.text = "get_acceleration_on($Joueur) = %s" % vent.round())
	code("var vent := ForceZone2D.get_acceleration_on($Joueur)\n$Echarpe.rotation = vent.angle() if vent.length() > 10.0 else 0.0")
	await piloter(heros.get_node("Sol"), [[2.0, Vector2.RIGHT], [1.0, Vector2.ZERO], [1.5, Vector2.RIGHT], [2.0, Vector2.LEFT]])
