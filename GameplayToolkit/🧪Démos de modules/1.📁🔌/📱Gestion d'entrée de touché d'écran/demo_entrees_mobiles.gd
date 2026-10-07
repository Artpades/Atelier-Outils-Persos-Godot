extends Control

## 📱 Démo de MobileInput (module mobile_input).
##
## - Au lancement, exécute la suite de tests automatiques (suite_tests_entrees_mobiles.gd) et affiche le bilan.
## - Zone tactile : une carte à manipuler. Glisser d'un doigt la déplace, pincer la redimensionne,
##   un appui long change sa couleur, un double tap la réinitialise. Les doigts posés, les taps et les
##   swipes sont dessinés ; tous les signaux sont notés dans le journal.
## - Sur ordinateur, la souris émule un doigt (case à cocher) ; les boutons simulent les gestes à deux doigts.
## - Capteurs : valeurs lues en direct et niveau à bulle piloté par la gravité (inclinaison simulable).

const JournalTests := preload("../../2.🧭🌍/journal_tests.gd")
const SuiteTests := preload("suite_tests_entrees_mobiles.gd")
const NOMS_SWIPE := ["aucune", "⬅️ gauche", "➡️ droite", "⬆️ haut", "⬇️ bas"]
const COULEURS_CARTE := [Color.SEA_GREEN, Color.ROYAL_BLUE, Color.ORANGE, Color.MEDIUM_PURPLE, Color.FIREBRICK]
## Index des doigts simulés par les boutons (pour ne pas se mélanger au doigt émulé par la souris, index 0).
const DOIGT_SIMULE_A := 10
const DOIGT_SIMULE_B := 11

@onready var mobile: MobileInput = %Mobile
@onready var zone: Control = %Zone
@onready var etiquette_capteurs: Label = %EtiquetteCapteurs
@onready var journal: RichTextLabel = %Journal

var _emulation_initiale := false
var _taille_carte := Vector2(160, 110)
var _centre_carte := Vector2.ZERO
var _echelle_carte := 1.0
var _echelle_debut_pincement := 1.0
var _couleur_carte := 0
## Marques éphémères dessinées dans la zone : { position, texte, couleur, vie }.
var _marques: Array[Dictionary] = []
var _inclinaison_simulee := false
var _temps := 0.0
var _simulation_en_cours := false


func _ready() -> void:
	for bouton in find_children("*", "BaseButton"):
		bouton.focus_mode = Control.FOCUS_NONE

	_emulation_initiale = Input.is_emulating_touch_from_mouse()
	%CaseSouris.button_pressed = true
	%CaseSouris.toggled.connect(func(actif: bool): Input.set_emulate_touch_from_mouse(actif))
	Input.set_emulate_touch_from_mouse(true)
	%CaseInclinaison.toggled.connect(_sur_inclinaison_simulee)

	%BoutonPincement.pressed.connect(_simuler_pincement)
	%BoutonSwipe.pressed.connect(_simuler_swipe)
	%BoutonDoubleTap.pressed.connect(_simuler_double_tap)
	%BoutonTests.pressed.connect(_lancer_tests)

	_connecter_signaux()
	zone.draw.connect(_dessiner_zone)
	zone.resized.connect(_recentrer_carte)
	_recentrer_carte.call_deferred()

	_lancer_tests()


func _exit_tree() -> void:
	Input.set_emulate_touch_from_mouse(_emulation_initiale)
	if _inclinaison_simulee:
		Input.set_gravity(Vector3.ZERO)


func _connecter_signaux() -> void:
	mobile.touch_pressed.connect(func(index: int, point: Vector2): _journaliser("[color=gray]👇 touch_pressed(%d, %s)[/color]" % [index, _local(point).round()]))
	mobile.touch_released.connect(func(index: int, point: Vector2): _journaliser("[color=gray]☝️ touch_released(%d, %s)[/color]" % [index, _local(point).round()]))
	mobile.multi_touch_changed.connect(func(nombre: int): _journaliser("[color=gray]🖐️ multi_touch_changed(%d)[/color]" % nombre))
	mobile.drag.connect(_sur_glissement)

	mobile.tap.connect(func(point: Vector2):
		_journaliser("[color=cyan]👆 tap %s[/color]" % _local(point).round())
		_marquer(point, "tap", Color.CYAN))
	mobile.double_tap.connect(func(point: Vector2):
		_journaliser("[color=cyan][b]👆👆 double_tap %s[/b] → carte réinitialisée[/color]" % _local(point).round())
		_marquer(point, "double tap", Color.AQUAMARINE)
		_recentrer_carte())
	mobile.long_press.connect(func(index: int, point: Vector2):
		_couleur_carte = (_couleur_carte + 1) % COULEURS_CARTE.size()
		_journaliser("[color=orange]⏳ long_press(%d) → couleur suivante[/color]" % index)
		_marquer(point, "appui long", Color.ORANGE))
	mobile.swipe.connect(func(direction: int, depart: Vector2, vitesse: Vector2):
		_journaliser("[color=yellow][b]👉 swipe %s[/b] (%.0f px/s)[/color]" % [NOMS_SWIPE[direction], vitesse.length()])
		_marquer(depart, "swipe " + NOMS_SWIPE[direction], Color.YELLOW))

	mobile.pinch_started.connect(func(_centre: Vector2):
		_echelle_debut_pincement = _echelle_carte
		_journaliser("[color=violet]🤏 pinch_started[/color]"))
	mobile.pinch.connect(func(facteur: float, _centre: Vector2):
		_echelle_carte = clampf(_echelle_debut_pincement * facteur, 0.3, 4.0))
	mobile.pinch_ended.connect(func(): _journaliser("[color=violet]🤏 pinch_ended → échelle %.2f[/color]" % _echelle_carte))

	mobile.pan.connect(func(delta: Vector2): _centre_carte += delta * 10.0)
	mobile.magnify.connect(func(facteur: float): _echelle_carte = clampf(_echelle_carte * facteur, 0.3, 4.0))


func _sur_glissement(_index: int, _position: Vector2, relatif: Vector2, _pression: float) -> void:
	# Un seul doigt : la carte suit le doigt. À deux doigts, c'est le pincement qui agit.
	if mobile.get_finger_count() == 1:
		_centre_carte += relatif


func _process(delta: float) -> void:
	_temps += delta
	if _inclinaison_simulee:
		Input.set_gravity(Vector3(sin(_temps * 1.3) * 4.0, -9.8 + absf(cos(_temps)) * 2.0, cos(_temps * 0.9) * 4.0))

	for marque in _marques:
		marque.vie -= delta
	_marques = _marques.filter(func(m): return m.vie > 0.0)

	etiquette_capteurs.text = "📱 Gyroscope : %s\n📐 Accéléromètre : %s\n🌍 Gravité : %s\n🧭 Magnétomètre : %s\n🖐️ Doigts : %d%s" % [
		_vecteur(mobile.get_gyroscope()), _vecteur(mobile.get_accelerometer()), _vecteur(mobile.get_gravity()),
		_vecteur(mobile.get_magnetometer()), mobile.get_finger_count(), "  (pincement)" if mobile.is_pinching() else ""]
	zone.queue_redraw()


# 🎨 Dessin de la zone tactile

func _dessiner_zone() -> void:
	var police := get_theme_default_font()

	# Carte manipulable
	var taille := _taille_carte * _echelle_carte
	zone.draw_rect(Rect2(_centre_carte - taille / 2.0, taille), COULEURS_CARTE[_couleur_carte])
	zone.draw_string(police, _centre_carte + Vector2(-taille.x / 2.0 + 8, 6), "×%.2f" % _echelle_carte, HORIZONTAL_ALIGNMENT_LEFT, -1, 18)

	# Niveau à bulle : la bulle se déplace avec la gravité (x, z)
	var centre_niveau := Vector2(zone.size.x - 70, 70)
	zone.draw_circle(centre_niveau, 50, Color(1, 1, 1, 0.08))
	zone.draw_arc(centre_niveau, 50, 0, TAU, 48, Color(1, 1, 1, 0.4), 2)
	var gravite := mobile.get_gravity()
	var bulle := Vector2(gravite.x, gravite.z).limit_length(1.0) * 40.0
	zone.draw_circle(centre_niveau + bulle, 10, Color.LIME_GREEN if gravite != Vector3.ZERO else Color.DIM_GRAY)
	zone.draw_string(police, centre_niveau + Vector2(-30, 68), "gravité", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(1, 1, 1, 0.6))

	# Doigts posés
	var doigts := mobile.get_fingers()
	for index in doigts:
		var doigt: Dictionary = doigts[index]
		var point := _local(doigt.position)
		zone.draw_line(_local(doigt.start_position), point, Color(1, 1, 1, 0.3), 2)
		zone.draw_circle(point, 28, Color(1, 1, 1, 0.25))
		zone.draw_arc(point, 28, 0, TAU, 32, Color.WHITE, 2)
		zone.draw_string(police, point + Vector2(-5, 6), str(index), HORIZONTAL_ALIGNMENT_LEFT, -1, 18)
	if doigts.size() == 2:
		var positions := doigts.values().map(func(d): return _local(d.position))
		zone.draw_dashed_line(positions[0], positions[1], Color.VIOLET, 2, 8)

	# Marques des gestes
	for marque in _marques:
		var couleur: Color = marque.couleur
		couleur.a = clampf(marque.vie, 0.0, 1.0)
		var point := _local(marque.position)
		zone.draw_arc(point, 20 + (1.0 - marque.vie) * 30, 0, TAU, 32, couleur, 3)
		zone.draw_string(police, point + Vector2(-30, -40), marque.texte, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, couleur)


## Position globale (évènements tactiles) → position dans la zone.
func _local(point: Vector2) -> Vector2:
	return point - zone.global_position


func _marquer(point: Vector2, texte: String, couleur: Color) -> void:
	_marques.append({"position": point, "texte": texte, "couleur": couleur, "vie": 1.2})


func _recentrer_carte() -> void:
	_centre_carte = zone.size / 2.0
	_echelle_carte = 1.0


func _vecteur(v: Vector3) -> String:
	return "(%.2f, %.2f, %.2f)" % [v.x, v.y, v.z]


func _journaliser(texte: String) -> void:
	journal.append_text(texte + "\n")


# 🤖 Gestes simulés (passent par Input, donc par MobileInput._input comme un vrai toucher)

## Point de la zone → coordonnées de la fenêtre, celles des vrais évènements (l'étirement canvas_items
## est ensuite défait par le viewport avant _input).
func _vers_fenetre() -> Transform2D:
	return get_viewport().get_final_transform() * zone.get_global_transform_with_canvas()


func _toucher(index: int, point: Vector2, appuye: bool) -> void:
	var evenement := InputEventScreenTouch.new()
	evenement.index = index
	evenement.position = _vers_fenetre() * point
	evenement.pressed = appuye
	Input.parse_input_event(evenement)


func _glisser(index: int, depuis: Vector2, vers: Vector2, duree: float) -> void:
	var evenement := InputEventScreenDrag.new()
	evenement.index = index
	var transformation := _vers_fenetre()
	evenement.position = transformation * vers
	evenement.relative = transformation.basis_xform(vers - depuis)
	evenement.velocity = transformation.basis_xform(vers - depuis) / maxf(duree, 0.001)
	Input.parse_input_event(evenement)


func _simuler_pincement() -> void:
	if _simulation_en_cours:
		return
	_simulation_en_cours = true
	var centre := _centre_carte
	var ecart_depart := 60.0
	var ecart_fin := 140.0 if _echelle_carte < 2.0 else 25.0 # écarte, ou resserre si déjà grande
	_toucher(DOIGT_SIMULE_A, centre - Vector2(ecart_depart, 0), true)
	_toucher(DOIGT_SIMULE_B, centre + Vector2(ecart_depart, 0), true)
	var ecart := ecart_depart
	for etape in 20:
		await get_tree().process_frame
		var suivant := lerpf(ecart_depart, ecart_fin, (etape + 1) / 20.0)
		_glisser(DOIGT_SIMULE_A, centre - Vector2(ecart, 0), centre - Vector2(suivant, 0), 1.0 / 60.0)
		_glisser(DOIGT_SIMULE_B, centre + Vector2(ecart, 0), centre + Vector2(suivant, 0), 1.0 / 60.0)
		ecart = suivant
	_toucher(DOIGT_SIMULE_A, centre - Vector2(ecart, 0), false)
	_toucher(DOIGT_SIMULE_B, centre + Vector2(ecart, 0), false)
	_simulation_en_cours = false


func _simuler_swipe() -> void:
	if _simulation_en_cours:
		return
	_simulation_en_cours = true
	var depart := Vector2(zone.size.x * 0.25, zone.size.y * 0.8)
	var arrivee := Vector2(zone.size.x * 0.75, zone.size.y * 0.8)
	_toucher(DOIGT_SIMULE_A, depart, true)
	var precedent := depart
	for etape in 8:
		await get_tree().process_frame
		var suivant := depart.lerp(arrivee, (etape + 1) / 8.0)
		_glisser(DOIGT_SIMULE_A, precedent, suivant, 1.0 / 60.0)
		precedent = suivant
	_toucher(DOIGT_SIMULE_A, arrivee, false)
	_simulation_en_cours = false


func _simuler_double_tap() -> void:
	if _simulation_en_cours:
		return
	_simulation_en_cours = true
	var point := Vector2(zone.size.x * 0.3, zone.size.y * 0.3)
	for i in 2:
		_toucher(DOIGT_SIMULE_A, point, true)
		await get_tree().process_frame
		_toucher(DOIGT_SIMULE_A, point, false)
		await get_tree().create_timer(0.1).timeout
	_simulation_en_cours = false


func _sur_inclinaison_simulee(actif: bool) -> void:
	_inclinaison_simulee = actif
	if not actif:
		Input.set_gravity(Vector3.ZERO)


# 🧪 Tests automatiques

func _lancer_tests() -> void:
	var resultats: Array[Dictionary] = await SuiteTests.new().executer(self)
	_journaliser(JournalTests.formater("Tests des entrées mobiles", resultats))
