extends Node2D

## Décor de la démo caméras 2D : une grille repérée et le cadre de chaque caméra fixe, pour voir les
## transitions de position, de zoom et de rotation.

const ETENDUE := 2000
const PAS := 100


func _draw() -> void:
	for i in range(-ETENDUE, ETENDUE + 1, PAS):
		var couleur := Color(1, 1, 1, 0.25 if i == 0 else 0.08)
		draw_line(Vector2(i, -ETENDUE), Vector2(i, ETENDUE), couleur)
		draw_line(Vector2(-ETENDUE, i), Vector2(ETENDUE, i), couleur)

	var police := ThemeDB.fallback_font
	for camera: Camera2D in get_parent().find_children("*", "Camera2D", false):
		var taille := get_viewport_rect().size / camera.zoom
		var couleur := Color.from_hsv(float(camera.get_index()) * 0.27, 0.6, 1.0)
		draw_set_transform(camera.position + camera.offset, 0.0 if camera.ignore_rotation else camera.rotation)
		draw_rect(Rect2(-taille / 2.0, taille), couleur, false, 4.0)
		draw_circle(Vector2.ZERO, 12.0, couleur)
		draw_string(police, Vector2(18, -18), camera.name, HORIZONTAL_ALIGNMENT_LEFT, -1, 32, couleur)
	draw_set_transform(Vector2.ZERO)
