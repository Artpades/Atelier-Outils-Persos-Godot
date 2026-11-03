extends Control

## 🏆 Démo du module de succès : un AchievementManager et dix succès configurés dans la scène (inspecteur).
##
## - Au lancement, exécute la suite de tests automatiques du module (suite_tests_succes.gd) ; ses gestionnaires
##   de test sont indépendants de celui de la démo.
## - Les boutons de gauche simulent le jeu : ils n'appellent que Gestionnaire.notify(évènement, id, quantité),
##   sans connaître les succès. Chaque condition décide si l'évènement la concerne.
## - Succès inspirés de Terraria / tModLoader : drapeau, compteurs (cumul, record), collections, « l'un ou
##   l'autre » (COMPLETION_ANY), succès secret à condition GDScript, et méta-succès (débloquer d'autres succès).
## - La progression est sauvegardée dans user://demo_succes.cfg (auto_save / auto_load) : elle est retrouvée
##   au lancement suivant. Le panneau permet de sauvegarder, recharger et tout réinitialiser.

const Panneau := preload("../panneau_demo.gd")
const SuiteTests := preload("suite_tests_succes.gd")
const CATEGORIES := ["Général", "⚔️ Combat", "💰 Objets", "🧭 Exploration", "🏁 Défi"]
const ENNEMIS := ["slime", "zombie", "squelette"]
const GEMMES := ["rubis", "saphir", "émeraude", "topaze", "améthyste"]
const BIOMES := ["forêt", "désert", "toundra", "jungle"]
const POSITIONS := ["Haut gauche", "Haut centre", "Haut droite", "Bas gauche", "Bas centre", "Bas droite"]

@onready var panneau: Panneau = $Panneau
@onready var gestionnaire: AchievementManager = $Gestionnaire
@onready var zone: VBoxContainer = %Zone

var _bilan: Label
var _barre_bilan: ProgressBar
var _cartes: GridContainer
var _rafraichissement_prevu := false


func _ready() -> void:
	gestionnaire.achievement_unlocked.connect(func(succes: Achievement):
		panneau.journaliser("[color=gold]🏆 achievement_unlocked : %s[/color]" % succes.display_name)
		_rafraichir())
	gestionnaire.achievement_progressed.connect(func(_succes: Achievement, _progres: float): _rafraichir())
	gestionnaire.event_notified.connect(func(evenement: StringName, id: String, quantite: float):
		panneau.journaliser("[color=gray]📣 notify(%s, \"%s\", %s)[/color]" % [evenement, id, quantite]))

	_construire_zone_de_jeu()
	_construire_panneau()
	_rafraichir()
	_lancer_tests()


# 🎮 Zone de jeu : évènements simulés et liste des succès

func _construire_zone_de_jeu() -> void:
	_bilan = Label.new()
	_bilan.add_theme_font_size_override("font_size", 20)
	zone.add_child(_bilan)
	_barre_bilan = ProgressBar.new()
	_barre_bilan.show_percentage = false
	_barre_bilan.custom_minimum_size.y = 8
	zone.add_child(_barre_bilan)

	var actions := HFlowContainer.new()
	actions.add_theme_constant_override("h_separation", 6)
	actions.add_theme_constant_override("v_separation", 6)
	zone.add_child(actions)
	for ennemi in ENNEMIS:
		_action(actions, "⚔️ Vaincre un %s" % ennemi, &"enemy_killed", ennemi)
	_action(actions, "👑 Vaincre le boss", &"boss_defeated")
	_action(actions, "🏃 Fuir le boss", &"boss_fled")
	_action(actions, "🪙 +1 pièce", &"coin_collected", "", 1)
	_action(actions, "💰 +25 pièces", &"coin_collected", "", 25)
	_bouton(actions, "💎 Gemme au hasard", func(): gestionnaire.notify(&"gem_found", GEMMES.pick_random()))
	for biome in BIOMES:
		_action(actions, "🧭 %s" % biome.capitalize(), &"biome_visited", biome)
	_bouton(actions, "🎲 Partie (score au hasard)", func(): gestionnaire.notify(&"score", "", randi_range(100, 1200)))
	_action(actions, "👆 Clic frénétique !", &"clic_frenetique")

	var defilement := ScrollContainer.new()
	defilement.size_flags_vertical = Control.SIZE_EXPAND_FILL
	defilement.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	zone.add_child(defilement)
	_cartes = GridContainer.new()
	_cartes.columns = 2
	_cartes.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_cartes.add_theme_constant_override("h_separation", 8)
	_cartes.add_theme_constant_override("v_separation", 8)
	defilement.add_child(_cartes)


func _action(parent: Control, texte: String, evenement: StringName, id: String = "", quantite: float = 1.0) -> void:
	_bouton(parent, texte, func(): gestionnaire.notify(evenement, id, quantite))


func _bouton(parent: Control, texte: String, action: Callable) -> void:
	var bouton := Button.new()
	bouton.text = texte
	bouton.focus_mode = Control.FOCUS_NONE
	bouton.pressed.connect(action)
	parent.add_child(bouton)


## Reconstruit les cartes à la fin de la frame (plusieurs signaux d'un même notify n'en font qu'une).
func _rafraichir() -> void:
	if _rafraichissement_prevu:
		return
	_rafraichissement_prevu = true
	_reconstruire_cartes.call_deferred()


func _reconstruire_cartes() -> void:
	_rafraichissement_prevu = false
	for carte in _cartes.get_children():
		carte.queue_free()
	for succes: Achievement in gestionnaire.achievements:
		_cartes.add_child(_carte(succes))

	_bilan.text = "🏆 Succès : %d / %d (%d %%)" % [gestionnaire.get_unlocked_count(), gestionnaire.get_achievement_count(),
		roundi(gestionnaire.get_completion_ratio() * 100.0)]
	_barre_bilan.value = gestionnaire.get_completion_ratio() * 100.0


func _carte(succes: Achievement) -> Control:
	var secret := succes.is_hidden() and not succes.is_unlocked()
	var carte := PanelContainer.new()
	carte.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.16, 0.15, 0.12) if succes.is_unlocked() else Color(0.13, 0.13, 0.16)
	style.border_color = Color(0.95, 0.75, 0.25) if succes.is_unlocked() else Color(0.3, 0.3, 0.35)
	style.set_border_width_all(2 if succes.is_unlocked() else 1)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(8)
	carte.add_theme_stylebox_override("panel", style)

	var ligne := HBoxContainer.new()
	ligne.add_theme_constant_override("separation", 10)
	carte.add_child(ligne)
	var icone := TextureRect.new()
	icone.texture = succes.get_current_icon()
	icone.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	icone.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	icone.custom_minimum_size = Vector2(44, 44)
	icone.size_flags_vertical = Control.SIZE_SHRINK_BEGIN
	ligne.add_child(icone)

	var textes := VBoxContainer.new()
	textes.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	textes.add_theme_constant_override("separation", 2)
	ligne.add_child(textes)
	var nom := Label.new()
	nom.text = "???" if secret else succes.display_name
	nom.add_theme_font_size_override("font_size", 16)
	textes.add_child(nom)
	var description := Label.new()
	description.text = "Succès secret." if secret else succes.description
	description.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	description.add_theme_font_size_override("font_size", 12)
	description.modulate.a = 0.75
	textes.add_child(description)

	if succes.has_progress() and not secret:
		var progression := HBoxContainer.new()
		var barre := ProgressBar.new()
		barre.show_percentage = false
		barre.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		barre.size_flags_vertical = Control.SIZE_SHRINK_CENTER
		barre.custom_minimum_size.y = 8
		barre.value = succes.get_progress() * 100.0
		progression.add_child(barre)
		var texte := Label.new()
		texte.text = succes.get_progress_text()
		texte.add_theme_font_size_override("font_size", 12)
		progression.add_child(texte)
		textes.add_child(progression)

	var pied := Label.new()
	pied.text = CATEGORIES[succes.category]
	if succes.is_unlocked():
		var local: int = int(succes.get_unlock_time()) + Time.get_time_zone_from_system().bias * 60
		pied.text += "  ·  débloqué à %s" % Time.get_time_string_from_unix_time(local)
	pied.add_theme_font_size_override("font_size", 11)
	pied.modulate.a = 0.55
	textes.add_child(pied)
	return carte


# 🎛️ Panneau

func _construire_panneau() -> void:
	panneau.ajouter_section("🔔 Notifications")
	panneau.ajouter_case("Afficher les notifications", gestionnaire.show_notifications, func(actif: bool): gestionnaire.show_notifications = actif)
	panneau.ajouter_options("Position", POSITIONS, gestionnaire.notification_position,
		func(index: int): gestionnaire.notification_position = index as AchievementManager.NotificationPosition)
	panneau.ajouter_curseur("Durée (s)", 0.5, 8.0, 0.5, gestionnaire.notification_duration, func(valeur: float): gestionnaire.notification_duration = valeur)

	panneau.ajouter_section("💾 Sauvegarde (%s)" % gestionnaire.save_path)
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Sauvegarder", func(): panneau.journaliser("💾 save_achievements() → %s" % error_string(gestionnaire.save_achievements())), rangee)
	panneau.ajouter_bouton("Recharger", func():
		panneau.journaliser("📂 load_achievements() → %s" % error_string(gestionnaire.load_achievements()))
		_rafraichir(), rangee)
	rangee = panneau.ajouter_rangee()
	panneau.ajouter_bouton("Tout réinitialiser", func():
		gestionnaire.reset_all()
		panneau.journaliser("🧹 reset_all()"), rangee)
	panneau.ajouter_bouton("Relancer les tests", _lancer_tests, rangee)


func _lancer_tests() -> void:
	await panneau.lancer_tests("Tests du module de succès", SuiteTests, self)
