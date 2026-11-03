extends "../../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques du dialogue : DialogueResource et ses nœuds, DialogueSystem, DialogueBox.
##
## Dialogue de test :
##   bonjour (réplique) → menu (choix) ─ « Qui es-tu ? »  → identite → menu
##                                     ├ « Une quête ? » [niveau >= 2] → quete (action : quete = true, pieces += 10, ⚡quete_acceptee) → fin
##                                     └ « Au revoir »   (une seule fois) → test_ami (embranchement ami) → merci / adieu → fin
##
## Avertissements attendus dans la console : lien cassé, affectation invalide.


func _executer() -> void:
	_tester_ressource()
	_tester_deroulement()
	_tester_conditions_et_actions()
	_tester_option_unique()
	_tester_embranchement()
	_tester_cas_limites()
	_tester_instantane()
	await _tester_avance_automatique()
	await _tester_boite()


# 🧱 Construction

func _ligne(id: StringName, orateur: String, texte: String, suite: StringName = &"") -> DialogueLine:
	var ligne := DialogueLine.new()
	ligne.node_id = id
	ligne.speaker = orateur
	ligne.text = texte
	ligne.next = suite
	return ligne


func _option(texte: String, suite: StringName, condition := "", unique := false) -> DialogueOption:
	var option := DialogueOption.new()
	option.text = texte
	option.next = suite
	option.condition = condition
	option.once_only = unique
	return option


func _dialogue() -> DialogueResource:
	var menu := DialogueChoice.new()
	menu.node_id = &"menu"
	menu.prompt = "Que veux-tu, {nom} ?"
	menu.options.assign([_option("Qui es-tu ?", &"identite"), _option("Une quête ?", &"quete", "niveau >= 2"), _option("Au revoir", &"test_ami", "", true)])
	var quete := DialogueAction.new()
	quete.node_id = &"quete"
	quete.assignments = ["quete = true", "pieces += 10"]
	quete.event = &"quete_acceptee"
	quete.event_arguments = ["dragon"]
	quete.next = &"fin"
	var test_ami := DialogueBranch.new()
	test_ami.node_id = &"test_ami"
	test_ami.condition = "ami"
	test_ami.next_if_true = &"merci"
	test_ami.next_if_false = &"adieu"
	var fin := DialogueEnd.new()
	fin.node_id = &"fin"
	var dialogue := DialogueResource.new()
	dialogue.start_id = &"bonjour"
	dialogue.nodes.assign([
		_ligne(&"bonjour", "Garde", "Halte, {nom} !", &"menu"), menu,
		_ligne(&"identite", "Garde", "Je garde la porte.", &"menu"), quete, test_ami,
		_ligne(&"merci", "Garde", "À bientôt, ami.", &"fin"), _ligne(&"adieu", "Garde", "Circule.", &"fin"), fin,
	])
	return dialogue


func _systeme() -> DialogueSystem:
	var systeme := DialogueSystem.new()
	_ajouter(systeme)
	systeme.variables.set_values({"nom": "Ada", "niveau": 1, "ami": false})
	return systeme


# 📜 Ressource

func _tester_ressource() -> void:
	var dialogue := _dialogue()
	_verifier("Ressource : nœud de départ", dialogue.get_start_node().node_id == &"bonjour")
	_verifier("Ressource : recherche par identifiant", dialogue.get_dialogue_node(&"menu") is DialogueChoice and not dialogue.has_dialogue_node(&"absent"))
	_verifier("Ressource : valide", dialogue.validate().is_empty(), str(dialogue.validate()))
	_verifier("Ressource : identifiant libre", dialogue.generate_id("ligne") == &"ligne_1")
	var liens: Array = dialogue.get_dialogue_node(&"menu").get_links()
	_verifier("Liens d'un choix (une entrée par option)", liens.size() == 3 and liens[1].to == "quete" and "niveau" in liens[1].label, str(liens))
	dialogue.get_dialogue_node(&"menu").set_link(0, &"adieu")
	_verifier("set_link : change la cible d'une option", dialogue.get_dialogue_node(&"menu").options[0].next == &"adieu")
	dialogue.get_dialogue_node(&"merci").next = &"perdu"
	_verifier("validate : lien cassé signalé", dialogue.validate().size() == 1 and "perdu" in dialogue.validate()[0])


# ▶️ Déroulement

func _tester_deroulement() -> void:
	var systeme := _systeme()
	var lignes := _espionner(systeme.line_shown)
	var choix := _espionner(systeme.choices_shown)
	var fins := _espionner(systeme.dialogue_ended)
	_verifier("start : démarré", systeme.start(_dialogue()) and systeme.is_running())
	_verifier("Réplique : orateur et texte interpolé", lignes.args == ["Garde", "Halte, Ada !"], str(lignes.args))
	_verifier("Réplique : attente de advance()", systeme.get_wait_state() == DialogueSystem.WAIT_LINE and systeme.get_current_id() == &"bonjour")
	systeme.choose(0)
	_verifier("choose ignoré pendant une réplique", systeme.get_current_id() == &"bonjour")
	systeme.advance()
	_verifier("Choix : invite interpolée", choix.n == 1 and choix.args[0] == "Que veux-tu, Ada ?")
	var options: Array = systeme.get_available_options()
	_verifier("Choix : option conditionnelle masquée (niveau 1)", options.size() == 2 and options[1].text == "Au revoir", str(options.map(func(o): return o.text)))
	systeme.choose(0)
	_verifier("Choix : option suivie", systeme.get_current_id() == &"identite")
	systeme.advance()
	_verifier("Retour au menu", systeme.get_current_id() == &"menu")
	systeme.choose(1)
	_verifier("Embranchement traité aussitôt (ami = false → adieu)", systeme.get_current_id() == &"adieu" and lignes.args[1] == "Circule.")
	systeme.advance()
	_verifier("DialogueEnd : dialogue terminé", not systeme.is_running() and fins.n == 1)
	_verifier("Historique", systeme.get_history().size() == 5 and systeme.get_history()[1] == "→ Qui es-tu ?", str(systeme.get_history()))


func _tester_conditions_et_actions() -> void:
	var systeme := _systeme()
	systeme.variables.set_value("niveau", 3)
	var evenements := _espionner(systeme.dialogue_event)
	systeme.start(_dialogue())
	systeme.advance()
	_verifier("Condition vraie : option affichée", systeme.get_available_options().size() == 3)
	systeme.choose(1)
	_verifier("Action : affectations", systeme.variables.get_value("quete") == true and systeme.variables.get_value("pieces") == 10)
	_verifier("Action : événement et arguments", evenements.n == 1 and evenements.args == [&"quete_acceptee", ["dragon"]], str(evenements.args))
	_verifier("Action → fin", not systeme.is_running())
	systeme.start(_dialogue(), &"quete")
	_verifier("start(id) : départ choisi ; pieces += 10 cumulé", systeme.variables.get_value("pieces") == 20)
	_verifier("execute_assignment : -= et ==", systeme.execute_assignment("pieces -= 5") and systeme.variables.get_value("pieces") == 15
		and systeme.execute_assignment("egal = pieces == 15") and systeme.variables.get_value("egal") == true)
	_verifier("evaluate_condition", systeme.evaluate_condition("pieces > 10 and quete") and systeme.evaluate_condition(""))


func _tester_option_unique() -> void:
	var systeme := _systeme()
	var dialogue := _dialogue()
	systeme.start(dialogue)
	systeme.advance()
	systeme.choose(1) # « Au revoir » (unique)
	systeme.start(dialogue)
	systeme.advance()
	var textes: Array = systeme.get_available_options().map(func(o): return o.text)
	_verifier("once_only : option déjà prise masquée", textes == ["Qui es-tu ?"], str(textes))
	systeme.clear_seen_options()
	systeme.start(dialogue)
	systeme.advance()
	_verifier("clear_seen_options : option de nouveau proposée", systeme.get_available_options().size() == 2)


func _tester_embranchement() -> void:
	var systeme := _systeme()
	systeme.variables.set_value("ami", true)
	systeme.start(_dialogue(), &"test_ami")
	_verifier("Embranchement vrai", systeme.get_current_id() == &"merci")
	var dialogue := _dialogue()
	var menu: DialogueChoice = dialogue.get_dialogue_node(&"menu")
	for option in menu.options:
		option.condition = "false"
	menu.fallback_next = &"adieu"
	systeme.start(dialogue, &"menu")
	_verifier("Choix sans option disponible → fallback_next", systeme.get_current_id() == &"adieu")


func _tester_cas_limites() -> void:
	var systeme := _systeme()
	var dialogue := _dialogue()
	dialogue.get_dialogue_node(&"bonjour").next = &"inexistant"
	systeme.start(dialogue)
	systeme.advance()
	_verifier("Lien cassé : fin propre", not systeme.is_running())
	_verifier("Affectation invalide refusée", not systeme.execute_assignment("pas une affectation"))
	_verifier("Mot réservé refusé comme variable", not systeme.execute_assignment("or = 3"))
	_verifier("start sans dialogue : refusé", not systeme.start())
	systeme.default_dialogue = _dialogue()
	_verifier("start() : default_dialogue", systeme.start() and systeme.get_current_id() == &"bonjour")
	var interlocuteur := Node.new()
	interlocuteur.name = "Pnj"
	_ajouter(interlocuteur)
	systeme.start(_dialogue(), &"", interlocuteur)
	_verifier("Interlocuteur = agent des expressions", systeme.get_interlocutor() == interlocuteur and systeme.evaluate_condition("agent.name == 'Pnj'"))
	_verifier("DialogueSystem.get_main", DialogueSystem.get_main() != null)
	systeme.stop()


func _tester_instantane() -> void:
	var systeme := _systeme()
	var dialogue := _dialogue()
	var orpheline := _ligne(&"orpheline", "", "Personne ne vient ici.")
	dialogue.nodes.append(orpheline)
	systeme.start(dialogue)
	systeme.advance()
	var instantane: Dictionary = systeme.get_debug_snapshot()
	var noeuds := {}
	for noeud in instantane.nodes:
		noeuds[noeud.id] = noeud
	_verifier("Instantané : type, racine et tous les nœuds", instantane.type == "dialogue" and noeuds.has(".") and noeuds.size() == 10, str(noeuds.keys()))
	_verifier("Instantané : arbre de parcours (parent = premier lien)", noeuds["menu"].parent == "bonjour" and noeuds["identite"].parent == "menu" and noeuds["identite"].edge_label == "Qui es-tu ?")
	_verifier("Instantané : lien de retour en arc", noeuds["identite"].links.size() == 1 and noeuds["identite"].links[0].to == "menu")
	_verifier("Instantané : nœud courant actif, nœuds visités", noeuds["menu"].active and noeuds["bonjour"].status == 0 and noeuds["merci"].status == -1)
	_verifier("Instantané : nœud inaccessible rattaché à la racine", noeuds["orpheline"].parent == "." and "inaccessible" in noeuds["orpheline"].info)
	var vue := BehaviorGraphView.new()
	vue.size = Vector2(800, 500)
	_ajouter(vue)
	vue.set_target(systeme)
	_verifier("BehaviorGraphView : cible DialogueSystem", vue.get_node_ids().size() == 10)


# ⏱️ Dans l'arbre

func _tester_avance_automatique() -> void:
	var systeme := _systeme()
	var dialogue := DialogueResource.new()
	var ligne := _ligne(&"a", "", "Auto")
	ligne.auto_advance_time = 0.05
	dialogue.nodes.assign([ligne])
	systeme.start(dialogue)
	await _secondes(0.2)
	_verifier("auto_advance_time : avance seule", not systeme.is_running())


func _tester_boite() -> void:
	var systeme := _systeme()
	var boite := DialogueBox.new()
	boite.characters_per_second = 100
	boite.size = Vector2(600, 150)
	_ajouter(boite)
	boite.set_dialogue_system(systeme)
	await _frames(1)
	_verifier("Boîte : masquée sans dialogue", not boite.visible)
	systeme.start(_dialogue())
	_verifier("Boîte : affichée, orateur", boite.visible and boite.get_speaker_text() == "Garde")
	_verifier("Boîte : écriture lettre par lettre", boite.is_typing() and boite.get_displayed_text().length() < "Halte, Ada !".length())
	boite.press_advance()
	_verifier("Avance pendant l'écriture : texte complet, même réplique", not boite.is_typing() and boite.get_displayed_text() == "Halte, Ada !" and systeme.get_current_id() == &"bonjour")
	boite.press_advance()
	_verifier("Avance : réplique suivante (choix → boutons)", systeme.get_current_id() == &"menu" and boite.get_choice_count() == 2)
	await _frames(1)
	var bouton: Button = boite.find_children("*", "Button", true, false)[0]
	bouton.pressed.emit()
	_verifier("Bouton de choix : choose()", systeme.get_current_id() == &"identite")
	await _secondes(0.3)
	_verifier("Écriture terminée seule", not boite.is_typing() and boite.get_displayed_text() == "Je garde la porte.")
	systeme.stop()
	_verifier("Fin : boîte masquée", not boite.visible)
