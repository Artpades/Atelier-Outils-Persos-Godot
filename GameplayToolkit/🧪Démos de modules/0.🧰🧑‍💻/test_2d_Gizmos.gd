@tool
extends Node

@export_category("Gizmos2D Test Configuration")
@export var test_texture: Texture2D
@export var test_material: ShaderMaterial

var line_gizmo: RID
var arrow_gizmo: RID
var circle_gizmo: RID
var rect_gizmo: RID
var capsule_gizmo: RID
var polygon_gizmo: RID
var path_gizmo: RID
var origin_gizmo: RID
var icon_gizmo: RID
var text_gizmo: RID
var cleared_gizmo: RID

var time: float = 0.0

func _enter_tree() -> void:
	# Allocation via les deux surcharges de création (Node et Viewport)
	line_gizmo = Gizmos2D.create_2D_gizmos(get_viewport())
	arrow_gizmo = Gizmos2D.create_2D_gizmos(get_viewport())
	circle_gizmo = Gizmos2D.create_2D_gizmos(get_viewport())
	rect_gizmo = Gizmos2D.create_2D_gizmos(get_viewport())
	capsule_gizmo = Gizmos2D.create_2D_gizmos(get_viewport())
	polygon_gizmo = Gizmos2D.create_2D_gizmos(get_viewport())
	path_gizmo = Gizmos2D.create_2D_gizmos(get_viewport())
	origin_gizmo = Gizmos2D.create_2D_gizmos(get_viewport())
	icon_gizmo = Gizmos2D.create_2D_gizmos(get_viewport())
	text_gizmo = Gizmos2D.create_2D_gizmos(get_viewport())
	cleared_gizmo = Gizmos2D.create_2D_gizmos(get_viewport())

func _process(delta: float) -> void:
	time += delta
	var wave: float = sin(time * 3.0) * 20.0

	# 1. Ligne simple + application de matériau/shader
	Gizmos2D.draw_2D_line(line_gizmo, Vector2(50, 50), Vector2(220 + wave, 50), 4.0, Color.RED, true)
	if test_material:
		Gizmos2D.apply_shader_on_gizmo(line_gizmo, test_material)

	# 2. Flèche animée
	Gizmos2D.draw_2D_arrow(arrow_gizmo, Vector2(50, 100), Vector2(220 + wave, 100), 16.0, 3.0, Color.GREEN)

	# 3. Cercle
	Gizmos2D.draw_2D_circle(circle_gizmo, Vector2(100, 180), 30.0 + wave * 0.5, Color.CYAN, false, 3.0)

	# 4. Rectangle
	Gizmos2D.draw_2D_rectangle(rect_gizmo, Rect2(200, 150, 80 + wave, 60), Color.MAGENTA, true)

	# 5. Capsule
	Gizmos2D.draw_2D_capsule(capsule_gizmo, Vector2(360, 180), Vector2(40, 80 + wave), Color.ORANGE, 2.0)

	# 6. Polygone
	var poly_points := PackedVector2Array([
		Vector2(50, 260),
		Vector2(90 + wave, 240),
		Vector2(130, 300),
		Vector2(40, 310)
	])
	Gizmos2D.draw_2D_polygon(polygon_gizmo, poly_points, Color.YELLOW, false, 2.0)

	# 7. Chemin avec courbe Bézier et flèches
	var path_points := PackedVector2Array([
		Vector2(200, 260),
		Vector2(250, 220 + wave),
		Vector2(300, 300),
		Vector2(360, 250)
	])
	Gizmos2D.draw_2D_path(path_gizmo, path_points, true, 4, 2.0, true, true, 5.0, 2.0, Color.DODGER_BLUE, 10.0)

	# 8. Repère d'origine orienté
	var t := Transform2D(time, Vector2(100, 400))
	Gizmos2D.draw_2D_origin(origin_gizmo, t, 40.0, 10.0, 10.0, 2.0, Color.WHITE)

	# 9. Icône / Texture
	if test_texture:
		Gizmos2D.draw_2D_icon(icon_gizmo, Vector2(250, 400), test_texture, Vector2(48, 48))

	# 10. Texte
	Gizmos2D.draw_2D_text(text_gizmo, Vector2(50, 470), "Gizmos2D Active - Temps : %.2f" % time, Color.GOLD, 16)

	# 11. Test d'effacement (efface le rendu pour vérifier clear_2D_gizmos)
	Gizmos2D.draw_2D_line(cleared_gizmo, Vector2(0, 0), Vector2(100, 100), 2.0, Color.WHITE)
	Gizmos2D.clear_2D_gizmos(cleared_gizmo)

func _exit_tree() -> void:
	# Libération de tous les RIDs
	Gizmos2D.free_gizmos(line_gizmo)
	Gizmos2D.free_gizmos(arrow_gizmo)
	Gizmos2D.free_gizmos(circle_gizmo)
	Gizmos2D.free_gizmos(rect_gizmo)
	Gizmos2D.free_gizmos(capsule_gizmo)
	Gizmos2D.free_gizmos(polygon_gizmo)
	Gizmos2D.free_gizmos(path_gizmo)
	Gizmos2D.free_gizmos(origin_gizmo)
	Gizmos2D.free_gizmos(icon_gizmo)
	Gizmos2D.free_gizmos(text_gizmo)
	Gizmos2D.free_gizmos(cleared_gizmo)
