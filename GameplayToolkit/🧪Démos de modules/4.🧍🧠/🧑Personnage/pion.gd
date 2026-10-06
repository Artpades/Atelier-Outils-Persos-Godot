extends Node2D

## Visuel des démos de personnages : disque coloré, nom, barre de vie (si le personnage parent a un
## HealthComponent), clignotement aux dégâts, grisé à la mort.

@export var couleur := Color(0.4, 0.75, 1.0)
@export var rayon := 12.0
@export var afficher_nom := true

var _flash := 0.0


func _ready() -> void:
	var vie: HealthComponent = get_parent().get_health_component()
	if vie:
		vie.damaged.connect(func(_montant, _source): _flash = 0.25)
		vie.died.connect(queue_redraw)
		vie.revived.connect(queue_redraw)


func _process(delta: float) -> void:
	_flash = maxf(0.0, _flash - delta)
	queue_redraw()


func _draw() -> void:
	var personnage := get_parent()
	var teinte := couleur if personnage.is_alive() else Color(0.35, 0.35, 0.38)
	if _flash > 0.0:
		teinte = Color.WHITE
	draw_circle(Vector2.ZERO, rayon, teinte)
	var regard: Vector2 = personnage.get_facing()
	draw_line(Vector2.ZERO, regard * (rayon + 6), Color(1, 1, 1, 0.8), 2.0)
	var police := ThemeDB.fallback_font
	if afficher_nom:
		draw_string(police, Vector2(-50, -rayon - 14), personnage.display_name, HORIZONTAL_ALIGNMENT_CENTER, 100, 12)
	if personnage.get_health_component() == null:
		return
	var part: float = personnage.get_health() / maxf(1.0, personnage.get_max_health())
	draw_rect(Rect2(-16, -rayon - 9, 32, 4), Color(0, 0, 0, 0.6))
	draw_rect(Rect2(-16, -rayon - 9, 32 * part, 4), Color(0.9, 0.25, 0.25) if part < 0.35 else Color(0.35, 0.85, 0.4))
