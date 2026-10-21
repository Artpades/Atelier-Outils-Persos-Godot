extends "res://🎥Démos de documentation/socle/apercu.gd"

## 🔁 Aperçu interactif de MusicLooper : un morceau synthétisé (intro 0-2 s, refrain 2-6 s, fin 6-8 s),
## réglage de la boucle, du nombre de boucles, pause et fondus.

const Sons := preload("res://🎥Démos de documentation/socle/sons.gd")

var _musique := MusicLooper.new()
var _fondu := 1.0


func titre() -> String:
	return "Music Looper"


func aide() -> String:
	return "Jouer, puis déplacer loop_start / loop_end : la frise montre la section bouclée et la tête de lecture."


func construire() -> void:
	var notes := PackedFloat32Array([262, 330, 392, 523, 392, 330, 392, 523, 659, 587, 523, 587, 659, 659, 659, 587, 587, 587, 659, 784, 784, 659, 587, 523, 523, 392, 330, 262, 196, 196, 131, 131])
	_musique.stream = Sons.melodie(notes, 0.25)
	_musique.loop_start = 2.0
	_musique.loop_end = 6.0
	add_child(_musique)
	var frise := preload("res://🎥Démos de documentation/3.✨🎵/🔁Music Looper/frise_musique.gd").new()
	frise.musique = _musique
	frise.largeur = 600.0
	frise.position = Vector2(56, 300)
	add_child(frise)
	var vumetre := Sons.Vumetre.new()
	vumetre.position = Vector2(CENTRE.x - 120, 480)
	add_child(vumetre)
	_musique.looped.connect(func(n: int): journaliser("looped(%d)" % n))
	_musique.finished.connect(func(): journaliser("finished"))

	panneau.ajouter_curseur("loop_start (s)", 0.0, 7.5, 0.25, 2.0, func(v: float): _musique.loop_start = v; _maj_code())
	panneau.ajouter_curseur("loop_end (s, 0 = fin)", 0.0, 8.0, 0.25, 6.0, func(v: float): _musique.loop_end = v; _maj_code())
	panneau.ajouter_curseur("loop_count (-1 = sans fin)", -1.0, 5.0, 1.0, -1.0, func(v: float): _musique.loop_count = int(v); _maj_code())
	panneau.ajouter_case("loop_enabled", true, func(v: bool): _musique.loop_enabled = v; _maj_code())
	panneau.ajouter_curseur("volume_db", -30.0, 6.0, 1.0, 0.0, func(v: float): _musique.volume_db = v; _maj_code())
	panneau.ajouter_curseur("Durée des fondus (s)", 0.0, 3.0, 0.1, _fondu, func(v: float): _fondu = v; _maj_code())
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("play()", _jouer, rangee)
	panneau.ajouter_bouton("stop()", func(): _musique.stop(_fondu), rangee)
	panneau.ajouter_case("set_paused", false, func(v: bool): _musique.set_paused(v))
	_maj_code()


func _jouer() -> void:
	_musique.play(0.0, _fondu)


func _maj_code() -> void:
	afficher_code("$MusicLooper.loop_start = %s\n$MusicLooper.loop_end = %s\n$MusicLooper.loop_count = %d · loop_enabled = %s\n$MusicLooper.play(0.0, %s)   # fondu d'entrée" % [
		_musique.loop_start, _musique.loop_end, _musique.loop_count, _musique.loop_enabled, _fondu])
