@tool
extends Node

@export var icon_texture: Texture2D
@export var custom_material: Material

var _gizmos: Array[RID] = []

# Un RID dédié par forme/dessin
var _rid_line: RID
var _rid_arrow: RID
var _rid_cube: RID
var _rid_sphere: RID
var _rid_circle: RID
var _rid_rectangle: RID
var _rid_capsule: RID
var _rid_polygon: RID
var _rid_path: RID
var _rid_arrow_path: RID
var _rid_origin: RID
var _rid_frustum: RID
var _rid_icon: RID
var _rid_text : RID
var _rid_shader: RID

var _time: float = 0.0

func _enter_tree() -> void:
	var vp := get_viewport()

	# 1. Instanciation d'un gizmo distinct pour chaque élément
	_rid_line = Gizmos3D.create_3D_gizmo(vp)
	_rid_arrow = Gizmos3D.create_3D_gizmo(vp)
	_rid_cube = Gizmos3D.create_3D_gizmo(vp)
	_rid_sphere = Gizmos3D.create_3D_gizmo(vp)
	_rid_circle = Gizmos3D.create_3D_gizmo(vp)
	_rid_rectangle = Gizmos3D.create_3D_gizmo(vp)
	_rid_capsule = Gizmos3D.create_3D_gizmo(vp)
	_rid_polygon = Gizmos3D.create_3D_gizmo(vp)
	_rid_path = Gizmos3D.create_3D_gizmo(vp)
	_rid_arrow_path = Gizmos3D.create_3D_gizmo(vp)
	_rid_origin = Gizmos3D.create_3D_gizmo(vp)
	_rid_frustum = Gizmos3D.create_3D_gizmo(vp)
	_rid_icon = Gizmos3D.create_3D_gizmo(vp)
	_rid_shader = Gizmos3D.create_3D_gizmo(vp)
	_rid_text = Gizmos3D.create_3D_gizmo(vp)

	# Registre pour la libération automatique en fin de vie
	_gizmos = [
		_rid_line, _rid_arrow, _rid_cube, _rid_sphere,
		_rid_circle, _rid_rectangle, _rid_capsule, _rid_polygon,
		_rid_path, _rid_arrow_path, _rid_origin, _rid_frustum,
		_rid_icon, _rid_text, _rid_shader
	]

	# Fallbacks pour les ressources de démonstration
	if not icon_texture:
		icon_texture = PlaceholderTexture2D.new()
		(icon_texture as PlaceholderTexture2D).size = Vector2(64, 64)

	if not custom_material:
		var mat := StandardMaterial3D.new()
		mat.albedo_color = Color.MAGENTA
		custom_material = mat

	# 2. Application d'un matériau / shader personnalisé sur un gizmo spécifique
	Gizmos3D.apply_shader_on_gizmo(_rid_shader, custom_material)

func _process(delta: float) -> void:
	_time += delta
	var offset := sin(_time * 2.0)
	var cos_offset := cos(_time * 2.0)

	# 3. Exécution de toutes les méthodes de dessin avec géométrie animée
	Gizmos3D.draw_3D_line(_rid_line, Vector3(-6, offset, 0), Vector3(-4, -offset, 0), Color.RED, 3.0)
	Gizmos3D.draw_3D_arrow(_rid_arrow, Vector3(-3, -1, 0), Vector3(-3, 1 + offset, 0), Color.GREEN, 0.4, 2.0)
	Gizmos3D.draw_3D_cube(_rid_cube, Vector3(-1, offset, 0), Vector3(1, 1, 1), Color.CYAN, false, 1.5)
	Gizmos3D.draw_3D_sphere(_rid_sphere, Vector3(1, 0, offset), 0.8, Color.YELLOW, true, 16)

	var circle_norm := Vector3(0, 1, offset).normalized()
	Gizmos3D.draw_3D_circle(_rid_circle, Vector3(3, offset, 0), 1.0, circle_norm, Color.MAGENTA, false, 24, 2.0)
	Gizmos3D.draw_3D_rectangle(_rid_rectangle, Vector3(5, offset, 0), Vector2(1.5, 1.0), Vector3(0, 1, 0), Color.ORANGE, true)
	Gizmos3D.draw_3D_capsule(_rid_capsule, Vector3(-5, 0, -3 + offset), 0.5, 2.0, Color.WHITE, false, 1.5)

	var poly_points := PackedVector3Array([
		Vector3(-3 + offset, 0, -3),
		Vector3(-2 + offset, 1.5, -3),
		Vector3(-1 + offset, 0, -3)
	])
	Gizmos3D.draw_3D_polygon(_rid_polygon, poly_points, Color.DODGER_BLUE, true)

	var path_points := PackedVector3Array([
		Vector3(0, 0, -3),
		Vector3(0.5, offset, -3),
		Vector3(1, 0, -3),
		Vector3(2, 0, -3),
		Vector3(2.5, -offset, -3),
		Vector3(3, 0, -3)
	])
	Gizmos3D.draw_3D_path(_rid_path, path_points, false, 30, 0, true, true, 0.1)

	var origin_transform := Transform3D(Basis().rotated(Vector3.UP, _time), Vector3(5, 2 + offset, -3))
	Gizmos3D.draw_3D_origin(_rid_origin, origin_transform, 1.0, 0.2, 2.0)

	var frustum_transform := Transform3D(Basis().rotated(Vector3.RIGHT, offset * 0.2), Vector3(-3, 3, 0))
	Gizmos3D.draw_3D_camera_frustum(_rid_frustum, frustum_transform, 60.0, 0.5, 3.0, 16.0 / 9.0, Color.DEEP_PINK)

	Gizmos3D.draw_3D_icon(_rid_icon, Vector3(0, 3, offset), icon_texture, Vector2(1, 1), BaseMaterial3D.TEXTURE_FILTER_NEAREST)
	Gizmos3D.draw_3D_cube(_rid_shader, Vector3(3, 3, offset), Vector3(0.8, 0.8, 0.8), Color.WHITE, true)
	Gizmos3D.draw_3D_text(_rid_text, Vector3.ONE * 3,  "Gizmos3D Active - Temps : %.2f" % _time, Color.BROWN, 32)

# Exemple de vidage ponctuel de géométrie sans détruire l'instance du RenderingServer
func clear_single_gizmo(gizmo_rid: RID) -> void:
	Gizmos3D.clear_3D_gizmo(gizmo_rid)

func _exit_tree() -> void:
	# 4. Nettoyage complet des ressources RIDs à la destruction du nœud
	for rid in _gizmos:
		Gizmos3D.free_3D_gizmo(rid)
	_gizmos.clear()
