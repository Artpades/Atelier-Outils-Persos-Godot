@tool
extends Node

@export_tool_button("Ouvres-toi") var open : Callable = sesame_ouvre_toi

@export var window : WindowEditor = WindowEditor.new()

func sesame_ouvre_toi():
	window.open_window()

func _ready() -> void:
	if !Engine.is_editor_hint():
		window.open_window()
