extends Node

## Scène de destination de la démo du gestionnaire de scènes.
## Vérifie que load_finished arrive après le changement de scène, puis permet de revenir à la démo
## (encore par SceneManager.load_scene_async).

const DEMO := "res://🧪Démos de modules/2.🧭🌍/🎬Gestionnaire de scènes/Démo Gestionnaire de scènes.tscn"

@onready var etat: Label = %Etat

var _gestionnaire: SceneManager


func _ready() -> void:
	_gestionnaire = SceneManager.get_instance()
	# _ready précède le changement effectif : load_finished doit arriver juste après.
	_gestionnaire.load_finished.connect(_on_fin)
	%Retour.pressed.connect(func(): _gestionnaire.load_scene_async(DEMO))


func _on_fin() -> void:
	etat.text = "✔ load_finished reçu : la scène a été chargée par le SceneManager"
	etat.modulate = Color(0.5, 1.0, 0.6)
