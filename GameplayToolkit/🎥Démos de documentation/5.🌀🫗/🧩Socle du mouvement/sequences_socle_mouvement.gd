extends "res://🎥Démos de documentation/5.🌀🫗/sequences_mouvement.gd"

## 🧩 Plans filmés de la page Socle du mouvement (mouvement/socle-mouvement.html) : MovementComponent.


## Composant écrit en GDScript : une échelle (tutoriel B du Mode d'emploi).
class LadderMovement2D extends MovementComponent:
	var vitesse := 120.0

	func _movement_step(_delta: float) -> void:
		var corps := get_body() as CharacterBody2D
		corps.velocity = Vector2(get_move_input().x * 40.0, get_move_input().y * vitesse)
		if has_jump_request(true):
			corps.velocity.y = -300.0
			active = false
		corps.move_and_slide()


func titre_module() -> String:
	return "🧩 Socle du mouvement · MovementComponent"


func plans() -> Dictionary:
	return {
		"mode-emploi-tuto-composant": _tuto_composant,
		"mode-emploi-tuto-script": _tuto_script,
		"classe-MovementComponent": _classe,
		"MovementComponent.active": _active,
		"MovementComponent.debug_mode": _debug_mode,
		"MovementComponent.release_jump": _release_jump,
		"MovementComponent.set_running": _set_running,
		"ex-ia": _ex_ia,
		"ex-buffer": _ex_buffer,
		"ex-vent": _ex_vent,
		"ex-signaux": _ex_signaux,
		"ex-input": _ex_input,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi-tuto-composant": "Un GroundMovement2D sous le CharacterBody2D : le script n'envoie que l'intention (<code>move</code>, <code>jump</code>) ; avec <code>debug_mode</code>, flèche verte = vitesse, jaune = intention.",
		"mode-emploi-tuto-script": "<code>LadderMovement2D</code>, composant écrit en GDScript (<code>_movement_step</code>) : activé dans la zone de l'échelle, il fait monter le personnage ; un saut le quitte.",
		"classe-MovementComponent": "Les composants reçoivent tous la même intention ; seul le composant actif déplace le corps (ici le sol, puis la nage dans l'eau).",
		"MovementComponent.active": "Même intention envoyée aux deux : le composant inactif ne déplace pas son corps.",
		"MovementComponent.debug_mode": "<code>debug_mode</code> : flèches de vitesse (vert) et d'intention (jaune), nom et état du composant.",
		"MovementComponent.release_jump": "Saut maintenu (haut) et saut relâché aussitôt (bas) : <code>release_jump</code> coupe la montée (<code>jump_cut</code>).",
		"MovementComponent.set_running": "<code>set_running(true)</code> : la vitesse est multipliée par <code>run_multiplier</code>.",
		"ex-ia": "Une IA qui patrouille avec le même composant que le joueur : elle saute ou fait demi-tour contre un mur.",
		"ex-buffer": "Saut demandé 0,15 s avant d'atterrir : refusé sans tampon, accepté avec <code>jump_buffer_time = 0.2</code>.",
		"ex-vent": "Un composant maison qui ajoute <code>external_acceleration_2d(corps)</code> : la zone de vent le pousse.",
		"ex-signaux": "<code>activated</code> / <code>deactivated</code> de la nage : l'animation (ici la couleur) change en entrant et en sortant de l'eau.",
		"ex-input": "L'intention est envoyée au MovementDirector, qui la relaie à tous les composants.",
	}


func schemas() -> Dictionary:
	return {
		"figure-1": ["arbre.svg", "Un personnage 2D complet : le CharacterBody2D porte ses composants de mouvement et un MovementDirector ; avec <code>debug_mode</code>, flèches de vitesse et d'intention et texte d'état."],
		"figure-2": ["installation.svg", "Le dossier <code>bin/movement_physics/</code>, puis les composants dans « Ajouter un nœud » (MovementComponent, abstraite, est grisée)."],
		"intention": ["intention.svg", "L'intention reçue est stockée par la base : <code>move</code>, <code>set_vertical_input</code>, <code>jump</code> / <code>release_jump</code>, <code>set_running</code> ; le composant la relit avec les accesseurs."],
	}


func reutilisations() -> Dictionary:
	return {
		"MovementComponent.jump_buffer_time": "ex-buffer",
		"MovementComponent._movement_step": "mode-emploi-tuto-script",
		"MovementComponent.external_acceleration_2d": "ex-vent",
		"MovementComponent.external_acceleration_3d": ["ex-vent", "Même principe en 3D avec les ForceZone3D."],
		"MovementComponent.move": "mode-emploi-tuto-composant",
		"MovementComponent.jump": "mode-emploi-tuto-composant",
		"MovementComponent.get_body": "intention",
		"MovementComponent.get_move_input": "intention",
		"MovementComponent.get_vertical_input": "intention",
		"MovementComponent.set_vertical_input": "intention",
		"MovementComponent.has_jump_request": "intention",
		"MovementComponent.is_jump_held": "intention",
		"MovementComponent.is_running": "intention",
		"MovementComponent.activated": "ex-signaux",
		"MovementComponent.deactivated": "ex-signaux",
	}


# ⚙️ Mode d'emploi

func _tuto_composant() -> void:
	sujet("A. Faire marcher un personnage avec un composant")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(-150, 110, 220, 24))
	var heros := heros_2d(vue, Vector2(-350, 150), {"Sol": "GroundMovement2D"})
	debug(vue)
	code("sol.move(Vector2(Input.get_axis(\"ui_left\", \"ui_right\"), 0))\nif Input.is_action_just_pressed(\"ui_accept\"): sol.jump()\nelif Input.is_action_just_released(\"ui_accept\"): sol.release_jump()")
	await secondes(0.5)
	await piloter(heros.get_node("Sol"), [[1.2, Vector2.RIGHT], [0.6, Vector2.RIGHT, "saut_long"], [1.0, Vector2.RIGHT], [0.5, Vector2.ZERO],
		[1.0, Vector2.LEFT], [0.6, Vector2.LEFT, "saut_court"], [1.2, Vector2.LEFT], [0.8, Vector2.ZERO]])


func _tuto_script() -> void:
	sujet("B. Écrire son propre mode de déplacement (échelle)")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(60, -120, 400, 24))
	var echelle := Node2D.new()
	echelle.position = Vector2(0, 40)
	echelle.draw.connect(func():
		for x in [-18.0, 18.0]:
			echelle.draw_line(Vector2(x, -170), Vector2(x, 160), Color("8a5a3c"), 4.0)
		for y in range(-160, 160, 24):
			echelle.draw_line(Vector2(-18, y), Vector2(18, y), Color("8a5a3c"), 3.0))
	vue.add_child(echelle)
	var heros := heros_2d(vue, Vector2(-300, 150), {"Sol": "GroundMovement2D"})
	var echelon := LadderMovement2D.new()
	echelon.name = "Echelle"
	echelon.active = false
	heros.add_child(echelon)
	var sol: MovementComponent = heros.get_node("Sol")
	debug(vue)
	code("class_name LadderMovement2D\nextends MovementComponent\n\nfunc _movement_step(delta):\n\tcorps.velocity = Vector2(entree.x * 40, entree.y * vitesse)\n\tif has_jump_request(true): active = false")
	legende("Au sol : GroundMovement2D actif")
	await piloter(sol, [[1.4, Vector2.RIGHT]])
	legende("Dans la zone de l'échelle : sol.active = false, echelle.active = true")
	sol.active = false
	echelon.active = true
	for composant in [sol, echelon]:
		composant.move(Vector2.ZERO)
	await piloter(echelon, [[1.8, Vector2.UP]])
	legende("Saut : l'échelle se désactive elle-même, le sol reprend")
	echelon.jump()
	await secondes(0.1)
	sol.active = true
	await piloter(sol, [[1.5, Vector2.RIGHT], [0.6, Vector2.ZERO]])


# 💻 API

func _classe() -> void:
	sujet("MovementComponent : une intention, un composant actif")
	var vue := monde_2d()
	arene(vue)
	eau(vue, Rect2(-100, 40, 400, 160))
	bloc(vue, Rect2(-100, 200, 400, 120), Formes.SOL)
	var heros := heros_2d(vue, Vector2(-500, 150), {"Sol": "GroundMovement2D", "Nage": "SwimMovement2D", "Directeur": "MovementDirector"})
	debug(vue)
	var etat := etiquette(Vector2(576, 140), "", 18, ACCENT)
	var directeur: MovementDirector = heros.get_node("Directeur")
	get_tree().process_frame.connect(func():
		if is_instance_valid(etat):
			etat.text = "actif : %s" % ("Sol" if heros.get_node("Sol").active else "Nage"))
	code("$MovementDirector.move(direction)   # relayé à tous les composants")
	await secondes(0.5)
	await piloter(directeur, [[1.6, Vector2.RIGHT], [1.6, Vector2(1, 0.3)], [0.6, Vector2.RIGHT, "saut"], [1.4, Vector2.RIGHT], [0.6, Vector2.ZERO]])


func _active() -> void:
	sujet("active : le composant agit-il ?")
	var vue := monde_2d()
	arene(vue)
	var actif := heros_2d(vue, Vector2(-500, 60), {"Sol": "GroundMovement2D"})
	var inactif := heros_2d(vue, Vector2(-500, 150), {"Sol": "GroundMovement2D"}, {"Sol": {"active": false}}, VARIANTES[2])
	bloc(vue, Rect2(-800, 90, 1600, 20), Color(0, 0, 0, 0))
	etiquette(Vector2(576, 290), "active = true", 18, VARIANTES[0])
	etiquette(Vector2(576, 420), "active = false (intention reçue, aucun mouvement)", 18, VARIANTES[2])
	code("$Sol.active = false")
	for i in 3:
		for corps in [actif, inactif]:
			corps.get_node("Sol").move(Vector2.RIGHT if i % 2 == 0 else Vector2.LEFT)
		await secondes(1.5)
	for corps in [actif, inactif]:
		corps.get_node("Sol").move(Vector2.ZERO)
	await secondes(0.5)


func _debug_mode() -> void:
	sujet("debug_mode : gizmos du composant")
	var vue := monde_2d()
	arene(vue)
	var heros := heros_2d(vue, Vector2(-400, 150), {"Sol": "GroundMovement2D"}, {"Sol": {"max_jumps": 2}})
	var sol: MovementComponent = heros.get_node("Sol")
	code("$Sol.debug_mode = true")
	legende("debug_mode = false")
	await piloter(sol, [[1.0, Vector2.RIGHT], [0.6, Vector2.RIGHT, "saut_long"]])
	sol.debug_mode = true
	legende("debug_mode = true : vitesse (vert), intention (jaune), état")
	await piloter(sol, [[1.0, Vector2.LEFT], [0.5, Vector2.LEFT, "saut_long"], [0.5, Vector2.LEFT, "saut_long"], [1.2, Vector2.ZERO], [1.0, Vector2.RIGHT]])


func _release_jump() -> void:
	sujet("jump() puis release_jump() : hauteur de saut variable")
	var vue := monde_2d()
	arene(vue)
	var haut := heros_2d(vue, Vector2(-200, 150), {"Sol": "GroundMovement2D"})
	var bas := heros_2d(vue, Vector2(200, 150), {"Sol": "GroundMovement2D"}, {}, VARIANTES[2])
	etiquette(Vector2(376, 520), "jump() maintenu", 18, VARIANTES[0])
	etiquette(Vector2(776, 520), "jump() puis release_jump() aussitôt", 18, VARIANTES[2])
	code("sol.jump()\n# ... relâché tôt :\nsol.release_jump()   # vitesse × jump_cut")
	for i in 3:
		await secondes(0.6)
		haut.get_node("Sol").jump()
		bas.get_node("Sol").jump()
		await secondes(0.08)
		bas.get_node("Sol").release_jump()
		await secondes(0.8)
		haut.get_node("Sol").release_jump()
		await secondes(0.6)


func _set_running() -> void:
	sujet("set_running() : course")
	var vue := monde_2d()
	arene(vue)
	var marche := heros_2d(vue, Vector2(-700, 60), {"Sol": "GroundMovement2D"})
	var course := heros_2d(vue, Vector2(-700, 150), {"Sol": "GroundMovement2D"}, {}, VARIANTES[1])
	bloc(vue, Rect2(-800, 90, 1600, 20))
	course.get_node("Sol").set_running(true)
	etiquette(Vector2(576, 290), "marche (max_speed 200)", 18, VARIANTES[0])
	etiquette(Vector2(576, 420), "set_running(true) : × run_multiplier (1,6)", 18, VARIANTES[1])
	code("sol.set_running(true)")
	for corps in [marche, course]:
		corps.get_node("Sol").move(Vector2.RIGHT)
	await secondes(5.0)


# 📌 Exemples

func _ex_ia() -> void:
	sujet("Exemple : une IA avec les mêmes composants")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(150, 140, 60, 60))
	bloc(vue, Rect2(-420, 120, 60, 80))
	var ennemi := heros_2d(vue, Vector2(0, 150), {"Sol": "GroundMovement2D"}, {}, MAUVAIS.lightened(0.2))
	var sol: GroundMovement2D = ennemi.get_node("Sol")
	debug(vue)
	code("if is_on_wall():\n\tif sol.is_on_floor() and randf() < 0.5: sol.jump()\n\telse: sens = -sens\nsol.move(Vector2(sens, 0))")
	var sens := 1.0
	var aleatoire := RandomNumberGenerator.new()
	aleatoire.seed = 4
	var t := 0.0
	while t < 10.0:
		await get_tree().physics_frame
		t += get_physics_process_delta_time()
		if ennemi.is_on_wall():
			if sol.is_on_floor() and aleatoire.randf() < 0.5:
				sol.jump()
			else:
				sens = -sens
		sol.move(Vector2(sens, 0))


func _ex_buffer() -> void:
	sujet("Exemple : un saut plus tolérant (jump_buffer_time)")
	var vue := monde_2d()
	arene(vue)
	var sans := heros_2d(vue, Vector2(-250, -150), {"Sol": "GroundMovement2D"}, {"Sol": {"jump_buffer_time": 0.0}}, VARIANTES[2])
	var avec := heros_2d(vue, Vector2(250, -150), {"Sol": "GroundMovement2D"}, {"Sol": {"jump_buffer_time": 0.2}})
	etiquette(Vector2(326, 520), "jump_buffer_time = 0 : appui perdu", 18, VARIANTES[2])
	etiquette(Vector2(826, 520), "jump_buffer_time = 0,2 : saut à l'atterrissage", 18, VARIANTES[0])
	code("$GroundMovement2D.jump_buffer_time = 0.2\n$GroundMovement2D.coyote_time = 0.15")
	for i in 3:
		for corps in [sans, avec]:
			corps.position.y = -150
			corps.velocity = Vector2.ZERO
		# Chute : appui 0,15 s avant de toucher le sol
		var t := 0.0
		var appuye := false
		while t < 1.6:
			await get_tree().physics_frame
			t += get_physics_process_delta_time()
			if not appuye and avec.position.y > 120:
				appuye = true
				legende("Saut demandé juste avant l'atterrissage")
				for corps in [sans, avec]:
					corps.get_node("Sol").jump()
		for corps in [sans, avec]:
			corps.get_node("Sol").release_jump()
		await secondes(0.5)
		legende("")


## Composant maison qui subit les zones de force (ex-vent).
class Planeur2D extends MovementComponent:
	func _movement_step(delta: float) -> void:
		var corps := get_body() as CharacterBody2D
		corps.velocity += MovementComponent.external_acceleration_2d(corps) * delta
		corps.velocity.x = move_toward(corps.velocity.x, get_move_input().x * 150.0, 400.0 * delta)
		corps.velocity.y = minf(corps.velocity.y + 420.0 * delta, 400.0)
		corps.move_and_slide()


func _ex_vent() -> void:
	sujet("Exemple : un composant GDScript qui subit le vent")
	var vue := monde_2d()
	arene(vue)
	zone_force(vue, Rect2(-20, -200, 240, 400), {"direction": Vector2.UP, "strength": 520.0})
	var heros := CharacterBody2D.new()
	heros.position = Vector2(-450, 100)
	var collision := CollisionShape2D.new()
	collision.shape = CircleShape2D.new()
	collision.shape.radius = 16.0
	heros.add_child(collision)
	var visuel := Polygon2D.new()
	visuel.polygon = Formes.rectangle(Vector2(30, 30))
	visuel.color = ACCENT
	heros.add_child(visuel)
	vue.add_child(heros)
	var planeur := Planeur2D.new()
	heros.add_child(planeur)
	debug(vue)
	code("corps.velocity += MovementComponent.external_acceleration_2d(corps) * delta\ncorps.move_and_slide()")
	legende("La zone de vent (ForceZone2D) pousse vers le haut")
	await piloter(planeur, [[3.0, Vector2.RIGHT], [2.5, Vector2.LEFT], [2.0, Vector2.RIGHT]])


func _ex_signaux() -> void:
	sujet("Exemple : changer d'animation quand un composant prend la main")
	var vue := monde_2d()
	arene(vue)
	eau(vue, Rect2(-100, 40, 400, 160))
	bloc(vue, Rect2(-100, 200, 400, 120))
	var heros := heros_2d(vue, Vector2(-500, 150), {"Sol": "GroundMovement2D", "Nage": "SwimMovement2D", "Directeur": "MovementDirector"})
	var animation := etiquette(Vector2(576, 140), "animation : marche", 22, BON)
	var nage: MovementComponent = heros.get_node("Nage")
	nage.activated.connect(func():
		animation.text = "activated → animation : nage"
		heros.get_node("Visuel").modulate = Color("9fd8ff"))
	nage.deactivated.connect(func():
		animation.text = "deactivated → animation : marche"
		heros.get_node("Visuel").modulate = Color.WHITE)
	code("$SwimMovement2D.activated.connect(func(): $AnimationPlayer.play(\"nage\"))\n$SwimMovement2D.deactivated.connect(func(): $AnimationPlayer.play(\"marche\"))")
	await secondes(0.5)
	await piloter(heros.get_node("Directeur"), [[1.6, Vector2.RIGHT], [1.4, Vector2.RIGHT], [0.6, Vector2.RIGHT, "saut"], [1.6, Vector2.RIGHT], [0.5, Vector2.ZERO]])


func _ex_input() -> void:
	sujet("Exemple : relier les composants aux entrées via le directeur")
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(-100, 100, 200, 24))
	var heros := heros_2d(vue, Vector2(-500, 150), {"Sol": "GroundMovement2D", "Directeur": "MovementDirector"})
	debug(vue)
	var touches := etiquette(Vector2(576, 140), "", 22, ACCENT)
	code("d.move(Input.get_vector(\"ui_left\", \"ui_right\", \"ui_up\", \"ui_down\"))\nd.set_running(Input.is_key_pressed(KEY_SHIFT))\nif Input.is_action_just_pressed(\"ui_accept\"): d.jump()")
	var directeur: MovementDirector = heros.get_node("Directeur")
	for etape in [["→", 1.0, Vector2.RIGHT, ""], ["→ + Maj (course)", 1.0, Vector2.RIGHT, "course"], ["→ + Espace", 0.7, Vector2.RIGHT, "saut_long"],
			["←", 1.2, Vector2.LEFT, ""], ["← + Espace", 0.7, Vector2.LEFT, "saut_long"], ["", 0.6, Vector2.ZERO, ""]]:
		touches.text = etape[0]
		await piloter(directeur, [[etape[1], etape[2], etape[3]]])
