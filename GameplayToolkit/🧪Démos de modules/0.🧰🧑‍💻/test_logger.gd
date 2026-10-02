@tool
extends Node

@export_tool_button("Exécuter Journal") var log : Callable = log_it
# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	log_it()
	pass # Replace with function body.

func log_it():
	L.log("Message ajouté", "TestLogger", "res://🖼️Editor Icons/🧰🧑‍💻/TaskManagement icon.png")
# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
