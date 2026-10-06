extends Node2D

## 💂 Agent de la démo HFSM : un disque qui se déplace vers `destination` à `vitesse`.
## Il écrit dans le tableau noir de sa machine la distance à l'intrus (le curseur de la souris).
## Les états (scripts etat_*.gd) choisissent destination, vitesse et couleur.

@export var vitesse := 0.0
@export var couleur := Color.WHITE
var destination := Vector2.ZERO
var intrus := Vector2(-1000, -1000)

@onready var machine: StateMachine = $Machine


func _ready() -> void:
	destination = position


func _process(delta: float) -> void:
	position = position.move_toward(destination, vitesse * delta)
	if machine.blackboard:
		machine.blackboard.set_value("distance", position.distance_to(intrus))
	queue_redraw()


func _draw() -> void:
	draw_circle(Vector2.ZERO, 120.0, Color(couleur, 0.06)) # rayon d'alerte (« distance < 120 »)
	draw_circle(Vector2.ZERO, 18.0, couleur)
	var direction := (destination - position).normalized() if destination != position else Vector2.RIGHT
	draw_line(Vector2.ZERO, direction * 28.0, Color.WHITE, 3.0)
	var etat := machine.get_active_state()
	if etat:
		draw_string(ThemeDB.fallback_font, Vector2(-40, -28), etat.name, HORIZONTAL_ALIGNMENT_CENTER, 80, 14)
