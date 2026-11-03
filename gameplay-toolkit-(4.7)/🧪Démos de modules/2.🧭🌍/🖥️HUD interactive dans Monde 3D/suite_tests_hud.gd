extends "../suite_tests_base.gd"

## Suite de tests automatiques du module HUD spatial interactif (InteractiveHUD3D).
##
## La souris est simulée par des évènements envoyés au Viewport (push_input en coordonnées locales) : ils
## suivent le vrai trajet des entrées (_input, interface, _unhandled_input). Les positions à l'écran sont
## calculées en projetant les pixels du HUD (hud_to_world puis Camera3D.unproject_position).
## Le HUD de test est placé à gauche de l'écran (le panneau des démos intercepte les clics à droite).
## Erreur attendue dans la console : press_pointer() hors du mode POINTER_SOURCE_MANUAL.

const TAILLE := Vector2i(256, 160)
const PIXEL := 0.002
const CENTRE_BOUTON := Vector2(70, 40)
const ESPION := "extends Node\nvar clics := 0\nfunc _unhandled_input(evenement: InputEvent) -> void:\n\tif evenement is InputEventMouseButton and evenement.pressed:\n\t\tclics += 1\n"

var _camera: Camera3D
var _racine: Node3D


func _executer() -> void:
	_tester_defauts()
	await _tester_construction()
	await _tester_correspondance()
	await _tester_survol()
	await _tester_clic()
	await _tester_capture()
	await _tester_consommation()
	await _tester_plus_proche()
	await _tester_face_arriere()
	await _tester_distance_max()
	await _tester_occultation()
	await _tester_clavier()
	await _tester_billboard()
	await _tester_pointeur_manuel()
	await _tester_centre_ecran()
	await _tester_camera_mobile()


# 🧱 Construction

func _tester_defauts() -> void:
	var hud := InteractiveHUD3D.new()
	_verifier("Défauts : 512×320 px, 0,0025 m/px (quad de 1,28 × 0,8 m)",
		hud.viewport_size == Vector2i(512, 320) and is_equal_approx(hud.pixel_size, 0.0025) and hud.get_quad_size().is_equal_approx(Vector2(1.28, 0.8)))
	_verifier("Défauts : interactif, souris, double face, sans ombrage, sans billboard ni distance max",
		hud.interactive and hud.pointer_source == InteractiveHUD3D.POINTER_SOURCE_MOUSE and hud.double_sided and hud.unshaded
		and hud.billboard == InteractiveHUD3D.BILLBOARD_DISABLED and hud.max_distance == 0.0 and not hud.check_occlusion and hud.forward_keyboard)
	_verifier("Défauts : rien de survolé ni capturé", not hud.is_hovered() and not hud.is_captured() and not hud.is_focused() and hud.get_pointer_position() == Vector2(-1, -1))
	hud.free()


func _tester_construction() -> void:
	_preparer()
	var hud := _hud(Vector3.ZERO)
	_verifier("SubViewport interne à la taille demandée", hud.get_sub_viewport() != null and hud.get_sub_viewport().size == TAILLE)
	_verifier("Enfants internes : non visibles dans get_children (jamais sauvegardés)", hud.get_child_count() == 0 and hud.get_child_count(true) == 2)
	var contenu: Control = hud.get_content()
	_verifier("set_content : Control étiré sur tout le SubViewport", contenu != null and contenu.size == Vector2(TAILLE), str(contenu.size if contenu else null))

	var modele := Label.new()
	modele.text = "Depuis une scène"
	hud.content_scene = _empaqueter(modele)
	await _frames(1)
	_verifier("content_scene : instanciée et remplace l'ancien contenu", hud.get_content() is Label and hud.get_content().text == "Depuis une scène"
		and not is_instance_valid(contenu))
	hud.viewport_size = Vector2i(100, 50)
	hud.pixel_size = 0.01
	_verifier("viewport_size / pixel_size : SubViewport et quad redimensionnés",
		hud.get_sub_viewport().size == Vector2i(100, 50) and hud.get_quad_size().is_equal_approx(Vector2(1.0, 0.5)))
	await _liberer()


func _tester_correspondance() -> void:
	_preparer()
	var hud := _hud(Vector3(-0.4, 0, 0))
	_verifier("hud_to_world : centre du SubViewport = position du HUD", hud.hud_to_world(Vector2(TAILLE) / 2).is_equal_approx(Vector3(-0.4, 0, 0)))
	_verifier("hud_to_world : coin haut gauche", hud.hud_to_world(Vector2.ZERO).is_equal_approx(Vector3(-0.4 - 0.256, 0.16, 0)))
	var justes := true
	for pixel in [Vector2(10, 10), Vector2(128, 80), Vector2(250, 150)]:
		var retour = hud.screen_to_hud(_ecran(hud, pixel))
		justes = justes and retour != null and (retour as Vector2).distance_to(pixel) < 0.5
	_verifier("screen_to_hud(unproject(hud_to_world(p))) ≈ p", justes)
	_verifier("screen_to_hud : hors du quad → null", hud.screen_to_hud(_ecran(hud, Vector2(300, 80))) == null)
	_verifier("ray_to_hud : rayon parallèle au plan → null", hud.ray_to_hud(Vector3(-2, 0, 0), Vector3.RIGHT) == null)
	_verifier("ray_to_hud : plan derrière l'origine du rayon → null", hud.ray_to_hud(Vector3(-0.4, 0, 1), Vector3.BACK) == null)
	await _liberer()


# 🖱️ Souris

func _tester_survol() -> void:
	_preparer()
	var hud := _hud(Vector3(-0.4, 0, 0))
	var entree := _espionner(hud.pointer_entered)
	var sortie := _espionner(hud.pointer_exited)
	var bouton: Button = hud.get_content().get_node("Bouton")
	_bouger(_ecran(hud, CENTRE_BOUTON))
	_verifier("Survol : pointer_entered, position en pixels du HUD", hud.is_hovered() and entree.n == 1 and hud.get_pointer_position().distance_to(CENTRE_BOUTON) < 0.5,
		str(hud.get_pointer_position()))
	_verifier("Survol : le Control sous le pointeur est survolé dans le SubViewport", bouton.is_hovered())
	_bouger(_ecran(hud, Vector2(-50, 80)))
	_verifier("Sortie : pointer_exited, position (-1, -1)", not hud.is_hovered() and sortie.n == 1 and hud.get_pointer_position() == Vector2(-1, -1))
	_verifier("Sortie : plus aucun Control survolé dans le SubViewport", not bouton.is_hovered())
	await _liberer()


func _tester_clic() -> void:
	_preparer()
	var hud := _hud(Vector3(-0.4, 0, 0))
	var bouton: Button = hud.get_content().get_node("Bouton")
	var presse := _espionner(bouton.pressed)
	var appui := _espionner(hud.pointer_pressed)
	var relache := _espionner(hud.pointer_released)
	_cliquer(_ecran(hud, CENTRE_BOUTON))
	_verifier("Clic sur le HUD : Button.pressed", presse.n == 1)
	_verifier("pointer_pressed / pointer_released (pixel, bouton)", appui.n == 1 and relache.n == 1 and (appui.args[0] as Vector2).distance_to(CENTRE_BOUTON) < 0.5
		and appui.args[1] == MOUSE_BUTTON_LEFT, str(appui.args))
	_verifier("Clic : le HUD prend le focus, capture relâchée", hud.is_focused() and not hud.is_captured())
	_cliquer(_ecran(hud, Vector2(200, 40)))
	_verifier("Clic à côté du bouton (sur le HUD) : pas de pressed", presse.n == 1)

	_appuyer(_ecran(hud, CENTRE_BOUTON), MOUSE_BUTTON_WHEEL_DOWN)
	_appuyer(_ecran(hud, CENTRE_BOUTON), MOUSE_BUTTON_WHEEL_DOWN, false)
	_verifier("Molette : ne capture pas le pointeur", not hud.is_captured() and appui.n == 2)
	await _liberer()


func _tester_capture() -> void:
	_preparer()
	var hud := _hud(Vector3(-0.4, 0, 0))
	var curseur: HSlider = hud.get_content().get_node("Curseur")
	var relache := _espionner(hud.pointer_released)
	_appuyer(_ecran(hud, Vector2(25, 90)))
	_verifier("Appui : bouton capturé", hud.is_captured())
	_bouger(_ecran(hud, Vector2(400, 90)), MOUSE_BUTTON_MASK_LEFT) # bien au-delà du bord droit du quad
	_verifier("Capturé : le glissement continue hors du quad (curseur au maximum)", curseur.value == curseur.max_value, str(curseur.value))
	_verifier("Capturé : position suivie sur le plan, au-delà du SubViewport", hud.get_pointer_position().x > TAILLE.x)
	_appuyer(_ecran(hud, Vector2(400, 90)), MOUSE_BUTTON_LEFT, false)
	_verifier("Relâchement hors du quad : jamais perdu", not hud.is_captured() and relache.n == 1)
	await _frames(1)
	_verifier("Après relâchement hors du quad : plus survolé", not hud.is_hovered())
	await _liberer()


func _tester_consommation() -> void:
	_preparer()
	var espion := _espion()
	var hud := _hud(Vector3(-0.4, 0, 0))
	_cliquer(_ecran(hud, CENTRE_BOUTON))
	_verifier("Clic sur le HUD : consommé (pas d'_unhandled_input pour le jeu)", espion.clics == 0, "%d clic(s) reçus" % espion.clics)
	_cliquer(_ecran(hud, Vector2(-80, 80)))
	_verifier("Clic à côté du HUD : transmis au jeu", espion.clics == 1)
	_verifier("Clic à côté du HUD : le HUD perd le focus", not hud.is_focused())

	hud.interactive = false
	_cliquer(_ecran(hud, CENTRE_BOUTON))
	_verifier("interactive = false : le clic traverse le HUD", espion.clics == 2 and not hud.is_hovered())
	hud.interactive = true
	hud.visible = false
	_cliquer(_ecran(hud, CENTRE_BOUTON))
	_verifier("HUD caché : le clic le traverse", espion.clics == 3)
	await _liberer()


func _tester_plus_proche() -> void:
	_preparer()
	var arriere := _hud(Vector3(-0.4, 0, 0))
	var avant := _hud(Vector3(-0.4 * 0.75, 0, 0.375)) # même direction depuis la caméra, plus près
	var presse_arriere := _espionner((arriere.get_content().get_node("Bouton") as Button).pressed)
	var presse_avant := _espionner((avant.get_content().get_node("Bouton") as Button).pressed)
	var position := _ecran(arriere, Vector2(128, 80))
	_bouger(position)
	_verifier("Deux HUD superposés : seul le plus proche est survolé", avant.is_hovered() and not arriere.is_hovered())

	# Le plus proche n'interagit plus : l'autre reçoit le pointeur.
	avant.interactive = false
	_bouger(position)
	_verifier("Plus proche non interactif : le HUD de derrière prend le relais", arriere.is_hovered())
	avant.interactive = true
	avant.process_mode = Node.PROCESS_MODE_DISABLED
	_bouger(position)
	_verifier("Plus proche suspendu (process_mode) : ignoré", arriere.is_hovered() and not avant.is_hovered())
	_verifier("Aucun clic parasite", presse_arriere.n == 0 and presse_avant.n == 0)
	await _liberer()


func _tester_face_arriere() -> void:
	_preparer()
	var hud := _hud(Vector3(-0.4, 0, 0))
	hud.rotation.y = PI
	await _frames(1)
	var position := _ecran(hud, Vector2(128, 80))
	_bouger(position)
	_verifier("double_sided : survolable de dos", hud.is_hovered())
	hud.double_sided = false
	_bouger(position + Vector2(1, 0))
	_verifier("Simple face : ignoré de dos", not hud.is_hovered() and hud.screen_to_hud(position) == null)
	await _liberer()


func _tester_distance_max() -> void:
	_preparer()
	var hud := _hud(Vector3(-0.4, 0, 0))
	hud.max_distance = 1.0
	_bouger(_ecran(hud, CENTRE_BOUTON))
	_verifier("max_distance (1 m < 1,5 m) : hors de portée", not hud.is_hovered())
	hud.max_distance = 5.0
	_bouger(_ecran(hud, CENTRE_BOUTON) + Vector2(1, 0))
	_verifier("max_distance (5 m) : à portée", hud.is_hovered())
	await _liberer()


func _tester_occultation() -> void:
	_preparer()
	var hud := _hud(Vector3(-0.4, 0, 0))
	var mur := StaticBody3D.new()
	var forme := CollisionShape3D.new()
	forme.shape = BoxShape3D.new()
	(forme.shape as BoxShape3D).size = Vector3(2, 2, 0.1)
	mur.add_child(forme)
	mur.position = Vector3(0, 0, 0.7)
	_racine.add_child(mur)
	await _bac_a_sable.get_tree().physics_frame
	await _bac_a_sable.get_tree().physics_frame

	var position := _ecran(hud, CENTRE_BOUTON)
	_bouger(position)
	_verifier("check_occlusion = false : un corps devant n'empêche rien", hud.is_hovered())
	hud.check_occlusion = true
	_bouger(position + Vector2(1, 0))
	_verifier("check_occlusion : HUD caché derrière un corps physique", not hud.is_hovered())
	hud.occlusion_mask = 2
	_bouger(position)
	_verifier("occlusion_mask : couche non testée → pas d'occultation", hud.is_hovered())
	await _liberer()


func _tester_clavier() -> void:
	_preparer()
	var hud := _hud(Vector3(-0.4, 0, 0))
	var champ: LineEdit = hud.get_content().get_node("Champ")
	_touche(KEY_A, "a")
	_verifier("Sans focus : touche non transmise", champ.text == "")
	_cliquer(_ecran(hud, Vector2(100, 125)))
	_verifier("Clic sur le champ : focus du HUD et du LineEdit", hud.is_focused() and champ.has_focus())
	_touche(KEY_A, "a")
	_touche(KEY_B, "b")
	_verifier("Avec focus : touches transmises au LineEdit", champ.text == "ab", champ.text)
	hud.forward_keyboard = false
	_touche(KEY_C, "c")
	_verifier("forward_keyboard = false : non transmises", champ.text == "ab")
	hud.forward_keyboard = true
	_cliquer(_ecran(hud, Vector2(-80, 80)))
	_touche(KEY_D, "d")
	_verifier("Clic ailleurs : focus perdu, touches non transmises", not hud.is_focused() and not champ.has_focus() and champ.text == "ab")
	await _liberer()


func _tester_billboard() -> void:
	_preparer()
	var hud := _hud(Vector3(-0.4, 0, 0))
	_camera.position.x = -0.4 # dans l'axe du HUD
	hud.rotation.y = PI / 2 # vu par la tranche
	await _frames(1)
	_verifier("Sans billboard : vu par la tranche, non touché", hud.screen_to_hud(_camera.unproject_position(hud.global_position)) == null)
	hud.billboard = InteractiveHUD3D.BILLBOARD_ENABLED
	await _frames(1)
	var centre = hud.screen_to_hud(_camera.unproject_position(hud.global_position))
	_verifier("BILLBOARD_ENABLED : face à la caméra malgré la rotation du nœud", centre != null and (centre as Vector2).distance_to(Vector2(TAILLE) / 2) < 1.0, str(centre))

	_camera.position = Vector3(-0.4, 1.5, 1.5)
	_camera.look_at(hud.global_position)
	hud.billboard = InteractiveHUD3D.BILLBOARD_Y
	await _frames(1)
	var haut := (hud.hud_to_world(Vector2(128, 0)) - hud.hud_to_world(Vector2(128, 160))).normalized()
	var vers_camera := _camera.global_position - hud.global_position
	var droite := (hud.hud_to_world(Vector2(256, 80)) - hud.hud_to_world(Vector2(0, 80))).normalized()
	_verifier("BILLBOARD_Y : reste vertical, tourné vers la caméra autour de Y",
		haut.is_equal_approx(Vector3.UP) and absf(droite.dot(Vector3(vers_camera.x, 0, vers_camera.z).normalized())) < 0.001, "haut %s" % haut)
	await _liberer()


func _tester_pointeur_manuel() -> void:
	_preparer()
	var hud := _hud(Vector3(-0.4, 0, 0))
	var bouton: Button = hud.get_content().get_node("Bouton")
	var presse := _espionner(bouton.pressed)
	hud.press_pointer() # erreur attendue : mode souris
	hud.pointer_source = InteractiveHUD3D.POINTER_SOURCE_MANUAL
	_bouger(_ecran(hud, CENTRE_BOUTON))
	_verifier("MANUAL : la souris est ignorée", not hud.is_hovered())

	var cible := hud.hud_to_world(CENTRE_BOUTON)
	var origine := Vector3(-1, 0.5, 1)
	hud.set_pointer_ray(origine, cible - origine)
	_verifier("set_pointer_ray : survol au point visé", hud.is_hovered() and hud.get_pointer_position().distance_to(CENTRE_BOUTON) < 0.5, str(hud.get_pointer_position()))
	hud.press_pointer()
	_verifier("press_pointer : capturé", hud.is_captured())
	hud.release_pointer()
	_verifier("press_pointer / release_pointer : Button.pressed", presse.n == 1 and not hud.is_captured())

	hud.press_pointer()
	hud.clear_pointer_ray()
	_verifier("clear_pointer_ray : sortie, bouton capturé relâché", not hud.is_hovered() and not hud.is_captured())
	await _liberer()


func _tester_centre_ecran() -> void:
	_preparer()
	var hud := _hud(Vector3.ZERO)
	_camera.position = Vector3(0, 0, 1.5)
	hud.pointer_source = InteractiveHUD3D.POINTER_SOURCE_SCREEN_CENTER
	await _frames(2)
	_verifier("SCREEN_CENTER : survolé sans évènement souris (centre de l'écran)", hud.is_hovered()
		and hud.get_pointer_position().distance_to(Vector2(TAILLE) / 2) < 1.0, str(hud.get_pointer_position()))
	var appui := _espionner(hud.pointer_pressed)
	_cliquer(Vector2(5, 5)) # position du clic ignorée : c'est le centre qui vise
	_verifier("SCREEN_CENTER : un clic n'importe où vise le centre", appui.n == 1 and (appui.args[0] as Vector2).distance_to(Vector2(TAILLE) / 2) < 1.0)
	await _liberer()


func _tester_camera_mobile() -> void:
	_preparer()
	var hud := _hud(Vector3(-0.4, 0, 0))
	var sortie := _espionner(hud.pointer_exited)
	_bouger(_ecran(hud, CENTRE_BOUTON))
	await _frames(1)
	_camera.position.x += 3.0
	await _frames(2)
	_verifier("Caméra déplacée sous un pointeur immobile : sortie détectée à la frame suivante", not hud.is_hovered() and sortie.n == 1)
	_camera.position.x -= 3.0
	await _frames(2)
	_verifier("Caméra revenue : survol retrouvé sans évènement souris", hud.is_hovered())
	await _liberer()


# 🛠️ Utilitaires

## Racine 3D et caméra (z = 1,5 m, vers -Z) neuves dans le bac à sable.
func _preparer() -> void:
	_racine = _ajouter(Node3D.new())
	_camera = Camera3D.new()
	_camera.position = Vector3(0, 0, 1.5)
	_racine.add_child(_camera)
	_camera.make_current()


func _liberer() -> void:
	_racine.queue_free()
	await _frames(1)


## HUD de 256×160 px (0,512 × 0,32 m) avec un bouton, un curseur horizontal et un champ de texte.
func _hud(position: Vector3) -> InteractiveHUD3D:
	var hud := InteractiveHUD3D.new()
	hud.viewport_size = TAILLE
	hud.pixel_size = PIXEL
	hud.position = position
	_racine.add_child(hud)

	var contenu := Control.new()
	var bouton := Button.new()
	bouton.name = "Bouton"
	bouton.text = "OK"
	bouton.position = Vector2(20, 20)
	bouton.size = Vector2(100, 40)
	contenu.add_child(bouton)
	var curseur := HSlider.new()
	curseur.name = "Curseur"
	curseur.position = Vector2(20, 80)
	curseur.size = Vector2(200, 20)
	contenu.add_child(curseur)
	var champ := LineEdit.new()
	champ.name = "Champ"
	champ.position = Vector2(20, 110)
	champ.size = Vector2(200, 30)
	contenu.add_child(champ)
	hud.set_content(contenu)
	return hud


func _espion() -> Node:
	var script := GDScript.new()
	script.source_code = ESPION
	script.reload()
	var espion := Node.new()
	espion.set_script(script)
	# Ajouté avant le HUD : _unhandled_input parcourt l'arbre à l'envers, le HUD passe en premier.
	_racine.add_child(espion)
	return espion


func _ecran(hud: InteractiveHUD3D, pixel: Vector2) -> Vector2:
	return _camera.unproject_position(hud.hud_to_world(pixel))


func _bouger(position: Vector2, masque: int = 0) -> void:
	var mouvement := InputEventMouseMotion.new()
	mouvement.position = position
	mouvement.global_position = position
	mouvement.button_mask = masque
	_bac_a_sable.get_viewport().push_input(mouvement, true)


func _appuyer(position: Vector2, bouton: MouseButton = MOUSE_BUTTON_LEFT, presse: bool = true) -> void:
	var evenement := InputEventMouseButton.new()
	evenement.position = position
	evenement.global_position = position
	evenement.button_index = bouton
	evenement.pressed = presse
	evenement.button_mask = (1 << (bouton - 1)) if presse else 0
	_bac_a_sable.get_viewport().push_input(evenement, true)


func _cliquer(position: Vector2) -> void:
	_bouger(position)
	_appuyer(position)
	_appuyer(position, MOUSE_BUTTON_LEFT, false)


func _touche(code: Key, caractere: String) -> void:
	for presse in [true, false]:
		var touche := InputEventKey.new()
		touche.keycode = code
		touche.physical_keycode = code
		touche.unicode = caractere.unicode_at(0)
		touche.pressed = presse
		_bac_a_sable.get_viewport().push_input(touche, true)
