extends "../suite_tests_base.gd"

## Suite de tests automatiques du module caméras : CameraManager (2D et 3D), CameraPanner, OrbitalCamera.
##
## Chaque test travaille dans son propre SubViewport : la caméra courante de la démo n'est pas touchée.
## Erreurs attendues dans la console : caméra nulle, caméra hors de l'arbre, OrbitalCamera sans bras.

var _gestionnaire: CameraManager


func _executer() -> void:
	await _tester_singleton()
	if _gestionnaire == null:
		return
	await _tester_3d_instantane()
	await _tester_3d_transition()
	await _tester_3d_duree_par_defaut()
	await _tester_3d_interruption()
	await _tester_3d_meme_camera()
	await _tester_3d_entrees_invalides()
	await _tester_3d_transitoire_recreee()
	await _tester_2d_instantane()
	await _tester_2d_transition()
	await _tester_panner()
	await _tester_orbitale()


# 🛠️ Singleton (ManagerBuilder)

func _tester_singleton() -> void:
	_gestionnaire = CameraManager.get_instance()
	_verifier("CameraManager : get_instance renvoie une instance", _gestionnaire != null,
		"nul : le module tool_editor (ManagerBuilder) est-il chargé ?")
	if _gestionnaire == null:
		return
	_verifier("CameraManager : get_instance renvoie toujours la même instance", CameraManager.get_instance() == _gestionnaire)

	await _frames() # ajout différé sous la racine
	_verifier("CameraManager : singleton ajouté sous la racine du SceneTree",
		_gestionnaire.is_inside_tree() and _gestionnaire.get_parent() == _gestionnaire.get_tree().root,
		str(_gestionnaire.get_parent()))

	var doublon := CameraManager.new()
	_ajouter(doublon)
	_verifier("CameraManager : un second CameraManager est détruit (doublon)", doublon.is_queued_for_deletion())
	_verifier("CameraManager : le singleton reste le premier", CameraManager.get_instance() == _gestionnaire)


# 🎥 3D

func _tester_3d_instantane() -> void:
	var vp := _creer_viewport()
	var a := _camera_3d(vp, Vector3.ZERO)
	var b := _camera_3d(vp, Vector3(10, 0, 0))
	a.current = true
	var espion := _espionner(_gestionnaire.current_camera_3d_switched)

	_gestionnaire.switch_3d_camera(b, false)
	_verifier("3D instantané : la cible devient la caméra courante", vp.get_camera_3d() == b)
	_verifier("3D instantané : signal émis une fois, avec la cible", espion.n == 1 and espion.args[0] == b, "émissions : %d" % espion.n)
	_verifier("3D instantané : aucune transition en cours", not _gestionnaire.is_transitioning_3d())
	await _liberer(vp)


func _tester_3d_transition() -> void:
	var vp := _creer_viewport()
	var a := _camera_3d(vp, Vector3.ZERO, 75.0)
	var b := _camera_3d(vp, Vector3(10, 0, 0), 40.0)
	a.current = true
	var espion := _espionner(_gestionnaire.current_camera_3d_switched)

	_gestionnaire.switch_3d_camera(b, true, 0.3)
	var transitoire := vp.get_camera_3d()
	_verifier("3D transition : en cours", _gestionnaire.is_transitioning_3d())
	_verifier("3D transition : une caméra transitoire est courante", transitoire != null and transitoire != a and transitoire != b)
	_verifier("3D transition : elle part de la caméra précédente",
		transitoire.global_position.is_equal_approx(a.global_position) and is_equal_approx(transitoire.fov, 75.0))
	_verifier("3D transition : pas de signal avant la fin", espion.n == 0)

	await _secondes(0.15)
	_verifier("3D transition : à mi-parcours, position et fov intermédiaires",
		transitoire.global_position.x > 0.5 and transitoire.global_position.x < 9.5 and transitoire.fov < 74.0 and transitoire.fov > 41.0,
		"x = %.2f, fov = %.1f" % [transitoire.global_position.x, transitoire.fov])

	await _secondes(0.3)
	_verifier("3D transition : à la fin, la cible est courante", vp.get_camera_3d() == b and not _gestionnaire.is_transitioning_3d())
	_verifier("3D transition : signal émis une fois, avec la cible", espion.n == 1 and espion.args[0] == b, "émissions : %d" % espion.n)
	_verifier("3D transition : la caméra transitoire n'est plus courante", not transitoire.current)
	_verifier("3D transition : la caméra transitoire est un enfant interne (pas dans get_children)",
		transitoire.get_parent() == b and not b.get_children().has(transitoire))
	await _liberer(vp)


func _tester_3d_duree_par_defaut() -> void:
	var vp := _creer_viewport()
	var a := _camera_3d(vp, Vector3.ZERO)
	var b := _camera_3d(vp, Vector3(5, 0, 0))
	a.current = true
	var duree_initiale := _gestionnaire.transition_duration_3d
	_gestionnaire.transition_duration_3d = 0.1

	_gestionnaire.switch_3d_camera(b) # duration = -1 → transition_duration_3d
	await _secondes(0.3)
	_verifier("3D : durée -1 → transition_duration_3d utilisée", vp.get_camera_3d() == b and not _gestionnaire.is_transitioning_3d())

	_gestionnaire.transition_duration_3d = duree_initiale
	await _liberer(vp)


func _tester_3d_interruption() -> void:
	var vp := _creer_viewport()
	var a := _camera_3d(vp, Vector3.ZERO)
	var b := _camera_3d(vp, Vector3(10, 0, 0))
	var c := _camera_3d(vp, Vector3(0, 10, 0))
	a.current = true
	var espion := _espionner(_gestionnaire.current_camera_3d_switched)

	_gestionnaire.switch_3d_camera(b, true, 0.4)
	await _secondes(0.1)
	var transitoire := vp.get_camera_3d()
	var position_interrompue := transitoire.global_position
	_gestionnaire.switch_3d_camera(c, true, 0.2)
	_verifier("3D interruption : la transitoire repart de la position atteinte",
		transitoire.global_position.is_equal_approx(position_interrompue), "%s ≠ %s" % [transitoire.global_position, position_interrompue])

	await _secondes(0.4)
	_verifier("3D interruption : la nouvelle cible est courante", vp.get_camera_3d() == c)
	_verifier("3D interruption : un seul signal, pour la nouvelle cible", espion.n == 1 and espion.args[0] == c, "émissions : %d" % espion.n)
	await _liberer(vp)


func _tester_3d_meme_camera() -> void:
	var vp := _creer_viewport()
	var a := _camera_3d(vp, Vector3.ZERO)
	a.current = true
	var espion := _espionner(_gestionnaire.current_camera_3d_switched)

	_gestionnaire.switch_3d_camera(a, true, 0.5)
	_verifier("3D même caméra : signal immédiat, sans transition", espion.n == 1 and not _gestionnaire.is_transitioning_3d())
	await _liberer(vp)


func _tester_3d_entrees_invalides() -> void:
	_gestionnaire.switch_3d_camera(null)
	_verifier("3D : caméra nulle refusée sans plantage", not _gestionnaire.is_transitioning_3d())

	var hors_arbre := Camera3D.new()
	_gestionnaire.switch_3d_camera(hors_arbre)
	_verifier("3D : caméra hors de l'arbre refusée", not _gestionnaire.is_transitioning_3d())
	hors_arbre.free()

	_verifier("get_current_camera_3d(null) → null", CameraManager.get_current_camera_3d(null) == null)
	await _frames()


func _tester_3d_transitoire_recreee() -> void:
	# La transitoire est rattachée à la caméra cible : libérer la cible la libère aussi.
	var vp := _creer_viewport()
	var a := _camera_3d(vp, Vector3.ZERO)
	var b := _camera_3d(vp, Vector3(3, 0, 0))
	a.current = true
	_gestionnaire.switch_3d_camera(b, true, 0.05)
	await _secondes(0.15)
	var ancienne_transitoire := b.get_child(0, true)
	await _liberer(vp)
	_verifier("3D : la transitoire est libérée avec sa caméra cible", not is_instance_valid(ancienne_transitoire))

	var vp2 := _creer_viewport()
	var c := _camera_3d(vp2, Vector3.ZERO)
	var d := _camera_3d(vp2, Vector3(3, 0, 0))
	c.current = true
	_gestionnaire.switch_3d_camera(d, true, 0.05)
	var transitoire := vp2.get_camera_3d()
	_verifier("3D : une nouvelle transitoire est recréée ensuite", transitoire != null and transitoire != c and transitoire != d)
	await _secondes(0.15)
	_verifier("3D : ... et la transition aboutit", vp2.get_camera_3d() == d)
	await _liberer(vp2)


# 🎥 2D

func _tester_2d_instantane() -> void:
	var vp := _creer_viewport()
	var a := _camera_2d(vp, Vector2.ZERO)
	var b := _camera_2d(vp, Vector2(500, 0))
	await _frames()
	var espion := _espionner(_gestionnaire.current_camera_2d_switched)

	_gestionnaire.switch_2d_camera(b, false)
	_verifier("2D instantané : la cible devient la caméra courante", vp.get_camera_2d() == b)
	_verifier("2D instantané : les autres caméras sont désactivées", not a.enabled and b.enabled)
	_verifier("2D instantané : signal émis une fois, avec la cible", espion.n == 1 and espion.args[0] == b)
	_verifier("get_current_camera_2d(viewport)", CameraManager.get_current_camera_2d(vp) == b)
	_verifier("get_current_camera_2d(null) → null", CameraManager.get_current_camera_2d(null) == null)
	await _liberer(vp)


func _tester_2d_transition() -> void:
	var vp := _creer_viewport()
	var a := _camera_2d(vp, Vector2.ZERO)
	var b := _camera_2d(vp, Vector2(500, 0))
	b.zoom = Vector2(2, 2)
	b.offset = Vector2(0, 40)
	await _frames()
	_gestionnaire.switch_2d_camera(a, false)
	var espion := _espionner(_gestionnaire.current_camera_2d_switched)

	_gestionnaire.switch_2d_camera(b, true, 0.3)
	var transitoire := vp.get_camera_2d()
	_verifier("2D transition : une caméra transitoire est courante", transitoire != null and transitoire != a and transitoire != b)

	await _secondes(0.15)
	_verifier("2D transition : position, zoom et offset intermédiaires",
		transitoire.global_position.x > 10.0 and transitoire.global_position.x < 490.0 and transitoire.zoom.x > 1.05 and transitoire.zoom.x < 1.95 and transitoire.offset.y > 1.0,
		"x = %.1f, zoom = %.2f, offset = %s" % [transitoire.global_position.x, transitoire.zoom.x, transitoire.offset])

	await _secondes(0.3)
	_verifier("2D transition : à la fin, la cible est courante", vp.get_camera_2d() == b and not _gestionnaire.is_transitioning_2d())
	_verifier("2D transition : la transitoire est désactivée", not transitoire.enabled)
	_verifier("2D transition : signal émis une fois, avec la cible", espion.n == 1 and espion.args[0] == b, "émissions : %d" % espion.n)
	await _liberer(vp)


# 🖐️ CameraPanner

func _tester_panner() -> void:
	var vp := _creer_viewport() # 400 × 300 : centre en (200, 150)
	var panner := CameraPanner.new()
	_ajouter(panner, vp)
	await _frames()

	panner.teleport(Vector2(100, 50), 1.0)
	_verifier("CameraPanner : teleport place position et zoom", panner.position == Vector2(100, 50) and panner.zoom == Vector2.ONE)
	panner.teleport(Vector2.ZERO, 999.0)
	_verifier("CameraPanner : teleport borne le zoom à max_zoom", is_equal_approx(panner.zoom.x, panner.max_zoom))

	# Molette au centre : zoom × zoom_factor, position inchangée
	panner.teleport(Vector2.ZERO, 1.0)
	vp.push_input(_molette(MOUSE_BUTTON_WHEEL_UP, Vector2(200, 150)))
	await _secondes(0.5)
	_verifier("CameraPanner : molette vers le haut → zoom × zoom_factor", absf(panner.zoom.x - panner.zoom_factor) < 0.01, "zoom = %.3f" % panner.zoom.x)
	_verifier("CameraPanner : zoom au centre → position inchangée", panner.position.length() < 0.5, str(panner.position))

	# Molette décentrée : le point du monde sous le curseur reste fixe
	panner.teleport(Vector2.ZERO, 1.0)
	vp.push_input(_molette(MOUSE_BUTTON_WHEEL_DOWN, Vector2(300, 150)))
	await _secondes(0.5)
	var point_sous_curseur := panner.position.x + 100.0 / panner.zoom.x
	_verifier("CameraPanner : zoom vers le curseur (le point visé reste fixe)", absf(point_sous_curseur - 100.0) < 1.0, "point = %.2f" % point_sous_curseur)

	# Bornes
	panner.teleport(Vector2.ZERO, panner.max_zoom)
	vp.push_input(_molette(MOUSE_BUTTON_WHEEL_UP, Vector2(200, 150)))
	await _secondes(0.3)
	_verifier("CameraPanner : zoom borné à max_zoom", panner.zoom.x <= panner.max_zoom + 0.001)

	# Glissement à deux doigts
	panner.teleport(Vector2.ZERO, 1.0)
	var glissement := InputEventPanGesture.new()
	glissement.delta = Vector2(1, 0)
	vp.push_input(glissement)
	await _secondes(0.5)
	_verifier("CameraPanner : glissement à deux doigts → déplacement (15 px × sensibilité / zoom)",
		absf(panner.position.x - 15.0 * panner.touch_pan_sensitivity) < 0.5, str(panner.position))

	# Pincement
	panner.teleport(Vector2.ZERO, 1.0)
	var pincement := InputEventMagnifyGesture.new()
	pincement.factor = 1.5
	pincement.position = Vector2(200, 150)
	vp.push_input(pincement)
	await _secondes(0.5)
	_verifier("CameraPanner : pincement → zoom × facteur", absf(panner.zoom.x - 1.5) < 0.01, "zoom = %.3f" % panner.zoom.x)

	# Actions d'entrée configurables (action dédiée pour ne pas déplacer les caméras de la démo)
	panner.teleport(Vector2.ZERO, 1.0)
	InputMap.add_action(&"test_panner_droite")
	panner.action_pan_right = &"test_panner_droite"
	Input.action_press(&"test_panner_droite")
	await _secondes(0.3)
	Input.action_release(&"test_panner_droite")
	_verifier("CameraPanner : action_pan_right personnalisée → déplacement vers la droite", panner.position.x > 20.0, str(panner.position))
	InputMap.erase_action(&"test_panner_droite")
	panner.action_pan_right = &"ui_right"

	# Panoramique au bouton de souris maintenu (dépend de l'état réel de la souris)
	panner.teleport(Vector2.ZERO, 1.0)
	var appui := InputEventMouseButton.new()
	appui.button_index = panner.pan_mouse_button
	appui.pressed = true
	Input.parse_input_event(appui)
	await _frames(2)
	var mouvement := InputEventMouseMotion.new()
	mouvement.relative = Vector2(-50, 0)
	vp.push_input(mouvement)
	var relache := appui.duplicate()
	relache.pressed = false
	Input.parse_input_event(relache)
	await _secondes(0.5)
	_avertir_si("CameraPanner : bouton maintenu + mouvement → panoramique", absf(panner.position.x - 50.0) < 1.0,
		"position = %s (dépend de l'état réel de la souris : ne pas toucher la souris pendant les tests)" % panner.position)

	await _liberer(vp)


# 🛰️ OrbitalCamera

func _tester_orbitale() -> void:
	var sans_bras := OrbitalCamera.new()
	_ajouter(sans_bras) # erreur attendue : arm_3d n'est pas assigné
	sans_bras.look_around(Vector2(10, 10))
	await _frames()
	_verifier("OrbitalCamera : sans bras, aucun plantage (look_around et _process)", sans_bras.arm_3d == null)
	sans_bras.queue_free()

	var orbitale := OrbitalCamera.new()
	var bras := SpringArm3D.new()
	bras.rotation = Vector3(0.1, 0.2, 0.0)
	orbitale.add_child(bras)
	orbitale.arm_3d = bras
	_ajouter(orbitale)
	await _frames(3)
	_verifier("OrbitalCamera : la cible part de la rotation du bras (pas de saut au démarrage)",
		bras.rotation.is_equal_approx(Vector3(0.1, 0.2, 0.0)), str(bras.rotation))

	orbitale.use_smoothing = false
	orbitale.look_around(Vector2(100, 0))
	_verifier("OrbitalCamera : sans lissage, rotation appliquée immédiatement (lacet)",
		is_equal_approx(bras.rotation.y, 0.2 - 100.0 * orbitale.sensitivity), "y = %.4f" % bras.rotation.y)

	orbitale.look_around(Vector2(0, 1e6))
	_verifier("OrbitalCamera : tangage borné à min_pitch", is_equal_approx(bras.rotation.x, deg_to_rad(orbitale.min_pitch)), "x = %.4f" % bras.rotation.x)
	orbitale.look_around(Vector2(0, -1e7))
	_verifier("OrbitalCamera : tangage borné à max_pitch", is_equal_approx(bras.rotation.x, deg_to_rad(orbitale.max_pitch)), "x = %.4f" % bras.rotation.x)

	orbitale.use_smoothing = true
	var avant := bras.rotation.y
	var cible := avant + 200.0 * orbitale.sensitivity
	orbitale.look_around(Vector2(-200, 0))
	_verifier("OrbitalCamera : avec lissage, pas d'application immédiate", is_equal_approx(bras.rotation.y, avant))
	await _frames(2)
	_verifier("OrbitalCamera : avec lissage, progresse vers la cible", bras.rotation.y > avant + 0.0001, "y = %.4f" % bras.rotation.y)
	await _secondes(0.6)
	_verifier("OrbitalCamera : avec lissage, atteint la cible", absf(bras.rotation.y - cible) < 0.01, "y = %.4f / %.4f" % [bras.rotation.y, cible])

	bras.free()
	await _frames()
	_verifier("OrbitalCamera : bras libéré → arm_3d nul, sans plantage", orbitale.arm_3d == null)
	orbitale.look_around(Vector2.ONE)


# 🛠️ Utilitaires

func _creer_viewport() -> SubViewport:
	var vp := SubViewport.new()
	vp.size = Vector2i(400, 300)
	vp.own_world_3d = true
	_ajouter(vp)
	return vp


func _liberer(noeud: Node) -> void:
	noeud.queue_free()
	await _frames()


func _camera_3d(vp: SubViewport, position: Vector3, fov: float = 75.0) -> Camera3D:
	var camera := Camera3D.new()
	camera.position = position
	camera.fov = fov
	_ajouter(camera, vp)
	return camera


func _camera_2d(vp: SubViewport, position: Vector2) -> Camera2D:
	var camera := Camera2D.new()
	camera.position = position
	_ajouter(camera, vp)
	return camera


func _molette(bouton: MouseButton, position: Vector2) -> InputEventMouseButton:
	var evenement := InputEventMouseButton.new()
	evenement.button_index = bouton
	evenement.pressed = true
	evenement.position = position
	return evenement
