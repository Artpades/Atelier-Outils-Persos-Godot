extends "res://🎥Démos de documentation/socle/apercu.gd"

## ✨ Aperçu interactif du Socle des effets : composer la liste special_effects d'un PlayerFX, enchaîner un
## effet au jeu ou à l'arrêt d'un autre, changer de cible, et suivre fx_played / fx_stopped.

var _joueur := PlayerFX.new()
var _cibles: Array[Node2D] = []
var _cible := 0
var _choix := {"FXShake": true, "FXChangeSize": true, "FXFade": false}
var _flash_enchaine := false
var _pop_a_l_arret := false
var _etat: Label


func titre() -> String:
	return "Socle des effets (PlayerFX)"


func aide() -> String:
	return "Cocher les effets de special_effects, puis Jouer / Arrêter. Les effets enchaînés suivent l'effet auquel ils sont attachés."


func construire() -> void:
	for i in 2:
		var objet := carre(Vector2(220 + i * 270, CENTRE.y), Color.WHITE, 110)
		etiquette(objet.position + Vector2(-60, 80), "Objet %s" % ["A", "B"][i])
		_cibles.append(objet)
	_cibles[0].add_child(_joueur)
	_etat = etiquette(Vector2(30, 560), "", 16, Formes.ACCENT)

	panneau.ajouter_section("special_effects")
	panneau.ajouter_case("FXShake (secousse 0,6 s)", true, func(v: bool): _choix["FXShake"] = v; _composer())
	panneau.ajouter_case("FXChangeSize (agrandir)", true, func(v: bool): _choix["FXChangeSize"] = v; _composer())
	panneau.ajouter_case("FXFade (teinte bleue)", false, func(v: bool): _choix["FXFade"] = v; _composer())
	panneau.ajouter_section("Enchaînements de la secousse")
	panneau.ajouter_case("effects_on_play_fx : flash rouge", false, func(v: bool): _flash_enchaine = v; _composer())
	panneau.ajouter_case("effects_on_stop_fx : fondu vert", false, func(v: bool): _pop_a_l_arret = v; _composer())
	panneau.ajouter_section("Commandes")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Jouer  play()", _jouer, rangee)
	panneau.ajouter_bouton("Arrêter  stop()", _arreter, rangee)
	panneau.ajouter_options("Cible de play()", PackedStringArray(["Parent (objet A)", "play($ObjetB)"]), 0, func(i: int): _cible = i)
	_composer()


func _process(_delta: float) -> void:
	var actifs := []
	for effet in _joueur.special_effects:
		if effet.is_activated():
			actifs.append(effet.get_class())
	_etat.text = "is_activated() : " + (", ".join(actifs) if actifs else "aucun effet actif")


func _composer() -> void:
	_joueur.stop()
	for objet in _cibles:
		objet.modulate = Color.WHITE
		objet.scale = Vector2.ONE
	var effets: Array[Effect] = []
	var lignes := PackedStringArray()
	if _choix["FXShake"]:
		var secousse := FXShake.new()
		secousse.duration = 0.6
		secousse.max_amplitude = 10.0
		if _flash_enchaine:
			var flash := FXFade.new()
			flash.color = Formes.MAUVAIS
			flash.duration = 0.08
			secousse.effects_on_play_fx = [flash]
			lignes.append("secousse.effects_on_play_fx = [flash_rouge]")
		if _pop_a_l_arret:
			var vert := FXFade.new()
			vert.color = Formes.BON
			vert.duration = 0.25
			vert.revert_on_stop = false
			secousse.effects_on_stop_fx = [vert]
			lignes.append("secousse.effects_on_stop_fx = [fondu_vert]")
		secousse.fx_played.connect(func(): journaliser("FXShake : fx_played"))
		secousse.fx_stopped.connect(func(): journaliser("FXShake : fx_stopped"))
		effets.append(secousse)
	if _choix["FXChangeSize"]:
		var taille := FXChangeSize.new()
		taille.final_scale = 1.35
		taille.duration = 0.3
		effets.append(taille)
	if _choix["FXFade"]:
		var teinte := FXFade.new()
		teinte.color = Formes.VARIANTES[0]
		teinte.duration = 0.3
		effets.append(teinte)
	_joueur.special_effects = effets
	var noms := PackedStringArray()
	for effet in effets:
		noms.append(effet.get_class())
	lignes.insert(0, "$PlayerFX.special_effects = [%s]" % ", ".join(noms))
	lignes.append("$PlayerFX.play(%s)" % ("" if _cible == 0 else "$ObjetB"))
	afficher_code("\n".join(lignes))


func _jouer() -> void:
	_composer_cible()
	_joueur.play(null if _cible == 0 else _cibles[1])


func _arreter() -> void:
	_joueur.stop()


func _composer_cible() -> void:
	var code := _code_lignes()
	code[code.size() - 1] = "$PlayerFX.play(%s)" % ("" if _cible == 0 else "$ObjetB")
	afficher_code("\n".join(code))


func _code_lignes() -> PackedStringArray:
	return PackedStringArray(_code.text.split("\n"))
