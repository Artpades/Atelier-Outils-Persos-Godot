extends "../suite_tests_base.gd"

## Suite de tests automatiques du module glisser-déposer : DraggableComponent et DragAndDropSystem.
##
## La souris est simulée par des événements envoyés au Viewport (push_input en coordonnées locales) :
## ils suivent le vrai trajet des entrées (_input, interface, _unhandled_input). Chaque test enchaîne ses
## événements sans attendre de frame, pour qu'un mouvement de la vraie souris ne s'intercale pas.
## Erreurs attendues dans la console : saisie d'un composant nul, saisie alors qu'un objet est déjà saisi.


func _executer() -> void:
	_tester_composant()
	_tester_position_initiale()
	await _tester_systeme_defauts()
	await _tester_resolution_chemins()
	await _tester_enregistrement()
	await _tester_survol_node2d()
	await _tester_survol_control()
	await _tester_priorite()
	await _tester_maintien()
	await _tester_coller_au_curseur()
	await _tester_bascule()
	await _tester_bouton_action()
	await _tester_interface_bloquante()
	await _tester_relachement_retour()
	await _tester_relachement_point_valide()
	await _tester_relachement_sur_place()
	await _tester_relachement_sans_effet()
	await _tester_aimantage_permanent()
	await _tester_seuil_et_plus_proche()
	await _tester_control_glisse()
	await _tester_saisie_par_code()
	await _tester_cible_liberee()
	await _tester_detection_3d()
	await _tester_glissement_3d_plan_camera()
	await _tester_glissement_3d_sol()
	await _tester_aimantage_3d()


# 📋 Ressource

func _tester_composant() -> void:
	var composant := DraggableComponent.new()
	_verifier("DraggableComponent : zone 2D centrée de 64 px", composant.local_bounds_2d == Rect2(-32, -32, 64, 64), str(composant.local_bounds_2d))
	_verifier("DraggableComponent : boîte 3D unité centrée", composant.bounds_3d == AABB(Vector3(-0.5, -0.5, -0.5), Vector3.ONE), str(composant.bounds_3d))
	_verifier("DraggableComponent : défauts (point valide, aimanté si proche, 50, plan caméra)",
		composant.release_behavior == DraggableComponent.RELEASE_SNAP_TO_VALID_POINT and composant.snap_behavior == DraggableComponent.SNAP_WHEN_CLOSE
		and is_equal_approx(composant.snap_distance_threshold, 50.0) and composant.drag_plane_3d == DraggableComponent.DRAG_PLANE_CAMERA
		and not composant.snap_to_cursor and composant.snap_targets.is_empty())

	composant.target_node_path = NodePath("../Cible")
	composant.release_behavior = DraggableComponent.RELEASE_LEAVE_IN_PLACE
	composant.snap_targets = [NodePath("../A"), NodePath("../B")] as Array[NodePath]
	var copie: DraggableComponent = composant.duplicate()
	_verifier("DraggableComponent : sérialisation (duplicate)",
		copie.target_node_path == NodePath("../Cible") and copie.release_behavior == DraggableComponent.RELEASE_LEAVE_IN_PLACE and copie.snap_targets.size() == 2)


func _tester_position_initiale() -> void:
	var composant := DraggableComponent.new()
	var node_2d := Node2D.new()
	node_2d.position = Vector2(12, 34)
	var control := Control.new()
	control.position = Vector2(56, 78)
	var node_3d := Node3D.new()
	node_3d.position = Vector3(1, 2, 3)
	for noeud in [node_2d, control, node_3d]:
		_ajouter(noeud)

	composant.save_initial_position(node_2d)
	_verifier("save_initial_position : Node2D", composant.initial_position_2d == Vector2(12, 34))
	composant.save_initial_position(control)
	_verifier("save_initial_position : Control", composant.initial_position_2d == Vector2(56, 78))
	composant.save_initial_position(node_3d)
	_verifier("save_initial_position : Node3D", composant.initial_position_3d == Vector3(1, 2, 3))
	composant.target_node = node_2d
	node_2d.position = Vector2(90, 10)
	composant.save_initial_position()
	_verifier("save_initial_position : nœud ciblé par défaut", composant.initial_position_2d == Vector2(90, 10))

	for noeud in [node_2d, control, node_3d]:
		noeud.free()


# 🧩 Système

func _tester_systeme_defauts() -> void:
	var systeme := DragAndDropSystem.new()
	_verifier("DragAndDropSystem : bouton gauche, mode maintien",
		systeme.action_button == MOUSE_BUTTON_LEFT and systeme.input_mode == DragAndDropSystem.INPUT_MODE_HOLD)
	_verifier("DragAndDropSystem : libre au départ", systeme.is_free() and not systeme.is_occupied() and systeme.get_grabbed_component() == null)
	systeme.release() # sans objet saisi : sans effet
	_verifier("release() sans objet saisi : sans effet", systeme.is_free())
	systeme.free()
	await _frames(0)


func _tester_resolution_chemins() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(50, 60), "Jeton")
	var composant := DraggableComponent.new()
	composant.target_node_path = NodePath("../Jeton")
	var systeme := DragAndDropSystem.new()
	systeme.draggables = [composant] as Array[DraggableComponent]
	_ajouter(systeme, racine) # _ready : résolution et position initiale

	_verifier("_ready : target_node_path résolu (relatif au système)", composant.target_node == jeton)
	_verifier("_ready : position initiale mémorisée", composant.initial_position_2d == Vector2(50, 60))

	# Nœud ajouté après coup : résolu à la première détection, à sa position du moment.
	var tardif := DraggableComponent.new()
	tardif.target_node_path = NodePath("../Tardif")
	systeme.register_draggable(tardif)
	_verifier("Chemin pas encore résolvable : pas de cible", tardif.target_node == null)
	var noeud_tardif := _noeud_2d(racine, Vector2(300, 80), "Tardif")
	_verifier("Nœud ajouté plus tard : détecté", systeme.get_draggable_at(Vector2(300, 80)) == tardif)
	_verifier("Nœud ajouté plus tard : résolu et position initiale mémorisée",
		tardif.target_node == noeud_tardif and tardif.initial_position_2d == Vector2(300, 80))
	await _liberer(racine)


func _tester_enregistrement() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var montage := _monter(racine, jeton)
	var systeme: DragAndDropSystem = montage.systeme
	var composant: DraggableComponent = montage.composant

	systeme.register_draggable(composant)
	systeme.register_draggable(null)
	_verifier("register_draggable : ni doublon ni null", systeme.draggables.size() == 1)

	var depointe := _espionner(systeme.depointed)
	_souris(Vector2(100, 100))
	_verifier("Survol avant désinscription", systeme.get_hovered_component() == composant)
	systeme.unregister_draggable(composant)
	_verifier("unregister_draggable : retiré, survol annulé avec depointed",
		systeme.draggables.is_empty() and systeme.get_hovered_component() == null and depointe.n == 1)

	systeme.register_draggable(composant)
	systeme.grab(composant)
	systeme.unregister_draggable(composant)
	_verifier("unregister_draggable de l'objet saisi : saisie abandonnée", systeme.is_free())

	systeme.draggables = [null, composant] as Array[DraggableComponent]
	_verifier("Composant null dans la liste : ignoré", systeme.get_draggable_at(Vector2(100, 100)) == composant)
	await _liberer(racine)


# 👆 Survol

func _tester_survol_node2d() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var montage := _monter(racine, jeton)
	var systeme: DragAndDropSystem = montage.systeme
	var composant: DraggableComponent = montage.composant

	_verifier("Node2D : détecté au centre", systeme.get_draggable_at(Vector2(100, 100)) == composant)
	_verifier("Node2D : détecté près du bord", systeme.get_draggable_at(Vector2(131, 69)) == composant)
	_verifier("Node2D : rien hors de la zone", systeme.get_draggable_at(Vector2(133, 100)) == null)

	jeton.rotation = PI / 4
	_verifier("Node2D tourné : la zone suit la rotation", systeme.get_draggable_at(Vector2(100, 140)) == composant
		and systeme.get_draggable_at(Vector2(127, 127)) == null)
	jeton.rotation = 0.0
	jeton.scale = Vector2(2, 2)
	_verifier("Node2D agrandi : la zone suit l'échelle", systeme.get_draggable_at(Vector2(160, 100)) == composant)
	jeton.scale = Vector2.ONE
	composant.local_bounds_2d = Rect2(0, 0, 40, 20)
	_verifier("local_bounds_2d décalée (coin haut-gauche à l'origine)",
		systeme.get_draggable_at(Vector2(130, 110)) == composant and systeme.get_draggable_at(Vector2(95, 95)) == null)
	composant.local_bounds_2d = Rect2(-32, -32, 64, 64)
	jeton.visible = false
	_verifier("Node2D invisible : ignoré", systeme.get_draggable_at(Vector2(100, 100)) == null)
	jeton.visible = true

	var pointe := _espionner(systeme.pointed)
	var depointe := _espionner(systeme.depointed)
	_souris(Vector2(300, 300))
	_souris(Vector2(105, 95))
	_verifier("Signal pointed au survol (avec le nœud)", pointe.n == 1 and pointe.args[0] == jeton and systeme.get_hovered_component() == composant,
		"%d émission(s)" % pointe.n)
	_souris(Vector2(110, 100))
	_verifier("Pas de nouveau pointed tant que le survol continue", pointe.n == 1)
	_souris(Vector2(300, 300))
	_verifier("Signal depointed en quittant l'objet", depointe.n == 1 and depointe.args[0] == jeton and systeme.get_hovered_component() == null)
	await _liberer(racine)


func _tester_survol_control() -> void:
	var racine := _racine()
	var carte := _carte(racine, Vector2(100, 100))
	var montage := _monter(racine, carte)
	var systeme: DragAndDropSystem = montage.systeme
	var composant: DraggableComponent = montage.composant

	_verifier("Control : détecté dans son rectangle", systeme.get_draggable_at(Vector2(175, 195)) == composant)
	_verifier("Control : rien hors de son rectangle", systeme.get_draggable_at(Vector2(185, 150)) == null
		and systeme.get_draggable_at(Vector2(95, 150)) == null)
	carte.rotation = PI / 2 # pivot au coin haut-gauche
	_verifier("Control tourné : le rectangle suit la rotation",
		systeme.get_draggable_at(Vector2(50, 120)) == composant and systeme.get_draggable_at(Vector2(150, 150)) == null)
	carte.rotation = 0.0
	carte.hide()
	_verifier("Control invisible : ignoré", systeme.get_draggable_at(Vector2(150, 150)) == null)
	await _liberer(racine)


func _tester_priorite() -> void:
	var racine := _racine()
	var premier := _noeud_2d(racine, Vector2(100, 100))
	var second := _noeud_2d(racine, Vector2(120, 100))
	var montage := _monter(racine, premier)
	var systeme: DragAndDropSystem = montage.systeme
	var composant_second := _composant(second)
	systeme.register_draggable(composant_second)
	_verifier("Chevauchement 2D : le premier de la liste l'emporte", systeme.get_draggable_at(Vector2(110, 100)) == montage.composant)
	_verifier("Hors chevauchement : le second est détecté", systeme.get_draggable_at(Vector2(145, 100)) == composant_second)
	await _liberer(racine)


# ✊ Saisie

func _tester_maintien() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var montage := _monter(racine, jeton, {"release_behavior": DraggableComponent.RELEASE_DO_NOTHING})
	var systeme: DragAndDropSystem = montage.systeme
	var saisi := _espionner(systeme.grabbed)
	var relache := _espionner(systeme.released)

	_souris(Vector2(300, 300))
	_bouton(Vector2(300, 300), true)
	_verifier("Clic dans le vide : rien n'est saisi", systeme.is_free() and saisi.n == 0)
	_bouton(Vector2(300, 300), false)

	_souris(Vector2(110, 105))
	_bouton(Vector2(110, 105), true)
	_verifier("Maintien : clic sur l'objet → saisi (signal grabbed)",
		systeme.is_occupied() and systeme.get_grabbed_component() == montage.composant and saisi.n == 1 and saisi.args[0] == jeton)
	_souris(Vector2(210, 155))
	_verifier("Glissement : l'écart pointeur/objet est conservé", jeton.position.is_equal_approx(Vector2(200, 150)), str(jeton.position))
	_bouton(Vector2(210, 155), false)
	_verifier("Maintien : bouton relâché → libre (signal released)", systeme.is_free() and relache.n == 1 and relache.args[0] == jeton)
	_souris(Vector2(400, 300))
	_verifier("Après relâchement : l'objet ne suit plus le pointeur", jeton.position.is_equal_approx(Vector2(200, 150)), str(jeton.position))
	await _liberer(racine)


func _tester_coller_au_curseur() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	_monter(racine, jeton, {"release_behavior": DraggableComponent.RELEASE_DO_NOTHING, "snap_to_cursor": true})
	_souris(Vector2(120, 110))
	_bouton(Vector2(120, 110), true)
	_souris(Vector2(250, 180))
	_verifier("snap_to_cursor : l'origine de l'objet est sous le pointeur", jeton.position.is_equal_approx(Vector2(250, 180)), str(jeton.position))
	_bouton(Vector2(250, 180), false)
	await _liberer(racine)


func _tester_bascule() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var montage := _monter(racine, jeton, {"release_behavior": DraggableComponent.RELEASE_DO_NOTHING})
	var systeme: DragAndDropSystem = montage.systeme
	systeme.input_mode = DragAndDropSystem.INPUT_MODE_TOGGLE
	var saisi := _espionner(systeme.grabbed)
	var relache := _espionner(systeme.released)

	_souris(Vector2(100, 100))
	_bouton(Vector2(100, 100), true)
	_bouton(Vector2(100, 100), false)
	_verifier("Bascule : un clic saisit, relâcher le bouton ne relâche pas l'objet", systeme.is_occupied() and relache.n == 0)
	_souris(Vector2(200, 200))
	_verifier("Bascule : l'objet suit sans bouton enfoncé", jeton.position.is_equal_approx(Vector2(200, 200)), str(jeton.position))
	_bouton(Vector2(200, 200), true)
	_verifier("Bascule : le clic suivant relâche, sans ressaisir", systeme.is_free() and relache.n == 1 and saisi.n == 1,
		"saisi %d fois, relâché %d fois" % [saisi.n, relache.n])
	_bouton(Vector2(200, 200), false)
	await _liberer(racine)


func _tester_bouton_action() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var systeme: DragAndDropSystem = _monter(racine, jeton).systeme
	_souris(Vector2(100, 100))
	_bouton(Vector2(100, 100), true, MOUSE_BUTTON_RIGHT)
	_verifier("Autre bouton que action_button : ignoré", systeme.is_free())
	_bouton(Vector2(100, 100), false, MOUSE_BUTTON_RIGHT)
	systeme.action_button = MOUSE_BUTTON_RIGHT
	_bouton(Vector2(100, 100), true, MOUSE_BUTTON_RIGHT)
	_verifier("action_button = droit : le clic droit saisit", systeme.is_occupied())
	_bouton(Vector2(100, 100), false, MOUSE_BUTTON_LEFT)
	_verifier("Relâcher un autre bouton ne relâche pas l'objet", systeme.is_occupied())
	_bouton(Vector2(100, 100), false, MOUSE_BUTTON_RIGHT)
	_verifier("Relâcher action_button relâche l'objet", systeme.is_free())
	await _liberer(racine)


func _tester_interface_bloquante() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var systeme: DragAndDropSystem = _monter(racine, jeton, {"release_behavior": DraggableComponent.RELEASE_DO_NOTHING}).systeme
	var ecran := ColorRect.new()
	ecran.size = Vector2(400, 300)
	ecran.mouse_filter = Control.MOUSE_FILTER_STOP
	_ajouter(ecran, racine)

	_souris(Vector2(100, 100))
	_bouton(Vector2(100, 100), true)
	_verifier("Interface (MOUSE_FILTER_STOP) au-dessus : pas de saisie", systeme.is_free())
	_bouton(Vector2(100, 100), false)

	ecran.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_bouton(Vector2(100, 100), true)
	_verifier("Interface en MOUSE_FILTER_IGNORE : saisie possible", systeme.is_occupied())
	ecran.mouse_filter = Control.MOUSE_FILTER_STOP
	_souris(Vector2(150, 150))
	_bouton(Vector2(150, 150), false)
	_verifier("Relâchement au-dessus d'une interface : jamais perdu", systeme.is_free())
	await _liberer(racine)


# 🧲 Relâchement et aimantage

func _tester_relachement_retour() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var point := _noeud_2d(racine, Vector2(300, 100))
	_monter(racine, jeton, {"release_behavior": DraggableComponent.RELEASE_RETURN_TO_INITIAL}, [point])
	_glisser(Vector2(100, 100), Vector2(300, 100))
	_verifier("RETURN_TO_INITIAL : revient à sa position initiale, même sur un point", jeton.position.is_equal_approx(Vector2(100, 100)), str(jeton.position))
	await _liberer(racine)


func _tester_relachement_point_valide() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var point := _noeud_2d(racine, Vector2(300, 100))
	var systeme: DragAndDropSystem = _monter(racine, jeton, {"release_behavior": DraggableComponent.RELEASE_SNAP_TO_VALID_POINT}, [point]).systeme
	var aimante := _espionner(systeme.snapped)
	var relache := _espionner(systeme.released)

	_glisser(Vector2(100, 100), Vector2(280, 120))
	_verifier("SNAP_TO_VALID_POINT : posé sur le point proche", jeton.position.is_equal_approx(Vector2(300, 100)), str(jeton.position))
	_verifier("Signal snapped (nœud, point) puis released", aimante.n == 1 and aimante.args == [jeton, point] and relache.n == 1)
	_glisser(Vector2(300, 100), Vector2(500, 300))
	_verifier("SNAP_TO_VALID_POINT : loin de tout point → retour à la position initiale",
		jeton.position.is_equal_approx(Vector2(100, 100)) and aimante.n == 1, str(jeton.position))
	await _liberer(racine)


func _tester_relachement_sur_place() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var point := _noeud_2d(racine, Vector2(300, 100))
	var montage := _monter(racine, jeton, {"release_behavior": DraggableComponent.RELEASE_LEAVE_IN_PLACE, "snap_behavior": DraggableComponent.SNAP_WHEN_CLOSE}, [point])
	var composant: DraggableComponent = montage.composant

	_glisser(Vector2(100, 100), Vector2(280, 120))
	_verifier("LEAVE_IN_PLACE + SNAP_WHEN_CLOSE : aimanté près d'un point", jeton.position.is_equal_approx(Vector2(300, 100)), str(jeton.position))
	_glisser(Vector2(300, 100), Vector2(500, 300))
	_verifier("LEAVE_IN_PLACE : loin de tout point → reste où il est lâché", jeton.position.is_equal_approx(Vector2(500, 300)), str(jeton.position))
	composant.snap_behavior = DraggableComponent.SNAP_NONE
	_glisser(Vector2(500, 300), Vector2(290, 110))
	_verifier("LEAVE_IN_PLACE + SNAP_NONE : jamais aimanté", jeton.position.is_equal_approx(Vector2(290, 110)), str(jeton.position))
	await _liberer(racine)


func _tester_relachement_sans_effet() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var point := _noeud_2d(racine, Vector2(300, 100))
	var systeme: DragAndDropSystem = _monter(racine, jeton, {"release_behavior": DraggableComponent.RELEASE_DO_NOTHING, "snap_behavior": DraggableComponent.SNAP_WHEN_CLOSE}, [point]).systeme
	var aimante := _espionner(systeme.snapped)
	_glisser(Vector2(100, 100), Vector2(290, 110))
	_verifier("DO_NOTHING : reste où il est lâché, même près d'un point", jeton.position.is_equal_approx(Vector2(290, 110)) and aimante.n == 0, str(jeton.position))
	await _liberer(racine)


func _tester_aimantage_permanent() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var point := _noeud_2d(racine, Vector2(300, 100))
	var systeme: DragAndDropSystem = _monter(racine, jeton, {"release_behavior": DraggableComponent.RELEASE_LEAVE_IN_PLACE, "snap_behavior": DraggableComponent.SNAP_ALWAYS}, [point]).systeme
	var aimante := _espionner(systeme.snapped)

	_souris(Vector2(100, 100))
	_bouton(Vector2(100, 100), true)
	_souris(Vector2(200, 100))
	_verifier("SNAP_ALWAYS : suit le pointeur loin des points", jeton.position.is_equal_approx(Vector2(200, 100)), str(jeton.position))
	_souris(Vector2(280, 115))
	_verifier("SNAP_ALWAYS : aimanté pendant le glissement", jeton.position.is_equal_approx(Vector2(300, 100)), str(jeton.position))
	_souris(Vector2(420, 250))
	_verifier("SNAP_ALWAYS : se décroche quand le pointeur s'éloigne", jeton.position.is_equal_approx(Vector2(420, 250)), str(jeton.position))
	_souris(Vector2(310, 90))
	_bouton(Vector2(310, 90), false)
	_verifier("SNAP_ALWAYS : relâché aimanté → signal snapped", jeton.position.is_equal_approx(Vector2(300, 100)) and aimante.n == 1, str(jeton.position))
	await _liberer(racine)


func _tester_seuil_et_plus_proche() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var loin := _noeud_2d(racine, Vector2(300, 100))
	var proche := _noeud_2d(racine, Vector2(340, 100))
	var montage := _monter(racine, jeton, {"release_behavior": DraggableComponent.RELEASE_LEAVE_IN_PLACE, "snap_distance_threshold": 30.0}, [loin, proche])
	_glisser(Vector2(100, 100), Vector2(325, 100))
	_verifier("Deux points dans le seuil : le plus proche est choisi", jeton.position.is_equal_approx(Vector2(340, 100)), str(jeton.position))
	_glisser(Vector2(340, 100), Vector2(300, 140))
	_verifier("Au-delà du seuil (40 > 30) : pas d'aimantage", jeton.position.is_equal_approx(Vector2(300, 140)), str(jeton.position))
	montage.composant.snap_distance_threshold = 40.0
	_glisser(Vector2(300, 140), Vector2(300, 140))
	_verifier("Exactement au seuil : aimanté", jeton.position.is_equal_approx(Vector2(300, 100)), str(jeton.position))
	await _liberer(racine)


func _tester_control_glisse() -> void:
	var racine := _racine()
	var carte := _carte(racine, Vector2(100, 100))
	var emplacement := _carte(racine, Vector2(300, 100))
	var point_2d := _noeud_2d(racine, Vector2(100, 300))
	var montage := _monter(racine, carte, {"release_behavior": DraggableComponent.RELEASE_SNAP_TO_VALID_POINT}, [emplacement, point_2d])
	var systeme: DragAndDropSystem = montage.systeme
	var aimante := _espionner(systeme.snapped)

	_souris(Vector2(110, 110))
	_bouton(Vector2(110, 110), true)
	_souris(Vector2(210, 160))
	_verifier("Control : glissé en conservant l'écart", carte.position.is_equal_approx(Vector2(200, 150)), str(carte.position))
	_souris(Vector2(305, 120))
	_bouton(Vector2(305, 120), false)
	_verifier("Control : posé sur l'emplacement (Control) proche", carte.position.is_equal_approx(Vector2(300, 100)) and aimante.args == [carte, emplacement],
		str(carte.position))
	_glisser(Vector2(310, 110), Vector2(110, 310))
	_verifier("Control : un Node2D n'est pas un point valide (autre repère) → retour",
		carte.position.is_equal_approx(Vector2(100, 100)), str(carte.position))
	await _liberer(racine)


func _tester_saisie_par_code() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var autre := _noeud_2d(racine, Vector2(300, 300))
	var montage := _monter(racine, jeton, {"release_behavior": DraggableComponent.RELEASE_RETURN_TO_INITIAL})
	var systeme: DragAndDropSystem = montage.systeme
	var composant_autre := _composant(autre)
	systeme.register_draggable(composant_autre)

	_souris(Vector2(120, 100))
	systeme.grab(montage.composant)
	_verifier("grab() par code : saisi", systeme.get_grabbed_component() == montage.composant)
	systeme.grab(composant_autre) # erreur attendue
	_verifier("grab() alors qu'un objet est saisi : refusé", systeme.get_grabbed_component() == montage.composant)
	systeme.grab(null) # erreur attendue
	_souris(Vector2(220, 150))
	_verifier("Saisie par code : l'objet suit le pointeur", jeton.position.is_equal_approx(Vector2(200, 150)), str(jeton.position))
	systeme.release()
	_verifier("release() par code : comportement appliqué", systeme.is_free() and jeton.position.is_equal_approx(Vector2(100, 100)))
	await _liberer(racine)


func _tester_cible_liberee() -> void:
	var racine := _racine()
	var jeton := _noeud_2d(racine, Vector2(100, 100))
	var montage := _monter(racine, jeton)
	var systeme: DragAndDropSystem = montage.systeme
	var relache := _espionner(systeme.released)
	_souris(Vector2(100, 100))
	_bouton(Vector2(100, 100), true)
	jeton.free()
	_souris(Vector2(150, 150))
	_verifier("Nœud saisi libéré : saisie abandonnée sans plantage", systeme.is_free() and relache.n == 0)
	_verifier("Nœud libéré : plus de cible ni de détection",
		montage.composant.target_node == null and systeme.get_draggable_at(Vector2(100, 100)) == null)
	_bouton(Vector2(150, 150), false)
	await _liberer(racine)


# 🧊 3D

func _tester_detection_3d() -> void:
	var racine := _racine()
	var camera := _camera(racine, Vector3(0, 0, 10), Vector3.ZERO)
	var cube := _noeud_3d(racine, Vector3.ZERO)
	var montage := _monter(racine, cube)
	var systeme: DragAndDropSystem = montage.systeme

	_verifier("Node3D : détecté sous le pointeur (rayon / boîte)", systeme.get_draggable_at(camera.unproject_position(Vector3.ZERO)) == montage.composant)
	_verifier("Node3D : rien à côté de la boîte", systeme.get_draggable_at(camera.unproject_position(Vector3(2, 0, 0))) == null)
	cube.scale = Vector3(5, 5, 5)
	_verifier("Node3D agrandi : la boîte suit l'échelle", systeme.get_draggable_at(camera.unproject_position(Vector3(2, 0, 0))) == montage.composant)
	cube.scale = Vector3.ONE
	montage.composant.bounds_3d = AABB(Vector3(1.5, -0.5, -0.5), Vector3.ONE)
	_verifier("bounds_3d décalée : détectée à son emplacement", systeme.get_draggable_at(camera.unproject_position(Vector3(2, 0, 0))) == montage.composant
		and systeme.get_draggable_at(camera.unproject_position(Vector3.ZERO)) == null)
	montage.composant.bounds_3d = AABB(Vector3(-0.5, -0.5, -0.5), Vector3.ONE)

	var devant := _noeud_3d(racine, Vector3(0, 0, 3))
	var composant_devant := _composant(devant)
	systeme.register_draggable(composant_devant)
	_verifier("Node3D superposés : le plus proche de la caméra l'emporte (malgré l'ordre)",
		systeme.get_draggable_at(camera.unproject_position(Vector3.ZERO)) == composant_devant)
	devant.visible = false
	_verifier("Node3D invisible : ignoré", systeme.get_draggable_at(camera.unproject_position(Vector3.ZERO)) == montage.composant)

	var jeton := _noeud_2d(racine, camera.unproject_position(Vector3.ZERO))
	var composant_2d := _composant(jeton)
	systeme.register_draggable(composant_2d)
	_verifier("2D et 3D superposés : le 2D l'emporte", systeme.get_draggable_at(camera.unproject_position(Vector3.ZERO)) == composant_2d)
	await _liberer(racine)


func _tester_glissement_3d_plan_camera() -> void:
	var racine := _racine()
	var camera := _camera(racine, Vector3(0, 0, 10), Vector3.ZERO)
	var cube := _noeud_3d(racine, Vector3.ZERO)
	_monter(racine, cube, {"release_behavior": DraggableComponent.RELEASE_DO_NOTHING, "snap_to_cursor": true, "drag_plane_3d": DraggableComponent.DRAG_PLANE_CAMERA})

	var depart := camera.unproject_position(Vector3.ZERO)
	_souris(depart)
	_bouton(depart, true)
	var arrivee := camera.unproject_position(Vector3(-2, 1, 0))
	_souris(arrivee)
	_verifier("3D plan caméra : l'objet suit le pointeur dans le plan face à la caméra",
		_proche(cube.position, Vector3(-2, 1, 0)), str(cube.position))
	_bouton(arrivee, false)
	await _liberer(racine)


func _tester_glissement_3d_sol() -> void:
	var racine := _racine()
	var camera := _camera(racine, Vector3(0, 6, 6), Vector3.ZERO)
	var cube := _noeud_3d(racine, Vector3.ZERO)
	_monter(racine, cube, {"release_behavior": DraggableComponent.RELEASE_DO_NOTHING, "drag_plane_3d": DraggableComponent.DRAG_PLANE_WORLD_Y})

	# Saisie à côté du centre : l'écart est conservé, l'objet ne saute pas sous le pointeur.
	# (Points à gauche de l'écran : dans la démo, le panneau de droite intercepte les clics.)
	var saisie := camera.unproject_position(Vector3(-0.3, 0, 0.2))
	_souris(saisie)
	_bouton(saisie, true)
	_souris(saisie)
	_verifier("3D sol : pas de saut à la saisie (écart conservé)", _proche(cube.position, Vector3.ZERO), str(cube.position))
	var arrivee := camera.unproject_position(Vector3(-2.3, 0, 1.2))
	_souris(arrivee)
	_verifier("3D sol (World Y) : glissé à plat, écart conservé", _proche(cube.position, Vector3(-2, 0, 1)), str(cube.position))
	_bouton(arrivee, false)
	await _liberer(racine)


func _tester_aimantage_3d() -> void:
	var racine := _racine()
	var camera := _camera(racine, Vector3(0, 6, 6), Vector3.ZERO)
	var cube := _noeud_3d(racine, Vector3.ZERO)
	var socle := _noeud_3d(racine, Vector3(-3, 0, 0))
	var systeme: DragAndDropSystem = _monter(racine, cube, {"drag_plane_3d": DraggableComponent.DRAG_PLANE_WORLD_Y, "snap_to_cursor": true, "snap_distance_threshold": 0.5}, [socle]).systeme
	var aimante := _espionner(systeme.snapped)
	_glisser(camera.unproject_position(Vector3.ZERO), camera.unproject_position(Vector3(-2.8, 0, 0.1)))
	_verifier("3D : posé sur le point valide proche (seuil en unités)", cube.position.is_equal_approx(Vector3(-3, 0, 0)) and aimante.n == 1, str(cube.position))
	_glisser(camera.unproject_position(Vector3(-3, 0, 0)), camera.unproject_position(Vector3(-1.5, 0, 1)))
	_verifier("3D : loin du point → retour à la position initiale", cube.position.is_equal_approx(Vector3.ZERO), str(cube.position))
	await _liberer(racine)


# 🛠️ Montage

## Nœud racine d'un test, libéré à la fin du test (_liberer).
func _racine() -> Node:
	return _ajouter(Node.new())


func _liberer(racine: Node) -> void:
	racine.queue_free()
	await _frames()


func _noeud_2d(parent: Node, position: Vector2, nom: String = "") -> Node2D:
	var noeud := Node2D.new()
	noeud.position = position
	if nom:
		noeud.name = nom
	return _ajouter(noeud, parent)


func _noeud_3d(parent: Node, position: Vector3) -> Node3D:
	var noeud := Node3D.new()
	noeud.position = position
	return _ajouter(noeud, parent)


## Control de 80 × 100 qui laisse passer la souris (comme doit l'être un Control déplaçable).
func _carte(parent: Node, position: Vector2) -> Control:
	var carte := Control.new()
	carte.position = position
	carte.size = Vector2(80, 100)
	carte.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return _ajouter(carte, parent)


func _camera(parent: Node, position: Vector3, cible: Vector3) -> Camera3D:
	var camera := Camera3D.new()
	_ajouter(camera, parent)
	camera.look_at_from_position(position, cible)
	camera.make_current()
	return camera


func _composant(cible: Node, reglages: Dictionary = {}) -> DraggableComponent:
	var composant := DraggableComponent.new()
	composant.target_node = cible
	for propriete in reglages:
		composant.set(propriete, reglages[propriete])
	return composant


## Système ajouté sous p_racine, puis composant de p_cible enregistré (position initiale mémorisée).
## Les points d'aimantage sont convertis en chemins relatifs au système.
func _monter(racine: Node, cible: Node, reglages: Dictionary = {}, points: Array = []) -> Dictionary:
	var systeme := DragAndDropSystem.new()
	_ajouter(systeme, racine)
	var composant := _composant(cible, reglages)
	var chemins: Array[NodePath] = []
	for point in points:
		chemins.append(systeme.get_path_to(point))
	composant.snap_targets = chemins
	systeme.register_draggable(composant)
	return {"systeme": systeme, "composant": composant}


## Égalité à p_tolerance près (projection écran ↔ monde : arrondis au-delà de is_equal_approx).
func _proche(a: Vector3, b: Vector3, tolerance: float = 0.01) -> bool:
	return a.distance_to(b) < tolerance


# 🖱️ Souris simulée (coordonnées du Viewport)

func _souris(position: Vector2) -> void:
	var evenement := InputEventMouseMotion.new()
	evenement.position = position
	evenement.global_position = position
	_bac_a_sable.get_viewport().push_input(evenement, true)


func _bouton(position: Vector2, presse: bool, bouton: MouseButton = MOUSE_BUTTON_LEFT) -> void:
	var evenement := InputEventMouseButton.new()
	evenement.position = position
	evenement.global_position = position
	evenement.button_index = bouton
	evenement.pressed = presse
	_bac_a_sable.get_viewport().push_input(evenement, true)


## Survol, clic en p_de, glissement jusqu'à p_a, relâchement.
func _glisser(de: Vector2, a: Vector2) -> void:
	_souris(de)
	_bouton(de, true)
	_souris(a)
	_bouton(a, false)
