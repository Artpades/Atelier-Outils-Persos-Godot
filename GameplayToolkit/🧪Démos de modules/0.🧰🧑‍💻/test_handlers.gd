@tool
extends Node

## Node de test isolé : enregistre UN handle 2D et UN handle 3D auprès du HandlersManager,
## puis loggue chaque changement d'état de survol. Sert uniquement à valider manuellement
## que la détection (Rect2 en 2D, AABB + raycast caméra en 3D) fonctionne à chaque frame
## et que le gizmo change bien de couleur au survol.
##
## Pour tester : ajouter ce script à un Node dans une scène contenant une Camera3D active
## (nécessaire pour la détection 3D — HandlersManager ignore le 3D sans caméra active dans
## le Viewport), lancer la scène, survoler avec la souris la zone [100,100]-[164,164] en
## écran (handle 2D) ou l'origine du monde (handle 3D, AABB 1x1x1 centrée sur (0,0,0)).

const HANDLE_2D_ID := &"test_handle_2d"
const HANDLE_3D_ID := &"test_handle_3d"

var _manager: HandlersManager


func _ready() -> void:
	_manager = HandlersManager.get_singleton()
	if _manager == null:
		push_error("HandlersManagerTest : impossible d'obtenir le singleton HandlersManager (classe enregistrée dans register_types.cpp ?)")
		return

	# Le Viewport est maintenant obligatoire à l'enregistrement (comme Gizmos2D/Gizmos3D) :
	# on utilise ici celui de ce Node de test. Dans une vraie fenêtre d'éditeur (WindowEditor),
	# ce serait le Viewport de cette fenêtre à la place.
	var vp := get_viewport()

	_manager.register_handle_2d(HANDLE_2D_ID, Rect2(Vector2(100, 100), Vector2(64, 64)), vp)
	# camera = null : retombe sur vp.get_camera_3d() automatiquement.
	_manager.register_handle_3d(HANDLE_3D_ID, AABB(Vector3(-0.5, -0.5, -0.5), Vector3(1.0, 1.0, 1.0)), vp, null)

	_manager.handle_2d_hover_entered.connect(_on_handle_2d_hover_entered)
	_manager.handle_2d_hover_exited.connect(_on_handle_2d_hover_exited)
	_manager.handle_3d_hover_entered.connect(_on_handle_3d_hover_entered)
	_manager.handle_3d_hover_exited.connect(_on_handle_3d_hover_exited)
	_manager.handle_2d_moved.connect(_on_handle_2d_moved)
	_manager.handle_3d_moved.connect(_on_handle_3d_moved)

	print_rich("[color=green]HandlersManagerTest prêt : cliquez-glissez le rectangle [100,100]-[164,164] en écran (2D) ou le cube à l'origine du monde (3D, nécessite une Camera3D active)[/color]")


func _exit_tree() -> void:
	if _manager == null:
		return
	# Les connexions de signal sont coupées automatiquement par Godot à la destruction
	# de l'un ou l'autre des deux nœuds : pas de disconnect() manuel nécessaire ici.
	_manager.unregister_handle_2d(HANDLE_2D_ID)
	_manager.unregister_handle_3d(HANDLE_3D_ID)


func _on_handle_2d_hover_entered(id: StringName) -> void:
	print_rich("[color=yellow]2D survolé : %s[/color]" % id)


func _on_handle_2d_hover_exited(id: StringName) -> void:
	print("2D relâché : %s" % id)


func _on_handle_3d_hover_entered(id: StringName) -> void:
	print_rich("[color=yellow]3D survolé : %s[/color]" % id)


func _on_handle_3d_hover_exited(id: StringName) -> void:
	print("3D relâché : %s" % id)


func _on_handle_2d_moved(id: StringName, position: Vector2) -> void:
	print("2D déplacé : %s -> %s" % [id, position])


func _on_handle_3d_moved(id: StringName, position: Vector3) -> void:
	print("3D déplacé : %s -> %s" % [id, position])
