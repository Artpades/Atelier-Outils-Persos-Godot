extends Node2D

## 🔁 Frise d'un MusicLooper (plans filmés et aperçu web) : zones intro / boucle / fin, tête de lecture et
## boucles effectuées. Morceau de démonstration de 8 s.

var musique: MusicLooper
var largeur := 900.0
const DUREE := 8.0

func _process(_delta: float) -> void:
	queue_redraw()

func _draw() -> void:
	var police := ThemeDB.fallback_font
	var debut := musique.loop_start / DUREE * largeur
	var fin := musique.get_effective_loop_end() / DUREE * largeur
	draw_rect(Rect2(0, 0, debut, 60), Color(0.31, 0.63, 1.0, 0.35))
	draw_rect(Rect2(debut, 0, fin - debut, 60), Color(0.95, 0.64, 0.23, 0.45) if musique.loop_enabled else Color(0.5, 0.5, 0.5, 0.3))
	draw_rect(Rect2(fin, 0, largeur - fin, 60), Color(0.78, 0.47, 0.87, 0.35))
	for i in 33:
		var x := i * largeur / 32.0
		var h := 10.0 + absf(sin(i * 1.7)) * 30.0
		draw_line(Vector2(x, 30 - h / 2.0), Vector2(x, 30 + h / 2.0), Color(1, 1, 1, 0.35), 2.0)
	draw_string(police, Vector2(6, 80), "intro", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1, 1, 1, 0.7))
	draw_string(police, Vector2(debut + 6, 80), "section bouclée (loop_start → loop_end)", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1, 1, 1, 0.7))
	draw_string(police, Vector2(fin + 6, 80), "fin", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1, 1, 1, 0.7))
	if musique.loop_enabled:
		# Arc aplati (ellipse) de loop_end vers loop_start, avec le compteur au sommet
		var milieu := (debut + fin) / 2.0
		var rayon_x := (fin - debut) / 2.0
		var arc := PackedVector2Array()
		for i in 33:
			var angle := PI + PI * i / 32.0
			arc.append(Vector2(milieu + cos(angle) * rayon_x, sin(angle) * 50.0))
		draw_polyline(arc, Color(0.95, 0.64, 0.23), 2.0, true)
		draw_string(police, Vector2(milieu - 70, -58), "boucles : %d%s" % [musique.get_loops_done(),
			"" if musique.loop_count < 0 else " / %d" % musique.loop_count], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0.95, 0.64, 0.23))
	var x_tete := musique.get_playback_position() / DUREE * largeur
	draw_line(Vector2(x_tete, -8), Vector2(x_tete, 68), Color.WHITE if musique.is_playing() else Color(1, 1, 1, 0.4), 3.0)
	draw_string(police, Vector2(x_tete - 30, 100), "%.1f s" % musique.get_playback_position(), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color.WHITE)
