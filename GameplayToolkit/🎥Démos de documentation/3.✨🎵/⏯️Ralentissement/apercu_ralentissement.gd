extends "res://🎥Démos de documentation/socle/apercu.gd"

## ⏯️ Aperçu interactif de TimeManipulatorFX : gels (hit stop), ralenti progressif et vitesse de retour,
## sur des balles qui rebondissent ; courbe de Engine.time_scale.

var _temps := TimeManipulatorFX.new()
var _balles: Array[Node2D] = []
var _vitesses: Array[Vector2] = []
var _etat: Label
var _duree := 0.1
var _echelle_gel := 0.0


func titre() -> String:
	return "Ralentissement (TimeManipulatorFX)"


func aide() -> String:
	return "Les balles suivent le temps du jeu. Geler, ralentir, accélérer et observer la courbe de Engine.time_scale."


func construire() -> void:
	add_child(_temps)
	for i in 4:
		_balles.append(disque(Vector2(120 + i * 150, 380), Formes.VARIANTES[i], 20))
		_vitesses.append(Vector2(180 + i * 40, -500 + i * 40))
	var trace := traceur(Vector2(CENTRE.x, 540), func() -> float: return Engine.time_scale - 0.5, Formes.ACCENT, 80.0, 600.0)
	trace.hauteur = 100.0
	trace.duree_visible = 6.0
	etiquette(Vector2(56, 600), "Engine.time_scale (de 0 à 1) au cours du temps réel", 14)
	_etat = etiquette(Vector2(56, 150), "", 18, Formes.ACCENT)
	tree_exiting.connect(func(): Engine.time_scale = 1.0)

	panneau.ajouter_section("Hit stop")
	panneau.ajouter_curseur("duration (s)", 0.02, 1.0, 0.01, _duree, func(v: float): _duree = v; _maj_code())
	panneau.ajouter_curseur("scale pendant le gel", 0.0, 0.5, 0.05, _echelle_gel, func(v: float): _echelle_gel = v; _maj_code())
	panneau.ajouter_bouton("trigger_hit_stop()", _jouer)
	panneau.ajouter_section("Ralenti")
	panneau.ajouter_curseur("set_time_scale", 0.0, 2.0, 0.05, 1.0, func(v: float): _temps.set_time_scale(v); _maj_code())
	panneau.ajouter_curseur("recovery_speed", 0.5, 30.0, 0.5, _temps.recovery_speed, func(v: float): _temps.recovery_speed = v; _maj_code())
	_temps.hit_stop_started.connect(func(): journaliser("hit_stop_started"))
	_temps.hit_stop_finished.connect(func(): journaliser("hit_stop_finished"))
	_maj_code()


func _process(delta: float) -> void:
	for i in _balles.size():
		_vitesses[i].y += 1100.0 * delta
		_balles[i].position += _vitesses[i] * delta
		if _balles[i].position.y > 420:
			_balles[i].position.y = 420
			_vitesses[i].y = -absf(_vitesses[i].y)
		if _balles[i].position.x < 40 or _balles[i].position.x > 670:
			_vitesses[i].x = -_vitesses[i].x
	_etat.text = "Engine.time_scale = %.2f   is_hit_stopping() = %s" % [Engine.time_scale, _temps.is_hit_stopping()]


func _jouer() -> void:
	_temps.trigger_hit_stop(_duree, _echelle_gel)


func _maj_code() -> void:
	afficher_code("Temps.trigger_hit_stop(%s, %s)\nTemps.set_time_scale(%s)\nTemps.recovery_speed = %s" % [_duree, _echelle_gel, snappedf(_temps.target_time_scale, 0.01), _temps.recovery_speed])
