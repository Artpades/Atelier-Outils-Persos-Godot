extends "res://🎥Démos de documentation/5.🌀🫗/apercu_mouvement.gd"

## 🎬 Aperçu interactif de MovementDirector : un héros qui marche, nage et glisse aux murs ; contexte en
## direct, forçage, changement automatique et priorités.

var _tableau: Label


func titre() -> String:
	return "Orchestrateur (MovementDirector)"


func aide() -> String:
	return "Clavier : flèches, Espace (saut), Maj (course). Plonger dans le bassin, sauter contre le mur de droite."


func composants() -> Dictionary:
	return {"Sol": "GroundMovement2D", "Nage": "SwimMovement2D", "Mur": "WallMovement2D"}


func construire_niveau() -> void:
	arene()
	bloc(Rect2(-120, 200, 360, 200), Formes.SOL.darkened(0.2))
	eau(Rect2(-120, 60, 360, 140))
	bloc(Rect2(-120, 200, 360, 30))
	bloc(Rect2(400, -300, 40, 440))


func reglages() -> void:
	_tableau = panneau.ajouter_etiquette("")
	panneau.ajouter_case("auto_switch", true, func(v: bool): directeur.auto_switch = v)
	var rangee := panneau.ajouter_rangee()
	for nom in ["sol", "eau", "mur"]:
		panneau.ajouter_bouton("set_context(%s)" % nom, func(): directeur.set_context(StringName(nom)), rangee)
	panneau.ajouter_bouton("clear_forced_context()", func(): directeur.clear_forced_context())
	panneau.ajouter_curseur("Priorité du mur", 0.0, 100.0, 5.0, 40.0, func(v: float):
		var mur := directeur.get_context(&"mur")
		if mur:
			mur.priority = int(v))


func _process(_delta: float) -> void:
	if directeur == null or _tableau == null:
		return
	var lignes := PackedStringArray()
	for contexte in directeur.contexts:
		lignes.append("%s %s (priorité %d) : %s" % ["▶" if contexte.context_name == directeur.get_current_context() else "·",
			contexte.context_name, contexte.priority, "vrai" if directeur.is_condition_met(contexte) else "faux"])
	_tableau.text = "\n".join(lignes)
	afficher_code("contexte forcé : %s\nget_environment() = %s" % [directeur.get_forced_context(), directeur.get_environment()])
