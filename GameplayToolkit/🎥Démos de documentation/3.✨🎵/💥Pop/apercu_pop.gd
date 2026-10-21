extends "res://🎥Démos de documentation/socle/apercu.gd"

## 💥 Aperçu interactif de PopSpawner / PopStyle : cliquer sur l'ennemi (dégâts) ou utiliser les boutons,
## et régler le mouvement et l'apparence des pops.

var _style := PopStyle.new()
var _spawner := PopSpawner.new()
var _ennemi: Node2D
var _piece: Texture2D


func titre() -> String:
	return "Pop (PopSpawner, PopStyle)"


func aide() -> String:
	return "Cliquer dans la zone de jeu fait jaillir des dégâts au point cliqué. Les boutons utilisent les autres appels."


func construire() -> void:
	_piece = Formes.pixel_art(PackedStringArray(["..jjjj..", ".jjooojj", "jjoojjoj", "jojjjjoj", "jojjjjoj", "jjoojjoj", ".jjooojj", "..jjjj.."]),
		{"j": Color("ffd166"), "o": Color("f2a33a")}, 4)
	_ennemi = carre(Vector2(CENTRE.x, 420), Formes.MAUVAIS.lightened(0.2), 80)
	_spawner.style = _style
	_spawner.offset = Vector3(0, -50, 0)
	_ennemi.add_child(_spawner)
	_spawner.popped.connect(func(_pop): _maj_compte())
	var sol := Line2D.new()
	sol.points = PackedVector2Array([Vector2(20, 500), Vector2(690, 500)])
	sol.width = 2
	sol.default_color = Formes.DECOR
	add_child(sol)

	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("pop_number(-12)", func(): _spawner.pop_number(-randi_range(5, 40)), rangee)
	panneau.ajouter_bouton("pop_number(+8)", func(): _spawner.pop_number(randi_range(1, 20)), rangee)
	rangee = panneau.ajouter_rangee()
	panneau.ajouter_bouton("pop_text(\"Paf\")", func(): _spawner.pop_text("Paf"), rangee)
	panneau.ajouter_bouton("6 x pop_texture", func():
		for i in 6:
			_spawner.pop_texture(_piece), rangee)
	panneau.ajouter_bouton("clear()", func(): _spawner.clear())
	for reglage in [["direction_degrees", -180.0, 180.0, 5.0], ["spread_degrees", 0.0, 360.0, 5.0], ["speed_max", 0.0, 900.0, 10.0],
			["gravity", -800.0, 2000.0, 20.0], ["lifetime", 0.2, 4.0, 0.1], ["pop_scale", 0.5, 3.0, 0.1], ["font_size", 8.0, 80.0, 2.0], ["bounce", 0.0, 1.0, 0.05]]:
		var nom: String = reglage[0]
		panneau.ajouter_curseur(nom, reglage[1], reglage[2], reglage[3], _style.get(nom), func(v: float):
			_style.set(nom, int(v) if nom == "font_size" else v)
			if nom == "speed_max":
				_style.speed_min = v * 0.6
			_maj_code())
	panneau.ajouter_case("use_floor", false, func(v: bool): _style.use_floor = v; _maj_code())
	_maj_code()


func _unhandled_input(evenement: InputEvent) -> void:
	if evenement is InputEventMouseButton and evenement.pressed and ZONE.has_point(evenement.position):
		var critique := randf() < 0.25
		_spawner.pop("CRITIQUE !" if critique else str(-randi_range(5, 40)), null, null, Color.ORANGE if critique else Color(0, 0, 0, 0), get_global_mouse_position())


func _jouer() -> void:
	for i in 6:
		_spawner.pop_texture(_piece)


func _maj_compte() -> void:
	_maj_code()


func _maj_code() -> void:
	afficher_code("style.direction_degrees = %s · spread_degrees = %s\nstyle.speed_min / max = %d / %d · gravity = %d\nstyle.lifetime = %s · pop_scale = %s · use_floor = %s\n$PopSpawner.pop_number(-12)   # get_active_count() = %d" % [
		_style.direction_degrees, _style.spread_degrees, _style.speed_min, _style.speed_max, _style.gravity,
		snappedf(_style.lifetime, 0.1), snappedf(_style.pop_scale, 0.1), _style.use_floor, _spawner.get_active_count() if _spawner.is_inside_tree() else 0])
