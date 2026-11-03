extends Node2D

## 🕹️ Démo du module Entrées de périphérique.
##
## Le PlayerInput « Entrees » (contextes « vaisseau » et « pause ») pilote le vaisseau uniquement par ses
## liaisons (InputBinding) vers les méthodes ci-dessous (piloter, tirer, charger, rafale, regler_gaz, viser,
## ouvrir_pause, fermer_pause). Les jauges en bas à gauche montrent la valeur de chaque action (bouton, axe,
## cercle, sphère, souris, gyroscope) ; le journal du panneau liste les évènements reçus.

const SuiteTests := preload("tests/suite_tests_entrees.gd")
const ZONE := Rect2(0, 0, 740, 640)

@onready var entrees: PlayerInput = $Entrees
@onready var panneau = $Interface/Panneau
@onready var pause: Control = $Interface/Pause
@onready var controles: PlayerInputUI = $Interface/Pause/Boite/Controles

var position_vaisseau := Vector2(370, 230)
var direction := Vector2.ZERO
var gaz := 0.0
var angle_tourelle := 0.0
var inclinaison := Vector3.ZERO
var tirs: Array[Dictionary] = []
var _etat: Label
var _fin_reassignation := 0


func _ready() -> void:
	panneau.ajouter_section("🎮 Périphérique")
	_etat = panneau.ajouter_etiquette()
	var rangee: HBoxContainer = panneau.ajouter_rangee()
	panneau.ajouter_bouton("📳 Vibrer", func(): entrees.start_vibration(0.5, 1.0, 0.4), rangee)
	panneau.ajouter_bouton("⏸ Pause", ouvrir_pause, rangee)
	panneau.ajouter_options("Filtre", PackedStringArray(["Tous", "Clavier et souris", "Manette"]), 0, func(i: int): entrees.device_filter = i + 1)
	panneau.ajouter_case("Bascule automatique du périphérique", true, func(v: bool): entrees.auto_switch_device = v)
	panneau.ajouter_bouton("🧪 Lancer les tests du module", func(): panneau.lancer_tests("Entrées de périphérique", SuiteTests, self))
	entrees.action_triggered.connect(_journaliser)
	entrees.device_type_changed.connect(func(type: int): panneau.journaliser("[color=cyan]🎮 périphérique : %s[/color]" % ("manette" if type == InputAction.DEVICE_GAMEPAD else "clavier / souris")))
	entrees.context_changed.connect(func(nom: StringName, actif: bool): panneau.journaliser("[color=violet]🗂️ « %s » %s[/color]" % [nom, "actif" if actif else "inactif"]))
	var rebinder: InputRebinder = controles.get_active_rebinder()
	rebinder.rebind_completed.connect(func(_a, _s): _fin_reassignation = Time.get_ticks_msec())
	rebinder.rebind_cancelled.connect(func(_a, _s): _fin_reassignation = Time.get_ticks_msec())


func _journaliser(action: StringName, evenement: StringName, arguments: Array) -> void:
	# Les évènements continus (vitesse, orientation, souris) noieraient le journal.
	if evenement in [&"velocity_changed", &"orientation_changed", &"motion", &"position_changed", &"gyroscope_changed"]:
		return
	var valeur := "" if arguments.is_empty() else " (%s)" % ", ".join(arguments.map(func(a): return str(a)))
	panneau.journaliser("[b]%s[/b] · %s%s" % [action, evenement, valeur])


func _process(delta: float) -> void:
	position_vaisseau += direction * (140.0 + 220.0 * gaz) * delta
	position_vaisseau = position_vaisseau.clamp(ZONE.position + Vector2(20, 20), ZONE.end - Vector2(20, 140))
	for tir in tirs:
		tir.position += tir.vitesse * delta
		tir.vie -= delta
	tirs = tirs.filter(func(t): return t.vie > 0.0)
	_etat.text = "Dernier périphérique : %s\nManette %d : %s\nContextes actifs : %s" % [
		"manette" if entrees.get_last_device_type() == InputAction.DEVICE_GAMEPAD else "clavier / souris",
		entrees.device, entrees.get_joy_name() if entrees.is_device_connected() else "(non connectée)",
		", ".join(entrees.get_active_contexts())]
	queue_redraw()


# 🔗 Méthodes appelées par les liaisons

func piloter(valeur: Vector2) -> void:
	direction = valeur


func regler_gaz(valeur: float) -> void:
	gaz = valeur


func viser(mouvement: Vector2) -> void:
	angle_tourelle += mouvement.x


func tirer() -> void:
	_tir(1.0, 420.0)


func charger() -> void:
	_tir(3.0, 260.0)
	entrees.start_vibration(0.2, 0.6, 0.15)


func rafale(_nombre: int) -> void:
	for i in 5:
		_tir(0.8, 380.0, (i - 2) * 0.15)
	panneau.journaliser("[color=orange]💥 rafale ![/color]")


func _tir(taille: float, vitesse: float, decalage := 0.0) -> void:
	var sens := Vector2.RIGHT.rotated(angle_tourelle + decalage)
	tirs.append({"position": position_vaisseau + sens * 24, "vitesse": sens * vitesse, "taille": taille, "vie": 2.0})


func ouvrir_pause() -> void:
	if pause.visible:
		return
	pause.visible = true
	entrees.push_context(&"pause")
	controles.refresh()


func fermer_pause() -> void:
	# Échap annule aussi une réassignation en cours : ne pas reprendre dans la foulée.
	if controles.get_active_rebinder().is_listening() or Time.get_ticks_msec() - _fin_reassignation < 400:
		return
	pause.visible = false
	entrees.pop_context()


# 🎨 Dessin : vaisseau et jauges des actions

func _draw() -> void:
	draw_rect(ZONE, Color(0.1, 0.11, 0.16))
	for tir in tirs:
		draw_circle(tir.position, 3.0 * tir.taille, Color(1.0, 0.85, 0.35))
	var avant := direction if direction != Vector2.ZERO else Vector2.UP
	var corps := PackedVector2Array([avant * 18, avant.rotated(2.5) * 12, avant.rotated(-2.5) * 12])
	var decale := PackedVector2Array()
	for point in corps:
		decale.append(position_vaisseau + point.rotated(inclinaison.z * 0.3))
	draw_colored_polygon(decale, Color(0.4, 0.75, 1.0))
	draw_line(position_vaisseau, position_vaisseau + Vector2.RIGHT.rotated(angle_tourelle) * 24, Color.WHITE, 3.0)
	draw_rect(Rect2(position_vaisseau + Vector2(-20, 22), Vector2(40 * gaz, 4)) if gaz >= 0 else Rect2(position_vaisseau + Vector2(-20 + 40 * gaz, 22), Vector2(-40 * gaz, 4)), Color(0.4, 1.0, 0.5))

	# Jauges : une par action du contexte « vaisseau ».
	var contexte := entrees.get_context(&"vaisseau")
	if contexte == null:
		return
	var police := ThemeDB.fallback_font
	var x := 10.0
	for action: InputAction in contexte.actions:
		var centre := Vector2(x + 45, ZONE.end.y - 70)
		_jauge(action, centre)
		draw_string(police, Vector2(x, ZONE.end.y - 12), action.action_name, HORIZONTAL_ALIGNMENT_CENTER, 90, 12)
		draw_string(police, Vector2(x, ZONE.end.y + 4), action.get_value_text(), HORIZONTAL_ALIGNMENT_CENTER, 90, 10, Color(1, 1, 1, 0.6))
		x += 100.0


func _jauge(action: InputAction, centre: Vector2) -> void:
	var valeur: Variant = action.get_value()
	var cadre := Color(0.55, 0.6, 0.7)
	var vert := Color(0.35, 0.9, 0.5)
	match typeof(valeur):
		TYPE_BOOL:
			draw_circle(centre, 16, vert if valeur else Color(0.25, 0.27, 0.3))
		TYPE_FLOAT:
			draw_rect(Rect2(centre - Vector2(40, 6), Vector2(80, 12)), Color(0.2, 0.22, 0.25))
			draw_rect(Rect2(centre - Vector2(0 if valeur >= 0 else -40 * valeur, 6), Vector2(40 * absf(valeur), 12)), vert)
		TYPE_VECTOR2, TYPE_VECTOR3:
			var plat: Vector2 = valeur if valeur is Vector2 else Vector2(valeur.x, valeur.z)
			if action is InputMouseAction:
				plat = (plat * 20.0).limit_length(1.0)
			draw_arc(centre, 30, 0, TAU, 32, cadre, 1.5)
			draw_circle(centre + plat.limit_length(1.0) * 30, 4, vert)
			if valeur is Vector3:
				draw_rect(Rect2(centre + Vector2(36, -30), Vector2(4, 60)), Color(0.2, 0.22, 0.25))
				draw_rect(Rect2(centre + Vector2(36, 0), Vector2(4, -30 * clampf(valeur.y, -1, 1))), Color(0.95, 0.75, 0.3))
