extends RefCounted

## 🔊 Sons synthétisés pour les démos (aucun fichier audio à embarquer) : AudioStreamWAV 16 bits mono.
## const Sons := preload("res://🎥Démos de documentation/socle/sons.gd") ; Sons.bip(880.0, 0.12)...

const FREQUENCE := 22050


## Note pure qui s'éteint (`frequence` Hz, `duree` s).
static func bip(frequence: float = 880.0, duree: float = 0.15, volume: float = 0.5) -> AudioStreamWAV:
	return _creer(duree, func(t: float) -> float:
		return sin(TAU * frequence * t) * exp(-t * 6.0 / duree) * volume)


## Impact sourd : bruit filtré + basse qui chute.
static func impact(duree: float = 0.25) -> AudioStreamWAV:
	var bruit := RandomNumberGenerator.new()
	bruit.seed = 7
	var precedent := [0.0]
	return _creer(duree, func(t: float) -> float:
		precedent[0] = lerpf(precedent[0], bruit.randf_range(-1.0, 1.0), 0.25)
		var basse := sin(TAU * (120.0 - 200.0 * t) * t)
		return (precedent[0] * 0.6 + basse * 0.6) * exp(-t * 14.0))


## Bruit de pas (claquement court) ; `variante` change le timbre.
static func pas(variante: int = 0) -> AudioStreamWAV:
	var bruit := RandomNumberGenerator.new()
	bruit.seed = 31 + variante
	var tonalite: float = [300.0, 420.0, 230.0][variante % 3]
	return _creer(0.09, func(t: float) -> float:
		return (bruit.randf_range(-1.0, 1.0) * 0.5 + sin(TAU * tonalite * t) * 0.5) * exp(-t * 45.0))


## Alarme à deux tons, en boucle (loop_mode activé).
static func alarme(duree: float = 0.8) -> AudioStreamWAV:
	var son := _creer(duree, func(t: float) -> float:
		var note: float = 660.0 if fmod(t, duree) < duree / 2.0 else 520.0
		return signf(sin(TAU * note * t)) * 0.18)
	son.loop_mode = AudioStreamWAV.LOOP_FORWARD
	son.loop_end = int(duree * FREQUENCE)
	return son


## Mélodie en boucle (arpège) de `duree` s : pour les démos de musique.
static func melodie(notes: PackedFloat32Array, duree_note: float = 0.25, boucle: bool = false) -> AudioStreamWAV:
	var duree := duree_note * notes.size()
	var son := _creer(duree, func(t: float) -> float:
		var index := mini(int(t / duree_note), notes.size() - 1)
		var local := t - index * duree_note
		return (sin(TAU * notes[index] * t) * 0.35 + sin(TAU * notes[index] * 2.0 * t) * 0.1) * minf(1.0, exp(-local * 5.0) + 0.15))
	if boucle:
		son.loop_mode = AudioStreamWAV.LOOP_FORWARD
		son.loop_end = int(duree * FREQUENCE)
	return son


static func _creer(duree: float, echantillon: Callable) -> AudioStreamWAV:
	var nombre := int(duree * FREQUENCE)
	var donnees := PackedByteArray()
	donnees.resize(nombre * 2)
	for i in nombre:
		donnees.encode_s16(i * 2, int(clampf(echantillon.call(float(i) / FREQUENCE), -1.0, 1.0) * 32000.0))
	var son := AudioStreamWAV.new()
	son.format = AudioStreamWAV.FORMAT_16_BITS
	son.mix_rate = FREQUENCE
	son.stereo = false
	son.data = donnees
	return son


## VU-mètre du bus `bus` (barre horizontale), à ajouter à la scène.
class Vumetre extends Node2D:
	var bus := 0
	var largeur := 240.0
	var _niveau := 0.0

	func _process(delta: float) -> void:
		var db := maxf(AudioServer.get_bus_peak_volume_left_db(bus, 0), AudioServer.get_bus_peak_volume_right_db(bus, 0))
		var cible := clampf(inverse_lerp(-50.0, 0.0, db), 0.0, 1.0)
		_niveau = maxf(cible, _niveau - delta * 1.5)
		queue_redraw()

	func _draw() -> void:
		draw_rect(Rect2(0, 0, largeur, 14), Color(0, 0, 0, 0.35))
		draw_rect(Rect2(0, 0, largeur * _niveau, 14), Color("5ccf7a").lerp(Color("ef5f5f"), _niveau * _niveau))
		draw_string(ThemeDB.fallback_font, Vector2(0, -6), "Volume (bus Master)", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(1, 1, 1, 0.6))
