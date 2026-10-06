extends "../../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques de la machine à états hiérarchique : StateMachine, State, StateTransition,
## Blackboard.
##
## Machine de test (mise à jour manuelle, PROCESS_MANUAL) :
##   Agent
##   └ Machine
##     ├ Repos
##     ├ Mouvement  (composé)
##     │ ├ Marche
##     │ └ Course
##     └ Mort
##
## Avertissement attendu dans la console : expression de condition invalide.

const EtatJournal := preload("etat_journal.gd")


func _executer() -> void:
	_tester_demarrage()
	_tester_etat_initial()
	_tester_evenements()
	_tester_remontee_des_evenements()
	_tester_consommation()
	_tester_condition_tableau_noir()
	_tester_duree_minimale()
	_tester_memoire_sous_etat()
	_tester_transition_vers_soi()
	_tester_transition_en_chaine()
	_tester_transition_scriptee()
	_tester_arret()
	_tester_historique_et_instantane()
	_tester_agent_et_expression_invalide()
	await _tester_demarrage_automatique()
	await _tester_traitement_automatique()


# 🧱 Construction

## Crée la machine de test et renvoie { agent, machine, journal, <nom d'état>... }.
func _machine(demarrer := true) -> Dictionary:
	var agent := Node.new()
	agent.name = "Agent"
	agent.set_meta("agent_test", true) # le nom peut changer : plusieurs « Agent » dans le bac à sable
	var machine := StateMachine.new()
	machine.name = "Machine"
	machine.autostart = false
	machine.process_callback = StateMachine.PROCESS_MANUAL
	agent.add_child(machine)
	var journal: Array = []
	var etats := {"agent": agent, "machine": machine, "journal": journal}
	for definition in [["Repos", machine], ["Mouvement", machine], ["Marche", "Mouvement"], ["Course", "Mouvement"], ["Mort", machine]]:
		var etat = State.new() # non typé : les propriétés du script (journal...) sont ajoutées par set_script
		etat.set_script(EtatJournal)
		etat.name = definition[0]
		etat.journal = journal
		var parent: Node = definition[1] if definition[1] is Node else etats[definition[1]]
		parent.add_child(etat)
		etats[definition[0]] = etat
	_ajouter(agent)
	if demarrer:
		machine.start()
	return etats


func _transition(source: State, cible: String, evenement: StringName = &"", condition := "", duree_min := 0.0) -> StateTransition:
	var transition := StateTransition.new()
	transition.target_state = NodePath(cible)
	transition.event = evenement
	transition.condition = condition
	transition.min_time_in_state = duree_min
	var liste := source.transitions.duplicate()
	liste.append(transition)
	source.transitions = liste
	return transition


func _feuille(m: Dictionary) -> String:
	var feuille: State = m.machine.get_active_state()
	return feuille.name if feuille else "—"


# ▶️ Démarrage

func _tester_demarrage() -> void:
	var m := _machine()
	_verifier("Démarrage : la machine tourne", m.machine.is_running())
	_verifier("Démarrage : premier enfant actif par défaut", _feuille(m) == "Repos", _feuille(m))
	var chemin: Array = m.machine.get_active_path().map(func(e): return str(e.name))
	_verifier("Démarrage : chemin actif racine → feuille", chemin == ["Machine", "Repos"], str(chemin))
	_verifier("Démarrage : _on_enter appelé", m.journal == ["+Repos"], str(m.journal))
	_verifier("Démarrage : tableau noir créé", m.machine.blackboard != null)
	_verifier("Démarrage : un état composé n'est pas actif", not m.Mouvement.is_active())


func _tester_etat_initial() -> void:
	var m := _machine()
	m.Mouvement.initial_state = NodePath("Course")
	m.journal.clear()
	_verifier("État initial : transition_to accepte un chemin", m.machine.transition_to("Mouvement"))
	_verifier("État initial : initial_state choisit le sous-état", _feuille(m) == "Course", _feuille(m))
	_verifier("État initial : ordre sortie / entrées", m.journal == ["-Repos", "+Mouvement", "+Course"], str(m.journal))
	_verifier("État initial : get_initial_substate", m.Mouvement.get_initial_substate() == m.Course)
	var etranger := Node.new()
	_verifier("État initial : transition_to refuse la racine et un nœud étranger",
		not m.machine.transition_to(m.machine) and not m.machine.transition_to(etranger))
	etranger.free()


# ⚡ Événements

func _tester_evenements() -> void:
	var m := _machine()
	_transition(m.Repos, "../Mouvement", &"bouger")
	var changements := _espionner(m.machine.state_changed)
	_verifier("Événement : inconnu non consommé", not m.machine.send_event(&"inconnu"))
	_verifier("Événement : déclenche la transition", m.machine.send_event(&"bouger"))
	_verifier("Événement : feuille = premier sous-état de la cible", _feuille(m) == "Marche", _feuille(m))
	_verifier("Événement : state_changed (Repos → Marche)", changements.n == 1 and changements.args[0] == m.Repos and changements.args[1] == m.Marche)
	_verifier("Événement : State.send_event passe par la machine", m.Marche.send_event(&"inconnu") == false)


func _tester_remontee_des_evenements() -> void:
	var m := _machine()
	_transition(m.Mouvement, "../Mort", &"mourir")
	m.machine.transition_to(m.Marche)
	m.journal.clear()
	_verifier("Remontée : la transition du parent s'applique depuis l'enfant", m.machine.send_event(&"mourir") and _feuille(m) == "Mort", _feuille(m))
	_verifier("Remontée : sortie de la feuille avant le parent", m.journal == ["-Marche", "-Mouvement", "+Mort"], str(m.journal))


func _tester_consommation() -> void:
	var m := _machine()
	_transition(m.Mouvement, "../Mort", &"mourir")
	m.machine.transition_to(m.Marche)
	m.Marche.evenements_consommes.assign([&"mourir"])
	_verifier("Consommation : _on_event renvoie vrai", m.machine.send_event(&"mourir"))
	_verifier("Consommation : l'événement ne remonte pas", _feuille(m) == "Marche", _feuille(m))
	_verifier("Consommation : l'état a reçu l'événement", m.Marche.dernier_evenement == &"mourir")


# 🔁 Transitions automatiques

func _tester_condition_tableau_noir() -> void:
	var m := _machine()
	_transition(m.Repos, "../Mouvement", &"", "vitesse > 1")
	var tableau: Blackboard = m.machine.blackboard
	tableau.set_value("vitesse", 0)
	m.machine.update(0.1)
	_verifier("Condition : fausse, pas de transition", _feuille(m) == "Repos", _feuille(m))
	_verifier("Condition : _on_update appelé à chaque mise à jour", m.Repos.mises_a_jour == 1)
	tableau.set_value("vitesse", 2.5)
	m.machine.update(0.1)
	_verifier("Condition : vraie, transition", _feuille(m) == "Marche", _feuille(m))
	var signal_valeur := _espionner(tableau.value_changed)
	tableau.set_value("vitesse", 2.5)
	tableau.set_value("vitesse", 3)
	_verifier("Tableau noir : value_changed seulement si la valeur change", signal_valeur.n == 1, str(signal_valeur.n))


func _tester_duree_minimale() -> void:
	var m := _machine()
	_transition(m.Repos, "../Mort", &"", "", 0.5)
	m.machine.update(0.2)
	m.machine.update(0.2)
	_verifier("Durée minimale : pas avant 0,5 s", _feuille(m) == "Repos" and is_equal_approx(m.Repos.get_time_in_state(), 0.4))
	m.machine.update(0.2)
	_verifier("Durée minimale : transition après 0,5 s", _feuille(m) == "Mort", _feuille(m))
	_verifier("Durée minimale : temps remis à zéro à l'entrée", m.Mort.get_time_in_state() == 0.0)


func _tester_memoire_sous_etat() -> void:
	var m := _machine()
	m.Mouvement.remember_last_substate = true
	m.machine.transition_to(m.Course)
	m.machine.transition_to(m.Repos)
	m.machine.transition_to(m.Mouvement)
	_verifier("Mémoire : retour au dernier sous-état actif", _feuille(m) == "Course", _feuille(m))
	m.machine.restart()
	m.machine.transition_to(m.Mouvement)
	_verifier("Mémoire : oubliée par restart()", _feuille(m) == "Marche", _feuille(m))


func _tester_transition_vers_soi() -> void:
	var m := _machine()
	_transition(m.Repos, ".", &"reinitialiser")
	m.machine.update(0.3)
	m.journal.clear()
	m.machine.send_event(&"reinitialiser")
	_verifier("Vers soi : sortie puis entrée", m.journal == ["-Repos", "+Repos"], str(m.journal))
	_verifier("Vers soi : temps remis à zéro", m.Repos.get_time_in_state() == 0.0)


func _tester_transition_en_chaine() -> void:
	var m := _machine()
	m.Mort.rediriger_vers = "../Repos"
	var transitions := {"n": 0}
	m.machine.transitioned.connect(func(_source, _cible, _evenement): transitions.n += 1)
	m.journal.clear()
	m.machine.transition_to(m.Mort)
	_verifier("En chaîne : la transition demandée dans _on_enter est appliquée après", _feuille(m) == "Repos", _feuille(m))
	_verifier("En chaîne : ordre complet", m.journal == ["-Repos", "+Mort", "-Mort", "+Repos"], str(m.journal))
	_verifier("En chaîne : deux signaux transitioned", transitions.n == 2, str(transitions.n))
	# Boucle infinie (Repos ↔ Mort) : coupée par le garde-fou, sans planter.
	m.Repos.rediriger_vers = "../Mort"
	m.machine.transition_to(m.Mort)
	_verifier("En chaîne : boucle infinie coupée", m.machine.is_running())


func _tester_transition_scriptee() -> void:
	var m := _machine()
	var script := GDScript.new()
	script.source_code = "extends StateTransition\nfunc _check(state: Node) -> bool:\n\treturn state.has_meta(\"autorise\")\n"
	script.reload()
	var transition := _transition(m.Repos, "../Mort")
	transition.set_script(script)
	m.machine.update(0.1)
	_verifier("_check : faux, pas de transition", _feuille(m) == "Repos", _feuille(m))
	m.Repos.set_meta("autorise", true)
	m.machine.update(0.1)
	_verifier("_check : vrai, transition", _feuille(m) == "Mort", _feuille(m))
	transition.enabled = false
	_verifier("Transition désactivée : jamais remplie", not transition.is_satisfied(m.Repos))


func _tester_arret() -> void:
	var m := _machine()
	m.machine.transition_to(m.Course)
	m.journal.clear()
	var arrets := _espionner(m.machine.stopped)
	m.machine.stop()
	_verifier("Arrêt : sortie de la feuille vers la racine", m.journal == ["-Course", "-Mouvement"], str(m.journal))
	_verifier("Arrêt : plus aucun état actif", not m.machine.is_running() and m.machine.get_active_state() == null and not m.Course.is_active())
	_verifier("Arrêt : signal stopped", arrets.n == 1)
	_verifier("Arrêt : événements ignorés", not m.machine.send_event(&"bouger"))


func _tester_historique_et_instantane() -> void:
	var m := _machine()
	_transition(m.Repos, "../Mouvement/Course", &"courir", "vitesse > 5")
	m.machine.blackboard.set_value("vitesse", 10)
	m.machine.send_event(&"courir")
	var historique: Array = m.machine.get_history()
	_verifier("Historique : une entrée par transition", historique.size() == 1, str(historique))
	_verifier("Historique : de / vers / événement", historique[0].from == "Repos" and historique[0].to == "Mouvement/Course" and historique[0].event == &"courir", str(historique))

	var instantane: Dictionary = m.machine.get_debug_snapshot()
	var noeuds := {}
	for noeud in instantane.nodes:
		noeuds[noeud.id] = noeud
	_verifier("Instantané : type et tous les états", instantane.type == "hfsm" and noeuds.size() == 6, str(noeuds.keys()))
	_verifier("Instantané : racine « . » de type machine", noeuds.has(".") and noeuds["."].kind == "machine")
	_verifier("Instantané : état composé et parent", noeuds["Mouvement"].kind == "compound" and noeuds["Mouvement/Course"].parent == "Mouvement")
	_verifier("Instantané : états actifs", noeuds["Mouvement/Course"].active and noeuds["Mouvement"].active and not noeuds["Repos"].active)
	_verifier("Instantané : état initial marqué", noeuds["Repos"].initial and noeuds["Mouvement/Marche"].initial)
	var liens: Array = noeuds["Repos"].links
	_verifier("Instantané : transition avec libellé", liens.size() == 1 and liens[0].to == "Mouvement/Course" and "courir" in liens[0].label and "vitesse" in liens[0].label, str(liens))


func _tester_agent_et_expression_invalide() -> void:
	var m := _machine()
	_verifier("Agent : parent de la machine par défaut", m.Repos.get_agent() == m.agent)
	_transition(m.Repos, "../Mort", &"", "agent.has_meta('agent_test')")
	m.machine.update(0.1)
	_verifier("Expression : accès à l'agent", _feuille(m) == "Mort", _feuille(m))
	var transition := _transition(m.Mort, "../Repos", &"", "vitesse >> ) 2")
	m.machine.update(0.1)
	_verifier("Expression invalide : pas de transition, pas de plantage", _feuille(m) == "Mort")
	_verifier("Expression invalide : erreur disponible", transition.get_condition_error() != "", transition.get_condition_error())


# ⏱️ Dans l'arbre de scène

func _tester_demarrage_automatique() -> void:
	var m := _machine(false)
	var machine: StateMachine = m.machine
	var autre := StateMachine.new()
	autre.add_child(State.new())
	_ajouter(autre)
	_verifier("Démarrage auto : différé (pas pendant _ready)", not autre.is_running())
	await _frames(1)
	_verifier("Démarrage auto : démarrée à l'image suivante", autre.is_running())
	_verifier("autostart = false : reste à l'arrêt", not machine.is_running())


func _tester_traitement_automatique() -> void:
	var m := _machine()
	var machine: StateMachine = m.machine
	var aller := _transition(m.Repos, "../Mort", &"", "", 0.05)
	machine.process_callback = StateMachine.PROCESS_IDLE
	await _secondes(0.2)
	_verifier("PROCESS_IDLE : mises à jour et transitions automatiques", _feuille(m) == "Mort" and m.Repos.mises_a_jour > 0, _feuille(m))
	aller.enabled = false # sinon Repos ↔ Mort en boucle
	machine.process_callback = StateMachine.PROCESS_PHYSICS
	_transition(m.Mort, "../Repos", &"", "", 0.05)
	await _secondes(0.2)
	_verifier("PROCESS_PHYSICS : transitions dans _physics_process", _feuille(m) == "Repos", _feuille(m))
	machine.process_callback = StateMachine.PROCESS_MANUAL
