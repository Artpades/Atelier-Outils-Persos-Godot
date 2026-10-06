extends Node2D

## 🧺 Agent de la démo d'arbre de comportement : se déplace vers `destination`, dépense de l'énergie en
## bougeant, ramasse les pièces. Il tient à jour le tableau noir de son arbre :
##   danger (distance au curseur), energie, piece_visible, score.

signal piece_ramassee(total: int)

@export var vitesse := 120.0
var destination := Vector2.ZERO
var danger_position := Vector2(-10000, -10000)
var pieces: Array[Vector2] = []
var couleur := Color(0.45, 0.85, 0.45)

@onready var arbre: BehaviorTree = $Arbre


func _ready() -> void:
	destination = position
	arbre.blackboard.set_value("energie", 100.0)
	arbre.blackboard.set_value("score", 0)


func _process(delta: float) -> void:
	var avant := position
	position = position.move_toward(destination, vitesse * delta)
	var tableau := arbre.blackboard
	tableau.set_value("energie", maxf(0.0, tableau.get_value("energie") - position.distance_to(avant) * 0.08))
	tableau.set_value("danger", position.distance_to(danger_position))
	tableau.set_value("piece_visible", not pieces.is_empty())
	queue_redraw()


## Pièce la plus proche (position de l'agent s'il n'y en a pas).
func piece_la_plus_proche() -> Vector2:
	var meilleure := position
	var distance := INF
	for piece in pieces:
		if position.distance_to(piece) < distance:
			distance = position.distance_to(piece)
			meilleure = piece
	return meilleure


## Appelée par un BTCallMethod : ramasse la pièce sous l'agent. Renvoie vrai si une pièce a été ramassée.
func ramasser() -> bool:
	for i in pieces.size():
		if position.distance_to(pieces[i]) < 12.0:
			pieces.remove_at(i)
			piece_ramassee.emit(arbre.blackboard.get_value("score", 0) + 1)
			return true
	return false


func _draw() -> void:
	draw_circle(Vector2.ZERO, 16.0, couleur)
	var energie: float = arbre.blackboard.get_value("energie", 100.0) if arbre.blackboard else 100.0
	draw_rect(Rect2(-20, 22, 40, 5), Color(0, 0, 0, 0.5))
	draw_rect(Rect2(-20, 22, 40 * energie / 100.0, 5), Color(1, 0.85, 0.2))
