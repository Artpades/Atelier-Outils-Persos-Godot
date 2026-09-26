@tool

extends Node

signal EventSignalWithParameters(variant_value: Variant)

@export_category("Tout Type")
@export var variant_value: Variant

@export_category("Variant de base")
@export var boolean_value: bool
@export var integer_value: int
@export_range(0, 1000, 0.5, "or_greater", "or_less", "read_only") var float_value: float
@export var string_value: String
@export var string_name_value: StringName

enum Enumeration { Option1, Option2, Option3, Option4, Option5 }

@export_subgroup("Enumeration Value", "Enumeration, read_only, show(boolean_value)")
@export var enumeration_value: Enumeration
@export var color_value: Color

@export_tool_button("Appeler Fonction", "Callable") 
var callable_value: Callable = Callable(self, "function")

@export var node_path_value: NodePath
var object_value: Object
@export var array_value: Array
@export var array_enum_value: Array[Enumeration]
@export var dictionary_value: Dictionary

@export_category("Structure de données")
@export var aabb_value: AABB
@export var basis_value: Basis
@export var plane_value: Plane
@export var projection_value: Projection
@export var quat_value: Quaternion
@export var rect2_value: Rect2
@export var rect2i_value: Rect2i
@export var rid_value: RID
@export var transform2d_value: Transform2D
@export var transform_value: Transform3D
@export var vector2_value: Vector2
@export var vector2i_value: Vector2i
@export var vector3_value: Vector3
@export var vector3i_value: Vector3i
@export var vector4_value: Vector4
@export var vector4i_value: Vector4i

@export_category("Tableau packés")
@export var packed_byte_array_value: PackedByteArray
@export var packed_int32_array_value: PackedInt32Array
@export var packed_float32_array_value: PackedFloat32Array
@export var packed_float64_array_value: PackedFloat64Array
@export var packed_string_array_value: PackedStringArray
@export var packed_vector2_array_value: PackedVector2Array
@export var packed_vector3_array_value: PackedVector3Array
@export var packed_vector4_array_value: PackedVector4Array
@export var packed_color_array_value: PackedColorArray


@onready var control: Control = $"../Control"


func _ready() -> void:
#	WidgetPropertyModifierBuilder.create_bool_check_box(self, "boolean_value", true,control)
	pass
var window : WindowEditor = WindowEditor.new()
func function() -> void:
	if window != null:
		print("Fonction appelée sans paramètre")
		window.open_window()
	


func function_with_parametters(p_variant_value: Variant = null) -> void:
	print("Fonction appelée avec ", p_variant_value, " en résultat d'argument")
	emit_signal("EventSignalWithParameters", p_variant_value)


func function_with_return_value(p_variant_value: Variant = null) -> Variant:
	emit_signal("EventSignalWithParameters", p_variant_value)
	return p_variant_value
