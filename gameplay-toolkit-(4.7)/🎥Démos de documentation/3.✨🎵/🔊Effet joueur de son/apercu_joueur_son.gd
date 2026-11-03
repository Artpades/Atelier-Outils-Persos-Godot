extends "res://🎥Démos de documentation/socle/apercu.gd"

## 🔊 Aperçu interactif de FXAudio : choisir un son synthétisé, régler hauteur, volume, hauteur aléatoire,
## spatialisation (déplacer l'objet) et coupure à l'arrêt.

const Sons := preload("res://🎥Démos de documentation/socle/sons.gd")
const NOMS_SONS := ["Impact", "Bip", "Pas (3 variantes)", "Mélodie longue", "Alarme en boucle"]

var _son := FXAudio.new()
var _joueur := PlayerFX.new()
var _objet: Node2D
var _sons: Array[AudioStream] = []
var _choix := 0
var _polyphonie := 1


func titre() -> String:
	return "Effet joueur de son (FXAudio)"


func aide() -> String:
	return "Cliquer sur Jouer pour entendre le son. Avec positional, déplacer l'objet change le côté d'où il vient."


func construire() -> void:
	var variantes := AudioStreamRandomizer.new()
	for i in 3:
		variantes.add_stream(i, Sons.pas(i))
	_sons = [Sons.impact(), Sons.bip(880.0, 0.25), variantes, Sons.melodie(PackedFloat32Array([392.0, 494.0, 587.0, 784.0, 587.0, 494.0, 392.0]), 0.25), Sons.alarme()]
	_objet = carre(Vector2(CENTRE.x, 300), Formes.VARIANTES[0], 90)
	_objet.add_child(_joueur)
	_son.stream = _sons[0]
	_joueur.special_effects = [_son]
	var vumetre := Sons.Vumetre.new()
	vumetre.position = Vector2(CENTRE.x - 150, 470)
	vumetre.largeur = 300.0
	add_child(vumetre)

	panneau.ajouter_options("stream", PackedStringArray(NOMS_SONS), 0, func(i: int): _choix = i; _joueur.stop(); _son.stream = _sons[i]; _maj_code())
	panneau.ajouter_curseur("volume_db", -40.0, 6.0, 1.0, 0.0, func(v: float): _son.volume_db = v; _maj_code())
	panneau.ajouter_curseur("pitch_scale", 0.25, 3.0, 0.05, 1.0, func(v: float): _son.pitch_scale = v; _maj_code())
	panneau.ajouter_case("randomize_pitch_scale", false, func(v: bool): _son.randomize_pitch_scale = v; _maj_code())
	panneau.ajouter_curseur("min_pitch", 0.5, 1.0, 0.01, _son.min_pitch, func(v: float): _son.min_pitch = v; _maj_code())
	panneau.ajouter_curseur("max_pitch", 1.0, 2.0, 0.01, _son.max_pitch, func(v: float): _son.max_pitch = v; _maj_code())
	panneau.ajouter_case("positional (lecteur 2D)", false, _positionnel)
	panneau.ajouter_curseur("Position x de l'objet", 40.0, 670.0, 5.0, _objet.position.x, func(v: float): _objet.position.x = v)
	panneau.ajouter_case("stop_sound_on_stop", false, func(v: bool): _son.stop_sound_on_stop = v; _maj_code())
	panneau.ajouter_curseur("max_polyphony du lecteur", 1.0, 8.0, 1.0, 1.0, func(v: float): _polyphonie = int(v); _maj_code())
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Jouer  play()", _jouer, rangee)
	panneau.ajouter_bouton("Arrêter  stop()", func(): _joueur.stop(), rangee)
	_maj_code()


func _positionnel(actif: bool) -> void:
	# Le lecteur est créé au premier jeu : on repart d'un effet neuf pour changer de sorte de lecteur.
	_joueur.stop()
	var ancien := _son
	_son = ancien.duplicate()
	_son.positional = actif
	var lecteur = ancien.get_audio_player()
	if lecteur:
		lecteur.queue_free()
	_joueur.special_effects = [_son]
	_maj_code()


func _jouer() -> void:
	_joueur.play()
	var lecteur = _son.get_audio_player()
	if lecteur:
		lecteur.max_polyphony = _polyphonie
		journaliser("%s : pitch %.2f" % [lecteur.get_class(), lecteur.pitch_scale])


func _maj_code() -> void:
	afficher_code("var son := FXAudio.new()\nson.stream = %s\nson.volume_db = %s\nson.pitch_scale = %s\nson.randomize_pitch_scale = %s  # %s - %s\nson.positional = %s\nson.stop_sound_on_stop = %s\n$Objet/PlayerFX.play()" % [
		["impact", "bip", "variantes_pas", "melodie", "alarme_boucle"][_choix], _son.volume_db, snappedf(_son.pitch_scale, 0.01),
		_son.randomize_pitch_scale, snappedf(_son.min_pitch, 0.01), snappedf(_son.max_pitch, 0.01), _son.positional, _son.stop_sound_on_stop])
