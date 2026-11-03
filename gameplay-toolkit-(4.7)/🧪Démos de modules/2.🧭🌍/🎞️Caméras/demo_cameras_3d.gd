extends Node3D

## 🎞️ Démo du module caméras (3D) : CameraManager et OrbitalCamera.
##
## - Au lancement, exécute la suite de tests automatiques du module (suite_tests_cameras.gd).
## - Partie interactive : basculer entre quatre caméras, instantanément ou par transition (durée, courbe
##   et accélération réglables, interruption possible en cliquant pendant une transition).
##   La caméra orbitale tourne avec le clic droit maintenu.

const Panneau := preload("../panneau_demo.gd")
const SuiteTests := preload("suite_tests_cameras.gd")
const CIBLE := Vector3(0, 0.5, 0)

@onready var panneau: Panneau = $Interface/Panneau
@onready var orbitale: OrbitalCamera = $Orbitale
@onready var cameras: Array[Camera3D] = [$CamFace, $CamDessus, $CamCote, $Orbitale/Bras/CamOrbitale]

var _gestionnaire: CameraManager
var _fluide := true
var _duree := 1.0
var _transition := Tween.TRANS_CUBIC
var _acceleration := Tween.EASE_IN_OUT
var _etat: Label


func _ready() -> void:
	for camera in [$CamFace, $CamDessus, $CamCote]:
		camera.look_at(CIBLE)

	_gestionnaire = CameraManager.get_instance()
	_gestionnaire.current_camera_3d_switched.connect(_on_camera_basculee)

	panneau.ajouter_section("🎥 Caméras")
	for camera in cameras:
		panneau.ajouter_bouton(camera.name, _basculer.bind(camera))
	_etat = panneau.ajouter_etiquette()

	panneau.ajouter_section("〰️ Transition")
	panneau.ajouter_case("Transition fluide", _fluide, func(actif: bool): _fluide = actif)
	panneau.ajouter_curseur("Durée (s)", 0.1, 3.0, 0.1, _duree, func(valeur: float): _duree = valeur)
	panneau.ajouter_options("Courbe", _noms_enum("TransitionType"), _transition, func(index: int): _transition = index as Tween.TransitionType)
	panneau.ajouter_options("Accélération", _noms_enum("EaseType"), _acceleration, func(index: int): _acceleration = index as Tween.EaseType)

	panneau.ajouter_section("🛰️ Caméra orbitale")
	panneau.ajouter_case("Lissage", orbitale.use_smoothing, func(actif: bool): orbitale.use_smoothing = actif)
	panneau.ajouter_curseur("Sensibilité", 0.001, 0.02, 0.001, orbitale.sensitivity, func(valeur: float): orbitale.sensitivity = valeur)

	panneau.ajouter_section("🧪 Tests")
	panneau.ajouter_bouton("Relancer les tests automatiques", _lancer_tests)
	_lancer_tests()


func _process(_delta: float) -> void:
	var courante := get_viewport().get_camera_3d()
	_etat.text = "Caméra courante : %s%s" % [
		courante.name if courante else "aucune",
		"  (transition en cours)" if _gestionnaire.is_transitioning_3d() else ""]


func _unhandled_input(event: InputEvent) -> void:
	# Clic droit maintenu : orbiter
	if event is InputEventMouseMotion and Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
		orbitale.look_around(event.relative)


func _basculer(camera: Camera3D) -> void:
	_gestionnaire.switch_3d_camera(camera, _fluide, _duree, _transition, _acceleration)


func _on_camera_basculee(camera: Camera3D) -> void:
	panneau.journaliser("📡 current_camera_3d_switched → [b]%s[/b]" % camera.name)


func _lancer_tests() -> void:
	panneau.lancer_tests("Tests du module caméras", SuiteTests, self)


func _noms_enum(nom: String) -> PackedStringArray:
	return ClassDB.class_get_enum_constants("Tween", nom)
