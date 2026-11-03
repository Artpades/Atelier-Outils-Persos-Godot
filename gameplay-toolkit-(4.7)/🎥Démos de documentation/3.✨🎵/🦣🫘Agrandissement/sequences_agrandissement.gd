extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## 🦣🫘 Plans filmés de la page Agrandissement / rétrécissement (effets/agrandissement-retrecissement.html).

const Sons := preload("res://🎥Démos de documentation/socle/sons.gd")


func titre_module() -> String:
	return "🦣🫘 Agrandissement · FXChangeSize"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-FXChangeSize": _classe,
		"FXChangeSize.duration": _duration,
		"FXChangeSize.final_scale": _final_scale,
		"FXChangeSize.original_value": _original_value,
		"FXChangeSize.transition": _transition,
		"FXChangeSize.ease_type": _ease_type,
		"ex-disparition": _ex_disparition,
		"ex-pulse": _ex_pulse,
		"ex-echelle": _ex_echelle,
		"ex-combo": _ex_combo,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Le pointeur survole les boutons : <code>mouse_entered</code> → <code>play()</code> (agrandir), <code>mouse_exited</code> → <code>stop()</code> (revenir).",
		"classe-FXChangeSize": "Le même effet sur un Node2D, un Control et un Node3D : agrandissement au jeu, retour à <code>original_value</code> à l'arrêt.",
		"FXChangeSize.duration": "0,15 s, 0,5 s et 1,2 s, à l'aller comme au retour. Courbe : échelle au cours du temps.",
		"FXChangeSize.final_scale": "0,5 (rétrécir), 1,2 et 2 : l'échelle atteinte au jeu.",
		"FXChangeSize.original_value": "Un objet au repos à l'échelle 2 : avec <code>original_value = 1</code> il rapetisse à l'arrêt, avec 2 il revient bien à sa taille.",
		"FXChangeSize.transition": "Linear, Sine, Back (défaut) et Elastic, en ease Out : la forme de la courbe d'échelle.",
		"FXChangeSize.ease_type": "Courbe Back en In, Out et In-Out : où se place le dépassement.",
		"ex-disparition": "Ramasser : léger gonflement (Back, ease In) puis disparition, et <code>queue_free()</code> après la durée.",
		"ex-pulse": "Jeu puis arrêt 0,15 s plus tard, en boucle : un battement.",
		"ex-echelle": "Un arbre à l'échelle 3 : <code>original_value</code> et <code>final_scale</code> sont calculés depuis son échelle.",
		"ex-combo": "Agrandissement au survol, avec un son de clic enchaîné (<code>effects_on_play_fx</code>).",
	}


func reutilisations() -> Dictionary:
	return {}


## Courbe de l'échelle x de `cible` (1 = axe central).
func _mesure_echelle(cible: Node) -> float:
	return (cible.scale.x - 1.0) * 30.0


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : un bouton qui grossit au survol")
	arbre(Vector2(60, 150), PackedStringArray(["Bouton (Button)", "  PlayerFX"]))
	etiquette(Vector2(200, 230), "special_effects = [FXChangeSize]\nfinal_scale = 1.15 · duration = 0.15", 15, ACCENT)
	var boutons: Array[Button] = []
	var joueurs: Array[PlayerFX] = []
	for i in 3:
		var b := bouton(Vector2(760, 230 + i * 100), ["Jouer", "Options", "Quitter"][i], Vector2(240, 64))
		boutons.append(b)
		joueurs.append(joueur_fx(b, effet("FXChangeSize", {"final_scale": 1.15, "duration": 0.15})))
	code("$Bouton.mouse_entered.connect($Bouton/PlayerFX.play)\n$Bouton.mouse_exited.connect($Bouton/PlayerFX.stop)")
	var fleche := pointeur()
	fleche.position = Vector2(1000, 560)
	var survole := -1
	for i in [0, 1, 2, 1, 0]:
		await deplacer(fleche, boutons[i].position + Vector2(150, 40), 0.45)
		if survole >= 0:
			joueurs[survole].stop()
		joueurs[i].play()
		survole = i
		legende("Survol de « %s » : play() · sortie : stop()" % boutons[i].text)
		await secondes(0.6)
	await deplacer(fleche, Vector2(1000, 560), 0.4)
	joueurs[survole].stop()
	legende("Plus de survol : retour à original_value (1)")
	await secondes(0.8)


# 💻 API

func _classe() -> void:
	sujet("FXChangeSize : Node2D, Control ou Node3D")
	var objet := carre(Vector2(220, 300), VARIANTES[0], 100)
	etiquette(Vector2(220, 430), "Node2D")
	var b := bouton(Vector2(576, 300), "Valider")
	etiquette(Vector2(576, 430), "Control")
	var vue := vue_3d(Rect2(790, 200, 300, 200))
	var bloc := cube(vue, Vector3(0, 0.7, 0), VARIANTES[1], Vector3.ONE)
	cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(6, 0.2, 6))
	etiquette(Vector2(940, 430), "Node3D")
	var joueurs := [joueur_fx(objet, effet("FXChangeSize", {"final_scale": 1.5})), joueur_fx(b, effet("FXChangeSize", {"final_scale": 1.3})),
		joueur_fx(bloc, effet("FXChangeSize", {"final_scale": 1.5}))]
	code("$Cible/PlayerFX.play()   # puis .stop()")
	legende("play() : agrandir · stop() : revenir à original_value")
	await jouer_ensemble(joueurs, 3, 1.6, 0.9)


func _duration() -> void:
	sujet("duration : durée de l'animation (s)")
	legende("Jeu, puis arrêt 1,4 s plus tard : même durée à l'aller et au retour")
	var joueurs := colonnes("FXChangeSize", "duration", [0.15, 0.5, 1.2], ["0,15 s", "0,5 s", "1,2 s"], {"final_scale": 1.6},
		_mesure_echelle, 1.0)
	code("taille.duration = 0.5")
	await jouer_ensemble(joueurs, 2, 2.8, 1.4)


func _final_scale() -> void:
	sujet("final_scale : échelle atteinte au jeu")
	legende("1 = taille de repos (original_value)")
	var joueurs := colonnes("FXChangeSize", "final_scale", [0.5, 1.2, 2.0], ["0,5", "1,2", "2"], {"duration": 0.4},
		_mesure_echelle, 1.0)
	code("taille.final_scale = 1.2")
	await jouer_ensemble(joueurs, 2, 2.2, 1.2)


func _original_value() -> void:
	sujet("original_value : échelle rejointe à l'arrêt")
	legende("La cible est au repos à l'échelle 2 : original_value n'est pas lu sur l'objet")
	var gauche := carre(Vector2(380, 270), MAUVAIS, 60)
	var droite := carre(Vector2(780, 270), BON, 60)
	for objet in [gauche, droite]:
		objet.scale = Vector2(2, 2)
	etiquette(Vector2(380, 400), "original_value = 1 (défaut)  ✗", 18, MAUVAIS)
	etiquette(Vector2(780, 400), "original_value = 2  ✓", 18, BON)
	var joueurs := [joueur_fx(gauche, effet("FXChangeSize", {"final_scale": 2.4, "original_value": 1.0, "duration": 0.35})),
		joueur_fx(droite, effet("FXChangeSize", {"final_scale": 2.4, "original_value": 2.0, "duration": 0.35}))]
	code("grossir.original_value = $Objet.scale.x   # 2.0")
	for i in 2:
		gauche.scale = Vector2(2, 2)
		await jouer_ensemble(joueurs, 1, 2.0, 1.0)


func _transition() -> void:
	sujet("transition : forme de la courbe (ease Out)")
	var valeurs := [Tween.TRANS_LINEAR, Tween.TRANS_SINE, Tween.TRANS_BACK, Tween.TRANS_ELASTIC]
	var joueurs := colonnes("FXChangeSize", "transition", valeurs, ["LINEAR", "SINE", "BACK (défaut)", "ELASTIC"],
		{"final_scale": 1.6, "duration": 0.8, "ease_type": Tween.EASE_OUT}, _mesure_echelle, 1.0)
	code("taille.transition = Tween.TRANS_BACK")
	await jouer_ensemble(joueurs, 2, 2.6, 1.3)


func _ease_type() -> void:
	sujet("ease_type : accélération de la courbe (transition Back)")
	var valeurs := [Tween.EASE_IN, Tween.EASE_OUT, Tween.EASE_IN_OUT]
	var joueurs := colonnes("FXChangeSize", "ease_type", valeurs, ["EASE_IN", "EASE_OUT (défaut)", "EASE_IN_OUT"],
		{"final_scale": 1.6, "duration": 0.8}, _mesure_echelle, 1.0)
	code("taille.ease_type = Tween.EASE_OUT")
	await jouer_ensemble(joueurs, 2, 2.6, 1.3)


# 📌 Exemples

func _ex_disparition() -> void:
	sujet("Exemple : rapetisser un objet ramassé, puis le supprimer")
	code("disparition.final_scale = 0.0\ndisparition.transition = Tween.TRANS_BACK\ndisparition.ease_type = Tween.EASE_IN\n$PlayerFX.play()\nawait get_tree().create_timer(disparition.duration).timeout\nqueue_free()")
	var heros := carre(Vector2(160, 380), VARIANTES[0], 50)
	for passage in 2:
		var pieces: Array[Node2D] = []
		for i in 4:
			var piece := disque(Vector2(330 + i * 180, 380), Color("ffd166"), 22)
			pieces.append(piece)
		heros.position.x = 160
		for piece in pieces:
			await deplacer(heros, Vector2(piece.position.x - 40, 380), 0.45)
			var joueur := joueur_fx(piece, effet("FXChangeSize", {"final_scale": 0.0, "duration": 0.25,
				"transition": Tween.TRANS_BACK, "ease_type": Tween.EASE_IN}))
			joueur.play()
			legende("ramasser() : gonfle, disparaît, puis queue_free()")
			get_tree().create_timer(0.25).timeout.connect(piece.queue_free)
		await secondes(0.6)


func _ex_pulse() -> void:
	sujet("Exemple : faire « pulser » un objet")
	var coeur := disque(Vector2(576, 290), MAUVAIS, 70)
	etiquette(Vector2(576, 420), "PV critiques !", 20, MAUVAIS)
	var joueur := joueur_fx(coeur, effet("FXChangeSize", {"final_scale": 1.25, "duration": 0.15}))
	code("func pulser() -> void:\n\t$PlayerFX.play()\n\tawait get_tree().create_timer(0.15).timeout\n\t$PlayerFX.stop()")
	legende("pulser() appelé toutes les 0,6 s")
	for i in 10:
		joueur.play()
		await secondes(0.15)
		joueur.stop()
		await secondes(0.45)


func _ex_echelle() -> void:
	sujet("Exemple : un objet qui n'est pas à l'échelle 1")
	var arbre_noeud := Node2D.new()
	arbre_noeud.position = Vector2(576, 420)
	arbre_noeud.scale = Vector2(3, 3)
	scene.add_child(arbre_noeud)
	var tronc := Polygon2D.new()
	tronc.polygon = PackedVector2Array([Vector2(-5, 0), Vector2(5, 0), Vector2(5, -25), Vector2(-5, -25)])
	tronc.color = Color("8a5a3c")
	arbre_noeud.add_child(tronc)
	var feuillage := Polygon2D.new()
	feuillage.polygon = PackedVector2Array([Vector2(-28, -22), Vector2(28, -22), Vector2(0, -75)])
	feuillage.color = BON
	arbre_noeud.add_child(feuillage)
	var echelle := etiquette(Vector2(576, 470), "", 18, ACCENT)
	var joueur := joueur_fx(arbre_noeud, effet("FXChangeSize", {"original_value": 3.0, "final_scale": 3.3, "duration": 0.3}))
	code("grossir.original_value = $Arbre.scale.x        # 3.0\ngrossir.final_scale = $Arbre.scale.x * 1.1      # 3.3")
	get_tree().process_frame.connect(func(): echelle.text = "scale.x = %.2f" % arbre_noeud.scale.x)
	legende("Le vent : l'arbre grossit de 10 %, puis revient exactement à l'échelle 3")
	await jouer_ensemble([joueur], 4, 1.5, 0.6)


func _ex_combo() -> void:
	sujet("Exemple : agrandissement et son ensemble")
	var b := bouton(Vector2(576, 290), "Jouer", Vector2(260, 70))
	var grossir := effet("FXChangeSize", {"final_scale": 1.2, "duration": 0.15})
	var clic: FXAudio = effet("FXAudio", {"stream": Sons.bip(1200.0, 0.08)})
	grossir.effects_on_play_fx = [clic]
	var joueur := joueur_fx(b, grossir)
	var vumetre := Sons.Vumetre.new()
	vumetre.position = Vector2(456, 430)
	scene.add_child(vumetre)
	code("grossir.effects_on_play_fx = [clic]   # FXAudio\n$Bouton/PlayerFX.special_effects = [grossir]")
	var fleche := pointeur()
	fleche.position = Vector2(900, 520)
	for i in 3:
		await deplacer(fleche, Vector2(590, 300), 0.5)
		legende("Survol : le bouton grossit et le clic sonne en même temps")
		joueur.play()
		await secondes(0.7)
		await deplacer(fleche, Vector2(900, 520), 0.4)
		joueur.stop()
		await secondes(0.3)
