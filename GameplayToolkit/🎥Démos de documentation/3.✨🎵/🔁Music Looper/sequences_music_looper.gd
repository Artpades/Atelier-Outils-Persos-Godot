extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## 🔁 Plans filmés (avec le son) de la page Music Looper (effets/music-looper.html).
## Morceau synthétisé : intro 0-2 s (montée), section bouclée 2-6 s (refrain), fin 6-8 s (descente).

const Sons := preload("res://🎥Démos de documentation/socle/sons.gd")
const Frise := preload("res://🎥Démos de documentation/3.✨🎵/🔁Music Looper/frise_musique.gd")
const INTRO := 2.0
const FIN_BOUCLE := 6.0

var _musique: MusicLooper
var _journal: Label
var _lignes := PackedStringArray()


func titre_module() -> String:
	return "🔁 Music Looper · MusicLooper"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-MusicLooper": _classe,
		"MusicLooper.loop_enabled": _loop_enabled,
		"MusicLooper.set_paused": _set_paused,
		"ex-combat": _ex_combat,
		"ex-fin": _ex_fin,
		"ex-pause": _ex_pause,
		"ex-reprise": _ex_reprise,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "<code>play(0.0, 2.0)</code> : fondu d'entrée de 2 s, l'intro, puis la section bouclée qui se répète (avec le son).",
		"classe-MusicLooper": "Intro, section bouclée entre <code>loop_start</code> et <code>loop_end</code>, fin : la tête de lecture et le compteur de boucles en direct.",
		"MusicLooper.loop_enabled": "<code>loop_enabled = false</code> : le morceau est joué une fois du début à la fin, puis <code>finished</code>.",
		"MusicLooper.set_paused": "<code>set_paused(true)</code> suspend la lecture à la même position, <code>set_paused(false)</code> la reprend.",
		"ex-combat": "Exploration → combat : fondu de sortie d'1 s, changement de morceau, fondu d'entrée.",
		"ex-fin": "<code>loop_count = 2</code> : <code>looped(1)</code>, <code>looped(2)</code>, puis la fin du morceau et <code>finished</code>.",
		"ex-pause": "Menu de pause : <code>volume_db = -12</code>, appliqué immédiatement, puis 0 à la fermeture.",
		"ex-reprise": "Entrée au magasin : position sauvegardée et fondu de sortie ; à la sortie, <code>play(position_sauvee, 0.5)</code>.",
	}


func schemas() -> Dictionary:
	return {
		"figure-1": ["forme_onde.svg", "Un morceau en trois zones : l'intro avant <code>loop_start</code>, la section bouclée jusqu'à <code>loop_end</code> (répétée <code>loop_count</code> fois, ou sans fin), puis la fin."],
		"inspecteur": ["inspecteur.svg", "Dans l'inspecteur : <code>autoplay</code>, <code>bus</code>, et les boutons Écouter (<code>listen</code>) / Arrêter (<code>stop_listening</code>) pour régler la boucle à l'oreille, sans lancer le jeu."],
		"precision": ["precision.svg", "<code>is_loop_sample_accurate()</code> : avec <code>loop_end</code> = 0 (ou un WAV), la boucle est faite par le flux audio lui-même, exacte à l'échantillon ; sinon elle est vérifiée à chaque image."],
	}


func reutilisations() -> Dictionary:
	return {
		"MusicLooper.stream": "classe-MusicLooper",
		"MusicLooper.loop_start": ["classe-MusicLooper", "<code>loop_start</code> = 2 s : la fin de l'intro, où la lecture revient à chaque boucle."],
		"MusicLooper.loop_end": ["classe-MusicLooper", "<code>loop_end</code> = 6 s : la fin de la section bouclée (0 : fin du fichier)."],
		"MusicLooper.get_effective_loop_end": "classe-MusicLooper",
		"MusicLooper.get_playback_position": "classe-MusicLooper",
		"MusicLooper.is_playing": "classe-MusicLooper",
		"MusicLooper.loop_count": "ex-fin",
		"MusicLooper.get_loops_done": "ex-fin",
		"MusicLooper.looped": "ex-fin",
		"MusicLooper.finished": "ex-fin",
		"MusicLooper.volume_db": "ex-pause",
		"MusicLooper.play": "mode-emploi",
		"MusicLooper.stop": "ex-combat",
		"MusicLooper.is_paused": "MusicLooper.set_paused",
		"MusicLooper.autoplay": "inspecteur",
		"MusicLooper.bus": "inspecteur",
		"MusicLooper.listen": "inspecteur",
		"MusicLooper.stop_listening": "inspecteur",
		"MusicLooper.is_loop_sample_accurate": "precision",
	}


## Morceau en trois sections (notes de 0,25 s) ; `grave` : variante pour un second morceau.
func _morceau(grave: bool = false) -> AudioStreamWAV:
	var f := 0.75 if grave else 1.0
	var notes := PackedFloat32Array()
	for n in [262.0, 330.0, 392.0, 523.0, 392.0, 330.0, 392.0, 523.0]:       # intro (2 s)
		notes.append(n * f)
	for n in [659.0, 587.0, 523.0, 587.0, 659.0, 659.0, 659.0, 587.0, 587.0, 587.0, 659.0, 784.0, 784.0, 659.0, 587.0, 523.0]:  # boucle (4 s)
		notes.append(n * f)
	for n in [523.0, 392.0, 330.0, 262.0, 196.0, 196.0, 131.0, 131.0]:      # fin (2 s)
		notes.append(n * f)
	return Sons.melodie(notes, 0.25)


func _preparer(reglages: Dictionary = {}) -> void:
	_musique = MusicLooper.new()
	_musique.stream = _morceau()
	_musique.loop_start = INTRO
	_musique.loop_end = FIN_BOUCLE
	for propriete in reglages:
		_musique.set(propriete, reglages[propriete])
	scene.add_child(_musique)
	var frise := Frise.new()
	frise.musique = _musique
	frise.position = Vector2(126, 300)
	scene.add_child(frise)
	var vumetre := Sons.Vumetre.new()
	vumetre.position = Vector2(456, 450)
	scene.add_child(vumetre)
	_journal = etiquette(Vector2(576, 380), "", 18, ACCENT)
	_musique.looped.connect(func(n: int): _noter("looped(%d)" % n))
	_musique.finished.connect(func(): _noter("finished"))


func _noter(texte: String) -> void:
	_lignes.append(texte)
	_journal.text = "  ·  ".join(_lignes.slice(-5))


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : une musique avec intro et boucle")
	_preparer()
	code("$MusicLooper.loop_start = 2.0   # fin de l'intro\n$MusicLooper.play(0.0, 2.0)       # fondu d'entrée de 2 s")
	legende("play(0.0, 2.0) : fondu d'entrée, l'intro, puis la boucle")
	_musique.play(0.0, 2.0)
	await secondes(13.0)
	_musique.stop(1.0)
	await secondes(1.0)


# 💻 API

func _classe() -> void:
	sujet("MusicLooper : intro, boucle, fin")
	_preparer()
	code("$MusicLooper.loop_start = 2.0\n$MusicLooper.loop_end = 6.0\n$MusicLooper.play()")
	_musique.play()
	await secondes(11.0)


func _loop_enabled() -> void:
	sujet("loop_enabled : désactiver la boucle")
	_preparer({"loop_enabled": false})
	code("$MusicLooper.loop_enabled = false")
	_musique.play()
	await secondes(8.8)


func _set_paused() -> void:
	sujet("set_paused() et is_paused()")
	_preparer()
	var etat := etiquette(Vector2(576, 180), "", 20, TEXTE)
	_musique.play()
	for i in 3:
		await secondes(1.6)
		_musique.set_paused(true)
		etat.text = "set_paused(true) · is_paused() = %s" % _musique.is_paused()
		code("$MusicLooper.set_paused(true)")
		await secondes(1.2)
		_musique.set_paused(false)
		etat.text = "set_paused(false) · is_paused() = %s" % _musique.is_paused()
		code("$MusicLooper.set_paused(false)")


# 📌 Exemples

func _ex_combat() -> void:
	sujet("Exemple : de l'exploration au combat")
	_preparer()
	var etat := etiquette(Vector2(576, 170), "Exploration", 24, VARIANTES[0])
	code("Musique.stop(1.0)\nawait get_tree().create_timer(1.0).timeout\nMusique.stream = morceau\nMusique.play(0.0, 1.0)")
	_musique.play()
	await secondes(3.5)
	etat.text = "Un ennemi ! stop(1.0)"
	_musique.stop(1.0)
	await secondes(1.0)
	_musique.stream = _morceau(true)
	etat.text = "Combat : play(0.0, 1.0)"
	etat.add_theme_color_override("font_color", MAUVAIS)
	_musique.play(0.0, 1.0)
	await secondes(5.0)


func _ex_fin() -> void:
	sujet("Exemple : la boucle deux fois, puis la fin")
	_preparer({"loop_count": 2})
	code("$MusicLooper.loop_count = 2\n$MusicLooper.looped.connect(func(n): print(\"Boucle \", n))\n$MusicLooper.finished.connect(_generique)")
	_musique.play()
	await secondes(16.5)


func _ex_pause() -> void:
	sujet("Exemple : baisser la musique en pause")
	_preparer()
	var etat := etiquette(Vector2(576, 170), "", 22, TEXTE)
	_musique.play()
	for i in 2:
		etat.text = "En jeu : volume_db = 0"
		code("Musique.volume_db = 0.0")
		await secondes(2.0)
		etat.text = "Menu de pause : volume_db = -12"
		code("Musique.volume_db = -12.0")
		_musique.volume_db = -12.0
		await secondes(2.0)
		_musique.volume_db = 0.0


func _ex_reprise() -> void:
	sujet("Exemple : reprendre là où on s'était arrêté")
	_preparer()
	var etat := etiquette(Vector2(576, 170), "Dehors", 22, TEXTE)
	_musique.play()
	await secondes(3.3)
	var sauvee := _musique.get_playback_position()
	etat.text = "entrer_magasin() : position sauvée = %.1f s, stop(0.5)" % sauvee
	code("position_sauvee = Musique.get_playback_position()\nMusique.stop(0.5)")
	_musique.stop(0.5)
	await secondes(2.0)
	etat.text = "sortir_magasin() : play(%.1f, 0.5)" % sauvee
	code("Musique.play(position_sauvee, 0.5)")
	_musique.play(sauvee, 0.5)
	await secondes(3.5)
