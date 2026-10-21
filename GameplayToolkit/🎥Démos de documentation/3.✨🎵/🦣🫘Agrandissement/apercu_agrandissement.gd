extends "res://🎥Démos de documentation/socle/apercu.gd"

## 🦣🫘 Aperçu interactif de FXChangeSize : boutons qui grossissent au survol (vraie souris), réglages de
## l'effet partagés par tous les boutons, courbe de l'échelle.

const TRANSITIONS := ["LINEAR", "SINE", "QUINT", "QUART", "QUAD", "EXPO", "ELASTIC", "CUBIC", "CIRC", "BOUNCE", "BACK", "SPRING"]
const EASES := ["EASE_IN", "EASE_OUT", "EASE_IN_OUT", "EASE_OUT_IN"]

var _effet := FXChangeSize.new()
var _joueurs: Array[PlayerFX] = []
var _boutons: Array[Button] = []


func titre() -> String:
	return "Agrandissement (FXChangeSize)"


func aide() -> String:
	return "Survoler les boutons : mouse_entered joue l'effet, mouse_exited l'arrête. Les réglages s'appliquent à tous."


func construire() -> void:
	_effet.final_scale = 1.15
	_effet.duration = 0.15
	for i in 3:
		var b := Button.new()
		b.text = ["Jouer", "Options", "Quitter"][i]
		b.add_theme_font_size_override("font_size", 24)
		b.size = Vector2(260, 64)
		b.position = Vector2(CENTRE.x - 130, 170 + i * 100)
		add_child(b)
		var joueur := PlayerFX.new()
		joueur.special_effects = [_effet.duplicate()]
		b.add_child(joueur)
		b.mouse_entered.connect(func(): joueur.play())
		b.mouse_exited.connect(func(): joueur.stop())
		_boutons.append(b)
		_joueurs.append(joueur)
	traceur(Vector2(CENTRE.x, 520), func() -> float: return (_boutons[0].scale.x - 1.0) * 60.0, Formes.VARIANTES[1], 1.0)
	etiquette(Vector2(CENTRE.x - 210, 575), "Échelle du bouton « Jouer » au cours du temps")

	panneau.ajouter_section("Réglages")
	panneau.ajouter_curseur("final_scale", 0.0, 2.0, 0.05, _effet.final_scale, func(v: float): _regler("final_scale", v))
	panneau.ajouter_curseur("duration (s)", 0.0, 2.0, 0.05, _effet.duration, func(v: float): _regler("duration", v))
	panneau.ajouter_curseur("original_value", 0.5, 1.5, 0.05, _effet.original_value, func(v: float): _regler("original_value", v))
	panneau.ajouter_options("transition", PackedStringArray(TRANSITIONS), Tween.TRANS_BACK, func(i: int): _regler("transition", i))
	panneau.ajouter_options("ease_type", PackedStringArray(EASES), Tween.EASE_OUT, func(i: int): _regler("ease_type", i))
	panneau.ajouter_section("Commandes")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Jouer « Jouer »", _jouer, rangee)
	panneau.ajouter_bouton("Arrêter", func(): _joueurs[0].stop(), rangee)
	_maj_code()


func _regler(propriete: String, valeur) -> void:
	_effet.set(propriete, valeur)
	for joueur in _joueurs:
		joueur.stop()
		joueur.special_effects[0].set(propriete, valeur)
	_maj_code()


func _jouer() -> void:
	_joueurs[0].play()


func _maj_code() -> void:
	afficher_code("var taille := FXChangeSize.new()\ntaille.final_scale = %s\ntaille.duration = %s\ntaille.original_value = %s\ntaille.transition = Tween.TRANS_%s\ntaille.ease_type = Tween.%s\n$Bouton.mouse_entered.connect($Bouton/PlayerFX.play)" % [
		snappedf(_effet.final_scale, 0.01), snappedf(_effet.duration, 0.01), snappedf(_effet.original_value, 0.01),
		TRANSITIONS[_effet.transition], EASES[_effet.ease_type]])
