extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## ⏯️ Plans filmés de la page Ralentissement / arrêt du temps (effets/ralentissement-arret-du-temps.html).
## Les attentes des plans (secondes()) ignorent Engine.time_scale : elles comptent en temps réel.

var _temps: TimeManipulatorFX
var _jauge: Label


func titre_module() -> String:
	return "⏯️ Ralentissement · TimeManipulatorFX"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-TimeManipulatorFX": _classe,
		"TimeManipulatorFX.recovery_speed": _recovery_speed,
		"TimeManipulatorFX.trigger_hit_stop": _hit_stop,
		"ex-proportionnel": _ex_proportionnel,
		"ex-secousse": _ex_secousse,
		"ex-mort-boss": _ex_mort_boss,
		"ex-ui": _ex_ui,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Un coup : <code>trigger_hit_stop(0.08)</code> fige l'image ; une esquive : <code>set_time_scale(0.3)</code> ralentit progressivement, puis 1 revient en douceur.",
		"classe-TimeManipulatorFX": "Tout le jeu suit <code>Engine.time_scale</code> (affiché en haut), piloté en douceur par TimeManipulatorFX.",
		"TimeManipulatorFX.recovery_speed": "Le même passage à 0,2 puis retour à 1 avec <code>recovery_speed</code> 1,5, 5 (défaut) et 20 : la courbe de <code>Engine.time_scale</code>.",
		"TimeManipulatorFX.trigger_hit_stop": "Gels de 0,05, 0,15 et 0,4 s, puis un gel à 0,1 au lieu de 0 : le mouvement reste visible.",
		"ex-proportionnel": "Plus le coup fait de dégâts, plus le gel dure ; au-delà de 30, un ralenti à 0,1 au lieu d'un arrêt.",
		"ex-secousse": "Pendant un gel à 0 la secousse ne peut pas bouger : on la lance sur <code>hit_stop_finished</code>.",
		"ex-mort-boss": "Boss vaincu : plongée rapide vers 0,2 (<code>recovery_speed</code> 8), puis retour lent à 1 (1,5).",
		"ex-ui": "Pendant le ralenti, le panneau animé par un Tween qui ignore <code>time_scale</code> reste fluide ; l'autre est ralenti.",
	}


func schemas() -> Dictionary:
	return {
		"etats": ["etats.svg", "<code>set_time_scale</code> fixe la vitesse visée (<code>target_time_scale</code>) que <code>Engine.time_scale</code> rejoint en douceur ; <code>trigger_hit_stop</code> la remplace d'un coup pendant un temps réel, entre <code>hit_stop_started</code> et <code>hit_stop_finished</code> (<code>is_hit_stopping()</code>)."],
	}


func reutilisations() -> Dictionary:
	return {
		"TimeManipulatorFX.target_time_scale": "TimeManipulatorFX.recovery_speed",
		"TimeManipulatorFX.set_time_scale": "classe-TimeManipulatorFX",
		"TimeManipulatorFX.is_hit_stopping": "etats",
		"TimeManipulatorFX.hit_stop_started": "etats",
		"TimeManipulatorFX.hit_stop_finished": "ex-secousse",
	}


func _preparer() -> void:
	_temps = TimeManipulatorFX.new()
	_temps.name = "Temps"
	add_child(_temps)
	_jauge = etiquette(Vector2(330, 130), "", 20, ACCENT)
	get_tree().process_frame.connect(_maj_jauge)
	tree_exiting.connect(func(): Engine.time_scale = 1.0)


func _maj_jauge() -> void:
	if is_instance_valid(_jauge):
		_jauge.text = "Engine.time_scale = %.2f%s" % [Engine.time_scale, "   (hit stop)" if _temps.is_hit_stopping() else ""]


## Balle qui rebondit (suit le temps du jeu) : un repère visuel de la vitesse.
func _balle(position: Vector2, couleur: Color = VARIANTES[0]) -> Node2D:
	var balle := disque(position, couleur, 22)
	var vitesse := [Vector2(260, -420)]
	var sol := position.y
	var depart_x := position.x
	balle.set_process(true)
	get_tree().process_frame.connect(func():
		if not is_instance_valid(balle):
			return
		var delta := get_process_delta_time()
		vitesse[0].y += 1100.0 * delta
		balle.position += vitesse[0] * delta
		if balle.position.y > sol:
			balle.position.y = sol
			vitesse[0].y = -absf(vitesse[0].y) * 0.95
		if absf(balle.position.x - depart_x) > 220:
			vitesse[0].x = -vitesse[0].x)
	return balle


## Courbe de Engine.time_scale (0 à 1) au cours du temps réel.
func _courbe(position: Vector2, couleur: Color, largeur: float = 900.0) -> Traceur:
	var trace := traceur(position, func() -> float: return Engine.time_scale - 0.5, couleur, 100.0, largeur)
	trace.hauteur = 110.0
	trace.duree_visible = 6.0
	return trace


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : gel à l'impact, ralenti progressif")
	_preparer()
	var heros := carre(Vector2(380, 330), VARIANTES[0], 60)
	var ennemi := carre(Vector2(620, 330), MAUVAIS, 70)
	_balle(Vector2(860, 380), VARIANTES[3])
	_courbe(Vector2(576, 500), ACCENT)
	code("func _on_coup_porte() -> void:\n\tTemps.trigger_hit_stop(0.08)")
	for i in 2:
		await secondes(0.6)
		legende("Coup porté : le jeu se fige 80 ms")
		await deplacer(heros, Vector2(540, 330), 0.12)
		_temps.trigger_hit_stop(0.08)
		var recul := ennemi.create_tween()
		recul.tween_property(ennemi, "position:x", 680.0, 0.15)
		recul.tween_property(ennemi, "position:x", 620.0, 0.3)
		await secondes(0.4)
		heros.position.x = 380
	code("func _on_visee(active: bool) -> void:\n\tTemps.set_time_scale(0.3 if active else 1.0)")
	legende("Esquive : set_time_scale(0.3), le ralenti s'installe en douceur")
	_temps.set_time_scale(0.3)
	await secondes(2.2)
	legende("set_time_scale(1.0) : retour progressif (recovery_speed = 5)")
	_temps.set_time_scale(1.0)
	await secondes(2.0)


# 💻 API

func _classe() -> void:
	sujet("TimeManipulatorFX : piloter Engine.time_scale")
	_preparer()
	for i in 3:
		_balle(Vector2(300 + i * 280, 400), VARIANTES[i])
	_courbe(Vector2(576, 520), ACCENT)
	for etape in [[0.5, "set_time_scale(0.5)"], [0.1, "set_time_scale(0.1)"], [2.0, "set_time_scale(2.0)"], [1.0, "set_time_scale(1.0)"]]:
		code("Temps.%s" % etape[1])
		legende(etape[1])
		_temps.set_time_scale(etape[0])
		await secondes(1.8)


func _recovery_speed() -> void:
	sujet("recovery_speed et target_time_scale")
	_preparer()
	var courbes := [[1.5, VARIANTES[0]], [5.0, VARIANTES[1]], [20.0, VARIANTES[2]]]
	for c in courbes:
		var trace := _courbe(Vector2(576, 260 + courbes.find(c) * 120), c[1], 820.0)
		etiquette(Vector2(110, 260 + courbes.find(c) * 120), "recovery_speed = %s" % c[0], 15, c[1])
		trace.mesure = Callable()
	code("Temps.recovery_speed = 5.0\nTemps.target_time_scale = 0.2   # = set_time_scale(0.2)")
	# Une courbe à la fois (Engine.time_scale est global)
	for i in courbes.size():
		_temps.recovery_speed = courbes[i][0]
		var traces := scene.get_children().filter(func(n): return n is Traceur)
		traces[i].mesure = func() -> float: return Engine.time_scale - 0.5
		legende("recovery_speed = %s : 1 → 0,2 → 1" % courbes[i][0])
		_temps.target_time_scale = 0.2
		await secondes(2.0)
		_temps.target_time_scale = 1.0
		await secondes(2.0)
		traces[i].mesure = Callable()


func _hit_stop() -> void:
	sujet("trigger_hit_stop(duration, scale)")
	_preparer()
	_balle(Vector2(576, 420), VARIANTES[0])
	_courbe(Vector2(576, 530), ACCENT)
	var signaux := etiquette(Vector2(950, 300), "", 18, TEXTE)
	_temps.hit_stop_started.connect(func(): signaux.text = "hit_stop_started")
	_temps.hit_stop_finished.connect(func(): signaux.text = "hit_stop_finished")
	for appel in [[0.05, 0.0], [0.15, 0.0], [0.4, 0.0], [0.4, 0.1]]:
		await secondes(1.0)
		code("Temps.trigger_hit_stop(%s, %s)" % appel)
		legende("Gel de %s s à la vitesse %s" % appel)
		_temps.trigger_hit_stop(appel[0], appel[1])
	await secondes(1.4)


# 📌 Exemples

func _ex_proportionnel() -> void:
	sujet("Exemple : un gel proportionnel aux dégâts")
	_preparer()
	var ennemi := carre(Vector2(700, 330), MAUVAIS, 80)
	var heros := carre(Vector2(420, 330), VARIANTES[0], 60)
	var chiffre := etiquette(Vector2(700, 230), "", 30, ACCENT)
	_courbe(Vector2(576, 500), ACCENT)
	code("var duree := clampf(degats * 0.004, 0.03, 0.15)\nTemps.trigger_hit_stop(duree, 0.1 if degats > 30 else 0.0)")
	for degats in [8, 22, 45]:
		await secondes(0.8)
		await deplacer(heros, Vector2(620, 330), 0.12)
		var duree := clampf(degats * 0.004, 0.03, 0.15)
		chiffre.text = "-%d  (gel %.2f s%s)" % [degats, duree, ", vitesse 0,1" if degats > 30 else ""]
		_temps.trigger_hit_stop(duree, 0.1 if degats > 30 else 0.0)
		var recul := ennemi.create_tween()
		recul.tween_property(ennemi, "position:x", 700.0 + degats * 2.0, 0.2)
		recul.tween_property(ennemi, "position:x", 700.0, 0.3)
		await secondes(0.5)
		heros.position.x = 420


func _ex_secousse() -> void:
	sujet("Exemple : gel puis secousse de caméra")
	_preparer()
	var vue := niveau(Rect2(76, 150, 1000, 330))
	var camera: Camera2D = vue.get_meta("camera")
	var joueur := joueur_fx(camera, effet("FXShake", {"duration": 0.35, "max_amplitude": 14.0}))
	_temps.hit_stop_finished.connect(joueur.play)
	code("Temps.hit_stop_finished.connect($Camera2D/PlayerFX.play)\nTemps.trigger_hit_stop(0.1)")
	for position in [Vector2(120, 60), Vector2(-80, 80), Vector2(250, 40)]:
		await secondes(1.0)
		legende("impact() : gel de 0,1 s, puis la secousse démarre à la reprise")
		explosion(vue, position)
		_temps.trigger_hit_stop(0.1)
		await secondes(0.8)


func _ex_mort_boss() -> void:
	sujet("Exemple : un ralenti final qui revient tout seul")
	_preparer()
	var boss := carre(Vector2(576, 300), MAUVAIS, 140)
	for i in 3:
		_balle(Vector2(250 + i * 330, 420), VARIANTES[i])
	_courbe(Vector2(576, 530), ACCENT)
	code("Temps.recovery_speed = 8.0\nTemps.set_time_scale(0.2)\nawait get_tree().create_timer(1.5, true, false, true).timeout\nTemps.recovery_speed = 1.5\nTemps.set_time_scale(1.0)")
	await secondes(1.0)
	legende("Boss vaincu : plongée rapide vers 0,2")
	var disparition := boss.create_tween()
	disparition.tween_property(boss, "scale", Vector2(1.6, 0.0), 1.2)
	_temps.recovery_speed = 8.0
	_temps.set_time_scale(0.2)
	await secondes(1.5)
	legende("1,5 s réelles plus tard : retour lent à la normale")
	_temps.recovery_speed = 1.5
	_temps.set_time_scale(1.0)
	await secondes(3.5)


func _ex_ui() -> void:
	sujet("Exemple : une interface fluide pendant un ralenti")
	_preparer()
	var panneaux: Array[Node2D] = []
	for i in 2:
		var p := carre(Vector2(300, 260 + i * 160), VARIANTES[i], 70)
		etiquette(Vector2(300, 330 + i * 160), ["Tween normal (ralenti)", "set_ignore_time_scale(true)"][i], 16, VARIANTES[i])
		panneaux.append(p)
	code("var tween := create_tween()\ntween.set_ignore_time_scale(true)")
	_temps.recovery_speed = 20.0
	_temps.set_time_scale(0.2)
	legende("Jeu au ralenti (0,2)")
	for aller in 3:
		for i in 2:
			var tween := panneaux[i].create_tween()
			tween.set_ignore_time_scale(i == 1)
			tween.tween_property(panneaux[i], "position:x", 850.0 if aller % 2 == 0 else 300.0, 0.6).set_trans(Tween.TRANS_SINE)
		await secondes(1.6)
	_temps.set_time_scale(1.0)
	await secondes(0.6)
