extends Node2D

## 🌿 Démo de l'arbre de comportement : un récolteur ramasse les pièces, fuit le curseur, se repose quand
## il est fatigué, et erre sinon.
##
##   Arbre
##   └ Choix (sélecteur réactif : une branche prioritaire interrompt la branche en cours)
##     ├ Fuir        : [danger < 120] → action_fuir
##     ├ Se reposer  : [energie < 20] → action_repos
##     ├ Récolter    : [piece_visible] → action_vers_piece → ramasser() (BTCallMethod) → score += 1 (BTSetValue)
##     └ Errer       : action_errer (limitée à 4 s, BTTimeLimit) → attendre 0,6 s ± 0,4 (BTWait)
##
## Clic gauche dans la zone de jeu : poser une pièce. En bas : BehaviorGraphView en direct.

const Panneau := preload("../../../2.🧭🌍/panneau_demo.gd")
const SuiteTests := preload("../suite_tests_arbre.gd")
const ZONE_DE_JEU := Rect2(0, 0, 712, 250)

@onready var panneau: Panneau = $Interface/Panneau
@onready var recolteur: Node2D = $Recolteur
@onready var arbre: BehaviorTree = $Recolteur/Arbre
@onready var graphe: BehaviorGraphView = $Interface/Graphe

var _etat: Label
var _branche := ""


func _ready() -> void:
	recolteur.piece_ramassee.connect(func(total: int): panneau.journaliser("[color=gold]🪙 Pièce ramassée (%d)[/color]" % total))
	_poser_pieces(4)

	panneau.ajouter_section("🌿 Commandes")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("🪙 Pièces ×5", _poser_pieces.bind(5), rangee)
	panneau.ajouter_bouton("😴 Fatiguer", func(): arbre.blackboard.set_value("energie", 5.0), rangee)
	panneau.ajouter_case("Arbre actif", true, func(actif: bool): arbre.active = actif)
	panneau.ajouter_options("Disposition", ["TechTree →", "Arbre vertical ↓", "MindMap ↔"], graphe.layout_mode,
		func(index: int): graphe.layout_mode = index as BehaviorGraphView.LayoutMode)
	panneau.ajouter_bouton("🧪 Lancer les tests", func(): panneau.lancer_tests("Tests de l'arbre de comportement", SuiteTests, self))
	_etat = panneau.ajouter_etiquette()


func _unhandled_input(event: InputEvent) -> void:
	var clic := event as InputEventMouseButton
	if clic and clic.pressed and clic.button_index == MOUSE_BUTTON_LEFT and ZONE_DE_JEU.has_point(clic.position):
		_poser_piece(clic.position)


func _process(_delta: float) -> void:
	var souris := get_global_mouse_position()
	recolteur.danger_position = souris if ZONE_DE_JEU.has_point(souris) else Vector2(-10000, -10000)
	# Branche choisie par le sélecteur : son enfant en cours d'exécution.
	var branche := "—"
	for enfant in $Recolteur/Arbre/Choix.get_bt_children():
		if enfant.is_running():
			branche = enfant.name
	if branche != _branche and branche != "—":
		panneau.journaliser("🌿 Branche : %s" % branche)
	_branche = branche
	var tableau := arbre.blackboard
	_etat.text = "Branche : %s\nÉnergie : %.0f   ·   Score : %d   ·   Pièces : %d" % [
		branche, tableau.get_value("energie", 0.0), tableau.get_value("score", 0), recolteur.pieces.size()]
	queue_redraw()


func _poser_piece(position_piece: Vector2) -> void:
	recolteur.pieces.append(position_piece)


func _poser_pieces(nombre: int) -> void:
	for i in nombre:
		_poser_piece(Vector2(randf_range(40, 670), randf_range(30, 220)))


func _draw() -> void:
	draw_rect(ZONE_DE_JEU, Color(0.13, 0.16, 0.14))
	for piece in recolteur.pieces:
		draw_circle(piece, 7.0, Color(1, 0.82, 0.2))
	if recolteur.danger_position.x > -1000:
		draw_circle(recolteur.danger_position, 120.0, Color(1, 0.3, 0.3, 0.08))
		draw_circle(recolteur.danger_position, 8.0, Color(1, 0.4, 0.4))
