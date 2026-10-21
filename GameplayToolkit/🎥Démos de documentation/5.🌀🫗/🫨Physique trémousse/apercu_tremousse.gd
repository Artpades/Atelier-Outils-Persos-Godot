extends "res://🎥Démos de documentation/socle/apercu.gd"

## 🫨 Aperçu interactif de Jiggle2D : le support suit la souris ; une antenne, une gelée et une queue en
## chaîne réagissent. Réglages appliqués à l'antenne.

var _support: Node2D
var _antenne := Jiggle2D.new()


func titre() -> String:
	return "Physique trémousse (Jiggle2D)"


func aide() -> String:
	return "Bouger la souris dans la zone de jeu : le support la suit. Cliquer pour donner une impulsion."


func construire() -> void:
	_support = carre(CENTRE, Formes.VARIANTES[0], 70)
	_antenne.position = Vector2(-15, -35)
	var tige := Line2D.new()
	tige.points = PackedVector2Array([Vector2.ZERO, Vector2(0, -50)])
	tige.width = 4
	tige.default_color = Color("cfd6e6")
	_antenne.add_child(tige)
	var pompon := Polygon2D.new()
	pompon.polygon = Formes.rectangle(Vector2(16, 16))
	pompon.position = Vector2(0, -56)
	pompon.color = Formes.ACCENT
	_antenne.add_child(pompon)
	_support.add_child(_antenne)
	var gelee := Jiggle2D.new()
	gelee.position = Vector2(20, -40)
	gelee.stiffness = 60.0
	gelee.damping = 3.0
	gelee.squash_amount = 0.004
	Formes.disque(gelee, Vector2.ZERO, Formes.BON, 14)
	_support.add_child(gelee)
	var parent: Node2D = _support
	for i in 4:
		var segment := Jiggle2D.new()
		segment.position = Vector2(-40 if i == 0 else -22, 10 if i == 0 else 0)
		segment.stiffness = 120.0 - i * 20.0
		Formes.disque(segment, Vector2.ZERO, Formes.VARIANTES[2].darkened(i * 0.1), 10 - i)
		parent.add_child(segment)
		parent = segment
	for reglage in [["stiffness", 10.0, 500.0, 5.0], ["damping", 0.5, 30.0, 0.5], ["max_offset", 2.0, 100.0, 1.0], ["lean_amount", -0.1, 0.1, 0.005],
			["squash_amount", 0.0, 0.01, 0.0005], ["max_stretch", 0.0, 0.95, 0.05]]:
		var nom: String = reglage[0]
		panneau.ajouter_curseur(nom, reglage[1], reglage[2], reglage[3], _antenne.get(nom), func(v: float): _antenne.set(nom, v); _maj_code())
	panneau.ajouter_curseur("gravity.y", -400.0, 800.0, 20.0, 0.0, func(v: float): _antenne.gravity = Vector2(0, v); _maj_code())
	panneau.ajouter_case("enabled", true, func(v: bool): _antenne.enabled = v)
	panneau.ajouter_case("debug_mode", false, func(v: bool): _antenne.debug_mode = v)
	panneau.ajouter_bouton("reset()", func(): _antenne.reset())
	_maj_code()


func _process(delta: float) -> void:
	var souris := get_global_mouse_position()
	if ZONE.has_point(souris):
		_support.position = _support.position.lerp(souris, minf(1.0, delta * 12.0))


func _unhandled_input(evenement: InputEvent) -> void:
	if evenement is InputEventMouseButton and evenement.pressed:
		_jouer()


func _jouer() -> void:
	_antenne.impulse(Vector2(randf_range(-500, 500), -400))


func _maj_code() -> void:
	afficher_code("$Jiggle2D.stiffness = %d · damping = %s · max_offset = %d\n$Jiggle2D.lean_amount = %s · squash_amount = %s\n$Jiggle2D.impulse(Vector2(...))   # clic" % [
		_antenne.stiffness, _antenne.damping, _antenne.max_offset, snappedf(_antenne.lean_amount, 0.001), snappedf(_antenne.squash_amount, 0.0001)])
