extends Node3D

## 🌀 Démo Mouvement & Physique 3D, pilotée par le module Entrées de périphérique.
##
## Même principe que la démo 2D : le héros (CharacterBody3D) porte tous les composants de mouvement 3D (sol,
## mur, nage, plané / vol, corde, recul) et un MovementDirector qui choisit le contexte selon l'environnement.
## Le PlayerInput « Entrees » n'a aucun code : ses liaisons appellent le MovementDirector, et les méthodes
## de cette démo pour la caméra (orbiter, tourner_camera), le vol et le menu des contrôles.
## Les déplacements sont relatifs à la caméra (camera_relative des composants).
##
## Le niveau (construit ici) : cheminée à sauts muraux, fosse remplie d'eau, courant ascendant (ForceZone3D)
## vers une plateforme haute, corde, pics qui repoussent (Knockback3D), caisses et feuille (RigidBody3D).

const SuiteTests := preload("suite_tests_mouvement.gd")
const VITESSE_CAMERA := 2.5 # rad/s au stick
const DEPART := Vector3(0, 1.5, 6)

@onready var heros: CharacterBody3D = $Heros
@onready var directeur: Node = $Heros/Directeur
@onready var entrees: PlayerInput = $Entrees
@onready var pivot: Node3D = $Pivot
@onready var panneau = $Interface/Panneau
@onready var menu: Control = $Interface/Menu
@onready var controles: PlayerInputUI = $Interface/Menu/Controles

var lacet := 0.0
var tangage := -0.35
var rotation_camera := Vector2.ZERO
var _etat: Label
var _fin_reassignation := 0
var _pics: Area3D


func _ready() -> void:
	_construire_niveau()
	panneau.ajouter_section("🎬 Contexte de mouvement")
	_etat = panneau.ajouter_etiquette()
	var rangee: HBoxContainer = panneau.ajouter_rangee()
	panneau.ajouter_bouton("💥 Recul", func(): $Heros/Recul.apply_knockback(-heros.global_basis.z + Vector3.UP * 0.5), rangee)
	panneau.ajouter_bouton("🫨 Secouer", func(): $Heros/Antenne.impulse(Vector3(6, 2, 0)), rangee)
	panneau.ajouter_bouton("📳 Vibrer", func(): entrees.start_vibration(0.6, 0.8, 0.3), rangee)
	panneau.ajouter_bouton("⚙️ Contrôles", ouvrir_menu)
	panneau.ajouter_bouton("🧪 Lancer les tests du module", func(): panneau.lancer_tests("Mouvement & Physique", SuiteTests, self))

	directeur.context_changed.connect(func(_ancien: StringName, nom: StringName): panneau.journaliser("🎬 contexte : [b]%s[/b]" % nom))
	entrees.device_type_changed.connect(func(type: int): panneau.journaliser("🎮 périphérique : %s" % ("manette" if type == InputAction.DEVICE_GAMEPAD else "clavier / souris")))
	entrees.binding_failed.connect(func(action, _liaison, raison): panneau.journaliser("[color=orange]⚠️ %s : %s[/color]" % [action, raison]))
	var rebinder: InputRebinder = controles.get_active_rebinder()
	rebinder.rebind_completed.connect(func(_a, _s): _fin_reassignation = Time.get_ticks_msec())
	rebinder.rebind_cancelled.connect(func(_a, _s): _fin_reassignation = Time.get_ticks_msec())
	$Heros/Sol.jumped.connect(func(): $Heros/Antenne.impulse(Vector3(0, -3, 0)))
	$Heros/Sol.landed.connect(func(): $Heros/Antenne.impulse(Vector3(0, 4, 0)))
	$Heros/Mur.wall_jumped.connect(func(): panneau.journaliser("🧗 saut mural"))
	_capturer_souris(true)
	_placer_camera(1.0)


func _physics_process(delta: float) -> void:
	if heros.global_position.y < -20.0:
		heros.global_position = DEPART
		heros.velocity = Vector3.ZERO
	lacet -= rotation_camera.x * VITESSE_CAMERA * delta
	tangage = clampf(tangage - rotation_camera.y * VITESSE_CAMERA * delta, -1.3, 0.6)
	_placer_camera(minf(1.0, 8.0 * delta))
	_etat.text = "Contexte : %s%s\nEntrée : %s · vitesse : %.1f m/s\nContextes d'entrées actifs : %s" % [
		directeur.get_current_context(), " (forcé)" if directeur.get_forced_context() != &"" else "",
		entrees.get_vector("deplacement"), heros.velocity.length(), ", ".join(entrees.get_active_contexts())]


func _placer_camera(poids: float) -> void:
	pivot.global_position = pivot.global_position.lerp(heros.global_position + Vector3.UP, poids)
	pivot.rotation = Vector3(tangage, lacet, 0)


func _capturer_souris(capturer: bool) -> void:
	if DisplayServer.get_name() != "headless":
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED if capturer else Input.MOUSE_MODE_VISIBLE


# 🔗 Méthodes appelées par les liaisons du PlayerInput

func orbiter(mouvement: Vector2) -> void:
	if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		lacet -= mouvement.x
		tangage = clampf(tangage - mouvement.y, -1.3, 0.6)


func tourner_camera(valeur: Vector2) -> void:
	rotation_camera = valeur


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
		panneau.journaliser("🪂 vol libre : saut / F pour monter, C pour descendre, V pour atterrir")


func ouvrir_menu() -> void:
	if menu.visible:
		return
	menu.visible = true
	entrees.push_context(&"menu")
	controles.refresh()
	_capturer_souris(false)


func fermer_menu() -> void:
	# Échap annule aussi une réassignation : on ne ferme pas le menu dans la foulée.
	if controles.get_active_rebinder().is_listening() or Time.get_ticks_msec() - _fin_reassignation < 400:
		return
	menu.visible = false
	entrees.pop_context()
	_capturer_souris(true)


# 🏗️ Niveau

func _materiau(couleur: Color) -> StandardMaterial3D:
	var materiau := StandardMaterial3D.new()
	materiau.albedo_color = couleur
	if couleur.a < 1.0:
		materiau.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	return materiau


func _maillage(taille: Vector3, couleur: Color) -> MeshInstance3D:
	var maillage := MeshInstance3D.new()
	var boite := BoxMesh.new()
	boite.size = taille
	maillage.mesh = boite
	maillage.material_override = _materiau(couleur)
	return maillage


func _forme(taille: Vector3) -> CollisionShape3D:
	var forme := CollisionShape3D.new()
	var boite := BoxShape3D.new()
	boite.size = taille
	forme.shape = boite
	return forme


func _bloc(centre: Vector3, taille: Vector3, couleur := Color(0.32, 0.35, 0.42)) -> StaticBody3D:
	var corps := StaticBody3D.new()
	corps.position = centre
	corps.add_child(_forme(taille))
	corps.add_child(_maillage(taille, couleur))
	add_child(corps)
	return corps


func _zone(zone: Area3D, centre: Vector3, taille: Vector3, couleur: Color) -> Area3D:
	zone.position = centre
	zone.add_child(_forme(taille))
	zone.add_child(_maillage(taille, couleur))
	add_child(zone)
	return zone


func _etiquette(texte: String, position_texte: Vector3) -> void:
	var etiquette := Label3D.new()
	etiquette.text = texte
	etiquette.font_size = 96
	etiquette.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	etiquette.position = position_texte
	add_child(etiquette)


func _construire_niveau() -> void:
	# 🌤️ Lumière et ciel
	var soleil := DirectionalLight3D.new()
	soleil.rotation = Vector3(-0.9, 0.6, 0)
	soleil.shadow_enabled = true
	add_child(soleil)
	var environnement := WorldEnvironment.new()
	environnement.environment = Environment.new()
	environnement.environment.background_mode = Environment.BG_SKY
	environnement.environment.sky = Sky.new()
	environnement.environment.sky.sky_material = ProceduralSkyMaterial.new()
	environnement.environment.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	add_child(environnement)

	# Sol (épais), avec une fosse en x 5..11, z -3..3.
	_bloc(Vector3(-12.5, -2, 0), Vector3(35, 4, 60))
	_bloc(Vector3(20.5, -2, 0), Vector3(19, 4, 60))
	_bloc(Vector3(8, -2, -16.5), Vector3(6, 4, 27))
	_bloc(Vector3(8, -2, 16.5), Vector3(6, 4, 27))
	_bloc(Vector3(8, -3.5, 0), Vector3(6, 1, 6))
	# 🧗 Cheminée (deux murs à 1,4 m) et corniche à 8 m.
	_bloc(Vector3(-8, 4, 0), Vector3(0.6, 8, 4))
	_bloc(Vector3(-6, 4, 0), Vector3(0.6, 8, 4))
	_bloc(Vector3(-10, 7.8, 0), Vector3(3, 0.4, 4))
	_etiquette("🧗 cheminée", Vector3(-7, 9, 0))
	# 🌊 Fosse remplie d'eau
	var eau: Area3D = _zone(WaterArea3D.new(), Vector3(8, -1.6, 0), Vector3(6, 2.8, 6), Color(0.2, 0.45, 0.9, 0.45))
	eau.get_child(1).cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	_etiquette("🌊 fosse", Vector3(8, 1.5, 0))
	# 🧲 Courant ascendant, puis plateforme haute
	var vent := _zone(ForceZone3D.new(), Vector3(16, 4, -8), Vector3(3, 8, 3), Color(0.8, 0.9, 1.0, 0.12)) as ForceZone3D
	vent.direction = Vector3.UP
	vent.strength = 26.0
	vent.turbulence = 0.3
	vent.get_child(1).cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	_bloc(Vector3(16, 7.5, -13), Vector3(5, 0.4, 5))
	_etiquette("🧲 courant ascendant (saut maintenu : plané)", Vector3(16, 9, -8))
	# 🪢 Corde
	_bloc(Vector3(16, 9.2, 8), Vector3(1.5, 0.3, 1.5), Color(0.5, 0.35, 0.2)) # potence
	var corde := Rope3D.new()
	corde.position = Vector3(16, 9, 8)
	corde.rope_length = 7.5
	corde.collide = false
	add_child(corde)
	_etiquette("🪢 corde (E / Y)", Vector3(16, 10, 8))
	# 🌬️ Pics
	_pics = _zone(Area3D.new(), Vector3(22, 0.25, 0), Vector3(2, 0.5, 2), Color(0.85, 0.3, 0.3))
	_pics.body_entered.connect(func(corps: Node3D):
		if corps == heros:
			$Heros/Recul.apply_knockback_from(_pics.global_position - Vector3(0, 0.5, 0))
			entrees.start_vibration(0.3, 0.9, 0.25)
			panneau.journaliser("🌬️ aïe, des pics !"))
	_etiquette("🌬️ pics", Vector3(22, 1.5, 0))
	# 📦 Caisses (eau, vent) et feuille (aérodynamique passive)
	for position_caisse in [Vector3(7, 1, -1), Vector3(9, 1, 1), Vector3(16, 1, -8)]:
		_caisse(position_caisse, Vector3.ONE * 0.8, Color(0.75, 0.55, 0.3))
	var feuille := _caisse(Vector3(12, 8, -6), Vector3(0.8, 0.05, 0.6), Color(0.4, 0.85, 0.35))
	feuille.mass = 0.05
	var aero := AerodynamicMovement3D.new()
	aero.flutter_frequency = 1.5
	feuille.add_child(aero)


func _caisse(centre: Vector3, taille: Vector3, couleur: Color) -> RigidBody3D:
	var caisse := RigidBody3D.new()
	caisse.position = centre
	caisse.add_child(_forme(taille))
	caisse.add_child(_maillage(taille, couleur))
	add_child(caisse)
	return caisse
