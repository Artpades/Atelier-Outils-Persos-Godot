extends "res://🧪Démos de modules/2.🧭🌍/suite_tests_base.gd"

## 🧪 Tests du module Entrées de périphérique : actions (bouton, axe, cercle, sphère, souris, gyroscope,
## action GDScript), liaisons, contextes (priorité, exclusivité, pile), copies de jeu, simulation, sauvegarde
## des contrôles, InputRebinder, PlayerInputUI, détection du périphérique et pilotage d'un déplacement.
##
## Les entrées sont simulées avec Input.parse_input_event. La manette simulée est le périphérique 7 : une
## vraie manette branchée (périphérique 0, avec sa dérive) ne perturbe pas les tests.

const MANETTE := 7
const FICHIER_CONTROLES := "user://tests_controles_entrees.cfg"


func _executer() -> void:
	await _tester_bouton()
	await _tester_bouton_manette()
	await _tester_axe()
	await _tester_cercle()
	await _tester_sphere()
	await _tester_souris()
	await _tester_gyroscope_et_script()
	await _tester_liaisons()
	await _tester_contextes()
	await _tester_copies()
	await _tester_sauvegarde()
	await _tester_rebinder()
	await _tester_detection_peripherique()
	await _tester_interface()
	await _tester_avertissements()
	await _tester_deplacement()
	_relacher_tout()


# 🛠️ Simulation des périphériques

func _touche(code: Key, appui: bool) -> void:
	var evenement := InputEventKey.new()
	evenement.keycode = code
	evenement.physical_keycode = code
	evenement.pressed = appui
	Input.parse_input_event(evenement)
	Input.flush_buffered_events()


func _bouton_manette(bouton: JoyButton, appui: bool) -> void:
	var evenement := InputEventJoypadButton.new()
	evenement.device = MANETTE
	evenement.button_index = bouton
	evenement.pressed = appui
	Input.parse_input_event(evenement)
	Input.flush_buffered_events()


func _axe_manette(axe: JoyAxis, valeur: float) -> void:
	var evenement := InputEventJoypadMotion.new()
	evenement.device = MANETTE
	evenement.axis = axe
	evenement.axis_value = valeur
	Input.parse_input_event(evenement)
	Input.flush_buffered_events()


func _relacher_tout() -> void:
	for code in [KEY_SPACE, KEY_W, KEY_A, KEY_S, KEY_D, KEY_E, KEY_Q, KEY_F, KEY_G, KEY_RIGHT, KEY_LEFT, KEY_ESCAPE, KEY_SHIFT]:
		_touche(code, false)
	for bouton in [JOY_BUTTON_A, JOY_BUTTON_B, JOY_BUTTON_DPAD_UP, JOY_BUTTON_DPAD_RIGHT]:
		_bouton_manette(bouton, false)
	for axe in [JOY_AXIS_LEFT_X, JOY_AXIS_LEFT_Y, JOY_AXIS_TRIGGER_RIGHT]:
		_axe_manette(axe, 0.0)


# 🧱 Construction

func _bouton(nom: String, touche: Key = KEY_NONE) -> InputButtonAction:
	var action := InputButtonAction.new()
	action.action_name = nom
	action.key = touche
	return action


func _contexte(nom: String, actions: Array, priorite := 0, exclusif := false) -> InputContext:
	var contexte := InputContext.new()
	contexte.context_name = nom
	contexte.priority = priorite
	contexte.exclusive = exclusif
	contexte.actions = actions
	return contexte


func _joueur(contextes: Array) -> PlayerInput:
	var joueur := PlayerInput.new()
	joueur.name = "Joueur"
	joueur.device = MANETTE
	joueur.contexts = contextes
	_ajouter(joueur)
	return joueur


# 🔘 Bouton

func _tester_bouton() -> void:
	var saut := _bouton("saut", KEY_SPACE)
	saut.held_delay = 0.1
	var joueur := _joueur([_contexte("jeu", [saut])])
	await _frames(1)
	var action: InputButtonAction = joueur.get_action("saut")
	_verifier("get_action renvoie la copie de jeu (pas la ressource d'origine)", action != null and action != saut and action.action_name == "saut")
	var appuis := _espionner(action.pressed)
	var relaches := _espionner(action.released)
	var bascules := _espionner(action.toggled)
	var maintiens := _espionner(action.held)
	var multiples := _espionner(action.multi_pressed)
	_touche(KEY_SPACE, true)
	await _frames(1)
	_verifier("Bouton : pressed et toggled(true)", appuis.n == 1 and bascules.n == 1 and bascules.args[0] == true and action.is_pressed())
	_verifier("Bouton : is_just_pressed pendant une image", joueur.is_just_pressed("saut") and joueur.is_pressed("saut"))
	await _frames(1)
	_verifier("Bouton : is_just_pressed retombe", not joueur.is_just_pressed("saut") and action.is_pressed())
	await _secondes(0.2)
	_verifier("Bouton : held après held_delay", maintiens.n == 1 and action.get_held_time() >= 0.1)
	await _frames(1) # réaligne sur le début d'une image (un minuteur reprend en cours d'image)
	_touche(KEY_SPACE, false)
	await _frames(1)
	_verifier("Bouton : released et toggled(false)", relaches.n == 1 and bascules.args[0] == false and joueur.is_just_released("saut") and not action.is_pressed(), "%s %s %s %s" % [relaches, bascules, joueur.is_just_released("saut"), action.is_pressed()])
	_touche(KEY_SPACE, true)
	await _frames(1)
	_touche(KEY_SPACE, false)
	await _frames(1)
	_touche(KEY_SPACE, true)
	await _frames(1)
	_verifier("Bouton : multi_pressed(2) sur double appui rapide", multiples.n >= 1 and multiples.args[0] == 2, str(multiples))
	_touche(KEY_SPACE, false)
	await _frames(1)
	_verifier("Bouton : valeur bool et texte", action.get_value() == false and action.get_value_text() != "")
	var souris := _bouton("tir")
	souris.mouse_button = MOUSE_BUTTON_LEFT
	_verifier("Textes des entrées", InputAction.key_text(KEY_SPACE) == "Space" and InputAction.joy_button_text(JOY_BUTTON_A).begins_with("A") and souris.get_slot_text("key") == "Clic gauche")


func _tester_bouton_manette() -> void:
	var tir := _bouton("tir", KEY_F)
	tir.joy_button = JOY_BUTTON_B
	var gachette := _bouton("gachette")
	gachette.joy_axis = JOY_AXIS_TRIGGER_RIGHT
	var joueur := _joueur([_contexte("jeu", [tir, gachette])])
	await _frames(1)
	_bouton_manette(JOY_BUTTON_B, true)
	await _frames(1)
	_verifier("Bouton de manette (périphérique du PlayerInput)", joueur.is_pressed("tir"))
	_bouton_manette(JOY_BUTTON_B, false)
	_axe_manette(JOY_AXIS_TRIGGER_RIGHT, 0.1)
	await _frames(1)
	_verifier("Gâchette sous la zone morte : relâchée", not joueur.is_pressed("tir") and not joueur.is_pressed("gachette"))
	_axe_manette(JOY_AXIS_TRIGGER_RIGHT, 0.9)
	await _frames(1)
	_verifier("Gâchette enfoncée (axe → bouton)", joueur.is_pressed("gachette"))
	_axe_manette(JOY_AXIS_TRIGGER_RIGHT, 0.0)
	joueur.device_filter = InputAction.DEVICE_KEYBOARD_MOUSE
	_bouton_manette(JOY_BUTTON_B, true)
	await _frames(1)
	_verifier("Filtre clavier-souris : manette ignorée", not joueur.is_pressed("tir"))
	_touche(KEY_F, true)
	await _frames(1)
	_verifier("Filtre clavier-souris : clavier lu", joueur.is_pressed("tir"))
	_touche(KEY_F, false)
	joueur.device_filter = InputAction.DEVICE_GAMEPAD
	_touche(KEY_F, true)
	_bouton_manette(JOY_BUTTON_B, false)
	await _frames(1)
	_verifier("Filtre manette : clavier ignoré", not joueur.is_pressed("tir"))
	joueur.get_action("tir").device_filter = InputAction.DEVICE_ALL
	await _frames(1)
	_verifier("Filtre de l'action (Tous) prioritaire sur celui du PlayerInput", joueur.is_pressed("tir"))
	_touche(KEY_F, false)
	await _frames(1)


# ↔️ Axe, 🕹️ cercle, 🌐 sphère

func _tester_axe() -> void:
	var axe := InputAxisAction.new()
	axe.action_name = "gaz"
	axe.positive_key = KEY_D
	axe.negative_key = KEY_A
	axe.joy_axis = JOY_AXIS_LEFT_X
	axe.dead_zone = 0.2
	var joueur := _joueur([_contexte("jeu", [axe])])
	await _frames(1)
	var action: InputAxisAction = joueur.get_action("gaz")
	var valeurs := _espionner(action.value_changed)
	_touche(KEY_D, true)
	await _frames(1)
	_verifier("Axe : touche + → 1", is_equal_approx(joueur.get_axis("gaz"), 1.0) and valeurs.n == 1 and valeurs.args[0] == 1.0)
	_touche(KEY_A, true)
	await _frames(1)
	_verifier("Axe : + et − s'annulent → 0 (émis)", joueur.get_axis("gaz") == 0.0 and valeurs.args[0] == 0.0)
	_touche(KEY_D, false)
	await _frames(1)
	_verifier("Axe : touche − → -1", is_equal_approx(joueur.get_axis("gaz"), -1.0))
	_touche(KEY_A, false)
	_axe_manette(JOY_AXIS_LEFT_X, 0.6)
	await _frames(1)
	_verifier("Axe de manette : zone morte remise à l'échelle (0.6 → 0.5)", is_equal_approx(action.get_axis(), 0.5), str(action.get_axis()))
	action.invert = true
	await _frames(1)
	_verifier("Axe inversé", is_equal_approx(action.get_axis(), -0.5))
	_axe_manette(JOY_AXIS_LEFT_X, 0.0)
	await _frames(1)
	_verifier("Axe : retour exact à 0", action.get_axis() == 0.0 and valeurs.args[0] == 0.0)


func _tester_cercle() -> void:
	var deplacement := InputCircleAction.new()
	deplacement.action_name = "deplacement"
	_verifier("Cercle : préréglages par défaut (WASD, stick gauche, croix)", deplacement.key_preset == InputCircleAction.KEYS_WASD and deplacement.joy_preset == InputCircleAction.JOY_LEFT_STICK and deplacement.dpad_preset == InputCircleAction.DPAD_CROSS)
	deplacement.key_preset = InputCircleAction.KEYS_ZQSD
	_verifier("Cercle : préréglage ZQSD appliqué aux touches", deplacement.up_key == KEY_Z and deplacement.left_key == KEY_Q)
	deplacement.up_key = KEY_F
	_verifier("Cercle : touche modifiée → préréglage Personnalisé", deplacement.key_preset == InputCircleAction.KEYS_CUSTOM)
	deplacement.key_preset = InputCircleAction.KEYS_WASD
	var joueur := _joueur([_contexte("jeu", [deplacement])])
	await _frames(1)
	var action: InputCircleAction = joueur.get_action("deplacement")
	var valeurs := _espionner(action.value_changed)
	var orientations := _espionner(action.orientation_changed)
	_touche(KEY_D, true)
	await _frames(1)
	_verifier("Cercle : D → (1, 0)", joueur.get_vector("deplacement") == Vector2(1, 0) and valeurs.n == 1)
	_touche(KEY_W, true)
	await _frames(1)
	var diagonale: Vector2 = joueur.get_vector("deplacement")
	_verifier("Cercle : diagonale normalisée (y vers le bas)", diagonale.is_equal_approx(Vector2(1, -1).normalized()) and is_equal_approx(orientations.args[0], -PI / 4), str(diagonale))
	_touche(KEY_D, false)
	_touche(KEY_W, false)
	_axe_manette(JOY_AXIS_LEFT_X, -1.0)
	await _frames(1)
	_verifier("Cercle : stick gauche", action.get_vector().is_equal_approx(Vector2(-1, 0)), str(action.get_vector()))
	_axe_manette(JOY_AXIS_LEFT_X, 0.1)
	_bouton_manette(JOY_BUTTON_DPAD_UP, true)
	await _frames(1)
	_verifier("Cercle : stick dans la zone morte → croix directionnelle", action.get_vector() == Vector2(0, -1))
	_bouton_manette(JOY_BUTTON_DPAD_UP, false)
	_axe_manette(JOY_AXIS_LEFT_X, 0.0)
	await _frames(1)
	_verifier("Cercle : retour à (0, 0) émis", action.get_vector() == Vector2.ZERO and valeurs.args[0] == Vector2.ZERO)


func _tester_sphere() -> void:
	var vol := InputSphereAction.new()
	vol.action_name = "vol"
	var joueur := _joueur([_contexte("jeu", [vol])])
	await _frames(1)
	var action: InputSphereAction = joueur.get_action("vol")
	_touche(KEY_W, true)
	await _frames(1)
	_verifier("Sphère : avant = -Z", action.get_vector() == Vector3(0, 0, -1) and joueur.get_vector("vol") == Vector2(0, -1))
	_touche(KEY_E, true)
	await _frames(1)
	_verifier("Sphère : haut + avant normalisés, tangage 45°", action.get_vector().is_equal_approx(Vector3(0, 1, -1).normalized()) and is_equal_approx(action.get_orientation().x, PI / 4))
	_touche(KEY_W, false)
	_touche(KEY_E, false)
	await _frames(1)
	_verifier("Sphère : repos", action.get_vector() == Vector3.ZERO)


# 🖱️ Souris, 🧭 gyroscope, 🧩 action GDScript

func _tester_souris() -> void:
	var souris := InputMouseAction.new()
	souris.action_name = "visee"
	souris.motion_sensitivity = 2.0
	var joueur := _joueur([_contexte("jeu", [souris])])
	await _frames(1)
	var action: InputMouseAction = joueur.get_action("visee")
	var mouvements := _espionner(action.motion)
	var molettes := _espionner(action.scrolled)
	var mouvement := InputEventMouseMotion.new()
	mouvement.relative = Vector2(5, 3)
	joueur.handle_input_event(mouvement)
	joueur.handle_input_event(mouvement)
	var molette := InputEventMouseButton.new()
	molette.button_index = MOUSE_BUTTON_WHEEL_UP
	molette.pressed = true
	joueur.handle_input_event(molette)
	await _frames(1)
	_verifier("Souris : mouvement cumulé × sensibilité", mouvements.n == 1 and mouvements.args[0] == Vector2(20, 12), str(mouvements.args))
	_verifier("Souris : molette", molettes.n == 1 and molettes.args[0] == Vector2(0, -1))
	await _frames(1)
	_verifier("Souris : (0, 0) émis à l'arrêt, une seule fois", mouvements.n == 2 and mouvements.args[0] == Vector2.ZERO)
	await _frames(2)
	_verifier("Souris : rien ensuite", mouvements.n == 2)


func _tester_gyroscope_et_script() -> void:
	var gyro := InputGyroAction.new()
	gyro.action_name = "gyro"
	gyro.sensitivity = 2.0
	var script := GDScript.new()
	script.source_code = "extends InputAction\nsignal tic(numero: int)\nvar compte := 0\nfunc _update_action(_delta: float) -> void:\n\tcompte += 1\n\temit_event(&\"tic\", [compte])\nfunc _get_value() -> Variant:\n\treturn compte\n"
	script.reload()
	var personnalisee: InputAction = script.new()
	personnalisee.action_name = "perso"
	var joueur := _joueur([_contexte("jeu", [gyro, personnalisee])])
	await _frames(1)
	var action: InputGyroAction = joueur.get_action("gyro")
	var rotations := _espionner(action.gyroscope_changed)
	joueur.simulate("gyro", Vector3(0, 1, 0))
	await _frames(1)
	_verifier("Gyroscope (simulé) : gyroscope_changed × sensibilité", rotations.n == 1 and rotations.args[0] == Vector3(0, 2, 0))
	joueur.stop_simulation("gyro")
	await _frames(1)
	var copie: InputAction = joueur.get_action("perso")
	var tics := _espionner(copie.tic)
	await _frames(2)
	_verifier("Action GDScript : _update_action et emit_event", tics.n == 2 and copie.compte >= 3 and copie.get_events().any(func(e): return e.name == &"tic"))


# 🔗 Liaisons

func _tester_liaisons() -> void:
	var cible := Node2D.new()
	cible.name = "Cible"
	_ajouter(cible)
	var deplacement := InputCircleAction.new()
	deplacement.action_name = "deplacement"
	var vers_position := InputBinding.new()
	vers_position.event = &"value_changed"
	vers_position.target = NodePath("../Cible")
	vers_position.method = &"set_position"
	deplacement.bindings = [vers_position]
	var cacher := _bouton("cacher", KEY_G)
	var liaison_cacher := InputBinding.new()
	liaison_cacher.event = &"pressed"
	liaison_cacher.target = NodePath("../Cible")
	liaison_cacher.method = &"hide"
	var liaison_meta := InputBinding.new()
	liaison_meta.event = &"released"
	liaison_meta.target = NodePath("../Cible")
	liaison_meta.method = &"set_meta"
	liaison_meta.pass_value = false
	liaison_meta.arguments = ["relache", 5]
	var liaison_cassee := InputBinding.new()
	liaison_cassee.event = &"pressed"
	liaison_cassee.target = NodePath("../Absente")
	liaison_cassee.method = &"hide"
	cacher.bindings = [liaison_cacher, liaison_meta, liaison_cassee]
	var joueur := _joueur([_contexte("jeu", [deplacement, cacher])])
	var declenchements := _espionner(joueur.action_triggered)
	var echecs := _espionner(joueur.binding_failed)
	await _frames(1)
	_touche(KEY_D, true)
	await _frames(1)
	_verifier("Liaison value_changed → Cible.set_position(valeur)", cible.position == Vector2(1, 0))
	_verifier("Signal action_triggered(action, évènement, arguments)", declenchements.n >= 1 and declenchements.args[0] == &"deplacement")
	_touche(KEY_D, false)
	_touche(KEY_G, true)
	await _frames(1)
	_verifier("Liaison pressed → Cible.hide() (sans argument)", not cible.visible)
	_verifier("Liaison cassée : binding_failed, les autres continuent", echecs.n == 1 and echecs.args[0] == &"cacher")
	_touche(KEY_G, false)
	await _frames(1)
	_verifier("Liaison avec arguments fixes, sans la valeur", cible.get_meta("relache", 0) == 5)
	var journal: Array = joueur.get_event_log()
	_verifier("Journal : évènements et appels décrits", journal.size() >= 3 and journal.any(func(e): return e.calls.size() > 0))
	_verifier("Description d'une liaison", liaison_meta.get_description().contains("set_meta") and liaison_meta.build_call_arguments([true]) == ["relache", 5])

	# Essai à blanc (débogage) : liaisons décrites, pas appelées.
	cible.show()
	joueur.dispatch_bindings = false
	_touche(KEY_G, true)
	await _frames(1)
	_verifier("dispatch_bindings = false : aucun appel", cible.visible)
	_touche(KEY_G, false)
	await _frames(1)
	_verifier("La ressource d'origine n'a pas changé d'état", not cacher.is_pressed() and cacher.get_event_count() == 0)


# 🗂️ Contextes

func _tester_contextes() -> void:
	var saut_jeu := _bouton("valider", KEY_SPACE)
	var saut_menu := _bouton("valider", KEY_SPACE)
	var marche := _bouton("marche", KEY_W)
	var jeu := _contexte("jeu", [saut_jeu, marche], 0)
	var menu := _contexte("menu", [saut_menu], 10, true)
	menu.enabled = false
	var joueur := _joueur([jeu, menu])
	var changements := _espionner(joueur.context_changed)
	await _frames(1)
	_verifier("Contextes : seul « jeu » actif", joueur.get_active_contexts() == PackedStringArray(["jeu"]) and changements.n == 1)
	_touche(KEY_W, true)
	await _frames(1)
	var marche_jeu: InputButtonAction = joueur.get_context("jeu").get_action("marche")
	var relachements := _espionner(marche_jeu.released)
	joueur.set_context_enabled("menu", true)
	await _frames(1)
	_verifier("Contexte exclusif prioritaire : « jeu » bloqué", joueur.is_context_active("menu") and not joueur.is_context_active("jeu"))
	_verifier("Actions du contexte bloqué relâchées (released)", relachements.n == 1 and not marche_jeu.is_pressed())
	_verifier("get_action : contexte actif le plus prioritaire", joueur.get_action("valider") == joueur.get_context("menu").get_action("valider"))
	joueur.set_context_enabled("menu", false)
	await _frames(1)
	_verifier("Retour au jeu : la touche toujours enfoncée est relue", marche_jeu.is_pressed())
	_touche(KEY_W, false)
	joueur.get_context("menu").exclusive = false
	joueur.push_context("menu")
	await _frames(1)
	_verifier("push_context : empilé, prioritaire et bloquant", joueur.get_context_stack() == PackedStringArray(["menu"]) and joueur.is_context_active("menu") and not joueur.is_context_active("jeu"))
	_verifier("pop_context : rétablit l'état d'avant", joueur.pop_context() == &"menu" and not joueur.is_context_enabled("menu"))
	await _frames(1)
	_verifier("Après pop_context : « jeu » de nouveau actif", joueur.is_context_active("jeu"))
	joueur.enable_only("menu")
	await _frames(1)
	_verifier("enable_only", joueur.get_active_contexts() == PackedStringArray(["menu"]) and not joueur.is_context_enabled("jeu"))
	_verifier("Signal context_changed", changements.n >= 6, str(changements.n))


func _tester_copies() -> void:
	var commun := _contexte("jeu", [_bouton("saut", KEY_SPACE)])
	var un := _joueur([commun])
	var deux := _joueur([commun])
	await _frames(1)
	_verifier("Deux PlayerInput, mêmes ressources : copies distinctes", un.get_action("saut") != deux.get_action("saut"))
	un.simulate("saut", true)
	await _frames(1)
	_verifier("Simulation sur l'un seulement", un.is_pressed("saut") and not deux.is_pressed("saut") and not commun.get_action("saut").is_pressed())
	un.stop_all_simulations()
	await _frames(1)
	_verifier("stop_all_simulations : relâché", not un.is_pressed("saut"))
	var partage := _joueur([commun])
	partage.runtime_copies = false
	partage.setup()
	_verifier("runtime_copies = false : ressource d'origine utilisée", partage.get_action("saut") == commun.get_action("saut"))


# 💾 Contrôles

func _tester_sauvegarde() -> void:
	var saut := _bouton("saut", KEY_SPACE)
	var deplacement := InputCircleAction.new()
	deplacement.action_name = "deplacement"
	var joueur := _joueur([_contexte("jeu", [saut, deplacement])])
	await _frames(1)
	var etat: Dictionary = joueur.get_bindings_state()
	_verifier("get_bindings_state : « contexte/action » → entrées", etat.has("jeu/saut") and etat["jeu/saut"]["key"] == KEY_SPACE and etat.has("jeu/deplacement"))
	joueur.get_action("saut").key = KEY_F
	joueur.get_action("deplacement").up_key = KEY_E
	_verifier("save_bindings", joueur.save_bindings(FICHIER_CONTROLES) == OK)
	var changements := _espionner(joueur.bindings_changed)
	joueur.reset_bindings()
	_verifier("reset_bindings : contrôles d'origine", joueur.get_action("saut").key == KEY_SPACE and joueur.get_action("deplacement").up_key == KEY_W and changements.n == 1)
	_verifier("load_bindings", joueur.load_bindings(FICHIER_CONTROLES) == OK and joueur.get_action("saut").key == KEY_F and joueur.get_action("deplacement").up_key == KEY_E)
	_verifier("Ressources d'origine intactes", saut.key == KEY_SPACE and deplacement.up_key == KEY_W)
	_verifier("load_bindings : fichier absent → erreur", joueur.load_bindings("user://absent_entrees.cfg") != OK)
	DirAccess.remove_absolute(ProjectSettings.globalize_path(FICHIER_CONTROLES))


# 🔄 Rebinder

func _tester_rebinder() -> void:
	var saut := _bouton("saut", KEY_SPACE)
	saut.joy_button = JOY_BUTTON_A
	var tir := _bouton("tir", KEY_F)
	var axe := InputAxisAction.new()
	axe.action_name = "gaz"
	axe.positive_joy_button = JOY_BUTTON_B
	var joueur := _joueur([_contexte("jeu", [saut, tir, axe])])
	var rebinder := InputRebinder.new()
	rebinder.listening_timeout = 0.2
	_ajouter(rebinder)
	await _frames(1)
	var action: InputButtonAction = joueur.get_action("saut")
	var debuts := _espionner(rebinder.rebind_started)
	var fins := _espionner(rebinder.rebind_completed)
	var annulations := _espionner(rebinder.rebind_cancelled)
	rebinder.start_rebind(action, &"key")
	var mouvement := InputEventMouseMotion.new()
	_verifier("Rebinder : écoute ; un mouvement de souris ne convient pas", rebinder.is_listening() and debuts.n == 1 and not rebinder.submit_event(mouvement))
	var touche := InputEventKey.new()
	touche.physical_keycode = KEY_G
	touche.keycode = KEY_G
	touche.pressed = true
	_verifier("Rebinder : touche assignée", rebinder.submit_event(touche) and action.key == KEY_G and fins.n == 1 and not rebinder.is_listening())
	rebinder.start_rebind(action, &"key")
	var clic := InputEventMouseButton.new()
	clic.button_index = MOUSE_BUTTON_RIGHT
	clic.pressed = true
	rebinder.submit_event(clic)
	_verifier("Rebinder : clic de souris remplace la touche", action.mouse_button == MOUSE_BUTTON_RIGHT and action.key == KEY_NONE and action.get_slot_text("key") == "Clic droit")
	rebinder.start_rebind(action, &"joy_button")
	var gachette := InputEventJoypadMotion.new()
	gachette.axis = JOY_AXIS_TRIGGER_LEFT
	gachette.axis_value = 0.3
	_verifier("Rebinder : axe sous le seuil refusé", not rebinder.submit_event(gachette))
	gachette.axis_value = 0.9
	_verifier("Rebinder : gâchette → axe du bouton", rebinder.submit_event(gachette) and action.joy_axis == JOY_AXIS_TRIGGER_LEFT and action.joy_button == JOY_BUTTON_INVALID)
	var copie_axe: InputAxisAction = joueur.get_action("gaz")
	rebinder.start_rebind(copie_axe, &"joy_axis")
	var stick := InputEventJoypadMotion.new()
	stick.axis = JOY_AXIS_RIGHT_X
	stick.axis_value = -1.0
	_verifier("Rebinder : axe remplace les boutons de l'axe", rebinder.submit_event(stick) and copie_axe.joy_axis == JOY_AXIS_RIGHT_X and copie_axe.positive_joy_button == JOY_BUTTON_INVALID)
	rebinder.start_rebind(action, &"key")
	var echap := InputEventKey.new()
	echap.keycode = KEY_ESCAPE
	echap.pressed = true
	rebinder.submit_event(echap)
	_verifier("Rebinder : Échap annule", annulations.n == 1 and not rebinder.is_listening())
	rebinder.start_rebind(action, &"key")
	await _secondes(0.35)
	_verifier("Rebinder : délai d'écoute dépassé → annulé", annulations.n == 2 and not rebinder.is_listening())
	var copie_tir: InputButtonAction = joueur.get_action("tir")
	copie_tir.key = KEY_F
	action.mouse_button = MOUSE_BUTTON_NONE
	action.key = KEY_F
	var conflits: Array = rebinder.find_conflicts(joueur, action, &"key")
	_verifier("find_conflicts : même touche sur une autre action", conflits.size() == 1 and conflits[0][0] == copie_tir)


# 🎮 Périphérique

func _tester_detection_peripherique() -> void:
	var joueur := _joueur([_contexte("jeu", [_bouton("saut", KEY_SPACE)])])
	joueur.auto_switch_device = true
	await _frames(1)
	var bascules := _espionner(joueur.device_type_changed)
	var bouton := InputEventJoypadButton.new()
	bouton.device = MANETTE
	bouton.button_index = JOY_BUTTON_A
	bouton.pressed = true
	joueur.handle_input_event(bouton)
	_verifier("Bouton de manette → type Manette, filtre basculé", bascules.n == 1 and bascules.args[0] == InputAction.DEVICE_GAMEPAD and joueur.device_filter == InputAction.DEVICE_GAMEPAD)
	var autre := InputEventJoypadButton.new()
	autre.device = MANETTE + 1
	autre.button_index = JOY_BUTTON_A
	autre.pressed = true
	var touche := InputEventKey.new()
	touche.keycode = KEY_SPACE
	touche.pressed = true
	joueur.handle_input_event(autre)
	_verifier("Manette d'un autre joueur ignorée", bascules.n == 1)
	joueur.handle_input_event(touche)
	_verifier("Touche → Clavier et souris", bascules.n == 2 and joueur.get_last_device_type() == InputAction.DEVICE_KEYBOARD_MOUSE and joueur.device_filter == InputAction.DEVICE_KEYBOARD_MOUSE)
	_verifier("Informations du périphérique (sans manette réelle n° 7)", not joueur.is_device_connected() and joueur.get_joy_info() is Dictionary)


# ⚙️ PlayerInputUI

func _tester_interface() -> void:
	var saut := _bouton("saut", KEY_SPACE)
	saut.joy_button = JOY_BUTTON_A
	var deplacement := InputCircleAction.new()
	deplacement.action_name = "deplacement"
	var joueur := _joueur([_contexte("jeu", [saut, deplacement])])
	joueur.name = "JoueurMenu"
	var interface := PlayerInputUI.new()
	interface.player_input = NodePath("../JoueurMenu")
	interface.save_path = FICHIER_CONTROLES
	interface.auto_load = false
	_ajouter(interface)
	await _frames(1)
	var onglets := interface.find_children("*", "TabContainer", true, false)
	var boutons := interface.find_children("*", "Button", true, false).filter(func(b): return b.text.contains(":"))
	_verifier("PlayerInputUI : deux onglets, un bouton par emplacement", onglets.size() == 1 and onglets[0].get_tab_count() == 2 and boutons.size() == 1 + 4 + 1 + 6, str(boutons.size()))
	var bouton_saut = boutons.filter(func(b): return b.text.contains("Space"))[0]
	bouton_saut.pressed.emit()
	var rebinder: InputRebinder = interface.get_active_rebinder()
	_verifier("Clic sur un emplacement : écoute", rebinder.is_listening() and bouton_saut.text.contains("appuyez"))
	var touche := InputEventKey.new()
	touche.keycode = KEY_E
	touche.physical_keycode = KEY_E
	touche.pressed = true
	rebinder.submit_event(touche)
	_verifier("Touche assignée et bouton mis à jour (conflit signalé)", joueur.get_action("saut").key == KEY_E and bouton_saut.text.ends_with("E"))
	var sauvegardes := _espionner(interface.bindings_saved)
	_verifier("Enregistrer", interface.save_bindings() == OK and sauvegardes.n == 1 and FileAccess.file_exists(FICHIER_CONTROLES))
	interface.reset_bindings()
	_verifier("Réinitialiser", joueur.get_action("saut").key == KEY_SPACE and bouton_saut.text.ends_with("Space"))
	interface.queue_free()
	var relue := PlayerInputUI.new()
	relue.player_input = NodePath("../JoueurMenu")
	relue.save_path = FICHIER_CONTROLES
	_ajouter(relue)
	await _frames(1)
	_verifier("auto_load : contrôles relus au démarrage", joueur.get_action("saut").key == KEY_E)
	DirAccess.remove_absolute(ProjectSettings.globalize_path(FICHIER_CONTROLES))


func _tester_avertissements() -> void:
	var vide := PlayerInput.new()
	_ajouter(vide)
	_verifier("Avertissement : aucun contexte", vide.get_configuration_issues().size() == 1)
	var doublon := _contexte("jeu", [_bouton("saut"), _bouton("saut")])
	var liaison := InputBinding.new()
	liaison.target = NodePath("../Nulle/Part")
	liaison.method = &"hide"
	doublon.get_actions()[0].bindings = [liaison]
	vide.contexts = [doublon, _contexte("jeu", [])]
	var avertissements: PackedStringArray = vide.get_configuration_issues()
	_verifier("Avertissements : contextes et actions en double, cible introuvable", avertissements.size() == 3, str(avertissements))


# 🌀 Pilotage d'un déplacement (module Mouvement & Physique, par liaisons)

func _tester_deplacement() -> void:
	if not ClassDB.class_exists(&"MovementDirector"):
		_avertir_si("Module Mouvement & Physique absent : test de pilotage ignoré", false, "MovementDirector introuvable")
		return
	var corps := CharacterBody2D.new()
	corps.name = "Corps"
	corps.motion_mode = CharacterBody2D.MOTION_MODE_FLOATING
	corps.position = Vector2(90000, 0)
	var forme := CollisionShape2D.new()
	forme.shape = CircleShape2D.new()
	corps.add_child(forme)
	var sol: Node = ClassDB.instantiate(&"GroundMovement2D")
	sol.name = "Sol"
	sol.set("mode", 1)
	corps.add_child(sol)
	var directeur: Node = ClassDB.instantiate(&"MovementDirector")
	directeur.name = "Directeur"
	corps.add_child(directeur)
	_ajouter(corps)
	var deplacement := InputCircleAction.new()
	deplacement.action_name = "deplacement"
	var liaison := InputBinding.new()
	liaison.event = &"value_changed"
	liaison.target = NodePath("../Corps/Directeur")
	liaison.method = &"move"
	deplacement.bindings = [liaison]
	var joueur := _joueur([_contexte("jeu", [deplacement])])
	joueur.process_in_physics = true
	await _frames(1)
	joueur.simulate("deplacement", Vector2(1, 0))
	for i in 20:
		await _bac_a_sable.get_tree().physics_frame
	_verifier("PlayerInput → MovementDirector.move : le corps avance", corps.position.x > 90010, str(corps.position))
	joueur.stop_simulation("deplacement")
	for i in 40:
		await _bac_a_sable.get_tree().physics_frame
	_verifier("Relâché : le corps s'arrête", corps.velocity.length() < 1.0, str(corps.velocity))
