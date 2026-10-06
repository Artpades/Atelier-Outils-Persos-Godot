extends Node2D

## 🧑 Démo des personnages, du dialogue et des capacités (2D, vue de dessus).
##
## - Héros (Character2D + PlayerController + AbilitySystem) : ZQSD / WASD / flèches, Maj pour courir,
##   E pour parler, J pour frapper (cible auto à 50 px), K pour se soigner (20 mana, régénération).
## - Meunier (NPCController) : erre près du moulin ; son dialogue (dialogue_meunier.tres : choix,
##   conditions, actions, variables) donne une quête, puis une récompense quand tous les slimes sont battus.
## - Slimes (slime.tscn) : AutomateController + BehaviorTree (BTFindTarget → BTMoveTo → BTUseAbility,
##   sinon errance) ; leur morsure est une capacité qui retire de la vie au héros.
## Le panneau affiche le graphe en direct du comportement choisi (ou du dialogue en cours).
## Dans l'éditeur : sélectionner Meunier/NPCController ou Systeme → « 💬 Éditeur de dialogue » ;
## ouvrir slime.tscn → Comportement → « 🗺️ Éditeur de comportement ».

const Panneau := preload("../../2.🧭🌍/panneau_demo.gd")
const SceneSlime := preload("slime.tscn")
# Les tests des personnages simulent les touches : seuls ceux des capacités et du dialogue sont lancés ici.
const SUITES := {
	"Système de capacités": preload("../🌟Système de Capacité/suite_tests_capacites.gd"),
	"Système de dialogue": preload("../💬Système de Dialogue/suite_tests_dialogue.gd"),
}
const ZONE_DE_JEU := Rect2(0, 0, 712, 648)

@onready var panneau: Panneau = $Interface/Panneau
@onready var joueur: Character2D = $Joueur
@onready var controleur: PlayerController = $Joueur/PlayerController
@onready var capacites: AbilitySystem = $Joueur/Capacites
@onready var systeme: DialogueSystem = $Systeme
@onready var slimes: Node2D = $Slimes

var _graphe: BehaviorGraphView
var _etat: Label
var _choix_graphe: OptionButton


func _ready() -> void:
	_creer_actions()
	_creer_murs()
	systeme.variables.set_values({"nom": joueur.display_name, "slimes_restants": slimes.get_child_count(),
		"quete_acceptee": false, "quete_finie": false, "recompense_donnee": false, "pieces": 0})
	systeme.dialogue_event.connect(func(evenement: StringName, _arguments): panneau.journaliser("[color=violet]⚡ %s[/color]" % evenement))
	systeme.dialogue_started.connect(func(_d): _choisir_graphe(_choix_graphe.item_count - 1))
	for slime in slimes.get_children():
		_suivre_slime(slime)
	joueur.get_health_component().died.connect(_sur_mort_du_heros)
	capacites.ability_activated.connect(func(id: StringName): panneau.journaliser("🌟 %s" % id))
	capacites.ability_failed.connect(func(id: StringName, raison: String): panneau.journaliser("[color=gray]✋ %s : %s[/color]" % [id, raison]))

	panneau.ajouter_section("🧑 Commandes")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("🟢 Nouveau slime", _ajouter_slime, rangee)
	panneau.ajouter_bouton("❤️ Soigner", func(): joueur.get_health_component().revive(); joueur.heal(1000), rangee)
	_graphe = BehaviorGraphView.new()
	_graphe.custom_minimum_size = Vector2(0, 230)
	_graphe.show_history = false
	_choix_graphe = panneau.ajouter_options("Graphe", PackedStringArray(["…"]), 0, _choisir_graphe)
	panneau.ajouter(_graphe)
	_maj_choix_graphe()
	panneau.ajouter_bouton("🧪 Lancer les tests (capacités, dialogue)", _lancer_tests)
	_etat = panneau.ajouter_etiquette()


func _process(_delta: float) -> void:
	var variables := systeme.variables
	_etat.text = "Vie %d / %d · Mana %d / %d · Pièces %d\nCoup : %s · Soin : %s · Slimes : %d" % [
		joueur.get_health(), joueur.get_max_health(), capacites.get_value(&"mana"), capacites.get_value(&"max_mana"),
		variables.get_value("pieces", 0), _recharge(&"coup"), _recharge(&"soin"), variables.get_value("slimes_restants", 0)]
	queue_redraw()


func _draw() -> void:
	draw_rect(ZONE_DE_JEU, Color(0.15, 0.2, 0.15))
	draw_rect(Rect2(310, 90, 100, 70), Color(0.45, 0.35, 0.25)) # le moulin
	var cible := controleur.get_focused_interactable()
	if cible and not systeme.is_running():
		var position_cible: Vector3 = cible.get_world_position()
		draw_string(ThemeDB.fallback_font, Vector2(position_cible.x - 60, position_cible.y - 34), "E : %s" % cible.prompt, HORIZONTAL_ALIGNMENT_CENTER, 120, 14)


# 🟢 Slimes

func _suivre_slime(slime: Character2D) -> void:
	slime.get_health_component().died.connect(_sur_mort_de_slime.bind(slime))


func _sur_mort_de_slime(slime: Character2D) -> void:
	var restants: int = systeme.variables.get_value("slimes_restants") - 1
	systeme.variables.set_value("slimes_restants", restants)
	panneau.journaliser("💥 Slime vaincu (%d restant(s))" % restants)
	if restants <= 0 and systeme.variables.get_value("quete_acceptee"):
		systeme.variables.set_value("quete_finie", true)
		panneau.journaliser("[color=gold]✅ Quête terminée : retourne voir le meunier ![/color]")
	await get_tree().create_timer(0.6).timeout
	slime.queue_free()
	await get_tree().process_frame
	_maj_choix_graphe()


func _ajouter_slime() -> void:
	var slime: Character2D = SceneSlime.instantiate()
	slime.position = Vector2(randf_range(420, 680), randf_range(260, 600))
	slimes.add_child(slime)
	_suivre_slime(slime)
	systeme.variables.set_value("slimes_restants", systeme.variables.get_value("slimes_restants") + 1)
	_maj_choix_graphe()


func _sur_mort_du_heros() -> void:
	panneau.journaliser("[color=red]☠ Le héros est tombé… il se relève dans 2 s.[/color]")
	await get_tree().create_timer(2.0).timeout
	joueur.get_health_component().revive()


# 🗺️ Graphe

func _maj_choix_graphe() -> void:
	_choix_graphe.clear()
	for slime in slimes.get_children():
		if not slime.is_queued_for_deletion():
			_choix_graphe.add_item("Comportement : %s" % slime.name)
			_choix_graphe.set_item_metadata(_choix_graphe.item_count - 1, slime.get_node("Comportement"))
	_choix_graphe.add_item("Dialogue en cours")
	_choix_graphe.set_item_metadata(_choix_graphe.item_count - 1, systeme)
	_choisir_graphe(0)


func _choisir_graphe(index: int) -> void:
	_choix_graphe.select(index)
	var cible = _choix_graphe.get_item_metadata(index)
	_graphe.set_target(cible if is_instance_valid(cible) else null)


# 🛠️ Mise en place

func _creer_actions() -> void:
	for action in [[&"attaque", KEY_J], [&"soin", KEY_K]]:
		if not InputMap.has_action(action[0]):
			InputMap.add_action(action[0])
			var touche := InputEventKey.new()
			touche.physical_keycode = action[1]
			InputMap.action_add_event(action[0], touche)


func _creer_murs() -> void:
	for rect in [Rect2(-20, -20, 752, 20), Rect2(-20, 648, 752, 20), Rect2(-20, 0, 20, 648), Rect2(712, 0, 20, 648), Rect2(310, 90, 100, 70)]:
		var mur := StaticBody2D.new()
		var forme := CollisionShape2D.new()
		var rectangle := RectangleShape2D.new()
		rectangle.size = rect.size
		forme.shape = rectangle
		mur.position = rect.get_center()
		mur.add_child(forme)
		add_child(mur)


func _recharge(id: StringName) -> String:
	var reste := capacites.get_cooldown_remaining(id)
	return "prêt" if reste <= 0.0 else "%.1f s" % reste


func _lancer_tests() -> void:
	for titre in SUITES:
		await panneau.lancer_tests(titre, SUITES[titre], self)
