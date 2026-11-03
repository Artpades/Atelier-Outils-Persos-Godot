extends Node2D

## Pastille de la démo glisser-déposer : jeton déplaçable (disque plein) ou point d'aimantage (anneau),
## avec une étiquette dessous. Mise en valeur au survol et à la saisie (survole, saisi).

@export var couleur: Color = Color.WHITE
@export var texte: String = ""
@export var rayon: float = 28.0
## Anneau seul : point d'aimantage plutôt que jeton.
@export var anneau: bool = false

var survole := false:
	set(valeur):
		if survole != valeur:
			survole = valeur
			queue_redraw()
var saisi := false:
	set(valeur):
		if saisi != valeur:
			saisi = valeur
			queue_redraw()


func _draw() -> void:
	if anneau:
		draw_arc(Vector2.ZERO, rayon, 0.0, TAU, 48, Color(couleur, 0.6), 3.0, true)
	else:
		var r := rayon * (1.12 if saisi else 1.0)
		draw_circle(Vector2.ZERO, r, couleur.lightened(0.3) if survole or saisi else couleur)
		draw_arc(Vector2.ZERO, r, 0.0, TAU, 48, Color.WHITE if survole or saisi else Color(0, 0, 0, 0.35), 2.0, true)
	if texte:
		draw_string(ThemeDB.fallback_font, Vector2(-55, rayon + 18), texte, HORIZONTAL_ALIGNMENT_CENTER, 110, 13, Color(1, 1, 1, 0.75))
