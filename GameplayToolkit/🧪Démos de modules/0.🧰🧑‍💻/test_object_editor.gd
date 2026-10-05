extends Node
## Démo de l'éditeur d'objet : interface générée en jeu pour un ObjectEditorDemo.
## La cible est déclarée dans la scène (target_path du ProceduralEditor) ; dans l'éditeur, sélectionner le
## ProceduralEditor puis « Configurer l'interface » dans l'inspecteur pour choisir données et widgets.
## - Ctrl+Z / Ctrl+Maj+Z (ou Ctrl+Y) : annuler / rétablir.
## - Le compteur est incrémenté de l'extérieur toutes les 0,5 s (le widget doit suivre).
## - "valeur_bornee" est bornée entre 0 et 10 par son setter (saisir 50 : le widget revient à 10 + avertissement).
## - Touche Suppr : libère la cible (l'interface doit afficher « Objet libéré », sans crash).

@onready var cible: Node = $Cible


func _ready() -> void:
	cible.set("auto_increment", true)


func _unhandled_key_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and event.keycode == KEY_DELETE and is_instance_valid(cible):
		cible.queue_free()
