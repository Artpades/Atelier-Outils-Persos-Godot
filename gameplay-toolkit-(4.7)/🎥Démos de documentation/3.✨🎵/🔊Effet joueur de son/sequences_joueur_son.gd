extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## 🔊 Plans filmés (avec le son) de la page Effet joueur de son (effets/effet-joueur-de-son.html) : FXAudio.

const Sons := preload("res://🎥Démos de documentation/socle/sons.gd")


func titre_module() -> String:
	return "🔊 Effet joueur de son · FXAudio"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-FXAudio": _classe,
		"FXAudio.stream": _stream,
		"FXAudio.pitch_scale": _pitch_scale,
		"FXAudio.randomize_pitch_scale": _randomize,
		"FXAudio.volume_db": _volume,
		"FXAudio.positional": _positional,
		"FXAudio.stop_sound_on_stop": _stop_sound,
		"ex-rafale": _ex_rafale,
		"ex-mort": _ex_mort,
		"ex-variantes": _ex_variantes,
		"ex-boucle": _ex_boucle,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Un FXAudio dans les effets enchaînés d'une secousse : à chaque coup, le son avec une hauteur tirée au hasard (avec le son).",
		"classe-FXAudio": "Un son par objet ; avec <code>positional</code>, celui de gauche s'entend à gauche et celui de droite à droite.",
		"FXAudio.stream": "Sans stream l'effet ne fait rien ; avec un stream, il le joue à chaque <code>play()</code>.",
		"FXAudio.pitch_scale": "0,5 (une octave plus bas, plus lent), 1 et 2 (une octave plus haut).",
		"FXAudio.randomize_pitch_scale": "Six pas : hauteur fixe (mécanique) puis tirée entre <code>min_pitch</code> 0,9 et <code>max_pitch</code> 1,15.",
		"FXAudio.volume_db": "-20 dB, -8 dB et 0 dB : le VU-mètre montre le niveau de chaque son.",
		"FXAudio.positional": "Un objet traverse l'écran : positionnel, le son passe de gauche à droite et s'atténue ; sinon il reste au centre.",
		"FXAudio.stop_sound_on_stop": "Le même son long, effet arrêté au bout de 0,5 s : coupé avec true, joué jusqu'au bout avec false.",
		"ex-rafale": "Tirs rapprochés : une seule voix coupe le son précédent, <code>max_polyphony = 4</code> les superpose.",
		"ex-mort": "Joué sur l'ennemi supprimé, le son est coupé ; joué sur la scène, il va jusqu'au bout.",
		"ex-variantes": "Un AudioStreamRandomizer de trois pas : le son lui-même change, pas seulement sa hauteur.",
		"ex-boucle": "Alarme en boucle tant que l'état dure : <code>stop_sound_on_stop</code> la coupe à <code>stop()</code>.",
	}


func schemas() -> Dictionary:
	return {
		"lecteur": ["lecteur.svg", "Au premier jeu, FXAudio crée un lecteur sous la cible (AudioStreamPlayer, 2D ou 3D selon la cible et <code>positional</code>) : <code>get_audio_player()</code> le renvoie pour le régler finement."],
		"FXAudio.bus": ["bus.svg", "Le son passe par le bus choisi (ex. SFX), lui-même mixé dans Master : un réglage « Bruitages » agit sur tous les sons de ce bus."],
	}


func reutilisations() -> Dictionary:
	return {
		"FXAudio.min_pitch": "FXAudio.randomize_pitch_scale",
		"FXAudio.max_pitch": "FXAudio.randomize_pitch_scale",
		"FXAudio.get_audio_player": "lecteur",
	}


## Objet sonore : carré + VU-mètre global + notes qui montent à chaque son.
func _objet(position: Vector2, couleur: Color, nom: String, cote: float = 80.0) -> Node2D:
	var objet := carre(position, couleur, cote)
	etiquette(position + Vector2(0, cote * 0.75 + 20), nom, 16)
	return objet


func _vumetre(position: Vector2 = Vector2(456, 540)) -> void:
	var vumetre := Sons.Vumetre.new()
	vumetre.position = position
	scene.add_child(vumetre)


## Note de musique qui s'envole au-dessus de `cible` (montre qu'un son part), avec un texte facultatif.
func _note(cible: Node2D, texte: String = "") -> void:
	var label := etiquette(cible.global_position + Vector2(0, -70), "♪ " + texte, 22, ACCENT)
	var animation := label.create_tween().set_parallel()
	animation.tween_property(label, "position:y", label.position.y - 50, 0.8)
	animation.tween_property(label, "modulate:a", 0.0, 0.8).set_delay(0.3)
	animation.chain().tween_callback(label.queue_free)


func _audio(reglages: Dictionary) -> FXAudio:
	return effet("FXAudio", reglages)


func _hauteur(son: FXAudio) -> String:
	var lecteur = son.get_audio_player()
	return "pitch %.2f" % lecteur.pitch_scale if lecteur else ""


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : un son à chaque coup")
	arbre(Vector2(60, 150), PackedStringArray(["Ennemi", "  PlayerFX"]))
	etiquette(Vector2(220, 240), "FXShake\n  effects_on_play_fx = [FXAudio]\n    stream, bus, randomize_pitch_scale", 15, ACCENT)
	var ennemi := _objet(Vector2(760, 300), MAUVAIS.lightened(0.3), "Ennemi", 100)
	var son := _audio({"stream": Sons.impact(), "randomize_pitch_scale": true, "min_pitch": 0.85, "max_pitch": 1.2})
	var secousse := effet("FXShake", {"duration": 0.25, "max_amplitude": 8.0})
	secousse.effects_on_play_fx = [son]
	var joueur := joueur_fx(ennemi, secousse)
	_vumetre()
	code("son.stream = preload(\"res://sons/impact.wav\")\nson.randomize_pitch_scale = true\nsecousse.effects_on_play_fx = [son]")
	for i in 6:
		await secondes(0.6)
		joueur.play()
		_note(ennemi, _hauteur(son))
		legende("Coup n°%d : même son, hauteur légèrement différente" % (i + 1))


# 💻 API

func _classe() -> void:
	sujet("FXAudio : un son au déclenchement")
	var objets: Array[Node2D] = []
	var joueurs: Array[PlayerFX] = []
	var reglages := [{"positional": true}, {}, {"positional": true}]
	var noms := ["Gauche (positional)", "Centre", "Droite (positional)"]
	var notes := [440.0, 660.0, 880.0]
	for i in 3:
		var objet := _objet(Vector2(150 + i * 426, 300), VARIANTES[i], noms[i])
		var r: Dictionary = reglages[i].duplicate()
		r["stream"] = Sons.bip(notes[i], 0.35)
		joueurs.append(joueur_fx(objet, _audio(r)))
		objets.append(objet)
	_vumetre()
	code("son.stream = mon_son\nson.positional = true\n$Objet/PlayerFX.play()")
	for tour in 2:
		for i in 3:
			await secondes(0.7)
			legende(noms[i])
			joueurs[i].play()
			_note(objets[i])


func _stream() -> void:
	sujet("stream : le son joué")
	var vide := _objet(Vector2(380, 300), Formes.DECOR, "stream = null : rien")
	var plein := _objet(Vector2(780, 300), VARIANTES[0], "stream = impact.wav")
	var joueurs := [joueur_fx(vide, _audio({})), joueur_fx(plein, _audio({"stream": Sons.impact()}))]
	_vumetre()
	code("son.stream = preload(\"res://sons/impact.wav\")")
	for i in 4:
		await secondes(0.8)
		var cible := i % 2
		joueurs[cible].play()
		legende("play() sur « %s »" % ["stream = null", "stream = impact.wav"][cible])
		if cible == 1:
			_note(plein)


func _pitch_scale() -> void:
	sujet("pitch_scale : hauteur et vitesse de lecture")
	var valeurs := [0.5, 1.0, 2.0]
	var joueurs: Array[PlayerFX] = []
	var objets: Array[Node2D] = []
	for i in 3:
		var objet := _objet(Vector2(288 + i * 288, 300), VARIANTES[i], "pitch_scale = %s" % valeurs[i])
		joueurs.append(joueur_fx(objet, _audio({"stream": Sons.melodie(PackedFloat32Array([523.0, 659.0, 784.0]), 0.18), "pitch_scale": valeurs[i]})))
		objets.append(objet)
	_vumetre()
	code("son.pitch_scale = 2.0  # une octave plus haut")
	for tour in 2:
		for i in 3:
			await secondes(0.4)
			joueurs[i].play()
			_note(objets[i])
			await secondes(1.2 / valeurs[i] if valeurs[i] < 1.0 else 0.8)


func _randomize() -> void:
	sujet("randomize_pitch_scale, min_pitch, max_pitch")
	var heros := _objet(Vector2(200, 330), VARIANTES[0], "")
	var son := _audio({"stream": Sons.pas(0)})
	var joueur := joueur_fx(heros, son)
	_vumetre()
	for aleatoire in [false, true]:
		son.randomize_pitch_scale = aleatoire
		son.min_pitch = 0.9
		son.max_pitch = 1.15
		code("son.randomize_pitch_scale = %s\nson.min_pitch = 0.9\nson.max_pitch = 1.15" % aleatoire)
		legende("Hauteur tirée entre 0,9 et 1,15 à chaque pas" if aleatoire else "Hauteur fixe : répétition mécanique")
		heros.position.x = 200
		for pas in 6:
			await deplacer(heros, heros.position + Vector2(120, 0), 0.32)
			joueur.play()
			_note(heros, _hauteur(son))
		await secondes(0.6)


func _volume() -> void:
	sujet("volume_db : volume en décibels")
	var valeurs := [-20.0, -8.0, 0.0]
	var joueurs: Array[PlayerFX] = []
	var objets: Array[Node2D] = []
	for i in 3:
		var objet := _objet(Vector2(288 + i * 288, 300), VARIANTES[i], "volume_db = %d" % valeurs[i])
		joueurs.append(joueur_fx(objet, _audio({"stream": Sons.bip(660.0, 0.4, 0.9), "volume_db": valeurs[i]})))
		objets.append(objet)
	_vumetre()
	code("son.volume_db = -8.0")
	for tour in 2:
		for i in 3:
			await secondes(0.8)
			joueurs[i].play()
			_note(objets[i])


func _positional() -> void:
	sujet("positional : son placé à la position de la cible")
	_vumetre()
	for positionnel in [true, false]:
		var objet := _objet(Vector2(60, 300), VARIANTES[0] if positionnel else VARIANTES[1], "")
		var son := _audio({"stream": Sons.bip(700.0, 0.12), "positional": positionnel})
		var joueur := joueur_fx(objet, son)
		code("son.positional = %s" % positionnel)
		legende("positional = true : le son suit l'objet de gauche à droite" if positionnel else "positional = false : le son reste au centre, au même volume")
		for i in 9:
			joueur.play()
			_note(objet)
			await deplacer(objet, objet.position + Vector2(130, 0), 0.33)
		objet.queue_free()
		await secondes(0.4)


func _stop_sound() -> void:
	sujet("stop_sound_on_stop : couper le son à l'arrêt ?")
	var objets: Array[Node2D] = []
	var joueurs: Array[PlayerFX] = []
	for i in 2:
		var objet := _objet(Vector2(380 + i * 400, 300), VARIANTES[i], ["stop_sound_on_stop = true", "false (défaut)"][i])
		joueurs.append(joueur_fx(objet, _audio({"stream": Sons.melodie(PackedFloat32Array([392.0, 494.0, 587.0, 784.0, 587.0, 494.0]), 0.25), "stop_sound_on_stop": i == 0})))
		objets.append(objet)
	_vumetre()
	code("son.stop_sound_on_stop = true\n$PlayerFX.play()\n$PlayerFX.stop()  # 0,5 s plus tard")
	for i in 2:
		await secondes(0.5)
		legende("play() puis stop() 0,5 s plus tard : « %s »" % ["true", "false"][i])
		joueurs[i].play()
		_note(objets[i])
		await secondes(0.5)
		joueurs[i].stop()
		await secondes(1.4)


# 📌 Exemples

func _ex_rafale() -> void:
	sujet("Exemple : des sons rapprochés qui ne se coupent pas")
	_vumetre()
	for polyphonie in [1, 4]:
		var arme := _objet(Vector2(260, 300), VARIANTES[0] if polyphonie == 1 else VARIANTES[3], "max_polyphony = %d" % polyphonie)
		var tir := _audio({"stream": Sons.melodie(PackedFloat32Array([880.0, 660.0, 440.0]), 0.12)})
		var joueur := joueur_fx(arme, tir)
		code("$Arme/PlayerFX.play()\nvar lecteur := tir.get_audio_player()\nlecteur.max_polyphony = %d" % polyphonie)
		legende("Une voix : chaque tir coupe le précédent" if polyphonie == 1 else "Quatre voix : les tirs se superposent")
		for i in 6:
			joueur.play()
			var lecteur = tir.get_audio_player()
			if lecteur:
				lecteur.max_polyphony = polyphonie
			var balle := disque(arme.position + Vector2(50, 0), ACCENT, 6)
			deplacer(balle, Vector2(1150, 300), 0.6)
			get_tree().create_timer(0.6).timeout.connect(balle.queue_free)
			await secondes(0.15)
		await secondes(1.0)
		arme.queue_free()


func _ex_mort() -> void:
	sujet("Exemple : le son de mort d'un ennemi supprimé")
	_vumetre()
	var son_mort := Sons.melodie(PackedFloat32Array([660.0, 523.0, 392.0, 262.0]), 0.22)
	for sur_scene in [false, true]:
		var ennemi := _objet(Vector2(576, 300), MAUVAIS.lightened(0.3), "")
		var joueur := joueur_fx(ennemi, _audio({"stream": son_mort}))
		code("$PlayerFX.play(get_tree().current_scene)\nqueue_free()" if sur_scene else "$PlayerFX.play()   # lecteur créé sous l'ennemi\nqueue_free()")
		legende("Sur la scène : le son survit à l'ennemi" if sur_scene else "Sur l'ennemi : supprimé, il emporte son lecteur, le son est coupé")
		await secondes(0.8)
		joueur.play(scene if sur_scene else null)
		_note(ennemi)
		await secondes(0.2)
		if sur_scene:
			joueur.reparent(scene)
		ennemi.queue_free()
		await secondes(1.6)


func _ex_variantes() -> void:
	sujet("Exemple : varier le son lui-même")
	var variantes := AudioStreamRandomizer.new()
	for i in 3:
		variantes.add_stream(i, Sons.pas(i))
	var heros := _objet(Vector2(200, 330), VARIANTES[2], "")
	var joueur := joueur_fx(heros, _audio({"stream": variantes}))
	_vumetre()
	code("var variantes := AudioStreamRandomizer.new()\nvariantes.add_stream(0, pas_1)\nvariantes.add_stream(1, pas_2)\nvariantes.add_stream(2, pas_3)\npas.stream = variantes")
	legende("Chaque pas tire un des trois sons")
	for tour in 2:
		heros.position.x = 200
		for i in 6:
			await deplacer(heros, heros.position + Vector2(120, 0), 0.3)
			joueur.play()
			_note(heros)


func _ex_boucle() -> void:
	sujet("Exemple : un son en boucle tant qu'un état dure")
	var lampe := _objet(Vector2(576, 300), Formes.DECOR, "Alarme", 100)
	var etat := etiquette(Vector2(576, 180), "", 22, MAUVAIS)
	var alarme := _audio({"stream": Sons.alarme(), "stop_sound_on_stop": true})
	var clignote := effet("FXFade", {"color": MAUVAIS, "duration": 0.3})
	var joueur := joueur_fx(lampe, [alarme, clignote])
	_vumetre()
	code("alarme.stream = preload(\"res://sons/alarme_boucle.ogg\")\nalarme.stop_sound_on_stop = true\n$PlayerFX.play()   # ... $PlayerFX.stop()")
	for i in 2:
		await secondes(0.8)
		etat.text = "ALARME"
		legende("play() : l'alarme sonne en boucle")
		joueur.play()
		await secondes(2.4)
		etat.text = ""
		legende("stop() : le son est coupé net")
		joueur.stop()
		await secondes(0.8)
