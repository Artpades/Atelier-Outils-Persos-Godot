extends "../../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques de l'arbre de comportement : BehaviorTree, composites (séquence, sélecteur,
## parallèle), décorateurs (inverseur, résultat forcé, répéteur, recharge, limite de temps) et feuilles
## (action par script, condition, attente, écriture du tableau noir, appel de méthode, événement HFSM).
## Les arbres sont exécutés à la main (PROCESS_MANUAL, tick(delta)) sauf le dernier test.
##
## Avertissement attendu dans la console : expression de condition invalide.

const ActionTest := preload("action_test.gd")

# Valeurs de BTNode.Status (une constante d'une classe d'extension n'est pas une expression constante).
const S := 0 # SUCCESS
const F := 1 # FAILURE
const R := 2 # RUNNING


func _executer() -> void:
	_verifier("Statuts : SUCCESS, FAILURE, RUNNING = 0, 1, 2", BTNode.SUCCESS == S and BTNode.FAILURE == F and BTNode.RUNNING == R)
	_tester_sequence()
	_tester_sequence_en_cours()
	_tester_sequence_reactive()
	_tester_selecteur()
	_tester_ordre_aleatoire()
	_tester_parallele()
	_tester_inverseur_et_resultat_force()
	_tester_repeteur()
	_tester_recharge()
	_tester_limite_de_temps()
	_tester_attente()
	_tester_condition()
	_tester_ecriture_tableau_noir()
	_tester_appel_de_methode()
	_tester_evenement_hfsm()
	_tester_interruption_et_signaux()
	_tester_instantane()
	await _tester_traitement_automatique()


# 🧱 Construction

## Arbre manuel sous un agent, avec p_racine comme racine.
func _arbre(racine: BTNode) -> BehaviorTree:
	var agent := Node.new()
	agent.name = "Agent"
	agent.set_meta("agent_test", true) # le nom peut changer : plusieurs « Agent » dans le bac à sable
	var arbre := BehaviorTree.new()
	arbre.name = "Arbre"
	arbre.process_callback = BehaviorTree.PROCESS_MANUAL
	agent.add_child(arbre)
	arbre.add_child(racine)
	_ajouter(agent)
	return arbre


## Action de test qui renvoie p_resultats dans l'ordre.
func _action(resultats: Array[int], nom := "Action") -> BTAction:
	var action = BTAction.new()
	action.set_script(ActionTest)
	action.resultats = resultats
	action.name = nom
	return action


func _avec(parent: BTNode, enfants: Array) -> BTNode:
	for enfant in enfants:
		parent.add_child(enfant)
	return parent


func _ticks(arbre: BehaviorTree, nombre: int, delta := 0.1) -> Array:
	var statuts := []
	for i in nombre:
		statuts.append(arbre.tick(delta))
	return statuts


# 🔀 Composites

func _tester_sequence() -> void:
	var a := _action([S], "A")
	var b := _action([S], "B")
	var arbre := _arbre(_avec(BTSequence.new(), [a, b]))
	_verifier("Séquence : tous réussissent → SUCCESS", arbre.tick(0.1) == S)
	_verifier("Séquence : chaque enfant exécuté une fois", a.ticks == 1 and b.ticks == 1)

	var c := _action([S], "C")
	var d := _action([F], "D")
	var e := _action([S], "E")
	arbre = _arbre(_avec(BTSequence.new(), [c, d, e]))
	_verifier("Séquence : un échec → FAILURE", arbre.tick(0.1) == F)
	_verifier("Séquence : les suivants ne sont pas exécutés", e.ticks == 0)


func _tester_sequence_en_cours() -> void:
	var a := _action([S], "A")
	var b := _action([R, R, S], "B")
	var arbre := _arbre(_avec(BTSequence.new(), [a, b]))
	_verifier("Séquence : RUNNING tant que l'enfant tourne", _ticks(arbre, 3) == [R, R, S])
	_verifier("Séquence : reprend à l'enfant en cours (sans réexécuter les précédents)", a.ticks == 1, str(a.ticks))
	_verifier("Action : _on_enter / _on_exit une fois par exécution", b.entrees == 1 and b.sorties == 1)


func _tester_sequence_reactive() -> void:
	var garde := _action([S, S, F], "Garde")
	var tache := _action([R], "Tache")
	var sequence := BTSequence.new()
	sequence.reactive = true
	var arbre := _arbre(_avec(sequence, [garde, tache]))
	_verifier("Séquence réactive : réévalue le premier enfant à chaque tick", _ticks(arbre, 3) == [R, R, F])
	_verifier("Séquence réactive : garde exécutée à chaque tick", garde.ticks == 3)
	_verifier("Séquence réactive : la tâche en cours est interrompue", tache.sorties == 1 and not tache.is_running())


func _tester_selecteur() -> void:
	var a := _action([F], "A")
	var b := _action([S], "B")
	var c := _action([S], "C")
	var arbre := _arbre(_avec(BTSelector.new(), [a, b, c]))
	_verifier("Sélecteur : premier succès → SUCCESS", arbre.tick(0.1) == S)
	_verifier("Sélecteur : s'arrête au premier succès", c.ticks == 0)
	arbre = _arbre(_avec(BTSelector.new(), [_action([F]), _action([F])]))
	_verifier("Sélecteur : tous échouent → FAILURE", arbre.tick(0.1) == F)

	# Sélecteur réactif : une branche prioritaire qui redevient possible interrompt la branche en cours.
	var prioritaire := _action([F, S], "Prioritaire")
	var secours := _action([R], "Secours")
	var selecteur := BTSelector.new()
	selecteur.reactive = true
	arbre = _arbre(_avec(selecteur, [prioritaire, secours]))
	_verifier("Sélecteur réactif : branche prioritaire reprise", _ticks(arbre, 2) == [R, S])
	_verifier("Sélecteur réactif : branche de secours interrompue", secours.sorties == 1)


func _tester_ordre_aleatoire() -> void:
	var enfants: Array = []
	for i in 5:
		enfants.append(_action([S], "A%d" % i))
	var sequence := BTSequence.new()
	sequence.random_order = true
	var arbre := _arbre(_avec(sequence, enfants))
	_verifier("Ordre aléatoire : SUCCESS", arbre.tick(0.1) == S)
	_verifier("Ordre aléatoire : chaque enfant exécuté une fois", enfants.all(func(a): return a.ticks == 1))


func _tester_parallele() -> void:
	var a := _action([S], "A")
	var b := _action([R, S], "B")
	var arbre := _arbre(_avec(BTParallel.new(), [a, b]))
	_verifier("Parallèle (tous) : RUNNING puis SUCCESS", _ticks(arbre, 2) == [R, S])
	_verifier("Parallèle : un enfant terminé n'est pas réexécuté", a.ticks == 1)

	var echec := _action([F], "Echec")
	var long := _action([R], "Long")
	arbre = _arbre(_avec(BTParallel.new(), [echec, long]))
	_verifier("Parallèle : un échec (seuil 1) → FAILURE", arbre.tick(0.1) == F)
	_verifier("Parallèle : les enfants en cours sont interrompus", long.sorties == 1)

	var rapide := _action([S], "Rapide")
	var lent := _action([R], "Lent")
	var parallele := BTParallel.new()
	parallele.success_threshold = 1
	arbre = _arbre(_avec(parallele, [lent, rapide]))
	_verifier("Parallèle : seuil de réussite 1 → SUCCESS", arbre.tick(0.1) == S and lent.sorties == 1)


# ◇ Décorateurs

func _tester_inverseur_et_resultat_force() -> void:
	_verifier("Inverseur : SUCCESS → FAILURE", _arbre(_avec(BTInverter.new(), [_action([S])])).tick(0.1) == F)
	_verifier("Inverseur : FAILURE → SUCCESS", _arbre(_avec(BTInverter.new(), [_action([F])])).tick(0.1) == S)
	_verifier("Inverseur : RUNNING inchangé", _arbre(_avec(BTInverter.new(), [_action([R])])).tick(0.1) == R)
	var force := BTForceResult.new()
	force.result = BTNode.FAILURE
	_verifier("Résultat forcé : FAILURE", _arbre(_avec(force, [_action([S])])).tick(0.1) == F)
	_verifier("Décorateur sans enfant : FAILURE", _arbre(BTInverter.new()).tick(0.1) == F)


func _tester_repeteur() -> void:
	var action := _action([S])
	var repeteur := BTRepeater.new()
	repeteur.count = 3
	var arbre := _arbre(_avec(repeteur, [action]))
	_verifier("Répéteur (3) : RUNNING, RUNNING, SUCCESS", _ticks(arbre, 3) == [R, R, S])
	_verifier("Répéteur : une exécution de l'enfant par tick", action.ticks == 3)

	var jusqu_a_echec := BTRepeater.new()
	jusqu_a_echec.mode = BTRepeater.REPEAT_UNTIL_FAILURE
	arbre = _arbre(_avec(jusqu_a_echec, [_action([S, S, F])]))
	_verifier("Répéteur jusqu'à échec : SUCCESS au premier échec", _ticks(arbre, 3) == [R, R, S])

	var jusqu_a_reussite := BTRepeater.new()
	jusqu_a_reussite.mode = BTRepeater.REPEAT_UNTIL_SUCCESS
	jusqu_a_reussite.count = 2
	arbre = _arbre(_avec(jusqu_a_reussite, [_action([F])]))
	_verifier("Répéteur jusqu'à réussite (2 essais) : FAILURE si jamais réussi", _ticks(arbre, 2) == [R, F])


func _tester_recharge() -> void:
	var action := _action([S])
	var recharge := BTCooldown.new()
	recharge.duration = 1.0
	var arbre := _arbre(_avec(recharge, [action]))
	_verifier("Recharge : première exécution", arbre.tick(0.1) == S)
	_verifier("Recharge : FAILURE sans exécuter l'enfant pendant la recharge", arbre.tick(0.5) == F and action.ticks == 1)
	_verifier("Recharge : temps restant", is_equal_approx(recharge.get_remaining(), 0.5), str(recharge.get_remaining()))
	_verifier("Recharge : de nouveau disponible", arbre.tick(0.6) == S and action.ticks == 2)


func _tester_limite_de_temps() -> void:
	var action := _action([R])
	var limite := BTTimeLimit.new()
	limite.duration = 0.5
	var arbre := _arbre(_avec(limite, [action]))
	_verifier("Limite de temps : RUNNING avant la limite", arbre.tick(0.3) == R)
	_verifier("Limite de temps : FAILURE après, enfant interrompu", arbre.tick(0.3) == F and action.sorties == 1)


# 🍃 Feuilles

func _tester_attente() -> void:
	var attente := BTWait.new()
	attente.duration = 1.0
	var arbre := _arbre(attente)
	_verifier("Attente 1 s par pas de 0,25 s", _ticks(arbre, 4, 0.25) == [R, R, R, S])
	_verifier("Attente : relancée à zéro", arbre.tick(0.25) == R)


func _tester_condition() -> void:
	var condition := BTCondition.new()
	condition.expression = "vie < 20 and agent.has_meta('agent_test')"
	var arbre := _arbre(condition)
	arbre.blackboard = Blackboard.new()
	arbre.blackboard.set_value("vie", 10)
	_verifier("Condition vraie → SUCCESS", arbre.tick(0.1) == S)
	arbre.blackboard.set_value("vie", 50)
	_verifier("Condition fausse → FAILURE", arbre.tick(0.1) == F)
	condition.expression = "vie <"
	_verifier("Condition invalide → FAILURE avec erreur", arbre.tick(0.1) == F and condition.get_error() != "")


func _tester_ecriture_tableau_noir() -> void:
	var ecriture := BTSetValue.new()
	ecriture.key = "double"
	ecriture.value_expression = "vie * 2"
	var arbre := _arbre(ecriture)
	arbre.blackboard = Blackboard.new()
	arbre.blackboard.set_value("vie", 21)
	_verifier("Écriture : SUCCESS", arbre.tick(0.1) == S)
	_verifier("Écriture : valeur calculée", arbre.blackboard.get_value("double") == 42)


func _tester_appel_de_methode() -> void:
	var appel := BTCallMethod.new()
	appel.method = &"get_child_count"
	appel.result_key = "enfants"
	var arbre := _arbre(appel)
	_verifier("Appel de méthode sur l'agent : SUCCESS et résultat rangé", arbre.tick(0.1) == S and arbre.blackboard.get_value("enfants") == 1)
	appel.method = &"is_inside_tree"
	appel.use_result_as_status = true
	_verifier("Appel : résultat booléen utilisé comme statut", arbre.tick(0.1) == S)
	appel.method = &"methode_inexistante"
	_verifier("Appel : méthode introuvable → FAILURE", arbre.tick(0.1) == F)


func _tester_evenement_hfsm() -> void:
	var evenement := BTSendEvent.new()
	evenement.event = &"bouger"
	var arbre := _arbre(evenement)
	_verifier("Événement HFSM : sans machine → FAILURE", arbre.tick(0.1) == F)

	var machine := StateMachine.new()
	machine.autostart = false
	machine.process_callback = StateMachine.PROCESS_MANUAL
	var repos := State.new()
	repos.name = "Repos"
	var mouvement := State.new()
	mouvement.name = "Mouvement"
	machine.add_child(repos)
	machine.add_child(mouvement)
	var transition := StateTransition.new()
	transition.event = &"bouger"
	transition.target_state = NodePath("../Mouvement")
	repos.transitions = [transition]
	arbre.get_parent().add_child(machine)
	machine.start()
	_verifier("Événement HFSM : envoyé à la machine de l'agent", arbre.tick(0.1) == S and machine.get_active_state() == mouvement)
	evenement.require_consumed = true
	_verifier("Événement HFSM : non consommé → FAILURE si require_consumed", arbre.tick(0.1) == F)


func _tester_interruption_et_signaux() -> void:
	var action := _action([R])
	var arbre := _arbre(_avec(BTSequence.new(), [action]))
	var fins := _espionner(arbre.finished)
	arbre.tick(0.1)
	arbre.interrupt()
	_verifier("Interruption : _on_exit appelé, plus rien en cours", action.sorties == 1 and not action.is_running())
	arbre.tick(0.1)
	_verifier("Interruption : reprise depuis le début", action.entrees == 2)
	_verifier("finished : pas émis tant que RUNNING", fins.n == 0)
	action.resultats.assign([S])
	arbre.tick(0.1)
	_verifier("finished : émis avec le statut", fins.n == 1 and fins.args[0] == S)


func _tester_instantane() -> void:
	var garde := BTCondition.new()
	garde.name = "Garde"
	garde.expression = "false"
	var action := _action([S], "Agir")
	var selecteur := _avec(BTSelector.new(), [_avec(BTSequence.new(), [garde, _action([S], "Jamais")]), action])
	selecteur.name = "Choix"
	var arbre := _arbre(selecteur)
	arbre.tick(0.1)
	var instantane: Dictionary = arbre.get_debug_snapshot()
	var noeuds := {}
	for noeud in instantane.nodes:
		noeuds[noeud.name] = noeud
	_verifier("Instantané : type et nombre de nœuds", instantane.type == "bt" and instantane.nodes.size() == 6, str(instantane.nodes.size()))
	_verifier("Instantané : racine « . » de type tree", instantane.nodes[0].id == "." and instantane.nodes[0].kind == "tree")
	_verifier("Instantané : catégories", noeuds["Choix"].kind == "composite" and noeuds["Garde"].kind == "condition" and noeuds["Agir"].kind == "action")
	_verifier("Instantané : statuts du dernier tick", noeuds["Garde"].status == F and noeuds["Agir"].status == S and noeuds["Choix"].status == S)
	_verifier("Instantané : nœud non exécuté (jamais)", noeuds["Jamais"].status == -1)
	garde.expression = "true"
	arbre.tick(0.1)
	instantane = arbre.get_debug_snapshot()
	for noeud in instantane.nodes:
		noeuds[noeud.name] = noeud
	_verifier("Instantané : statut ancien marqué « non frais »", not noeuds["Agir"].fresh and noeuds["Jamais"].fresh)


func _tester_traitement_automatique() -> void:
	var action := _action([R])
	var arbre := _arbre(action)
	arbre.process_callback = BehaviorTree.PROCESS_IDLE
	await _frames(5)
	_verifier("PROCESS_IDLE : un tick par image", arbre.get_tick_count() >= 4, str(arbre.get_tick_count()))
	var avant := arbre.get_tick_count()
	arbre.tick_interval = 10.0
	await _frames(5)
	_verifier("tick_interval : ticks espacés", arbre.get_tick_count() == avant, str(arbre.get_tick_count() - avant))
	arbre.active = false
	_verifier("active = false : branche interrompue", action.sorties == 1)
