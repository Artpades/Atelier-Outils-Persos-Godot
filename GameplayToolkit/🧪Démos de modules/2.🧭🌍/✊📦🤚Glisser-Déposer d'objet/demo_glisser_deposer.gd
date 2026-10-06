extends Node3D

## ✊📦🤚 Démo du module glisser-déposer : un seul DragAndDropSystem pour des Control, Node2D et Node3D.
##
## - Au lancement, exécute la suite de tests automatiques du module (suite_tests_glisser_deposer.gd),
##   pendant laquelle le système de la démo est suspendu (les tests simulent la souris).
## - Cartes (Control) : posées sur un emplacement proche, sinon retour (SNAP_TO_VALID_POINT).
## - Jetons (Node2D) : un comportement de relâchement / d'aimantage différent par jeton.
## - Objets 3D : cubes glissés sur le sol (plan World Y) vers les socles, sphère face à la caméra.
## Le panneau change le mode de saisie, le bouton, snap_to_cursor et le plan de la sphère ; le journal
## affiche les signaux du système.

const Panneau := preload("../panneau_demo.gd")
const SuiteTests := preload("suite_tests_glisser_deposer.gd")
const BOUTONS: Array[MouseButton] = [MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE]
const PLANS := ["Caméra", "Objet X", "Objet Y", "Objet Z", "Monde X", "Monde Y (sol)", "Monde Z"]

@onready var panneau: Panneau = $Interface/Panneau
@onready var systeme: DragAndDropSystem = $GlisserDeposer
@onready var camera: Camera3D = $Camera
@onready var sphere: MeshInstance3D = $SphereVerte

var _etat: Label


func _ready() -> void:
	systeme.pointed.connect(func(noeud: Node): panneau.journaliser("[color=gray]👆 pointed : %s[/color]" % noeud.name))
	systeme.depointed.connect(func(noeud: Node): panneau.journaliser("[color=gray]👋 depointed : %s[/color]" % noeud.name))
	systeme.grabbed.connect(_sur_saisie)
	systeme.released.connect(func(noeud: Node): panneau.journaliser("🤚 released : %s" % noeud.name))
	systeme.snapped.connect(func(noeud: Node, point: Node): panneau.journaliser("[color=green]🧲 snapped : %s → %s[/color]" % [noeud.name, point.name]))

	panneau.ajouter_section("🖱️ Réglages")
	panneau.ajouter_options("Mode", ["Maintien (Hold)", "Bascule (Toggle)"], systeme.input_mode,
		func(index: int): systeme.input_mode = index as DragAndDropSystem.InputMode)
	panneau.ajouter_options("Bouton", ["Gauche", "Droit", "Milieu"], 0, func(index: int): systeme.action_button = BOUTONS[index])
	panneau.ajouter_case("Coller au curseur (snap_to_cursor)", false, _coller_au_curseur)

	panneau.ajouter_options("Plan de la sphère", PLANS, DraggableComponent.DRAG_PLANE_CAMERA,
		func(index: int): _composant_de(sphere).drag_plane_3d = index as DraggableComponent.DragPlane)
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Replacer les objets", _replacer, rangee)
	panneau.ajouter_bouton("Relancer les tests", _lancer_tests, rangee)
	_etat = panneau.ajouter_etiquette()

	_lancer_tests()


func _process(_delta: float) -> void:
	var survole := systeme.get_hovered_component()
	var saisi := systeme.get_grabbed_component()
	for composant: DraggableComponent in systeme.draggables:
		_mettre_en_valeur(composant.target_node, composant == survole, composant == saisi)

	_etat.text = "Survolé : %s   ·   Saisi : %s" % [_nom(survole), _nom(saisi)]


func _sur_saisie(noeud: Node) -> void:
	panneau.journaliser("✊ grabbed : %s" % noeud.name)
	# Dessiné au-dessus des autres pendant le glissement
	if noeud is CanvasItem:
		noeud.move_to_front()


func _mettre_en_valeur(noeud: Node, survole: bool, saisi: bool) -> void:
	if noeud == null:
		return
	if noeud is MeshInstance3D:
		var materiau: StandardMaterial3D = noeud.material_override
		materiau.emission_enabled = survole or saisi
		materiau.emission = materiau.albedo_color
		materiau.emission_energy_multiplier = 0.8 if saisi else 0.35
	elif "survole" in noeud:
		noeud.survole = survole
		noeud.saisi = saisi
	elif noeud is CanvasItem:
		noeud.modulate = Color(1.3, 1.3, 1.3) if survole or saisi else Color.WHITE
		if noeud is Control:
			noeud.scale = Vector2.ONE * (1.08 if saisi else 1.0)


func _coller_au_curseur(actif: bool) -> void:
	for composant: DraggableComponent in systeme.draggables:
		composant.snap_to_cursor = actif


func _replacer() -> void:
	systeme.release()
	for composant: DraggableComponent in systeme.draggables:
		var noeud := composant.target_node
		if noeud is Node3D:
			noeud.global_position = composant.initial_position_3d
		elif noeud is Node2D or noeud is Control:
			noeud.global_position = composant.initial_position_2d


func _composant_de(noeud: Node) -> DraggableComponent:
	for composant: DraggableComponent in systeme.draggables:
		if composant.target_node == noeud:
			return composant
	return null


func _nom(composant: DraggableComponent) -> String:
	return composant.target_node.name if composant and composant.target_node else "—"


func _lancer_tests() -> void:
	# Les tests simulent la souris : le système de la démo ne doit pas y réagir.
	systeme.process_mode = Node.PROCESS_MODE_DISABLED
	await panneau.lancer_tests("Tests du module glisser-déposer", SuiteTests, self)
	systeme.process_mode = Node.PROCESS_MODE_INHERIT
	camera.make_current() # les tests 3D ont rendu leur propre caméra courante
