extends Control

## 🔁 Démo du MusicLooper : intro + section bouclée + fin, fondus d'entrée / de sortie, nombre de boucles.
##
## La frise montre l'intro, la section bouclée et la fin du morceau, ainsi que la position de lecture.
## Le choix du flux montre les deux façons de boucler : native (exacte à l'échantillon) ou par frame.

const Panneau := preload("../2.🧭🌍/panneau_demo.gd")

@export var musique_wav: AudioStream
@export var musique_ogg: AudioStream

@onready var panneau: Panneau = $Interface/Panneau
@onready var lecteur: MusicLooper = $Lecteur
@onready var frise: Control = $Frise
@onready var etat: Label = $Etat

var _curseur_debut: HSlider
var _curseur_fin: HSlider


func _ready() -> void:
	lecteur.looped.connect(func(n: int): panneau.journaliser("🔁 Boucle n° %d" % n))
	lecteur.finished.connect(func(): panneau.journaliser("⏹️ Morceau terminé"))
	frise.draw.connect(_dessiner_frise)

	panneau.ajouter_section("▶️ Lecture")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Jouer", _jouer.bind(0.0), rangee)
	panneau.ajouter_bouton("Jouer (fondu 1 s)", _jouer.bind(1.0), rangee)
	rangee = panneau.ajouter_rangee()
	panneau.ajouter_bouton("Arrêter", lecteur.stop.bind(0.0), rangee)
	panneau.ajouter_bouton("Arrêter (fondu 1,5 s)", lecteur.stop.bind(1.5), rangee)
	panneau.ajouter_bouton("Pause", func(): lecteur.set_paused(not lecteur.is_paused()), rangee)
	panneau.ajouter_bouton("Sauter juste avant la fin de boucle", func(): _jouer(0.0, lecteur.get_effective_loop_end() - 1.0))

	panneau.ajouter_section("🔁 Boucle")
	panneau.ajouter_options("Flux", PackedStringArray(["WAV (exacte)", "OGG, fin à 6 s (par frame)", "OGG, jusqu'à la fin (exacte)"]), 0, _choisir_flux)
	_curseur_debut = panneau.ajouter_curseur("Début (s)", 0.0, 8.0, 0.25, lecteur.loop_start, func(v: float): lecteur.loop_start = v)
	_curseur_fin = panneau.ajouter_curseur("Fin (s, 0 = fin)", 0.0, 8.0, 0.25, lecteur.loop_end, func(v: float): lecteur.loop_end = v)
	panneau.ajouter_curseur("Nombre (-1 = infini)", -1.0, 5.0, 1.0, lecteur.loop_count, func(v: float): lecteur.loop_count = int(v))
	panneau.ajouter_curseur("Volume (dB)", -40.0, 6.0, 1.0, lecteur.volume_db, func(v: float): lecteur.volume_db = v)
	var note := panneau.ajouter_etiquette("Les réglages de boucle s'appliquent au prochain « Jouer ».")
	note.modulate.a = 0.7


func _process(_delta: float) -> void:
	frise.queue_redraw()
	etat.text = "Position : %.2f s / %.2f s\nBoucles faites : %d\nBoucle exacte à l'échantillon : %s\nÉtat : %s" % [
		lecteur.get_playback_position(), _duree(), lecteur.get_loops_done(),
		"oui" if lecteur.is_loop_sample_accurate() else "non (retour par frame)",
		"en pause" if lecteur.is_paused() else ("lecture" if lecteur.is_playing() else "arrêté")]


func _jouer(fondu: float, depuis: float = 0.0) -> void:
	lecteur.play(maxf(depuis, 0.0), fondu)
	panneau.journaliser("▶️ Lecture depuis %.2f s (boucle %.2f → %.2f s)" % [maxf(depuis, 0.0), lecteur.loop_start, lecteur.get_effective_loop_end()])


func _choisir_flux(index: int) -> void:
	lecteur.stop()
	lecteur.stream = musique_wav if index == 0 else musique_ogg
	_curseur_fin.value = 0.0 if index == 2 else 6.0
	_curseur_debut.value = 2.0


func _duree() -> float:
	return lecteur.stream.get_length() if lecteur.stream else 0.0


func _dessiner_frise() -> void:
	var taille := frise.size
	var duree := _duree()
	if duree <= 0.0:
		return
	var x := func(t: float) -> float: return t / duree * taille.x
	var debut: float = x.call(lecteur.loop_start)
	var fin: float = x.call(lecteur.get_effective_loop_end())

	frise.draw_rect(Rect2(Vector2.ZERO, taille), Color(0.25, 0.25, 0.32))
	frise.draw_rect(Rect2(debut, 0, fin - debut, taille.y), Color(0.3, 0.55, 0.9, 0.6))
	var police := get_theme_default_font()
	frise.draw_string(police, Vector2(6, 20), "Intro")
	frise.draw_string(police, Vector2(debut + 6, 20), "Boucle")
	frise.draw_string(police, Vector2(fin + 6, 20), "Fin")
	for t in range(int(duree) + 1):
		frise.draw_line(Vector2(x.call(t), taille.y - 8), Vector2(x.call(t), taille.y), Color(1, 1, 1, 0.5))

	var tete: float = x.call(lecteur.get_playback_position())
	frise.draw_line(Vector2(tete, 0), Vector2(tete, taille.y), Color(1, 0.8, 0.3), 3.0)
