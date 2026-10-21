extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## ✨ Plans filmés de la page Socle des effets (Documentation/modules/effets/socle-effets.html).


## Effet écrit en script, comme dans le tutoriel C du Mode d'emploi.
class FXClignote extends Effect:
	var frequence := 10.0
	var _temps := 0.0

	func _play_fx(_cible: Node) -> void:
		_temps = 0.0

	func _process_fx(delta: float, cible: Node) -> void:
		_temps += delta
		if cible is CanvasItem:
			cible.visible = fmod(_temps * frequence, 1.0) < 0.5

	func _stop_fx(cible: Node) -> void:
		if cible is CanvasItem:
			cible.visible = true


## Effet qui signale chaque appel de son cycle de vie (journal à l'écran).
class FXJournal extends Effect:
	var journal: Callable

	func _play_fx(_cible: Node) -> void:
		journal.call("_play_fx(cible)")

	func _process_fx(_delta: float, _cible: Node) -> void:
		journal.call("_process_fx(delta, cible)")

	func _stop_fx(_cible: Node) -> void:
		journal.call("_stop_fx(cible)")


func titre_module() -> String:
	return "✨ Socle des effets · Effect, PlayerFX"


func plans() -> Dictionary:
	return {
		"mode-emploi-tuto-jouer": _tuto_jouer,
		"mode-emploi-tuto-enchainer": _tuto_enchainer,
		"mode-emploi-tuto-script": _tuto_script,
		"classe-Effect": _classe_effect,
		"classe-PlayerFX": _classe_player,
		"Effect.effects_on_play_fx": _effects_on_play,
		"Effect.effects_on_stop_fx": _effects_on_stop,
		"PlayerFX.node_to_play_effects": _node_to_play,
		"ex-degats": _ex_degats,
		"ex-autre-cible": _ex_autre_cible,
		"ex-attendre": _ex_attendre,
		"ex-sans-player": _ex_sans_player,
		"ex-pause": _ex_pause,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi-tuto-jouer": "Un PlayerFX sous l'objet, FXShake et FXChangeSize dans <code>special_effects</code> : <code>play()</code> les joue ensemble, <code>stop()</code> rend la taille d'origine.",
		"mode-emploi-tuto-enchainer": "Une « recette » : le flash rouge est enchaîné au jeu de la secousse, le passage au vert à son arrêt.",
		"mode-emploi-tuto-script": "<code>FXClignote</code>, effet écrit en GDScript (<code>extends Effect</code>), joué par un PlayerFX comme un effet C++.",
		"classe-Effect": "Le cycle de vie d'un effet : chaque appel s'affiche au moment où il se produit.",
		"classe-PlayerFX": "Un PlayerFX joue tous ses effets sur son parent, les met à jour à chaque image, puis les arrête ensemble.",
		"Effect.effects_on_play_fx": "Le flash enchaîné au jeu commence avec la secousse et s'arrête avec elle (fin de <code>duration</code>).",
		"Effect.effects_on_stop_fx": "Effet enchaîné à l'arrêt : joué une seule fois quand la secousse se termine.",
		"PlayerFX.node_to_play_effects": "Vide : les effets visent le parent du PlayerFX. Renseigné : ils visent ce nœud.",
		"ex-degats": "Un seul <code>play()</code> : secousse, flash rouge (et son, coupé ici) ensemble.",
		"ex-autre-cible": "<code>stop()</code> puis <code>play(carte)</code> : les mêmes effets passent d'une carte à l'autre.",
		"ex-attendre": "<code>await secousse.fx_stopped</code> : la porte s'ouvre quand la secousse se termine seule.",
		"ex-sans-player": "Sans PlayerFX : <code>play_fx</code>, puis <code>process_fx</code> appelé à chaque image par le script, puis <code>stop_fx</code>.",
		"ex-pause": "Jeu en pause : le titre du menu (PROCESS_MODE_ALWAYS) continue de pulser, l'objet du jeu est figé.",
	}


func schemas() -> Dictionary:
	return {
		"cycle": ["cycle.svg", "Ordre des appels : <code>play_fx</code> → <code>_play_fx</code> → <code>fx_played</code> → effets enchaînés au jeu ; à chaque image <code>process_fx</code> → <code>_process_fx</code> ; <code>stop_fx</code> → <code>_stop_fx</code> → enchaînés arrêtés puis effets d'arrêt → <code>fx_stopped</code>. <code>is_activated()</code> est vrai entre les deux."],
		"lien-player": ["lien_player.svg", "Le PlayerFX se transmet à ses effets et à leurs effets enchaînés : leurs chemins de nœuds (NodePath) sont relatifs à lui."],
		"PlayerFX.play_effect": ["inspecteur.svg", "Les boutons « Jouer effet » / « Arrêter effet » de l'inspecteur jouent les effets dans l'éditeur, sans lancer le jeu."],
	}


func reutilisations() -> Dictionary:
	return {
		"Effect": "cycle",
		"Effect._play_fx": "cycle",
		"Effect._process_fx": "cycle",
		"Effect._physics_process_fx": "cycle",
		"Effect._stop_fx": "cycle",
		"Effect.is_activated": "cycle",
		"Effect.physics_process_fx": "cycle",
		"Effect.fx_played": "classe-Effect",
		"Effect.fx_stopped": "ex-attendre",
		"Effect.play_fx": "ex-sans-player",
		"Effect.process_fx": "ex-sans-player",
		"Effect.stop_fx": "ex-sans-player",
		"Effect.set_player_fx": "lien-player",
		"Effect.get_player_fx": "lien-player",
		"PlayerFX.special_effects": "classe-PlayerFX",
		"PlayerFX.play": "ex-autre-cible",
		"PlayerFX.stop": "ex-autre-cible",
		"PlayerFX.stop_effect": "PlayerFX.play_effect",
		"PlayerFX.get_default_target": "PlayerFX.node_to_play_effects",
	}


## Objet (carré nommé) avec son nom affiché dessous.
func _objet(position: Vector2, couleur: Color, nom: String, cote: float = 90.0) -> Node2D:
	var objet := carre(position, couleur, cote)
	etiquette(position + Vector2(0, cote * 0.75 + 22), nom, 16)
	return objet


# ⚙️ Mode d'emploi

func _tuto_jouer() -> void:
	sujet("A. Jouer des effets sur un objet")
	arbre(Vector2(60, 160), PackedStringArray(["Piece (Sprite2D)", "  PlayerFX"]))
	etiquette(Vector2(190, 250), "special_effects = [FXShake, FXChangeSize]", 15, ACCENT)
	var piece := _objet(Vector2(720, 300), VARIANTES[1], "Pièce")
	var joueur := joueur_fx(piece, [effet("FXShake", {"duration": 0.5, "max_amplitude": 12.0}),
		effet("FXChangeSize", {"final_scale": 1.5, "duration": 0.3})])
	legende("Le PlayerFX joue ses effets sur son parent")
	await secondes(1.5)
	for i in 2:
		code("func _on_ramasse() -> void:\n\t$PlayerFX.play()")
		legende("play() : secousse (s'arrête seule) et agrandissement ensemble")
		joueur.play()
		await secondes(1.6)
		code("$PlayerFX.stop()")
		legende("stop() : l'agrandissement revient à la taille d'origine")
		joueur.stop()
		await secondes(1.2)


func _tuto_enchainer() -> void:
	sujet("B. Enchaîner des effets (recette réutilisable)")
	arbre(Vector2(60, 150), PackedStringArray(["FXShake  (duration 1 s)", "  effects_on_play_fx", "    FXFade  (rouge)", "  effects_on_stop_fx", "    FXFade  (vert)"]))
	var objet := _objet(Vector2(760, 300), Color.WHITE, "Caisse", 110)
	var flash := effet("FXFade", {"color": MAUVAIS, "duration": 0.15})
	var vert := effet("FXFade", {"color": BON, "duration": 0.3, "revert_on_stop": false})
	var secousse := effet("FXShake", {"duration": 1.0, "max_amplitude": 10.0})
	secousse.effects_on_play_fx = [flash]
	secousse.effects_on_stop_fx = [vert]
	var joueur := joueur_fx(objet, secousse)
	code("secousse.effects_on_play_fx = [flash]\nsecousse.effects_on_stop_fx = [vert]\n$PlayerFX.play()")
	for i in 2:
		objet.modulate = Color.WHITE
		await secondes(0.8)
		legende("Jeu : la secousse et le flash rouge, enchaîné à son jeu")
		joueur.play()
		await secondes(1.05)
		legende("Arrêt (fin de duration) : le flash s'arrête, l'effet d'arrêt passe la caisse au vert")
		await secondes(1.6)


func _tuto_script() -> void:
	sujet("C. Écrire son propre effet (GDScript)")
	code("class_name FXClignote\nextends Effect\n\nfunc _process_fx(delta, target):\n\t_temps += delta\n\ttarget.visible = fmod(_temps * frequence, 1.0) < 0.5\n\nfunc _stop_fx(target):\n\ttarget.visible = true")
	var objet := _objet(Vector2(300, 320), VARIANTES[2], "Bonus")
	var clignote := FXClignote.new()
	clignote.frequence = 6.0
	var joueur := joueur_fx(objet, clignote)
	for i in 2:
		legende("special_effects = [FXClignote] : play()")
		joueur.play()
		await secondes(2.0)
		legende("stop() : _stop_fx rend l'objet visible")
		joueur.stop()
		await secondes(1.0)


# 💻 API

func _classe_effect() -> void:
	sujet("Effect : cycle de vie")
	var objet := _objet(Vector2(260, 300), VARIANTES[0], "Cible")
	var lignes: Array[Label] = []
	var journal := func(texte: String) -> void:
		if lignes.size() >= 10:
			lignes.pop_front().queue_free()
			for ligne in lignes:
				ligne.position.y -= 30
		var label := etiquette(Vector2(760, 205 + lignes.size() * 30), texte, 17, ACCENT)
		for ligne in lignes:
			ligne.add_theme_color_override("font_color", TEXTE_DOUX)
		lignes.append(label)
	var suivi := FXJournal.new()
	suivi.journal = journal
	var secousse := effet("FXShake", {"duration": 0.0, "max_amplitude": 8.0})
	suivi.effects_on_play_fx = [secousse]
	suivi.fx_played.connect(func(): journal.call("signal fx_played"))
	suivi.fx_stopped.connect(func(): journal.call("signal fx_stopped"))
	code("effet.play_fx(cible)\n# à chaque image : effet.process_fx(delta, cible)\neffet.stop_fx(cible)")
	legende("play_fx → _play_fx, fx_played, puis les effets enchaînés (ici une secousse)")
	journal.call("play_fx(cible)")
	suivi.play_fx(objet)
	for i in 3:
		await secondes(0.5)
		journal.call("process_fx(delta, cible)")
		suivi.process_fx(0.5, objet)
		var depart := Time.get_ticks_msec()
		while Time.get_ticks_msec() - depart < 400:
			secousse.process_fx(get_process_delta_time(), objet)
			await images()
	legende("stop_fx → _stop_fx, les enchaînés s'arrêtent, puis fx_stopped")
	journal.call("stop_fx(cible)")
	suivi.stop_fx(objet)
	journal.call("is_activated() = %s" % suivi.is_activated())
	await secondes(2.0)


func _classe_player() -> void:
	sujet("PlayerFX : joue, met à jour et arrête une liste d'effets")
	arbre(Vector2(60, 160), PackedStringArray(["Lanterne (Sprite2D)", "  PlayerFX"]))
	etiquette(Vector2(200, 250), "special_effects = [FXChangeSize, FXFade, FXShake]", 15, ACCENT)
	var objet := _objet(Vector2(740, 300), Color.WHITE, "Lanterne", 100)
	var joueur := joueur_fx(objet, [effet("FXChangeSize", {"final_scale": 1.35, "duration": 0.35}),
		effet("FXFade", {"color": Color("ffd166"), "duration": 0.35}), effet("FXShake", {"duration": 0.0, "max_amplitude": 3.0})])
	for i in 2:
		await secondes(0.8)
		code("$PlayerFX.play()")
		legende("play() : les trois effets démarrent ensemble sur le parent")
		joueur.play()
		await secondes(1.8)
		code("$PlayerFX.stop()")
		legende("stop() : ils s'arrêtent ensemble, l'objet retrouve son état d'origine")
		joueur.stop()
		await secondes(0.8)


func _effects_on_play() -> void:
	sujet("effects_on_play_fx : joués et arrêtés avec l'effet")
	var gauche := _objet(Vector2(380, 280), Color.WHITE, "FXShake seul")
	var droite := _objet(Vector2(780, 280), Color.WHITE, "FXShake + FXFade enchaîné au jeu")
	var seul := joueur_fx(gauche, effet("FXShake", {"duration": 1.0, "max_amplitude": 10.0}))
	var secousse := effet("FXShake", {"duration": 1.0, "max_amplitude": 10.0})
	secousse.effects_on_play_fx = [effet("FXFade", {"color": MAUVAIS, "duration": 0.15})]
	var enchaine := joueur_fx(droite, secousse)
	code("secousse.effects_on_play_fx = [flash_rouge]")
	legende("Le flash rouge dure exactement autant que la secousse")
	await jouer_ensemble([seul, enchaine], 3, 1.8)


func _effects_on_stop() -> void:
	sujet("effects_on_stop_fx : joués à l'arrêt de l'effet")
	var objet := _objet(Vector2(576, 280), Color.WHITE, "Coffre", 110)
	var secousse := effet("FXShake", {"duration": 1.2, "max_amplitude": 8.0})
	secousse.effects_on_stop_fx = [effet("FXChangeSize", {"final_scale": 1.4, "duration": 0.25})]
	var joueur := joueur_fx(objet, secousse)
	code("secousse.effects_on_stop_fx = [FXChangeSize]  # « pop » final")
	for i in 3:
		objet.scale = Vector2.ONE
		await secondes(0.6)
		legende("Secousse...")
		joueur.play()
		await secondes(1.25)
		legende("... arrêt : l'agrandissement enchaîné à l'arrêt est joué (puis jamais arrêté)")
		await secondes(1.0)


func _node_to_play() -> void:
	sujet("node_to_play_effects : la cible par défaut")
	arbre(Vector2(60, 150), PackedStringArray(["Personnage", "  Corps", "  Bouclier", "  PlayerFX"]))
	var personnage := Node2D.new()
	personnage.position = Vector2(700, 300)
	scene.add_child(personnage)
	var corps := Formes.carre(personnage, Vector2(-70, 0), VARIANTES[0], 90)
	var bouclier := Formes.carre(personnage, Vector2(70, 0), VARIANTES[1], 70)
	etiquette(Vector2(630, 380), "Corps", 16)
	etiquette(Vector2(770, 380), "Bouclier", 16)
	var joueur := joueur_fx(personnage, effet("FXShake", {"duration": 0.6, "max_amplitude": 12.0}))
	for i in 2:
		joueur.node_to_play_effects = null
		code("node_to_play_effects = <vide>  # → le parent (Personnage)")
		legende("Vide : la cible est le parent du PlayerFX, tout le personnage tremble")
		await secondes(0.6)
		joueur.play()
		await secondes(1.4)
		joueur.node_to_play_effects = bouclier
		code("node_to_play_effects = $Bouclier")
		legende("Renseigné : seul le bouclier tremble")
		await secondes(0.6)
		joueur.play()
		await secondes(1.4)
	corps.name = "Corps"


# 📌 Exemples

func _ex_degats() -> void:
	sujet("Exemple : un retour de dégâts complet en un play()")
	var ennemi := _objet(Vector2(576, 280), Color.WHITE, "Ennemi", 110)
	var secousse := effet("FXShake", {"duration": 0.25, "max_amplitude": 6.0})
	secousse.effects_on_play_fx = [effet("FXFade", {"color": Color(1, 0.3, 0.3), "duration": 0.05})]
	var joueur := joueur_fx(ennemi, secousse)
	code("secousse.effects_on_play_fx = [flash, son]\n$PlayerFX.special_effects = [secousse]\n\nfunc toucher() -> void:\n\t$PlayerFX.play()")
	var epee := carre(Vector2(300, 280), Color("cfd6e6"), 20)
	epee.scale = Vector2(5, 0.6)
	for i in 4:
		await deplacer(epee, Vector2(480, 280), 0.18)
		legende("toucher() : secousse + flash")
		joueur.play()
		await deplacer(epee, Vector2(300, 280), 0.3)
		await secondes(0.4)


func _ex_autre_cible() -> void:
	sujet("Exemple : les mêmes effets sur une autre cible")
	var cartes: Array[Node2D] = []
	for i in 4:
		cartes.append(_objet(Vector2(270 + i * 200, 290), VARIANTES[i], "Carte %d" % (i + 1), 110))
	var porteur := Node.new()
	scene.add_child(porteur)
	var joueur := joueur_fx(porteur, [effet("FXChangeSize", {"final_scale": 1.25, "duration": 0.2}), effet("FXShake", {"duration": 0.3, "max_amplitude": 5.0})])
	var fleche := pointeur()
	fleche.position = Vector2(560, 560)
	code("func surligner(carte) -> void:\n\t$PlayerFX.stop()\n\t$PlayerFX.play(carte)")
	for i in [0, 2, 1, 3, 0]:
		await deplacer(fleche, cartes[i].position + Vector2(10, 20), 0.45)
		legende("surligner(%s)" % ("Carte %d" % (i + 1)))
		joueur.stop()
		joueur.play(cartes[i])
		await secondes(0.8)


func _ex_attendre() -> void:
	sujet("Exemple : réagir à la fin d'un effet (fx_stopped)")
	var levier := _objet(Vector2(320, 300), VARIANTES[1], "Levier", 80)
	var porte := carre(Vector2(800, 280), Formes.DECOR, 10)
	porte.scale = Vector2(14, 22)
	etiquette(Vector2(800, 420), "Porte", 16)
	var secousse := effet("FXShake", {"duration": 1.2, "max_amplitude": 9.0})
	var joueur := joueur_fx(levier, secousse)
	code("$PlayerFX.play()\nawait secousse.fx_stopped\nporte.ouvrir()")
	for i in 2:
		porte.position.y = 280
		await secondes(0.6)
		legende("Le levier tremble 1,2 s...")
		joueur.play()
		await secousse.fx_stopped
		legende("... fx_stopped : la porte s'ouvre")
		await deplacer(porte, Vector2(800, 120), 0.5)
		await secondes(1.0)


func _ex_sans_player() -> void:
	sujet("Exemple : piloter un effet sans PlayerFX")
	var icone := _objet(Vector2(400, 300), VARIANTES[3], "Icône")
	var vibre := effet("FXShake", {"duration": 0.0, "max_amplitude": 6.0})
	var etat := etiquette(Vector2(400, 430), "", 18, ACCENT)
	var actif := false
	get_tree().process_frame.connect(func():
		if actif:
			vibre.process_fx(get_process_delta_time(), icone))
	for i in 2:
		code("vibre.play_fx($Icone)")
		etat.text = "is_activated() = true"
		legende("play_fx(cible), puis process_fx(delta, cible) dans _process du script")
		vibre.play_fx(icone)
		actif = true
		await secondes(2.0)
		code("vibre.stop_fx($Icone)")
		vibre.stop_fx(icone)
		actif = false
		etat.text = "is_activated() = false"
		legende("stop_fx(cible) : position rendue")
		await secondes(1.2)


func _ex_pause() -> void:
	sujet("Exemple : des effets qui tournent pendant la pause")
	var objet := _objet(Vector2(300, 300), VARIANTES[0], "Objet du jeu (PROCESS_MODE_INHERIT)")
	var titre := _objet(Vector2(840, 300), VARIANTES[1], "Titre du menu (PROCESS_MODE_ALWAYS)")
	var effets := func(): return [effet("FXShake", {"duration": 0.0, "max_amplitude": 6.0})]
	var j_objet := joueur_fx(objet, effets.call())
	var j_titre := joueur_fx(titre, effets.call())
	j_titre.process_mode = Node.PROCESS_MODE_ALWAYS
	titre.process_mode = Node.PROCESS_MODE_ALWAYS
	j_objet.play()
	j_titre.play()
	code("$MenuPause/Titre/PlayerFX.process_mode = Node.PROCESS_MODE_ALWAYS")
	var etat := etiquette(Vector2(576, 160), "Jeu en cours", 22, BON)
	for i in 2:
		legende("En jeu : les deux effets tournent")
		await secondes(1.5)
		get_tree().paused = true
		etat.text = "get_tree().paused = true"
		etat.add_theme_color_override("font_color", MAUVAIS)
		legende("En pause : seul le PlayerFX en PROCESS_MODE_ALWAYS continue")
		await secondes(2.0)
		get_tree().paused = false
		etat.text = "Jeu en cours"
		etat.add_theme_color_override("font_color", BON)
