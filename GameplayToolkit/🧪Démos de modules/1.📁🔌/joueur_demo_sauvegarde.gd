extends ColorRect

## Joueur fictif de la démo de sauvegarde : ses propriétés sont suivies par un SaveDataBinder (cible Node).
## Se déplace avec les flèches / ZQSD dans les limites de son parent.
##
## La position est exposée en deux flottants (pos_x / pos_y) : le JSON ne conserve pas les Vector2
## (ils sont écrits sous forme de texte), voir le test « Types » de la suite automatique.

const VITESSE := 300.0

@export var pseudo: String = "Héros"
@export var pieces: int = 0
@export var niveau: int = 1
@export var pv: float = 100.0

var pos_x: float:
	get:
		return position.x
	set(valeur):
		position.x = valeur

var pos_y: float:
	get:
		return position.y
	set(valeur):
		position.y = valeur


func _process(delta: float) -> void:
	# Pas de déplacement pendant la saisie du pseudo.
	if get_viewport().gui_get_focus_owner() is LineEdit:
		return

	var direction := Vector2(
		Input.get_axis(&"ui_left", &"ui_right") + _axe(KEY_Q, KEY_D),
		Input.get_axis(&"ui_up", &"ui_down") + _axe(KEY_Z, KEY_S)
	).limit_length(1.0)
	if direction == Vector2.ZERO:
		return

	position += direction * VITESSE * delta
	var zone: Vector2 = get_parent_area_size()
	position = position.clamp(Vector2.ZERO, (zone - size).max(Vector2.ZERO))


func _axe(negatif: Key, positif: Key) -> float:
	return float(Input.is_physical_key_pressed(positif)) - float(Input.is_physical_key_pressed(negatif))
