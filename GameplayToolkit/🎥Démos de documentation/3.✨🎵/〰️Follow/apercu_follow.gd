extends "res://🎥Démos de documentation/socle/apercu.gd"

## 〰️ Aperçu interactif de FXFollower : une Camera2D entre deux joueurs (le premier suit la souris, le
## second se promène), zoom automatique, vitesse et marges réglables.

var _suivi := FXFollower.new()
var _vue: SubViewport
var _a: Node2D
var _b: Node2D
var _temps := 0.0
var _ecart := 300.0
var _dessin: Node2D


func titre() -> String:
	return "Follow (FXFollower)"


func aide() -> String:
	return "Le joueur bleu suit la souris dans l'écran de jeu ; le violet se promène. La caméra reste entre eux."


func construire() -> void:
	_vue = vue_2d(Rect2(30, 150, 650, 460))
	for i in range(-15, 16):
		var bloc := Polygon2D.new()
		bloc.polygon = Formes.rectangle(Vector2(60, 30 + (i * 37) % 70))
		bloc.position = Vector2(i * 160, 140)
		bloc.color = Formes.DECOR
		_vue.add_child(bloc)
	_a = Formes.carre(_vue, Vector2(-100, 100), Formes.VARIANTES[0], 36)
	_a.name = "Joueur1"
	_b = Formes.carre(_vue, Vector2(100, 100), Formes.VARIANTES[2], 36)
	_b.name = "Joueur2"
	_dessin = Node2D.new()
	_dessin.z_index = 10
	_vue.add_child(_dessin)
	_dessin.draw.connect(func():
		var boite := Rect2(_a.global_position, Vector2.ZERO).expand(_b.global_position)
		_dessin.draw_rect(boite, Color(1, 1, 1, 0.6), false, 2.0)
		_dessin.draw_rect(boite.grow(_suivi.zoom_margin), Color(0.95, 0.64, 0.23, 0.8), false, 2.0))
	_suivi.targets = [NodePath("../../Joueur1"), NodePath("../../Joueur2")]
	var joueur := PlayerFX.new()
	joueur.special_effects = [_suivi]
	_vue.get_meta("camera").add_child(joueur)
	joueur.play()

	panneau.ajouter_curseur("Écart du joueur violet", 0.0, 1500.0, 10.0, _ecart, func(v: float): _ecart = v)
	panneau.ajouter_curseur("follow_speed", 0.5, 30.0, 0.5, _suivi.follow_speed, func(v: float): _suivi.follow_speed = v; _maj_code())
	panneau.ajouter_case("use_interpolation", true, func(v: bool): _suivi.use_interpolation = v; _maj_code())
	panneau.ajouter_case("auto_zoom_2d", true, func(v: bool): _suivi.auto_zoom_2d = v; _maj_code())
	panneau.ajouter_curseur("zoom_margin", 0.0, 400.0, 5.0, _suivi.zoom_margin, func(v: float): _suivi.zoom_margin = v; _maj_code())
	panneau.ajouter_curseur("min_zoom", 0.1, 1.0, 0.05, _suivi.min_zoom.x, func(v: float): _suivi.min_zoom = Vector2(v, v); _maj_code())
	panneau.ajouter_curseur("max_zoom", 1.0, 4.0, 0.1, _suivi.max_zoom.x, func(v: float): _suivi.max_zoom = Vector2(v, v); _maj_code())
	_maj_code()


func _process(delta: float) -> void:
	_temps += delta
	var locale := get_global_mouse_position() - Vector2(30, 150)
	if Rect2(Vector2.ZERO, Vector2(650, 460)).has_point(locale):
		var souris := _vue.get_canvas_transform().affine_inverse() * locale
		_a.position = _a.position.lerp(souris, minf(1.0, delta * 8.0))
	_b.position = _a.position + Vector2(_ecart * cos(_temps * 0.6), 40 + sin(_temps) * 60)
	_dessin.queue_redraw()


func _jouer() -> void:
	_ecart = 1200.0


func _maj_code() -> void:
	afficher_code("suivi.targets = [NodePath(\"../../Joueur1\"), NodePath(\"../../Joueur2\")]\nsuivi.follow_speed = %s · use_interpolation = %s\nsuivi.auto_zoom_2d = %s · zoom_margin = %s\nsuivi.min_zoom = %s · max_zoom = %s" % [
		_suivi.follow_speed, _suivi.use_interpolation, _suivi.auto_zoom_2d, _suivi.zoom_margin, snappedf(_suivi.min_zoom.x, 0.01), snappedf(_suivi.max_zoom.x, 0.1)])
