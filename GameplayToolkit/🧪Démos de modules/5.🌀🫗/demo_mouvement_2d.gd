extends Node2D

## 🌀 Démo Mouvement & Physique 2D, pilotée par le module Entrées de périphérique.
##
## Le héros (CharacterBody2D) porte tous les composants de mouvement : sol, mur, nage, plané / vol, corde,
## recul, et un MovementDirector qui choisit le contexte (sol, air, eau, mur, corde...) selon l'environnement.
## Le PlayerInput « Entrees » (contextes « jeu » et « menu ») n'a aucun code : ses actions sont reliées au
## MovementDirector par des liaisons (sélectionner Entrees → inspecteur → 🕹️ Éditeur d'entrées).
##
## Le niveau (construit ici) : cheminée à sauts muraux, bassin, courant ascendant (ForceZone2D), plateforme
## haute, corde au-dessus d'une fosse, pics qui repoussent (Knockback2D), caisses et feuille (RigidBody2D
## soumis au vent, à l'eau et à l'aérodynamique).

const SuiteTests := preload("suite_tests_mouvement.gd")

const COULEUR_SOL := Color(0.3, 0.33, 0.4)
const COULEUR_EAU := Color(0.2, 0.45, 0.9, 0.45)
const COULEUR_VENT := Color(0.75, 0.9, 1.0, 0.12)

@onready var heros: CharacterBody2D = $Heros
@onready var directeur: Node = $Heros/Directeur
@onready var entrees: PlayerInput = $Entrees
@onready var panneau = $Interface/Panneau
@onready var menu: Control = $Interface/Menu
@onready var controles: PlayerInputUI = $Interface/Menu/Controles

var _blocs: Array[Rect2] = []
var _eau: Rect2
var _vent: Rect2
var _pics: Rect2
var _etat: Label
var _fin_reassignation := 0
var _case_debug: CheckBox


func _ready() -> void:
	_construire_niveau()
	panneau.ajouter_section("🎬 Contexte de mouvement")
	_etat = panneau.ajouter_etiquette()
	var rangee: HBoxContainer = panneau.ajouter_rangee()
	panneau.ajouter_bouton("💥 Recul", func(): $Heros/Recul.apply_knockback(Vector2(-1, -0.6)), rangee)
	panneau.ajouter_bouton("🫨 Secouer", func(): $Heros/Antenne.impulse(Vector2(400, -200)), rangee)
	panneau.ajouter_bouton("📳 Vibrer", func(): entrees.start_vibration(0.6, 0.8, 0.3), rangee)
	panneau.ajouter_bouton("⚙️ Contrôles", ouvrir_menu)
	_case_debug = panneau.ajouter_case("🐞 Gizmos de débogage (F1)", false, func(actif: bool): _appliquer_debug(actif))
	panneau.ajouter_bouton("🧪 Lancer les tests du module", func(): panneau.lancer_tests("Mouvement & Physique", SuiteTests, self))

	directeur.context_changed.connect(func(_ancien: StringName, nom: StringName): panneau.journaliser("🎬 contexte : [b]%s[/b]" % nom))
	entrees.device_type_changed.connect(func(type: int): panneau.journaliser("🎮 périphérique : %s" % ("manette" if type == InputAction.DEVICE_GAMEPAD else "clavier / souris")))
	entrees.context_changed.connect(func(nom: StringName, actif: bool): panneau.journaliser("🗂️ contexte d'entrées « %s » : %s" % [nom, "actif" if actif else "inactif"]))
	entrees.binding_failed.connect(func(action, _liaison, raison): panneau.journaliser("[color=orange]⚠️ %s : %s[/color]" % [action, raison]))
	var rebinder: InputRebinder = controles.get_active_rebinder()
	rebinder.rebind_completed.connect(func(_a, _s): _fin_reassignation = Time.get_ticks_msec())
	rebinder.rebind_cancelled.connect(func(_a, _s): _fin_reassignation = Time.get_ticks_msec())
	$Heros/Sol.jumped.connect(func(): $Heros/Antenne.impulse(Vector2(0, 250)))
	$Heros/Sol.landed.connect(func(): $Heros/Antenne.impulse(Vector2(0, -350)))
	$Heros/Mur.wall_jumped.connect(func(): panneau.journaliser("🧗 saut mural"))


func _physics_process(_delta: float) -> void:
	if heros.position.y > 1400:
		heros.position = Vector2(100, 600)
		heros.velocity = Vector2.ZERO
	_etat.text = "Contexte : %s%s\nEntrée : %s · vitesse : %d px/s\nContextes d'entrées actifs : %s" % [
		directeur.get_current_context(), " (forcé)" if directeur.get_forced_context() != &"" else "",
		entrees.get_vector("deplacement"), heros.velocity.length(), ", ".join(entrees.get_active_contexts())]
	queue_redraw()


# 🔗 Méthodes appelées par les liaisons du PlayerInput

## Gizmos de débogage : debug_mode de tous les nœuds du module Mouvement & Physique de la scène (composants,
## directeur, eau, zones de force, corde, trémoussement, recul).
func basculer_debug() -> void:
	_case_debug.button_pressed = not _case_debug.button_pressed


func _appliquer_debug(actif: bool) -> void:
	for noeud in find_children("*", "", true, false):
		if "debug_mode" in noeud:
			noeud.debug_mode = actif
	panneau.journaliser("🐞 gizmos %s" % ("affichés" if actif else "masqués"))


## Le composant aérodynamique plane (mode 0) par défaut ; le vol libre (mode 1) passe par un contexte
## « vol » MANUAL, forcé avec set_context puis rendu à l'automatique avec clear_forced_context.
func basculer_vol() -> void:
	var plane: Node = $Heros/Plane
	if directeur.get_forced_context() == &"vol":
		directeur.clear_forced_context()
		plane.mode = 0
		panneau.journaliser("🪂 vol terminé (contexte automatique)")
		return
	if directeur.get_context(&"vol") == null:
		var vol := MovementContext.new()
		vol.context_name = &"vol"
		vol.components = [NodePath("../Plane")]
		vol.condition = MovementContext.CONDITION_MANUAL
		vol.priority = 70
		var contextes: Array = directeur.contexts.duplicate()
		contextes.append(vol)
		directeur.contexts = contextes
	plane.mode = 1
	if directeur.set_context(&"vol"):
		panneau.journaliser("🪂 vol libre (contexte forcé) : haut / bas pour monter / descendre, V pour atterrir")


func ouvrir_menu() -> void:
	if menu.visible:
		return
	menu.visible = true
	entrees.push_context(&"menu")
	controles.refresh()


func fermer_menu() -> void:
	# Échap annule aussi une réassignation : on ne ferme pas le menu dans la foulée.
	if controles.get_active_rebinder().is_listening() or Time.get_ticks_msec() - _fin_reassignation < 400:
		return
	menu.visible = false
	entrees.pop_context()


# 🏗️ Niveau

func _bloc(rect: Rect2) -> void:
	var corps := StaticBody2D.new()
	corps.position = rect.get_center()
	var forme := CollisionShape2D.new()
	var rectangle := RectangleShape2D.new()
	rectangle.size = rect.size
	forme.shape = rectangle
	corps.add_child(forme)
	add_child(corps)
	_blocs.append(rect)


func _zone(zone: Area2D, rect: Rect2) -> Area2D:
	zone.position = rect.get_center()
	var forme := CollisionShape2D.new()
	var rectangle := RectangleShape2D.new()
	rectangle.size = rect.size
	forme.shape = rectangle
	zone.add_child(forme)
	add_child(zone)
	return zone


func _construire_niveau() -> void:
	# Sol, bassin, fosse sous la corde.
	_bloc(Rect2(-400, 640, 1000, 80))
	_bloc(Rect2(600, 800, 300, 40))
	_bloc(Rect2(900, 640, 600, 80))
	_bloc(Rect2(1500, 900, 300, 40))
	_bloc(Rect2(1800, 640, 700, 80))
	# Cheminée pour les sauts muraux, et sa corniche.
	_bloc(Rect2(-260, 160, 40, 480))
	_bloc(Rect2(-150, 260, 40, 380))
	_bloc(Rect2(-400, 140, 180, 20))
	# Plateforme haute (après le courant ascendant).
	_bloc(Rect2(1150, 260, 260, 20))
	# 🌊 Bassin
	_eau = Rect2(600, 660, 300, 140)
	var eau: Area2D = _zone(ClassDB.instantiate(&"WaterArea2D"), _eau)
	# 🧲 Courant ascendant
	_vent = Rect2(1000, 120, 120, 520)
	var vent: Area2D = _zone(ClassDB.instantiate(&"ForceZone2D"), _vent)
	vent.set("direction", Vector2(0, -1))
	vent.set("strength", 2600.0)
	vent.set("turbulence", 0.3)
	# 🪢 Corde au-dessus de la fosse
	var corde: Node2D = ClassDB.instantiate(&"Rope2D")
	corde.position = Vector2(1650, 260)
	corde.set("rope_length", 300.0)
	corde.set("collide", false)
	add_child(corde)
	# 🌬️ Pics : repoussent le héros
	_pics = Rect2(2000, 610, 80, 30)
	var pics := _zone(Area2D.new(), _pics)
	pics.body_entered.connect(func(corps: Node2D):
		if corps == heros:
			$Heros/Recul.apply_knockback_from(_pics.get_center() + Vector2(0, 30))
			entrees.start_vibration(0.3, 0.9, 0.25)
			panneau.journaliser("🌬️ aïe, des pics !"))
	# 📦 Objets physiques : caisses (eau, vent) et feuille (aérodynamique passive)
	for x in [700.0, 760.0, 1040.0]:
		_caisse(Vector2(x, 560), Color(0.75, 0.55, 0.3))
	var feuille := _caisse(Vector2(1300, 100), Color(0.4, 0.85, 0.35), Vector2(26, 6))
	feuille.mass = 0.05
	var aero: Node = ClassDB.instantiate(&"AerodynamicMovement2D")
	aero.set("flutter_frequency", 1.5)
	feuille.add_child(aero)


func _caisse(position_depart: Vector2, couleur: Color, taille := Vector2(28, 28)) -> RigidBody2D:
	var caisse := RigidBody2D.new()
	caisse.position = position_depart
	var forme := CollisionShape2D.new()
	var rectangle := RectangleShape2D.new()
	rectangle.size = taille
	forme.shape = rectangle
	caisse.add_child(forme)
	var visuel := Polygon2D.new()
	visuel.color = couleur
	visuel.polygon = PackedVector2Array([-taille / 2, Vector2(taille.x, -taille.y) / 2, taille / 2, Vector2(-taille.x, taille.y) / 2])
	caisse.add_child(visuel)
	add_child(caisse)
	return caisse


func _draw() -> void:
	draw_rect(_vent, COULEUR_VENT)
	for i in 6:
		var x := _vent.position.x + 12 + i * 20
		var y := _vent.end.y - fmod(Time.get_ticks_msec() * 0.25 + i * 90, _vent.size.y)
		draw_line(Vector2(x, y), Vector2(x, y - 24), Color(1, 1, 1, 0.35), 2)
	for bloc in _blocs:
		draw_rect(bloc, COULEUR_SOL)
	draw_rect(_eau, COULEUR_EAU)
	var pointes := PackedVector2Array()
	for i in 5:
		pointes.append_array([Vector2(_pics.position.x + i * 16, _pics.end.y), Vector2(_pics.position.x + i * 16 + 8, _pics.position.y), Vector2(_pics.position.x + i * 16 + 16, _pics.end.y)])
	for i in range(0, pointes.size(), 3):
		draw_colored_polygon(pointes.slice(i, i + 3), Color(0.85, 0.3, 0.3))
	var police := ThemeDB.fallback_font
	draw_string(police, Vector2(-250, 120), "🧗 cheminée", HORIZONTAL_ALIGNMENT_LEFT, -1, 14)
	draw_string(police, Vector2(640, 650), "🌊 bassin", HORIZONTAL_ALIGNMENT_LEFT, -1, 14)
	draw_string(police, Vector2(1000, 110), "🧲 courant ascendant", HORIZONTAL_ALIGNMENT_LEFT, -1, 14)
	draw_string(police, Vector2(1600, 240), "🪢 corde (E / Y)", HORIZONTAL_ALIGNMENT_LEFT, -1, 14)
	draw_string(police, Vector2(1990, 600), "🌬️ pics", HORIZONTAL_ALIGNMENT_LEFT, -1, 14)
