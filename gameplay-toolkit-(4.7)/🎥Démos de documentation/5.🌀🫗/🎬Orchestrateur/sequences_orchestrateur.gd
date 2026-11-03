extends "res://🎥Démos de documentation/5.🌀🫗/sequences_mouvement.gd"

## 🎬 Plans filmés de la page Orchestrateur de mouvement (mouvement/orchestrateur-de-mouvement.html).


func titre_module() -> String:
	return "🎬 Orchestrateur · MovementDirector, MovementContext"


func plans() -> Dictionary:
	return {
		"mode-emploi-tuto-auto": _tuto_auto,
		"classe-MovementDirector": _classe_director,
		"classe-MovementContext": _classe_context,
		"MovementDirector.auto_switch": _auto_switch,
		"MovementContext.priority": _priority,
		"ex-code": _ex_code,
		"ex-vol": _ex_vol,
		"ex-expression": _ex_expression,
		"ex-animation": _ex_animation,
		"ex-recul": _ex_recul,
		"ex-env": _ex_env,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi-tuto-auto": "Contextes créés automatiquement (eau 50, mur 40, sol 0) : le directeur active la nage dans l'eau, le mur contre une paroi, le sol ailleurs.",
		"classe-MovementDirector": "Le directeur reçoit l'intention, la relaie à tous les composants et n'active que ceux du contexte courant.",
		"classe-MovementContext": "Un contexte : un nom, une condition, une priorité et ses composants ; parmi les contextes vrais, le plus prioritaire l'emporte.",
		"MovementDirector.auto_switch": "<code>auto_switch = false</code> : le contexte ne change plus seul, même dans l'eau ; seul <code>set_context</code> le change.",
		"MovementContext.priority": "Dans l'eau contre un mur : avec eau 50 / mur 40 la nage l'emporte ; en inversant les priorités, le mur l'emporte.",
		"ex-code": "Contextes « sol » et « air » créés par code, affectés avant <code>add_child</code> : l'air (priorité 10) prend la main pendant les sauts.",
		"ex-vol": "Contexte « vol » en Manual : <code>set_context(&\"vol\")</code> le force, <code>clear_forced_context()</code> rend la main à l'automatique.",
		"ex-expression": "Condition Custom : « glissade » quand on court vite au sol (expression sur <code>body</code> et <code>director</code>).",
		"ex-animation": "<code>context_changed(avant, après)</code> : l'animation (ici la couleur) suit le contexte.",
		"ex-recul": "Contexte « recul » (CONDITION_KNOCKED, priorité 100) sans composant : pendant le recul, le Knockback2D déplace seul le personnage.",
		"ex-env": "<code>get_environment()</code> affiché en direct : sol, eau, mur, corde, recul et contexte.",
	}


func schemas() -> Dictionary:
	return {
		"figure-2": ["inspecteur.svg", "Inspecteur d'un MovementDirector avec trois MovementContext : sol (Always, 0), eau (In Water, 50) et mur (On Wall, 40), et leurs composants."],
		"conditions": ["conditions.svg", "Les conditions d'un MovementContext et ce qu'elles testent ; le directeur choisit, parmi les contextes vrais, celui de plus haute priorité."],
		"choix": ["choix.svg", "À chaque image physique, <code>update_context()</code> : contexte forcé, sinon le plus prioritaire dont la condition est vraie ; ses composants sont activés, les autres désactivés."],
	}


func reutilisations() -> Dictionary:
	return {
		"MovementDirector.contexts": "classe-MovementContext",
		"MovementDirector.auto_configure": "mode-emploi-tuto-auto",
		"MovementDirector.create_default_contexts": "mode-emploi-tuto-auto",
		"MovementDirector.debug_mode": "mode-emploi-tuto-auto",
		"MovementDirector.set_context": "ex-vol",
		"MovementDirector.clear_forced_context": "ex-vol",
		"MovementDirector.get_forced_context": "ex-vol",
		"MovementDirector.get_current_context": "ex-animation",
		"MovementDirector.context_changed": "ex-animation",
		"MovementDirector.get_environment": "ex-env",
		"MovementDirector.is_on_floor": "ex-env",
		"MovementDirector.is_in_air": "ex-env",
		"MovementDirector.is_in_water": "ex-env",
		"MovementDirector.is_on_wall": "ex-env",
		"MovementDirector.is_on_rope": "ex-env",
		"MovementDirector.is_knocked": "ex-recul",
		"MovementDirector.move": "classe-MovementDirector",
		"MovementDirector.jump": "classe-MovementDirector",
		"MovementDirector.release_jump": "classe-MovementDirector",
		"MovementDirector.set_running": "ex-expression",
		"MovementDirector.set_vertical_input": "classe-MovementDirector",
		"MovementDirector.get_move_input": "classe-MovementDirector",
		"MovementDirector.is_jump_held": "classe-MovementDirector",
		"MovementDirector.get_body": "choix",
		"MovementDirector.get_components": "choix",
		"MovementDirector.get_context": "choix",
		"MovementDirector.update_context": "choix",
		"MovementDirector.is_condition_met": "conditions",
		"MovementContext.condition": "conditions",
		"MovementContext.context_name": "classe-MovementContext",
		"MovementContext.components": "classe-MovementContext",
		"MovementContext.expression": "ex-expression",
		"MovementContext.evaluate_expression": "ex-expression",
	}


## Niveau type : sol, bassin, mur haut ; renvoie la vue.
func _niveau() -> SubViewport:
	var vue := monde_2d()
	arene(vue)
	bloc(vue, Rect2(-120, 200, 360, 200), Formes.SOL.darkened(0.2))
	eau(vue, Rect2(-120, 60, 360, 140))
	bloc(vue, Rect2(-120, 200, 360, 30))
	bloc(vue, Rect2(400, -300, 40, 440))
	return vue


func _heros_complet(vue: Node, position: Vector2, reglages: Dictionary = {}) -> CharacterBody2D:
	return heros_2d(vue, position, {"Sol": "GroundMovement2D", "Nage": "SwimMovement2D", "Mur": "WallMovement2D", "Directeur": "MovementDirector"}, reglages)


## Étiquette en haut à gauche : contexte courant du directeur, mise à jour à chaque image.
func _afficher_contexte(directeur: MovementDirector) -> Label:
	var label := etiquette(Vector2(300, 140), "", 20, ACCENT)
	get_tree().process_frame.connect(func():
		if is_instance_valid(label):
			label.text = "contexte : %s%s" % [directeur.get_current_context(), " (forcé)" if directeur.get_forced_context() != &"" else ""])
	return label


## Parcours type : marche, saut dans l'eau, sortie, course vers le mur, glissade.
func _parcours(directeur: MovementDirector) -> void:
	await piloter(directeur, [[1.0, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [1.6, Vector2(1, 0.2)],
		[0.6, Vector2.RIGHT, "saut_long"], [0.8, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [1.4, Vector2.RIGHT], [0.6, Vector2.ZERO]])


# ⚙️ Mode d'emploi

func _tuto_auto() -> void:
	sujet("Un personnage qui marche, nage et glisse aux murs")
	var vue := _niveau()
	var heros := _heros_complet(vue, Vector2(-500, 150))
	debug(vue)
	arbre(Vector2(40, 140), PackedStringArray(["CharacterBody2D", "  MovementDirector", "  GroundMovement2D", "  SwimMovement2D", "  WallMovement2D"]))
	code("directeur.move(Input.get_vector(\"ui_left\", \"ui_right\", \"ui_up\", \"ui_down\"))\nif Input.is_action_just_pressed(\"ui_accept\"): directeur.jump()")
	_afficher_contexte(heros.get_node("Directeur"))
	await secondes(0.4)
	await _parcours(heros.get_node("Directeur"))


# 💻 API

func _classe_director() -> void:
	sujet("MovementDirector : une intention, le bon composant")
	var vue := _niveau()
	var heros := _heros_complet(vue, Vector2(-500, 150))
	var directeur: MovementDirector = heros.get_node("Directeur")
	var actifs := etiquette(Vector2(576, 175), "", 17, TEXTE)
	get_tree().process_frame.connect(func():
		if is_instance_valid(actifs):
			var noms := PackedStringArray()
			for c in directeur.get_components():
				noms.append(("● " if c.active else "○ ") + String(c.name))
			actifs.text = "   ".join(noms))
	_afficher_contexte(directeur)
	code("$MovementDirector.move(direction)\n$MovementDirector.jump()")
	await secondes(0.4)
	await _parcours(directeur)


func _classe_context() -> void:
	sujet("MovementContext : nom, condition, priorité, composants")
	var vue := _niveau()
	var heros := _heros_complet(vue, Vector2(-500, 150))
	var directeur: MovementDirector = heros.get_node("Directeur")
	var tableau := etiquette(Vector2(860, 175), "", 15, TEXTE)
	tableau.horizontal_alignment = HORIZONTAL_ALIGNMENT_LEFT
	get_tree().process_frame.connect(func():
		if not is_instance_valid(tableau):
			return
		var lignes := PackedStringArray()
		for contexte in directeur.contexts:
			lignes.append("%s %s · priorité %d · %s" % ["▶" if contexte.context_name == directeur.get_current_context() else "  ",
				contexte.context_name, contexte.priority, "vrai" if directeur.is_condition_met(contexte) else "faux"])
		tableau.text = "\n".join(lignes))
	code("contexte.context_name = &\"eau\"\ncontexte.condition = MovementContext.CONDITION_IN_WATER\ncontexte.priority = 50\ncontexte.components = [NodePath(\"../SwimMovement2D\")]")
	await secondes(0.4)
	await _parcours(directeur)


func _auto_switch() -> void:
	sujet("auto_switch : changement automatique de contexte")
	var vue := _niveau()
	var heros := _heros_complet(vue, Vector2(-500, 150))
	var directeur: MovementDirector = heros.get_node("Directeur")
	await images(4)  # premier contexte (« sol ») appliqué, puis plus de changement automatique
	directeur.auto_switch = false
	debug(vue)
	_afficher_contexte(directeur)
	code("$MovementDirector.auto_switch = false\n$MovementDirector.set_context(&\"eau\")")
	await secondes(0.4)
	legende("auto_switch = false : on entre dans l'eau, mais le contexte reste « sol »")
	await piloter(directeur, [[1.0, Vector2.RIGHT], [0.5, Vector2.RIGHT, "saut_long"], [1.4, Vector2.ZERO]])
	legende("set_context(&\"eau\") : seul un changement explicite bascule")
	directeur.set_context(&"eau")
	await piloter(directeur, [[1.0, Vector2(1, -0.4)], [0.6, Vector2.RIGHT, "saut_long"], [1.2, Vector2.RIGHT]])


func _priority() -> void:
	sujet("priority : quel contexte l'emporte")
	for essai in 2:
		effacer()
		var vue := monde_2d()
		arene(vue)
		eau(vue, Rect2(-300, -200, 700, 400))
		bloc(vue, Rect2(150, -400, 40, 600))
		var heros := _heros_complet(vue, Vector2(0, 0))
		var directeur: MovementDirector = heros.get_node("Directeur")
		await images(3)
		if essai == 1:
			directeur.get_context(&"mur").priority = 60
		debug(vue)
		_afficher_contexte(directeur)
		code("eau.priority = 50 · mur.priority = %d" % (40 if essai == 0 else 60))
		legende("Dans l'eau, contre le mur : %s" % ("eau (50) > mur (40) → nage" if essai == 0 else "mur (60) > eau (50) → mur"))
		await piloter(directeur, [[1.0, Vector2(0.6, -0.8)], [2.4, Vector2(1, -0.5)]])


# 📌 Exemples

func _ex_code() -> void:
	sujet("Exemple : créer les contextes par code")
	var vue := monde_2d()
	arene(vue)
	var heros := heros_2d(vue, Vector2(-500, 150), {"Sol": "GroundMovement2D", "Air": "AerodynamicMovement2D"})
	var sol := MovementContext.new()
	sol.context_name = &"sol"
	sol.components = [NodePath("../Sol")]
	var air := MovementContext.new()
	air.context_name = &"air"
	air.condition = MovementContext.CONDITION_IN_AIR
	air.priority = 10
	air.components = [NodePath("../Air")]
	var directeur := MovementDirector.new()
	directeur.contexts = [sol, air]
	heros.add_child(directeur)
	debug(vue)
	_afficher_contexte(directeur)
	code("var d := MovementDirector.new()\nd.contexts = [sol, air]      # AVANT add_child\ncorps.add_child(d)")
	await piloter(directeur, [[0.8, Vector2.RIGHT], [1.0, Vector2.RIGHT, "saut_long"], [0.8, Vector2.RIGHT], [1.0, Vector2.RIGHT, "saut_long"], [0.8, Vector2.ZERO]])


func _ex_vol() -> void:
	sujet("Exemple : activer un mode à la demande (vol)")
	var vue := monde_2d()
	arene(vue)
	var heros := heros_2d(vue, Vector2(-450, 150), {"Sol": "GroundMovement2D", "Vol": "AerodynamicMovement2D", "Directeur": "MovementDirector"}, {"Vol": {"mode": 1}})
	var directeur: MovementDirector = heros.get_node("Directeur")
	await images(3)
	var vol := MovementContext.new()
	vol.context_name = &"vol"
	vol.condition = MovementContext.CONDITION_MANUAL
	vol.priority = 70
	vol.components = [NodePath("../Vol")]
	var contextes: Array = directeur.contexts.duplicate()
	contextes.append(vol)
	directeur.contexts = contextes
	debug(vue)
	_afficher_contexte(directeur)
	code("if event.is_action_pressed(\"voler\"): $MovementDirector.set_context(&\"vol\")\nelif event.is_action_pressed(\"atterrir\"): $MovementDirector.clear_forced_context()")
	await piloter(directeur, [[1.0, Vector2.RIGHT]])
	legende("« voler » : set_context(&\"vol\")")
	directeur.set_context(&"vol")
	directeur.set_vertical_input(1.0)
	await piloter(directeur, [[1.2, Vector2(1, -1)], [1.5, Vector2.RIGHT]])
	directeur.set_vertical_input(0.0)
	legende("« atterrir » : clear_forced_context(), retour au sol")
	directeur.clear_forced_context()
	await piloter(directeur, [[2.0, Vector2.LEFT], [0.6, Vector2.ZERO]])


func _ex_expression() -> void:
	sujet("Exemple : une condition sur mesure (Custom)")
	var vue := monde_2d()
	arene(vue, 2400.0)
	var heros := heros_2d(vue, Vector2(-1000, 150), {"Sol": "GroundMovement2D", "Glissade": "GroundMovement2D", "Directeur": "MovementDirector"},
		{"Glissade": {"deceleration": 150.0, "max_speed": 420.0}})
	var directeur: MovementDirector = heros.get_node("Directeur")
	var camera: Camera2D = vue.get_meta("camera")
	await images(3)
	var glissade := MovementContext.new()
	glissade.context_name = &"glissade"
	glissade.condition = MovementContext.CONDITION_CUSTOM
	glissade.expression = "director.is_on_floor() and abs(body.velocity.x) > 300"
	glissade.priority = 30
	glissade.components = [NodePath("../Glissade")]
	var contextes: Array = directeur.contexts.duplicate()
	contextes.append(glissade)
	directeur.contexts = contextes
	debug(vue)
	_afficher_contexte(directeur)
	get_tree().process_frame.connect(func():
		if is_instance_valid(camera):
			camera.position.x = heros.position.x)
	code("glissade.condition = MovementContext.CONDITION_CUSTOM\nglissade.expression = \"director.is_on_floor() and abs(body.velocity.x) > 300\"")
	await piloter(directeur, [[1.2, Vector2.RIGHT], [2.0, Vector2.RIGHT, "course"], [2.0, Vector2.ZERO], [1.0, Vector2.LEFT]])


func _ex_animation() -> void:
	sujet("Exemple : l'animation suit le contexte")
	var vue := _niveau()
	var heros := _heros_complet(vue, Vector2(-500, 150))
	var directeur: MovementDirector = heros.get_node("Directeur")
	var animation := etiquette(Vector2(576, 140), "", 22, BON)
	directeur.context_changed.connect(func(_avant: StringName, apres: StringName):
		var couleurs := {&"eau": Color("9fd8ff"), &"mur": Color("ffd166")}
		heros.get_node("Visuel").modulate = couleurs.get(apres, Color.WHITE)
		animation.text = "context_changed → animation « %s »" % {&"eau": "nage", &"mur": "glissade"}.get(apres, "marche"))
	code("func _sur_contexte(_avant, apres):\n\tmatch apres:\n\t\t&\"eau\": $AnimationPlayer.play(\"nage\")\n\t\t&\"mur\": $AnimationPlayer.play(\"glissade\")")
	await secondes(0.4)
	await _parcours(directeur)


func _ex_recul() -> void:
	sujet("Exemple : laisser le recul déplacer le personnage")
	var vue := monde_2d()
	arene(vue)
	var heros := heros_2d(vue, Vector2(-300, 150), {"Sol": "GroundMovement2D", "Recul": "Knockback2D", "Directeur": "MovementDirector"})
	var directeur: MovementDirector = heros.get_node("Directeur")
	await images(3)
	var recul := MovementContext.new()
	recul.context_name = &"recul"
	recul.condition = MovementContext.CONDITION_KNOCKED
	recul.priority = 100
	var contextes: Array = directeur.contexts.duplicate()
	contextes.append(recul)
	directeur.contexts = contextes
	var pics := Polygon2D.new()
	pics.polygon = PackedVector2Array([Vector2(60, 200), Vector2(80, 170), Vector2(100, 200), Vector2(120, 170), Vector2(140, 200)])
	pics.color = MAUVAIS
	vue.add_child(pics)
	debug(vue)
	_afficher_contexte(directeur)
	code("recul.condition = MovementContext.CONDITION_KNOCKED\nrecul.priority = 100   # aucun composant : le Knockback2D déplace seul")
	for i in 3:
		await piloter(directeur, [[0.9, Vector2.RIGHT]])
		legende("Touché : apply_knockback, contexte « recul »")
		heros.get_node("Recul").apply_knockback(Vector2(-1, -0.6))
		await secondes(1.2)
		legende("")


func _ex_env() -> void:
	sujet("Exemple : afficher l'état du personnage")
	var vue := _niveau()
	var heros := _heros_complet(vue, Vector2(-500, 150))
	var directeur: MovementDirector = heros.get_node("Directeur")
	var etat := etiquette(Vector2(576, 140), "", 15, TEXTE)
	get_tree().process_frame.connect(func():
		if is_instance_valid(etat):
			etat.text = str(directeur.get_environment()))
	code("$Etiquette.text = str($MovementDirector.get_environment())")
	await secondes(0.4)
	await _parcours(directeur)
