extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## 🌈 Plans filmés de la page Fondu couleur (effets/fondu-couleur.html) : FXFade et ScreenFader.


func titre_module() -> String:
	return "🌈 Fondu couleur · FXFade, ScreenFader"


func plans() -> Dictionary:
	return {
		"mode-emploi-tuto-objet": _tuto_objet,
		"mode-emploi-tuto-ecran": _tuto_ecran,
		"classe-FXFade": _classe_fade,
		"classe-ScreenFader": _classe_fader,
		"FXFade.color": _fade_color,
		"FXFade.color_property": _fade_color_property,
		"FXFade.duration": _fade_duration,
		"FXFade.mode": _fade_mode,
		"FXFade.revert_on_stop": _fade_revert,
		"FXFade.transition": _fade_transition,
		"ScreenFader.color": _fader_color,
		"ScreenFader.texture": _fader_texture,
		"ScreenFader.mask": _fader_mask,
		"ScreenFader.mask_smoothness": _fader_smoothness,
		"ScreenFader.progress": _fader_progress,
		"ScreenFader.always_process": _fader_always_process,
		"ScreenFader.block_input": _fader_block_input,
		"ScreenFader.fade_to": _fader_fade_to,
		"ScreenFader.fade_finished": _fader_signaux,
		"ex-disparition": _ex_disparition,
		"ex-texture": _ex_texture,
		"ex-lumiere": _ex_lumiere,
		"ex-enchainement": _ex_enchainement,
		"ex-iris": _ex_iris,
		"ex-pause": _ex_pause,
		"ex-await": _ex_await,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi-tuto-objet": "Touché : <code>play()</code> teinte le sprite en rouge, <code>stop()</code> 0,15 s plus tard le ramène à sa couleur.",
		"mode-emploi-tuto-ecran": "<code>fade_out()</code>, <code>await faded_out</code>, changement de scène sous le voile, puis <code>fade_in()</code>.",
		"classe-FXFade": "FXFade anime une propriété Color (ici <code>modulate</code>) ou remplace une texture en fondu.",
		"classe-ScreenFader": "ScreenFader couvre puis découvre tout l'écran : uni, puis avec des masques.",
		"FXFade.color": "Rouge (flash), blanc transparent (disparition, valeur par défaut) et (2, 2, 2) surexposé (flash lumineux).",
		"FXFade.color_property": "<code>modulate</code> teinte le nœud et ses enfants, <code>self_modulate</code> seulement le nœud.",
		"FXFade.duration": "0,1 s, 0,5 s et 1,5 s, à l'aller comme au retour.",
		"FXFade.mode": "FADE_TEXTURE : les deux textures sont réellement mélangées pendant le fondu.",
		"FXFade.revert_on_stop": "À l'arrêt : true refait le fondu inverse, false garde la nouvelle couleur.",
		"FXFade.transition": "Linear, Sine (défaut) et Bounce : la forme de la courbe ; ease_type règle où elle accélère.",
		"ScreenFader.color": "Noir (défaut), blanc et rouge ; l'alpha de la couleur limite l'opacité maximale.",
		"ScreenFader.texture": "Une image (motif) à la place de la couleur unie, teintée par <code>color</code>.",
		"ScreenFader.mask": "Sans masque, volet, iris, losanges et dissolution : les pixels sombres du masque se couvrent en premier.",
		"ScreenFader.mask_smoothness": "0,02 (bord net) et 0,4 (bord flou), avec le même volet.",
		"ScreenFader.progress": "<code>progress</code> modifié directement (0 → 1) : utile avec un AnimationPlayer ou une jauge.",
		"ScreenFader.always_process": "Jeu en pause et <code>time_scale</code> bas : avec <code>always_process</code> le fondu avance quand même.",
		"ScreenFader.block_input": "Écran couvert : avec <code>block_input</code> le clic est intercepté, sans lui il atteint le bouton.",
		"ScreenFader.fade_to": "<code>fade_to(0.6)</code> : voile partiel, puis <code>fade_in()</code>.",
		"ScreenFader.fade_finished": "Les signaux au fil des fondus : <code>fade_finished(progress)</code> à chaque fin, puis <code>faded_out</code> ou <code>faded_in</code>.",
		"ex-disparition": "L'ennemi vaincu s'efface en 0,6 s (blanc transparent), puis <code>queue_free()</code>.",
		"ex-texture": "Mode texture : la porte s'ouvre en fondu enchaîné (et le reste, <code>revert_on_stop = false</code>).",
		"ex-lumiere": "<code>color_property = \"light_color\"</code> : une OmniLight3D passe au rouge tant que l'alarme est active.",
		"ex-enchainement": "Flash surexposé (2, 2, 2) avec une secousse enchaînée à son jeu.",
		"ex-iris": "Un GradientTexture2D radial comme <code>mask</code> : l'iris se ferme des bords vers le centre.",
		"ex-pause": "Menu de pause : <code>fade_to(0.6)</code> pendant la pause, le menu reste cliquable (<code>block_input = false</code>).",
		"ex-await": "Fondu instantané : le signal partirait avant le <code>await</code> ; tester <code>is_covered()</code> d'abord évite le blocage.",
	}


func schemas() -> Dictionary:
	return {
		"figure-2": ["installation.svg", "Le dossier <code>bin/fx/</code> dans le FileSystem, puis ScreenFader dans la boîte « Ajouter un nœud » (recherche « Fader »)."],
		"etats": ["etats.svg", "<code>is_fading()</code> est vrai pendant un fondu, <code>is_covered()</code> quand <code>progress</code> vaut 1 ; chaque fondu se termine par <code>fade_finished</code> puis <code>faded_out</code> / <code>faded_in</code>."],
	}


func reutilisations() -> Dictionary:
	return {
		"FXFade.texture": "FXFade.mode",
		"FXFade.texture_property": "FXFade.mode",
		"FXFade.ease_type": "FXFade.transition",
		"ScreenFader.duration": ["classe-ScreenFader", "<code>duration</code> : durée des fondus appelés sans durée (ici 0,8 s)."],
		"ScreenFader.transition": ["FXFade.transition", "Mêmes courbes que FXFade : <code>transition</code> et <code>ease_type</code>."],
		"ScreenFader.ease_type": ["FXFade.transition", "Mêmes courbes que FXFade : <code>transition</code> et <code>ease_type</code>."],
		"ScreenFader.fade_in": "mode-emploi-tuto-ecran",
		"ScreenFader.fade_out": "mode-emploi-tuto-ecran",
		"ScreenFader.is_covered": "etats",
		"ScreenFader.is_fading": "etats",
		"ScreenFader.faded_in": "ScreenFader.fade_finished",
		"ScreenFader.faded_out": "ScreenFader.fade_finished",
	}


## Sprite2D blanc (personnage simple) centré sur `position`.
var _fleche: Node2D


func _sprite(position: Vector2, taille: Vector2i = Vector2i(90, 120)) -> Sprite2D:
	var sprite := Sprite2D.new()
	sprite.texture = Formes.dessin(taille, Color("e8ecf4"), [[Rect2i(taille.x / 4, taille.y / 5, 10, 10), Color("1b1f2a")],
		[Rect2i(taille.x * 3 / 4 - 10, taille.y / 5, 10, 10), Color("1b1f2a")]])
	sprite.position = position
	scene.add_child(sprite)
	return sprite


## Petite « scène » de jeu (décor coloré + nom) dans une vue ; renvoie la racine pour la remplacer.
func _scene_jeu(vue: Node, nom: String, couleur: Color) -> Node2D:
	var racine := Node2D.new()
	for i in 6:
		var bloc := Polygon2D.new()
		bloc.polygon = Formes.rectangle(Vector2(90, 60 + i * 25))
		bloc.position = Vector2(-300 + i * 120, 100 - i * 12)
		bloc.color = couleur.darkened(i * 0.08)
		racine.add_child(bloc)
	var titre := Formes.etiquette(racine, Vector2(-120, -150), nom, 32, Formes.TEXTE)
	titre.size.x = 240
	titre.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vue.add_child(racine)
	return racine


## ScreenFader placé dans une vue (il ne couvre alors que cette vue).
func _fader(vue: Node, reglages: Dictionary = {}) -> ScreenFader:
	var fader := ScreenFader.new()
	for propriete in reglages:
		fader.set(propriete, reglages[propriete])
	vue.add_child(fader)
	return fader


## Vues côte à côte (largeur totale 1040) avec leur scène et leur ScreenFader réglé par `reglages[i]`.
func _vues_fader(reglages: Array, textes: Array) -> Array[ScreenFader]:
	var faders: Array[ScreenFader] = []
	var n := reglages.size()
	var largeur := (1040.0 - (n - 1) * 16.0) / n
	var hauteur := minf(largeur * 0.62, 330.0)
	for i in n:
		var cadre := Rect2(56 + i * (largeur + 16), 130, largeur, hauteur)
		var vue := vue_2d(cadre)
		vue.get_meta("camera").zoom = Vector2.ONE * (largeur / 760.0)
		_scene_jeu(vue, "", VARIANTES[i % VARIANTES.size()])
		faders.append(_fader(vue, reglages[i]))
		etiquette(Vector2(cadre.get_center().x, cadre.end.y + 24), textes[i], 17, VARIANTES[i % VARIANTES.size()])
	return faders


func _fondu_aller_retour(faders: Array, repetitions: int = 2, couvert: float = 0.6) -> void:
	for i in repetitions:
		await secondes(0.5)
		for fader in faders:
			fader.fade_out()
		await secondes(faders[0].duration + couvert)
		for fader in faders:
			fader.fade_in()
		await secondes(faders[0].duration + 0.4)


# ⚙️ Mode d'emploi

func _tuto_objet() -> void:
	sujet("A. Faire flasher un objet en rouge (FXFade)")
	arbre(Vector2(60, 150), PackedStringArray(["Heros (Sprite2D)", "  PlayerFX"]))
	etiquette(Vector2(210, 240), "FXFade : mode Color, color rouge,\nduration 0.1", 15, ACCENT)
	var heros := _sprite(Vector2(700, 300))
	var joueur := joueur_fx(heros, effet("FXFade", {"color": Color.RED, "duration": 0.1}))
	code("func _on_touche() -> void:\n\t$PlayerFX.play()\n\tawait get_tree().create_timer(0.15).timeout\n\t$PlayerFX.stop()")
	var balle := disque(Vector2(1100, 300), ACCENT, 12)
	for i in 4:
		balle.position = Vector2(1100, 300)
		await deplacer(balle, Vector2(750, 300), 0.4)
		legende("_on_touche() : flash rouge aller-retour")
		joueur.play()
		balle.position.x = 2000
		await secondes(0.15)
		joueur.stop()
		await secondes(0.5)


func _tuto_ecran() -> void:
	sujet("B. Passer au noir entre deux scènes (ScreenFader)")
	var vue := vue_2d(Rect2(156, 120, 840, 400))
	var fader := _fader(vue, {"duration": 0.5})
	var scenes := [["Niveau 1", VARIANTES[0]], ["Niveau 2", VARIANTES[3]], ["Niveau 3", VARIANTES[2]]]
	var courante := _scene_jeu(vue, scenes[0][0], scenes[0][1])
	code("Fondu.fade_out()\nawait Fondu.faded_out\nget_tree().change_scene_to_file(chemin)\nFondu.fade_in()")
	for i in [1, 2]:
		await secondes(1.0)
		legende("fade_out() : l'écran se couvre en 0,5 s")
		fader.fade_out()
		await fader.faded_out
		legende("Écran noir : changement de scène, invisible pour le joueur")
		courante.queue_free()
		courante = _scene_jeu(vue, scenes[i][0], scenes[i][1])
		await secondes(0.5)
		legende("fade_in() : la nouvelle scène apparaît")
		fader.fade_in()
		await fader.faded_in
	await secondes(0.8)


# 💻 API : FXFade

func _classe_fade() -> void:
	sujet("FXFade : couleur ou texture")
	var heros := _sprite(Vector2(330, 290))
	etiquette(Vector2(330, 400), "mode Color (modulate)")
	var panneau_texture := Sprite2D.new()
	panneau_texture.texture = Formes.dessin(Vector2i(140, 140), Color("4ea1ff"), [[Rect2i(20, 20, 100, 100), Color("1b1f2a")]])
	panneau_texture.position = Vector2(820, 290)
	scene.add_child(panneau_texture)
	etiquette(Vector2(820, 400), "mode Texture")
	var autre := Formes.dessin(Vector2i(140, 140), Color("c678dd"), [[Rect2i(50, 0, 40, 140), Color("ffd166")], [Rect2i(0, 50, 140, 40), Color("ffd166")]])
	var joueurs := [joueur_fx(heros, effet("FXFade", {"color": Color("ff9f43"), "duration": 0.5})),
		joueur_fx(panneau_texture, effet("FXFade", {"mode": FXFade.FADE_TEXTURE, "texture": autre, "duration": 0.8}))]
	code("$PlayerFX.play()  # fondu aller\n$PlayerFX.stop()  # fondu retour (revert_on_stop)")
	await jouer_ensemble(joueurs, 3, 2.2, 1.2)


func _fade_colonnes(propriete: String, valeurs: Array, textes: Array, communs: Dictionary = {}) -> Array[PlayerFX]:
	return colonnes("FXFade", propriete, valeurs, textes, communs, Callable(), 1.0, func(position: Vector2, _c: Color) -> Node:
		return _sprite(position))


func _fade_color() -> void:
	sujet("color : couleur atteinte (mode Color)")
	var joueurs := _fade_colonnes("color", [Color.RED, Color(1, 1, 1, 0), Color(2, 2, 2)], ["Color.RED", "Color(1, 1, 1, 0) (défaut)", "Color(2, 2, 2)"], {"duration": 0.4})
	code("fondu.color = Color.RED")
	await jouer_ensemble(joueurs, 3, 2.0, 1.0)


func _fade_color_property() -> void:
	sujet("color_property : la propriété Color animée")
	var joueurs: Array[PlayerFX] = []
	for i in 2:
		var parent := _sprite(Vector2(380 + i * 400, 270))
		var enfant := Sprite2D.new()
		enfant.texture = Formes.dessin(Vector2i(40, 40), Color("ffd166"))
		enfant.position = Vector2(70, -40)
		parent.add_child(enfant)
		etiquette(Vector2(380 + i * 400, 380), ["\"modulate\" (défaut) : nœud + enfants", "\"self_modulate\" : le nœud seul"][i], 18, VARIANTES[i])
		joueurs.append(joueur_fx(parent, effet("FXFade", {"color": Color("4ea1ff"), "duration": 0.4, "color_property": ["modulate", "self_modulate"][i]})))
	code("fondu.color_property = \"self_modulate\"")
	legende("L'étoile jaune est un enfant du sprite")
	await jouer_ensemble(joueurs, 3, 2.0, 1.0)


func _fade_duration() -> void:
	sujet("duration : durée du fondu (s)")
	var joueurs := _fade_colonnes("duration", [0.1, 0.5, 1.5], ["0,1 s", "0,5 s (défaut)", "1,5 s"], {"color": Color.RED})
	code("fondu.duration = 0.5")
	await jouer_ensemble(joueurs, 2, 3.6, 1.8)


func _fade_mode() -> void:
	sujet("mode = FADE_TEXTURE : texture, texture_property")
	var fermee := Formes.dessin(Vector2i(160, 220), Color("8a5a3c"), [[Rect2i(10, 10, 140, 200), Color("a0714f")], [Rect2i(120, 110, 14, 14), Color("ffd166")]])
	var ouverte := Formes.dessin(Vector2i(160, 220), Color("8a5a3c"), [[Rect2i(10, 10, 140, 200), Color("1b1f2a")], [Rect2i(10, 10, 30, 200), Color("a0714f")]])
	var porte := Sprite2D.new()
	porte.texture = fermee
	porte.position = Vector2(576, 290)
	scene.add_child(porte)
	var joueur := joueur_fx(porte, effet("FXFade", {"mode": FXFade.FADE_TEXTURE, "texture": ouverte, "duration": 1.0}))
	code("fondu.mode = FXFade.FADE_TEXTURE\nfondu.texture = porte_ouverte\nfondu.texture_property = \"texture\"  # défaut")
	legende("Les deux images sont mélangées pendant la seconde du fondu")
	await jouer_ensemble([joueur], 3, 2.6, 1.4)


func _fade_revert() -> void:
	sujet("revert_on_stop : fondu inverse à l'arrêt ?")
	var joueurs := _fade_colonnes("revert_on_stop", [true, false], ["true (défaut)", "false"], {"color": Color("5ccf7a"), "duration": 0.4})
	code("fondu.revert_on_stop = false")
	for i in 2:
		for joueur in joueurs:
			joueur.get_parent().modulate = Color.WHITE
		legende("Jeu : les deux passent au vert")
		await jouer_ensemble(joueurs, 1, 1.2)
		legende("Arrêt : seul revert_on_stop = true revient au blanc")
		for joueur in joueurs:
			joueur.stop()
		await secondes(1.4)


func _fade_transition() -> void:
	sujet("transition et ease_type : la courbe du fondu")
	var valeurs := [Tween.TRANS_LINEAR, Tween.TRANS_SINE, Tween.TRANS_BOUNCE]
	var joueurs := _fade_colonnes("transition", valeurs, ["TRANS_LINEAR", "TRANS_SINE (défaut)", "TRANS_BOUNCE"], {"color": Color.RED, "duration": 1.0, "ease_type": Tween.EASE_OUT})
	code("fondu.transition = Tween.TRANS_SINE\nfondu.ease_type = Tween.EASE_IN_OUT")
	await jouer_ensemble(joueurs, 2, 3.0, 1.5)


# 💻 API : ScreenFader

func _classe_fader() -> void:
	sujet("ScreenFader : couvrir et découvrir l'écran")
	var vue := vue_2d(Rect2(156, 120, 840, 400))
	_scene_jeu(vue, "Mon jeu", VARIANTES[0])
	var fader := _fader(vue, {"duration": 0.8})
	code("$ScreenFader.fade_out()\n$ScreenFader.fade_in()")
	for masque in ["", "iris", "volet"]:
		fader.mask = Formes.masque(masque) if masque else null
		legende("Fondu uni" if masque.is_empty() else "Avec un masque : %s" % masque)
		await _fondu_aller_retour([fader], 1, 0.4)


func _fader_color() -> void:
	sujet("color : couleur du voile")
	var faders := _vues_fader([{"color": Color.BLACK}, {"color": Color.WHITE}, {"color": Color(0.8, 0.1, 0.1, 0.7)}],
		["Color.BLACK (défaut)", "Color.WHITE", "Color(0.8, 0.1, 0.1, 0.7)"])
	code("fondu.color = Color.WHITE")
	await _fondu_aller_retour(faders, 2)


func _fader_texture() -> void:
	sujet("texture : une image à la place de la couleur unie")
	var motif := Formes.dessin(Vector2i(64, 64), Color("2a2f45"), [[Rect2i(0, 0, 32, 32), Color("c678dd")], [Rect2i(32, 32, 32, 32), Color("c678dd")]])
	var faders := _vues_fader([{"texture": motif, "color": Color.WHITE}, {"texture": motif, "color": Color("4ea1ff")}],
		["texture = motif, color = blanc", "même texture, color = bleu (teinte)"])
	code("fondu.texture = preload(\"res://motif.png\")\nfondu.color = Color.WHITE  # teinte")
	await _fondu_aller_retour(faders, 2)


func _fader_mask() -> void:
	sujet("mask : la forme de la transition")
	var sortes := ["", "volet", "iris", "losanges", "dissolution"]
	var reglages := []
	for sorte in sortes:
		reglages.append({"mask": Formes.masque(sorte) if sorte else null, "duration": 1.0})
	var faders := _vues_fader(reglages, ["aucun", "volet", "iris", "losanges", "dissolution"])
	code("fondu.mask = preload(\"res://masques/iris.png\")")
	legende("Les pixels sombres du masque sont couverts en premier, les clairs en dernier")
	await _fondu_aller_retour(faders, 2, 0.4)


func _fader_smoothness() -> void:
	sujet("mask_smoothness : douceur du bord")
	var faders := _vues_fader([{"mask": Formes.masque("volet"), "mask_smoothness": 0.02, "duration": 1.2},
		{"mask": Formes.masque("volet"), "mask_smoothness": 0.4, "duration": 1.2}], ["0,02 : bord net", "0,4 : bord flou"])
	code("fondu.mask_smoothness = 0.1  # défaut")
	await _fondu_aller_retour(faders, 2, 0.3)


func _fader_progress() -> void:
	sujet("progress : le recouvrement, modifiable directement")
	var faders := _vues_fader([{}, {"mask": Formes.masque("iris")}], ["sans masque", "avec l'iris"])
	var valeur := etiquette(Vector2(576, 540), "", 22, ACCENT)
	code("$ScreenFader.progress = valeur   # 0 → 1")
	for i in 2:
		for etape in 61:
			var p := float(etape) / 60.0 if i == 0 else 1.0 - float(etape) / 60.0
			for fader in faders:
				fader.progress = p
			valeur.text = "progress = %.2f" % p
			await images()
		await secondes(0.5)


func _fader_always_process() -> void:
	sujet("always_process : fondu pendant la pause et les ralentis")
	var faders := _vues_fader([{"always_process": true, "duration": 0.8}, {"always_process": false, "duration": 0.8}],
		["always_process = true (défaut)", "always_process = false"])
	var etat := etiquette(Vector2(576, 100), "", 20, MAUVAIS)
	code("get_tree().paused = true\nfondu.fade_out()")
	for i in 2:
		get_tree().paused = true
		etat.text = "Jeu en pause"
		legende("fade_out() en pause : seul le fondu always_process avance")
		for fader in faders:
			fader.fade_out()
		await secondes(1.6)
		get_tree().paused = false
		etat.text = ""
		legende("Fin de la pause : l'autre fondu reprend")
		await secondes(1.2)
		for fader in faders:
			fader.fade_in(0.3)
		await secondes(0.8)


func _fader_block_input() -> void:
	sujet("block_input : le voile intercepte les clics")
	var cibles: Array[Button] = []
	var faders: Array[ScreenFader] = []
	var compteurs: Array[Label] = []
	for i in 2:
		var cadre := Rect2(90 + i * 520, 140, 450, 300)
		var vue := vue_2d(cadre)
		var b := Button.new()
		b.text = "Acheter"
		b.add_theme_font_size_override("font_size", 24)
		b.size = Vector2(180, 60)
		b.position = Vector2(-90, -30)
		vue.add_child(b)
		var compteur := etiquette(Vector2(cadre.get_center().x, cadre.end.y + 52), "clics reçus : 0", 18, ACCENT)
		b.pressed.connect(func(): compteur.set_meta("n", compteur.get_meta("n", 0) + 1); compteur.text = "clics reçus : %d" % compteur.get_meta("n"))
		var fader := _fader(vue, {"block_input": i == 0, "duration": 0.3})
		fader.fade_to(0.6, 0.0)
		etiquette(Vector2(cadre.get_center().x, cadre.end.y + 22), ["block_input = true (défaut)", "block_input = false"][i], 17, VARIANTES[i])
		cibles.append(b)
		faders.append(fader)
		compteurs.append(compteur)
	code("fondu.block_input = false  # l'interface reste cliquable")
	legende("Voile à 60 % : on clique sur les deux boutons")
	_fleche = pointeur()
	_fleche.position = Vector2(576, 600)
	for clic in 3:
		for i in 2:
			await secondes(0.5)
			await _cliquer(cibles[i])
	await secondes(1.0)


## Clic simulé au centre d'un Control (même dans une vue), avec un pointeur visible.
func _cliquer(cible: Control) -> void:
	var position := centre_controle(cible)
	await deplacer(_fleche, position, 0.35)
	await souris_clic(position, 0.12)


func _fader_fade_to() -> void:
	sujet("fade_to(progress, duration) : voile partiel")
	var faders := _vues_fader([{"duration": 0.4}], ["fade_to(0.6) puis fade_in()"])
	var valeur := etiquette(Vector2(576, 100), "", 20, ACCENT)
	get_tree().process_frame.connect(func(): valeur.text = "progress = %.2f" % faders[0].progress)
	code("Fondu.fade_to(0.6, 0.2)\nFondu.fade_in(0.2)")
	for i in 3:
		await secondes(0.6)
		faders[0].fade_to(0.6, 0.2)
		await secondes(1.2)
		faders[0].fade_in(0.2)
		await secondes(0.6)


func _fader_signaux() -> void:
	sujet("Signaux : fade_finished, faded_out, faded_in")
	var vue := vue_2d(Rect2(70, 130, 560, 360))
	_scene_jeu(vue, "", VARIANTES[2])
	var fader := _fader(vue, {"duration": 0.6})
	var lignes: Array[Label] = []
	var journal := func(texte: String) -> void:
		if lignes.size() >= 9:
			lignes.pop_front().queue_free()
			for ligne in lignes:
				ligne.position.y -= 32
		for ligne in lignes:
			ligne.add_theme_color_override("font_color", TEXTE_DOUX)
		lignes.append(etiquette(Vector2(880, 160 + lignes.size() * 32), texte, 18, ACCENT))
	fader.fade_finished.connect(func(p: float): journal.call("fade_finished(%.1f)" % p))
	fader.faded_out.connect(func(): journal.call("faded_out"))
	fader.faded_in.connect(func(): journal.call("faded_in"))
	code("Fondu.faded_out.connect(...)")
	for appel in ["fade_out()", "fade_in()", "fade_to(0.5)", "fade_in()"]:
		journal.call(appel)
		match appel:
			"fade_out()": fader.fade_out()
			"fade_in()": fader.fade_in()
			"fade_to(0.5)": fader.fade_to(0.5)
		await secondes(1.3)


# 📌 Exemples

func _ex_disparition() -> void:
	sujet("Exemple : un ennemi vaincu s'efface, puis est supprimé")
	code("effacement.color = Color(1, 1, 1, 0)\neffacement.duration = 0.6\neffacement.revert_on_stop = false\n$PlayerFX.play()\nawait get_tree().create_timer(0.6).timeout\nqueue_free()")
	for passage in 2:
		var ennemis: Array[Sprite2D] = []
		for i in 3:
			var ennemi := _sprite(Vector2(330 + i * 250, 320), Vector2i(70, 90))
			ennemi.modulate = MAUVAIS.lightened(0.3)
			ennemis.append(ennemi)
		for ennemi in ennemis:
			await secondes(0.6)
			legende("mourir() : effacement en 0,6 s, puis queue_free()")
			var joueur := joueur_fx(ennemi, effet("FXFade", {"color": Color(1, 1, 1, 0), "duration": 0.6, "revert_on_stop": false}))
			joueur.play()
			get_tree().create_timer(0.6).timeout.connect(ennemi.queue_free)
		await secondes(1.0)


func _ex_texture() -> void:
	sujet("Exemple : changer d'apparence en douceur (fondu enchaîné)")
	var fermee := Formes.dessin(Vector2i(160, 220), Color("8a5a3c"), [[Rect2i(10, 10, 140, 200), Color("a0714f")], [Rect2i(120, 110, 14, 14), Color("ffd166")]])
	var ouverte := Formes.dessin(Vector2i(160, 220), Color("8a5a3c"), [[Rect2i(10, 10, 140, 200), Color("1b1f2a")], [Rect2i(10, 10, 30, 200), Color("a0714f")]])
	var porte := Sprite2D.new()
	porte.position = Vector2(576, 290)
	scene.add_child(porte)
	code("fondu.mode = FXFade.FADE_TEXTURE\nfondu.texture = porte_ouverte\nfondu.duration = 0.4\nfondu.revert_on_stop = false")
	for i in 3:
		porte.texture = fermee
		await secondes(1.0)
		legende("ouvrir() : la porte s'ouvre en 0,4 s et le reste")
		var joueur := joueur_fx(porte, effet("FXFade", {"mode": FXFade.FADE_TEXTURE, "texture": ouverte, "duration": 0.4, "revert_on_stop": false}))
		joueur.play()
		await secondes(1.2)
		joueur.queue_free()


func _ex_lumiere() -> void:
	sujet("Exemple : la couleur d'une lumière 3D")
	var vue := vue_3d(Rect2(76, 110, 1000, 420))
	cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(14, 0.2, 10))
	for x in [-2.5, 0.0, 2.5]:
		cube(vue, Vector3(x, 0.5, -1.0), Color("cfd6e6"), Vector3(1, 1, 1))
	for enfant in vue.get_children():
		if enfant is DirectionalLight3D:
			enfant.light_energy = 0.15
	var lampe := OmniLight3D.new()
	lampe.position = Vector3(0, 2.2, 0.5)
	lampe.omni_range = 8.0
	lampe.light_energy = 3.0
	vue.add_child(lampe)
	Formes.sphere(vue, lampe.position, Color(1, 1, 0.8), 0.15)
	var joueur := joueur_fx(lampe, effet("FXFade", {"color_property": "light_color", "color": Color.RED, "duration": 1.0}))
	code("alarme.color_property = \"light_color\"\nalarme.color = Color.RED\nalarme.duration = 1.0")
	for i in 2:
		await secondes(0.8)
		legende("set_alarme(true) : la lampe passe au rouge")
		joueur.play()
		await secondes(2.0)
		legende("set_alarme(false) : retour à sa couleur d'origine")
		joueur.stop()
		await secondes(1.6)


func _ex_enchainement() -> void:
	sujet("Exemple : un flash combiné à une secousse")
	var heros := _sprite(Vector2(576, 290), Vector2i(110, 140))
	var flash := effet("FXFade", {"color": Color(2, 2, 2), "duration": 0.05})
	flash.effects_on_play_fx = [effet("FXShake", {"duration": 0.3, "max_amplitude": 10.0})]
	var joueur := joueur_fx(heros, flash)
	code("flash.color = Color(2, 2, 2)\nflash.effects_on_play_fx = [FXShake.new()]\n$PlayerFX.play()")
	for i in 4:
		await secondes(0.6)
		legende("Flash lumineux et secousse, d'un seul play()")
		joueur.play()
		await secondes(0.35)
		joueur.stop()
		await secondes(0.4)


func _ex_iris() -> void:
	sujet("Exemple : une transition en iris")
	var vue := vue_2d(Rect2(156, 120, 840, 400))
	var scenes := [["Monde 1", VARIANTES[3]], ["Monde 2", VARIANTES[1]], ["Monde 3", VARIANTES[0]]]
	var courante := _scene_jeu(vue, scenes[0][0], scenes[0][1])
	var fader := _fader(vue, {"mask": Formes.masque("iris"), "mask_smoothness": 0.05, "duration": 0.9})
	code("iris.fill = GradientTexture2D.FILL_RADIAL\nFondu.mask = iris\nFondu.mask_smoothness = 0.05")
	for i in [1, 2]:
		await secondes(0.8)
		legende("L'iris se ferme : les bords d'abord, le centre en dernier")
		fader.fade_out()
		await fader.faded_out
		courante.queue_free()
		courante = _scene_jeu(vue, scenes[i][0], scenes[i][1])
		await secondes(0.3)
		legende("Puis s'ouvre sur le monde suivant")
		fader.fade_in()
		await fader.faded_in
	await secondes(0.6)


func _ex_pause() -> void:
	sujet("Exemple : assombrir l'écran derrière un menu de pause")
	var vue := vue_2d(Rect2(156, 120, 840, 400))
	_scene_jeu(vue, "", VARIANTES[0])
	var fader := _fader(vue, {"layer": 1})
	var menu_couche := CanvasLayer.new()
	menu_couche.layer = 2
	vue.add_child(menu_couche)
	var menu := VBoxContainer.new()
	menu.position = Vector2(330, 120)
	menu_couche.add_child(menu)
	for texte in ["Reprendre", "Options", "Quitter"]:
		var b := Button.new()
		b.text = texte
		b.custom_minimum_size = Vector2(180, 44)
		menu.add_child(b)
	menu.hide()
	code("get_tree().paused = true\nFondu.block_input = false\nFondu.fade_to(0.6, 0.2)\n$MenuPause.show()")
	for i in 2:
		await secondes(1.0)
		legende("ouvrir_pause() : voile à 60 %, menu par-dessus et cliquable")
		fader.block_input = false
		fader.fade_to(0.6, 0.2)
		menu.show()
		await secondes(1.8)
		legende("fermer_pause() : fade_in(0.2), puis le jeu reprend")
		menu.hide()
		fader.fade_in(0.2)
		await fader.faded_in
		fader.block_input = true


func _ex_await() -> void:
	sujet("Exemple : attendre un fondu sans risquer de bloquer")
	var vue := vue_2d(Rect2(60, 140, 520, 330))
	_scene_jeu(vue, "", VARIANTES[2])
	var fader := _fader(vue, {})
	var etat := etiquette(Vector2(860, 200), "", 20, ACCENT)
	code("func couvrir(duree := -1.0) -> void:\n\tFondu.fade_out(duree)\n\tif not Fondu.is_covered():\n\t\tawait Fondu.faded_out")
	for duree in [0.6, 0.0, 0.6]:
		fader.fade_in(0.0)
		await secondes(0.8)
		etat.text = "couvrir(%s)" % duree
		legende("Durée nulle : faded_out est émis tout de suite, is_covered() est déjà vrai" if duree == 0.0 else "Fondu de 0,6 s : on attend faded_out")
		fader.fade_out(duree)
		if not fader.is_covered():
			etat.text += "\nawait faded_out..."
			await fader.faded_out
		etat.text += "\n✓ écran couvert, la suite s'exécute"
		await secondes(1.4)
