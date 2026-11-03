extends "../../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques de MobileInput (module mobile_input) : doigts, glisser, limite de doigts,
## tap / double tap / appui long, swipe, pincement, toucher annulé, gestes du pavé tactile, passage par
## _input et capteurs (sensibilité, zone morte, signaux _updated / _changed).
## Les touchers sont simulés avec handle_input_event (sauf le test de _input) ; les capteurs avec
## Input.set_gravity / set_accelerometer..., remis à zéro en fin de suite.


func _executer() -> void:
	_tester_valeurs_par_defaut()
	_tester_toucher_simple()
	_tester_glisser()
	_tester_limite_doigts()
	await _tester_tap()
	await _tester_double_tap()
	await _tester_taps_eloignes()
	await _tester_tap_trop_long()
	await _tester_tap_trop_deplace()
	await _tester_appui_long()
	_tester_swipe()
	_tester_pincement()
	await _tester_doigts_multi_touch_sans_geste()
	await _tester_toucher_annule()
	_tester_gestes_pave_tactile()
	await _tester_passage_par_input()
	_tester_reinitialisation()
	await _tester_capteurs()
	await _tester_capteur_desactive()


# 🛠️ Utilitaires

func _mobile() -> MobileInput:
	var mobile := MobileInput.new()
	# Capteurs coupés par défaut dans les tests tactiles : seuls les tests de capteurs les activent.
	for capteur in MobileInput.SENSOR_MAX:
		mobile.set_sensor_enabled(capteur, false)
	return _ajouter(mobile) as MobileInput


func _appuyer(mobile: MobileInput, index: int, position: Vector2) -> void:
	var evenement := InputEventScreenTouch.new()
	evenement.index = index
	evenement.position = position
	evenement.pressed = true
	mobile.handle_input_event(evenement)


func _relacher(mobile: MobileInput, index: int, position: Vector2, annule := false) -> void:
	var evenement := InputEventScreenTouch.new()
	evenement.index = index
	evenement.position = position
	evenement.pressed = false
	evenement.canceled = annule
	mobile.handle_input_event(evenement)


func _glisser(mobile: MobileInput, index: int, position: Vector2, relatif: Vector2, vitesse := Vector2.ZERO, pression := 1.0) -> void:
	var evenement := InputEventScreenDrag.new()
	evenement.index = index
	evenement.position = position
	evenement.relative = relatif
	evenement.velocity = vitesse
	evenement.pressure = pression
	mobile.handle_input_event(evenement)


## Tap complet (appui + relâchement immédiat) du doigt 0 en p_position.
func _taper(mobile: MobileInput, position: Vector2) -> void:
	_appuyer(mobile, 0, position)
	_relacher(mobile, 0, position)


# 🧪 Tests

func _tester_valeurs_par_defaut() -> void:
	var mobile := MobileInput.new()
	_verifier("Réglages tactiles par défaut", is_equal_approx(mobile.tap_max_duration, 0.3) and mobile.tap_max_distance == 20.0
		and is_equal_approx(mobile.double_tap_delay, 0.4) and mobile.swipe_min_distance == 100.0 and mobile.swipe_min_velocity == 500.0
		and mobile.max_fingers == 5)
	_verifier("Capteurs actifs par défaut, sensibilités propres à chacun", mobile.use_gyroscope and mobile.use_accelerometer
		and mobile.use_gravity and mobile.use_magnetometer and mobile.gyroscope_sensitivity == 1.0
		and is_equal_approx(mobile.accelerometer_sensitivity, 0.1) and is_equal_approx(mobile.magnetometer_sensitivity, 0.02),
		"gyro %s, accel %s, magnéto %s" % [mobile.gyroscope_sensitivity, mobile.accelerometer_sensitivity, mobile.magnetometer_sensitivity])
	_verifier("Aucun doigt au départ", mobile.get_finger_count() == 0 and mobile.get_primary_finger_position() == null)
	mobile.free()


func _tester_toucher_simple() -> void:
	var mobile := _mobile()
	var appuis := _espionner(mobile.touch_pressed)
	var relachements := _espionner(mobile.touch_released)
	var nombre := _espionner(mobile.multi_touch_changed)

	_appuyer(mobile, 3, Vector2(50, 60))
	_verifier("touch_pressed(index, position)", appuis.n == 1 and appuis.args == [3, Vector2(50, 60)], "%s" % [appuis])
	_verifier("multi_touch_changed(1) à l'appui", nombre.n == 1 and nombre.args[0] == 1, "%s" % [nombre])
	_verifier("Doigt suivi : has_finger, get_finger_position, doigt principal", mobile.has_finger(3)
		and mobile.get_finger_position(3) == Vector2(50, 60) and mobile.get_primary_finger_position() == Vector2(50, 60))
	_verifier("Doigt absent : get_finger_position renvoie null", mobile.get_finger_position(7) == null)

	_appuyer(mobile, 1, Vector2(10, 10))
	_verifier("Doigt principal = plus petit index", mobile.get_primary_finger_position() == Vector2(10, 10))
	var doigts := mobile.get_fingers()
	_verifier("get_fingers : un dictionnaire par doigt", doigts.size() == 2 and doigts[3].start_position == Vector2(50, 60)
		and doigts[3].has("velocity") and doigts[3].has("press_time"), "%s" % doigts)

	_relacher(mobile, 3, Vector2(55, 60))
	_verifier("touch_released(index, position)", relachements.n == 1 and relachements.args == [3, Vector2(55, 60)], "%s" % [relachements])
	_verifier("Doigt oublié au relâchement", not mobile.has_finger(3) and mobile.get_finger_count() == 1)
	_relacher(mobile, 9, Vector2.ZERO)
	_verifier("Relâchement d'un doigt inconnu ignoré", relachements.n == 1 and nombre.n == 3, "relâchements %d, changements %d" % [relachements.n, nombre.n])


func _tester_glisser() -> void:
	var mobile := _mobile()
	var glissements := _espionner(mobile.drag)
	var details := []
	mobile.drag.connect(func(index, position, relatif, pression): details.assign([index, position, relatif, pression]))

	_glisser(mobile, 0, Vector2(20, 20), Vector2(5, 5))
	_verifier("Glisser sans doigt posé ignoré", glissements.n == 0)

	_appuyer(mobile, 0, Vector2(10, 10))
	_glisser(mobile, 0, Vector2(40, 30), Vector2(30, 20), Vector2(300, 200), 0.5)
	_verifier("drag(index, position, relative, pressure)", details == [0, Vector2(40, 30), Vector2(30, 20), 0.5], "%s" % [details])
	# Corrige le défaut de la version C# (struct copiée : la position n'était jamais mise à jour).
	_verifier("La position du doigt suit le glissement", mobile.get_finger_position(0) == Vector2(40, 30))
	_verifier("La vitesse du doigt est mémorisée", mobile.get_fingers()[0].velocity == Vector2(300, 200))


func _tester_limite_doigts() -> void:
	var mobile := _mobile()
	mobile.max_fingers = 2
	var appuis := _espionner(mobile.touch_pressed)
	for i in 4:
		_appuyer(mobile, i, Vector2(i * 50, 0))
	_verifier("max_fingers : les doigts en trop sont ignorés", mobile.get_finger_count() == 2 and appuis.n == 2 and not mobile.has_finger(2),
		"%d doigts, %d appuis" % [mobile.get_finger_count(), appuis.n])
	_appuyer(mobile, 0, Vector2(5, 5))
	_verifier("Nouvel appui d'un doigt déjà suivi (relâchement perdu) accepté", mobile.get_finger_position(0) == Vector2(5, 5) and appuis.n == 3)


func _tester_tap() -> void:
	var mobile := _mobile()
	var taps := _espionner(mobile.tap)
	_taper(mobile, Vector2(100, 100))
	_verifier("Tap : pas émis tout de suite (attente d'un éventuel double tap)", taps.n == 0)
	await _secondes(mobile.double_tap_delay + 0.15)
	await _frames(1)
	_verifier("Tap émis après le délai de double tap", taps.n == 1 and taps.args[0] == Vector2(100, 100), "%s" % [taps])
	await _secondes(0.1)
	_verifier("Tap émis une seule fois", taps.n == 1)


func _tester_double_tap() -> void:
	var mobile := _mobile()
	var taps := _espionner(mobile.tap)
	var doubles := _espionner(mobile.double_tap)
	_taper(mobile, Vector2(100, 100))
	await _frames(2)
	_taper(mobile, Vector2(110, 105))
	_verifier("Double tap émis au second relâchement", doubles.n == 1 and doubles.args[0] == Vector2(110, 105), "%s" % [doubles])
	await _secondes(mobile.double_tap_delay + 0.15)
	_verifier("Pas de tap simple après un double tap", taps.n == 0, "%d taps" % taps.n)


func _tester_taps_eloignes() -> void:
	var mobile := _mobile()
	var taps := _espionner(mobile.tap)
	var doubles := _espionner(mobile.double_tap)
	_taper(mobile, Vector2(100, 100))
	_taper(mobile, Vector2(400, 100))
	_verifier("Deux taps éloignés : le premier est émis aussitôt", taps.n == 1 and taps.args[0] == Vector2(100, 100) and doubles.n == 0)
	await _secondes(mobile.double_tap_delay + 0.15)
	_verifier("puis le second après le délai", taps.n == 2 and taps.args[0] == Vector2(400, 100), "%s" % [taps])


func _tester_tap_trop_long() -> void:
	var mobile := _mobile()
	mobile.tap_max_duration = 0.05
	var taps := _espionner(mobile.tap)
	_appuyer(mobile, 0, Vector2(100, 100))
	await _secondes(0.15)
	_relacher(mobile, 0, Vector2(100, 100))
	await _secondes(mobile.double_tap_delay + 0.15)
	_verifier("Appui plus long que tap_max_duration : pas de tap", taps.n == 0)


func _tester_tap_trop_deplace() -> void:
	var mobile := _mobile()
	var taps := _espionner(mobile.tap)
	_appuyer(mobile, 0, Vector2(100, 100))
	_glisser(mobile, 0, Vector2(150, 100), Vector2(50, 0))
	_relacher(mobile, 0, Vector2(150, 100))
	await _secondes(mobile.double_tap_delay + 0.15)
	_verifier("Déplacement supérieur à tap_max_distance : pas de tap", taps.n == 0)


func _tester_appui_long() -> void:
	var mobile := _mobile()
	mobile.long_press_duration = 0.2
	var longs := _espionner(mobile.long_press)
	var taps := _espionner(mobile.tap)
	mobile.tap_max_duration = 1.0 # pour vérifier que l'appui long annule bien le tap
	_appuyer(mobile, 2, Vector2(80, 90))
	await _secondes(0.1)
	_verifier("Pas d'appui long avant long_press_duration", longs.n == 0)
	await _secondes(0.2)
	await _frames(1)
	_verifier("long_press(index, position)", longs.n == 1 and longs.args == [2, Vector2(80, 90)], "%s" % [longs])
	await _secondes(0.1)
	_verifier("Appui long émis une seule fois", longs.n == 1)
	_relacher(mobile, 2, Vector2(80, 90))
	await _secondes(mobile.double_tap_delay + 0.15)
	_verifier("Pas de tap au relâchement d'un appui long", taps.n == 0)

	_appuyer(mobile, 0, Vector2(0, 0))
	_glisser(mobile, 0, Vector2(60, 0), Vector2(60, 0))
	await _secondes(0.3)
	_verifier("Doigt déplacé : pas d'appui long", longs.n == 1)
	_relacher(mobile, 0, Vector2(60, 0))


func _tester_swipe() -> void:
	var mobile := _mobile()
	var swipes := _espionner(mobile.swipe)
	var departs := []
	mobile.swipe.connect(func(_direction, depart, vitesse): departs.assign([depart, vitesse]))

	_appuyer(mobile, 0, Vector2(100, 300))
	_glisser(mobile, 0, Vector2(400, 320), Vector2(300, 20), Vector2(1500, 50))
	_relacher(mobile, 0, Vector2(400, 320))
	_verifier("Swipe vers la droite", swipes.n == 1 and swipes.args[0] == MobileInput.SWIPE_RIGHT, "%s" % [swipes])
	_verifier("Swipe : position de départ et vitesse", departs == [Vector2(100, 300), Vector2(1500, 50)], "%s" % [departs])

	_appuyer(mobile, 0, Vector2(300, 500))
	_glisser(mobile, 0, Vector2(290, 200), Vector2(-10, -300), Vector2(0, -2000))
	_relacher(mobile, 0, Vector2(290, 200))
	_verifier("Swipe vers le haut (axe dominant)", swipes.n == 2 and swipes.args[0] == MobileInput.SWIPE_UP, "%s" % [swipes])

	_appuyer(mobile, 0, Vector2(400, 100))
	_glisser(mobile, 0, Vector2(100, 100), Vector2(-300, 0), Vector2(-100, 0))
	_relacher(mobile, 0, Vector2(100, 100))
	_verifier("Glissement trop lent : pas de swipe", swipes.n == 2)

	_appuyer(mobile, 0, Vector2(100, 100))
	_glisser(mobile, 0, Vector2(150, 100), Vector2(50, 0), Vector2(3000, 0))
	_relacher(mobile, 0, Vector2(150, 100))
	_verifier("Glissement trop court : pas de swipe", swipes.n == 2)


func _tester_pincement() -> void:
	var mobile := _mobile()
	var debuts := _espionner(mobile.pinch_started)
	var fins := _espionner(mobile.pinch_ended)
	var pincements := _espionner(mobile.pinch)

	_appuyer(mobile, 0, Vector2(100, 100))
	_verifier("Un doigt : pas de pincement", not mobile.is_pinching() and debuts.n == 0)
	_appuyer(mobile, 1, Vector2(200, 100))
	_verifier("Second doigt : pinch_started(centre)", mobile.is_pinching() and debuts.n == 1 and debuts.args[0] == Vector2(150, 100), "%s" % [debuts])

	_glisser(mobile, 1, Vector2(300, 100), Vector2(100, 0))
	_verifier("Écartement ×2 : pinch(2.0, centre)", pincements.n == 1 and is_equal_approx(pincements.args[0], 2.0)
		and pincements.args[1] == Vector2(200, 100), "%s" % [pincements])
	_glisser(mobile, 0, Vector2(150, 100), Vector2(50, 0))
	_verifier("Facteur relatif au début du geste : 150 / 100 → 1.5", pincements.n == 2 and is_equal_approx(pincements.args[0], 1.5), "%s" % [pincements])
	_glisser(mobile, 0, Vector2(150.1, 100), Vector2(0.1, 0))
	_verifier("Variation infime : pas de signal", pincements.n == 2)

	_appuyer(mobile, 2, Vector2(500, 500))
	_verifier("Troisième doigt : pinch_ended, plus de pincement", fins.n == 1 and not mobile.is_pinching())
	_glisser(mobile, 1, Vector2(400, 100), Vector2(100, 0))
	_verifier("Pas de pinch à trois doigts", pincements.n == 2)
	_relacher(mobile, 2, Vector2(500, 500))
	_verifier("Retour à deux doigts : le pincement repart", mobile.is_pinching() and debuts.n == 2)
	_relacher(mobile, 1, Vector2(400, 100))
	_verifier("Un doigt levé : pinch_ended", fins.n == 2 and not mobile.is_pinching())


func _tester_doigts_multi_touch_sans_geste() -> void:
	var mobile := _mobile()
	var swipes := _espionner(mobile.swipe)
	var taps := _espionner(mobile.tap)
	# Pincement rapide et ample : chaque doigt, pris seul, ressemblerait à un swipe.
	_appuyer(mobile, 0, Vector2(300, 100))
	_appuyer(mobile, 1, Vector2(400, 100))
	_glisser(mobile, 0, Vector2(100, 100), Vector2(-200, 0), Vector2(-3000, 0))
	_glisser(mobile, 1, Vector2(600, 100), Vector2(200, 0), Vector2(3000, 0))
	_relacher(mobile, 0, Vector2(100, 100))
	_relacher(mobile, 1, Vector2(600, 100))
	# Tap à deux doigts : ce n'est pas un tap simple.
	_appuyer(mobile, 0, Vector2(50, 50))
	_appuyer(mobile, 1, Vector2(80, 50))
	_relacher(mobile, 1, Vector2(80, 50))
	_relacher(mobile, 0, Vector2(50, 50))
	await _secondes(mobile.double_tap_delay + 0.15)
	_verifier("Doigts d'un geste à plusieurs doigts : ni swipe ni tap au relâchement", swipes.n == 0 and taps.n == 0,
		"swipes %d, taps %d" % [swipes.n, taps.n])


func _tester_toucher_annule() -> void:
	var mobile := _mobile()
	var taps := _espionner(mobile.tap)
	var swipes := _espionner(mobile.swipe)
	var relachements := _espionner(mobile.touch_released)
	_appuyer(mobile, 0, Vector2(100, 100))
	_relacher(mobile, 0, Vector2(100, 100), true)
	_appuyer(mobile, 0, Vector2(100, 300))
	_glisser(mobile, 0, Vector2(400, 300), Vector2(300, 0), Vector2(2000, 0))
	_relacher(mobile, 0, Vector2(400, 300), true)
	await _secondes(mobile.double_tap_delay + 0.15)
	_verifier("Toucher annulé : relâché mais ni tap ni swipe", relachements.n == 2 and taps.n == 0 and swipes.n == 0,
		"relâchements %d, taps %d, swipes %d" % [relachements.n, taps.n, swipes.n])


func _tester_gestes_pave_tactile() -> void:
	var mobile := _mobile()
	var pans := _espionner(mobile.pan)
	var magnifies := _espionner(mobile.magnify)
	var pan := InputEventPanGesture.new()
	pan.delta = Vector2(3, -2)
	mobile.handle_input_event(pan)
	var magnify := InputEventMagnifyGesture.new()
	magnify.factor = 1.25
	mobile.handle_input_event(magnify)
	_verifier("pan(delta) relayé", pans.n == 1 and pans.args[0] == Vector2(3, -2), "%s" % [pans])
	_verifier("magnify(factor) relayé", magnifies.n == 1 and is_equal_approx(magnifies.args[0], 1.25), "%s" % [magnifies])
	mobile.handle_input_event(InputEventKey.new())
	mobile.handle_input_event(null)
	_verifier("Évènement non tactile ou nul ignoré", pans.n == 1 and mobile.get_finger_count() == 0)


func _tester_passage_par_input() -> void:
	var mobile := _mobile()
	await _frames(1) # _ready active le traitement des entrées
	var appuis := _espionner(mobile.touch_pressed)
	var evenement := InputEventScreenTouch.new()
	evenement.index = 6
	evenement.position = Vector2(30, 40)
	evenement.pressed = true
	Input.parse_input_event(evenement)
	Input.flush_buffered_events()
	_verifier("Évènement reçu par _input", appuis.n == 1 and mobile.has_finger(6), "%s" % [appuis])
	var relache := evenement.duplicate()
	relache.pressed = false
	Input.parse_input_event(relache)
	Input.flush_buffered_events()
	_verifier("Relâchement reçu par _input", not mobile.has_finger(6))


func _tester_reinitialisation() -> void:
	var mobile := _mobile()
	var fins := _espionner(mobile.pinch_ended)
	var nombre := _espionner(mobile.multi_touch_changed)
	_appuyer(mobile, 0, Vector2(0, 0))
	_appuyer(mobile, 1, Vector2(100, 0))
	mobile.reset_touches()
	_verifier("reset_touches : doigts et pincement oubliés", mobile.get_finger_count() == 0 and not mobile.is_pinching()
		and fins.n == 1 and nombre.args[0] == 0, "%d doigts, %s" % [mobile.get_finger_count(), nombre])


func _tester_capteurs() -> void:
	var mobile := _mobile()
	mobile.use_gravity = true
	mobile.use_accelerometer = true
	mobile.use_gyroscope = true
	var mises_a_jour := _espionner(mobile.gravity_updated)
	var changements := _espionner(mobile.gravity_changed)
	var changements_accel := _espionner(mobile.accelerometer_changed)

	Input.set_gravity(Vector3(0, -9.8, 0))
	Input.set_accelerometer(Vector3(0.5, 0, 0)) # × 0.1 = 0.05 < zone morte 0.15
	Input.set_gyroscope(Vector3(0, 0.3, 0.4)) # longueur 0.5
	await _frames(3)
	_verifier("Gravité × sensibilité 0.1", mobile.get_gravity().is_equal_approx(Vector3(0, -0.98, 0)), "%s" % mobile.get_gravity())
	_verifier("gravity_updated à chaque image", mises_a_jour.n >= 3, "%d émissions" % mises_a_jour.n)
	_verifier("gravity_changed une seule fois pour une valeur stable", changements.n == 1, "%d émissions" % changements.n)
	_verifier("Zone morte sphérique : petite accélération ramenée à zéro", mobile.get_accelerometer() == Vector3.ZERO and changements_accel.n == 0)
	_verifier("get_sensor_value(SENSOR_GYROSCOPE)", mobile.get_sensor_value(MobileInput.SENSOR_GYROSCOPE).is_equal_approx(Vector3(0, 0.3, 0.4)))

	mobile.gravity_sensitivity = 0.2
	await _frames(2)
	_verifier("Changement de sensibilité pris en compte", mobile.get_gravity().is_equal_approx(Vector3(0, -1.96, 0)) and changements.n == 2,
		"%s, %d changements" % [mobile.get_gravity(), changements.n])

	Input.set_gravity(Vector3.ZERO)
	Input.set_accelerometer(Vector3.ZERO)
	Input.set_gyroscope(Vector3.ZERO)
	await _frames(2)
	_verifier("Retour à zéro signalé", mobile.get_gravity() == Vector3.ZERO and changements.n == 3)


func _tester_capteur_desactive() -> void:
	var mobile := _mobile()
	mobile.use_magnetometer = true
	Input.set_magnetometer(Vector3(40, 0, 0))
	await _frames(2)
	_verifier("Magnétomètre lu (40 µT × 0.02)", mobile.get_magnetometer().is_equal_approx(Vector3(0.8, 0, 0)), "%s" % mobile.get_magnetometer())
	var mises_a_jour := _espionner(mobile.magnetometer_updated)
	mobile.use_magnetometer = false
	await _frames(2)
	_verifier("Capteur désactivé : valeur remise à zéro, plus de signal", mobile.get_magnetometer() == Vector3.ZERO and mises_a_jour.n == 0)
	Input.set_magnetometer(Vector3.ZERO)
