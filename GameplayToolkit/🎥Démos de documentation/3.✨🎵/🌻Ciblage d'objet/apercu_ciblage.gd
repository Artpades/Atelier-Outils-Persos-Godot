extends "res://🎥Démos de documentation/socle/apercu.gd"

## 🌻 Aperçu interactif de FXTargeter : une tourelle 2D vise la souris (ou la moyenne souris + drone),
## et une caméra 3D cadre un groupe dont on règle l'écartement.

var _ciblage := FXTargeter.new()
var _cadrage := FXTargeter.new()
var _souris := Marker2D.new()
var _drone: Node2D
var _membres: Array[Node3D] = []
var _ecart := 2.0
var _avec_drone := false
var _temps := 0.0


func titre() -> String:
	return "Ciblage d'objet (FXTargeter)"


func aide() -> String:
	return "Bouger la souris dans la zone du haut : la tourelle la vise. En bas, la caméra 3D cadre le groupe."


func construire() -> void:
	_souris.name = "Souris"
	add_child(_souris)
	_drone = carre(Vector2(200, 160), Formes.VARIANTES[2], 26)
	_drone.name = "Drone"
	var tourelle := Node2D.new()
	tourelle.name = "Tourelle"
	tourelle.position = Vector2(CENTRE.x, 250)
	var canon := Polygon2D.new()
	canon.polygon = PackedVector2Array([Vector2(0, -8), Vector2(60, -8), Vector2(60, 8), Vector2(0, 8)])
	canon.color = Formes.ACCENT.darkened(0.2)
	tourelle.add_child(canon)
	tourelle.draw.connect(func(): tourelle.draw_circle(Vector2.ZERO, 26, Formes.ACCENT))
	add_child(tourelle)
	var joueur := PlayerFX.new()
	_ciblage.nodes_to_target = [NodePath("../../Souris")]
	_ciblage.add_lerp = true
	_ciblage.lerp_speed = 5.0
	joueur.special_effects = [_ciblage]
	tourelle.add_child(joueur)
	joueur.play()

	var vue := vue_3d(Rect2(66, 350, 580, 280))
	Formes.cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(80, 0.2, 80))
	for i in 3:
		var m := Formes.sphere(vue, Vector3.ZERO, Formes.VARIANTES[i], 0.5)
		m.name = "M%d" % i
		_membres.append(m)
	var camera := vue.get_camera_3d()
	camera.position = Vector3(0, 4, 9)
	_cadrage.nodes_to_target = [NodePath("../../M0"), NodePath("../../M1"), NodePath("../../M2")]
	_cadrage.add_lerp = true
	_cadrage.lerp_speed = 3.0
	_cadrage.min_distance = 2.0
	var joueur_3d := PlayerFX.new()
	joueur_3d.special_effects = [_cadrage]
	camera.add_child(joueur_3d)
	joueur_3d.play()

	panneau.ajouter_section("Tourelle 2D")
	panneau.ajouter_case("add_lerp", true, func(v: bool): _ciblage.add_lerp = v; _maj_code())
	panneau.ajouter_curseur("lerp_speed", 0.5, 20.0, 0.5, 5.0, func(v: float): _ciblage.lerp_speed = v; _maj_code())
	panneau.ajouter_case("Viser aussi le drone (moyenne)", false, func(v: bool):
		_avec_drone = v
		_ciblage.nodes_to_target = [NodePath("../../Souris"), NodePath("../../Drone")] if v else [NodePath("../../Souris")]
		_maj_code())
	panneau.ajouter_section("Caméra 3D")
	panneau.ajouter_curseur("Écartement du groupe", 0.5, 8.0, 0.1, _ecart, func(v: float): _ecart = v)
	panneau.ajouter_case("enable_camera_3d_depth_control", true, func(v: bool): _cadrage.enable_camera_3d_depth_control = v; _maj_code())
	panneau.ajouter_curseur("depth_margin", 0.5, 3.0, 0.1, _cadrage.depth_margin, func(v: float): _cadrage.depth_margin = v; _maj_code())
	panneau.ajouter_curseur("min_distance", 1.0, 20.0, 0.5, _cadrage.min_distance, func(v: float): _cadrage.min_distance = v; _maj_code())
	panneau.ajouter_curseur("max_distance", 5.0, 60.0, 1.0, _cadrage.max_distance, func(v: float): _cadrage.max_distance = v; _maj_code())
	_maj_code()


func _process(delta: float) -> void:
	_temps += delta
	var souris := get_global_mouse_position()
	if Rect2(0, 0, 712, 340).has_point(souris):
		_souris.global_position = souris
	_drone.position = Vector2(CENTRE.x + cos(_temps) * 250, 150 + sin(_temps * 1.7) * 40)
	for i in 3:
		_membres[i].position = Vector3((i - 1) * _ecart, 0.5, sin(i * 2.0 + _temps * 0.5) * _ecart * 0.5)


func _jouer() -> void:
	_ecart = 6.0


func _maj_code() -> void:
	afficher_code("ciblage.nodes_to_target = [%s]\nciblage.add_lerp = %s · lerp_speed = %s\ncadrage.enable_camera_3d_depth_control = %s\ncadrage.depth_margin = %s · min %s / max %s" % [
		"Souris, Drone" if _avec_drone else "Souris", _ciblage.add_lerp, _ciblage.lerp_speed, _cadrage.enable_camera_3d_depth_control,
		snappedf(_cadrage.depth_margin, 0.1), _cadrage.min_distance, _cadrage.max_distance])
