extends "../../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques de BehaviorGraphView : cible en direct (HFSM et arbre), dispositions
## TechTree / vertical / MindMap, repli des branches, souris simulée (sélection, menu contextuel,
## glisser-déposer, Maj + glisser), instantané fourni à distance.

const EtatJournal := preload("etat_journal.gd")


func _executer() -> void:
	await _tester_cible_hfsm()
	await _tester_dispositions()
	await _tester_repli()
	await _tester_souris()
	await _tester_instantane_distant()
	await _tester_arbre_en_direct()


# 🧱 Construction

func _machine() -> StateMachine:
	var agent := Node.new()
	agent.name = "Agent"
	var machine := StateMachine.new()
	machine.name = "Machine"
	machine.autostart = false
	machine.process_callback = StateMachine.PROCESS_MANUAL
	agent.add_child(machine)
	var etats := {}
	for definition in [["Repos", machine], ["Mouvement", machine], ["Marche", "Mouvement"], ["Course", "Mouvement"], ["Saut", "Mouvement"], ["Mort", machine]]:
		var etat := State.new()
		etat.name = definition[0]
		(definition[1] if definition[1] is Node else etats[definition[1]]).add_child(etat)
		etats[definition[0]] = etat
	var transition := StateTransition.new()
	transition.target_state = NodePath("../Mouvement")
	transition.event = &"bouger"
	etats.Repos.transitions = [transition]
	_ajouter(agent)
	machine.start()
	return machine


func _vue(cible: Node = null) -> BehaviorGraphView:
	var vue := BehaviorGraphView.new()
	vue.position = Vector2.ZERO
	vue.size = Vector2(900, 600)
	_ajouter(vue)
	vue.set_target(cible)
	return vue


func _centre(vue: BehaviorGraphView, id: String) -> Vector2:
	return vue.get_node_rect(id).get_center()


# 🎯 Cible

func _tester_cible_hfsm() -> void:
	var machine := _machine()
	# Un état quelconque de la machine désigne toute la machine.
	var vue := _vue(machine.get_node("Mouvement/Course"))
	await _frames(1)
	_verifier("Cible : résolue vers la StateMachine racine", vue.get_target() == machine)
	var ids := vue.get_node_ids()
	_verifier("Cible : tous les états affichés", ids.size() == 7 and "." in ids and "Mouvement/Course" in ids, str(ids))
	_verifier("Cible : racine de type machine", vue.get_node_data(".").kind == "machine")
	_verifier("Cible : état actif signalé", vue.get_node_data("Repos").active and not vue.get_node_data("Mort").active)
	_verifier("Cible : vue cadrée (zoom borné)", vue.get_zoom() > 0.1 and vue.get_zoom() <= 1.25, str(vue.get_zoom()))

	machine.send_event(&"bouger")
	await _secondes(vue.refresh_interval + 0.05)
	_verifier("En direct : l'instantané suit les transitions", vue.get_node_data("Mouvement/Marche").active and not vue.get_node_data("Repos").active)
	_verifier("Placeholder : vide sans cible", _vue().get_node_ids().is_empty())


func _tester_dispositions() -> void:
	var vue := _vue(_machine())
	await _frames(1)
	vue.layout_mode = BehaviorGraphView.LAYOUT_TECH_TREE
	var racine := vue.get_node_rect(".")
	var mouvement := vue.get_node_rect("Mouvement")
	var marche := vue.get_node_rect("Mouvement/Marche")
	var course := vue.get_node_rect("Mouvement/Course")
	_verifier("TechTree : les enfants sont à droite du parent", mouvement.position.x > racine.end.x and marche.position.x > mouvement.end.x)
	_verifier("TechTree : les frères sont empilés sans se chevaucher", not marche.intersects(course) and is_equal_approx(marche.position.x, course.position.x))
	_verifier("TechTree : parent centré sur ses enfants",
		absf(mouvement.get_center().y - vue.get_node_rect("Mouvement/Course").get_center().y) < 1.0, "%s / %s" % [mouvement, course])

	vue.layout_mode = BehaviorGraphView.LAYOUT_TOP_DOWN
	racine = vue.get_node_rect(".")
	mouvement = vue.get_node_rect("Mouvement")
	_verifier("Vertical : les enfants sont sous le parent", mouvement.position.y > racine.end.y)

	vue.layout_mode = BehaviorGraphView.LAYOUT_MIND_MAP
	racine = vue.get_node_rect(".")
	var droite := 0
	var gauche := 0
	for id in ["Repos", "Mouvement", "Mort"]:
		var rect := vue.get_node_rect(id)
		if rect.position.x > racine.end.x:
			droite += 1
		elif rect.end.x < racine.position.x:
			gauche += 1
	_verifier("MindMap : branches réparties des deux côtés de la racine", droite >= 1 and gauche >= 1, "droite %d, gauche %d" % [droite, gauche])
	var tous_dedans := true
	for id in vue.get_node_ids():
		tous_dedans = tous_dedans and Rect2(Vector2.ZERO, vue.size).encloses(vue.get_node_rect(id))
	_verifier("Changer de disposition recadre tout le graphe", tous_dedans)


func _tester_repli() -> void:
	var vue := _vue(_machine())
	await _frames(1)
	vue.toggle_collapsed("Mouvement")
	_verifier("Repli : sous-états masqués", vue.get_node_rect("Mouvement/Marche") == Rect2() and vue.is_collapsed("Mouvement"))
	_verifier("Repli : le nœud replié reste visible", vue.get_node_rect("Mouvement").size.x > 0)
	vue.toggle_collapsed("Mouvement")
	_verifier("Dépli : sous-états visibles", vue.get_node_rect("Mouvement/Marche").size.x > 0)


# 🖱️ Souris simulée

func _clic(vue: BehaviorGraphView, position: Vector2, bouton := MOUSE_BUTTON_LEFT, presse := true, maj := false) -> void:
	var evenement := InputEventMouseButton.new()
	evenement.button_index = bouton
	evenement.pressed = presse
	evenement.shift_pressed = maj
	evenement.position = position
	evenement.global_position = position
	vue.get_viewport().push_input(evenement, true)


func _deplacer(vue: BehaviorGraphView, position: Vector2, relatif: Vector2) -> void:
	var evenement := InputEventMouseMotion.new()
	evenement.position = position
	evenement.global_position = position
	evenement.relative = relatif
	evenement.button_mask = MOUSE_BUTTON_MASK_LEFT
	vue.get_viewport().push_input(evenement, true)


func _glisser(vue: BehaviorGraphView, depart: Vector2, arrivee: Vector2, maj := false) -> void:
	_clic(vue, depart, MOUSE_BUTTON_LEFT, true, maj)
	await _frames(1)
	_deplacer(vue, depart.lerp(arrivee, 0.5), (arrivee - depart) * 0.5)
	_deplacer(vue, arrivee, (arrivee - depart) * 0.5)
	await _frames(1)
	_clic(vue, arrivee, MOUSE_BUTTON_LEFT, false, maj)
	await _frames(1)


func _tester_souris() -> void:
	var vue := _vue(_machine())
	await _frames(2)
	var selections := _espionner(vue.node_selected)
	var menus := _espionner(vue.node_context_menu_requested)
	var deposes := _espionner(vue.node_dropped)
	var liens := _espionner(vue.link_requested)

	var centre_mort := _centre(vue, "Mort")
	_verifier("get_node_at : nœud sous un point", vue.get_node_at(centre_mort) == "Mort")
	_clic(vue, centre_mort)
	_clic(vue, centre_mort, MOUSE_BUTTON_LEFT, false)
	await _frames(1)
	_verifier("Clic : node_selected", selections.n >= 1 and selections.args[0] == "Mort" and vue.get_selected_id() == "Mort", str(selections))

	_clic(vue, _centre(vue, "Repos"), MOUSE_BUTTON_RIGHT)
	await _frames(1)
	_verifier("Clic droit : menu contextuel demandé pour le nœud", menus.n == 1 and menus.args[0] == "Repos", str(menus))

	vue.editable = false
	await _glisser(vue, _centre(vue, "Mort"), _centre(vue, "Mouvement"))
	_verifier("Non éditable : glisser un nœud ne demande rien", deposes.n == 0)

	vue.editable = true
	await _glisser(vue, _centre(vue, "Mort"), _centre(vue, "Mouvement"))
	_verifier("Glisser-déposer : node_dropped (nœud, nouveau parent)", deposes.n == 1 and deposes.args == ["Mort", "Mouvement"], str(deposes))
	await _glisser(vue, _centre(vue, "Repos"), _centre(vue, "Mort"), true)
	_verifier("Maj + glisser : link_requested", liens.n == 1 and liens.args == ["Repos", "Mort"], str(liens))

	var decalage := vue.get_view_offset()
	var vide := Vector2(vue.size.x - 5, 5)
	if vue.get_node_at(vide) == "":
		await _glisser(vue, vide, vide + Vector2(-40, 30))
		_verifier("Glisser le fond : déplace la vue", vue.get_view_offset().distance_to(decalage + Vector2(-40, 30)) < 1.0, str(vue.get_view_offset() - decalage))

	var zoom := vue.get_zoom()
	var molette := InputEventMouseButton.new()
	molette.button_index = MOUSE_BUTTON_WHEEL_UP
	molette.pressed = true
	molette.position = vue.size / 2
	molette.global_position = vue.size / 2
	vue.get_viewport().push_input(molette, true)
	await _frames(1)
	_verifier("Molette : zoom avant", vue.get_zoom() > zoom)


# 🛰️ Instantané à distance

func _tester_instantane_distant() -> void:
	var vue := _vue()
	vue.live_update = false
	vue.set_snapshot({
		"type": "bt", "name": "Distant", "running": true, "time": 3.0,
		"nodes": [
			{"id": ".", "parent": "", "name": "Arbre", "class": "BehaviorTree", "kind": "tree", "status": 2},
			{"id": "Choix", "parent": ".", "name": "Choix", "class": "BTSelector", "kind": "composite", "status": 2, "active": true},
			{"id": "Choix/Fuir", "parent": "Choix", "name": "Fuir", "class": "BTAction", "kind": "action", "status": 1},
			{"id": "Choix/Errer", "parent": "Choix", "name": "Errer", "class": "BTAction", "kind": "action", "status": 2, "active": true},
		],
	})
	await _frames(1)
	_verifier("Distant : instantané affiché tel quel", vue.get_node_ids().size() == 4 and vue.get_node_data("Choix/Errer").status == 2)
	_verifier("Distant : disposition calculée", vue.get_node_rect("Choix/Errer").position.x > vue.get_node_rect("Choix").end.x)
	_verifier("Distant : infobulle", "Errer" in vue.get_tooltip(_centre(vue, "Choix/Errer")))
	vue.clear()
	_verifier("clear() : vue vidée", vue.get_node_ids().is_empty())


func _tester_arbre_en_direct() -> void:
	var agent := Node.new()
	var arbre := BehaviorTree.new()
	arbre.process_callback = BehaviorTree.PROCESS_MANUAL
	agent.add_child(arbre)
	var sequence := BTSequence.new()
	sequence.name = "Suite"
	var attente := BTWait.new()
	attente.name = "Attendre"
	sequence.add_child(attente)
	arbre.add_child(sequence)
	_ajouter(agent)
	var vue := _vue(attente)
	vue.refresh_interval = 0.0
	arbre.tick(0.1)
	await _frames(2)
	_verifier("Arbre : cible résolue vers le BehaviorTree", vue.get_target() == arbre)
	_verifier("Arbre : statut RUNNING de l'attente", vue.get_node_data("Suite/Attendre").status == BTNode.RUNNING)
	_verifier("Arbre : détail de l'attente (temps écoulé)", "/" in str(vue.get_node_data("Suite/Attendre").info), str(vue.get_node_data("Suite/Attendre").info))
	arbre.queue_free()
	await _frames(2)
	_verifier("Cible libérée : la vue se vide", vue.get_node_ids().is_empty() and vue.get_target() == null)
