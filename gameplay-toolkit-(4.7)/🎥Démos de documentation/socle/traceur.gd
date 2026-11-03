extends Node2D

## 📈 Trace au cours du temps une valeur lue à chaque image (décalage d'une cible secouée, échelle...).
## Défile de droite à gauche sur `largeur` pixels pendant `duree_visible` secondes ; l'axe horizontal
## (valeur 0) est au milieu, `echelle` pixels par unité.

## Renvoie la valeur à tracer (float).
var mesure: Callable
var couleur := Color("4ea1ff")
var largeur := 260.0
var hauteur := 90.0
var echelle := 1.0
var duree_visible := 2.0

var _points: Array[Vector2] = [] # (temps, valeur)
var _temps := 0.0


func _process(delta: float) -> void:
	_temps += delta
	if mesure.is_valid():
		_points.append(Vector2(_temps, float(mesure.call())))
	while _points.size() > 2 and _points[0].x < _temps - duree_visible:
		_points.pop_front()
	queue_redraw()


func vider() -> void:
	_points.clear()


func _draw() -> void:
	var cadre := Rect2(0, -hauteur / 2.0, largeur, hauteur)
	draw_rect(cadre, Color(0, 0, 0, 0.25))
	draw_line(Vector2(0, 0), Vector2(largeur, 0), Color(1, 1, 1, 0.15))
	if _points.size() < 2:
		return
	var ligne := PackedVector2Array()
	for p in _points:
		var x := largeur - (_temps - p.x) / duree_visible * largeur
		ligne.append(Vector2(x, clampf(-p.y * echelle, -hauteur / 2.0, hauteur / 2.0)))
	draw_polyline(ligne, couleur, 2.0, true)
