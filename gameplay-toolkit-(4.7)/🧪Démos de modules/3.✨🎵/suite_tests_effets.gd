extends "../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques du module d'effets spéciaux (bibliothèque fx) : Effect et PlayerFX, chaque
## effet converti, TimeManipulatorFX, et les modules infobulle, curseur, fondu, dock et boucle musicale.
##
## Sans souris réelle (headless), le survol est simulé en émettant mouse_entered / mouse_exited, et le dock
## est piloté par le focus clavier.
## Avertissements attendus dans la console : propriété couleur absente (FXFade), boucle invalide (MusicLooper).


func _executer() -> void:
	await _tester_effet_de_base()
	await _tester_effet_script()
	await _tester_player_fx()
	await _tester_change_size()
	await _tester_shake()
	await _tester_stretch_and_squash()
	await _tester_targeter()
	await _tester_follower()
	await _tester_audio()
	await _tester_time_manipulator()
	await _tester_fondu_couleur()
	await _tester_fondu_texture()
	await _tester_screen_fader()
	await _tester_infobulle()
	await _tester_curseur()
	await _tester_dock()
	await _tester_music_looper()


# ✨ Effect et PlayerFX

func _tester_effet_de_base() -> void:
	var effet := Effect.new()
	var enchaine := Effect.new()
	var a_l_arret := Effect.new()
	effet.effects_on_play_fx = [enchaine]
	effet.effects_on_stop_fx = [a_l_arret]
	var joue := _espionner(effet.fx_played)
	var arrete := _espionner(effet.fx_stopped)

	effet.stop_fx()
	_verifier("Effect : stop_fx sans play_fx ne fait rien", arrete.n == 0 and not a_l_arret.is_activated())

	effet.play_fx()
	_verifier("Effect : play_fx active l'effet et émet fx_played", effet.is_activated() and joue.n == 1)
	_verifier("Effect : les effets enchaînés au jeu sont joués", enchaine.is_activated())
	effet.stop_fx()
	_verifier("Effect : stop_fx désactive l'effet et émet fx_stopped", not effet.is_activated() and arrete.n == 1)
	_verifier("Effect : les effets enchaînés au jeu sont arrêtés", not enchaine.is_activated())
	_verifier("Effect : les effets d'arrêt sont joués", a_l_arret.is_activated())

	var boucle := Effect.new()
	boucle.effects_on_play_fx = [boucle]
	boucle.play_fx()
	boucle.process_fx(0.016)
	boucle.stop_fx()
	_verifier("Effect : un effet qui s'enchaîne lui-même ne boucle pas à l'infini", not boucle.is_activated())
	boucle.effects_on_play_fx = [] # cycle de références : sinon jamais libéré
	_verifier("Effect : local à la scène par défaut", Effect.new().resource_local_to_scene)
	await _frames()


func _tester_effet_script() -> void:
	var script := GDScript.new()
	script.source_code = "extends Effect\nvar jeux := 0\nvar mises_a_jour := 0\nvar arrets := 0\nfunc _play_fx(_cible: Node) -> void:\n\tjeux += 1\nfunc _process_fx(_delta: float, _cible: Node) -> void:\n\tmises_a_jour += 1\nfunc _stop_fx(_cible: Node) -> void:\n\tarrets += 1\n"
	script.reload()
	var effet: Effect = Effect.new()
	effet.set_script(script)
	effet.play_fx()
	effet.process_fx(0.016)
	effet.stop_fx()
	_verifier("Effect : un script dérivé reçoit _play_fx / _process_fx / _stop_fx",
		effet.jeux == 1 and effet.mises_a_jour == 1 and effet.arrets == 1,
		"appels : %d / %d / %d" % [effet.jeux, effet.mises_a_jour, effet.arrets])
	await _frames()


func _tester_player_fx() -> void:
	var parent: Node2D = _ajouter(Node2D.new())
	var joueur := PlayerFX.new()
	var effet := Effect.new()
	var enchaine := Effect.new()
	effet.effects_on_play_fx = [enchaine]
	joueur.special_effects = [effet]
	parent.add_child(joueur)

	_verifier("PlayerFX : cible par défaut = le parent", joueur.get_default_target() == parent)
	_verifier("PlayerFX : les effets (et leurs enchaînés) connaissent le PlayerFX",
		effet.get_player_fx() == joueur and enchaine.get_player_fx() == joueur)

	var autre: Node2D = _ajouter(Node2D.new())
	joueur.node_to_play_effects = autre
	_verifier("PlayerFX : node_to_play_effects remplace le parent", joueur.get_default_target() == autre)

	joueur.play()
	_verifier("PlayerFX : play() active les effets", effet.is_activated())
	joueur.stop()
	_verifier("PlayerFX : stop() arrête les effets", not effet.is_activated())
	parent.free()
	autre.free()
	await _frames()


# 🦣🫘 Agrandissement

func _tester_change_size() -> void:
	for classe in ["Node2D", "Control", "Node3D"]:
		var noeud: Node = _ajouter(ClassDB.instantiate(classe))
		var effet := FXChangeSize.new()
		effet.final_scale = 2.0
		effet.duration = 0.1
		effet.play_fx(noeud)
		await _secondes(0.2)
		var attendu = Vector3(2, 2, 2) if classe == "Node3D" else Vector2(2, 2)
		_verifier("FXChangeSize : échelle d'un %s portée à final_scale" % classe, noeud.scale.is_equal_approx(attendu), str(noeud.scale))
		effet.stop_fx(noeud)
		await _secondes(0.2)
		var origine = Vector3.ONE if classe == "Node3D" else Vector2.ONE
		_verifier("FXChangeSize : échelle d'un %s rendue à l'arrêt" % classe, noeud.scale.is_equal_approx(origine), str(noeud.scale))
		noeud.free()


# 💥 Secousse

func _tester_shake() -> void:
	var noeud: Node2D = _ajouter(Node2D.new())
	noeud.position = Vector2(50, 50)
	var effet := FXShake.new()
	effet.duration = 0.3
	effet.max_amplitude = 20.0
	var joueur := PlayerFX.new()
	joueur.special_effects = [effet]
	noeud.add_child(joueur)

	joueur.play()
	await _frames(3)
	joueur.play() # relancée en pleine secousse : l'origine ne doit pas dériver
	var deplace := false
	for i in 10:
		await _frames()
		deplace = deplace or not noeud.position.is_equal_approx(Vector2(50, 50))
	_verifier("FXShake : la cible est secouée", deplace)
	await _secondes(0.4)
	_verifier("FXShake : arrêt automatique après duration, position d'origine rendue",
		not effet.is_activated() and noeud.position.is_equal_approx(Vector2(50, 50)), str(noeud.position))
	noeud.free()


# 🪗 Squash and Stretch

func _tester_stretch_and_squash() -> void:
	var noeud: Node2D = _ajouter(Node2D.new())
	var effet := FXStretchAndSquash.new()
	effet.smooth_speed = 30.0
	effet.play_fx(noeud)
	for i in 10:
		noeud.position.x += 8.0
		effet.process_fx(1.0 / 60.0, noeud)
	_verifier("FXStretchAndSquash : étirée dans le sens du mouvement (x > 1 > y)",
		noeud.scale.x > 1.05 and noeud.scale.y < 0.95, str(noeud.scale))
	effet.play_fx(noeud) # relancée déformée : l'échelle d'origine est conservée
	effet.stop_fx(noeud)
	_verifier("FXStretchAndSquash : échelle d'origine rendue à l'arrêt", noeud.scale.is_equal_approx(Vector2.ONE), str(noeud.scale))
	noeud.free()
	await _frames()


# 🌻 Ciblage

func _tester_targeter() -> void:
	var racine: Node2D = _ajouter(Node2D.new())
	var viseur: Node2D = _ajouter(Node2D.new(), racine)
	var cible: Node2D = _ajouter(Node2D.new(), racine)
	cible.name = "Cible"
	cible.position = Vector2(0, 100)
	var joueur: PlayerFX = _ajouter(PlayerFX.new(), viseur)
	var effet := FXTargeter.new()
	effet.nodes_to_target = [NodePath("../../Cible")]
	joueur.special_effects = [effet]
	joueur.play()
	await _frames(2)
	_verifier("FXTargeter : un Node2D s'oriente vers la cible (+X vers elle)", is_equal_approx(viseur.rotation, PI / 2), str(viseur.rotation))
	racine.free()

	var racine_3d: Node3D = _ajouter(Node3D.new())
	var viseur_3d: Node3D = _ajouter(Node3D.new(), racine_3d)
	var cible_3d: Node3D = _ajouter(Node3D.new(), racine_3d)
	cible_3d.name = "Cible"
	cible_3d.position = Vector3(10, 0, 0)
	var joueur_3d: PlayerFX = _ajouter(PlayerFX.new(), viseur_3d)
	var effet_3d := FXTargeter.new()
	effet_3d.nodes_to_target = [NodePath("../../Cible")]
	joueur_3d.special_effects = [effet_3d]
	joueur_3d.play()
	await _frames(2)
	var avant := -viseur_3d.global_basis.z
	_verifier("FXTargeter : un Node3D regarde la cible (-Z vers elle)", avant.is_equal_approx(Vector3.RIGHT), str(avant))

	cible_3d.position = Vector3.ZERO # confondue avec le viseur : pas d'erreur de look_at
	await _frames(2)
	_verifier("FXTargeter : cible confondue avec le nœud sans erreur", effet_3d.is_activated())
	racine_3d.free()


# 〰️ Follow

func _tester_follower() -> void:
	var racine: Node2D = _ajouter(Node2D.new())
	var suiveur: Node2D = _ajouter(Node2D.new(), racine)
	for i in 2:
		var cible := Node2D.new()
		cible.name = "Cible%d" % i
		cible.position = Vector2(100 * i, 40)
		racine.add_child(cible)
	var joueur: PlayerFX = _ajouter(PlayerFX.new(), suiveur)
	var effet := FXFollower.new()
	effet.use_interpolation = false
	effet.targets = [NodePath("../../Cible0"), NodePath("../../Cible1")]
	joueur.special_effects = [effet]
	joueur.play()
	await _frames(2)
	_verifier("FXFollower : la cible rejoint le centre des nœuds suivis", suiveur.position.is_equal_approx(Vector2(50, 40)), str(suiveur.position))
	racine.free()


# 🔊 Son

func _tester_audio() -> void:
	var son := _son_wav(0.2)
	var noeud: Node2D = _ajouter(Node2D.new())
	var effet := FXAudio.new()
	effet.stream = son
	effet.play_fx(noeud)
	var lecteur: Node = effet.get_audio_player()
	_verifier("FXAudio : lecteur créé sous la cible avec le son",
		lecteur is AudioStreamPlayer and lecteur.get_parent() == noeud and lecteur.stream == son)
	_verifier("FXAudio : lecteur interne (absent des enfants visibles)", noeud.get_child_count() == 0)
	effet.stop_fx(noeud)
	effet.play_fx(noeud)
	_verifier("FXAudio : lecteur réutilisé au jeu suivant", effet.get_audio_player() == lecteur)

	var effet_2d := FXAudio.new()
	effet_2d.stream = son
	effet_2d.positional = true
	effet_2d.play_fx(noeud)
	_verifier("FXAudio : positional sur un Node2D -> AudioStreamPlayer2D", effet_2d.get_audio_player() is AudioStreamPlayer2D)
	noeud.free()
	await _frames()


# ⏯️ Temps

func _tester_time_manipulator() -> void:
	var temps: TimeManipulatorFX = _ajouter(TimeManipulatorFX.new())
	var debut := _espionner(temps.hit_stop_started)
	var fin := _espionner(temps.hit_stop_finished)
	temps.trigger_hit_stop(0.1)
	_verifier("TimeManipulatorFX : HitStop -> time_scale à 0", Engine.time_scale == 0.0 and temps.is_hit_stopping() and debut.n == 1)
	await _secondes_reelles(0.25)
	_verifier("TimeManipulatorFX : fin du HitStop malgré time_scale à 0", Engine.time_scale == 1.0 and not temps.is_hit_stopping() and fin.n == 1,
		"time_scale = %s" % Engine.time_scale)

	temps.set_time_scale(0.5)
	await _secondes_reelles(1.0)
	_verifier("TimeManipulatorFX : set_time_scale rejoint la vitesse visée", is_equal_approx(Engine.time_scale, 0.5), "time_scale = %s" % Engine.time_scale)
	temps.free()
	_verifier("TimeManipulatorFX : vitesse normale rendue à sa sortie de l'arbre", Engine.time_scale == 1.0)


# 🌈 Fondu

func _tester_fondu_couleur() -> void:
	var sprite: Sprite2D = _ajouter(Sprite2D.new())
	var effet := FXFade.new()
	effet.color = Color(1, 0, 0, 0.5)
	effet.duration = 0.1
	effet.play_fx(sprite)
	await _frames(2)
	effet.play_fx(sprite) # relancé en plein fondu : la couleur d'origine est conservée
	await _secondes(0.2)
	_verifier("FXFade : modulate porté à la couleur", sprite.modulate.is_equal_approx(Color(1, 0, 0, 0.5)), str(sprite.modulate))
	effet.stop_fx(sprite)
	await _secondes(0.2)
	_verifier("FXFade : couleur d'origine rendue à l'arrêt", sprite.modulate.is_equal_approx(Color.WHITE), str(sprite.modulate))

	effet.color_property = "light_color" # absente d'un Sprite2D : avertissement attendu
	effet.play_fx(sprite)
	_verifier("FXFade : propriété couleur absente -> rien ne change", sprite.modulate.is_equal_approx(Color.WHITE))
	effet.stop_fx(sprite)

	var lumiere: OmniLight3D = _ajouter(OmniLight3D.new())
	var effet_3d := FXFade.new()
	effet_3d.color_property = "light_color"
	effet_3d.color = Color.BLUE
	effet_3d.duration = 0.0
	effet_3d.play_fx(lumiere)
	_verifier("FXFade : autre propriété couleur (light_color), durée nulle -> immédiat", lumiere.light_color == Color.BLUE)
	sprite.free()
	lumiere.free()


func _tester_fondu_texture() -> void:
	var texture_a := _texture(Color.RED)
	var texture_b := _texture(Color.BLUE)
	var sprite: Sprite2D = _ajouter(Sprite2D.new())
	sprite.texture = texture_a
	var effet := FXFade.new()
	effet.mode = FXFade.FADE_TEXTURE
	effet.texture = texture_b
	effet.duration = 0.2
	effet.play_fx(sprite)
	await _frames(2)
	_verifier("FXFade : fondu enchaîné par shader pendant la transition", sprite.material is ShaderMaterial)
	await _secondes(0.3)
	_verifier("FXFade : texture finale posée et matériau d'origine rendu", sprite.texture == texture_b and sprite.material == null)
	effet.stop_fx(sprite)
	await _secondes(0.3)
	_verifier("FXFade : texture d'origine rendue à l'arrêt", sprite.texture == texture_a and sprite.material == null)

	effet.play_fx(sprite)
	await _frames(2)
	effet.stop_fx(sprite) # interrompu en plein fondu
	await _secondes(0.3)
	_verifier("FXFade : fondu interrompu -> retour propre à la texture d'origine", sprite.texture == texture_a and sprite.material == null)
	sprite.free()

	var sprite_3d: Sprite3D = _ajouter(Sprite3D.new())
	sprite_3d.texture = texture_a
	effet.play_fx(sprite_3d)
	await _secondes(0.3)
	_verifier("FXFade : Sprite3D (effacement / réapparition) -> nouvelle texture, opacité rendue",
		sprite_3d.texture == texture_b and is_equal_approx(sprite_3d.modulate.a, 1.0), "a = %s" % sprite_3d.modulate.a)
	sprite_3d.free()


func _tester_screen_fader() -> void:
	var voile: ScreenFader = _ajouter(ScreenFader.new())
	var couvert := _espionner(voile.faded_out)
	var decouvert := _espionner(voile.faded_in)
	_verifier("ScreenFader : écran découvert par défaut", voile.progress == 0.0 and not voile.is_covered())
	voile.fade_out(0.1)
	_verifier("ScreenFader : fondu en cours", voile.is_fading())
	await _secondes(0.25)
	_verifier("ScreenFader : fade_out couvre l'écran et émet faded_out", voile.is_covered() and couvert.n == 1 and not voile.is_fading())
	voile.mask = _texture(Color.GRAY)
	voile.fade_in(0.0)
	_verifier("ScreenFader : fade_in immédiat (durée 0) émet faded_in", voile.progress == 0.0 and decouvert.n == 1)

	get_tree_pause(true)
	voile.fade_out(0.1)
	await _secondes_reelles(0.25)
	_verifier("ScreenFader : always_process -> le fondu avance même en pause", voile.is_covered())
	get_tree_pause(false)
	voile.free()


# 🗨️ Infobulle

func _tester_infobulle() -> void:
	var bouton: Button = _ajouter(Button.new())
	var contenu := TooltipContent.new()
	contenu.title = "Épée"
	contenu.text = "Une [b]épée[/b] courte."
	contenu.icon = _texture(Color.YELLOW, 128)
	contenu.icon_max_size = Vector2(32, 32)
	var infobulle := CustomTooltip.new()
	infobulle.content = contenu
	infobulle.delay = 0.1
	infobulle.fade_duration = 0.0
	bouton.add_child(infobulle)
	var affichee := _espionner(infobulle.tooltip_shown)
	var masquee := _espionner(infobulle.tooltip_hidden)
	await _frames()

	bouton.mouse_entered.emit()
	await _frames(2)
	_verifier("CustomTooltip : pas d'affichage avant delay", not infobulle.is_tooltip_visible())
	await _secondes(0.2)
	_verifier("CustomTooltip : affichée après delay au survol", infobulle.is_tooltip_visible() and affichee.n == 1)

	var panneau: PanelContainer = infobulle.get_panel()
	var titre: Label = panneau.find_child("Title", true, false)
	var texte: RichTextLabel = panneau.find_child("Text", true, false)
	var icone: TextureRect = panneau.find_child("Icon", true, false)
	_verifier("CustomTooltip : titre, texte BBCode et icône affichés",
		titre and titre.text == "Épée" and texte and texte.get_parsed_text() == "Une épée courte." and icone)
	_verifier("CustomTooltip : icône réduite à icon_max_size", icone and icone.custom_minimum_size == Vector2(32, 32), str(icone.custom_minimum_size) if icone else "")
	_verifier("CustomTooltip : l'infobulle ne capte pas la souris", panneau.mouse_filter == Control.MOUSE_FILTER_IGNORE and texte.mouse_filter == Control.MOUSE_FILTER_IGNORE)
	var ecran := panneau.get_viewport().get_visible_rect()
	_verifier("CustomTooltip : placée dans l'écran", ecran.encloses(panneau.get_global_rect()), str(panneau.get_global_rect()))

	contenu.text = "Mot ".repeat(200)
	await _frames(2)
	texte = panneau.find_child("Text", true, false)
	_verifier("CustomTooltip : contenu modifié -> infobulle reconstruite, texte long replié à max_width",
		texte and texte.custom_minimum_size.x == contenu.max_width and panneau.size.x < contenu.max_width + 40, str(panneau.size))

	bouton.mouse_exited.emit()
	await _frames()
	_verifier("CustomTooltip : masquée en quittant la cible", not infobulle.is_tooltip_visible() and not panneau.visible and masquee.n == 1)

	contenu.video = VideoStreamTheora.new() # flux vide : seule la construction est vérifiée
	contenu.custom_scene = _empaqueter(ColorRect.new())
	infobulle.show_tooltip()
	await _frames(3)
	_verifier("CustomTooltip : show_tooltip() sans survol reste affichée", infobulle.is_tooltip_visible())
	_verifier("CustomTooltip : lecteur vidéo et scène personnalisée ajoutés",
		panneau.find_child("Video", true, false) is VideoStreamPlayer and panneau.find_children("*", "ColorRect", true, false).size() == 1)
	infobulle.hide_tooltip()
	bouton.free()
	await _frames()


# 👆 Curseur

func _tester_curseur() -> void:
	var style := CursorStyle.new()
	var images := [_texture(Color.RED), _texture(Color.GREEN), _texture(Color.BLUE)]
	style.frames = images
	style.frames_per_second = 10.0
	_verifier("CursorStyle : image selon le temps (animation en boucle)",
		style.get_frame_at(0.0) == images[0] and style.get_frame_at(0.15) == images[1] and style.get_frame_at(0.35) == images[0])
	_verifier("CursorStyle : sans image -> nulle", CursorStyle.new().get_frame_at(1.0) == null)

	var global: CursorChanger = _ajouter(CursorChanger.new())
	global.style = style
	await _frames()
	_verifier("CursorChanger : mode permanent actif dès l'entrée dans l'arbre", global.is_active() and global.is_current())

	var bouton: Button = _ajouter(Button.new())
	var survol := CursorChanger.new()
	survol.activation_mode = CursorChanger.ACTIVATION_HOVER
	survol.style = style
	bouton.add_child(survol)
	await _frames()
	_verifier("CursorChanger : mode survol inactif sans survol", not survol.is_active() and global.is_current())
	bouton.mouse_entered.emit()
	_verifier("CursorChanger : survol -> prend la main sur le curseur global", survol.is_current() and global.is_active())
	bouton.mouse_exited.emit()
	_verifier("CursorChanger : fin du survol -> le curseur global reprend la main", not survol.is_active() and global.is_current())

	var manuel: CursorChanger = _ajouter(CursorChanger.new())
	manuel.activation_mode = CursorChanger.ACTIVATION_MANUAL
	await _frames()
	_verifier("CursorChanger : mode manuel inactif par défaut", not manuel.is_active())
	manuel.activate()
	_verifier("CursorChanger : activate() le rend courant", manuel.is_current())
	manuel.free()
	_verifier("CursorChanger : libéré -> le précédent reprend la main", global.is_current())

	bouton.mouse_entered.emit()
	bouton.free()
	_verifier("CursorChanger : cible libérée pendant le survol -> désactivé", global.is_current())
	global.free()
	await _frames()


# 🛟 Dock

func _tester_dock() -> void:
	# Loin de l'origine : la souris (immobile en headless) ne survole pas le dock
	var dock := FloatingDock.new()
	dock.position = Vector2(300, 400)
	dock.smoothing = 0.0
	var boutons: Array[Button] = []
	for i in 5:
		var bouton := Button.new()
		bouton.focus_mode = Control.FOCUS_ALL
		dock.add_child(bouton)
		boutons.append(bouton)
	_ajouter(dock)
	dock.size = dock.get_combined_minimum_size()
	await _frames(2)

	_verifier("FloatingDock : taille minimale = éléments + séparations", dock.get_combined_minimum_size() == Vector2(5 * 64 + 4 * 8, 64), str(dock.get_combined_minimum_size()))
	_verifier("FloatingDock : éléments au repos à item_size, alignés",
		boutons.all(func(b): return b.size == Vector2(64, 64)) and boutons[1].position.x == 72.0, str(boutons[1].position))

	var change := _espionner(dock.hovered_item_changed)
	boutons[2].grab_focus()
	await _frames(2)
	_verifier("FloatingDock : élément qui a le focus grossi à magnification", boutons[2].size.is_equal_approx(Vector2(64, 64) * dock.magnification), str(boutons[2].size))
	_verifier("FloatingDock : grossissement décroissant avec la distance",
		dock.get_item_influence(2) > dock.get_item_influence(1) and dock.get_item_influence(1) > dock.get_item_influence(0) and is_equal_approx(dock.get_item_influence(1), dock.get_item_influence(3)))
	var sans_chevauchement := true
	for i in 4:
		sans_chevauchement = sans_chevauchement and boutons[i].get_rect().end.x <= boutons[i + 1].position.x + 0.01
	_verifier("FloatingDock : éléments écartés sans chevauchement", sans_chevauchement)
	_verifier("FloatingDock : espace supplémentaire autour de l'élément survolé",
		boutons[3].position.x - boutons[2].get_rect().end.x > dock.separation + dock.hover_spacing)
	_verifier("FloatingDock : collés au bord bas, ils grossissent vers le haut", is_equal_approx(boutons[2].get_rect().end.y, dock.size.y))
	_verifier("FloatingDock : hovered_item_changed émis", change.n == 1 and dock.get_hovered_item() == boutons[2])

	dock.resize_mode = FloatingDock.RESIZE_SCALE
	await _frames(2)
	_verifier("FloatingDock : mode échelle -> taille normale, échelle agrandie", boutons[2].size.is_equal_approx(Vector2(64, 64)) and is_equal_approx(boutons[2].scale.x, dock.magnification), "%s %s" % [boutons[2].size, boutons[2].scale])

	boutons[2].release_focus()
	await _frames(2)
	_verifier("FloatingDock : sans pointeur, retour au repos", is_equal_approx(dock.get_item_influence(2), 0.0) and dock.get_hovered_item() == null and boutons[2].scale == Vector2.ONE)
	dock.free()
	await _frames()


# 🔁 Boucle musicale

func _tester_music_looper() -> void:
	var son := _son_wav(1.0)
	var lecteur: MusicLooper = _ajouter(MusicLooper.new())
	lecteur.stream = son
	lecteur.loop_start = 0.1
	lecteur.loop_end = 0.3
	_verifier("MusicLooper : fin de boucle effective", is_equal_approx(lecteur.get_effective_loop_end(), 0.3))
	lecteur.loop_end = 0.0
	_verifier("MusicLooper : loop_end à 0 -> fin du flux", is_equal_approx(lecteur.get_effective_loop_end(), 1.0))
	lecteur.loop_end = 0.3

	var boucles := _espionner(lecteur.looped)
	lecteur.play()
	_verifier("MusicLooper : WAV -> boucle native (exacte à l'échantillon)", lecteur.is_playing() and lecteur.is_loop_sample_accurate())
	_verifier("MusicLooper : le flux d'origine n'est pas modifié", son.loop_mode == AudioStreamWAV.LOOP_DISABLED)
	await _secondes_reelles(0.8)
	_avertir_si("MusicLooper : la boucle se répète (lecture réelle)", boucles.n >= 1 and lecteur.is_playing(),
		"aucune boucle détectée : le pilote audio headless ne fait peut-être pas avancer la lecture")
	lecteur.stop(0.1)
	await _secondes_reelles(0.25)
	_verifier("MusicLooper : stop avec fondu -> arrêté", not lecteur.is_playing())

	lecteur.loop_start = 0.5 # après loop_end : avertissement attendu
	lecteur.play()
	_verifier("MusicLooper : boucle invalide -> lecture sans boucle", lecteur.is_playing() and not lecteur.is_loop_sample_accurate())
	lecteur.stop()
	lecteur.free()
	# Le serveur audio libère les lectures arrêtées au mixage suivant
	await _secondes_reelles(0.1)


# 🛠️ Utilitaires

func _texture(couleur: Color, taille: int = 8) -> ImageTexture:
	var image := Image.create(taille, taille, false, Image.FORMAT_RGBA8)
	image.fill(couleur)
	return ImageTexture.create_from_image(image)


## Son WAV muet de p_duree secondes (8 bits, mono, 22 050 Hz).
func _son_wav(duree: float) -> AudioStreamWAV:
	var son := AudioStreamWAV.new()
	son.format = AudioStreamWAV.FORMAT_8_BITS
	son.mix_rate = 22050
	son.stereo = false
	var donnees := PackedByteArray()
	donnees.resize(int(duree * 22050))
	son.data = donnees
	return son


## Attente en temps réel (insensible à Engine.time_scale et à la pause).
func _secondes_reelles(duree: float) -> void:
	await _bac_a_sable.get_tree().create_timer(duree, true, false, true).timeout


func get_tree_pause(pause: bool) -> void:
	_bac_a_sable.get_tree().paused = pause
