extends "res://🎥Démos de documentation/socle/apercu.gd"

## 🪗 Aperçu interactif de FXStretchAndSquash : une balle qui rebondit (cliquer pour la relancer vers la
## souris), réglages d'étirement, d'écrasement et de lissage.

var _effet := FXStretchAndSquash.new()
var _balle: Node2D
var _vitesse := Vector2(220, 0)
const SOL := 560.0


func titre() -> String:
	return "Squash and stretch"


func aide() -> String:
	return "Cliquer dans la zone de jeu pour lancer la balle vers la souris."


func construire() -> void:
	var sol := Line2D.new()
	sol.points = PackedVector2Array([Vector2(20, SOL), Vector2(690, SOL)])
	sol.width = 3
	sol.default_color = Formes.DECOR
	add_child(sol)
	_balle = Node2D.new()
	_balle.position = Vector2(CENTRE.x, 250)
	_balle.draw.connect(func():
		_balle.draw_circle(Vector2.ZERO, 30, Formes.VARIANTES[1], true, -1.0, true)
		_balle.draw_circle(Vector2(10, -9), 7, Color(1, 1, 1, 0.6)))
	add_child(_balle)
	var joueur := PlayerFX.new()
	joueur.special_effects = [_effet]
	_balle.add_child(joueur)
	joueur.play()
	for propriete in [["max_stretch", 0.0, 1.5, 0.05], ["max_speed", 100.0, 2000.0, 50.0], ["impact_threshold", 50.0, 1500.0, 50.0],
			["max_impact_squash", 0.0, 0.9, 0.05], ["impact_recovery_speed", 1.0, 40.0, 1.0], ["smooth_speed", 1.0, 60.0, 1.0]]:
		var nom: String = propriete[0]
		panneau.ajouter_curseur(nom, propriete[1], propriete[2], propriete[3], _effet.get(nom), func(v: float): _effet.set(nom, v); _maj_code())
	panneau.ajouter_case("orient_to_velocity", false, func(v: bool): _effet.orient_to_velocity = v; _balle.rotation = 0.0; _maj_code())
	panneau.ajouter_case("enable_impact_squash", true, func(v: bool): _effet.enable_impact_squash = v; _maj_code())
	panneau.ajouter_bouton("Relancer", _jouer)
	_maj_code()


func _unhandled_input(evenement: InputEvent) -> void:
	if evenement is InputEventMouseButton and evenement.pressed and ZONE.has_point(evenement.position):
		_vitesse = (get_global_mouse_position() - _balle.position) * 3.0


func _process(delta: float) -> void:
	_vitesse.y += 1400.0 * delta
	_balle.position += _vitesse * delta
	if _balle.position.y > SOL - 30:
		_balle.position.y = SOL - 30
		_vitesse.y = -absf(_vitesse.y) * 0.85
		_vitesse.x *= 0.9
	if _balle.position.x < 50 or _balle.position.x > 660:
		_balle.position.x = clampf(_balle.position.x, 50, 660)
		_vitesse.x = -_vitesse.x


func _jouer() -> void:
	_vitesse = Vector2(randf_range(-400, 400), -1100)


func _maj_code() -> void:
	afficher_code("etirement.max_stretch = %s · max_speed = %s\netirement.impact_threshold = %s · max_impact_squash = %s\netirement.orient_to_velocity = %s\n$Balle/PlayerFX.play()" % [
		snappedf(_effet.max_stretch, 0.01), _effet.max_speed, _effet.impact_threshold, snappedf(_effet.max_impact_squash, 0.01), _effet.orient_to_velocity])
