extends "../../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques des personnages : Character2D (vue de dessus et plateforme), Character3D,
## vie et AbilitySystem, CharacterRegister, Interactable, PlayerController (actions simulées),
## AutomateController, NPCController + dialogue, feuilles BTFindTarget / BTMoveTo / BTUseAbility.
## Les déplacements utilisent la vraie physique (attente d'images physiques).


func _executer() -> void:
	await _tester_composants()
	await _tester_vue_de_dessus()
	await _tester_plateforme()
	_tester_vie()
	_tester_vie_par_attribut()
	_tester_factions_et_registre()
	_tester_interactable()
	await _tester_automate()
	await _tester_pnj_et_dialogue()
	await _tester_joueur()
	await _tester_personnage_3d()
	await _tester_feuilles_arbre()


# 🧱 Construction

func _physique(images: int) -> void:
	for i in images:
		await _bac_a_sable.get_tree().physics_frame


## Personnage 2D avec un déplacement au sol (composant du module Mouvement & Physique).
## p_mode : 0 = plateforme, 1 = vue de dessus (GroundMovement2D.Mode).
func _personnage(position: Vector2, mode := 1) -> Character2D:
	var personnage := Character2D.new()
	personnage.position = position
	var forme := CollisionShape2D.new()
	var cercle := CircleShape2D.new()
	cercle.radius = 10
	forme.shape = cercle
	personnage.add_child(forme)
	var deplacement: Node = ClassDB.instantiate(&"GroundMovement2D")
	deplacement.name = "Deplacement"
	deplacement.mode = mode
	personnage.add_child(deplacement)
	_ajouter(personnage)
	return personnage


func _deplacement(personnage: Node) -> Node:
	return personnage.get_node("Deplacement")


func _vie(personnage: Node, maximum := 100.0) -> HealthComponent:
	var vie := HealthComponent.new()
	vie.max_health = maximum
	vie.health = maximum
	vie.name = "Vie"
	personnage.add_child(vie)
	return vie


func _sol(x: float, y: float) -> StaticBody2D:
	var sol := StaticBody2D.new()
	sol.position = Vector2(x, y)
	var forme := CollisionShape2D.new()
	var rectangle := RectangleShape2D.new()
	rectangle.size = Vector2(4000, 20)
	forme.shape = rectangle
	sol.add_child(forme)
	_ajouter(sol)
	return sol


# 🧩 Composants

func _tester_composants() -> void:
	var nu := Character2D.new()
	nu.position = Vector2(-20000, 0)
	_ajouter(nu)
	nu.set_move_input(Vector2(1, 0))
	nu.request_jump()
	await _physique(3)
	_verifier("Sans composant : vivant, aucun déplacement, pas d'erreur", nu.is_alive() and nu.get_health() == 0.0 and nu.position == Vector2(-20000, 0) and nu.get_active_movement() == null)

	var perso := _personnage(Vector2(-21000, 0))
	var vie := _vie(perso, 40)
	_verifier("get_components : seulement les nœuds non spatiaux", perso.get_components() == [_deplacement(perso), vie], str(perso.get_components()))
	_verifier("get_component par classe", perso.get_component(&"HealthComponent") == vie and perso.get_health_component() == vie)
	var noms := perso.get_property_list().map(func(p): return p.name)
	_verifier("Propriétés des composants exposées sur le personnage", noms.has("components/Deplacement/max_speed") and noms.has("components/Vie/max_health") and not noms.has("components/Deplacement/process_mode"))
	perso.set("components/Deplacement/max_speed", 321.0)
	_verifier("Écriture d'une propriété de composant depuis le personnage", is_equal_approx(_deplacement(perso).max_speed, 321.0) and is_equal_approx(perso.get("components/Deplacement/max_speed"), 321.0))
	perso.expose_components = false
	_verifier("expose_components = false : propriétés masquées", not perso.get_property_list().map(func(p): return p.name).has("components/Deplacement/max_speed"))


# 🏃 Déplacement 2D (par le composant GroundMovement2D)

func _tester_vue_de_dessus() -> void:
	var perso := _personnage(Vector2(5000, 0))
	var deplacement := _deplacement(perso)
	deplacement.max_speed = 200
	deplacement.acceleration = 2000
	perso.set_move_input(Vector2(1, 0))
	await _physique(20)
	_verifier("Vue de dessus : avance vers la droite", perso.position.x > 5020, str(perso.position))
	_verifier("Vue de dessus : vitesse plafonnée", is_equal_approx(perso.velocity.length(), 200.0), str(perso.velocity))
	perso.set_move_input(Vector2(3, 4))
	_verifier("Entrée limitée à une longueur de 1", is_equal_approx(perso.get_move_input().length(), 1.0))
	perso.set_running(true)
	perso.set_move_input(Vector2(0, 1))
	await _physique(30)
	_verifier("Course : vitesse × run_multiplier", is_equal_approx(perso.velocity.length(), 200.0 * deplacement.run_multiplier), str(perso.velocity))
	_verifier("Orientation (facing) vers la marche", perso.get_facing().is_equal_approx(Vector2(0, 1)))
	perso.set_move_input(Vector2.ZERO)
	await _physique(30)
	_verifier("Décélération jusqu'à l'arrêt", perso.velocity == Vector2.ZERO)
	_verifier("direction_input_to / planar_distance_to", perso.direction_input_to(perso.get_world_position() + Vector3(0, -50, 0)).is_equal_approx(Vector2(0, -1))
		and is_equal_approx(perso.planar_distance_to(perso.get_world_position() + Vector3(30, 40, 0)), 50.0))


func _tester_plateforme() -> void:
	_sol(5000, 5100)
	var perso := _personnage(Vector2(5000, 5000), 0)
	var deplacement := _deplacement(perso)
	var atterrissages := _espionner(deplacement.landed)
	var sauts := _espionner(deplacement.jumped)
	await _physique(60)
	_verifier("Plateforme : tombe et se pose sur le sol", perso.is_on_floor() and atterrissages.n == 1 and perso.is_side_view(), str(perso.position))
	var hauteur_sol := perso.position.y
	perso.request_jump()
	await _physique(8)
	_verifier("Saut : quitte le sol, signal jumped", sauts.n == 1 and perso.position.y < hauteur_sol - 20)
	perso.request_jump()
	await _physique(2)
	_verifier("max_jumps = 1 : pas de saut en l'air", sauts.n == 1)
	await _physique(90)
	deplacement.max_jumps = 2
	perso.request_jump()
	await _physique(6)
	perso.request_jump()
	await _physique(2)
	_verifier("Double saut (max_jumps = 2)", sauts.n == 3)
	await _physique(120)

	# Hauteur variable : un saut relâché aussitôt monte moins haut.
	var haut_complet := await _hauteur_de_saut(perso, false)
	var haut_court := await _hauteur_de_saut(perso, true)
	_verifier("Saut à hauteur variable (jump_cut)", haut_court < haut_complet * 0.8, "%s / %s" % [haut_court, haut_complet])
	perso.set_move_input(Vector2(0, -1))
	await _physique(5)
	_verifier("Plateforme : entrée verticale ignorée", absf(perso.velocity.x) < 0.01)


func _hauteur_de_saut(perso: Character2D, relacher: bool) -> float:
	var depart := perso.position.y
	perso.request_jump()
	await _physique(1)
	if relacher:
		perso.release_jump()
	var plus_haut := depart
	for i in 60:
		await _physique(1)
		plus_haut = minf(plus_haut, perso.position.y)
	perso.release_jump()
	await _physique(60)
	return depart - plus_haut


# ❤️ Vie (HealthComponent)

func _tester_vie() -> void:
	var perso := _personnage(Vector2(-5000, 0))
	var vie := _vie(perso, 50)
	vie.invulnerability_time = 0.5
	var degats := _espionner(vie.damaged)
	var morts := _espionner(vie.died)
	_verifier("Dégâts appliqués", perso.take_damage(20) == 20 and perso.get_health() == 30 and degats.n == 1)
	_verifier("Invulnérable juste après", vie.is_invulnerable() and perso.take_damage(5) == 0)
	await _secondes(0.6)
	_verifier("Soin plafonné à max_health", perso.heal(100) == 20 and perso.get_health() == 50)
	perso.take_damage(80)
	_verifier("Mort : died, plus d'action possible", not perso.is_alive() and morts.n == 1 and perso.is_movement_blocked() and perso.heal(10) == 0)
	perso.set_move_input(Vector2(1, 0))
	var x := perso.position.x
	await _physique(10)
	_verifier("Mort : l'intention de déplacement est coupée", absf(perso.position.x - x) < 0.01)
	vie.revive()
	_verifier("revive : vivant, vie pleine", perso.is_alive() and perso.get_health() == 50)


func _tester_vie_par_attribut() -> void:
	var perso := _personnage(Vector2(-5200, 0))
	var systeme := AbilitySystem.new()
	var vie_max := AttributeDefinition.new()
	vie_max.attribute_name = &"max_health"
	vie_max.base_value = 80
	var vie := AttributeDefinition.new()
	vie.attribute_name = &"health"
	vie.base_value = 80
	vie.max_attribute = &"max_health"
	systeme.attribute_definitions = [vie_max, vie]
	systeme.manual_update = true
	perso.add_child(systeme)
	var composant := _vie(perso) # après l'AbilitySystem : il s'y relie à _ready
	var morts := _espionner(composant.died)
	_verifier("Vie lue dans l'AbilitySystem", composant.uses_attribute() and perso.get_health() == 80 and perso.get_max_health() == 80 and perso.get_ability_system() == systeme)
	perso.take_damage(30)
	_verifier("Dégâts → attribut health", systeme.get_value(&"health") == 50)
	systeme.add_to_base(&"health", -100)
	_verifier("Attribut à zéro → mort", not perso.is_alive() and morts.n == 1)
	composant.revive()
	systeme.add_tag("etat.etourdi")
	_verifier("Tag etat.etourdi : déplacement bloqué", perso.is_movement_blocked())


# 📇 Factions et registre

func _tester_factions_et_registre() -> void:
	var heros := _personnage(Vector2(9000, 9000))
	heros.faction = &"heros"
	heros.hostile_factions = ["monstres"]
	var loup := _personnage(Vector2(9100, 9000))
	loup.faction = &"monstres"
	var ours := _personnage(Vector2(9300, 9000))
	ours.faction = &"monstres"
	var villageois := _personnage(Vector2(9050, 9000))
	villageois.faction = &"heros"
	_verifier("Hostilité (dans un sens suffit)", heros.is_hostile_to(loup) and loup.is_hostile_to(heros) and not heros.is_hostile_to(villageois))
	_verifier("Registre : personnages inscrits", CharacterRegister.get_characters().has(heros) and CharacterRegister.get_faction_members(&"monstres").size() >= 2)
	_verifier("Registre : plus proche (tous)", CharacterRegister.get_nearest(heros, 500) == villageois)
	_verifier("Registre : plus proche hostile", CharacterRegister.get_nearest(heros, 500, true) == loup)
	_verifier("Registre : rayon", CharacterRegister.get_in_radius(heros, 150, true) == [loup])
	_vie(loup).kill()
	_verifier("Registre : morts ignorés par défaut", CharacterRegister.get_nearest(heros, 500, true) == ours)


# 🤝 Interactable

func _tester_interactable() -> void:
	var perso := _personnage(Vector2(-9000, -9000))
	var coffre := Node2D.new()
	coffre.position = Vector2(-8960, -9000)
	var interactable := Interactable.new()
	interactable.interaction_range = 50
	interactable.one_shot = true
	coffre.add_child(interactable)
	_ajouter(coffre)
	var loin := Node2D.new()
	loin.position = Vector2(-8800, -9000)
	loin.add_child(Interactable.new())
	_ajouter(loin)
	_verifier("find_nearest : à portée", Interactable.find_nearest(perso) == interactable)
	var interactions := _espionner(interactable.interacted)
	_verifier("interact : signal avec l'auteur", interactable.interact(perso) and interactions.args[0] == perso)
	_verifier("one_shot : désactivé ensuite", not interactable.interact(perso) and Interactable.find_nearest(perso) == null)


# 🤖 Automate

func _tester_automate() -> void:
	var perso := _personnage(Vector2(20000, 0))
	_deplacement(perso).max_speed = 400
	var automate := AutomateController.new()
	perso.add_child(automate)
	var arrivees := _espionner(automate.destination_reached)
	automate.move_to(Vector2(20100, 50))
	_verifier("move_to : mode", automate.get_mode() == AutomateController.MODE_MOVE_TO)
	await _physique(40)
	_verifier("move_to : arrivé, signal, arrêt", arrivees.n == 1 and automate.get_mode() == AutomateController.MODE_IDLE
		and perso.planar_distance_to(Vector3(20100, 50, 0)) <= automate.get_effective_arrive_distance() + 4, str(perso.position))

	var points := _espionner(automate.patrol_point_reached)
	automate.wait_time = 0.0
	automate.patrol([Vector2(20100, 0), Vector2(20000, 0)])
	await _physique(90)
	_verifier("Patrouille : points atteints dans l'ordre", points.n >= 2, str(points))

	var cible := _personnage(Vector2(20300, 0))
	automate.follow(cible, 40)
	await _physique(60)
	var distance := perso.position.distance_to(cible.position)
	_verifier("follow : rejoint la cible à follow_distance", distance <= 48 and distance >= 30, str(distance))
	var perdues := _espionner(automate.target_lost)
	cible.queue_free()
	await _physique(3)
	_verifier("follow : cible libérée → target_lost, arrêt", perdues.n == 1 and automate.get_mode() == AutomateController.MODE_IDLE)

	automate.wander(60)
	var premiere := automate.get_destination()
	_verifier("wander : destination autour de home", premiere.distance_to(automate.get_home()) <= 60.01)
	automate.stop()
	_verifier("stop : à l'arrêt", automate.get_mode() == AutomateController.MODE_IDLE and perso.get_move_input() == Vector2.ZERO)


# 🧑‍🌾 PNG

func _dialogue_simple() -> DialogueResource:
	var ligne := DialogueLine.new()
	ligne.node_id = &"salut"
	ligne.speaker = "Meunier"
	ligne.text = "Bonjour !"
	var dialogue := DialogueResource.new()
	dialogue.nodes.assign([ligne])
	return dialogue


func _tester_pnj_et_dialogue() -> void:
	var systeme := DialogueSystem.new()
	_ajouter(systeme)
	var pnj := _personnage(Vector2(30000, 0))
	var controleur := NPCController.new()
	controleur.dialogue = _dialogue_simple()
	controleur.start_mode = AutomateController.MODE_WANDER
	controleur.wander_radius = 30
	pnj.add_child(controleur)
	await _frames(2)
	var interactable := controleur.get_interactable()
	_verifier("PNG : Interactable créé automatiquement", interactable != null and interactable.prompt == "Parler")
	var joueur := _personnage(Vector2(30040, 0))
	var debuts := _espionner(controleur.talk_started)
	var fins := _espionner(controleur.talk_ended)
	interactable.interact(joueur)
	_verifier("PNG : l'interaction lance le dialogue", systeme.is_running() and controleur.is_talking() and debuts.n == 1)
	_verifier("PNG : interlocuteur = le PNG", systeme.get_interlocutor() == pnj)
	await _physique(5)
	_verifier("PNG : immobile pendant la conversation", pnj.get_move_input() == Vector2.ZERO)
	_verifier("PNG : tourné vers le joueur", pnj.get_facing().x > 0)
	systeme.advance()
	_verifier("PNG : fin du dialogue → talk_ended", not controleur.is_talking() and fins.n == 1)
	systeme.queue_free()


# 🏌️ Joueur

func _tester_joueur() -> void:
	var perso := _personnage(Vector2(40000, 0))
	var controleur := PlayerController.new()
	controleur.ability_actions = {&"test_capacite": &"cri"}
	perso.add_child(controleur)
	var systeme := AbilitySystem.new()
	var cri := Ability.new()
	cri.ability_id = &"cri"
	cri.cooldown = 5
	systeme.granted_abilities = [cri]
	perso.add_child(systeme)
	await _frames(1)
	_verifier("Actions par défaut créées", InputMap.has_action(&"move_right") and InputMap.has_action(&"jump") and InputMap.has_action(&"interact"))
	if not InputMap.has_action(&"test_capacite"):
		InputMap.add_action(&"test_capacite")

	# Une manette réellement branchée envoie des mouvements d'axe (dérive) qui relâcheraient les actions
	# simulées : on ne garde que les touches clavier pour ce test.
	for action in [&"move_left", &"move_right", &"move_up", &"move_down", &"run", &"interact"]:
		for evenement in InputMap.action_get_events(action):
			if evenement is InputEventJoypadMotion or evenement is InputEventJoypadButton:
				InputMap.action_erase_event(action, evenement)
	Input.action_press(&"move_right")
	await _physique(2)
	_verifier("Action move_right → entrée (1, 0)", perso.get_move_input().distance_to(Vector2(1, 0)) < 0.05, str(perso.get_move_input())) # tolérance : dérive d'une manette branchée
	Input.action_release(&"move_right")
	Input.action_press(&"run")
	await _physique(2)
	_verifier("Arrêt et course", perso.get_move_input() == Vector2.ZERO and perso.is_running())
	Input.action_release(&"run")

	Input.action_press(&"test_capacite")
	await _physique(2)
	Input.action_release(&"test_capacite")
	_verifier("ability_actions : capacité activée", systeme.get_charges(&"cri") == 0)

	var pnj := _personnage(Vector2(40030, 0))
	var npc := NPCController.new()
	npc.dialogue = _dialogue_simple()
	pnj.add_child(npc)
	var systeme_dialogue := DialogueSystem.new()
	_ajouter(systeme_dialogue)
	await _frames(2)
	var focus := _espionner(controleur.focus_changed)
	await _physique(2)
	_verifier("Focus sur l'Interactable proche", controleur.get_focused_interactable() == npc.get_interactable() and focus.n >= 1)
	Input.action_press(&"interact")
	await _physique(2)
	Input.action_release(&"interact")
	_verifier("Action interact → dialogue du PNG", systeme_dialogue.is_running())
	Input.action_press(&"move_left")
	await _physique(2)
	_verifier("Commandes coupées pendant le dialogue", perso.get_move_input() == Vector2.ZERO)
	Input.action_release(&"move_left")
	systeme_dialogue.stop()
	systeme_dialogue.queue_free()


# 🧊 3D

func _tester_personnage_3d() -> void:
	var sol := StaticBody3D.new()
	var forme_sol := CollisionShape3D.new()
	var boite := BoxShape3D.new()
	boite.size = Vector3(200, 1, 200)
	forme_sol.shape = boite
	sol.add_child(forme_sol)
	sol.position = Vector3(0, -500.5, 0)
	_ajouter(sol)
	var perso := Character3D.new()
	var forme := CollisionShape3D.new()
	forme.shape = CapsuleShape3D.new()
	perso.add_child(forme)
	var deplacement: Node = ClassDB.instantiate(&"GroundMovement3D")
	deplacement.name = "Deplacement"
	perso.add_child(deplacement)
	perso.position = Vector3(0, -498, 0)
	_ajouter(perso)
	await _physique(60)
	_verifier("3D : tombe et se pose", perso.is_on_floor(), str(perso.position))
	perso.set_move_input(Vector2(1, 0))
	await _physique(40)
	_verifier("3D : avance sur X (plan horizontal)", perso.position.x > 1.0 and absf(perso.position.z) < 0.01, str(perso.position))
	_verifier("3D : tourné vers la marche (-Z → +X)", absf(wrapf(perso.rotation.y, -PI, PI) + PI / 2.0) < 0.01, str(perso.rotation.y))
	perso.set_move_input(Vector2.ZERO)
	perso.request_jump()
	await _physique(10)
	_verifier("3D : saut", perso.position.y > -498.5 and not perso.is_on_floor())
	_verifier("3D : direction_input_to dans le plan XZ", perso.direction_input_to(perso.position + Vector3(0, 10, -5)).is_equal_approx(Vector2(0, -1)))


# 🌿 Feuilles d'arbre de comportement

func _tester_feuilles_arbre() -> void:
	var chasseur := _personnage(Vector2(50000, 0))
	chasseur.faction = &"loups"
	chasseur.hostile_factions = ["moutons"]
	_deplacement(chasseur).max_speed = 400
	chasseur.add_child(AutomateController.new())
	var systeme := AbilitySystem.new()
	var morsure := Ability.new()
	morsure.ability_id = &"morsure"
	morsure.cooldown = 1
	systeme.granted_abilities = [morsure]
	chasseur.add_child(systeme)
	var mouton := _personnage(Vector2(50150, 0))
	mouton.faction = &"moutons"

	var trouver := BTFindTarget.new()
	trouver.max_distance = 300
	var aller := BTMoveTo.new()
	aller.arrive_distance = 30
	var mordre := BTUseAbility.new()
	mordre.ability_id = &"morsure"
	mordre.target_key = "cible"
	var sequence := BTSequence.new()
	for feuille in [trouver, aller, mordre]:
		sequence.add_child(feuille)
	var arbre := BehaviorTree.new()
	arbre.process_callback = BehaviorTree.PROCESS_PHYSICS
	arbre.add_child(sequence)
	chasseur.add_child(arbre)
	var reussites := {"n": 0} # l'arbre recommence ensuite (et échoue : capacité en recharge)
	arbre.finished.connect(func(statut: int): reussites.n += int(statut == BTNode.SUCCESS))
	await _physique(3)
	_verifier("BTFindTarget : cible hostile rangée dans le tableau noir", arbre.blackboard.get_value("cible") == mouton)
	_verifier("BTMoveTo : RUNNING en chemin", aller.is_running())
	await _physique(40)
	_verifier("Séquence complète : arrivé puis capacité activée", reussites.n >= 1 and systeme.get_charges(&"morsure") == 0, str(reussites))
	_verifier("BTMoveTo : à portée de la cible", chasseur.position.distance_to(mouton.position) <= 40, str(chasseur.position.distance_to(mouton.position)))
	_vie(mouton).kill()
	await _physique(2)
	_verifier("BTFindTarget : plus de cible vivante → FAILURE", arbre.get_last_status() == BTNode.FAILURE and not arbre.blackboard.has_value("cible"))
	arbre.active = false
