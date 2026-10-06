extends Node2D

## 🤖 Démo de la machine à états hiérarchique : un garde patrouille, poursuit puis attaque l'intrus
## (le curseur de la souris), et peut être étourdi par un événement.
##
##   Machine
##   ├ Patrouille ──[distance < 120]──────────▶ Alerte
##   ├ Alerte (composé, se souvient de son dernier sous-état) ──[distance > 220]──▶ Patrouille
##   │ ├ Poursuite ──[distance < 30]──▶ Attaque
##   │ └ Attaque ──[distance > 45]──▶ Poursuite
##   └ Etourdi ──[⏱ 2 s]──▶ Patrouille        (⚡etourdir depuis Patrouille ou Alerte)
##
## En bas : BehaviorGraphView en direct (molette = zoom, glisser = déplacer, clic droit, pastilles +/-).
## Dans l'éditeur : sélectionner « Machine » puis « 🗺️ Éditeur de comportement » dans l'inspecteur ;
## pendant que la démo tourne, la fenêtre de l'éditeur suit le jeu (case « Suivre le jeu »).

const Panneau := preload("../../../2.🧭🌍/panneau_demo.gd")
# La suite du graphe simule la souris : elle n'est lancée que par lancer_tous_les_tests.gd (l'interface
# de la démo intercepterait ses clics).
const SuiteTests := preload("../suite_tests_hfsm.gd")
const ZONE_DE_JEU := Rect2(0, 0, 712, 340)

@onready var panneau: Panneau = $Interface/Panneau
@onready var garde: Node2D = $Garde
@onready var machine: StateMachine = $Garde/Machine
@onready var graphe: BehaviorGraphView = $Interface/Graphe

var _etat: Label


func _ready() -> void:
	machine.state_changed.connect(func(de: State, vers: State):
		panneau.journaliser("🔀 %s → %s" % [de.name if de else "∅", vers.name if vers else "∅"]))
	machine.event_received.connect(func(evenement: StringName, _donnees): panneau.journaliser("[color=violet]⚡ %s[/color]" % evenement))

	panneau.ajouter_section("🤖 Commandes")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("⚡ Étourdir", func(): machine.send_event(&"etourdir"), rangee)
	panneau.ajouter_bouton("🔁 Redémarrer", func(): machine.restart(), rangee)
	panneau.ajouter_options("Disposition", ["TechTree →", "Arbre vertical ↓", "MindMap ↔"], graphe.layout_mode,
		func(index: int): graphe.layout_mode = index as BehaviorGraphView.LayoutMode)
	panneau.ajouter_case("Libellés des transitions", true, func(actif: bool): graphe.show_transition_labels = actif)
	panneau.ajouter_bouton("🧪 Lancer les tests", _lancer_tests)
	_etat = panneau.ajouter_etiquette()


func _process(_delta: float) -> void:
	var souris := get_global_mouse_position()
	# Hors de la zone de jeu (graphe, panneau), l'intrus « disparaît ».
	garde.intrus = souris if ZONE_DE_JEU.has_point(souris) else Vector2(-10000, -10000)
	var tableau := machine.blackboard
	if tableau:
		_etat.text = "État : %s\nDistance : %.0f   ·   Coups : %d" % [
			" / ".join(machine.get_active_path().map(func(e): return e.name)),
			tableau.get_value("distance", 0.0), tableau.get_value("coups", 0)]
	queue_redraw()


func _draw() -> void:
	draw_rect(ZONE_DE_JEU, Color(0.13, 0.15, 0.18))
	var points: Array[Vector2] = $Garde/Machine/Patrouille.points
	for i in points.size():
		draw_line(points[i], points[(i + 1) % points.size()], Color(1, 1, 1, 0.12), 2.0)
	if garde.intrus.x > -1000:
		draw_circle(garde.intrus, 8.0, Color(1, 0.4, 0.4))


func _lancer_tests() -> void:
	await panneau.lancer_tests("Tests de la machine à états", SuiteTests, self)
